# P0 strict guard patch (not a deployable loader)

Apply `0001-p0-strict-runtime-guards.patch` only to ShroudForge
`3ed90de1df266c4f0c217523a02a71dfa67243de`. Upstream source is MIT-licensed;
its notice is preserved in `UPSTREAM_LICENSE`. No upstream branch was pushed.
No game binaries, SDK binaries, compiled loader, or game data are included.

## Behavior

- Before installing the bootstrap entrypoint jump, require the exact client
  SHA-256, file length, PE timestamp and image size from the reviewed 26082781
  identity. The check uses allocation-free SHA-256 and Win32 file reads, with
  no crypto-provider DLL initialization under the loader lock. Keep a read
  handle that denies write/delete sharing until detach. No environment/config
  override or automatic admission of a newly installed executable.
- Require a unique exact SHA/PE execution profile. A manual profile ID cannot
  authorize structural fallback; existing signature/instruction checks stay.
- Propagate required engine-hook and resource initialization failure, clear
  initialized state, and shut down partial initialization. Required hooks are
  the upstream game-thread/entity-manager hooks; optional feature hooks remain
  unavailable on failure, never presumed passed.
- Stop bootstrap on failed native initialization before UI/asset preparation
  and before releasing the game entrypoint. Preserve asset checks for later
  successful initialization; do not bypass integrity or update protections.
- Roll back the entrypoint jump and close owned handles on attach failure.
  If original instructions cannot be restored, fail-stop rather than unload a
  DLL that the entrypoint could still jump into.

This is intentionally a client-only P0 variant, not a general ShroudForge
release or server compatibility claim. Startup/update services and artifact
integrity still require a reviewed deployment configuration before any install.

## Critical remaining blocker

The matching `client/26082781.json` has `provenance.status=read-validated`,
`ecsReadLayout` and resource layouts, **not `hooks` or `ecsLayout`**. It is put
in the resource-read profile table, not the execution-profile table. The only
client execution profile in this pin is older `1076226.json` with hash
`af2f5a1227911d8aa06b3908d6bd0211838211cae14ea91099cb57d0df990781`.

Therefore this strict variant **correctly refuses the current client** even
though its bootstrap file identity matches. Do not copy older offsets, change
provenance, or relax guards to get it to launch. Current-build execution
symbols/layouts need independent validation. A separately defined read-only
mode cannot be reported as this required-hook gate passing.

## Reproduce source and focused tests

Use a separate source checkout, not the game directory:

```sh
git clone https://github.com/bonsaibauer/shroudforge.git shroudforge-p0
cd shroudforge-p0
git checkout --detach 3ed90de1df266c4f0c217523a02a71dfa67243de
git apply --check /path/to/EnshroudedCompanion/patches/shroudforge/0001-p0-strict-runtime-guards.patch
git apply /path/to/EnshroudedCompanion/patches/shroudforge/0001-p0-strict-runtime-guards.patch
timeout 30s python3 dev/tests/p0-bootstrap-identity/check.py
timeout 30s python3 dev/tests/p0-strict-guards/check.py
timeout 30s python3 dev/tests/p0-bootstrap-attach/check.py
```

The first test compiles the actual SHA header. The other two compile extracted
production predicates/initialization/DllMain bodies against explicit fixtures;
they do not emulate or validate actual game symbols. Native Windows fixtures
use the actual identity/entrypoint guard headers and Win32 calls:

```sh
timeout 60s python3 dev/tests/p0-bootstrap-identity/native_checks.py \
  --cxx /path/to/x86_64-w64-mingw32-g++-posix \
  --wine '/path/to/Proton - Experimental/files/bin/wine' \
  --wineserver '/path/to/Proton - Experimental/files/bin/wineserver'
```

The runner creates its own temporary prefix, compiles synthetic executables,
runs bounded tests, and stops only that prefix's Wine server. It never points
at Steam's game prefix. Each subprocess has a timeout. Do not pass a game
executable as a test fixture. Tests run here used host g++ 15.2.0, isolated
MinGW g++ 13-posix and Proton Experimental `experimental-11.0-20261014-x86_64`.
Exact downloaded build package hashes are in the evidence directory.

## Validation and limits

- PASS: 15 SHA vectors, including padding boundaries and million-byte input.
- PASS: missing/wrong SHA, changed PE fields, missing required hooks,
  resource failure/exception, initialized-state cleanup fixtures.
- PASS: actual DllMain body fixtures for unknown image, event/gate/thread
  failure, cleanup ordering, success and detach.
- PASS: real Win32 unknown executable rejection, same-file-size hash mismatch,
  entrypoint release, preparation failure and entrypoint cancellation under
  isolated Wine. These executables contain no game code.
- PASS: changed bootstrap/native initialization translation units pass MinGW
  syntax checks with normal build definitions. Not a linked production DLL.
- PASS: patch applies cleanly to the exact source pin; all 16 changed files
  match the checkout used for tests.
- NOT_RUN: full production DLL bundle, actual game symbol scan, fault-injected
  Windows protection-denial branch, game launch, lifecycle mod, world reset.

The original upstream Windows/MSVC build remains the reference build route.
A fixture pass or cross-compiler syntax pass is not a supported release. Keep
RUNTIME HOLD and all live assertions NOT_RUN until exact symbols and the
complete artifact are validated. Do not install a mixture of patched source
and an unmodified prebuilt runtime.

## Subsequent build result

Apply `0002-p0-cross-build-portability.patch` after `0001` to the same source pin. The provider, bootstrap, Rust runtime, launcher and updater now cross-build. [Build reproduction and actual DLL tests](../../evidence/p0/build-investigation/README.md) supersede the earlier full-build NOT_RUN note above. This is still not an approved deployable runtime: the current execution-hook context/profile remains unvalidated. No guard was relaxed to obtain these artifacts.
