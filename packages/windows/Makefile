CC = gcc
CFLAGS = -Wall -O2 -I./src/constants -I./src/utils

BUILD_DIR = build
BUILD_CONSTANTS_DIR = build\constants
BUILD_UTILS_DIR = build\utils

TARGETS = $(BUILD_DIR)/cm.exe $(BUILD_DIR)/cm_runner.exe

all: $(TARGETS)

$(BUILD_DIR):
	if not exist $(BUILD_UTILS_DIR) mkdir $(BUILD_DIR)

$(BUILD_CONSTANTS_DIR): $(BUILD_DIR)
	if not exist $(BUILD_CONSTANTS_DIR) mkdir $(BUILD_CONSTANTS_DIR)

$(BUILD_UTILS_DIR): $(BUILD_DIR)
	if not exist $(BUILD_UTILS_DIR) mkdir $(BUILD_UTILS_DIR)

$(BUILD_CONSTANTS_DIR)/app_constants.o: src/constants/app_constants.c src/constants/app_constants.h | $(BUILD_CONSTANTS_DIR)
	$(CC) $(CFLAGS) -c src/constants/app_constants.c -o $(BUILD_CONSTANTS_DIR)/app_constants.o

$(BUILD_CONSTANTS_DIR)/key_constants.o: src/constants/key_constants.c src/constants/key_constants.h | $(BUILD_CONSTANTS_DIR)
	$(CC) $(CFLAGS) -c src/constants/key_constants.c -o $(BUILD_CONSTANTS_DIR)/key_constants.o

$(BUILD_CONSTANTS_DIR)/result_constants.o: src/constants/result_constants.c src/constants/result_constants.h | $(BUILD_CONSTANTS_DIR)
	$(CC) $(CFLAGS) -c src/constants/result_constants.c -o $(BUILD_CONSTANTS_DIR)/result_constants.o

$(BUILD_UTILS_DIR)/common.o: src/utils/common.c src/utils/common.h | $(BUILD_UTILS_DIR)
	$(CC) $(CFLAGS) -c src/utils/common.c -o $(BUILD_UTILS_DIR)/common.o

$(BUILD_UTILS_DIR)/mutex.o: src/utils/mutex.c src/utils/mutex.h | $(BUILD_UTILS_DIR)
	$(CC) $(CFLAGS) -c src/utils/mutex.c -o $(BUILD_UTILS_DIR)/mutex.o

$(BUILD_UTILS_DIR)/env.o: src/utils/env.c src/utils/env.h | $(BUILD_UTILS_DIR)
	$(CC) $(CFLAGS) -c src/utils/env.c -o $(BUILD_UTILS_DIR)/env.o

$(BUILD_UTILS_DIR)/registry.o: src/utils/registry.c src/utils/registry.h | $(BUILD_UTILS_DIR)
	$(CC) $(CFLAGS) -c src/utils/registry.c -o $(BUILD_UTILS_DIR)/registry.o

$(BUILD_DIR)/process.o: src/process.c src/process.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c src/process.c -o $(BUILD_DIR)/process.o

$(BUILD_DIR)/cm.exe: src/main.c $(BUILD_DIR)/process.o $(BUILD_UTILS_DIR)/common.o $(BUILD_UTILS_DIR)/mutex.o $(BUILD_UTILS_DIR)/env.o $(BUILD_UTILS_DIR)/registry.o $(BUILD_CONSTANTS_DIR)/app_constants.o $(BUILD_CONSTANTS_DIR)/key_constants.o $(BUILD_CONSTANTS_DIR)/result_constants.o
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/cm.exe src/main.c $(BUILD_DIR)/process.o $(BUILD_UTILS_DIR)/common.o $(BUILD_UTILS_DIR)/mutex.o $(BUILD_UTILS_DIR)/env.o $(BUILD_UTILS_DIR)/registry.o $(BUILD_CONSTANTS_DIR)/app_constants.o $(BUILD_CONSTANTS_DIR)/key_constants.o $(BUILD_CONSTANTS_DIR)/result_constants.o

$(BUILD_DIR)/cm_runner.exe: src/runner.c $(BUILD_DIR)/process.o $(BUILD_UTILS_DIR)/common.o $(BUILD_UTILS_DIR)/mutex.o $(BUILD_UTILS_DIR)/env.o $(BUILD_UTILS_DIR)/registry.o $(BUILD_CONSTANTS_DIR)/app_constants.o $(BUILD_CONSTANTS_DIR)/key_constants.o $(BUILD_CONSTANTS_DIR)/result_constants.o
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/cm_runner.exe src/runner.c $(BUILD_DIR)/process.o $(BUILD_UTILS_DIR)/common.o $(BUILD_UTILS_DIR)/mutex.o $(BUILD_UTILS_DIR)/env.o $(BUILD_UTILS_DIR)/registry.o $(BUILD_CONSTANTS_DIR)/app_constants.o $(BUILD_CONSTANTS_DIR)/key_constants.o $(BUILD_CONSTANTS_DIR)/result_constants.o

clean:
	del /Q build\*.exe build\*.o
