#include <stdio.h>
#include <windows.h>
#include "constants/app_constants.h"
#include "constants/key_constants.h"
#include "constants/result_constants.h"
#include "utils/mutex.h"

int isLeftDown = 0;

void sendMouseLeftDown() {
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    SendInput(1, &input, sizeof(INPUT));
}

void sendMouseLeftUp() {
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(1, &input, sizeof(INPUT));
}

// This function is called repeatedly by the OS while the key is held down (e.g., during a drag operation).
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT *p = (KBDLLHOOKSTRUCT *)lParam;
        // If the flag isLeftDown is not set, 
        // the folder opens when holding the click with Caps Lock pressed, unlike using the mouse where it doesn't open.
        if (p->vkCode == VK_CAPITAL) {
            if (wParam == WM_KEYDOWN && ! isLeftDown) {
                sendMouseLeftDown();
                isLeftDown = 1;
                return 1;
            } else if (wParam == WM_KEYUP && isLeftDown) {
                sendMouseLeftUp();
                isLeftDown = 0;
                return 1;
            }

            // During dragging, the keydown event is triggered repeatedly once the flag isLeftDown is set to 1.
            // To prevent this, we return early here.
            return 1;
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

int main() {
    FreeConsole();
    
    HANDLE hMutex;

    int response = find_mutex(MUTEX_KEY_RUNNER);
    if (response == MUTEX_FOUND) {
        return 0;
    } else {
        hMutex = create_global_mutex(MUTEX_KEY_RUNNER);

        if (! hMutex) {
            return 1;
        }
    }

    HHOOK hHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
    if (! hHook) {
        return 1;
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(hHook);
    close_mutex(hMutex);

    return 0;
}
