# Project status

Updated: 2026-10-22. **P0 HOLD — loader binaries built and synthetic checks pass; execution-hook context unresolved; live acceptance NOT_RUN.**

See [Phase 0 report](p0/REPORT.md) for evidence, exact code/source/build identities, time, blockers and next task.

| Area | State |
| --- | --- |
| P0 kickoff, native NPC embodiment, this-machine-only testing | OWNER_APPROVED |
| Investigation ceiling and runtime installation/mutation permissions | OWNER_APPROVED: proposed 5–10 focused developer-days |
| Single selected candidate | ShroudForge `3ed90de1`; compatibility NOT_VERIFIED |
| Local client identity | Steam build 26082781, recorded SHA-256 matching candidate profile |
| Offline collector | Three targeted tests PASS; no live capability implied |
| Disposable test world / demonstrated restore | OWNER_AUTHORIZED offline shared-world reset; reset observation NOT_RUN |
| Second authorized client | UNAVAILABLE; one client only |
| RUNTIME | HOLD: guard fixes tested, current-build execution profile absent; live NOT_RUN |
| OBSERVATION / QUEST / ACTOR / BUILDING | NOT_RUN |
| Current client identity | Post-update build 26082781 and SHA-256 unchanged; exact read-profile match only |
| Inert lifecycle probe | Prepared disabled; not installed or executed |
| P1–P6 | NOT_STARTED; no continuation authorization |
| Owner go/hold disposition beyond P0 | PENDING |
| Project license | NOT_SELECTED |

Next: resolve the current scheduler/query-frame contract and validate a current execution adapter. Required selected-path loader binaries now build and pass synthetic load/refusal checks; see [build evidence](../evidence/p0/build-investigation/README.md). The guard patch correctly refuses the current read-only profile. Offline reset check waits until a safe runtime candidate exists. Do not consume downstream dependencies as if they passed. Two-client quests and multiplayer assertions remain untested without another authorized client or an explicitly accepted reduced verification scope.

Guard patch/evidence: `4919b51e0acc5d7a2a75b7215af654946ae45f28`; [reproduction](../patches/shroudforge/README.md).

Current build/profile code and evidence commit: `158ec77`. This supersedes the earlier unbuilt-artifact limitation, while retaining RUNTIME HOLD and all live checks NOT_RUN.
