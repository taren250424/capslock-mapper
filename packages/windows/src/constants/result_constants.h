#ifndef RESULT_CONSTANTS_H
#define RESULT_CONSTANTS_H

extern const int SUCCESS;
extern const int FAIL;

// common
extern const int ERR_MEMORY_ALLOCATION;

extern const int ERR_GET_EXE_PATH;
extern const int ERR_CUTPOINT_NOT_FOUND;

extern const int ERR_REG_KEY_OPEN;

// mutex.
extern const int MUTEX_FOUND;
extern const int MUTEX_NOT_FOUND;

// env
extern const int ERR_ENV_QUERY_SIZE;
extern const int ERR_GET_ENVIRONMENT_VAR;
extern const int ERR_SET_ENVIRONMENT_VAR;

// reg
extern const int REGISTRY_FOUND;
extern const int REGISTRY_NOT_FOUND;
extern const int ERR_SET_REGISTRY;
extern const int ERR_DELETE_REGISTRY;

// * * *

extern const char* SUCCESS_MESSAGE;
extern const char* FAIL_MESSAGE;

// common
extern const char* ERR_MEMORY_ALLOCATION_MESSAGE;

extern const char* ERR_GET_EXE_PATH_MESSAGE;
extern const char* ERR_CUTPOINT_NOT_FOUND_MESSAGE;

extern const char* ERR_REG_KEY_OPEN_MESSAGE;

// mutex.
extern const char* MUTEX_FOUND_MESSAGE;
extern const char* MUTEX_NOT_FOUND_MESSAGE;

// env
extern const char* ERR_ENV_QUERY_SIZE_MESSAGE;
extern const char* ERR_GET_ENVIRONMENT_VAR_MESSAGE;
extern const char* ERR_SET_ENVIRONMENT_VAR_MESSAGE;

// reg
extern const char* REGISTRY_FOUND_MESSAGE;
extern const char* REGISTRY_NOT_FOUND_MESSAGE;
extern const char* ERR_SET_REGISTRY_MESSAGE;
extern const char* ERR_DELETE_REGISTRY_MESSAGE;

#endif