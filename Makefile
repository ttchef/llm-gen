
CC = gcc
CFLAGS = -g 

.PHONY: main editor ocr clean

all: main editor

main:
	$(CC) $(CFLAGS) main.c -o main -lcurl -lwsJson -lm

editor:
	$(CC) $(CFLAGS) editor.c ui.c -o editor -lm -lraylib

ocr: 
	$(CC) $(CFLAGS) ocr.c -o ocr 
 
clean:
	rm main editor


