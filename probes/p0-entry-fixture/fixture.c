/* Synthetic Windows x64 fixture only. No target image, loader, patch or profile. */
#include "entry_adapter.h"
#include <stdio.h>
#include <stdlib.h>
uint64_t seen_rcx, seen_alignment, seen_return, expected_return;
extern void fixture_callback(void);
extern void invoke_fixture(void *,void *);
static unsigned char *page;
static void check(int ok,const char *what) { if(!ok) { fprintf(stderr,"FAIL: %s\n",what); exit(1); } }
static void unwind_check(RUNTIME_FUNCTION *f,unsigned pc,unsigned depth,unsigned stage,int frame) {
 uint64_t stack[96]={0}; uint64_t *entry=&stack[64];
 entry[0]=0x123456789abcULL; entry[-1]=0xaabbccdd; entry[-2]=0x11223344;
 CONTEXT c; memset(&c,0,sizeof c); c.Rip=(DWORD64)page+pc;
 c.Rsp=(DWORD64)((unsigned char*)entry-depth); c.Rbx=0x99;
 c.Rbp=stage==3?(DWORD64)((unsigned char*)entry-16+frame):0x88;
 DWORD64 base=0; PRUNTIME_FUNCTION found=RtlLookupFunctionEntry(c.Rip,&base,NULL);
 check(found==f && base==(DWORD64)page,"dynamic unwind lookup");
 PVOID handler=NULL; DWORD64 establisher=0;
 RtlVirtualUnwind(UNW_FLAG_NHANDLER,base,c.Rip,found,&c,&handler,&establisher,NULL);
 if(c.Rip!=entry[0] || c.Rsp!=(DWORD64)(entry+1)) fprintf(stderr,"boundary pc=%u depth=%u stage=%u rip=%llx rsp_delta=%lld\n",pc,depth,stage,(unsigned long long)c.Rip,(long long)(c.Rsp-(DWORD64)entry));
 check(c.Rip==entry[0] && c.Rsp==(DWORD64)(entry+1),"virtual unwind caller RIP/RSP");
 if(stage>=1) check(c.Rbp==0xaabbccdd,"copied prologue saved RBP");
 if(stage>=2) check(c.Rbx==0x11223344,"copied prologue saved RBX");
}
static unsigned negative_count;
static void reject_unchanged(EntryAdapter *a,EntryParameters p) {
 EntryAdapter before=*a;
 check(!entry_adapter_build(a,&p),"reject unsupported contract");
 check(!memcmp(a,&before,sizeof *a),"failed builder leaves output unpublished and unchanged");
 negative_count++;
}
int main(void) {
 /* Entirely fixture-owned shape and displacement, no game input. */
 unsigned char prologue[]={0x40,0x55,0x53,0x48,0x8d,0xac,0x24,0x20,0,0,0};
 EntryAdapter a; memset(&a,0xa5,sizeof a);
 EntryParameters p={(uintptr_t)fixture_callback,1,prologue,sizeof prologue};
 EntryParameters bad=p; bad.overwrite_size=10; reject_unchanged(&a,bad);
 bad=p; bad.overwrite_size=12; reject_unchanged(&a,bad);
 bad=p; bad.callback=0; reject_unchanged(&a,bad);
 bad=p; bad.continuation=0; reject_unchanged(&a,bad);
 unsigned char unsupported[11]; memcpy(unsupported,prologue,11); unsupported[0]=0xe9;
 bad=p; bad.overwrite=unsupported; reject_unchanged(&a,bad); /* relative branch */
 memcpy(unsupported,prologue,11); unsupported[5]=0x2d;
 reject_unchanged(&a,bad); /* RIP-relative LEA */
 memcpy(unsupported,prologue,11); unsupported[3]=0x90;
 reject_unchanged(&a,bad); /* unsupported opcode */
 memcpy(unsupported,prologue,11); unsupported[7]=0x21;
 reject_unchanged(&a,bad); /* unrepresentable frame offset */
 memcpy(unsupported,prologue,11); unsupported[8]=1;
 reject_unchanged(&a,bad); /* out-of-range frame offset */
 int32_t invalid_displacements[]={-248,248,-201,241,INT32_MIN,INT32_MAX};
 for(unsigned i=0;i<6;i++) { memcpy(unsupported,prologue,11); memcpy(unsupported+7,&invalid_displacements[i],4); bad=p; bad.overwrite=unsupported; reject_unchanged(&a,bad); }
 bad=p; bad.overwrite=NULL; reject_unchanged(&a,bad);
 for(unsigned truncated=0;truncated<10;truncated++) { bad=p; bad.overwrite_size=truncated; reject_unchanged(&a,bad); }
 /* No executable allocation has occurred during any refusal test. */
 const int frames[]={-240,-200,-8,0,8,32,240};
 for(unsigned sample=0;sample<7;sample++) {
 int frame=frames[sample]; memcpy(prologue+7,&frame,4);
 page=VirtualAlloc(NULL,4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE); check(page!=NULL,"allocation");
 p.continuation=(uintptr_t)(page+1024);
 check(entry_adapter_build(&a,&p),"supported fixture build");
 memcpy(page,a.image,sizeof a.image);
 /* Synthetic original remainder: non-epilogue NOP then valid pops/ret. */
 const unsigned char remainder[]={0x90,0x5b,0x5d,0xc3}; memcpy(page+1024,remainder,sizeof remainder);
 const unsigned char uw_remainder[]={1,0,2,0,0,0x30,0,0x50};
 memcpy(page+1088,uw_remainder,sizeof uw_remainder);
 /* Deliberately hazardous transfer, never executed: old mod00 RIP jump. */
 const unsigned char hazard[]={0xff,0x25,0,0,0,0}; memcpy(page+1200,hazard,sizeof hazard);
 memcpy(page+1206,&p.continuation,8);
 RUNTIME_FUNCTION funcs[5]; memcpy(funcs,a.functions,sizeof a.functions);
 funcs[3]=(RUNTIME_FUNCTION){1024,1028,1088};
 funcs[4]=(RUNTIME_FUNCTION){1200,1214,1088};
 DWORD old; check(VirtualProtect(page,4096,PAGE_EXECUTE_READ,&old),"RX protection");
 check(FlushInstructionCache(GetCurrentProcess(),page,1214),"cache flush");
 check(RtlAddFunctionTable(funcs,5,(DWORD64)page),"unwind registration");
 uint64_t out[25]={0}; invoke_fixture(page,out);
 for(unsigned i=0;i<7;i++) check(out[i]==0x11111111ULL*(i+1),"volatile GPR restored");
 check((out[7]&0xcd5)==(0x246&0xcd5),"arithmetic/direction flags restored");
 for(unsigned i=0;i<6;i++) check(out[8+i*2]==0x0123456789abcdefULL && out[9+i*2]==0xfedcba9876543210ULL,"XMM0-5 restored");
 check(seen_return==expected_return,"return-address canary");
 check(out[20]==p.continuation,"only first original home word contains continuation");
 check(out[21]==0x20304050 && out[22]==0x30405060 && out[23]==0x40506070,"remaining original home-word canaries");
 check(out[24]==0x50607010,"first stack-argument canary");
 check(seen_rcx==0x22222222,"raw entry RCX"); check(seen_alignment==8,"callback alignment");
 for(unsigned i=0;i<a.count;i++) {
  EntryPoint x=a.points[i]; unsigned range=x.pc<a.restore_flags?0:x.pc<a.copied?1:2;
  unwind_check(&funcs[range],x.pc,x.depth,x.stage,frame);
 }
 unwind_check(&funcs[3],1024,16,3,frame);
 unwind_check(&funcs[3],1025,16,3,frame);
 unwind_check(&funcs[3],1026,8,1,frame);
 unwind_check(&funcs[3],1027,0,0,frame);
 /* Independent negative unwind control: mod00 jump is misread as epilogue. */
 { uint64_t stack[8]={0}; uint64_t *entry=&stack[4]; entry[0]=0x123456789abcULL; entry[-2]=0x11223344;
 CONTEXT c; memset(&c,0,sizeof c); c.Rsp=(DWORD64)(entry-2); c.Rbp=(DWORD64)((unsigned char*)entry-16+frame); c.Rip=(DWORD64)page+1200;
 PVOID handler=NULL; DWORD64 establisher=0;
 RtlVirtualUnwind(UNW_FLAG_NHANDLER,(DWORD64)page,c.Rip,&funcs[4],&c,&handler,&establisher,NULL);
 check(c.Rip==0x11223344 && c.Rip!=entry[0],"negative control catches RIP-indirect epilogue hazard"); }
 check(RtlDeleteFunctionTable(funcs),"unregister"); check(VirtualFree(page,0,MEM_RELEASE),"release");
 }
 printf("PASS: reusable fixture builder; %u contract refusals before executable allocation\n",negative_count);
 puts("PASS: volatile GPR/XMM0-5/flags, raw RCX, alignment and 32-byte shadow space");
 printf("PASS: copied synthetic 2/1/8 RBP/RBX/LEA prologue and shadow-slot continuation jump; 7 signed displacements including -200, %u unwind checks\n",(a.count+4)*7);
 puts("PASS: return/home/stack-argument canaries; rejected RIP-indirect unwind hazard reproduced separately");
 puts("LIMIT: synthetic only; no raised SEH/native Windows/game/installer/general relocation/teardown proof");
 return 0;
}
