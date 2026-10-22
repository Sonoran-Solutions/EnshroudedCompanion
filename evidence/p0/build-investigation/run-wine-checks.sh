#!/usr/bin/env bash
set -euo pipefail
ROOT=/tmp/enshrouded-p0-toolchain/root
CXX="$ROOT/usr/lib/llvm-21/bin/clang++"
WINE='/home/dq/.local/share/Steam/steamapps/common/Proton - Experimental/files/bin/wine'
WINESERVER='/home/dq/.local/share/Steam/steamapps/common/Proton - Experimental/files/bin/wineserver'
PROXY=/tmp/enshrouded-p0-toolchain/bootstrap-objects/winmm-quoted.dll
PROXY_IMPLIB=/tmp/enshrouded-p0-toolchain/bootstrap-objects/libwinmm-quoted.a
PROVIDER=/tmp/enshrouded-p0-toolchain/staged-bundle/shroudforge
HARNESS="$(dirname "$0")/harnesses"
TMP="$(mktemp -d /tmp/p0-native-checks.XXXXXX)"
cleanup() {
  for prefix in "$TMP/bootstrap-prefix" "$TMP/provider-prefix"; do
    timeout 10s env WINEPREFIX="$prefix" WINEDEBUG=-all "$WINESERVER" -k >/dev/null 2>&1 || true
    timeout 10s env WINEPREFIX="$prefix" WINEDEBUG=-all "$WINESERVER" -w >/dev/null 2>&1 || true
  done
  rm -rf "$TMP"
}
trap cleanup EXIT
export LD_LIBRARY_PATH="$ROOT/usr/lib/x86_64-linux-gnu:$ROOT/usr/lib/llvm-21/lib"
FLAGS="--target=x86_64-w64-windows-gnu --sysroot=$ROOT/usr/x86_64-w64-mingw32 --ld-path=$ROOT/usr/bin/x86_64-w64-mingw32-ld -L$ROOT/usr/lib/gcc/x86_64-w64-mingw32/13-posix -L$ROOT/usr/x86_64-w64-mingw32/lib"
mkdir -p "$TMP/bootstrap" "$TMP/bootstrap-prefix" "$TMP/provider/shroudforge" "$TMP/provider-prefix"
cp "$PROXY" "$TMP/bootstrap/winmm.dll"
timeout 30s "$CXX" $FLAGS -std=c++20 -static \
  -I"$ROOT/usr/x86_64-w64-mingw32/include" \
  "$HARNESS/forward_probe.cpp" "$PROXY_IMPLIB" -o "$TMP/bootstrap/forward_probe.exe"
timeout 30s "$CXX" $FLAGS -std=c++20 -static \
  -I"$ROOT/usr/x86_64-w64-mingw32/include" \
  "$HARNESS/guard_probe.cpp" "$PROXY_IMPLIB" -o "$TMP/bootstrap/safe_host.exe"
cp "$TMP/bootstrap/safe_host.exe" "$TMP/bootstrap/enshrouded.exe"
set +e
(cd "$TMP/bootstrap" && timeout 30s env WINEPREFIX="$TMP/bootstrap-prefix" WINEDEBUG=+loaddll \
  WINEDLLOVERRIDES=winmm=n,b "$WINE" forward_probe.exe) >"$TMP/forward.log" 2>&1
forward=$?
(cd "$TMP/bootstrap" && timeout 30s env WINEPREFIX="$TMP/bootstrap-prefix" WINEDEBUG=+loaddll \
  WINEDLLOVERRIDES=winmm=n,b "$WINE" safe_host.exe) >"$TMP/safe.log" 2>&1
safe=$?
(cd "$TMP/bootstrap" && timeout 30s env WINEPREFIX="$TMP/bootstrap-prefix" WINEDEBUG=+loaddll \
  WINEDLLOVERRIDES=winmm=n,b "$WINE" enshrouded.exe) >"$TMP/strict.log" 2>&1
strict=$?
set -e
test "$forward" -eq 0
test "$safe" -eq 0
test "$strict" -eq 66
rg -F 'c:\\windows\\system32\\winmm.dll' "$TMP/forward.log" >/dev/null
rg -F 'status c0000142' "$TMP/strict.log" >/dev/null
printf 'PASS: exact system32 winmm forwarder host exit=%s\n' "$forward"
printf 'PASS: unrelated host DllMain exit=%s\n' "$safe"
printf 'PASS: enshrouded.exe strict identity refusal exit=%s\n' "$strict"
rg -i 'Loaded.*(forward_probe|winmm.dll)' "$TMP/forward.log" | tail -5
rg -i 'failed to initialize|status c0000142' "$TMP/strict.log" | tail -5
cp "$PROVIDER"/*.dll "$TMP/provider/shroudforge/"
timeout 30s "$CXX" $FLAGS -std=c++20 -static \
  -I"$ROOT/usr/x86_64-w64-mingw32/include" \
  "$HARNESS/provider_probe.cpp" -o "$TMP/provider/provider_probe.exe"
set +e
(cd "$TMP/provider" && timeout 60s env WINEPREFIX="$TMP/provider-prefix" WINEDEBUG=-all \
  "$WINE" provider_probe.exe) >"$TMP/provider.log" 2>&1
provider=$?
set -e
test "$provider" -eq 0
rg -F 'missing-exact-sha256-profile' "$TMP/provider.log"
rg -F 'actor-placement=0/0' "$TMP/provider.log"
rg -F 'building-dispatch=0/0' "$TMP/provider.log"
echo 'PASS: ABI-12 provider loaded, refused unknown image, actor/building hooks inactive'
