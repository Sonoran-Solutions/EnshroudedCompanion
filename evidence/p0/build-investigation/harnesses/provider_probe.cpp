#include <windows.h>
#include <stdint.h>
#include <string.h>
using AbiFn = uint32_t (__cdecl*)();
using InitFn = bool (__cdecl*)();
using StatusFn = void (__cdecl*)(char*, size_t);
using ShutdownFn = void (__cdecl*)();
int main() {
    wchar_t path[32768]{};
    DWORD n = GetFullPathNameW(L"shroudforge\\kfc-runtime.dll", 32768, path, nullptr);
    if (!n || n >= 32768) return 10;
    HMODULE m = LoadLibraryExW(path, nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (!m) return 11;
    auto abi = reinterpret_cast<AbiFn>(GetProcAddress(m, "KfcRuntimeAbi"));
    auto init = reinterpret_cast<InitFn>(GetProcAddress(m, "KfcRuntimeInitialize"));
    auto status = reinterpret_cast<StatusFn>(GetProcAddress(m, "KfcRuntimeStatus"));
    auto shutdown = reinterpret_cast<ShutdownFn>(GetProcAddress(m, "KfcRuntimeShutdown"));
    if (!abi || !init || !status || !shutdown || abi() != 12) return 12;
    bool accepted = init();
    char text[512]{};
    status(text, sizeof(text));
    DWORD written{};
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), text, (DWORD)strlen(text), &written, nullptr);
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), "\n", 1, &written, nullptr);
    const bool refused_unknown_image =
        strstr(text, "missing-exact-sha256-profile") != nullptr;
    const bool actor_hooks_inactive =
        strstr(text, "actor-placement=0/0") != nullptr;
    const bool building_hooks_inactive =
        strstr(text, "building-dispatch=0/0") != nullptr;
    shutdown();
    FreeLibrary(m);
    if (accepted) return 13;
    if (!refused_unknown_image) return 14;
    if (!actor_hooks_inactive || !building_hooks_inactive) return 15;
    return 0;
}
