// Separate synthetic EXE only. No test override exists in the diagnostic DLL.
#include "diagnostic_bootstrap.cpp"
static void require(bool ok,const char* why) { if(!ok) { fprintf(stderr,"FAIL: %s\n",why); ExitProcess(1); } }
static DWORD WINAPI idle_thread(void*) { return 0; }
int main() {
 // Model loaded headers/sections in a private synthetic allocation.
 auto *image=static_cast<unsigned char*>(VirtualAlloc(nullptr,target_rva+4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
 require(image!=nullptr,"synthetic image allocation");
 expected_dos={}; expected_dos.e_magic=IMAGE_DOS_SIGNATURE; expected_dos.e_lfanew=128;
 expected_nt={}; expected_nt.Signature=IMAGE_NT_SIGNATURE; expected_nt.FileHeader.NumberOfSections=1;
 expected_nt.FileHeader.SizeOfOptionalHeader=sizeof(IMAGE_OPTIONAL_HEADER64);
 verified_sections={}; verified_sections[0].VirtualAddress=target_rva;
 verified_sections[0].SizeOfRawData=128; verified_sections[0].Characteristics=IMAGE_SCN_MEM_EXECUTE;
 memcpy(image,&expected_dos,sizeof expected_dos); memcpy(image+128,&expected_nt,sizeof expected_nt);
 memcpy(image+128+offsetof(IMAGE_NT_HEADERS64,OptionalHeader)+sizeof(IMAGE_OPTIONAL_HEADER64),verified_sections.data(),sizeof(IMAGE_SECTION_HEADER));
 unsigned char original[64]={0x40,0x55,0x53,0x48,0x8d,0xac,0x24,0x38,0xff,0xff,0xff};
 require(!unique_loaded_match(original,image),"missing loaded site refuses");
 memcpy(image+target_rva,original,11); require(unique_loaded_match(original,image),"unique exact synthetic site");
 memcpy(image+target_rva+32,original,11); require(!unique_loaded_match(original,image),"duplicate loaded site refuses");
 memset(image+target_rva+32,0,11);
 image[128+4+2]=2; require(!unique_loaded_match(original,image),"changed loaded metadata refuses");
 memcpy(image+128,&expected_nt,sizeof expected_nt);
 // A synthetic function with the accepted prologue and original remainder.
 auto *target=image+target_rva;
 const unsigned char remainder[]={0xb8,7,0,0,0,0x5b,0x5d,0xc3}; memcpy(target+11,remainder,sizeof remainder); memcpy(original+11,remainder,sizeof remainder);
 DWORD old{}; require(VirtualProtect(target,128,PAGE_EXECUTE_READ,&old),"synthetic target RX");
 require(FlushInstructionCache(GetCurrentProcess(),target,128),"synthetic cache flush");
 require(reinterpret_cast<int(*)()>(target)()==7,"original synthetic function");
 void *adapter{};
 require(PrepareDiagnosticAdapter(reinterpret_cast<uintptr_t>(target),reinterpret_cast<uintptr_t>(&observe),original,&adapter),"prepare shared adapter");
 DWORD64 unwind_base{}; require(RtlLookupFunctionEntry(reinterpret_cast<DWORD64>(adapter),&unwind_base,nullptr)!=nullptr,"prepared unwind registered before publication");
 unsigned char wrong[64]; memcpy(wrong,original,64); wrong[0]^=1;
 require(!publish_patch(target,wrong,adapter) && !memcmp(target,original,11),"changed expected bytes refuses without write");
 DWORD restore{}; require(VirtualProtect(target,64,PAGE_READWRITE,&restore),"continuation mutation fixture RW");
 target[20]^=1;
 require(VirtualProtect(target,64,PAGE_EXECUTE_READ,&restore),"continuation mutation fixture RX");
 require(!publish_patch(target,original,adapter) && target[20]==1 && !memcmp(target,original,11),"changed continuation refuses without target patch");
 require(VirtualProtect(target,64,PAGE_READWRITE,&restore),"continuation restore RW"); target[20]^=1;
 require(VirtualProtect(target,64,PAGE_EXECUTE_READ,&restore),"continuation restore RX");
 // Force the saved RIP of an owned, never-started suspended thread into range.
 HANDLE thread=CreateThread(nullptr,0,idle_thread,nullptr,CREATE_SUSPENDED,nullptr); require(thread!=nullptr,"owned suspended thread");
 CONTEXT context{}; context.ContextFlags=CONTEXT_CONTROL; require(GetThreadContext(thread,&context),"owned thread context");
 CONTEXT occupied=context; occupied.Rip=reinterpret_cast<DWORD64>(target); require(SetThreadContext(thread,&occupied),"set synthetic occupied RIP");
 require(!publish_patch(target,original,adapter) && !memcmp(target,original,11),"occupied RIP refuses without write");
 require(SetThreadContext(thread,&context),"restore owned thread RIP");
 require(ResumeThread(thread)==1,"preserve original suspend count");
 require(WaitForSingleObject(thread,5000)==WAIT_OBJECT_0,"owned thread completes"); CloseHandle(thread);
 require(publish_patch(target,original,adapter),"synthetic publication succeeds");
 require(reinterpret_cast<int(*)()>(target)()==7,"published shared adapter preserves original remainder result");
 const auto before_disable=sample(GetTickCount64()); require(before_disable.ready>=1 && before_disable.unique_threads==1,"bounded count-only sample");
 disable(); require(!observations.enabled(),"one-way capture disable");
 require(reinterpret_cast<int(*)()>(target)()==7,"disabled adapter remains callable");
 require(RtlLookupFunctionEntry(reinterpret_cast<DWORD64>(adapter),&unwind_base,nullptr)!=nullptr,"process-lifetime unwind retained after disable");
 require(sample(GetTickCount64()).reserved==before_disable.reserved,"disabled synthetic calls do not reserve slots");
 puts("PASS: missing/duplicate/metadata/expected-byte refusal; occupied RIP refusal and suspend-count preservation");
 puts("PASS: shared adapter synthetic publication/execution, one-way disable and retained unwind storage");
 puts("LIMIT: no positive exact-image admission, game, real exception, timeout failure injection or DLL lifetime proof");
 return 0;
}
