# Bounded current-build observation hook plan

Status: **UNINSTALLED — offline reset and live-job gates pending**. This plan is within the owner's approved P0 work; it neither approves a production execution profile nor authorizes a second runtime path. See [scheduler evidence](../../evidence/p0/profile-investigation/README.md) and [safety review](../../evidence/p0/scheduler-safety-review.md).

## Admission and scope

The only candidate is the original `actor_simulation_start` entry for Steam build 26082781, executable SHA256 `d64966e23b1a458ee6f147bcbceac0b577fa7e39a212f371287d8cc119817882`. Static evidence identifies its incoming RCX as a borrowed iterator, with a stack frame referring to the manager. Both inline and worker invocation exist. This is not permission to dereference that pointer in a telemetry callback, retain it for later access, call engine functions, or drain gameplay commands.

An observation-only adapter must have a separate typed diagnostic descriptor/target, outside production profile selection. The current read profile remains read-only; `runtimeLayoutValidated` remains unset. The separate diagnostic build composes the selected ShroudForge exact-identity and startup guards with its own typed target; production required-capability checks remain unchanged. See [build and synthetic integration results](../../probes/p0-shroudforge-diagnostic/README.md).

The proposed callback stores an opaque iterator number and thread ID in fixed storage, with bounded atomic reservation, no pointer dereference, allocation, blocking, logging or gameplay dispatch. Full storage or contention drops observations. Disable is one-way for a run; an already admitted capture may finish. Counters describe capture attempts, not ticks or unique actors. Raw addresses must stay local and be replaced with stable per-run labels in committed evidence. This telemetry alone cannot establish active-world ownership or sustained observation/quest feasibility.

## Required engineering gates

- Match the whole immutable file SHA, size, PE identity, target kind and loaded-image identity; refuse changes or ambiguity. Keep the source file protected from replacement during admission. Recheck original loaded bytes immediately before mutation.
- Verify one executable entry match and decoded complete overwrite, not historical short snippets. The 11-byte overwrite contains stack-changing instructions covered by the entry fixtures.
- Review register/flags/SIMD preservation, Windows x64 stack/shadow space, and instruction replay. Register unwind metadata for all generated code and test partial prologue and restoration boundaries, not just the callback body. Native Windows remains untested on this Linux-only project.
- Install before engine worker startup if using the approved bootstrap lifecycle; otherwise require a separately demonstrated patch transaction. Never weaken thread/RIP/expected-byte checks to force installation. Fail without publishing partially prepared code.
- Pin callback, telemetry, trampoline and function table storage through process exit. Disable capture without concurrent unpatch, unload or free. A restart is the cleanup boundary. Verify failure before publication leaves no executable patch.
- Keep actor, inventory, construction and general queue entry points absent. The same-Scene shared lock prevents the existing exclusive write guard from succeeding at this callback; do not spin or upgrade that lock.

These gates require code and fixture results; prose or a passing static scan does not satisfy them.

## Owner-assisted offline reset — next operational gate

This is the next owner-assisted gate. A private copy backup and separate-directory hash-verified restore rehearsal passed; this does not establish in-game recovery. Run the game **without the experimental adapter first** and guide the owner through:

1. Use the authorized default character and choose one identifiable shared-player template in **OFFLINE** mode. Record template/version, local/offline indicator, and character/inventory baseline; stop if the menu cannot confirm offline mode or character isolation is unclear.
2. In an allowed disposable area, make one ordinary, reversible change through the game's normal controls that clearly distinguishes the session from the template. Record the change and any inventory cost. Do not use valued saves or experimental native writes for this check.
3. Leave the session and reopen the same template offline. Compare the marked area and character/inventory state with the baseline. A restored world with persistent unwanted character loss does not prove full recovery. Record observations, not just the reported reset behavior.
4. Close the game cleanly. Confirm one game process has ended before installing any reviewed diagnostic files. If reset/character recovery differs from expectations, stop and resolve it before the experimental session.

## Live job, not launched

Purpose: observe original callback cadence and concurrency, then verify disable/restart without game-state actions. Expected active observation: 60–120 seconds; maximum session: 5 minutes, including load. Single owner-operated game process, one adapter, no server. Exact artifact hashes, launch command/Steam options, watchdog/termination method, source revision and log paths must be filled in from the final reviewed build before launch; this draft is not an executable job specification.

Success criteria: guarded load on the exact build; bounded captures during the chosen offline session; no callback-side dereferences/actions; clean one-way disable; no further captures after in-flight callbacks finish; clean exit; subsequent adapter-disabled launch; sanitized evidence. Failure, timeout, missing symbols, identity drift or unexpected state change ends the attempt. No retry with relaxed checks.

Even a successful observation-only job leaves production RUNTIME acceptance incomplete until the capability manifest, required initialization, unsupported-build refusal and disable/restart evidence qualify that route. OBSERVATION, QUEST, ACTOR and BUILDING require their own later gates. No P1 continuation is implied.
