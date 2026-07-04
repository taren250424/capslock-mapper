#ifndef ENV_H
#define ENV_H

#include <windows.h>

int get_env_path(HKEY* hKey, DWORD* size, char** envPath);

#endif