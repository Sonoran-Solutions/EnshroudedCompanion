# Isolated Rust and frontend build

Purpose: build the pinned loader runtime and updater required by the native
bootstrap, without installing anything in Steam or running the game. Initial
compile budget: five minutes, two compiler jobs. Each toolchain download and
extraction/install is limited to 60 seconds. Frontend install and build each
have a 120-second timeout. A successful compile alone is not deployment approval.

## Inputs and reproduction

Source is ShroudForge `3ed90de1df266c4f0c217523a02a71dfa67243de`, with this
project's patches applied in numeric order. Initialize only the pinned
`kfc-parser` submodule with `timeout 60s git submodule update --init kfc-parser`.
The resulting parser revision `2cf45e36cd4575fa3867761864a36e1883c82add` is already
integrated according to upstream `dev/parser/prepare_parser.ps1`.

The exact Rust package URLs and SHA-256 digests are in [rust-packages.json](rust-packages.json).
Download each archive, verify its digest, extract it, and run its `install.sh`
with `--prefix="$P0_TOOLCHAIN/rust" --disable-ldconfig`. This session used
`P0_TOOLCHAIN=/tmp/enshrouded-p0-toolchain`. Nothing was installed globally.
Native compiler inputs are documented in the sibling native build evidence.

Node 24.19.0 came from the desktop bundled runtime. npm 12.2.1 was downloaded
from its registry tarball and checked against the published SHA-512 integrity
before extraction under the isolated toolchain. Exact metadata and source lock
hashes are in [rust-ui-inputs.json](rust-ui-inputs.json). Put a symlink named
`npm` to the extracted `npm/bin/npm-cli.js` on a private PATH alongside Node.
Running the tarball's Windows-oriented `bin/npm` shell wrapper directly failed
to locate npm relative to the separately bundled Node, so this symlink is
required. It does not change project source.

From `modules/modloader/ui`, run `timeout 120s npm ci` then
`timeout 120s env -u CROWDIN_PERSONAL_TOKEN npm run build`. Both generated frontend
pages passed. The normal upstream build emits unavailable translation-progress
metadata when no Crowdin token is supplied. No private token was used. Preserve
the generated `build/ui/modloader/index.html` and `build/ui/updater/updater.html`
for the Rust `include_str!` inputs. Do not replace them with empty stubs.

From the source root, run [build_rust.sh](build_rust.sh), setting
`P0_TOOLCHAIN` to that isolated directory. This invokes `cargo build --locked`
for the loader and updater with the Windows GNU target and two jobs. Keep
`Cargo.lock` unchanged. No safety or runtime feature was disabled for compilation.

## Build-tool correction

The first locked Rust build reached project crates but failed because `winres`
invokes unprefixed `windres` (not the installed cross-prefixed command). A first
retry accidentally omitted the corrected PATH and repeated that failure. The
corrected retry adds a task-owned `rust-tools` directory before the toolchain:
`ar` links to `x86_64-w64-mingw32-ar`; `windres` is a wrapper invoking the cross
resource compiler with the cross GCC preprocessor explicitly specified:

```sh
exec "$P0_TOOLCHAIN/root/usr/bin/x86_64-w64-mingw32-windres" \
  --preprocessor="$P0_TOOLCHAIN/root/usr/bin/x86_64-w64-mingw32-gcc-posix" \
  --preprocessor-arg=-E --preprocessor-arg=-xc --preprocessor-arg=-DRC_INVOKED "$@"
```

The wrapper uses the literal resolved toolchain path in this session. No registry
crate or project build script was edited. The corrected retry is capped at five
minutes and uses the existing compilation cache, still with two jobs.

## Result

PASS: corrected locked release build completed in 1 minute 8 seconds. The runtime
DLL, launcher and updater all linked. [Artifact hashes/imports](rust-artifacts.json)
record each file. All reported imports are Windows system libraries. The runtime
exports `shroudforge_prepare_startup`, `shroudforge_create`, `shroudforge_update`
and `shroudforge_destroy`. Binaries remain local and are not committed.

A synthetic harness loads the actual Rust DLL, resolves all four exports, checks
null-input refusal, and unloads it in a new isolated Proton Wine prefix. It never
supplies a game root or invokes actual asset/runtime preparation. See
[test_rust_dll.py](test_rust_dll.py), [harness](rust_dll_probe.cpp) and
[result](rust-dll-tests.txt). An initial harness omitted an explicit wmain return
and exited 29 after printing its assertions; that run was not accepted. The
corrected harness explicitly returns zero and was rebuilt before the final run.

Run the Python runner with `--cxx`, `--wine`, `--wineserver` pointing to the
recorded toolchain/Proton binaries and `--dll` pointing to the actual built Rust
DLL. Compile timeout30s, DLL-load test45s, prefix-server shutdown/wait10s each.
Only the private prefix is stopped. This is an ABI/load check, not an in-game
lifecycle or world test. Launcher/updater execution is NOT_RUN.

Files are staged under `/tmp/enshrouded-p0-toolchain/staged-bundle/shroudforge/`
for inspection only. No Steam path, installed loader, game prefix, save or world
was changed. Full deployment readiness still depends on the bootstrap forwarding
checks and current-build execution-hook evidence.
