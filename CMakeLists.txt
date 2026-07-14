cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(mac_vga C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

pico_sdk_init()

set(PICO_FLASH_SIZE_BYTES 8388608)

set(DOWNLOADS_IMAGE_DIR "$ENV{HOME}/Downloads" CACHE PATH "Directory scanned for slideshow images")
set(DOWNLOADS_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/downloads_slideshow")
set(DOWNLOADS_GENERATED_SOURCE "${DOWNLOADS_GENERATED_DIR}/downloads_slideshow_assets.c")

add_custom_command(
    OUTPUT ${DOWNLOADS_GENERATED_SOURCE}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${DOWNLOADS_GENERATED_DIR}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/tools/generate_downloads_slideshow.py
            --input-dir ${DOWNLOADS_IMAGE_DIR}
            --output-file ${DOWNLOADS_GENERATED_SOURCE}
    DEPENDS ${CMAKE_CURRENT_LIST_DIR}/tools/generate_downloads_slideshow.py
    COMMENT "Generating Downloads slideshow assets"
    VERBATIM
)

add_executable(mac_vga
    firmware/core/main.c
    firmware/core/common.c
    firmware/core/video.c
    firmware/scenes/games/bouncing_box.c
    firmware/scenes/games/pong.c
    firmware/scenes/games/tetris.c
    firmware/scenes/games/pacman.c
    firmware/scenes/games/invaders.c
    firmware/scenes/games/donkey_kong.c
    firmware/scenes/games/mario_nes.c
    firmware/scenes/games/metroid.c
    firmware/scenes/visuals/pixel_art.c
    firmware/scenes/visuals/quotes.c
    firmware/scenes/games/frogger.c
    firmware/scenes/games/snake.c
    firmware/scenes/games/flappy_bird.c
    firmware/scenes/games/breakout.c
    firmware/scenes/games/asteroids.c
    firmware/scenes/games/tron.c
    firmware/scenes/games/sanic.c
    firmware/scenes/games/undertale.c
    firmware/scenes/games/deltarune.c
    firmware/scenes/visuals/splash.c
    firmware/scenes/games/mario_show.c
    firmware/scenes/games/minecraft.c
    firmware/scenes/games/chan.c
    firmware/scenes/games/revolution.c
    firmware/scenes/visuals/downloads_slideshow.c
    firmware/scenes/visuals/perkins.c
    firmware/hardware/bad_apple.s
    ${DOWNLOADS_GENERATED_SOURCE}
)

pico_generate_pio_header(mac_vga ${CMAKE_CURRENT_LIST_DIR}/firmware/hardware/vga.pio)

target_include_directories(mac_vga PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/firmware/core
    ${CMAKE_CURRENT_LIST_DIR}/firmware/hardware
)

target_link_libraries(mac_vga pico_stdlib hardware_pio hardware_dma hardware_clocks)

pico_enable_stdio_usb(mac_vga 1)
pico_enable_stdio_uart(mac_vga 0)

pico_add_extra_outputs(mac_vga)
