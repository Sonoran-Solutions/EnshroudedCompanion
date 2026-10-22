# Implementation plan

Status: **PROPOSED; all phases NOT_STARTED**. This is a task plan, not evidence that the underlying game hooks exist. Task IDs below are document identifiers, not created GitHub issues.

The owner authorized the documentation package. Starting experiments, choosing a paid provider, accessing a game installation or proceeding to a later phase requires the corresponding task authorization. Do not treat a roadmap dependency as permission to run it.

## Sequence and gates

| Phase | Outcome | Depends on | Exit evidence |
| --- | --- | --- | --- |
| P0 | Feasibility verdict for the actual game build | Explicit start, disposable world and test access | Separate runtime, observation, quest, actor and construction results |
| P1 | Versioned contracts and backend skeleton | P0 integration direction and owner go decision | Deterministic fixture tests, local/hosted provider contract tests |
| P2 | Embodied deterministic companion | P1 plus verified P0 actor/observation path | Follow/stop/assist and cleanup in a bounded live scenario |
| P3 | Contextual roleplay playable | P1; live acceptance requires P2 and quest gate | Grounded conversation, scoped memory, provider switching; optional voice |
| P4 | More reliable adventuring | P2 and basic P3 integration | Traversal, hazards, inventory and recovery matrix |
| P5 | Small blueprint builder | P1, P2, P4 inventory slice and construction gate | Approved construction, resources, interruption and reconciliation |
| P6 | Release hardening | Owner-approved feature scope and its completed gates | Packaging, compatibility, performance, privacy and recovery evidence |

A core-only release could omit P5 only after the owner explicitly accepts that scope. A builder gate failure must remain visible; it is not equivalent to completing the original vision.

## P0 — prove the bridge

**Proposed budget: 5–10 focused developer-days total.** Ratify the budget at kickoff. It is a cap on investigation, not a promised completion date. Use one selected runtime path, one disposable world, and the smallest native probe. Do not build competing frameworks in parallel.

Suggested allocation: one day for candidate selection/build setup; two for observations and quest scope; two for actor control; one for construction probe; one for evidence/review. Any remaining allowance addresses a specific promising blocker. Stop a capability after roughly two focused days with no reproducible route; do not let an unbounded reverse-engineering task consume the whole project.

| ID | Task and deliverable | Dependency | Acceptance |
| --- | --- | --- | --- |
| P0-01 | Select one loader/runtime path; record exact upstream commit, client/server fingerprints and toolchain | Owner kickoff | Repeatable load/disable; unsupported build refused before unsafe hooks; test-world backup/restore established |
| P0-02 | Read-only observation and quest probe; capability manifest and recorded fixtures | P0-01 | Sustained snapshots with identity, scope, freshness and gap handling; quest matrix below |
| P0-03 | Actor ownership and command probe | Safe read/identity subset of P0-02 | Verified follow/stop/assist/cleanup/replication using bounded commands |
| P0-04 | Isolated native construction probe | P0-01 plus identity/authority observation subset | Edit-type-specific material, permission, replication, persistence and recovery evidence |
| P0-05 | Evidence review and capability verdict | P0-02, P0-03, P0-04 concluded or explicitly blocked | Per-capability PASS/HOLD/NOT_RUN, selected route, remaining blockers and next-slice estimate |

The read-only gate precedes any actor or construction mutation. A completed P0-02 task means a documented result, not necessarily a pass; downstream tasks may use only the independently passed subsets.

### P0 test matrix

**Runtime:** actual installed client and server builds, loader commit and binary fingerprint; load, disable, restart; version mismatch; Linux/Wine/Proton versus native Windows recorded separately. Do not assume native Linux mod support.

**Observations:** stable owner/entity identifiers or an explicit safe identity fallback; position, health and supported nearby targets; timestamps/sequences; sustained short session; dropped/duplicate events; reconnect and world/load changes. A one-time memory peek is insufficient.

