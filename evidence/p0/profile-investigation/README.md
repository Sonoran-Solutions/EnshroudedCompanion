# Current-build execution-hook investigation

Date: 2026-10-22. Scope: static source and installed-file inspection, bounded to the initial 20-minute investigation. **Execution profile: HOLD. Live runtime/observation/quest/actor/building checks: NOT_RUN. No validated current-build execution profile was produced.** This describes evidence missing from the inspected source and file, not universal impossibility.

## Identity and reproducible result

A fresh read of the installed executable yielded SHA256 `d64966e23b1a458ee6f147bcbceac0b577fa7e39a212f371287d8cc119817882`, file length 35,338,312, PE timestamp 1792507660 and SizeOfImage 44,163,072. The parent associates this with Steam build 26082781. The collector checks inode/size/mtime before and after reading and refuses a changed file. No executable or code-byte dump is included in this evidence.

Source baseline is ShroudForge `3ed90de1df266c4f0c217523a02a71dfa67243de`. The working checkout includes the independently prepared strict guards; this worker did not alter that checkout. The historical profile source itself was read unchanged, and its checksum appears in [static-scan.json](static-scan.json).

Commands executed for the reproducible collection and tests (each under a 30-second command timeout):

```sh
timeout 30 python3 tools/p0_profile_inspect.py \
  --executable /home/dq/.local/share/Steam/steamapps/common/Enshrouded/enshrouded.exe \
  --historical-profile /tmp/enshrouded-p0-source-review/shroudforge/modules/game-runtime/profiles/enshrouded/client/1076226.json \
  > evidence/p0/profile-investigation/static-scan.json
timeout 30 python3 tools/p0_profile_test.py \
  2> evidence/p0/profile-investigation/scanner-tests.txt
```

An initial invocation used an incorrect duplicated directory in the executable path and refused with `FileNotFoundError`; the corrected invocation above completed. The scanner emitted only metadata and nonauthority flags. Six synthetic-PE tests passed: overlapping/wildcard signature matching, nonexecutable exclusion, unwind-bound descriptor recognition, invalid descriptor pointers, malformed/truncated image refusal, and RIP/direct-call candidate recovery. These tests verify the collector, not game compatibility.

## Historical hooks cannot be transferred

The scan searches raw file-backed executable sections `.text` and `.bind`, using the same section qualification as upstream `dev/profiles/cli/match_function_signatures.cpp`. It does not search arbitrary data for purported code.

| Historical hook | Full historical signature matches | Historical overwrite snippet matches |
| --- | ---: | ---: |
| game_thread | 0 | 0 |
| entity_manager | 0 | 18 |
| world_prop_update | 0 | 56 |
| world_actor_placement | 0 | 0 |
| world_building_dispatch | 0 | 4491 |
| world_cursor | 0 | 0 |

The short overwrite snippets are diagnostics only. Their ambiguous occurrences cannot be used as replacement signatures or evidence of equivalent semantics. In particular, the required game-thread and entity-manager signatures both fail the real match criterion. Copying the historical profile and replacing its hash/version would fabricate compatibility.

The exact current profile `modules/game-runtime/profiles/enshrouded/client/26082781.json` has `provenance.status=read-validated`, `ecsReadLayout` and exact owner/service/resource layouts. It lacks execution `hooks`, `ecsLayout`, `entityDefinitionLayout` and `lookupManager`. Only `client/1076226.json` supplies a client execution profile in the pinned production profile directory, for another hash and KFC3. CMake separately places read-validated profiles into `resource_read_profile_ids`; they are not execution profiles. Strict initialization correctly refuses to promote the current read profile into execution authority.

## Current, independently derived static leads

The collector mirrors the original descriptor model in pinned `modules/game-runtime/native/code-inventory/inspect_game_functions.h:126`: 15 qwords represent a name pointer/length, callback pointer and **six** dependency pointer/count pairs (indices 3,5,7,9,11,13). It requires a callback at an actual unwind-entry boundary, a lowercase original system name, valid bounded dependency spans, and at least one populated span. It recovered **722** named engine descriptors from this file.

The following are current metadata candidates, not aliases invented for a companion API:

