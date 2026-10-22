#ifndef P0_FIXTURE_ENTRY_ADAPTER_H
#define P0_FIXTURE_ENTRY_ADAPTER_H
/* Fixture-only pure code builder. Not a decoder, installer, or relocation API. */
#include <windows.h>
#include <stdint.h>
#include <string.h>
enum { ENTRY_IMAGE_SIZE=768, ENTRY_MAX_POINTS=64 };
typedef struct {
 unsigned pc, depth, stage; /* stage: 0 adapter, 1 saved RBP, 2 saved RBX, 3 frame */
} EntryPoint;
typedef struct {
 unsigned char image[ENTRY_IMAGE_SIZE];
 RUNTIME_FUNCTION functions[3];
 EntryPoint points[ENTRY_MAX_POINTS];
 unsigned count, code_end, copied, restore_flags, jump;
} EntryAdapter;
typedef struct {
 uintptr_t callback, continuation;
 const unsigned char *overwrite;
 size_t overwrite_size;
} EntryParameters;
static inline void entry_bytes(EntryAdapter *a,unsigned *n,const unsigned char *p,unsigned z) {
 memcpy(a->image+*n,p,z); *n+=z;
}
#define ENTRY_BYTES(...) do { const unsigned char b[]={__VA_ARGS__}; entry_bytes(&a,&n,b,sizeof b); } while(0)
#define ENTRY_POINT(d,s) do { a.points[a.count++]=(EntryPoint){n,d,s}; } while(0)
/* Success fills only a private byte image and relative unwind metadata. Caller must
   supply code lifetime, RX publication, unwind registration, and target identity.
   Contract: exact 11-byte REX push RBP / push RBX / LEA RBP,[RSP+disp32].
   Only signed, 8-byte-multiple LEA displacements -240..240 are supported.
   Requires ordinary Windows x64 caller shadow space; its first word is scratch
   for the continuation pointer and is not preserved. No RIP operand, branch,
   stack allocation, or other instruction is accepted. */
static inline int entry_adapter_build(EntryAdapter *out,const EntryParameters *p) {
 if(!out || !p || !p->callback || !p->continuation || !p->overwrite || p->overwrite_size!=11) return 0;
 const unsigned char prefix[]={0x40,0x55,0x53,0x48,0x8d,0xac,0x24};
 if(memcmp(p->overwrite,prefix,sizeof prefix)) return 0;
 int32_t frame; memcpy(&frame,p->overwrite+7,4);
 if(frame < -240 || frame>240 || frame%8) return 0;
 EntryAdapter a; memset(&a,0,sizeof a); unsigned n=0;
 ENTRY_POINT(0,0); ENTRY_BYTES(0x9c);
 ENTRY_POINT(8,0); ENTRY_BYTES(0x48,0x81,0xec,0,1,0,0);
 for(unsigned load=0;load<2;load++) {
  for(unsigned i=0;i<7;i++) {
   const unsigned char reg[]={0,1,2,0,1,2,3};
   ENTRY_POINT(264,0); ENTRY_BYTES(i<3?0x48:0x4c,load?0x8b:0x89,0x84+reg[i]*8,0x24);
   unsigned disp=0x80+i*8; entry_bytes(&a,&n,(unsigned char*)&disp,4);
  }
  for(unsigned i=0;i<6;i++) {
   ENTRY_POINT(264,0); ENTRY_BYTES(0xf3,0x0f,load?0x6f:0x7f,0x44+i*8,0x24,0x20+i*16);
  }
  if(!load) {
   ENTRY_POINT(264,0); ENTRY_BYTES(0xfc);
   ENTRY_POINT(264,0); ENTRY_BYTES(0x48,0xb8); entry_bytes(&a,&n,(const unsigned char*)&p->callback,8);
   ENTRY_POINT(264,0); ENTRY_BYTES(0xff,0xd0);
   /* Original caller shadow word [entry RSP+8] is callee-owned scratch.
      Use mod01 RSP-indirect jump later: mod00 RIP-indirect would be
      misidentified as an epilogue despite the copied pushes still live. */
   ENTRY_POINT(264,0); ENTRY_BYTES(0x48,0xb8); entry_bytes(&a,&n,(const unsigned char*)&p->continuation,8);
   ENTRY_POINT(264,0); ENTRY_BYTES(0x48,0x89,0x84,0x24,0x10,0x01,0,0);
  }
 }
 ENTRY_POINT(264,0); ENTRY_BYTES(0x48,0x81,0xc4,0,1,0,0);
 a.restore_flags=n; ENTRY_POINT(8,0); ENTRY_BYTES(0x9d);
 a.copied=n;
 ENTRY_POINT(0,0); entry_bytes(&a,&n,p->overwrite,2);
 ENTRY_POINT(8,1); entry_bytes(&a,&n,p->overwrite+2,1);
 ENTRY_POINT(16,2); entry_bytes(&a,&n,p->overwrite+3,8);
 a.jump=n; ENTRY_POINT(16,3); ENTRY_BYTES(0xff,0x64,0x24,0x18);
 a.code_end=n;
 const unsigned char uw1[]={1,8,3,0,8,1,32,0,1,2,0,0};
 const unsigned char uw2[]={1,0,1,0,0,2,0,0};
 /* LEA computes a pointer, not a stack allocation or formal unwind frame.
    RSP stays entry-16; stack-based PUSH_NONVOL codes recover both saves. */
 const unsigned char uw3[]={1,11,2,0,3,0x30,2,0x50};
 memcpy(a.image+512,uw1,sizeof uw1); memcpy(a.image+528,uw2,sizeof uw2); memcpy(a.image+544,uw3,sizeof uw3);
 a.functions[0]=(RUNTIME_FUNCTION){0,a.restore_flags,512};
 a.functions[1]=(RUNTIME_FUNCTION){a.restore_flags,a.copied,528};
 a.functions[2]=(RUNTIME_FUNCTION){a.copied,a.code_end,544};
 *out=a; return 1;
}
#undef ENTRY_BYTES
#undef ENTRY_POINT
#endif
