# Feasibility and research

Research checked: **2026-10-22**. Scope: public primary documentation, project repositories, and official game announcements. The initial documentation package examined no installed binaries or game hooks. The subsequent [P0 source review](p0/SOURCE_REVIEW.md) inspected current source and local client identity; no game hooks were tested.

## Bottom line

The model layer is feasible using existing local inference servers or hosted APIs. The hard part is giving that layer a dependable body and truthful game state. Enshrouded's proprietary Holistic engine and evolving community tooling make this a game-integration research project, not just an LLM plugin.

The current evidence does **not** establish a supported end-to-end API for actor spawning/control, navigation, quests, inventory and voxel construction. That is a statement about the evidence found, not proof that such integration is impossible.

The official hotfix baseline found is **1.0.0.3, revision 1273043, dated October 21, 2026** ([S1](https://enshrouded.com/en-US/news/changelog-for-hotfix-1003)). Before experiments, capture the actual client/server build and binary fingerprint. A version label or loader's general "1.0 support" is not sufficient proof for the exact installed build.

## What existing tooling establishes

| Tool | Evidence as checked | What it does not establish |
| --- | --- | --- |
| Enshrouded Mod Loader / kfc-parser | Latest listed release v0.2.0, October 16, adds 1.0 compatibility. Published docs still describe pre-launch asset changes and planned runtime work. A March workflow also records runtime DLL-loading work. | A stable, complete actor/navigation/quest/build API. Docs may lag code; inspect the selected revision. |
| Shroudtopia | Experimental native loader with lifecycle/update entry points and configuration/logging. Its README explicitly says the mod context lacks game-specific functions. | That loading C++ code supplies the required game actions. |
| Ember | In-development dedicated-server SDK. Current main documents lifecycle, chained hooks, reflection access, symbol resolution and save-cycle integration. README says no GitHub release yet carries the loader library. | A tested companion implementation, complete coverage of declared modules, or verified support for revision 1273043. |
| Vanilla summons / NPCs | Official sources describe combat summons and separately base-roaming NPCs/villagers. | Persistent player-like inventory, quest knowledge, free-world traversal or building control. |
| Sparkworks | Officially presented in-world building/adventure logic. | A general external AI or native actor-control interface. |

Tool sources: [EML release S2](https://github.com/Brabb3l/kfc-parser/releases/tag/v0.2.0), [EML docs S3](https://brabb3l.github.io/kfc-parser/), [DLL-loading workflow S4](https://github.com/Brabb3l/kfc-parser/actions/runs/23314673947), [Shroudtopia S5](https://github.com/s0t7x/shroudtopia), [Ember S6](https://github.com/zachthedev/enshrouded-ember), [Ember usage S7](https://github.com/zachthedev/enshrouded-ember/blob/main/docs/usage.md), [Sparkworks S8](https://enshrouded.com/en-US/news/turn-your-build-into-an-adventure-with-sparkworks).

Ember's published 0.1.0 crate documentation and current main are different evidence snapshots. A named module or declared surface is not automatically an implemented, tested feature. Inspect source and symbols for the exact candidate revision; do not dismiss current main as only a scaffold, or infer runtime completeness from its module list ([S9](https://docs.rs/ember-enshrouded/latest/ember_enshrouded/), [S10](https://docs.rs/crate/ember-enshrouded/latest/source/CHANGELOG.md)).

## Three embodiment routes

| Route | Advantage | Cost and uncertainty | Recommendation |
| --- | --- | --- | --- |
| Native host/server-controlled actor | Potential reuse of game navigation, combat and replication; no second rendered player client by design | Requires verified spawn/attach, ownership, action and persistence access; an NPC may not behave like a player | Investigate first; do not select a framework until P0 evidence |
| A real second player client driven by automation | Uses an actual player avatar and the game's normal player systems | Extra rendering/compute, authorized account/session requirements, reliable controls and observation still needed; no assumed headless mode | Explicit alternative if native actor route fails or proves unsuitable |
| Screen/vision-only controller | Can observe some things without deep hooks | Occlusion, UI ambiguity, slow perception-action loops, brittle menus and precision building | Optional observer/research fallback, not the primary real-time controller |

A synthetic network client would add protocol, authentication and compatibility work. It is not a shortcut in this plan. Do not assume a single purchase/session can host two players; investigate legitimate requirements only if that route is chosen.

## Game-specific distinctions that matter

**Summons are not full companions.** The 1.0 combat preview names elemental totems and Hollow Skeleton summons ([S11](https://enshrouded.com/en-US/news/10-combat-changes-and-additions)). These are useful places to inspect existing behavior, not evidence of a persistent follower API.

**Base NPCs are a different system.** Keen's November 2024 update describes crafting NPCs and villagers roaming and sleeping at base, with placement/range controls through the summoning scepter ([S12](https://enshrouded.com/en-US/news/enshrouded-souls-of-the-frozen-frontier-update)). Reusing such an actor for combat or construction would still need proof.

**Quest scope cannot be guessed.** The 1.0 changelog says the UI distinguishes personal and world quests ([S13](https://enshrouded.com/en-US/news/release-trailer-changelog-everything-you-need-to-wait-for-tomorrow)). This does not prove a mod can read quest IDs, progress or selection. Model context must distinguish shared world progress, individual progress, and the objective the owner wants to pursue. Unknown selection can be supplied by the player and labeled as such.

**Asset access is not runtime authority.** Reading item definitions or quest text can enrich a grounded description, but does not reveal what this player has actually done. Reading a container-placement event does not establish a safe general voxel-placement function.

## Model ecosystem evidence

- Local inference servers expose useful HTTP/chat interfaces. llama.cpp documents streaming, structured JSON and template-dependent tools. API resemblance is not full behavioral compatibility ([S14](https://github.com/ggml-org/llama.cpp/blob/master/tools/server/README.md)).
- OpenRouter documents chat completions and per-model/provider features. Tool and structured-output support must be checked for the selected route ([S15](https://openrouter.ai/docs/api/api-reference/chat/create-a-chat-completion), [S16](https://openrouter.ai/docs/api/api-reference/models/list-all-models-and-their-properties)).
- Jev/System One is described as typed decision generation, not free-form dialogue. An LM Studio compatible endpoint does not establish that official Jev weights are locally available or suitable for this game ([S17](https://typesafe.ai/blog/introducing-system-one-models-and-jev), [S18](https://lmstudio.ai/docs/developer/jev-compat/systemone)).
- Mantella demonstrates useful dialogue/context/voice patterns in supported games; Mindcraft demonstrates LLM planning above game actions in Minecraft. Neither provides an Enshrouded bridge ([S19](https://github.com/art-from-the-machine/Mantella), [S20](https://github.com/mindcraft-bots/mindcraft)).

See [models and voice](MODELS_AND_VOICE.md) for the implementation proposal. Do not port another game's prompts and assume its action layer transfers.

## Evidence labels

Every investigation entry should use one of these:

- **DOCUMENTED:** an upstream primary source describes it; include URL, source revision/date and relevant code path.
- **INSPECTED:** implementation was examined at an exact commit; name the function/type and its limits.
- **RUNTIME_VERIFIED:** exercised on recorded binaries and environment with reproducible evidence.
- **PROPOSED:** this project's design, not an upstream feature.
- **NOT_VERIFIED:** unresolved; never silently upgrade to available.
- **UNAVAILABLE_FOR_TESTED_BUILD:** a bounded investigation failed on a specific build; not a universal impossibility claim.

Current project status for all required live hooks is **NOT_VERIFIED**.

## Sources and refresh policy

S1–S20 above form the source register for the initial assessment. They are direct official/project sources; no secondary article is used as proof of technical capability. Release notes and tagged code are preferable to mutable main or latest pages. Record exact upstream commits when P0 chooses candidates. Links are not vendored dependency pins.

Refresh the source register when the game, loader, API, or model route changes. Do not copy a compatibility claim from an old research date into a release. Record what was actually tested in the [validation evidence](VALIDATION_AND_OPERATIONS.md), even if upstream documentation says it should work.

No legal permission, license compatibility, anti-cheat status or public-server acceptability is inferred from community tooling existing. Review applicable terms and component licenses before distribution; never bypass access restrictions.

## P0 source refresh — 2026-10-22

[Exact pinned implementation review](p0/SOURCE_REVIEW.md) supersedes the initial high-level loader assessment for this checkpoint. EML's inspected runtime DLL loop is commented; Ember's inspected loader/signature/entity surfaces are prose, not implemented functions. Shroudtopia supplies a generic loader. ShroudForge has implemented ECS and world operation code plus a profile matching the installed client; it is the selected investigation route. Its startup fallback and native-action semantics require further proof. No scoped quest or companion-control route has been established.

The [report](p0/REPORT.md) records all capabilities NOT_RUN, including voxel and prop construction independently. Matching a binary profile is not passing runtime validation.

The follow-up [runtime preflight](p0/RUNTIME_PREFLIGHT.md) establishes source-level RUNTIME HOLD for the stock selected loader's startup ordering/refusal behavior. No runtime result is inferred. A Steam update makes the earlier local identity historical until re-collected.

Guard work clarified the profile distinction: `client/26082781.json` is `read-validated` with no `hooks`/`ecsLayout`. The only client execution profile in the pinned source is older `1076226.json`. A matching executable hash therefore establishes read-profile identity, not executable action support. The [strict patch](../patches/shroudforge/README.md) must refuse it. Fresh post-update identity is unchanged; see [guard evidence](../evidence/p0/guards/VALIDATION.md).

Current follow-up: selected-path native and Rust binaries cross-build and pass isolated synthetic load/refusal checks. All six historical full hook signatures have zero current-build matches. Current descriptor registration is traced to a mutable record, but scheduler invocation/query ABI and owner lifetime remain unresolved. See [hook evidence](../evidence/p0/profile-investigation/README.md) and [build evidence](../evidence/p0/build-investigation/README.md). No live capability is established.
