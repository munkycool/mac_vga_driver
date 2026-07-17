CC = gcc
# Added -I../build to grab the generated vga.pio.h if it exists
CFLAGS = -Wall -Wextra -O2 -Istubs -I../build -I../firmware/core -I../firmware/hardware `sdl2-config --cflags` -Dmain=main_app -DEMULATOR=1
LDFLAGS = `sdl2-config --libs` -lm -Wl,--wrap,update_input

# Define object directory to keep source directories clean
OBJDIR = obj
$(shell mkdir -p $(OBJDIR))

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
       ../firmware/scenes/games/celeste.c \
       ../firmware/scenes/games/celeste_classic.c \
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
       ../firmware/scenes/visuals/happy.c \
       ../firmware/scenes/visuals/downloads_slideshow.c \
       ../build/generated/downloads_slideshow/downloads_slideshow_assets.c \
       sdl_emulator.c

# Map source files to object files in the local OBJDIR
OBJS = $(addprefix $(OBJDIR)/, $(notdir $(SRCS:.c=.o)))

TARGET = mac_vga_emulator

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Rule for local files
$(OBJDIR)/sdl_emulator.o: sdl_emulator.c
	$(CC) $(CFLAGS) -c $< -o $@

# Generic rule for firmware files
$(OBJDIR)/%.o: ../firmware/core/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: ../firmware/scenes/games/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: ../firmware/scenes/visuals/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: ../build/generated/downloads_slideshow/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(TARGET)
	# Also clean up the messy ones in source dirs if they exist
	rm -f ../firmware/core/*.o ../firmware/scenes/games/*.o ../firmware/scenes/visuals/*.o ../build/generated/downloads_slideshow/*.o
