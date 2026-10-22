#include <windows.h>
#include <cstdio>
#include <cstring>
extern "C" __declspec(dllimport) DWORD __cdecl ShroudforgeDiagnosticStatus();
extern "C" __declspec(dllimport) void __cdecl ShroudforgeDiagnosticDisable();
extern "C" int p0_composition_entry();
int main(int argc,char** argv) {
 const bool wait_expiry=argc==2 && strcmp(argv[1],"--wait-expiry")==0;
 if(argc>1 && !wait_expiry) return 9;
 const ULONGLONG started=GetTickCount64();
 if(ShroudforgeDiagnosticStatus()!=2) return 10;
 for(unsigned i=0;i<10;i++) if(p0_composition_entry()!=7) return 11;
 if(!wait_expiry) ShroudforgeDiagnosticDisable();
 const ULONGLONG limit=wait_expiry?130000:5000;
 while(ShroudforgeDiagnosticStatus()!=3 && GetTickCount64()-started<limit) Sleep(20);
 const ULONGLONG elapsed=GetTickCount64()-started;
 if(wait_expiry && elapsed<119000) return 16;
 if(ShroudforgeDiagnosticStatus()!=3 || p0_composition_entry()!=7) return 12;
 Sleep(300);
 wchar_t pattern[96]{}; swprintf(pattern,96,L"shroudforge-p0-%lu-*.json",GetCurrentProcessId());
 WIN32_FIND_DATAW found{}; HANDLE search=FindFirstFileW(pattern,&found);
 if(search==INVALID_HANDLE_VALUE) return 13; FindClose(search);
 HANDLE file=CreateFileW(found.cFileName,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);
 if(file==INVALID_HANDLE_VALUE) return 14;
 char text[32768]{}; DWORD read{}; BOOL ok=ReadFile(file,text,sizeof text-1,&read,nullptr); CloseHandle(file);
 if(!ok || !strstr(text,"\"reserved\":10") || !strstr(text,"\"quiescenceProven\":false")) return 15;
 const char* after=strstr(text,"\"afterDisable\":{\"elapsedMs\":");
 unsigned long long after_ms=0;
 if(!after || sscanf(after+strlen("\"afterDisable\":{\"elapsedMs\":"),"%llu",&after_ms)!=1) return 17;
 if(wait_expiry && after_ms<120000) return 18;
 if(wait_expiry) printf("PASS: 120-second automatic disable without explicit disable; mainElapsedMs=%llu afterDisableElapsedMs=%llu; continuation and count-only summary\n",elapsed,after_ms);
 else puts("PASS: synthetic-only identity StartupGate/hook composition, 10 captures, disable, continuation and count-only summary");
 printf("SUMMARY_FILE: %ls\n",found.cFileName);
 return 0;
}
