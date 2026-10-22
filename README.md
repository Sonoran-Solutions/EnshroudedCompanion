# Enshrouded Companion

An experimental AI companion for Enshrouded: someone who can adventure and fight alongside you, help with building, and roleplay using what is actually happening in your game. The intended backend can be local, such as Qwen or Gemma, or hosted through OpenRouter.

**Status: P0 HOLD.** Source investigation and offline client fingerprint collection are complete; all live tests remain NOT_RUN; RUNTIME is HOLD because the exact execution profile is absent. Required loader binaries now build and pass synthetic load/refusal checks; the current scheduler/query-frame contract remains unresolved. The owner approved bounded Phase 0 and native NPC embodiment. The owner authorized offline disposable shared worlds; reset and live runtime validation remain unverified; only one test client is available. See the [Phase 0 checkpoint](docs/p0/REPORT.md).

The main uncertainty is the **game bridge**, not the chat model. An LLM can suggest a plan and speak in character; dependable movement, combat, quests, and construction require verified access to Enshrouded's runtime. Loading a mod or reading an asset is not proof that those actions are available.

## Start here

| Document | Purpose |
| --- | --- |
| [Project brief](docs/PROJECT_BRIEF.md) | Intended experience, scope, and first playable target |
| [Feasibility and research](docs/FEASIBILITY_AND_RESEARCH.md) | Dated primary sources, integration options, and unresolved capabilities |
| [Implementation plan](docs/IMPLEMENTATION_PLAN.md) | Phases, task IDs, dependencies, acceptance gates, and parallel work |
| [Phase 0 handoff](docs/PHASE_0_HANDOFF.md) | Bounded discovery task to give an implementation agent after approval |
| [Architecture](docs/ARCHITECTURE.md) | State, model, action, authority, and persistence boundaries |
| [Models and voice](docs/MODELS_AND_VOICE.md) | Local/OpenRouter adapters, Jev, hardware, voice, cost, and privacy |
| [Companion behavior](docs/COMPANION_BEHAVIOR.md) | Adventuring, combat, contextual roleplay, and memory |
| [Building](docs/BUILDING.md) | Safe construction jobs, blueprints, resources, and recovery |
| [Validation and operations](docs/VALIDATION_AND_OPERATIONS.md) | Evidence, targeted testing, process budgets, and compatibility |
| [Decisions and risks](docs/DECISIONS_AND_RISKS.md) | Confirmed requirements versus proposed choices and open risks |
| [Status](docs/STATUS.md) | Current work state and next decision |

## Recommended first step

Validate the current scheduler/query-frame contract and a matching execution adapter. Loader binaries and guard tests now pass outside the game; post-update identity is unchanged. Verify the authorized offline reset only after a safe runtime candidate exists. Prove live state access and control of one companion actor before building the larger runtime. Evaluate native construction separately. If the required hooks are unavailable, stop with evidence and choose a reduced scope or another integration route explicitly.

The proposed first playable is **one companion that follows, stops, assists in a limited fight, and talks about verified events and the player's selected objective**. It is not an autonomous player that can already handle every dungeon, jump, puzzle, or building style.

## Working assumptions

- Prefer a native, host-authoritative companion if the game bridge permits it. A full second game client is an alternative, not a hidden requirement.
- Use deterministic controllers for immediate movement and combat; use models for dialogue and higher-level intent.
- Start with text and a narrow action set. Voice and building are staged features.
- Support local-only operation without silently sending prompts to a hosted provider.
- Treat every model action as an untrusted proposal. The executor checks current state and permissions.
- Never test unproven world-editing code against a valued save.

These are design recommendations, not recorded owner approval of an implementation stack or deployment topology. Creating these docs does not start Phase 0.

Research baseline: **2026-10-22**. The latest official hotfix found was **1.0.0.3, revision 1273043**. This is a research target, **not a tested compatibility claim**; see the linked source register.

This project is unofficial and is not affiliated with or endorsed by Keen Games. Enshrouded and its assets remain the property of their respective owners. A project license has not yet been selected.
