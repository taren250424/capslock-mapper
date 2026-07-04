// Resident mapper process: turns Caps Lock into the left mouse button.
// Built as a GUI-subsystem executable (see CMakeLists.txt), so it never
// opens a console window and detaches from the terminal on its own.

#include <windows.h>

#include "app.h"

static int isLeftDown = 0;

static void send_mouse(DWORD flags) {
  INPUT input = {0};
  input.type = INPUT_MOUSE;
  input.mi.dwFlags = flags;
  SendInput(1, &input, sizeof(INPUT));
}

static LRESULT CALLBACK keyboard_proc(int code, WPARAM wparam, LPARAM lparam) {
  if (code == HC_ACTION) {
    const KBDLLHOOKSTRUCT* key = (const KBDLLHOOKSTRUCT*)lparam;
    if (key->vkCode == VK_CAPITAL) {
      // While the key is held the OS keeps repeating WM_KEYDOWN (e.g. during
      // a drag). isLeftDown guarantees exactly one down and one up event;
      // without it the repeated downs would interrupt the drag.
      if (wparam == WM_KEYDOWN && !isLeftDown) {
        send_mouse(MOUSEEVENTF_LEFTDOWN);
        isLeftDown = 1;
      } else if (wparam == WM_KEYUP && isLeftDown) {
        send_mouse(MOUSEEVENTF_LEFTUP);
        isLeftDown = 0;
      }
      // Swallow Caps Lock in every case, including auto-repeats, so the
      // toggle state never changes while the mapper runs.
      return 1;
    }
  }
  return CallNextHookEx(NULL, code, wparam, lparam);
}

int main(void) {
  // Single-instance guard. The mutex is held for the runner's whole lifetime
  // and doubles as the "running" flag that cm.exe checks.
  HANDLE mutex = CreateMutexA(NULL, FALSE, RUNNER_MUTEX_NAME);
  if (!mutex) {
    return 1;
  }
  if (GetLastError() == ERROR_ALREADY_EXISTS) {
    CloseHandle(mutex);
    return 0;
  }

  // `cm off` signals this event instead of killing the process, so the hook
  // below is removed and all handles are released cleanly.
  HANDLE stop = CreateEventA(NULL, TRUE, FALSE, RUNNER_STOP_EVENT_NAME);
  if (!stop) {
    CloseHandle(mutex);
    return 1;
  }

  HHOOK hook = SetWindowsHookExA(WH_KEYBOARD_LL, keyboard_proc, NULL, 0);
  if (!hook) {
    CloseHandle(stop);
    CloseHandle(mutex);
    return 1;
  }

  // A low-level keyboard hook only fires while this thread pumps messages,
  // so wait on the stop event and the message queue at the same time.
  int running = 1;
  while (running) {
    DWORD wait = MsgWaitForMultipleObjects(1, &stop, FALSE, INFINITE, QS_ALLINPUT);
    if (wait == WAIT_OBJECT_0) {
      break; // stop requested by `cm off`
    }
    MSG msg;
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
      if (msg.message == WM_QUIT) {
        running = 0;
        break;
      }
      TranslateMessage(&msg);
      DispatchMessageA(&msg);
    }
  }

  UnhookWindowsHookEx(hook);
  CloseHandle(stop);
  CloseHandle(mutex);
  return 0;
}
