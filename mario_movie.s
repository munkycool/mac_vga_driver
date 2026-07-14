.syntax unified
.cpu cortex-m0plus
.thumb
.section .rodata
.global mario_movie_bin
.global mario_movie_bin_end

mario_movie_bin:
.incbin "mario_movie.bin"
mario_movie_bin_end:
