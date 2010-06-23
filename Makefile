# Makefile for sdlblocks

CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lSDL -lSDL_ttf -lSDL_mixer

SRC = sdlblocks.c

sdlblocks: $(SRC)
	$(CC) -c $(SRC)
	$(CC) -o sdlblocks sdlblocks.o $(LDFLAGS)

sdlblocks-debug: $(SRC)
	$(CC) $(CFLAGS) -c $(SRC) -DDEBUG_TETRIS
	$(CC) sdlblocks.o $(LDFLAGS) -o sdlblocks-debug

clean:
	rm sdlblocks
	rm *.o
