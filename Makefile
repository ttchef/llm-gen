
CC = gcc
CFLAGS = -g 

.PHONY: main editor clean

all: main editor

main:
	$(CC) $(CFLAGS) main.c -o main -lcurl -lwsJson -lm

editor:
	$(CC) $(CFLAGS) editor.c -o editor -lm -lraylib
 
clean:
	rm main editor


