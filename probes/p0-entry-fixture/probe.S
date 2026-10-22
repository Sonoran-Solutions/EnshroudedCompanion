.intel_syntax noprefix
.text
.global invoke_fixture
.seh_proc invoke_fixture
invoke_fixture:
 push r12
 .seh_pushreg r12
 push r13
 .seh_pushreg r13
 sub rsp, 40
 .seh_stackalloc 40
 .seh_endprologue
 mov r13, rcx
 mov r12, rdx
 mov qword ptr [rsp], 0x10203040
 mov qword ptr [rsp+8], 0x20304050
 mov qword ptr [rsp+16], 0x30405060
 mov qword ptr [rsp+24], 0x40506070
 mov qword ptr [rsp+32], 0x50607010
 lea rax, [rip+after_adapter]
 mov [rip+expected_return], rax
 mov rax, 0x11111111
 mov rcx, 0x22222222
 mov rdx, 0x33333333
 mov r8, 0x44444444
 mov r9, 0x55555555
 mov r10, 0x66666666
 mov r11, 0x77777777
 movdqu xmm0, [rip+pattern]
 movdqu xmm1, [rip+pattern]
 movdqu xmm2, [rip+pattern]
 movdqu xmm3, [rip+pattern]
 movdqu xmm4, [rip+pattern]
 movdqu xmm5, [rip+pattern]
 push 0x246
 popfq
 call r13
after_adapter:
 mov [r12], rax
 mov [r12+8], rcx
 mov [r12+16], rdx
 mov [r12+24], r8
 mov [r12+32], r9
 mov [r12+40], r10
 mov [r12+48], r11
 pushfq
 pop qword ptr [r12+56]
 movdqu [r12+64], xmm0
 movdqu [r12+80], xmm1
 movdqu [r12+96], xmm2
 movdqu [r12+112], xmm3
 movdqu [r12+128], xmm4
 movdqu [r12+144], xmm5
 mov rax, [rsp]
 mov [r12+160], rax
 mov rax, [rsp+8]
 mov [r12+168], rax
 mov rax, [rsp+16]
 mov [r12+176], rax
 mov rax, [rsp+24]
 mov [r12+184], rax
 mov rax, [rsp+32]
 mov [r12+192], rax
 add rsp, 40
 pop r13
 pop r12
 ret
.seh_endproc
.global fixture_callback
.seh_proc fixture_callback
fixture_callback:
 .seh_endprologue
 mov [rip+seen_rcx], rcx
 mov rax, [rsp+272]
 mov [rip+seen_return], rax
 mov rax, rsp
 and eax, 15
 mov [rip+seen_alignment], rax
 # Write all four legal shadow-space words, proving they do not overlap saves.
 mov qword ptr [rsp+8], -1
 mov qword ptr [rsp+16], -1
 mov qword ptr [rsp+24], -1
 mov qword ptr [rsp+32], -1
 xor eax, eax
 xor ecx, ecx
 xor edx, edx
 xor r8d, r8d
 xor r9d, r9d
 xor r10d, r10d
 xor r11d, r11d
 pcmpeqd xmm0, xmm0
 pcmpeqd xmm1, xmm1
 pcmpeqd xmm2, xmm2
 pcmpeqd xmm3, xmm3
 pcmpeqd xmm4, xmm4
 pcmpeqd xmm5, xmm5
 stc
 ret
.seh_endproc
.section .rdata,"dr"
pattern:
 .quad 0x0123456789abcdef, 0xfedcba9876543210
