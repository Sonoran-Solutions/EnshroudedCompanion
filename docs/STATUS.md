# Project status

Updated: 2026-10-22. **P0 HOLD — source investigation completed; live acceptance NOT_RUN.**

See [Phase 0 report](p0/REPORT.md) for evidence, exact code/source/build identities, time, blockers and next task.

| Area | State |
| --- | --- |
| P0 kickoff, native NPC embodiment, this-machine-only testing | OWNER_APPROVED |
| Investigation ceiling and runtime installation/mutation permissions | OWNER_APPROVED: proposed 5–10 focused developer-days |
| Single selected candidate | ShroudForge `3ed90de1`; compatibility NOT_VERIFIED |
| Local client identity | Steam build 26082781, recorded SHA-256 matching candidate profile |
| Offline collector | Three targeted tests PASS; no live capability implied |
| Disposable test world / demonstrated restore | NOT_AVAILABLE / NOT_RUN; owner will create world |
| Second authorized client | UNAVAILABLE; one client only |
| RUNTIME / OBSERVATION / QUEST / ACTOR / BUILDING | All NOT_RUN |
| P1–P6 | NOT_STARTED; no continuation authorization |
| Owner go/hold disposition beyond P0 | PENDING |
| Project license | NOT_SELECTED |

Next: resume DYL-136 after the disposable world exists. Establish backup/restore and strict runtime guards, then bounded load/disable/restart. Do not consume downstream dependencies as if they passed. Two-client quests and multiplayer assertions remain untested without another authorized client or an explicitly accepted reduced verification scope.
