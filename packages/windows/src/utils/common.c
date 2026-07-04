#include <stdio.h>
#include <windows.h>
#include "common.h"
#include "../constants/app_constants.h"
#include "../constants/result_constants.h"

// Get the current directory path excluding the executable file.
int get_current_path(char* exeDirPath , const char* fileName) {
    DWORD result = GetModuleFileName(NULL, exeDirPath , MAX_PATH);
    if (result == 0) return ERR_GET_EXE_PATH;
    char* cutPoint = strstr(exeDirPath , fileName);
    if (! cutPoint) return ERR_CUTPOINT_NOT_FOUND;
    *cutPoint = '\0';
    return SUCCESS;
}