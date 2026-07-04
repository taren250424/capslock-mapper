#ifndef MUTEX_H
#define MUTEX_H

#include <windows.h>

HANDLE create_global_mutex(const char* name);
int find_mutex(const char* name);
void close_mutex(HANDLE hMutex);

#endif