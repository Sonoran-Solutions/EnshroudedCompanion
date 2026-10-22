#!/usr/bin/env bash
# Separate compile-time synthetic identity. No override is linked into production.
set -euo pipefail
TASK_ROOT=/tmp/enshrouded-p0-toolchain/root
TASK_SOURCE=/tmp/enshrouded-p0-source-review/shroudforge/modules/entrypoints/windows-bootstrap
TASK_LOCAL="$(cd "$(dirname "$0")" && pwd)"
TASK_OUT=/tmp/enshrouded-p0-toolchain/diagnostic-review
TASK_TEMP="$(mktemp -d /tmp/p0-diagnostic-composition.XXXXXX)"
TASK_WINE='/home/dq/.local/share/Steam/steamapps/common/Proton - Experimental/files/bin/wine'
TASK_SERVER='/home/dq/.local/share/Steam/steamapps/common/Proton - Experimental/files/bin/wineserver'
cleanup() {
 timeout 10 env WINEPREFIX="$TASK_TEMP/prefix" WINEDEBUG=-all "$TASK_SERVER" -k >/dev/null 2>&1 || true
 timeout 10 env WINEPREFIX="$TASK_TEMP/prefix" WINEDEBUG=-all "$TASK_SERVER" -w >/dev/null 2>&1 || true
 rm -rf "$TASK_TEMP"
}
trap cleanup EXIT
sha256sum "$TASK_LOCAL/composition_host.cpp" "$TASK_LOCAL/composition_entry.S" "$TASK_LOCAL/test-composition.sh" "$TASK_LOCAL/diagnostic_bootstrap.cpp" "$TASK_SOURCE/verify_bootstrap_image.h" "$TASK_SOURCE/startup_gate.h" "$TASK_SOURCE/sha256_digest.h" > "$TASK_OUT/composition-input-hashes.txt"
TASK_PRODUCTION_HASH="$(sha256sum "$TASK_OUT/winmm-diagnostic-NOT_INSTALLABLE.dll" | awk '{print $1}')"
export LD_LIBRARY_PATH="$TASK_ROOT/usr/lib/x86_64-linux-gnu:$TASK_ROOT/usr/lib/llvm-21/lib"
FLAGS=(--target=x86_64-w64-windows-gnu --sysroot="$TASK_ROOT/usr/x86_64-w64-mingw32" --ld-path="$TASK_ROOT/usr/bin/x86_64-w64-mingw32-ld" -L"$TASK_ROOT/usr/lib/gcc/x86_64-w64-mingw32/13-posix" -L"$TASK_ROOT/usr/x86_64-w64-mingw32/lib")
INCLUDES=(-isystem "$TASK_ROOT/usr/lib/gcc/x86_64-w64-mingw32/13-posix/include/c++" -isystem "$TASK_ROOT/usr/lib/gcc/x86_64-w64-mingw32/13-posix/include/c++/x86_64-w64-mingw32" -isystem "$TASK_ROOT/usr/lib/gcc/x86_64-w64-mingw32/13-posix/include/c++/backward" -isystem "$TASK_ROOT/usr/x86_64-w64-mingw32/include")
CXX="$TASK_ROOT/usr/lib/llvm-21/bin/clang++"
mkdir -p "$TASK_TEMP/probes/p0-shroudforge-diagnostic"
ln -s "$TASK_LOCAL/../p0-scheduler-fixture" "$TASK_TEMP/probes/p0-scheduler-fixture"
timeout 60 "$CXX" "${FLAGS[@]}" "${INCLUDES[@]}" -O2 -static "$TASK_LOCAL/composition_host.cpp" "$TASK_LOCAL/composition_entry.S" "$TASK_OUT/libdiagnostic.a" -o "$TASK_TEMP/enshrouded.exe"
"$TASK_ROOT/usr/bin/x86_64-w64-mingw32-nm" --defined-only "$TASK_TEMP/enshrouded.exe" > "$TASK_TEMP/symbols.txt"
python3 - "$TASK_TEMP" "$TASK_SOURCE" "$TASK_LOCAL" "$TASK_OUT" <<'PY'
import sys,pathlib,struct,re,hashlib,json,difflib
root,up,local,out=map(pathlib.Path,sys.argv[1:]); host=(root/'enshrouded.exe').read_bytes()
pe=struct.unpack_from('<I',host,0x3c)[0]; stamp=struct.unpack_from('<I',host,pe+8)[0]; opt=pe+24
base=struct.unpack_from('<Q',host,opt+24)[0]; size=struct.unpack_from('<I',host,opt+56)[0]
address=int(next(x.split()[0] for x in (root/'symbols.txt').read_text().splitlines() if x.split()[-1]=='p0_composition_entry'),16); rva=address-base
count=struct.unpack_from('<H',host,pe+6)[0]; sec=opt+struct.unpack_from('<H',host,pe+20)[0]
for i in range(count):
 pos=sec+i*40; va,rawsize,raw=struct.unpack_from('<III',host,pos+12)
 if va<=rva and rva+11<=va+rawsize: entry=host[raw+rva-va:raw+rva-va+11]; break
