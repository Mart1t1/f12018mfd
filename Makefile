# Simple SDL mini Makefile

CC=gcc

CPPFLAGS= `pkg-config --cflags sdl` -MMD 
LDFLAGS=
LDLIBS= `pkg-config --libs sdl` -lSDL2_image -lSDL_ttf -pthread

f1: mfd.o

f1.o: structs.h

clean:
	${RM} *.o

# END