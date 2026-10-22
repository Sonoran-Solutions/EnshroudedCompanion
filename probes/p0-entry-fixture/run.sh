#!/usr/bin/env bash
set -euo pipefail
TASK_ROOT=/tmp/enshrouded-p0-toolchain/root
TASK_WINE='/home/dq/.local/share/Steam/steamapps/common/Proton - Experimental/files/bin/wine'
TASK_SERVER='/home/dq/.local/share/Steam/steamapps/common/Proton - Experimental/files/bin/wineserver'
TASK_DIR="$(mktemp -d /tmp/p0-entry-fixture.XXXXXX)"
TASK_SOURCE="$(cd "$(dirname "$0")" && pwd)"
cleanup() {
 timeout 10 env WINEPREFIX="$TASK_DIR/prefix" WINEDEBUG=-all "$TASK_SERVER" -k >/dev/null 2>&1 || true
 timeout 10 env WINEPREFIX="$TASK_DIR/prefix" WINEDEBUG=-all "$TASK_SERVER" -w >/dev/null 2>&1 || true
 rm -rf "$TASK_DIR"
}
trap cleanup EXIT
export LD_LIBRARY_PATH="$TASK_ROOT/usr/lib/x86_64-linux-gnu:$TASK_ROOT/usr/lib/llvm-21/lib"
timeout 10 "$TASK_ROOT/usr/lib/llvm-21/bin/clang" --version
timeout 10 "$TASK_ROOT/usr/bin/x86_64-w64-mingw32-ld" --version
timeout 10 env WINEPREFIX="$TASK_DIR/prefix" WINEDEBUG=-all "$TASK_WINE" --version
timeout 60 "$TASK_ROOT/usr/lib/llvm-21/bin/clang" --target=x86_64-w64-windows-gnu \
 --sysroot="$TASK_ROOT/usr/x86_64-w64-mingw32" \
 --ld-path="$TASK_ROOT/usr/bin/x86_64-w64-mingw32-ld" \
 -L"$TASK_ROOT/usr/lib/gcc/x86_64-w64-mingw32/13-posix" \
 -L"$TASK_ROOT/usr/x86_64-w64-mingw32/lib" \
 -Wall -Wextra -Werror -std=c11 -O2 -static \
 "$TASK_SOURCE/fixture.c" "$TASK_SOURCE/probe.S" -o "$TASK_DIR/fixture.exe"
printf "Synthetic fixture EXE SHA256: "
sha256sum "$TASK_DIR/fixture.exe" | awk '{print $1}'
timeout 60 env WINEPREFIX="$TASK_DIR/prefix" WINEDEBUG=-all "$TASK_WINE" "$TASK_DIR/fixture.exe"
