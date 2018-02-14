BITS 64

extern puts
extern exit
extern vram
extern V
extern DT

global emu_entry
global op_debug
global op_nop
global op_ret
global op_jp
global op_call
global op_se_imm
global op_sne_imm
global op_se
global op_ld_imm
global op_add_imm
global op_ld
global op_or
global op_and
global op_xor
global op_add
global op_ld_i
global op_drw
global op_unld_dt
global op_ld_dt
global op_add_i
global op_ld_regs

section .data

section .bss

stack: resq 16

section .text

align 4
emu_entry:
	add rsp, 8
	xor rdi, rdi; I register
	xor r11, r11; SP register
	ret

align 4
op_debug: ; not a real op, used for debugging
	pop rdi
	call puts
	
	mov rdi, -1
	call exit

align 4
op_nop: ; also not a real op
	add rsp, 8
	ret

align 4
op_ret:
	dec r11
	mov rsp, [stack+r11*8]
	ret

align 4
op_jp:
	pop rsp
	ret

align 4
op_call:
	pop rax
	mov [stack+r11*8], rsp
	inc r11
	mov rsp, rax
	ret

align 4
op_se_imm:
	pop rax
	movzx ecx, ah
	cmp [V+ecx], al
	jne .cont
	add rsp, 16
.cont	ret

align 4
op_sne_imm:
	pop rax
	movzx ecx, ah
	cmp [V+ecx], al
	je .cont
	add rsp, 16
.cont	ret

align 4
op_se:
	pop rax
	xor rbx, rbx
	mov bl, ah
	mov cl, [V+rbx]
	mov bl, al
	mov al, [V+rbx]
	
	cmp cl, al
	jne .cont
	add rsp, 16
.cont	ret

align 4
op_ld_imm:
	pop rax
	movzx ecx, ah
	mov [V+ecx], al
	ret

align 4
op_add_imm:
	pop rax
	movzx ecx, ah
	add [V+ecx], al
	ret

align 4
op_or:
	pop rax
	movzx ecx, al
	mov al, [V+ecx]
	movzx ecx, ah
	or [V+ecx], al
	ret

align 4
op_and:
	pop rax
	movzx ecx, al
	mov al, [V+ecx]
	movzx ecx, ah
	and [V+ecx], al
	ret

align 4
op_xor:
	pop rax
	movzx ecx, al
	mov al, [V+ecx]
	movzx ecx, ah
	xor [V+ecx], al
	ret

align 4
op_add:
	pop rax
	movzx ecx, al
	mov al, [V+ecx]
	movzx ecx, ah
	add [V+ecx], al
	ret

align 4
op_ld:
	pop rax
	movzx ecx, al
	mov al, [V+ecx]
	movzx ecx, ah
	mov [V+ecx], al
	ret

align 4
op_ld_i:
	pop rdi
	ret

align 4
op_drw:
	pop rax
	movzx ecx, ah
	mov cl, [V+ecx]
	add cl, 8
	mov dl, al
	xor rbx, rbx
	mov bl, al
	shr bl, 4
	mov bl, [V+rbx]
	
	mov byte [V+0xF], 0
	
	mov rsi, rdi
	
.loop	and dl, 0xF
	jz .done
	
	and rbx, 31
	xor rax, rax
	mov al, [rsi]
	ror rax, cl
	mov r8, rax
	and r8, [vram+rbx*8]
	
	jz .nocol
	mov byte [V+0xF], 1
	
.nocol	xor [vram+rbx*8], rax
	inc rbx
	inc rsi
	dec dl
	
	jmp .loop
	
.done	ret

align 4
op_unld_dt:
	pop rax
	mov dl, [DT]
	mov [rax], dl
	ret

align 4
op_ld_dt:
	pop rax
	mov al, [rax]
	mov [DT], al
	ret

align 4
op_add_i:
	pop rax
	xor rbx, rbx
	mov bl, byte [rax]
	add rdi, rbx
	ret

align 4
op_ld_regs:
	pop rax
.loop	mov bl, [rdi+rax]
	mov [V+rax], bl
	dec rax
	jns .loop
	ret
