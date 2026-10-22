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

## Scheduler-context continuation: original indirect call and query frame

A further bounded static pass on the same exact executable resolved the earlier mutable-record indirection. This is a stronger source/file result, **not a runtime pass or a ready execution profile**. The scanner's new `--scheduler-relations` mode invokes `objdump` locally and checks 24 selected operand relations against this exact hash. It exports match booleans and RVAs, not instruction/code dumps. All 24 matched; [scheduler-relations.json](scheduler-relations.json) records them. Seven synthetic collector tests now pass, including refusing this relation map for another binary.

```sh
timeout 60 python3 tools/p0_profile_inspect.py \
  --executable /home/dq/.local/share/Steam/steamapps/common/Enshrouded/enshrouded.exe \
  --scheduler-relations \
  > evidence/p0/profile-investigation/scheduler-relations.json
```

### Registry to batch to original callback

The original scheduler block at `0x9a271e` selects a system record with stride `0x1f20`; `0x9a2733` obtains registry owner from `[manager]`, adds its system-array offset `0x745a28`, then the selected stride. This agrees arithmetically with the earlier registration address: registry owner offset `0x928` within the outer execution owner plus `0x745a28` equals registration's `0x746350`. This is a static address relation; actual runtime owner pointers remain unobserved here.

The scheduler constructs batches of size `0x38`. At `0x9a27b0` through `0x9a27d3`, a batch receives the manager's task-record array from `manager+0x528`, its count from `manager+0x530`, and the selected system-record pointer at batch offset `0x10`. The same batch is passed directly to `0x9a3400` at `0x9a2b5f` or through original worker callback `0x9e2c80`. That worker receives a job frame containing manager at offset zero, batch array at `+8`, batch index at `+0x10` and task index at `+0x14`, selects the batch by stride `0x38`, and calls `0x9a3400` at `0x9e2cd4`. It then notifies original completion using the system ID (`0x9e2ce4`/`0x9e2ce8`). The submission path includes per-job wait `0x8e5c00` and queue wait `0x8e5e20` before return (`0x9a2c73`/`0x9a2c84`). These decoded joins are evidence for that original path, not a general promise that an injected callback may retain stack pointers after returning.

`0x9a3400` selects one task record at base plus task-index times `0xe0`. It builds a stack-local query frame and iterator. Specifically, manager RCX is saved at `frame+0` (`0x9a34ae`, frame at stack `+0x40`); the iterator's first word is set to that frame pointer and its uint32 cursor at `+8` starts at zero (`0x9a39eb` through `0x9a39f4`). The engine then sets callback RCX to the iterator address, loads the selected system record from `batch+0x10`, loads its original callback from `record+0x28`, and **calls it indirectly at `0x9a3a06`**. The alternate dispatcher `0x9a2d30` has the corresponding original indirect call at `0x9a332a`.

This closes the original descriptor → registered callback slot → batch → callback-iterator relation. It also explains why the current actor callbacks' common helper reads `[incoming RCX]`: that word points to the query frame whose first word is the manager. An arbitrary RCX object or the old generic trampoline's R15 does not have this proven relationship.

### Thread and lifetime boundaries

The original engine has both inline and job-based system-dispatch paths. A remembered thread ID cannot stand in for a current execution phase. A hook must preserve the original register/stack contract, must not retain the iterator/frame beyond the original call, and must not invoke blocking transport, allocate unbounded state or run arbitrary queued gameplay operations merely because this callback fires.

The current upstream `ecsWriteLayout` names the exact `GameServer` thread callback `0xe3bad0` and original scene context at Scene `+0xa0`; `lock_scene_access.cpp` validates thread-registry membership, callback/context identity, local-authority mode and owner/Scene identity under original guards. Independently inspected code shows that thread callback reads its stored context at thread `+0x78` and calls the frame loop; the frame loop's shared lock at context `+0x538` equals Scene `+0x5d8`. The parent investigator separately follows the complete Scene → execution → scheduling path. This worker does **not** claim that all worker/exception/cancellation branches have been proven to retain that Scene gate through every callback, or that the actual current session exhibits the recorded owner chain.

