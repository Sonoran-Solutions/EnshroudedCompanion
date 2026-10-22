#include <windows.h>
extern "C" __declspec(dllimport) BOOL __cdecl ShroudforgeStop(DWORD);
int main() { return ShroudforgeStop(0) ? 0 : 2; }
