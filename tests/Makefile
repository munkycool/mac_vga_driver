CC = gcc
CFLAGS = -Wall -Wextra -O1 -g \
         -I../emulator/stubs -I../firmware/core -I../firmware/scenes/games -I../firmware/scenes/visuals \
         -DEMULATOR=1
LDFLAGS = -lm

OBJDIR = obj
$(shell mkdir -p $(OBJDIR))

CORE_SRCS = ../firmware/core/common.c ../firmware/core/video.c test_stubs.c
SCENE_SRCS = $(wildcard ../firmware/scenes/games/*.c) $(wildcard ../firmware/scenes/visuals/*.c)
# main.c is deliberately not compiled separately: test_scenes.c #includes it directly
# to reach its static state-history helpers.

.PHONY: test clean

test: $(OBJDIR)/test_core $(OBJDIR)/test_celeste_classic $(OBJDIR)/test_scenes
	./$(OBJDIR)/test_core
	./$(OBJDIR)/test_celeste_classic
	./$(OBJDIR)/test_scenes

$(OBJDIR)/test_core: test_core.c $(CORE_SRCS) test.h
	$(CC) $(CFLAGS) test_core.c $(CORE_SRCS) -o $@ $(LDFLAGS)

$(OBJDIR)/test_celeste_classic: test_celeste_classic.c ../firmware/scenes/games/celeste_classic.c test.h
	$(CC) $(CFLAGS) test_celeste_classic.c ../firmware/scenes/games/celeste_classic.c -o $@ $(LDFLAGS)

$(OBJDIR)/test_scenes: test_scenes.c $(CORE_SRCS) $(SCENE_SRCS) test.h
	$(CC) $(CFLAGS) test_scenes.c $(CORE_SRCS) $(SCENE_SRCS) -o $@ $(LDFLAGS)

clean:
	rm -rf $(OBJDIR)
