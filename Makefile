

SRC_DIR := src
VENDOR_DIR := vendor
VENDOR_DIR := vendor
INCLUDE_DIR := include
BUILD_DIR := build

EXE_NAME := main

GUI ?= true

SRC_FILES := $(shell find $(SRC_DIR) -type f -name '*.c')
SRC_FILES += $(shell find $(VENDOR_DIR) -type f -name '*.c')

# Remove both entry points
SRC_FILES := $(filter-out $(SRC_DIR)/cli.c, $(SRC_FILES))
SRC_FILES := $(filter-out $(SRC_DIR)/server.c, $(SRC_FILES))

ifeq ($(GUI),true)
	SRC_FILES += $(SRC_DIR)/cli.c
else 
	SRC_FILES += $(SRC_DIR)/server.c 
endif

OBJ_FILES := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC_FILES))

CC := gcc
CFLAGS := -g -Wall -Wextra -pedantic -fsanitize=address -I$(INCLUDE_DIR) -I$(VENDOR_DIR) -DBUILD_DIR=\"$(BUILD_DIR)\"
LDFLAGS := -lraylib -lmupdf -lleptonica -ltesseract -lcurl -lwsJson -lm

.PHONY: app clean 

all: folders app

folders:
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/containers
	mkdir -p $(BUILD_DIR)/clay-renderers

app: $(OBJ_FILES)
	$(CC) $(CFLAGS) $^ -o $(EXE_NAME) $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c 
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf main $(BUILD_DIR)