else: raise RuntimeError('site not file-backed')
def array(b): return ','.join(f'0x{x:02x}' for x in b)
verify=(up/'verify_bootstrap_image.h').read_text(); fixture=re.sub(r'approved_timestamp=\d+',f'approved_timestamp={stamp}',verify); fixture=re.sub(r'approved_image_size=\d+',f'approved_image_size={size}',fixture); fixture=re.sub(r'approved_file_size=\d+',f'approved_file_size={len(host)}',fixture); fixture=re.sub(r'approved_digest\{.*?\};','approved_digest{'+array(hashlib.sha256(host).digest())+'};',fixture,flags=re.S)
cpp=(local/'diagnostic_bootstrap.cpp').read_text(); testcpp=re.sub(r'target_rva=0x[0-9a-f]+',f'target_rva=0x{rva:x}',cpp); testcpp=re.sub(r'entry_digest\{.*?\};','entry_digest{'+array(hashlib.sha256(entry).digest())+'};',testcpp,flags=re.S)
(root/'verify_bootstrap_image.h').write_text(fixture); (root/'probes/p0-shroudforge-diagnostic/diagnostic_bootstrap.cpp').write_text(testcpp)
manifest={'fixtureOnly':True,'runtimeOverrides':False,'hostSha256':hashlib.sha256(host).hexdigest(),'hostSiteRva':rva,'hostEntrySha256':hashlib.sha256(entry).hexdigest(),'originalHeaderSha256':hashlib.sha256(verify.encode()).hexdigest(),'fixtureHeaderSha256':hashlib.sha256(fixture.encode()).hexdigest(),'originalSourceSha256':hashlib.sha256(cpp.encode()).hexdigest(),'fixtureSourceSha256':hashlib.sha256(testcpp.encode()).hexdigest(),'headerDiff':list(difflib.unified_diff(verify.splitlines(),fixture.splitlines())),'sourceDiff':list(difflib.unified_diff(cpp.splitlines(),testcpp.splitlines()))}
(out/'composition-transform.json').write_text(json.dumps(manifest,indent=2)+'\n')
PY
timeout 60 "$CXX" "${FLAGS[@]}" "${INCLUDES[@]}" -I"$TASK_TEMP" -I"$TASK_SOURCE" -DNOMINMAX -DWIN32_LEAN_AND_MEAN -Wall -Wextra -Werror -Wno-missing-field-initializers -O2 -std=gnu++20 -shared -static -pthread -Wl,--exclude-all-symbols "$TASK_TEMP/probes/p0-shroudforge-diagnostic/diagnostic_bootstrap.cpp" "$TASK_OUT/adapter.o" "$TASK_OUT/winmm.def" -o "$TASK_TEMP/winmm.dll"
sha256sum "$TASK_TEMP/enshrouded.exe" "$TASK_TEMP/winmm.dll" > "$TASK_OUT/composition-artifacts.sha256"
(cd "$TASK_TEMP" && timeout 60 env WINEPREFIX="$TASK_TEMP/prefix" WINEDEBUG=-all WINEDLLOVERRIDES=winmm=n,b "$TASK_WINE" enshrouded.exe) > "$TASK_OUT/composition-fixture.txt" 2>&1
cat "$TASK_OUT/composition-fixture.txt"
(cd "$TASK_TEMP" && timeout 135 env WINEPREFIX="$TASK_TEMP/prefix" WINEDEBUG=-all WINEDLLOVERRIDES=winmm=n,b "$TASK_WINE" enshrouded.exe --wait-expiry) > "$TASK_OUT/composition-expiry.txt" 2>&1
cat "$TASK_OUT/composition-expiry.txt"
python3 - "$TASK_TEMP" "$TASK_OUT" <<'PYSUMMARY'
import pathlib,sys,re,json
root,out=map(pathlib.Path,sys.argv[1:])
for tag,log in [('manual','composition-fixture.txt'),('expiry','composition-expiry.txt')]:
 text=(out/log).read_text(); name=re.search(r'^SUMMARY_FILE: (shroudforge-p0-\d+-[\d-]+\.json)$',text,re.M).group(1)
 data=json.loads((root/name).read_text()); assert data['disabled'] and data['quiescenceProven'] is False
 assert data['afterDisable']['reserved']==10 and data['afterDisable']['published']==10
 if tag=='expiry': assert data['afterDisable']['elapsedMs']>=120000 and len(data['samples'])<=120
 (out/f'composition-{tag}-summary.json').write_text(json.dumps(data,indent=2)+'\n')
PYSUMMARY
cp "$TASK_OUT/winmm-diagnostic-NOT_INSTALLABLE.dll" "$TASK_TEMP/winmm.dll"
set +e
(cd "$TASK_TEMP" && timeout 60 env WINEPREFIX="$TASK_TEMP/prefix" WINEDEBUG=-all WINEDLLOVERRIDES=winmm=n,b "$TASK_WINE" enshrouded.exe) > "$TASK_OUT/composition-production-refusal.txt" 2>&1
result=$?
set -e
test "$result" -eq 66
test "$TASK_PRODUCTION_HASH" = "$(sha256sum "$TASK_OUT/winmm-diagnostic-NOT_INSTALLABLE.dll" | awk '{print $1}')"
printf 'PASS: unchanged production diagnostic DLL refuses the same synthetic composition host exit=%s\n' "$result"
