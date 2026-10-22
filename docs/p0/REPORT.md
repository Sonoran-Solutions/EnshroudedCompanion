# Phase 0 feasibility checkpoint — HOLD

Date: 2026-10-22. P0 was explicitly authorized. This bounded session completed source investigation and offline identity tooling; live feasibility is **not established**. Owner disposition on continuation beyond P0: **PENDING**. P1 and all later phases remain held.

## Follow-up checkpoint — disposable sessions and active update

The owner clarified that shared-player worlds can be loaded OFFLINE using the default character, are disposable, and return to their baseline after exiting and reopening. This is the authorized test-world/reset approach; a new local world is no longer required. Reset behavior remains owner-reported, not observed. Verify it before experimental mutation, including whether character state is isolated. Available computer tools do not control native game menus, so menu interactions/reset observations require a human operator.

Steam began updating Enshrouded during preflight. The fingerprint below is **historical**, not current launch authorization. No new fingerprint was captured during the update. Installed Proton Experimental version file reads `1791989105 experimental-11.0-20261014-x86_64`; its actual use and runtime compatibility remain untested.

**Current RUNTIME verdict: HOLD from inspected startup safety blockers; all live runtime assertions remain NOT_RUN.** [Detailed preflight](RUNTIME_PREFLIGHT.md) shows the selected stock loader patches the entrypoint before exact-profile checks and does not propagate all required initialization failures. No guards were bypassed. The other four capabilities remain NOT_RUN. No loadable native artifact was installed or launched.

An [inert lifecycle probe](../../probes/shroudforge/README.md) is now prepared and disabled. It only logs lifecycle callbacks. JSON syntax passed; Lua/native/package runtime validation NOT_RUN. It cannot repair or substitute for the underlying loader guards. Toolchain inspection found no ready documented Windows build toolchain on PATH. A bounded guard-hardening effort is within approved P0 but cannot be deployed without a runnable native negative-test route.

Follow-up effort: approximately 10 minutes elapsed, about 15 aggregate agent-minutes estimated. No executable downloads, game launches, builds, saves, Steam settings or update state were changed. The approved cap remains largely unspent. Next engineering task is strict loader startup/refusal hardening and its native negative tests, after fresh post-update identity; default-character/offline reset verification is also still required. The earlier 0.5–1-day load-test estimate is conditional on resolving this newly confirmed safety blocker, not a promise that stock deployment can proceed.

## Owner decisions and environment

The owner confirmed in this session: native NPC embodiment is acceptable; test only on this machine with the Steam installation; only one authorized client is available; the owner will create a disposable world (none exists yet); the proposed 5–10 focused developer-day maximum and runtime installation/mutation permissions are approved. This supersedes the planning-only authorization boundary for P0, not for later phases or valued saves.

Selected single investigation candidate: **ShroudForge**, commit `3ed90de1df266c4f0c217523a02a71dfa67243de`. See [source review](SOURCE_REVIEW.md) for comparison with EML, Shroudtopia and Ember and the guard limitations. Selection is not compatibility approval. Do not build or launch competing runtimes.

The local client manifest is Steam app `1203620`, build `26082781`; executable SHA-256 `d64966e23b1a458ee6f147bcbceac0b577fa7e39a212f371287d8cc119817882`, file length `35338312`. PE timestamp `1792507660` and SizeOfImage `44163072` match the candidate's shipped client profile. Host: Linux `7.0.0-38-generic`, x86_64. Exact Wine/Proton launch version and mod-loading compatibility remain unverified. Historical game log revision `1273043` is separate from the current file identity; it is not a fresh runtime test or proof of semantic version 1.0.0.3. No dedicated-server manifest was found in the registered Steam library; remote/manual installations were not inventoried because testing is limited to this machine.

## Capability matrix

| Capability | Verdict | Evidence | Blockers |
| --- | --- | --- | --- |
| RUNTIME | HOLD (source); live NOT_RUN | Historical file/profile match; inspected startup guard failures | Active Steam update; pre-hook exact-build refusal and required-symbol propagation absent; reset not demonstrated; Proton runtime untested |
| OBSERVATION | NOT_RUN | Source contains bounded ECS readers; no actual snapshots collected | RUNTIME gate; owner/entity/world identity, health/position semantics, sustained freshness and reload behavior unproven |
| QUEST | NOT_RUN | No scoped quest API established by targeted source review | Runtime/observation gates; personal/world scope unresolved; second client unavailable |
| ACTOR | NOT_RUN | Generic candidate SpawnEntity path inspected as building-prop operation, not NPC control | Identity/authority gate; NPC ownership, navigation, combat and cleanup path unproven; second client unavailable |
| BUILDING | NOT_RUN — voxels and props separately | Source has native voxel and prop operations; no operation attempted | Offline disposable reset unverified; native material debit/permissions/recovery unproven; second client unavailable |

