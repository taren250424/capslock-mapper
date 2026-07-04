// Resident mapper daemon: turns Caps Lock into the left mouse button.
//
// It grabs (EVIOCGRAB) every input device that has a Caps Lock key, then
// re-emits everything through two virtual uinput devices: Caps Lock events
// become BTN_LEFT on a virtual mouse, every other event is forwarded
// unchanged through a virtual keyboard. Requires root.
//
// NOTE: refactored on Windows without a Linux machine at hand. The logic is
// intentionally identical to v1; see packages/linux/TODO.md.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#define INPUT_PATH "/dev/input"
#define UINPUT_PATH "/dev/uinput"
#define MAX_DEVICES 32

typedef struct {
  int fd;
  char path[1024];
  char name[256];
} Device;

static volatile sig_atomic_t running = 1;

static void handle_signal(int sig) {
  (void)sig;
  running = 0;
}

static int setup_virtual_mouse(void) {
  int fd = open(UINPUT_PATH, O_WRONLY | O_NONBLOCK);
  if (fd < 0) {
    return -1;
  }

  ioctl(fd, UI_SET_EVBIT, EV_KEY);
  ioctl(fd, UI_SET_KEYBIT, BTN_LEFT);
  ioctl(fd, UI_SET_EVBIT, EV_SYN);

  struct uinput_setup usetup = {0};
  snprintf(usetup.name, UINPUT_MAX_NAME_SIZE, "CapsLock Virtual Mouse");
  usetup.id.bustype = BUS_USB;
  usetup.id.vendor = 0x1234;
  usetup.id.product = 0x5678;

  if (ioctl(fd, UI_DEV_SETUP, &usetup) < 0 || ioctl(fd, UI_DEV_CREATE) < 0) {
    close(fd);
    return -1;
  }
  return fd;
}

static int setup_virtual_keyboard(void) {
  int fd = open(UINPUT_PATH, O_WRONLY | O_NONBLOCK);
  if (fd < 0) {
    return -1;
  }

  ioctl(fd, UI_SET_EVBIT, EV_KEY);
  ioctl(fd, UI_SET_EVBIT, EV_SYN);
  ioctl(fd, UI_SET_EVBIT, EV_MSC);
  ioctl(fd, UI_SET_EVBIT, EV_REP);

  // The virtual keyboard must be able to re-emit any key the grabbed
  // physical keyboards can produce.
  for (int i = 0; i < KEY_MAX; i++) {
    ioctl(fd, UI_SET_KEYBIT, i);
  }
  ioctl(fd, UI_SET_MSCBIT, MSC_SCAN);

  struct uinput_setup usetup = {0};
  snprintf(usetup.name, UINPUT_MAX_NAME_SIZE, "CapsLock Virtual Keyboard");
  usetup.id.bustype = BUS_USB;
  usetup.id.vendor = 0x1234;
  usetup.id.product = 0x5679;

  if (ioctl(fd, UI_DEV_SETUP, &usetup) < 0 || ioctl(fd, UI_DEV_CREATE) < 0) {
    close(fd);
    return -1;
  }
  return fd;
}

// Writes one event to a uinput device. A failed write only drops that event;
// killing the daemon over it would be worse.
static void emit_event(int fd, unsigned short type, unsigned short code, int value) {
  struct input_event ev = {0};
  gettimeofday(&ev.time, NULL);
  ev.type = type;
  ev.code = code;
  ev.value = value;
  if (write(fd, &ev, sizeof(ev)) != sizeof(ev)) {
    // best effort
  }
}

static void send_click(int fd, int press) {
  emit_event(fd, EV_KEY, BTN_LEFT, press);
  emit_event(fd, EV_SYN, SYN_REPORT, 0);
}

// Forwards a captured event unchanged (its original timestamp included).
static void reinject_event(int fd, const struct input_event* ev) {
  if (write(fd, ev, sizeof(*ev)) != (ssize_t)sizeof(*ev)) {
    // best effort
  }
}

static int has_capslock_key(const char* path) {
  int fd = open(path, O_RDONLY);
  if (fd < 0) {
    return 0;
  }

  unsigned long evbit = 0;
  ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), &evbit);

  int has_capslock = 0;
  if (evbit & (1 << EV_KEY)) {
    unsigned char key_bits[KEY_MAX / 8 + 1] = {0};
    ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits);
    has_capslock = key_bits[KEY_CAPSLOCK / 8] & (1 << (KEY_CAPSLOCK % 8));
  }

  close(fd);
  return has_capslock;
}

