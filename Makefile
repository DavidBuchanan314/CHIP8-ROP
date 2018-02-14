all: chip8

chip8: opcodes.o chip8.c
	gcc chip8.c opcodes.o -O3 -o chip8 -Wall -no-pie

opcodes.o: opcodes.asm
	nasm opcodes.asm -felf64 -o opcodes.o
