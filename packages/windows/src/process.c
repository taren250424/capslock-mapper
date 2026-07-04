#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "constants/app_constants.h"
#include "constants/key_constants.h"
#include "constants/result_constants.h"
#include "utils/common.h"
#include "utils/mutex.h"
#include "utils/env.h"
#include "utils/registry.h"

#include "process.h"

int on_runner() {
    int response = find_mutex(MUTEX_KEY_RUNNER);
    if (response == MUTEX_FOUND) {
        printf(MUTEX_FOUND_MESSAGE);
    } else {
        char path[MAX_PATH];
        int currentPathResult = get_current_path(path, APP_CMD);
        
        if (currentPathResult != SUCCESS) {
            if  (currentPathResult == ERR_GET_EXE_PATH) printf(ERR_GET_EXE_PATH_MESSAGE);
            else if (currentPathResult == ERR_CUTPOINT_NOT_FOUND) printf(ERR_CUTPOINT_NOT_FOUND_MESSAGE);

            return currentPathResult;
        } 

        strcat(path, APP_RUNNER);

        char command[MAX_PATH + 20];
        // snprintf(command, sizeof(command), "start /B %s", path);
        snprintf(command, sizeof(command), "start \"\" /B \"%s\"", path);
        system(command);

        printf("Program has started.\n");
    }

    return SUCCESS;
}

int off_runner() {
    int response = find_mutex(MUTEX_KEY_RUNNER);
    if (response == MUTEX_FOUND) {
        char command[MAX_PATH + 20];
        snprintf(command, sizeof(command), "taskkill /F /IM %s >nul 2>&1", APP_RUNNER);
        system(command);
        printf("Program has stopped.\n");
    } else {
        printf("Program was not running.\n");
    }
    
    return SUCCESS;
}

int show_status() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return FAIL;

    DWORD dwMode = 0;
    if (GetConsoleMode(hOut, &dwMode)) {
        dwMode |= 0x0004;
        SetConsoleMode(hOut, dwMode);
    }

    // 1. Check Mutex.
    int mutex_response = find_mutex(MUTEX_KEY_RUNNER);

    // 2. Check Env.
    // Get env path.
    HKEY hKey_env = NULL;
    DWORD size = 0;
    char* envPath = NULL;

    int envPathResult = get_env_path(&hKey_env, &size, &envPath);
    if (envPathResult != SUCCESS) {
        if (envPathResult == ERR_REG_KEY_OPEN) printf(ERR_REG_KEY_OPEN_MESSAGE);
        else if (envPathResult == ERR_ENV_QUERY_SIZE) printf(ERR_ENV_QUERY_SIZE_MESSAGE);
        else if (envPathResult == ERR_MEMORY_ALLOCATION) printf(ERR_MEMORY_ALLOCATION_MESSAGE);
        else if (envPathResult == ERR_GET_ENVIRONMENT_VAR) printf(ERR_GET_ENVIRONMENT_VAR_MESSAGE);

        RegCloseKey(hKey_env);
        free(envPath);

        return envPathResult;
    }

    // Get exe dir path.
    char exeDirPath[MAX_PATH];
    int currentPathResult = get_current_path(exeDirPath, APP_CMD);
    strcat(exeDirPath, ";");
    
    if (currentPathResult != SUCCESS) {
        if  (currentPathResult == ERR_GET_EXE_PATH) printf(ERR_GET_EXE_PATH_MESSAGE);
        else if (currentPathResult == ERR_CUTPOINT_NOT_FOUND) printf(ERR_CUTPOINT_NOT_FOUND_MESSAGE);

        free(envPath);
        RegCloseKey(hKey_env);

        return currentPathResult;
    } 
    // Remember call free(envPath) & RegCloseKey(hKey_env)

    // 3. Check Registry.
    int registry_response = find_registry();
    if (registry_response == ERR_REG_KEY_OPEN) {
        free(envPath);
        RegCloseKey(hKey_env);

        printf(ERR_REG_KEY_OPEN_MESSAGE);
        return ERR_REG_KEY_OPEN;
    }

    // 4. print.
    printf("[Running status]    %s\n", mutex_response == MUTEX_FOUND ? "\033[0;32mRunning.\033[0m" : "\033[0;31mNot running.\033[0m");
    printf("[Env status]        %s\n", strstr(envPath, exeDirPath) != NULL ? "\033[0;32mfound.\033[0m" : "\033[0;31mNot Found.\033[0m");
    printf("[Registry status]   %s\n", registry_response == REGISTRY_FOUND ? "\033[0;32mFound.\033[0m" : "\033[0;31mNot found.\033[0m");
    
    // 5. clean.
    free(envPath);
    RegCloseKey(hKey_env);
    return SUCCESS;
}

