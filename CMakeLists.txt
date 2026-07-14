cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(mac_vga C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

set(PICO_FLASH_SIZE_BYTES 8388608)

# Include both main.c and bad_apple.s
add_executable(mac_vga
    main.c
    common.c
    video.c
    bouncing_box.c
    pong.c
    tetris.c
    pacman.c
    invaders.c
    donkey_kong.c
    mario_nes.c
    metroid.c
    pixel_art.c
    quotes.c
    frogger.c
    snake.c
    flappy_bird.c
    breakout.c
    asteroids.c
    tron.c
    sanic.c
    undertale.c
    deltarune.c
    splash.c
    mario_show.c
    minecraft.c
    chan.c
    revolution.c
    bad_apple.s
)

pico_generate_pio_header(mac_vga ${CMAKE_CURRENT_LIST_DIR}/vga.pio)

target_link_libraries(mac_vga pico_stdlib pico_status_led pico_multicore hardware_pio hardware_dma hardware_clocks)

pico_enable_stdio_usb(mac_vga 1)
pico_enable_stdio_uart(mac_vga 0)

pico_add_extra_outputs(mac_vga)
