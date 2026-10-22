.intel_syntax noprefix
.text
.global p0_composition_entry
.seh_proc p0_composition_entry
p0_composition_entry:
 .byte 0x40,0x55
 .seh_pushreg rbp
 push rbx
 .seh_pushreg rbx
 lea rbp, [rsp-200]
 .seh_endprologue
 mov eax, 7
 pop rbx
 pop rbp
 ret
.seh_endproc