// Opens and grabs every /dev/input/event* device that has a Caps Lock key.
// Grabbing (EVIOCGRAB) makes this process the sole reader, so nothing
// reaches applications except what we re-emit through uinput.
static int find_all_keyboards(Device* devices, int max_devices) {
  DIR* dir = opendir(INPUT_PATH);
  if (!dir) {
    return 0;
  }

  struct dirent* entry;
  int count = 0;
  while ((entry = readdir(dir)) != NULL && count < max_devices) {
    if (strncmp(entry->d_name, "event", 5) != 0) {
      continue;
    }

    char device_path[512];
    snprintf(device_path, sizeof(device_path), "%s/%s", INPUT_PATH, entry->d_name);
    if (!has_capslock_key(device_path)) {
      continue;
    }

    int fd = open(device_path, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
      continue;
    }
    devices[count].fd = fd;
    snprintf(devices[count].path, sizeof(devices[count].path), "%s", device_path);
    ioctl(fd, EVIOCGNAME(sizeof(devices[count].name)), devices[count].name);
    ioctl(fd, EVIOCGRAB, 1);
    count++;
  }

  closedir(dir);
  return count;
}

int main(void) {
  signal(SIGINT, handle_signal);
  signal(SIGTERM, handle_signal);

  static Device devices[MAX_DEVICES];
  int device_count = find_all_keyboards(devices, MAX_DEVICES);
  if (device_count == 0) {
    return 1;
  }

  int uinput_mouse_fd = setup_virtual_mouse();
  if (uinput_mouse_fd < 0) {
    return 1;
  }
  int uinput_kbd_fd = setup_virtual_keyboard();
  if (uinput_kbd_fd < 0) {
    close(uinput_mouse_fd);
    return 1;
  }

  // Like on Windows, holding Caps Lock auto-repeats key-down events;
  // caps_pressed guarantees exactly one click down/up pair per press so
  // drags are not interrupted.
  int caps_pressed = 0;
  struct input_event ev;

  while (running) {
    fd_set readfds;
    FD_ZERO(&readfds);

    int max_fd = -1;
    for (int i = 0; i < device_count; i++) {
      FD_SET(devices[i].fd, &readfds);
      if (devices[i].fd > max_fd) {
        max_fd = devices[i].fd;
      }
    }

    // The timeout lets the loop re-check `running` (set by SIGTERM) even
    // when no input arrives.
    struct timeval timeout = {1, 0};
    int ret = select(max_fd + 1, &readfds, NULL, NULL, &timeout);
    if (ret < 0) {
      if (errno == EINTR) {
        continue;
      }
      break;
    }
    if (ret == 0) {
      continue;
    }

    for (int i = 0; i < device_count; i++) {
      if (!FD_ISSET(devices[i].fd, &readfds)) {
        continue;
      }

      ssize_t n = read(devices[i].fd, &ev, sizeof(ev));
      if (n != sizeof(ev)) {
        continue;
      }

      if (ev.type == EV_KEY && ev.code == KEY_CAPSLOCK) {
        if (ev.value == 1 && !caps_pressed) {
          send_click(uinput_mouse_fd, 1);
          caps_pressed = 1;
        } else if (ev.value == 0 && caps_pressed) {
          send_click(uinput_mouse_fd, 0);
          caps_pressed = 0;
        }
        // Auto-repeat (ev.value == 2) and redundant transitions are
        // swallowed: Caps Lock never reaches applications.
      } else if (ev.type != EV_LED) {
        // Forward everything else; EV_LED is dropped so the Caps Lock LED
        // state doesn't fight with the grabbed keyboard.
        reinject_event(uinput_kbd_fd, &ev);
      }
    }
  }

  // Cleanup: release the grabs and destroy the virtual devices.
  for (int i = 0; i < device_count; i++) {
    ioctl(devices[i].fd, EVIOCGRAB, 0);
    close(devices[i].fd);
  }
  ioctl(uinput_mouse_fd, UI_DEV_DESTROY);
  close(uinput_mouse_fd);
  ioctl(uinput_kbd_fd, UI_DEV_DESTROY);
  close(uinput_kbd_fd);

  return 0;
}
