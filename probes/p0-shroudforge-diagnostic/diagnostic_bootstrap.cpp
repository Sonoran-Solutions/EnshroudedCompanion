// REVIEW ARTIFACT, NOT INSTALLABLE. No production runtime, profiles or actions.
#include <windows.h>
#include <tlhelp32.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
#include "verify_bootstrap_image.h"
#include "startup_gate.h"
#include "../p0-scheduler-fixture/telemetry_buffer.hpp"
extern "C" int PrepareDiagnosticAdapter(uintptr_t,uintptr_t,const unsigned char[11],void**);
namespace {
constexpr DWORD target_rva=0x2701e0;
constexpr std::array<std::uint8_t,32> entry_digest{
 0xa5,0x6e,0xc9,0xd0,0x1e,0x45,0x7f,0xd7,0xfe,0x42,0x92,0xc7,0xef,0x3a,0xc5,0x31,
 0xdf,0x2a,0x4e,0x21,0x64,0x04,0x12,0x12,0x4b,0x4a,0x43,0x7e,0xd1,0x76,0x0f,0x40};
p0_scheduler_fixture::TelemetryBuffer<8192> observations;
std::atomic<DWORD> phase{0}; // 0 attaching,1 parked,2 installed,3 disabled,4 refused
HANDLE disabled_event{};
HMODULE self{};
IMAGE_DOS_HEADER expected_dos{};
IMAGE_NT_HEADERS64 expected_nt{};
std::array<IMAGE_SECTION_HEADER,96> verified_sections{};
[[noreturn]] void fail_stop() noexcept { TerminateProcess(GetCurrentProcess(),ERROR_DLL_INIT_FAILED); ExitProcess(ERROR_DLL_INIT_FAILED); }
void disable() noexcept { observations.disable(); if(disabled_event) SetEvent(disabled_event); }
void __cdecl observe(void* raw_rcx) noexcept {
 if(!observations.enabled()) return;
 observations.capture(reinterpret_cast<uintptr_t>(raw_rcx),GetCurrentThreadId());
}
bool client_process() {
 wchar_t path[32768]{}; DWORD n=GetModuleFileNameW(nullptr,path,32768);
 if(!n || n>=32768) return false;
 const wchar_t *name=path;
 for(DWORD i=0;i<n;i++) if(path[i]==L'\\' || path[i]==L'/') name=path+i+1;
 return CompareStringOrdinal(name,-1,L"enshrouded.exe",-1,TRUE)==CSTR_EQUAL;
}
bool protected_original(unsigned char (&original)[64]) {
 // File is already SHA/size verified and held without write/delete sharing.
 HANDLE mapping=CreateFileMappingW(StrictBuildIdentity::verified_file,nullptr,PAGE_READONLY,0,0,nullptr);
 if(!mapping) return false;
 auto *disk=static_cast<const unsigned char*>(MapViewOfFile(mapping,FILE_MAP_READ,0,0,0));
 bool ok=false;
 if(disk) {
  const auto *dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(disk);
  if(dos->e_magic==IMAGE_DOS_SIGNATURE && dos->e_lfanew>0 && dos->e_lfanew<=4096) {
   auto *nt=reinterpret_cast<const IMAGE_NT_HEADERS64*>(disk+dos->e_lfanew);
   if(nt->Signature==IMAGE_NT_SIGNATURE && nt->FileHeader.NumberOfSections<=96) {
    expected_dos=*dos; expected_nt=*nt;
    const auto *sections=IMAGE_FIRST_SECTION(nt);
    memcpy(verified_sections.data(),sections,nt->FileHeader.NumberOfSections*sizeof(IMAGE_SECTION_HEADER));
    for(unsigned i=0;i<nt->FileHeader.NumberOfSections;i++) {
     const auto &s=sections[i];
     if(!(s.Characteristics&IMAGE_SCN_MEM_EXECUTE) || target_rva<s.VirtualAddress) continue;
     uint64_t delta=target_rva-s.VirtualAddress;
     uint64_t offset=uint64_t(s.PointerToRawData)+delta;
     if(delta+64>s.SizeOfRawData || offset+64>uint64_t(StrictBuildIdentity::approved_file_size)) continue;
     memcpy(original,disk+offset,64);
     StrictBuildIdentity::Sha256 hash; hash.update(original,11);
     int32_t displacement{}; memcpy(&displacement,original+7,4);
     ok=hash.finish()==entry_digest && displacement==-200;
    }
   }
  }
  UnmapViewOfFile(disk);
 }
 CloseHandle(mapping); return ok;
}
bool unique_loaded_match(const unsigned char (&original)[64],unsigned char* base) {
 IMAGE_DOS_HEADER loaded_dos{}; IMAGE_NT_HEADERS64 loaded_nt{};
 std::array<IMAGE_SECTION_HEADER,96> loaded_sections{}; SIZE_T read{};
 auto read_exact=[&](const void* from,void* to,size_t count) { return ReadProcessMemory(GetCurrentProcess(),from,to,count,&read) && read==count; };
 const size_t section_count=expected_nt.FileHeader.NumberOfSections;
 const size_t section_offset=expected_dos.e_lfanew+offsetof(IMAGE_NT_HEADERS64,OptionalHeader)+expected_nt.FileHeader.SizeOfOptionalHeader;
 if(!section_count || section_count>96 || section_offset+section_count*sizeof(IMAGE_SECTION_HEADER)>StrictBuildIdentity::approved_image_size) return false;
 if(!read_exact(base,&loaded_dos,sizeof loaded_dos) || memcmp(&loaded_dos,&expected_dos,sizeof loaded_dos) ||
    !read_exact(base+expected_dos.e_lfanew,&loaded_nt,sizeof loaded_nt) || memcmp(&loaded_nt,&expected_nt,sizeof loaded_nt) ||
    !read_exact(base+section_offset,loaded_sections.data(),section_count*sizeof(IMAGE_SECTION_HEADER)) ||
    memcmp(loaded_sections.data(),verified_sections.data(),section_count*sizeof(IMAGE_SECTION_HEADER))) return false;
 const auto *sections=verified_sections.data();
 unsigned matches=0; uintptr_t match=0;
 std::array<unsigned char,65536+10> buffer{};
 for(size_t i=0;i<section_count;i++) {
  const auto &s=sections[i];
  if(!(s.Characteristics&IMAGE_SCN_MEM_EXECUTE)) continue;
  const size_t size=s.SizeOfRawData;
  if(uint64_t(s.VirtualAddress)+size>StrictBuildIdentity::approved_image_size) return false;
  for(size_t offset=0;offset<size;offset+=65536) {
   size_t amount=(size-offset<buffer.size()?size-offset:buffer.size()); SIZE_T read{};
   if(!ReadProcessMemory(GetCurrentProcess(),base+s.VirtualAddress+offset,buffer.data(),amount,&read) || read!=amount) return false;
   const size_t starts=(size-offset<65536?size-offset:65536);
   for(size_t j=0;j<starts && j+11<=amount;j++) if(!memcmp(buffer.data()+j,original,11)) {
    matches++; match=reinterpret_cast<uintptr_t>(base+s.VirtualAddress+offset+j);
    if(matches>1) return false;
   }
  }
 }
 std::array<unsigned char,64> loaded_entry{};
 return matches==1 && match==reinterpret_cast<uintptr_t>(base+target_rva) && read_exact(base+target_rva,loaded_entry.data(),loaded_entry.size()) && !memcmp(loaded_entry.data(),original,64);
}
// One attempt, preserving the reviewed source writer's handle-before-suspend
// and RIP exclusion checks. No allocations occur while peers are suspended.
bool publish_patch(unsigned char* address,const unsigned char (&original)[64],void* adapter) {
 intptr_t distance=reinterpret_cast<intptr_t>(adapter)-reinterpret_cast<intptr_t>(address+5);
 if(distance<INT32_MIN || distance>INT32_MAX) return false;
 unsigned char replacement[11]; memset(replacement,0x90,sizeof replacement); replacement[0]=0xe9;
 int32_t relative=static_cast<int32_t>(distance); memcpy(replacement+1,&relative,4);
 std::vector<HANDLE> threads; threads.reserve(256);
 HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);
 if(snapshot==INVALID_HANDLE_VALUE) return false;
 THREADENTRY32 entry{sizeof entry}; bool prepared=Thread32First(snapshot,&entry)!=FALSE;
 if(prepared) do {
  if(entry.th32OwnerProcessID!=GetCurrentProcessId() || entry.th32ThreadID==GetCurrentThreadId()) continue;
  HANDLE thread=OpenThread(THREAD_SUSPEND_RESUME|THREAD_GET_CONTEXT|THREAD_QUERY_INFORMATION,FALSE,entry.th32ThreadID);
  if(!thread) { prepared=false; break; }
  threads.push_back(thread);
 } while(Thread32Next(snapshot,&entry));
 if(prepared && GetLastError()!=ERROR_NO_MORE_FILES) prepared=false;
 CloseHandle(snapshot);
 size_t suspended=0; bool safe=prepared;
 for(HANDLE thread:threads) {
  if(!safe) break;
  if(SuspendThread(thread)==DWORD(-1)) { safe=false; break; }
  suspended++; CONTEXT context{}; context.ContextFlags=CONTEXT_CONTROL;
  if(!GetThreadContext(thread,&context) || (context.Rip>=reinterpret_cast<uintptr_t>(address) && context.Rip<reinterpret_cast<uintptr_t>(address+11))) safe=false;
 }
 bool written=false; DWORD old{};
 if(safe && !memcmp(address,original,64) && VirtualProtect(address,11,PAGE_READWRITE,&old)) {
  memcpy(address,replacement,11);
  DWORD ignored{};
  if(!VirtualProtect(address,11,old,&ignored) || !FlushInstructionCache(GetCurrentProcess(),address,11)) fail_stop();
  written=true;
 }
 for(size_t i=0;i<suspended;i++) if(ResumeThread(threads[i])==DWORD(-1)) fail_stop();
 for(HANDLE thread:threads) CloseHandle(thread);
 return written;
}
struct Sample { ULONGLONG elapsed{}; size_t reserved{},dropped{},ready{},unique_threads{}; };
std::array<Sample,120> samples{};
size_t sample_count{};
Sample after_disable{};
Sample sample(ULONGLONG start) noexcept {
 Sample result{GetTickCount64()-start,observations.reserved(),observations.dropped(),0,0};
 std::array<std::uint64_t,8192> ids{};
 for(size_t i=0;i<result.reserved;i++) { p0_scheduler_fixture::TelemetryBuffer<8192>::Observation value{};
  if(observations.read(i,value)) ids[result.ready++]=value.thread_id; }
 std::sort(ids.begin(),ids.begin()+result.ready);
 for(size_t i=0;i<result.ready;i++) if(!i || ids[i]!=ids[i-1]) result.unique_threads++;
 return result;
}
void summary() noexcept {
 // Unique CREATE_NEW file beside this diagnostic DLL. Never raw pointer output.
 wchar_t path[32768]{}; DWORD n=GetModuleFileNameW(self,path,32768);
 if(!n || n>=32768) return;
 while(n && path[n-1]!=L'\\' && path[n-1]!=L'/') n--;
 SYSTEMTIME t{}; GetSystemTime(&t);
 int count=swprintf(path+n,32768-n,L"shroudforge-p0-%lu-%04u%02u%02u-%02u%02u%02u-%03u.json",GetCurrentProcessId(),t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,t.wMilliseconds);
 if(count<=0) return;
 HANDLE file=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
 if(file==INVALID_HANDLE_VALUE) return;
 std::array<char,32768> body{}; size_t used=0;
 auto append=[&](const char* format,auto... values) { if(used>=body.size()) return; int n=snprintf(body.data()+used,body.size()-used,format,values...); if(n<=0 || size_t(n)>=body.size()-used) used=body.size(); else used+=size_t(n); };
 append("%s","{\"diagnosticOnly\":true,\"disabled\":true,\"capacity\":8192,\"samples\":[");
 for(size_t i=0;i<sample_count;i++) { const auto &v=samples[i];
  append("%s{\"elapsedMs\":%llu,\"reserved\":%zu,\"dropped\":%zu,\"published\":%zu,\"uniqueThreads\":%zu}",i?",":"",v.elapsed,v.reserved,v.dropped,v.ready,v.unique_threads); }
 append("],\"afterDisable\":{\"elapsedMs\":%llu,\"reserved\":%zu,\"dropped\":%zu,\"published\":%zu,\"uniqueThreads\":%zu},\"quiescenceProven\":false,\"pendingCapturesMayFinish\":true}\n",after_disable.elapsed,after_disable.reserved,after_disable.dropped,after_disable.ready,after_disable.unique_threads);
 if(used<body.size()) { DWORD written{}; WriteFile(file,body.data(),DWORD(used),&written,nullptr); }
 CloseHandle(file);
}
DWORD WINAPI run(void*) noexcept {
 if(WaitForSingleObject(StartupGate::entered,30000)!=WAIT_OBJECT_0) { phase=4; StartupGate::Complete(false); return 1; }
 phase=1;
 HMODULE pinned{};
 if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(&run),&pinned)) { phase=4; StartupGate::Complete(false); return 1; }
 unsigned char original[64]{};
 auto *base=reinterpret_cast<unsigned char*>(GetModuleHandleW(nullptr)); void *adapter{};
 bool success=false;
 try { success=protected_original(original) && unique_loaded_match(original,base) && PrepareDiagnosticAdapter(reinterpret_cast<uintptr_t>(base+target_rva),reinterpret_cast<uintptr_t>(&observe),original,&adapter) && unique_loaded_match(original,base) && publish_patch(base+target_rva,original,adapter); } catch(...) { success=false; }
 if(!success) { disable(); phase=4; StartupGate::Complete(false); return 1; }
 phase=2; StartupGate::Complete(true);
 const ULONGLONG started=GetTickCount64();
 for(unsigned second=0;second<120;second++) {
  DWORD result=WaitForSingleObject(disabled_event,1000);
  samples[sample_count++]=sample(started);
  if(result!=WAIT_TIMEOUT || GetTickCount64()-started>=120000) break;
 }
 disable(); phase=3; Sleep(100); after_disable=sample(started); summary();
 return 0;
}
}
extern "C" __declspec(dllexport) DWORD __cdecl ShroudforgeDiagnosticStatus() noexcept { return phase.load(std::memory_order_acquire); }
extern "C" __declspec(dllexport) void __cdecl ShroudforgeDiagnosticDisable() noexcept { disable(); }
BOOL APIENTRY DllMain(HMODULE module,DWORD reason,LPVOID) {
 if(reason!=DLL_PROCESS_ATTACH) return TRUE;
 self=module; DisableThreadLibraryCalls(module);
 if(!client_process()) return TRUE;
 if(!StrictBuildIdentity::VerifyProcessImage()) return FALSE;
 disabled_event=CreateEventW(nullptr,TRUE,FALSE,nullptr);
 if(!disabled_event) { StrictBuildIdentity::Close(); return FALSE; }
 if(!StartupGate::Install()) { StartupGate::CancelInstallation(); CloseHandle(disabled_event); disabled_event=nullptr; StrictBuildIdentity::Close(); return FALSE; }
 HANDLE worker=CreateThread(nullptr,0,run,nullptr,0,nullptr);
 if(!worker) { StartupGate::CancelInstallation(); CloseHandle(disabled_event); disabled_event=nullptr; StrictBuildIdentity::Close(); return FALSE; }
 CloseHandle(worker); return TRUE;
}