**Quests:** two authorized test clients with differing personal progress; one personal quest transition; one world quest transition; late join; reconnect/reload. Verify which player/world each field describes. Record selected objective independently; owner-provided selection is allowed if journal selection is unavailable. If a required client or quest scenario cannot be obtained, mark NOT_RUN, not PASS.

**Actor:** attach/spawn one identifiable friendly actor; follow around an obstruction and one representative elevation or streamed-area transition; stop; assist against a designated target; observe authoritative damage and second-client visibility; detach/despawn; owner disconnect; reload without duplicates/orphans. Teleporting an actor is not a navigation pass. Unsupported traversal remains out of scope.

**Construction:** one whitelisted voxel and/or prop pathway, labeled by the exact supported edit type; allowed/denied area; material debit and insufficient-material refusal; second-client visibility; save/reload; duplicate request; interrupted operation and isolated backup recovery. Prop-only success must not be reported as voxel support.

### P0 decision rule

Record RUNTIME, OBSERVATION, QUEST, ACTOR and BUILDING separately. Each is PASS, HOLD (failed/blocked with evidence) or NOT_RUN.

- Core companion continuation requires RUNTIME + OBSERVATION + ACTOR + QUEST pass for the explicitly documented initial slice.
- If actor/observation pass but quests do not, offer a reduced non-quest-aware companion only as an owner decision.
- BUILDING can remain HOLD while an explicitly scoped core proceeds. P5 stays blocked.
- If native actor control fails, propose either a separately scoped real-client experiment or an observer-only downgrade. Neither is an automatic substitute or a P0 success.
- Stop on unsupported binaries, unreliable ownership/removal, unsafe writes, access restrictions or exhausted investigation budget.
- A P0 pass permits planning the next slice, not claiming public-playable compatibility.

## P1 — contracts and backend foundation

| ID | Task | Acceptance |
| --- | --- | --- |
| P1-01 | Implement versioned observation, capability, intent and result schemas from P0 evidence | Unknown fields/capabilities handled explicitly; version mismatch and stale/foreign identity rejected |
| P1-02 | Build replay/fake bridge and deterministic test provider | Reproduce ordered/gapped events, reload, cancellation, duplicate and unknown-outcome cases |
| P1-03 | Implement one local and one OpenRouter text adapter | Explicit feature negotiation, timeouts, bounded queues, usage ledger and no silent cloud fallback |
| P1-04 | Add owner controls, policy checks and minimal persistence | Authenticated stop/disable; permission denial; world-scoped memory reset; no secrets committed |
| P1-05 | Run shadow mode against read-only live observations | Log proposals without executing; recorded provenance; local/hosted capability smoke results |

P1-02 and P1-03 can proceed in parallel after P1-01. P1-04 consumes those contracts. Offline tests do not satisfy P1-05 or any live-game gate. Hosted smoke tests need an authorized key and spending limit; if unavailable, report the adapter's fixture tests separately from NOT_RUN live API tests.

## P2 — embodied deterministic companion

Implement P2-01 owner/actor lifecycle, P2-02 follow/stop/guard controllers, P2-03 bounded combat assist/retreat, and P2-04 disconnect/reload cleanup and live acceptance.

Initially issue commands from the explicit owner UI, with no model in the actuation path. This isolates game-control defects from model behavior. Connect planner proposals only after the same executor passes deterministic tests.

Exit: a short recorded route and encounter, owner stop latency measured, no unintended targets, no duplicate actor, correct second-client state, provider-disabled operation, and safe recovery. Test both success and refusal. Specify the supported actor/weapon/traversal subset; do not claim general dungeon competence.

## P3 — grounded roleplay

Implement P3-01 persona/context builder and selected objective, P3-02 scoped confirmed-event memory and rollback handling, P3-03 local/hosted text parity, and optional P3-04 push-to-talk/STT/TTS.

