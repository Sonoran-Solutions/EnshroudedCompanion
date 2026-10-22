# P0 inert lifecycle probe — NOT_RUN, disabled

This is a source-reviewed three-file package for ShroudForge commit
`3ed90de1df266c4f0c217523a02a71dfa67243de`. It logs load and unload callbacks
only. It does not query entities, write assets, register update handlers or UI
actions, call native actions, or infer game compatibility. It is disabled by
default and has **not been copied into the game installation**.

**Do not install/enable it yet.** The [runtime preflight](../../docs/p0/RUNTIME_PREFLIGHT.md)
records startup/version/symbol guard blockers in the underlying loader. An inert
Lua mod does not make its native loader inert. `runtime.require` checks feature
availability, not the native loader's pre-hook fingerprint or symbol behavior.
Steam's in-progress update also invalidates the earlier fingerprint for launch.

## Source mapping

At the pinned upstream SHA:

- `docs/examples/starter-mod/mod.json` and `src/mod.lua`: package capability and lifecycle requirement.
- `modules/modloader/manifest/extended.mod.schema.json`: disabled client-only extension.
- `modules/mod-runtime/lua/require_mod.rs`: canonical `src/mod.lua` entrypoint.
- `modules/mod-runtime/lua/ingame/check_ingame_access.rs`: lifecycle feature prerequisite.
- `modules/mod-runtime/ingame/load_runtime_callbacks.rs`: `on_load` callback.
- `modules/mod-runtime/ingame/unload_runtime_mod.rs`: `on_unload` and cleanup.

## Future bounded scenario

After fresh supported-build identification, required native guard tests, and an
observed disposable-session reset, copy **only** `p0-lifecycle/` to the selected
runtime's mods directory. Do not install bundled gameplay mods. Use the default
character and an owner-authorized shared-player world in OFFLINE mode. Record
its template identity and initial baseline, exit and reopen to check reset.
Native computer-menu automation is not available in this chat's tools; a human
operator is needed for these menu interactions and observations.

Proposed load/disable job: five-minute target, ten-minute hard timeout, one
client/process and one operator. Enable this package through the runtime's
supported configuration path and collect exactly one `loaded` marker; disable
it and collect `unloaded`. Repeat once, restart disabled and verify no additional
load marker for the new process. Bind logs to process/run, code, runtime and
fresh game identities; identical strings alone do not establish event ordering.

Stop on crash, missing unload, unexpected game-state change or any identity/
symbol failure. Remove only this package after the run and use the recorded
loader uninstall procedure. Probe disable does not claim to unload native DLLs.
This tests lifecycle callbacks only; no OBSERVATION, QUEST, ACTOR or BUILDING
pass follows from it. Restore/reset semantics must be verified before mutations.

Validation so far: JSON syntax and manual mapping to pinned upstream interfaces.
Lua parsing, upstream package validation and actual runtime execution: NOT_RUN
(no Lua or documented Windows build toolchain available on PATH).

The [strict guard patch](../../patches/shroudforge/README.md) now passes focused synthetic tests. Deployment remains held: the exact current-build execution profile is absent, and the complete production artifact is unbuilt. This lifecycle probe remains disabled and untested in-game.
