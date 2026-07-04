#include <windows.h>
#include "../constants/result_constants.h"
#include "env.h"

int get_env_path(HKEY* hKey, DWORD* size, char** envPath) {
    if (RegOpenKeyEx(HKEY_CURRENT_USER, "Environment", 0, KEY_READ | KEY_WRITE, hKey) != ERROR_SUCCESS) {
        return ERR_REG_KEY_OPEN;
    }

    if (RegGetValue(*hKey, NULL, "Path", RRF_RT_ANY, NULL, NULL, size) != ERROR_SUCCESS) {
        return ERR_ENV_QUERY_SIZE;
    }

    *envPath = (char*)malloc(*size + MAX_PATH + 2);
    if (! *envPath) return ERR_MEMORY_ALLOCATION;

    if (RegGetValue(*hKey, NULL, "Path", RRF_RT_ANY, NULL, *envPath, size) != ERROR_SUCCESS) {
        free(*envPath);
        return ERR_GET_ENVIRONMENT_VAR;
    }

    return SUCCESS;
}