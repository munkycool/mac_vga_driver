.PHONY: help all clean emulator run-emulator train-celeste test

help:
	@echo "Available targets: all clean emulator run-emulator train-celeste test"

all:
	mkdir -p build && cd build && cmake .. && make -j$(nproc)

clean:
	rm -rf build
	$(MAKE) -C emulator clean
	$(MAKE) -C tests clean
	rm -f *.o celeste_train

test:
	$(MAKE) -C tests test

emulator:
	$(MAKE) -C emulator

run-emulator: emulator
	./emulator/mac_vga_emulator

train-celeste:
	gcc -O3 -Wall -Wextra -I./firmware/scenes/games -I./firmware/core tools/celeste_train.c firmware/scenes/games/celeste_classic.c -o celeste_train -lm
	./celeste_train