int add_env() {
    HKEY hKey;
    DWORD size = 0;
    char* envPath = NULL;

    int envPathResult = get_env_path(&hKey, &size, &envPath);
    if (envPathResult != SUCCESS) {
        if (envPathResult == ERR_REG_KEY_OPEN) printf(ERR_REG_KEY_OPEN_MESSAGE);
        else if (envPathResult == ERR_ENV_QUERY_SIZE) printf(ERR_ENV_QUERY_SIZE_MESSAGE);
        else if (envPathResult == ERR_MEMORY_ALLOCATION) printf(ERR_MEMORY_ALLOCATION_MESSAGE);
        else if (envPathResult == ERR_GET_ENVIRONMENT_VAR) printf(ERR_GET_ENVIRONMENT_VAR_MESSAGE);

        free(envPath);
        RegCloseKey(hKey);

        return envPathResult;
    }

    char exeDirPath[MAX_PATH];
    int currentPathResult = get_current_path(exeDirPath, APP_CMD);

    if (currentPathResult != SUCCESS) {
        if  (currentPathResult == ERR_GET_EXE_PATH) printf(ERR_GET_EXE_PATH_MESSAGE);
        else if (currentPathResult == ERR_CUTPOINT_NOT_FOUND) printf(ERR_CUTPOINT_NOT_FOUND_MESSAGE);

        free(envPath);
        RegCloseKey(hKey);

        return currentPathResult;
    } 
    
    strcat(exeDirPath, ";");
    if (strstr(envPath, exeDirPath) == NULL) {
        size_t envLen = strlen(envPath);
        if (envLen > 0 && envPath[envLen - 1] != ';') {
            strcat(envPath, ";");
        }
        strcat(envPath, exeDirPath);

        if (RegSetValueEx(hKey, "Path", 0, REG_SZ, (BYTE*)envPath, strlen(envPath) + 1) != ERROR_SUCCESS) {
            free(envPath);
            RegCloseKey(hKey);
            printf(ERR_SET_ENVIRONMENT_VAR_MESSAGE);
            return ERR_SET_ENVIRONMENT_VAR;
        }

        SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)"Environment", SMTO_ABORTIFHUNG, 5000, NULL);
        printf("Environment path has been updated successfully.\n");
    } else {
        printf("The environment path already includes this executable.\n");
    }
    
    free(envPath);
    RegCloseKey(hKey);
    return SUCCESS;
}

int remove_env() {
    HKEY hKey;
    DWORD size = 0;
    char* envPath = NULL;

    int envPathResult = get_env_path(&hKey, &size, &envPath);
    if (envPathResult != SUCCESS) {
        if (envPathResult == ERR_REG_KEY_OPEN) printf(ERR_REG_KEY_OPEN_MESSAGE);
        else if (envPathResult == ERR_ENV_QUERY_SIZE) printf(ERR_ENV_QUERY_SIZE_MESSAGE);
        else if (envPathResult == ERR_MEMORY_ALLOCATION) printf(ERR_MEMORY_ALLOCATION_MESSAGE);
        else if (envPathResult == ERR_GET_ENVIRONMENT_VAR) printf(ERR_GET_ENVIRONMENT_VAR_MESSAGE);

        free(envPath);
        RegCloseKey(hKey);
        return envPathResult;
    }

    char exeDirPath[MAX_PATH];
    int currentPathResult = get_current_path(exeDirPath, APP_CMD);

    if (currentPathResult != SUCCESS) {
        if  (currentPathResult == ERR_GET_EXE_PATH) printf(ERR_GET_EXE_PATH_MESSAGE);
        else if (currentPathResult == ERR_CUTPOINT_NOT_FOUND) printf(ERR_CUTPOINT_NOT_FOUND_MESSAGE);

        free(envPath);
        RegCloseKey(hKey);
        return currentPathResult;
    } 

    char* pathStart = strstr(envPath, exeDirPath);

    if (pathStart != NULL) {
        size_t pathLen = strlen(exeDirPath);
        memmove(pathStart, pathStart + pathLen, strlen(pathStart + pathLen) + 1);

        size_t envLen = strlen(envPath);
        if (envLen > 0 && envPath[envLen - 1] == ';') {
            envPath[envLen - 1] = '\0';  
        }

        if (RegSetValueEx(hKey, "Path", 0, REG_SZ, (BYTE*)envPath, strlen(envPath) + 1) != ERROR_SUCCESS) {
            free(envPath);
            RegCloseKey(hKey);
            printf(ERR_SET_ENVIRONMENT_VAR_MESSAGE);
            return ERR_SET_ENVIRONMENT_VAR;
        }

        SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)"Environment", SMTO_ABORTIFHUNG, 5000, NULL);
        printf("Environment path has been removed successfully.\n");
    } else {
        printf("The specified path is not found in the environment variable.\n");
    }

    free(envPath);
    RegCloseKey(hKey);
    return SUCCESS;
}

