#!/usr/bin/env bash
set -euo pipefail
TASK_ROOT=/tmp/enshrouded-p0-toolchain/root
TASK_SOURCE=/tmp/enshrouded-p0-source-review/shroudforge
TASK_BOOT="$TASK_SOURCE/modules/entrypoints/windows-bootstrap"
TASK_LOCAL="$(cd "$(dirname "$0")" && pwd)"
TASK_OUT=/tmp/enshrouded-p0-toolchain/diagnostic-review
TASK_CXX="$TASK_ROOT/usr/lib/llvm-21/bin/clang++"
TASK_WINE='/home/dq/.local/share/Steam/steamapps/common/Proton - Experimental/files/bin/wine'
TASK_SERVER='/home/dq/.local/share/Steam/steamapps/common/Proton - Experimental/files/bin/wineserver'
TASK_TEMP="$(mktemp -d /tmp/p0-diagnostic-refusal.XXXXXX)"
cleanup() {
 timeout 10 env WINEPREFIX="$TASK_TEMP/prefix" WINEDEBUG=-all "$TASK_SERVER" -k >/dev/null 2>&1 || true
 timeout 10 env WINEPREFIX="$TASK_TEMP/prefix" WINEDEBUG=-all "$TASK_SERVER" -w >/dev/null 2>&1 || true
 rm -rf "$TASK_TEMP"
}
trap cleanup EXIT
mkdir -p "$TASK_OUT"
export LD_LIBRARY_PATH="$TASK_ROOT/usr/lib/x86_64-linux-gnu:$TASK_ROOT/usr/lib/llvm-21/lib"
FLAGS=(--target=x86_64-w64-windows-gnu --sysroot="$TASK_ROOT/usr/x86_64-w64-mingw32" --ld-path="$TASK_ROOT/usr/bin/x86_64-w64-mingw32-ld" -L"$TASK_ROOT/usr/lib/gcc/x86_64-w64-mingw32/13-posix" -L"$TASK_ROOT/usr/x86_64-w64-mingw32/lib")
INCLUDES=(-isystem "$TASK_ROOT/usr/lib/gcc/x86_64-w64-mingw32/13-posix/include/c++" -isystem "$TASK_ROOT/usr/lib/gcc/x86_64-w64-mingw32/13-posix/include/c++/x86_64-w64-mingw32" -isystem "$TASK_ROOT/usr/lib/gcc/x86_64-w64-mingw32/13-posix/include/c++/backward" -isystem "$TASK_ROOT/usr/x86_64-w64-mingw32/include")
{
 printf 'REVIEW ONLY — NOT_INSTALLABLE\n'
 git -C "$TASK_SOURCE" rev-parse HEAD
 "$TASK_CXX" --version
 "$TASK_ROOT/usr/bin/x86_64-w64-mingw32-ld" --version
 env WINEPREFIX="$TASK_TEMP/prefix" WINEDEBUG=-all "$TASK_WINE" --version
 sha256sum "$TASK_BOOT/verify_bootstrap_image.h" "$TASK_BOOT/startup_gate.h" "$TASK_BOOT/sha256_digest.h" "$TASK_BOOT/winmm_proxy.cpp" "$TASK_LOCAL/../p0-entry-fixture/entry_adapter.h" "$TASK_LOCAL/../p0-scheduler-fixture/telemetry_buffer.hpp" "$TASK_LOCAL/diagnostic_bootstrap.cpp" "$TASK_LOCAL/prepare_entry_adapter.c" "$TASK_LOCAL/refusal_host.cpp" "$TASK_LOCAL/test_admission_and_publication.cpp" "$TASK_LOCAL/build-review.sh"
} > "$TASK_OUT/build-identity.txt"
python3 - "$TASK_BOOT/winmm_proxy.cpp" "$TASK_OUT/winmm.def" <<'PY'
import re,sys
with open(sys.argv[2],'w') as out:
 out.write('LIBRARY winmm\nEXPORTS\n')
 count=0
 for line in open(sys.argv[1]):
  m=re.search(r'#pragma comment\(linker,"/export:([^=]+)=([^,]+),@(\d+)"\)',line,re.I)
  if not m: continue
  name,target,ordinal=m.groups(); target=target.replace('\\\\','\\')
  out.write(f' {name}="{target}" @{ordinal}\n'); count+=1
 assert count==180,count
