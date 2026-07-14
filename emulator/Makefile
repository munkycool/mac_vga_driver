CC = gcc
CFLAGS = -Wall -Wextra -O2 -Istubs -I../firmware/core -I../firmware/hardware `sdl2-config --cflags` -Dmain=main_app
LDFLAGS = `sdl2-config --libs` -lm -Wl,--wrap,update_input

SRCS = ../firmware/core/main.c \
       ../firmware/core/common.c \
       ../firmware/core/video.c \
       ../firmware/scenes/games/bouncing_box.c \
       ../firmware/scenes/games/pong.c \
       ../firmware/scenes/games/tetris.c \
       ../firmware/scenes/games/pacman.c \
       ../firmware/scenes/games/invaders.c \
       ../firmware/scenes/games/donkey_kong.c \
       ../firmware/scenes/games/mario_nes.c \
       ../firmware/scenes/games/metroid.c \
       ../firmware/scenes/visuals/pixel_art.c \
       ../firmware/scenes/visuals/quotes.c \
       ../firmware/scenes/games/frogger.c \
       ../firmware/scenes/games/snake.c \
       ../firmware/scenes/games/flappy_bird.c \
       ../firmware/scenes/games/breakout.c \
       ../firmware/scenes/games/asteroids.c \
       ../firmware/scenes/games/tron.c \
       ../firmware/scenes/games/sanic.c \
       ../firmware/scenes/games/undertale.c \
       ../firmware/scenes/games/deltarune.c \
       ../firmware/scenes/visuals/splash.c \
       ../firmware/scenes/games/mario_show.c \
       ../firmware/scenes/games/minecraft.c \
       ../firmware/scenes/games/chan.c \
       ../firmware/scenes/games/revolution.c \
       ../firmware/scenes/visuals/perkins.c \
       ../firmware/scenes/visuals/downloads_slideshow.c \
       ../build/generated/downloads_slideshow/downloads_slideshow_assets.c \
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
