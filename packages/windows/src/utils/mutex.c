#include <stdio.h>
#include <windows.h>
#include "../constants/result_constants.h"
#include "mutex.h"

HANDLE create_global_mutex(const char* name) {
    return CreateMutexA(NULL, FALSE, name);
}

int find_mutex(const char* name) {
    HANDLE hMutex = OpenMutexA(SYNCHRONIZE, FALSE, name);
    if (hMutex) {
        CloseHandle(hMutex);
        return MUTEX_FOUND;
    }
    return MUTEX_NOT_FOUND;
}

void close_mutex(HANDLE hMutex) {
    if (hMutex) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }
}