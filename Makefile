CC      := gcc
CFLAGS  := -std=gnu11 -Wall -Wextra -O2 -D_GNU_SOURCE `sdl2-config --cflags`
LDFLAGS := -lm `sdl2-config --libs` -lX11 -lXfixes -lXext -lSDL2_ttf

SRC    := main.c mem.c overlay.c game.c draw.c menu.c
OBJ    := $(SRC:.c=.o)
TARGET := confin

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

mem.o:     mem.c mem.h
overlay.o: overlay.c overlay.h
game.o:    game.c game.h mem.h
draw.o:    draw.c draw.h game.h config.h
menu.o:    menu.c menu.h overlay.h config.h vendor/nuklear.h vendor/nuklear_sdl_renderer.h
main.o:    main.c mem.h overlay.h game.h draw.h menu.h config.h

clean:
	rm -f $(OBJ) $(TARGET)
