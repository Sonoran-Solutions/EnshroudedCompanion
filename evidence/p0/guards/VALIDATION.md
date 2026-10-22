# Guard work evidence — 2026-10-22

Source: ShroudForge `3ed90de1df266c4f0c217523a02a71dfa67243de` plus the committed
strict guard patch. Test source is included in that patch. Source clones and
build packages remain outside the project; no proprietary data is included.

Patch SHA-256: `50e4916940ff2e1e83bef3d5f797f43f6f4d243c9ce555ee7170f941f9a52296`.
Clean-checkout apply and byte comparison of all 16 changed files: PASS.

Logs: [SHA](hash-tests.txt), [provider fixtures](provider-fixtures.txt),
[attach fixtures](attach-fixtures.txt), [Windows fixtures](windows-fixtures.txt).
These are different evidence classes: extracted-body fixtures, real synthetic
Win32 tests, and no game execution. No native symbol-resolution success is
claimed.

Host g++: Ubuntu 15.2.0-16ubuntu1. Windows fixtures: isolated MinGW GCC 13-posix,
x86_64-w64-mingw32. Packages downloaded with `apt-get download` and extracted
with `dpkg-deb -x` under `/tmp/enshrouded-p0-toolchain/root`; no system install
or root operation. See [package hashes](toolchain-packages.sha256).

Proton version file: `1791989105 experimental-11.0-20261014-x86_64`. Synthetic
Win32 programs ran in newly created `/tmp` prefixes, never Steam's game prefix.
Only owned per-prefix Wine servers were stopped/waited for. Native fixture
success does not establish game compatibility under Proton.

First bootstrap syntax attempt lacked nlohmann headers; isolated package added.
First native initialization syntax attempt omitted upstream `KFC_RUNTIME_BUILD`
and failed on dllimport definitions; rerun with the existing upstream define
passed. No source protection was removed to fix either build invocation.

Syntax check flags: `-std=c++20 -DWIN32_LEAN_AND_MEAN -DNOMINMAX`, add
`-DKFC_RUNTIME_BUILD` for provider files, nlohmann/native include directories.
Bootstrap `startup.cpp`, `runtime_lifecycle.cpp`; provider
`ecs/bind_world_entities.cpp`, `host/manage_runtime_lifecycle.cpp` passed.
Profile selector is checked separately with generated embedded-profile IDs;
this is a syntax input, not a resource-bundled production DLL.

Steam completion observation: StateFlags 4, TargetBuildID 0,
BytesDownloaded=BytesToDownload=22349108560,
BytesStaged=BytesToStage=23350485053. Fresh offline collector and independent
`sha256sum` match the previous executable hash. See
[post-update identity](../client-fingerprint-after-update.json). This does not
admit new builds automatically.

[Profile inventory](profile-inventory.json) establishes why a matched read
profile is not an exact execution profile. The patched runtime should refuse
this installed client, not report success.

No loader/game process was launched, no native artifact installed, no world or
character/save changed, and no system package installed. Game-menu reset is
not requested yet because no runtime candidate passes the strict gate.

## Full native build attempt (not passed)

Isolated Clang 21, CMake and Ninja were extracted for a bounded additional
cross-build. Clang accepts the source's Windows SEH syntax; CMake configured
and Zydis compiled. Full `kfc-runtime` linking was NOT reached. The first run
failed on a MSVC-specific `std::ifstream(std::wstring)` use and resource compiler
headers. A five-minute targeted retry temporarily used the standard filesystem
path constructor and explicit RC headers/preprocessor. It progressed to a
GNU windres resource-parser failure: adjacent quoted license resource entries
were parsed as a concatenated `LICENSEZYCORE_LICENSE` filename. No DLL resulted.
The temporary unrelated transport portability change was reverted; the guard
patch is byte-identical to the one tested above. No SEH, profile, signature or
instruction guard was stubbed out to force compilation.

The retained isolated toolchain definition is local at
`/tmp/enshrouded-p0-toolchain/mingw-clang-toolchain.cmake`; build at
`/tmp/enshrouded-p0-toolchain/full-native-build`. Failed build command:
`timeout 300s env LD_LIBRARY_PATH=/tmp/enshrouded-p0-toolchain/root/usr/lib/x86_64-linux-gnu /tmp/enshrouded-p0-toolchain/root/usr/bin/cmake --build /tmp/enshrouded-p0-toolchain/full-native-build --target kfc-runtime --parallel 2`.
No owned build or game processes remain. A complete supported Windows artifact
is still required; the exact-execution-profile absence independently prevents
live testing even if the cross-build issue is solved.

Final rollback correction retains the original page protection even if installation failed while restoring protection. The native cancellation fixture now checks `VirtualQuery` protection as well as original bytes/events. All five Windows cases reran successfully.
