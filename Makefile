

SRC_DIR = src
INCLUDE_DIR = include

SRC_FILES = $(wildcard SRC_DIR/**.c)
OBJ_FILES = $(SRC_FILES:.c=.o)

GUI ?= true

CC = gcc
CFLAGS = -g

.PHONY: main editor ocr pdf clean

all: generate editor ocr pdf

generate:
	$(CC) $(CFLAGS) generate.c -o generate -lcurl -lwsJson -lm

editor:
	$(CC) $(CFLAGS) editor.c ui.c -o editor -lm -lraylib

ocr: 
	$(CC) $(CFLAGS) ocr.c -o ocr -llept -ltesseract

pdf:
	$(CC) $(CFLAGS) pdf.c -o pdf -lm -lmupdf -lmupdf-third -lm
clean:
	rm main editor ocr pdf generate


