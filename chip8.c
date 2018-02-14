#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sched.h>

#define WIDTH 64
#define HEIGHT 32

uint8_t dram[0x1000]; // data ram
__attribute__((aligned(8))) void * iram[0x1000]; // instruction ram (opcode + operand word for each)
uint8_t vram[WIDTH * HEIGHT / 8]; // bitpacked graphics

uint8_t V[16]; // registers
uint8_t DT; // delay timer

void * emu_entry(char * arg); // emulator thread entrypoint

// these are actually code linked in from opcodes.asm
void * op_debug(char * arg);
void * op_nop(char * arg);
void * op_ret(void * arg);
void * op_jp(void * arg);
void * op_call(void * arg);
void * op_se_imm(void * arg);
void * op_sne_imm(void * arg);
void * op_se(void * arg);
void * op_ld_imm(void * arg);
void * op_add_imm(void * arg);
void * op_ld(void * arg);
void * op_or(void * arg);
void * op_and(void * arg);
void * op_xor(void * arg);
void * op_add(void * arg);
void * op_ld_i(void * arg);
void * op_drw(void * arg);
void * op_unld_dt(void * arg);
void * op_ld_dt(void * arg);
void * op_add_i(void * arg);
void * op_ld_regs(void * arg);

char err_buf[64];

void render_gfx(void) {
	char row[WIDTH*2+1];
	int i;
	row[WIDTH*2] = 0;
	for (int y = 0; y < HEIGHT; y++) {
		i = WIDTH*2;
		for (int x1 = 0; x1 < WIDTH/8; x1++) {
			for (int x0 = 0; x0 < 8; x0++) {
				row[--i] = (vram[y*(WIDTH/8)+x1]&(1<<x0)) ? '#' : ' ';
				row[i-1] = row[i]; // each pixel is rendered as two ascii chars
				i--;
			}
		}
		puts(row);
	}
}

int main(int argc, char *argv[]) {
	
	if (argc !=2) {
		printf("USAGE: %s ROMFILE\n", argv[0]);
		return -1;
	}
	
	FILE * romfile = fopen(argv[1], "r");
	
	if (!romfile) {
		perror("fopen");
		return -1;
	}
	
	printf("Loaded %lu bytes from %s.\n", fread(&dram[0x200], 1, 0xE00, romfile), argv[1]);
	fclose(romfile);
	
	for (int i = 0; i < sizeof(dram); i += 2) {
		uint8_t ophi = dram[i] >> 4;
		uint8_t oplo = dram[i] & 0xF;
		uint8_t operand = dram[i+1];
		uint8_t operhi = operand >> 4;
		uint8_t operlo = operand & 0xF;
		uint16_t lo3nibbles = (oplo << 8) | operand;
		switch(ophi) {
			case 0x0: // 1nnn - JP addr
				switch(operand) {
					case 0xE0: //  00E0 - CLS
						iram[i] = op_nop; //TODO
						break;
					case 0xEE: // 00EE - RET
						iram[i] = op_ret;
						break;
					default:
						goto unimp;
				}
				break;
			case 0x1: // 1nnn - JP addr
				iram[i] = op_jp;
				iram[i+1] = &iram[lo3nibbles];
				break;
			case 0x2: // 2nnn - CALL addr
				iram[i] = op_call;
				iram[i+1] = &iram[lo3nibbles];
				break;
			case 0x3: // 3xkk - SE Vx, byte
				iram[i] = op_se_imm;
				iram[i+1] = lo3nibbles;
				break;
			case 0x4: // 4xkk - SNE Vx, byte
				iram[i] = op_sne_imm;
				iram[i+1] = lo3nibbles;
				break;
			case 0x5: // 5xy0 - SE Vx, Vy
				iram[i] = op_se;
				iram[i+1] = oplo<<8 | operhi;
				break;
			case 0x6: // 6xkk - LD Vx, byte
				iram[i] = op_ld_imm;
				iram[i+1] = lo3nibbles;
				break;
			case 0x7: // 7xkk - ADD Vx, byte
				iram[i] = op_add_imm;
				iram[i+1] = lo3nibbles;
				break;
			case 0x8:
				switch(operlo) {
					case 0x0: // 8xy0 - LD Vx, Vy
						iram[i] = op_ld;
						iram[i+1] = oplo << 8 | operhi;
						break;
					case 0x1: // 8xy1 - OR Vx, Vy
						iram[i] = op_or;
						iram[i+1] = oplo << 8 | operhi;
						break;
					case 0x2: // 8xy2 - AND Vx, Vy
						iram[i] = op_and;
						iram[i+1] = oplo << 8 | operhi;
						break;
					case 0x3: // 8xy3 - XOR Vx, Vy
						iram[i] = op_xor;
						iram[i+1] = oplo << 8 | operhi;
						break;
					case 0x4: // 8xy4 - ADD Vx, Vy
						iram[i] = op_add;
						iram[i+1] = oplo << 8 | operhi;
						break;
					default:
						goto unimp;
				}
				break;
			case 0xA: // Annn - LD I, addr
				iram[i] = op_ld_i;
				iram[i+1] = &dram[lo3nibbles];
				break;
			case 0xC: //  Cxkk - RND Vx, byte
				iram[i] = op_nop; //TODO
				break;
			case 0xD: // Dxyn - DRW Vx, Vy, nibble
				iram[i] = op_drw;
				iram[i+1] = lo3nibbles;
				break;
			case 0xE:
				iram[i] = op_nop; //TODO
				break;
			case 0xF:
				switch(operand) {
					case 0x07: // Fx07 - LD Vx, DT
						iram[i] = op_unld_dt;
						iram[i+1] = &V[oplo];
						break;
					case 0x15: // Fx15 - LD DT, Vx
						iram[i] = op_ld_dt;
						iram[i+1] = &V[oplo];
						break;
					case 0x1e: // Fx1E - ADD I, Vx
						iram[i] = op_add_i;
						iram[i+1] = &V[oplo];
						break;
					case 0x65: // LD Vx, [I]
						iram[i] = op_ld_regs;
						iram[i+1] = oplo;
						break;
					default:
						goto unimp;
				}
				break;
			default:
			unimp:
				snprintf(err_buf, sizeof(err_buf), "Unimplemented instruction: 0x%02X%02X", dram[i], dram[i+1]);
				if (ophi != 0) {
					printf("JIT warning: %s\n", err_buf);
				}
				iram[i] = op_debug;
				iram[i+1] = strdup(err_buf);
		}
	}
	
	clone((int (*)(void *)) emu_entry, &iram[0x200], CLONE_VM | SIGKILL, NULL); // XXX stack position is sketchy
	
	// rendering and timing occurs in a seperate thread
	for (;;) {
		if (DT > 0) DT--;
		printf("DT=0x%02X\n", DT);
		render_gfx();
		usleep(1e6 / 60); // 60fps
		printf("\33[33A"); // reset cursor for redraw
	}
	
	return -1; // execution should never reach here...
}