int add_registry() {
    int response = find_registry();
    if (response == ERR_REG_KEY_OPEN) {
        printf(ERR_REG_KEY_OPEN_MESSAGE);
        return ERR_REG_KEY_OPEN;
    } else if (response == REGISTRY_FOUND) {
        printf("Already registered.\n");
        return SUCCESS;
    } 
    
    // response == REGISTRY_NOT_FOUND ↓
    
    char exeDirPath[MAX_PATH];
    int currentPathResult = get_current_path(exeDirPath, APP_CMD);

    if (currentPathResult != SUCCESS) {
        if  (currentPathResult == ERR_GET_EXE_PATH) printf(ERR_GET_EXE_PATH_MESSAGE);
        else if (currentPathResult == ERR_CUTPOINT_NOT_FOUND) printf(ERR_CUTPOINT_NOT_FOUND_MESSAGE);

        return currentPathResult;
    } 

    strcat(exeDirPath, APP_RUNNER);

    HKEY hKey;
    LONG openResult = RegOpenKeyEx(HKEY_CURRENT_USER, REG_PATH, 0, KEY_SET_VALUE, &hKey);

    if (openResult != ERROR_SUCCESS) {
        printf(ERR_REG_KEY_OPEN_MESSAGE);
        return ERR_REG_KEY_OPEN;
    }

    LONG setResult = RegSetValueEx(hKey, APP_NAME, 0, REG_SZ, (BYTE*)exeDirPath, strlen(exeDirPath) + 1);
    RegCloseKey(hKey);
    
    if (setResult == ERROR_SUCCESS) {
        printf("Registry successfully registered.\n");
        return SUCCESS;
    } else {
        printf(ERR_SET_REGISTRY_MESSAGE);
        return ERR_SET_REGISTRY;
    }
}

int remove_registry() {
    int response = find_registry();
    if (response == ERR_REG_KEY_OPEN) {
        printf(ERR_REG_KEY_OPEN_MESSAGE);
        return ERR_REG_KEY_OPEN;
    } else if (response == REGISTRY_NOT_FOUND) {
        printf("Already removed.\n");
        return SUCCESS;
    }

    // response == REGISTRY_FOUND ↓

    HKEY hKey;
    LONG regResult = RegOpenKeyEx(HKEY_CURRENT_USER, REG_PATH, 0, KEY_SET_VALUE, &hKey);

    if (regResult != ERROR_SUCCESS) {
        printf(ERR_REG_KEY_OPEN_MESSAGE);
        return ERR_REG_KEY_OPEN;
    }

    regResult = RegDeleteValue(hKey, APP_NAME);
    RegCloseKey(hKey);

    if (regResult == ERROR_SUCCESS) {
        printf("Registry entry removed successfully.\n");
        return SUCCESS;
    } else {
        printf(ERR_DELETE_REGISTRY_MESSAGE);
        return ERR_DELETE_REGISTRY;
    }
}

int show_version() {
    printf("%s\n", VERSION);
    return SUCCESS;
}

int show_help() {
    printf("Usage:\n");
    printf("  on                       - Start mapper process\n");
    printf("  off                      - Terminate mapper process\n\n");
    
    printf("  status | s               - Check mapper status\n\n");

    printf("  env | e                  - Manage environment variables\n");
    printf("      --add     | -a       - Add environment variables\n");
    printf("      --remove  | -r       - Remove environment variables\n\n");

    printf("  reg | r                  - Manage registry startup entry\n");
    printf("      --add     | -a       - Add mapper to startup (via registry)\n");
    printf("      --remove  | -r       - Remove mapper from startup\n\n");

    printf("  --help    | -h           - Show this help message\n");
    printf("  --version | -v           - Show version info\n");

    return SUCCESS;
}

int show_help_invalid() {
    printf("Invalid command. Use '--help' to see available options.\n");
    return SUCCESS;
}

// Check for NULL 'name' values to terminate the list of commands and options.
struct CommandWithOptions commandWithOptions[] = {
    {
        .command = {
            "on",
            "on",
            on_runner
        },
        .options = {
            { NULL, NULL, NULL }
        }
    },
    {
        .command = {
            "off",
            "off",
            off_runner
        },
        .options = {
            { NULL, NULL, NULL }
        }
    },
        {
        .command = {
            "status",
            "s",
            show_status
        },
        .options = {
            { NULL, NULL, NULL }
        }
    },
    {
        .command = {
            NULL,
            NULL,
            NULL
        },
        .options = {
            { "--help", "-h", show_help },
            { "--version", "-v", show_version },
            { NULL, NULL, NULL }
        }
    },
    {
        .command = {
            "env",
            "e",
            NULL
        },
        .options = {
            { "--add", "-a", add_env },
            { "--remove", "-r", remove_env },
            { NULL, NULL, NULL }
        }
    },
    {
        .command = {
            "reg",
            "r",
            NULL
        },
        .options = {
            { "--add", "-a", add_registry },
            { "--remove", "-r", remove_registry },
            { NULL, NULL, NULL }
        }
    },
    {
        .command = {
            NULL,
            NULL,
            NULL
        },
        .options = {
            { NULL, NULL, NULL }
        }
    },
};