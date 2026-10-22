#include <windows.h>
extern "C" __declspec(dllimport) DWORD __cdecl ShroudforgeDiagnosticStatus();
extern "C" __declspec(dllimport) void __cdecl ShroudforgeDiagnosticDisable();
int main() { if(ShroudforgeDiagnosticStatus()!=0) return 2; ShroudforgeDiagnosticDisable(); return 0; }
