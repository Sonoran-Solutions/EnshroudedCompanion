# P0 reproduction and gated continuation

The committed tool is an **offline identity collector**, not a native runtime probe. It reads two operator-selected files and prints only allowlisted identifiers. It never loads a DLL, opens a game process, changes a save, approves a build, or verifies symbols. Python 3.14.4 on Linux was used; it has no third-party dependencies. Other Python/platform combinations were not tested.

From the repository root, with the game and updater stopped, substitute the actual authorized paths:

```sh
timeout 30s python3 tools/p0_fingerprint.py \
  --manifest '/path/to/steamapps/appmanifest_1203620.acf' \
  --executable '/path/to/steamapps/common/Enshrouded/enshrouded.exe'
timeout 30s python3 -m unittest discover -s tests -v
```

For a dedicated server use its own manifest and executable. Accepted Steam application IDs are client `1203620` and server `2278520`. The collector refuses malformed/ambiguous numeric manifest fields, unrelated application IDs, and non-AMD64 PE input. These checks are input hygiene, **not unsupported-build protection for a running mod**. A valid PE may still be unsupported. The operator supplies manifest/binary association; a manifest does not cryptographically attest to binary contents. Do not collect during an update or use the result as a launch authorization.

Output contains file length/SHA-256 and the manifest's app/build IDs. It deliberately omits full paths, manifest owner IDs, raw game logs and saves. Review any additional evidence before publication. A matching upstream profile is source evidence only; no approved-build list is established by this repository.

## Next live job, not launched

Purpose: establish load/disable/restart and fail-closed behavior of the single candidate selected in [source review](SOURCE_REVIEW.md), before observations or mutation. Expected duration and timeout must be recorded for the concrete job after owner preflight answers; suggested first session 10–15 minutes with a hard end. One integrator owns the game process and any later world mutations. Do not start a competing framework or server.

Prerequisites:

1. Owner has confirmed NPC embodiment, this-machine testing and investigation cap. One client is available: two-client scenarios remain NOT_RUN. Establish the exact local launch topology.
2. Use the owner-authorized default-character shared world in OFFLINE mode. Verify its reported reset on exit/reopen and character-state isolation; record template identity and baseline. This supersedes the original new-world prerequisite. Keep any private save evidence outside Git.
3. Capture current client and server hashes independently, toolchain and compatibility-layer version, source SHA, exact launch/configuration and existing processes.
4. Resolve source-review safety blockers. Unknown builds and required missing symbols must refuse activation before unsafe hooks. No structural-revalidation override or guessed address to get past a gate.
5. Review the exact runtime configuration and permitted native actions. The present repo contains no loadable module or native action implementation.

Run only the smallest applicable rows in [scenario matrix](SCENARIOS.md). Capture timestamped expected/actual results, command and process ownership, exit status and cleanup. Record absent APIs as unresolved; do not substitute raw writes, teleportation, visual labels, or save edits. Subsequent read-only, actor and construction jobs remain gated as specified in the tickets. Stop after P0; P1 requires a separate owner disposition.

Follow-up: Steam update in progress and source-confirmed startup safety blockers prevent launch. See [runtime preflight](RUNTIME_PREFLIGHT.md) and the [disabled lifecycle package](../../probes/shroudforge/README.md). Refresh identities only after the update completes.

Current checkpoint: update completed and identity is unchanged. Follow the [guard-patch reproduction](../../patches/shroudforge/README.md) for synthetic tests only. Current-build execution symbols are missing from the pinned source, so no game launch/reset is requested.

Latest continuation: [native build](../../evidence/p0/build-investigation/README.md), [Rust build](../../evidence/p0/build-investigation/RUST_BUILD.md), and [static profile scanner](../../evidence/p0/profile-investigation/README.md) contain bounded reproduction commands. Selected-path binaries build; game testing remains held on scheduler/query ABI evidence.