| Original descriptor | Current callback RVA | Current descriptor RVA | Relevant dependency evidence |
| --- | --- | --- | --- |
| actor_simulation_start | `0x2701e0` | `0x1dc5b70` | Actor/ActorInput write slots; optional Health, Mana, Stamina and other read slots |
| actor_simulation_end | `0x270050` | `0x1dc58e0` | Actor/ActorInput write slots |
| actor_locomotion | `0x267870` | `0x1dc5020` | ActorInput/CurrentTransform reads; Actor/DynamicLocomotion/Velocity writes |
| player_control | `0x2d6c70` | `0x1df9970` | PlayerInput and movement reads; ActorInput/player state/consumed input writes |

The sanitized JSON includes the exact slot names for these and two other inspection leads. The current profile's `systemDependencyLayout` independently assigns descriptor offsets 24/40 to template configuration reads, 56/72 to runtime reads, and 88/104 to runtime writes. These masks constrain scheduling compatibility; they do not prove that an arbitrary extra operation is safe in the callback's phase. A function called `actor_spawn_entity` is not thereby an authorized general actor-spawn API.

## Why a profile alone is insufficient

Targeted local `objdump -d --no-show-raw-insn -M intel` inspection covered current callback entries at image addresses `0x1402701e0` and `0x140270050`, and common helper entries `0x1409bc1a0` and `0x1409b6e60` (bounded ranges of at most 0x1b0 bytes per command). No disassembly was saved or committed. The callbacks retain their incoming RCX and pass that object plus local iteration storage to the helpers. The helpers read query/iteration state; this does not identify a reusable old lookup frame or an unrestricted game-thread callback.

