/* Compile the existing fixture emitter as C, without a second emitter. */
#include "../p0-entry-fixture/entry_adapter.h"
/* Internal symbol. Generated code/table remain owned until process exit. */
int PrepareDiagnosticAdapter(uintptr_t target,uintptr_t callback,const unsigned char original[11],void **output) {
 EntryAdapter adapter;
 EntryParameters p={callback,target+11,original,11};
 if(!entry_adapter_build(&adapter,&p)) return 0;
 uintptr_t origin=target&~(uintptr_t)65535;
 unsigned char *page=NULL;
 for(uintptr_t delta=65536;delta<0x70000000;delta+=65536) {
  uintptr_t choices[2]={origin+delta,origin>delta?origin-delta:0};
  for(unsigned i=0;i<2 && !page;i++) if(choices[i])
   page=VirtualAlloc((void*)choices[i],4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
  if(page) break;
 }
 if(!page) return 0;
 memcpy(page,adapter.image,sizeof adapter.image);
 RUNTIME_FUNCTION *table=(RUNTIME_FUNCTION*)(page+800);
 memcpy(table,adapter.functions,sizeof adapter.functions);
 DWORD old;
 if(!VirtualProtect(page,4096,PAGE_EXECUTE_READ,&old) ||
    !FlushInstructionCache(GetCurrentProcess(),page,adapter.code_end)) { VirtualFree(page,0,MEM_RELEASE); return 0; }
 if(!RtlAddFunctionTable(table,3,(DWORD64)page)) { VirtualFree(page,0,MEM_RELEASE); return 0; }
 *output=page;
 return 1;
}