PY
timeout 60 "$TASK_ROOT/usr/lib/llvm-21/bin/clang" --target=x86_64-w64-windows-gnu --sysroot="$TASK_ROOT/usr/x86_64-w64-mingw32" -Wall -Wextra -Werror -O2 -std=c11 -c "$TASK_LOCAL/prepare_entry_adapter.c" -o "$TASK_OUT/adapter.o"
timeout 60 "$TASK_CXX" "${FLAGS[@]}" "${INCLUDES[@]}" -I"$TASK_BOOT" -DNOMINMAX -DWIN32_LEAN_AND_MEAN -Wall -Wextra -Werror -Wno-missing-field-initializers -O2 -std=gnu++20 -shared -static -pthread -Wl,--exclude-all-symbols -Wl,--out-implib,"$TASK_OUT/libdiagnostic.a" "$TASK_LOCAL/diagnostic_bootstrap.cpp" "$TASK_OUT/adapter.o" "$TASK_OUT/winmm.def" -o "$TASK_OUT/winmm-diagnostic-NOT_INSTALLABLE.dll"
"$TASK_ROOT/usr/bin/x86_64-w64-mingw32-objdump" -p "$TASK_OUT/winmm-diagnostic-NOT_INSTALLABLE.dll" > "$TASK_OUT/pe-inspection.txt"
python3 - "$TASK_OUT/pe-inspection.txt" "$TASK_BOOT/winmm_proxy.cpp" <<'PYVERIFY'
import re,sys
pe=open(sys.argv[1]).read(); source=open(sys.argv[2]).read()
forwarders=re.findall(r'Forwarder RVA -- (.+)',pe)
expected=[m.group(1).replace('\\\\','\\') for m in re.finditer(r'#pragma comment\(linker,"/export:[^=]+=([^,]+),@\d+"\)',source)]
assert len(forwarders)==180 and sorted(forwarders)==sorted(expected)
part=pe.split('[Ordinal/Name Pointer] Table',1)[1].split('The Function Table',1)[0]
names=re.findall(r'\[\s*\d+\] \+base\[\s*\d+\]\s+[0-9a-f]+ (\w+)',part)
assert len(names)==182,len(names)
assert set(names)-set(re.findall(r'/export:([^=]+)=',source))=={'ShroudforgeDiagnosticDisable','ShroudforgeDiagnosticStatus'}
imports=re.findall(r'DLL Name: (.+)',pe)
assert set(imports)=={'KERNEL32.dll','msvcrt.dll'},imports
print('PASS: 180 exact system32 forwarders; only typed disable/status additional exports; system DLL imports only')
PYVERIFY
sha256sum "$TASK_OUT/winmm-diagnostic-NOT_INSTALLABLE.dll" >> "$TASK_OUT/build-identity.txt"
timeout 60 "$TASK_CXX" "${FLAGS[@]}" -O2 -static "$TASK_LOCAL/refusal_host.cpp" "$TASK_OUT/libdiagnostic.a" -o "$TASK_TEMP/safe_host.exe"
cp "$TASK_TEMP/safe_host.exe" "$TASK_TEMP/enshrouded.exe"
cp "$TASK_OUT/winmm-diagnostic-NOT_INSTALLABLE.dll" "$TASK_TEMP/winmm.dll"
set +e
(cd "$TASK_TEMP" && timeout 60 env WINEPREFIX="$TASK_TEMP/prefix" WINEDEBUG=-all WINEDLLOVERRIDES=winmm=n,b "$TASK_WINE" safe_host.exe) > "$TASK_OUT/safe-host.txt" 2>&1
safe=$?
(cd "$TASK_TEMP" && timeout 60 env WINEPREFIX="$TASK_TEMP/prefix" WINEDEBUG=-all WINEDLLOVERRIDES=winmm=n,b "$TASK_WINE" enshrouded.exe) > "$TASK_OUT/wrong-image.txt" 2>&1
wrong=$?
set -e
test "$safe" -eq 0
test "$wrong" -eq 66
printf 'PASS: unrelated synthetic host inert exit=%s; named wrong-image host refused exit=%s\n' "$safe" "$wrong"
timeout 60 "$TASK_CXX" "${FLAGS[@]}" "${INCLUDES[@]}" -I"$TASK_BOOT" -DNOMINMAX -DWIN32_LEAN_AND_MEAN -Wall -Wextra -Werror -Wno-missing-field-initializers -O2 -std=gnu++20 -static -pthread "$TASK_LOCAL/test_admission_and_publication.cpp" "$TASK_OUT/adapter.o" -o "$TASK_TEMP/admission-fixture.exe"
(cd "$TASK_TEMP" && timeout 60 env WINEPREFIX="$TASK_TEMP/prefix" WINEDEBUG=-all "$TASK_WINE" admission-fixture.exe) > "$TASK_OUT/admission-fixture.txt" 2>&1
cat "$TASK_OUT/admission-fixture.txt"
printf 'Artifact: %s\n'  "$TASK_OUT/winmm-diagnostic-NOT_INSTALLABLE.dll"
