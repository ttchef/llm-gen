

SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build

EXE_NAME = main

SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC_FILES))

GUI ?= true

CC = gcc
CFLAGS = -g

.PHONY: app editor ocr pdf clean generate

all: $(BUILD_DIR) app generate editor ocr pdf $(OBJ_FILES)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

app: $(OBJ_FILES)
	$(CC) $(CFLAGS) $^ -o $(EXE_NAME)

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


