#!/usr/bin/env bash
# Execute from the pinned ShroudForge checkout after the frontend build.
# P0_TOOLCHAIN holds the isolated compiler/package tree described in README.md.
set -euo pipefail
: "${P0_TOOLCHAIN:?Set the isolated toolchain directory}"
export PATH="$P0_TOOLCHAIN/rust-tools:$P0_TOOLCHAIN/rust/bin:$P0_TOOLCHAIN/root/usr/bin:/usr/bin:/bin"
export CARGO_HOME="$P0_TOOLCHAIN/cargo-home"
export CARGO_TARGET_DIR="$P0_TOOLCHAIN/rust-target"
export CARGO_TARGET_X86_64_PC_WINDOWS_GNU_LINKER="$P0_TOOLCHAIN/root/usr/bin/x86_64-w64-mingw32-gcc-posix"
export CC_x86_64_pc_windows_gnu="$P0_TOOLCHAIN/root/usr/bin/x86_64-w64-mingw32-gcc-posix"
export CXX_x86_64_pc_windows_gnu="$P0_TOOLCHAIN/root/usr/bin/x86_64-w64-mingw32-g++-posix"
timeout 300s cargo build --locked --release -p shroudforge-modloader -p shroudforge-updater --target x86_64-pc-windows-gnu -j 2
