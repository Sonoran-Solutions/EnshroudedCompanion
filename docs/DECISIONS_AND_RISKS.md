# Decisions and risks

Updated 2026-10-22. Do not convert a recommendation into an accepted decision without recording the owner's choice and date.

## Confirmed requirements and authorization

| ID | Status | Record |
| --- | --- | --- |
| R001 | OWNER_REQUEST | Explore an AI-controlled companion that fights, adventures, builds and roleplays with game/quest awareness |
| R002 | OWNER_REQUEST | Consider local model backends and a hosted option such as OpenRouter |
| R003 | AUTHORIZED | Consolidate research and an implementation plan as documentation in this repository |
| R004 | NOT_AUTHORIZED_BY_THIS_TASK | Runtime implementation, paid inference, production-world mutation, issue creation, deployment or merging the documentation PR |

R004 describes the limits of this documentation task, not a permanent ban. A later explicit implementation request can authorize a bounded next task.

## Proposed architecture choices

| ID | Proposal | Why | Status |
| --- | --- | --- | --- |
| D001 | Native host-authoritative actor first | Potential reuse of game behaviors without a second rendered client | PROPOSED; P0-dependent |
| D002 | Deterministic controllers below model intent | Responsiveness and bounded actions | PROPOSED |
| D003 | Out-of-process model/context service | Avoid inference/network work on the game thread | PROPOSED |
| D004 | One logical provider interface for local and OpenRouter routes | Swappable backends with explicit feature checks | PROPOSED |
| D005 | Text-first, one companion, narrow traversal/combat | Smallest meaningful embodied slice | PROPOSED |
| D006 | Blueprint building before freeform layouts | Inspectable plans, resource limits and recoverability | PROPOSED |
| D007 | Separate construction gate from core companion gate | Preserve honest feature-level feasibility decisions | PROPOSED |
| D008 | Optional Jev decision-policy experiment | Interesting alternative without blocking dialogue/core work | PROPOSED |
| D009 | 5–10 developer-day P0 cap with subsystem stop points | Bound uncertain integration effort | PROPOSED; ratify at kickoff |

These choices guide the plan but are not marked owner-approved merely because the owner requested documentation.

## Open owner choices

Resolve only when needed; do not block source research on every future preference.

| Choice | Needed by | Proposed default |
| --- | --- | --- |
| Does a native NPC satisfy "companion player," or must it be a true player avatar? | P0 route selection | NPC acceptable if it delivers the experience; confirm |
| Test deployment and access: local host/dedicated server, OS, authorized second client | P0 kickoff | Private disposable dedicated test world if the selected bridge requires it |
| Exact investigation/time/resource budget | P0 kickoff | Bounded plan in IMPLEMENTATION_PLAN.md |
| Acceptable reduced scope if quests, actors or voxels are blocked | P0 review | No silent downgrade |
| Host/client installation requirements | P0/P1 | Minimize, but follow evidence |
| Provider/model, key handling, data sharing and spend limit | P1 live API test | Local-only until explicitly configured |
| Combat aggression, friendly fire, catch-up teleport and death policy | P2/P4 | Conservative, no unapproved teleport/resurrection |
| Persona, voice, persistence and spoiler preferences | P3 | Text-first; discovered knowledge only |
| Building regions, resource sources, demolition and recovery authority | P5 | Additive approved blueprints only |
| Project/dependency distribution license and release scope | Before public release | No license choice presumed |

## Risk register

| Risk | Impact | Mitigation / stop rule |
| --- | --- | --- |
| Required native hooks unavailable | Core project blocked | Bounded P0; explicit alternative or reduced scope |
| Patch changes symbols/state layout | Crashes or invalid reads/writes | Exact fingerprint, verified capability manifest, fail closed |
| NPC cannot traverse/fight like a player | Poor companion experience | Narrow actor probe before persona/polish investment |
| Quest scope/selection ambiguous | False roleplay, spoilers | Two-player matrix; provenance and unknown states |
| Duplicate/stale actions after reload | Wrong-world actions or material loss | Identity/epoch binding, journal, reconciliation, no blind replay |
| Construction/resources not transactional | Corruption, duplication or loss | Separate gate, bounded native edits, tested recovery |
| Model latency/unavailability | Unresponsive gameplay | Deterministic controllers, bounded queues and fallback |
| Local model competes with game GPU | Stutter or memory failure | Small-model baseline and measured concurrent workload |
| Provider capability/routing changes | Invalid actions or privacy drift | Capability checks, route metadata, no silent fallback |
| Prompt injection / unauthorized control | Unwanted game actions or data exposure | Authenticated owner policy and constrained executor |
| License/terms conflict | Distribution restrictions | Audit before bundling/release; do not assume permission |
| Runaway jobs / over-parallelization | Wasted time or conflicting world writes | Job budgets, process ownership, one live writer |
| Good demo mistaken for completion | Underestimated remaining work | Exact scenario evidence and feature-specific status |

## Recording future decisions

Append the choice, date, owner instruction, affected documents and consequences. Keep superseded decisions with a replacement reference. Never rewrite a HOLD as PASS because the roadmap would otherwise be blocked.

Source evidence belongs in [research](FEASIBILITY_AND_RESEARCH.md); execution evidence belongs in [validation](VALIDATION_AND_OPERATIONS.md); current phase/gate state belongs in [status](STATUS.md).

## P0 kickoff decisions — 2026-10-22

The owner explicitly authorized P0 and Sol/Luna orchestration, accepted a native NPC companion, restricted tests to this machine's Steam installation, confirmed only one client, and approved the proposed 5–10 focused developer-day ceiling plus runtime installation/mutation permissions. No disposable world exists yet; the owner will create one. This supersedes R004 for bounded P0 only. No P1, alternate embodiment, valued-world changes, merge or auto-merge is authorized.

ShroudForge `3ed90de1` is the single selected investigation candidate based on inspected source and matching local client fingerprint. It is not runtime-verified. Unknown-build fallback paths, scoped quests, actor control and material accounting remain risks; see [P0 report](p0/REPORT.md). All live capabilities are NOT_RUN. Single-client availability cannot satisfy multiplayer or two-client quest acceptance. Further owner disposition is PENDING.

## P0 disposable-session clarification and update

Owner authorizes the default character and any shared-player world loaded OFFLINE, reporting that these sessions reset to baseline on exit/reopen. This supersedes waiting for a newly created world. Reset is not yet observed; actor/construction gates still require demonstrated recovery and character-state isolation. Steam is updating Enshrouded, so previous file identities are historical. Source inspection establishes RUNTIME HOLD for strict startup refusal in stock ShroudForge; live tests remain NOT_RUN. See [runtime preflight](p0/RUNTIME_PREFLIGHT.md). No runtime installed, guard disabled or game launched.