The pinned generic trampoline at [`install_engine_hooks.cpp:45`](https://github.com/bonsaibauer/shroudforge/blob/3ed90de1df266c4f0c217523a02a71dfa67243de/modules/game-runtime/native/engine-hooks/install_engine_hooks.cpp#L45) saves eight stack words and subtracts 0x80, then explicitly supplies R15 and RBP as its callback arguments (line 55). That preserves the intended historical mid-function alignment. **Moving it unchanged to a standard Windows-x64 function entry would preserve entry RSP modulo 16 = 8 before its own call, violating the required aligned caller stack.** A suitable entry trampoline needs an independently reviewed ABI, stack adjustment and complete original instruction relocation. There is no basis for reusing the old mid-function register assumptions at a new descriptor entry.

Upstream also has a distinct `context_callback_code` at `install_engine_hooks.cpp:98`, using 0x88 stack adjustment and the original RCX for building-dispatch entry hooks. Thus an aligned-entry preservation pattern already exists. It is **not** the generic game-thread/entity-manager contract, and reusing it would still require current callback ABI, instruction relocation and scheduler/owner proof.

The entity-manager capture at [`observe_entity_manager.cpp:21`](https://github.com/bonsaibauer/shroudforge/blob/3ed90de1df266c4f0c217523a02a71dfa67243de/modules/game-runtime/native/engine-context/observe_entity_manager.cpp#L21) dereferences the passed lookup context, then reads root plus the execution profile's `lookup_manager`. The exact current owner chain establishes a different read path; it does not prove that incoming RCX, R15 or RBP at a newly chosen callback meets this old capture contract. A null/noncrashing pointer or matching count/table offsets would be insufficient proof.

Upstream `GameThreadDispatcher::drain` serializes reentrant callbacks with an atomic flag and dispatches a bounded queue, while `Ready` requires recent callbacks. Those implementation constraints do not prove engine lifetime, scheduler exclusion, local-world selection or safe writes when invoked from an arbitrary current callback. `FunctionInventory` deliberately marks native entries `callable=false` with unresolved ABI/context/effects; this report preserves that boundary.

## Smallest technical continuation

Do not create or approve a production execution profile yet. The next focused work must establish one exact current callback path and a matching adapter, in this order:

1. Recover the callback's original scheduler caller/dispatch registration, its incoming frame/query ABI, and the exact instruction boundary/ranges including chained unwind fragments. The named descriptor and current read-validated dependency model provide anchors. A first prologue fragment is not the complete function.
2. Select a callback phase that recurs in the intended local-authority world. Prove which owner/session it belongs to, required lifetimes/locks and that the proposed operation does not conflict with the phase's declared dependencies. Generic entity-manager reads must use a current validated ownership contract, not historical `lookupManager` guesses.
3. Prepare a current ABI-specific, preservation/relocation-tested hook stub and exact signature/original-instruction guards derived from this binary. A unique signature is necessary but not sufficient. If full current execution-profile schema requires unrelated unproven world-edit hooks/layouts, keep those capabilities absent and explicitly review the narrow adapter/schema change rather than filling them with old data.
4. Only after static/fixture review, a defined bounded test job, and verified offline reset, observe a **no-op counter/context-only** callback in the disposable session, prove sustained cadence/thread/owner identity and cleanup/restart. No actor, inventory or construction call belongs in that first hook probe. Static inspection cannot establish actual thread cadence or post-hook behavior.

A lifecycle/read-only mode using the existing exact read profile remains a separate explicitly scoped probe, with its own capability manifest and gate result. Removing the strict required-hook requirement to run that mode would not make this execution investigation pass. If the above static caller/ABI work yields no defensible hook within the next agreed budget, report that particular blocker and pause rather than widening into an unbounded engine reverse-engineering effort.

## Process accounting

This worker ran local source reads, PE metadata/signature/descriptor scans, bounded `objdump` inspections, synthetic tests and whitespace checks. No source checkout edits, native builds, executable downloads, installations, live process reads, game launches, injection or save/world writes. All invoked processes completed; none left running. Only the two `tools/p0_profile*` files and this `evidence/p0/profile-investigation/` package were created. No commit.

## Bounded follow-up: concrete descriptor registration relation

A further static pass found a reproducible registration chain for the exact same hash. The scanner now emits byte-level RIP-reference/direct-call **candidates**, explicitly labelled as requiring instruction decode. Targeted local `objdump` review verified these particular candidate instructions, without saving game bytes:

- `actor_simulation_start` descriptor RVA `0x1dc5b70` is returned by leaf getter `0x12ce9d0`. The getter has one discovered direct-call candidate, `0x4a7ba8`, verified as a real call. Immediately afterward the caller supplies returned RAX as RDX, owner RBX as RCX and zero R8D, and calls `0x9d6f60` at `0x4a7bb6`.
- `actor_simulation_end` descriptor `0x1dc58e0` has the analogous getter `0x12ce9c0`, called at `0x4a7d70`.
- The inspected registration helper `0x9d6f60` compares descriptor names against existing records, allocates/selects a system-record slot with stride `0x1f20`, stores the source descriptor pointer at record offset zero, and copies the descriptor callback pointer from `+0x10` to record `+0x28` (observed at `0x9d7164`/`0x9d7168`). This establishes a concrete static transfer from the named immutable descriptor into a runtime-built registry, not a direct scheduler call to the actor callback.
- The read-profile `systemDependencyLayout.recordBuilderRva` is `0x9f54a0`. A direct caller at `0x9f522b` passes an output record pointer, owner R13 and a source pointer obtained from the surrounding registry record. Inspection of the builder confirms dependency-span use consistent with the existing read-profile model. This does not yet establish that the actor callback receives this exact owner/frame instance, or resolve its eventual indirect dispatch.

The unresolved boundary is now precise: the runtime-created system-record callback pointer must be followed through scheduler plan/job construction to the actual indirect invocation and its RCX query frame. The immutable actor callback has only its descriptor as a discovered preferred-image data pointer; registration moves it into mutable records, so searching for more fixed callback addresses does not recover the dynamic call path. The follow-up did not prove which active world owner, scheduler worker, phase, lock lifetime or frame instance reaches a callback. It found no safe deterministic execution hook.

Next bounded static step: trace uses of the registration record's callback slot into the scheduler's plan/task record and inspect the full indirect call-site preparation, including chained function ranges. If this remains unresolved, the minimal read-only runtime observation should capture **the existing original** record/descriptor relationship and caller/query-frame provenance under validated owner guards, without calling or replacing the callback. Such observation is already within the owner's P0 authorization once static review, a bounded job and verified offline reset prerequisites are met. No renewed permission requirement is inferred.

No trampoline was implemented and no profile was promoted. The all-zero historical full-signature results remain unchanged. All follow-up commands completed within 30 seconds, no game/native processes started, and no process remains.
