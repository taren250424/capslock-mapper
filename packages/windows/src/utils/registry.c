#include <Windows.h>
#include "../constants/app_constants.h"
#include "../constants/key_constants.h"
#include "../constants/result_constants.h"
#include "registry.h"

int find_registry() {
    HKEY hKey;
    LONG openResult = RegOpenKeyEx(HKEY_CURRENT_USER, REG_PATH, 0, KEY_QUERY_VALUE, &hKey);

    if (openResult != ERROR_SUCCESS) {
        return ERR_REG_KEY_OPEN;
    } 

    LONG queryResult = RegQueryValueEx(hKey, APP_NAME, NULL, NULL, NULL, NULL);
    RegCloseKey(hKey);

    if (queryResult == ERROR_SUCCESS) return REGISTRY_FOUND;
    else return REGISTRY_NOT_FOUND;
}