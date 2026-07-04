#include "result_constants.h"

const int SUCCESS = 0;
const int FAIL = 1;

const int ERR_MEMORY_ALLOCATION = 11;

const int ERR_GET_EXE_PATH = 12;
const int ERR_CUTPOINT_NOT_FOUND = 13;

const int ERR_REG_KEY_OPEN = 14;

const int MUTEX_FOUND = 21;
const int MUTEX_NOT_FOUND = 22;

const int ERR_ENV_QUERY_SIZE = 31;
const int ERR_GET_ENVIRONMENT_VAR = 32;
const int ERR_SET_ENVIRONMENT_VAR = 33;

const int REGISTRY_FOUND = 41;
const int REGISTRY_NOT_FOUND = 42;
const int ERR_SET_REGISTRY = 43;
const int ERR_DELETE_REGISTRY = 44;

// * * *

const char* SUCCESS_MESSAGE = "";
const char* FAIL_MESSAGE = "";

const char* ERR_MEMORY_ALLOCATION_MESSAGE = "Memory allocation failed.\n";

const char* ERR_GET_EXE_PATH_MESSAGE = "Failed to get the executable path.\n";
const char* ERR_CUTPOINT_NOT_FOUND_MESSAGE = "Failed to find cut pointer.\n";

const char* ERR_REG_KEY_OPEN_MESSAGE = "Failed to open registry key.\n";

const char* MUTEX_FOUND_MESSAGE = "Program is already running.\n";
const char* MUTEX_NOT_FOUND_MESSAGE = "";

const char* ERR_ENV_QUERY_SIZE_MESSAGE = "Failed to query size of environment variable.\n";
const char* ERR_GET_ENVIRONMENT_VAR_MESSAGE = "Failed to get the current environment variable.\n";
const char* ERR_SET_ENVIRONMENT_VAR_MESSAGE = "Failed to set environment variable.\n";

const char* REGISTRY_FOUND_MESSAGE = "";
const char* REGISTRY_NOT_FOUND_MESSAGE = "";
const char* ERR_SET_REGISTRY_MESSAGE = "Failed to register in registry.\n";
const char* ERR_DELETE_REGISTRY_MESSAGE = "Failed to remove or key does not exist.\n";
