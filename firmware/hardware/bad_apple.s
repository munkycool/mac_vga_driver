.syntax unified
.cpu cortex-m0plus
.thumb
.section .rodata
.global bad_apple_bin
.global bad_apple_bin_end

bad_apple_bin:
.incbin "../../bad_apple.bin"
bad_apple_bin_end:
