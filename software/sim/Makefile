CC ?= cc
CFLAGS ?= -std=gnu99 -O0 -g -Wall -Wextra
CPPFLAGS := -I. -I../User -I../User/drivers -I../Debug
SDL_CFLAGS := $(shell pkg-config --cflags sdl2)
SDL_LIBS := $(shell pkg-config --libs sdl2)
SOURCES := sim.c ../User/ui.c ../User/menu.c ../User/states.c \
           ../User/drivers/ssd1306.c ../User/drivers/pcf8563.c
TARGET := hydrogen-sim

.PHONY: all run clean
all: $(TARGET)

$(TARGET): $(SOURCES) ch32v00X.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SDL_CFLAGS) $(SOURCES) -o $@ $(SDL_LIBS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