Context and memory work can use fixtures in parallel with P2 after P1 contracts stabilize. Live exit still requires P2 plus a passed quest-state slice. Voice is optional and does not block text acceptance.

Exit: the companion references an actual shared event, correctly distinguishes personal/world progress, acknowledges unknown facts, avoids undiscovered spoilers, remembers a verified event across a permitted reload, and never narrates an unconfirmed action as success. Exercise memory deletion and local-only mode. Record quality failures rather than hiding them behind an aggregate score.

## P4 — reliable adventuring

Implement P4-01 traversal/hazard matrix and bounded stuck recovery, P4-02 allowed gathering/inventory transfers, and P4-03 longer mixed-session regression.

Each traversal or interaction becomes its own capability. Inventory operations must preserve ownership, quantities and resource outcomes on cancellation/reconnect. Any catch-up teleport, death penalty or automatic resurrection requires an explicit gameplay decision.

Exit: a representative route covering the declared subset, interrupted combat, owner death/disconnect, blocked paths and inventory conflicts. Report unsupported mechanics. No unattended open-world endurance test before short bounded cases pass.

## P5 — blueprint construction

Implement P5-01 blueprint schema/compiler and preview, P5-02 owner approval/resource/site policy, P5-03 bounded native executor and journal, and P5-04 interruption/reconciliation acceptance.

P5-01 can be prototyped offline once the schema is stable, but no live building starts without the P0 edit-type gate and explicit phase authorization. Use a tiny additive template before layout generation.

Exit: preflight and build a small structure with correct materials, deny an unauthorized site, stop/resume without duplicate operations, detect a human edit, survive a restart, and demonstrate an approved recovery procedure. A whole-world backup is not a claim of atomic per-job rollback. See [building](BUILDING.md).

## P6 — hardening and release candidate

Implement P6-01 installation/configuration/removal documentation, P6-02 tested compatibility manifest and fail-closed updates, P6-03 performance/cost/privacy evaluation, and P6-04 bounded release-candidate soak and recovery review.

Audit dependencies, mod/voice/model licenses and applicable game/provider terms. Do not redistribute game assets, saves, credentials or proprietary binaries. Select the project's license explicitly.

Exit: clean installation and removal on the target topology, supported-version evidence, local and selected hosted failure tests, measured resource use, restore procedure, known limitations and owner playtest acceptance. Unsupported environments stay NOT_VERIFIED.

## Parallel work and ownership

| Window | Safe parallel work | Serialization boundary |
| --- | --- | --- |
| P0 | Source review, fixture/schema drafting, small provider research | One native integrator owns the disposable world and runtime writes |
| P1 | Provider adapters versus fake bridge/tests | One contract owner; changes reviewed before consumers update |
| P2/P3 | Deterministic controllers versus offline persona/memory tests | One live-game test owner; no concurrent actor controllers |
| P4/P5 preparation | Traversal tests versus offline blueprint compiler | Inventory and live world-edit changes integrated serially |
| P6 | Packaging docs, privacy/license review, targeted regression | One release candidate and bounded hardware sessions |

An orchestrator should assign file ownership and acceptance evidence per task. Parallelism does not mean starting every phase, launching duplicate servers or allowing multiple agents to mutate the same world.

## Estimates and handoff discipline

Re-estimate after P0 using the selected hook surface and observed failure modes. Budget in focused engineering effort; multiple agents do not remove binary reverse-engineering or hardware test bottlenecks. A polished full-featured companion is not a weekend commitment.

Each implementation PR must state exact head/base, task IDs, changed contracts, targeted checks, live checks NOT_RUN, observed limitations, process cleanup and remaining owner decisions. Do not merge, enable auto-merge, broaden scope or advance phase automatically. See the [Phase 0 handoff](PHASE_0_HANDOFF.md) and [validation rules](VALIDATION_AND_OPERATIONS.md).
