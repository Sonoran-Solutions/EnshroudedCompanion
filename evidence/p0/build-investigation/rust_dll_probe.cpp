#include <windows.h>
#include <cstdio>
using Prepare = bool (*)(const wchar_t*);
using Create = void* (*)(const wchar_t*);
using Update = bool (*)(void*, double);
using Destroy = void (*)(void*);
int wmain(int argc, wchar_t** argv) {
    if (argc != 2) return 2;
    HMODULE m = LoadLibraryW(argv[1]);
    if (!m) { std::printf("load-error=%lu\n", GetLastError()); return 3; }
    auto prepare = reinterpret_cast<Prepare>(GetProcAddress(m, "shroudforge_prepare_startup"));
    auto create = reinterpret_cast<Create>(GetProcAddress(m, "shroudforge_create"));
    auto update = reinterpret_cast<Update>(GetProcAddress(m, "shroudforge_update"));
    auto destroy = reinterpret_cast<Destroy>(GetProcAddress(m, "shroudforge_destroy"));
    if (!prepare || !create || !update || !destroy) return 4;
    if (prepare(nullptr) || create(nullptr) || update(nullptr, 0)) return 5;
    destroy(nullptr);
    if (!FreeLibrary(m)) return 6;
    std::puts("PASS: actual Rust DLL load, four exports, null-input refusal, unload; no game root supplied");
    return 0;
}