Overall HOLD reflects missing test prerequisites and unresolved technical surfaces, not proof that the game cannot support a companion. No passing core subset exists from which to approve a reduced-scope implementation. Single-client testing, once possible, cannot satisfy the required multiplayer and two-client quest assertions. Do not silently reduce those criteria.

## What actually ran

- Read all required project documents and all five full Linear tickets/relations. Dependencies: DYL-136 → DYL-137 → DYL-138 and DYL-139; all feed DYL-140. Completed research or ticket status never substitutes for a passing gate.
- Sol inspected pinned public source for four candidates; parent spot-checked loader stubs, profile identity and fallback guards. Luna prepared the [scenario matrix](SCENARIOS.md); parent reviewed and corrected missing acceptance cases.
- Read local manifest and executable identity, and extracted only allowlisted version strings from an existing log. No private saves/raw logs/binaries are committed.
- Implemented the [offline identity collector](../../tools/p0_fingerprint.py). Three focused synthetic-input tests pass; this is input/hash/privacy validation, not a simulated native companion.
- **Nothing was exercised in-game by this task.** No runtime DLL was built or loaded; no backup was made/restored; no actor, quest, resource or world state was modified. All live cases in the matrix remain NOT_RUN.

## Evidence and revisions

- Base: `96212a728dd2ae7866b3d3ffe644f873480fb10e` on `docs/initial-implementation-plan`; `main` remains `c800edb`. The P0 PR is stacked onto the documentation branch; no silent merge or duplicated initial-documentation diff.
- Tested code and machine-readable evidence commit: `d758ab5541ea7985333ab8f094a629d0d5fffb4b`. Later report-only changes do not imply additional runtime tests.
- [Sanitized inspection log](../../evidence/p0/inspection-log.md), [client identity](../../evidence/p0/client-fingerprint.json), [targeted test output](../../evidence/p0/offline-tests.txt), [source pins and findings](SOURCE_REVIEW.md), [reproduction](REPRODUCTION.md).
- Native builds and all live acceptance tests NOT_RUN because the disposable environment and required safety proof are absent. Broad test suites were unnecessary for the collector/documentation change.

## Time, resources and cleanup

This checkpoint used approximately 15 minutes elapsed orchestration and roughly 25–30 aggregate agent-minutes including parallel Sol/Luna source and acceptance work; these are estimates, not measured developer-day burn or token billing. Exact token/cost accounting is unavailable. The approved 5–10 focused developer-day cap is largely unspent; no two-day subsystem stop threshold was reached. No unattended continuation or background multi-day job is scheduled.

Network clones were bounded at 30 seconds each; fingerprint and targeted tests were bounded at 30 seconds each. Source work targeted 10 minutes with a 15-minute cap; acceptance drafting targeted 5 minutes with a 10-minute cap and a short revision pass. All completed. No game/server/inference jobs were launched; existing Steam processes were left untouched. Public source clones remain in `/tmp/enshrouded-p0-source-review/` for reproducible inspection; no private game material is stored there. No provider inference, purchases or deployment occurred.

## Recommended next task

**Resume DYL-136 using the owner-authorized offline disposable sessions, after the update and strict runtime guard blockers are resolved.** Establish and demonstrate its consistent backup/restore on this machine, pin the exact Proton/runtime toolchain, and resolve strict unsupported-build/missing-symbol behavior for the selected ShroudForge probe before a short load/disable/restart test. Expected initial engineering effort: 0.5–1 focused developer-day, conditional on toolchain and guard complexity; stop/report rather than exceed the subsystem limit. Record the exact job command, output, timeout, process owner and safety assertions before launch.

Only after RUNTIME passes, attempt the smallest read-only identity/snapshot slice of DYL-137. Two-client quest/replication coverage remains blocked unless authorized second-client access becomes available or the owner explicitly approves a precisely documented reduced verification scope. Neither an observer-only product nor second-client automation is implemented here. No P1 task is recommended to start yet.

The evidence PR must remain open and unmerged with auto-merge disabled. This report closes the present blocked checkpoint, not the unexecuted live acceptance work. Linear tickets retain unresolved acceptance criteria and link to the evidence; none is represented as a capability pass.
