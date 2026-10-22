#!/usr/bin/env bash
set -euo pipefail
ROOT=/tmp/enshrouded-p0-toolchain/root
SRC=/tmp/enshrouded-p0-source-review/shroudforge/modules/entrypoints/windows-bootstrap
OUT=/tmp/enshrouded-p0-toolchain/bootstrap-objects
CXX="$ROOT/usr/lib/llvm-21/bin/clang++"
export LD_LIBRARY_PATH="$ROOT/usr/lib/x86_64-linux-gnu:$ROOT/usr/lib/llvm-21/lib"
mkdir -p "$OUT"
for unit in startup module_processes runtime_lifecycle logging winmm_proxy runtime_bridge; do
  timeout 30s "$CXX" --target=x86_64-w64-windows-gnu \
    --sysroot="$ROOT/usr/x86_64-w64-mingw32" -std=gnu++20 -O2 \
    -DNOMINMAX -DWIN32_LEAN_AND_MEAN -fms-extensions \
    -isystem "$ROOT/usr/lib/gcc/x86_64-w64-mingw32/13-posix/include/c++" \
    -isystem "$ROOT/usr/lib/gcc/x86_64-w64-mingw32/13-posix/include/c++/x86_64-w64-mingw32" \
    -isystem "$ROOT/usr/lib/gcc/x86_64-w64-mingw32/13-posix/include/c++/backward" \
    -isystem "$ROOT/usr/x86_64-w64-mingw32/include" \
    -isystem /tmp/enshrouded-p0-toolchain/json-src/single_include \
    -I"$SRC" -I/tmp/enshrouded-p0-source-review/shroudforge/modules/game-runtime/native \
    -c "$SRC/$unit.cpp" -o "$OUT/$unit.obj"
done
# Preserve the absolute system32 winmm forwarder path from every source pragma.
python3 - <<'PY'
import re
src='/tmp/enshrouded-p0-source-review/shroudforge/modules/entrypoints/windows-bootstrap/winmm_proxy.cpp'
out='/tmp/enshrouded-p0-toolchain/winmm_exports_quoted.def'
with open(out,'w') as f:
    f.write('LIBRARY winmm\nEXPORTS\n')
    count=0
    for line in open(src):
        m=re.search(r'#pragma comment\(linker,"/export:([^=]+)=([^,]+),@(\d+)"\)',line,re.I)
        if not m: continue
        name,target,ordinal=m.groups()
        target=target.replace('\\\\','\\')
        f.write(f'  {name}="{target}" @{ordinal}\n')
        count+=1
    assert count == 180, f'expected 180 forwarders, found {count}'
PY
timeout 30s "$CXX" --target=x86_64-w64-windows-gnu \
  --sysroot="$ROOT/usr/x86_64-w64-mingw32" \
  --ld-path="$ROOT/usr/bin/x86_64-w64-mingw32-ld" \
  -L"$ROOT/usr/lib/gcc/x86_64-w64-mingw32/13-posix" \
  -L"$ROOT/usr/x86_64-w64-mingw32/lib" -pthread -shared -static \
  -Wl,--subsystem,windows \
  -Wl,--out-implib,"$OUT/libwinmm-quoted.a" \
  -o "$OUT/winmm-quoted.dll" \
  "$OUT/startup.obj" "$OUT/module_processes.obj" "$OUT/runtime_lifecycle.obj" \
  "$OUT/logging.obj" "$OUT/winmm_proxy.obj" "$OUT/runtime_bridge.obj" \
  /tmp/enshrouded-p0-toolchain/winmm_exports_quoted.def \
  -luser32 -lshell32 -lole32 -luuid -lshlwapi
