

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
SRC_FILES := $(filter-out $(SRC_DIR)/gui.c, $(SRC_FILES))
SRC_FILES := $(filter-out $(SRC_DIR)/server.c, $(SRC_FILES))

ifeq ($(GUI),true)
	SRC_FILES += $(SRC_DIR)/gui.c
else 
	SRC_FILES += $(SRC_DIR)/server.c 
endif

OBJ_FILES := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC_FILES))

CC := gcc
CFLAGS := -g -I$(INCLUDE_DIR) -I$(VENDOR_DIR)
LDFLAGS := -lraylib -lm

.PHONY: app editor ocr pdf clean generate

all: folders app generate editor ocr pdf $(OBJ_FILES)

folders:
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/containers

app: $(OBJ_FILES)
	$(CC) $(CFLAGS) $^ -o $(EXE_NAME) $(LDFLAGS)

generate:
	$(CC) $(CFLAGS) generate.c -o generate -lcurl -lwsJson -lm

editor:
	$(CC) $(CFLAGS) editor.c ui.c -o editor -lm -lraylib

ocr: 
	$(CC) $(CFLAGS) ocr.c -o ocr -llept -ltesseract

pdf:
	$(CC) $(CFLAGS) pdf.c -o pdf -lm -lmupdf -lmupdf-third -lm

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c 
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf main editor ocr pdf generate $(BUILD_DIR)


