# Phase 0 implementation handoff

Status: **P0 authorized; loader binaries built, live execution held on current execution-hook context before disposable-world reset verification.** See [checkpoint](p0/REPORT.md). This remains the bounded task brief. Use after the owner explicitly authorizes Phase 0 and provides an appropriate test environment. This document can be handed to an orchestrator without starting the rest of the roadmap.

## Mission

Determine whether Enshrouded Companion can obtain truthful live state and safely control one embodied companion on the selected game build. Evaluate native construction as a separate capability. Return evidence-backed feasibility decisions, not a polished chatbot or an unbounded framework.

Read, in order:

1. [Status](STATUS.md).
2. [Decisions and risks](DECISIONS_AND_RISKS.md).
3. [Feasibility and research](FEASIBILITY_AND_RESEARCH.md).
4. [Implementation plan](IMPLEMENTATION_PLAN.md), P0.
5. [Architecture](ARCHITECTURE.md).
6. [Validation and operations](VALIDATION_AND_OPERATIONS.md).
7. [Building](BUILDING.md) before the construction probe.

## Kickoff checks

- Confirm whether an NPC embodiment is acceptable or a true player avatar is required.
- Confirm authorized client/server access, matching builds, OS/compatibility layer, and availability of a second test client.
- Select a private disposable world. Establish a consistent backup and verified restore procedure.
- Ratify the proposed 5–10 developer-day investigation cap and smaller per-job limits.
- Inspect the actual repo and its current instructions; preserve any existing work.
- Refresh upstream evidence and choose one candidate loader/runtime path. Record exact commits and binaries.
- Identify permissions or terms restrictions before performing experiments. Do not bypass a blocker.
- Record unresolved prerequisites. If access or a required test client is absent, complete safe source/fixture work only and mark live checks NOT_RUN.

Do not ask the owner to decide every future persona or building detail now. Ask only choices that materially affect the spike.

## Work allocation

Assign one native integration owner. That worker alone controls the disposable world and live mutations.

A second worker may prepare versioned fixture/schema proposals and a small fake provider. A third may review sources and acceptance evidence. No worker starts a parallel loader implementation, launches competing game processes or writes to the same world.

The orchestrator remains responsible for dependencies, time/resource budgets, evidence and stopping at the gate. Workers may not convert a failed capability into an assumed fallback.

## Probe order

1. **P0-01 runtime:** validate exact build matching, load/disable/restart and version mismatch behavior. Unknown symbols/builds fail closed.
2. **P0-02 read-only:** obtain usable sustained observations with identity, provenance, sequence/freshness and resynchronization. Exercise personal/world quest scope and selected-objective handling using the plan's two-client matrix.
3. **P0-03 actor:** only after safe reads/authority pass, use a bounded deterministic command loop to test ownership, follow/stop/assist, obstruction or elevation, authoritative effects, replication, cleanup and reload.
4. **P0-04 construction:** only in the disposable approved area, test one native edit type with permission/resource checks, duplicate refusal, replication, save/reload and interrupted-operation recovery. Distinguish voxels from props.
5. **P0-05 report:** produce per-capability verdicts and the smallest next slice supported by evidence.

A one-time memory read, loader startup, teleported actor, stubbed attack, screenshot-only quest description or raw save edit is not an equivalent pass.

## Stop conditions

Stop the affected capability on unsafe/unexplained writes, unsupported binaries, uncertain actor ownership, failed cleanup, corruption/material duplication, access restrictions or exhausted budget.

After roughly two focused days without a reproducible route for a subsystem, report the blocker and use the remaining approved budget deliberately. Do not keep expanding reverse-engineering scope or start paid inference to compensate.

Do not restore a shared/valued world, install unrelated tooling, purchase a second copy, create external accounts, change provider privacy settings or broaden network exposure without new authorization.

## Required deliverables

- A minimal probe or a precise blocked report, not a full runtime.
- Exact integration candidate commit, game/client/server fingerprints and environment details.
- Capability manifest with each required field/action labeled verified, unavailable or unknown.
- Redacted reproducible scenarios, relevant logs and recorded observations.
- Quest-scope and actor/replication/cleanup matrices with PASS/FAIL/BLOCKED/NOT_RUN assertions.
- Separate RUNTIME, OBSERVATION, QUEST, ACTOR and BUILDING verdicts: PASS, HOLD or NOT_RUN.
- Known limitations, time spent, owned process cleanup and unresolved risks.
- A proposed next-slice estimate and owner decisions, only if justified.
- Updated research/status docs referencing the tested code commit.
- A reviewable PR with no automatic merge and no game assets, private saves, keys or proprietary binaries.

For PASS, include actual runtime evidence. For HOLD, explain the specific missing surface or failure and the bounded attempts made. For NOT_RUN, name the prerequisite that prevented the test. Do not report an untested feature as a failure or success.

## Completion boundary

Stop after the P0 evidence PR and handoff. Do not merge, start P1, implement a second-client fallback, launch an observer-only product, or advance to building automatically.

Recommend one of: continue the explicitly proven core, continue with an owner-approved reduced scope, authorize a separately bounded alternate-route experiment, or pause the project. Keep the construction result independent and visible.
