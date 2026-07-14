CC = gcc
CFLAGS = -Wall -Wextra -O2 -Istubs -I.. `sdl2-config --cflags` -Dmain=main_app
LDFLAGS = `sdl2-config --libs` -lm

SRCS = ../main.c \
       ../common.c \
       ../video.c \
       ../bouncing_box.c \
       ../pong.c \
       ../tetris.c \
       ../pacman.c \
       ../invaders.c \
       ../donkey_kong.c \
       ../mario_nes.c \
       ../metroid.c \
       ../pixel_art.c \
       ../quotes.c \
       ../frogger.c \
       ../snake.c \
       ../flappy_bird.c \
       ../breakout.c \
       ../asteroids.c \
       ../tron.c \
       ../sanic.c \
       ../undertale.c \
       ../deltarune.c \
       ../splash.c \
       ../mario_show.c \
       ../minecraft.c \
       ../chan.c \
       ../revolution.c \
       sdl_emulator.c

OBJS = $(SRCS:.c=.o)
TARGET = mac_vga_emulator

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
