# Validation and operations

Status: proposed working rules. No runtime tests have been executed for this planning package. Implementation must report actual evidence, including failures and checks not run.

## Test the smallest relevant surface

| Change | Required checks | Normally unnecessary |
| --- | --- | --- |
| Documentation only | Relative links, examples, status/decision consistency and factual sourcing | Game launch, GPU inference, full runtime suite |
| Prompt/persona change | Fixed grounding/no-spoiler scenarios; schema/action boundary regressions if affected | Rebuilding unrelated native code |
| Provider adapter | Fake HTTP/stream/error tests, schema conformance; bounded authorized live smoke | Full-world gameplay soak |
| Shared state/action contract | All affected consumer contracts and safety-negative tests | Unrelated visual/voice quality sweep |
| Native bridge/controller | Build/version tests, affected deterministic tests, bounded in-game scenarios | Every unrelated subsystem by default |
| Inventory/build/persistence | Resource, authority, duplicate, interruption, save/reload and recovery tests | Broad unscoped random world mutation |
| Integration/release candidate | Combined relevant regression and agreed hardware/session gate | Repeating an unchanged full suite after every docs-only evidence commit |

A full suite is justified for shared low-level changes, integration or release gates. It is not a substitute for selecting the checks that can detect the actual regression. Record what was omitted and why.

## Safety-negative regression set

When the corresponding boundary changes, cover:

- Wrong schema/capability version, malformed/partial model output and unsupported action.
- Stale snapshot, mismatched world/session/save epoch, recycled entity ID and missing authority.
- Provider timeout/refusal/rate limit, saturated queue, cancellation and delayed response.
- Duplicate proposal and crash between native effect and acknowledgement.
- Owner disconnect, world switch, save rollback and memory/profile mismatch.
- Unauthorized target/region, insufficient resources and concurrent inventory/world edits.
- Prompt injection in chat/lore/item labels and model attempts to raise its own permissions.
- Unknown outcome refusing automatic replay.

Fixture/replay tests prove application behavior against supplied inputs. They do not prove the bridge reads correct state or that the game executes a command.

## Evidence package

For each live gate, record:

1. Repository commit, upstream loader commit, game/client/server revisions and binary hashes.
2. OS, compatibility layer, hardware, launch topology and relevant mod configuration.
3. Disposable-world identity, initial conditions and backup/restore method.
4. Exact scenario, commands/actions, expected and actual results, timestamps and repetition count.
5. Snapshot/event/action/result IDs sufficient to reconstruct causality.
6. Logs and concise video/screenshots when they add evidence; sanitize before publishing.
7. PASS, FAIL, BLOCKED or NOT_RUN per assertion, with unresolved findings.
8. Owned processes started/stopped and any state intentionally left running.
9. Limitations: fixture-only, single-client-only, prop-only, one enemy type, etc.

Bind evidence to the tested code SHA. A later documentation-only commit may reference that tested SHA, but must not imply the runtime was rerun at a different head. Record source revisions and avoid relying on mutable "latest" alone.

Do not commit private saves, account identifiers, API keys, access tokens, raw personal voice/chat, large proprietary dumps or game binaries. Retain local evidence only as needed; publish redacted samples and reproducible instructions.

## Bounded jobs and process ownership

Before a long job starts, declare its purpose, expected duration, hard timeout, resource budget, output location, exact command and owner. Record PID/process group or equivalent identity and the worktree/commit. Inspect existing relevant jobs first.

Suggested defaults to ratify at kickoff:

- Documentation/fixture checks: target under five minutes; investigate if unexpectedly longer.
- Targeted native build or model smoke: ten-minute initial cap unless measured history justifies more.
- Live game scenario: plan a 10–15 minute session; stop when the assertions are collected.
- A longer soak: separate explicit task with success criteria, telemetry and a fixed end time.

These are starting budgets, not claims about available test runtimes. Do not launch another copy because the first appears quiet. Check progress/resource use/log timestamps before deciding whether it is stalled.

On timeout or failure, preserve useful logs and stop only the owned process tree. Never broadly kill all game, Python, model-server or build processes. Do not stop shared/user services without permission. Report leftover processes instead of hiding them.

Do not rerun an unchanged failed test indefinitely. Diagnose first; rerun only after a relevant change or a specific flaky-environment hypothesis. Stop at the approved attempt/time budget and report the blocker. No autonomous multi-day loop just because P0 has a multi-day planning allowance.

## Game-world safety

One agent/operator owns live mutations in each test world. Other workers use recordings or separate isolated fixtures. Keep production worlds out of discovery work.

Take a consistent backup and test restoration before risky mutations. Never restore a shared world while other writers are connected. Record the expected loss of intervening progress and get owner approval for a material restore.

A version mismatch disables unsafe features. Do not bypass failed symbol checks, guess offsets, or label an old-version smoke as current compatibility. Runtime crashes, orphan actors, unexpected resource changes or uncertain ownership are stop conditions.

## Performance and deployment

Measure the game and inference together on the intended topology: same GPU, separate inference machine, or real second-client mode if chosen. Capture frame-time/latency distributions, VRAM/RAM, queue depth and cost, not just standalone tokens per second.

Keep controller execution independent of HTTP latency. Verify UI responsiveness, game-thread overhead, bounded payload sizes and graceful backend restart. Do not publish numeric performance promises before measurement.

Default to local transport. Remote access, telemetry upload and hosted inference require explicit configuration/consent. Protect any LAN control path, redact secrets, and cap requests/spend.

## Release hygiene

Before distributing: choose a license, audit dependencies and model/voice licenses, review applicable terms, document supported binaries, install/remove steps, recovery, privacy and limitations. Do not claim official support.

A release candidate needs owner playtest acceptance. Technical tests and attractive dialogue demos do not replace hands-on judgment of whether the companion is helpful, predictable and enjoyable.