A related later drain job at `0x9e6a30` uses another explicit original job frame: owner at `+0`, item-pointer array at `+8`, uint32 index at `+0x10`. Its caller `0xa1fb20` submits a stack-local descriptor through `0x8e2820` then waits via `0x8e5e20`. It resolves the manager at owner `+0xe15dc0` for the command-drain path. This is useful original lifetime/context evidence, but it is **not** the actor-system iterator ABI and must not be substituted for it.

### Narrow candidate scope and remaining gap

The static relation now supports designing a current-build **counter/context-only** observation hook at a named actor-system entry, using the incoming RCX iterator and its manager provenance. It does not yet establish that such a hook will load safely: the exact entry instruction boundaries/relocation and register-preserving stub still require review and fixtures, and the complete main scheduler join/Scene lifetime path must be reconciled with the parent investigation. An existing aligned-entry stub is a starting implementation pattern, not proof that unwinding, concurrency and callback cleanup work for this hook.

Before any no-op test, bind the adapter to the exact digest and independently validated source, verify offline reset, define the bounded job, keep all actor/inventory/construction dispatch disabled, and log only a bounded counter plus validated identity/context metadata. Actual callback cadence, multiple-worker behavior, session ownership, enable/disable and restart/cleanup must be observed. No game launch, process-memory read or hook execution occurred in this continuation. Execution-profile and all live gate statuses remain unchanged.

All temporary disassembly remained outside the repository under `/tmp/enshrouded-p0-source-review/`; no proprietary instruction bytes or disassembly files were added to evidence. No source checkout modifications or commits; all commands completed and no process remains.

## Scene-to-scheduler bridge and normal-path joins

The next bounded pass independently decoded the parent's Scene anchors and connected them to the actor dispatch already recorded. [scheduler-relations.json](scheduler-relations.json) now contains **46 selected matching static operand checks** on the same exact SHA. Decode ranges start at actual inspected function boundaries (`0x9a1310`, `0xa1fd40`, `0xe3bad0`, `0xe48b60`, `0xe4a200`, `0x8e5c00`); the prior broad range starting at `0x9a1300` was replaced. These checks are selected relations, not a complete control-flow, exception or lifetime proof.

The normal original path is:

1. Registered `GameServer` callback `0xe3bad0` loads its context from the original thread object's `+0x78` (`0xe3bae4`). It passes that context to `0xe4a200` (`0xe3bcdb`/`0xe3bcde`). The profile associates this context with Scene `+0xa0`.
2. `0xe4a200` acquires a shared gate at context `+0x538` (`0xe4a215`/`0xe4a22f`), corresponding to Scene `+0x5d8`. Its scene-entry loop passes entry context in R8 to `0xe48b60` (`0xe4a30d`/`0xe4a324`). The callee retains it in R14 at `0xe48b89`.
3. `0xe48b60` reads outer execution owner from scene-context `+0x1d0` and calls `0xa1fd40` at `0xe49752` or `0xe4980d`, depending on the original branch. `0xa1fd40` computes its embedded manager at outer-owner `+0xe15dc0` and calls `0xa1fe10` (`0xa1fd77`/`0xa1fd7e`). This agrees with the current profile's independently recorded manager offset.
4. `0xa1fe10` retains manager RCX, runs prepass `0x9a1310` at `0xa2000a`, tests manager mode byte `+0x518`, then calls `0x9a21e0` at `0xa2001b` or alternate `0x9a3ae0` at `0xa20022`. The first selected scheduler contains the registry/batch construction and direct/worker dispatch to `0x9a3400` already proven above; the alternate contains dispatch to `0x9a2d30`.
5. In the first scheduler's normal join path, it waits on queued job handles via `0x8e5c00` (`0x9a2c73`) and on the job queue via `0x8e5e20` (`0x9a2c84`) before returning. Inspecting the queue-wait body confirms it repeatedly examines pending count at queue `+0x1c0` and outstanding-list state at `+0x1a0`, releases/reacquires its own gate while working/waiting, and loops while work remains. The handle-wait entry tailcalls the original wait implementation `0x8e0630`. On normal completion, the Scene frame loop eventually releases its shared gate with the original locked decrement at `0xe4a5b6`.

This closes a concrete normal original Scene → manager → scheduler → original actor-callback path and identifies normal joins before Scene gate release. **Other entrants, all alternate branch joins, timeout/error propagation, asynchronous cancellation, SEH/unwind and teardown paths have not been exhaustively proven.** Runtime active-Scene/session matching and actual thread/cadence still remain NOT_RUN. Do not infer lifetime for a borrowed pointer retained by new mod code merely from the original engine's synchronous normal path.

The gate also creates a concrete action restriction. Upstream `EngineContext::try_exclusive` in `lock_scene_access.cpp` requires Scene lock value zero to acquire `EcsWriteGuard`. A callback entered under the same original Scene shared lock cannot acquire that exclusive guard and receives `ecs-scene-frame-busy`. It must never spin/retry while holding the callback's shared gate, bypass the guard, or dispatch general gameplay writes there. This observation path is not a general write dispatcher. The parent separately reviewed the actor entry's first complete stack-only instructions; a proper trampoline/unwind/preservation review remains necessary and is not replaced by this call-chain result.

### Collector consistency correction

The scheduler scanner previously hashed bytes read from the installed path and then let `objdump` reopen that path, leaving a change-between-reads window. It now reads once, verifies the pinned SHA of those bytes, writes a private temporary **read-only** snapshot outside the repository, and decodes that snapshot only. The snapshot and directory are removed on completion. The evidence states `decode_input=private_read_only_snapshot_of_hashed_bytes`. A new synthetic test verifies the decoder receives exactly the supplied bytes at a private mode-0400 path and that the snapshot is removed afterward. **Eight collector tests pass.** No live game bytes or executable snapshots are retained in the repository.

This follow-up changed only the existing scanner/tests and this evidence package, made no upstream edits, and ran no live operations. No processes remain. All live gates remain NOT_RUN, and an execution profile remains HOLD pending the precisely scoped no-op adapter and runtime evidence.

### Actor entry metadata (NOT_INSTALLABLE)

The same private-snapshot decoder inspected `actor_simulation_start` at RVA `0x2701e0`. Its first four complete instruction lengths are **2, 1, 8, 7** bytes. A five-byte detour would therefore require an **11-byte** whole-instruction overwrite. The first 11 bytes have SHA256 `a56ec9d01e457fd7fe4292c7ef3ac531df2a4e21640412124b4a437ed1760f40` and occur once in the executable sections. No instruction bytes are exported. This uniqueness is a candidate guard, not a validated hook.

The original entry has an unwind-table fragment ending at RVA `0x270220`; its unwind header reports version 1, flags 0, prologue size 18, four unwind-code slots and no frame-register designation. That original metadata covers original code addresses, not a copied trampoline. Copying stack-changing prologue instructions without correctly registered trampoline unwind metadata would leave exception/unwind behavior unsupported. The evidence explicitly labels this candidate **NOT_INSTALLABLE** until trampoline unwind, ABI/register preservation and relocation fixtures exist, followed by actual session/cadence/cleanup evidence. No detour or hook was installed.


### Caller stack and home-space check

A parent static pass inspected both original dispatch functions (`0x9a2d30` and `0x9a3400`) from their entries through the indirect calls. Each saves five eight-byte registers and allocates `0x890` bytes, with no further explicit RSP mutation before the selected callback call. Given ordinary Windows x64 entry alignment, this produces an aligned caller stack; the actor callback receives entry RSP modulo 16 = 8. The iterator lives at caller RSP `+0x30`, and its query frame starts at `+0x40`, beyond the 32-byte callee home area. The separate fixture's continuation scratch word at actor entry RSP `+8` therefore maps to caller RSP `+0`, not to the iterator or query frame on these inspected paths. The normal callee-owned home-slot contract supports this use; other hypothetical entrants and actual live ownership remain unobserved.

The collector now records **60** matching selected relations, adding the two stack prologues and alternate iterator argument. This supersedes the earlier 46-check count. The current [entry adapter fixture](../../probes/p0-entry-fixture/README.md) tests signed displacement `-200`, preserves RBP/RBX through stack-based unwind metadata with no frame-register designation, and uses the home-slot continuation transfer. Its negative control reproduces the unsafe RIP-indirect epilogue interpretation. This is fixture/static evidence, not installation approval.
