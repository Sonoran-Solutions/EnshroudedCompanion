# Project brief

Status: proposed product scope. Updated 2026-10-22. See [current status](STATUS.md) and [decision register](DECISIONS_AND_RISKS.md) before starting work.

## The experience

The goal is a companion that feels like another adventurer, not a chatbot that happens to be open beside the game. It should notice relevant events, remember shared experiences, understand the objective the player is pursuing, and contribute through actual game actions.

The owner's request establishes four feature goals:

1. Fight alongside the player.
2. Adventure with the player.
3. Build things together.
4. Talk in character with awareness of game events and quests.

Local models, including Qwen or Gemma, and an optional hosted backend such as OpenRouter are desired. Jev is a candidate decision technology to investigate, not an assumed chat replacement. None of these requirements establishes that Enshrouded exposes the necessary hooks.

## What success could look like

- The player says, "Stay close; we're heading back to that ruin." The companion adopts follow mode and refers only to a ruin the pair has actually discovered or the player has identified.
- After a supported combat encounter, it can discuss the fight using confirmed events. It does not claim a kill because a planner merely requested an attack.
- It distinguishes a world quest from a personal quest, and the player's selected objective from both. If the selected objective cannot be read, it asks what the player wants to pursue.
- At base, the player approves a small work area and a blueprint. The companion reports missing resources, waits for approval, and builds through a verified game pathway.
- If the model is offline, the companion's immediate safety and stop controls continue to work. It does not freeze the game thread or spend money on an unapproved fallback.

These are acceptance scenarios to implement, not reports of working features.

## Scope ladder

| Slice | Included | Deliberately excluded |
| --- | --- | --- |
| Feasibility | Read-only game observations, one controlled actor probe, one isolated placement probe | Production saves, open-ended gameplay |
| First embodied slice | One actor, owner binding, follow/stop/limited assist, explicit command UI | Full-world navigation, autonomous looting, construction |
| First contextual playable | Grounded dialogue, selected objective, scoped memory, local and hosted text options | Perfect lore recall, all quest types, mandatory voice |
| Adventuring alpha | A tested traversal subset, bounded gathering and inventory policies, recovery | Completing arbitrary dungeons unaided |
| Builder alpha | Approved sites, deterministic small blueprints, resources and recovery | Unsupervised demolition, arbitrary generated castles |
| Later exploration | Richer personalities, more traversal, better layouts, optional learned decision policy | Any promise that these are feasible before bridge proof |

## Feature feasibility

| Feature | Assessment | Main dependency |
| --- | --- | --- |
| In-character text conversation | High feasibility in isolation | Model quality, prompts, scoped memory |
| Local or OpenRouter backend switching | High feasibility in isolation | Explicit provider capability contracts and configuration |
| Quest-aware roleplay | Conditional | Reliable state with personal/world/selected-objective scope |
| Follow, stop, guard, basic assist | Conditional, substantial integration work | Actor ownership, navigation, combat and replication hooks |
| Robust open-world adventuring | High effort even with hooks | Traversal, hazards, recovery and game-specific controllers |
| Template-based construction | Conditional, high risk | Native placement, permissions, inventory, persistence and recovery |
| Unrestricted autonomous building | Research-heavy, not an initial commitment | Spatial planning plus all builder safeguards |
| Screen-only general gameplay | Poor primary route | Perception ambiguity, latency and action reliability |

These are engineering judgments, not benchmark probabilities.

## First playable acceptance target

A private, disposable test world; one owner; one companion; a short known route; one supported enemy encounter; text interaction. The player can summon or attach the companion, follow, stop, request assist, and disable it. The companion can describe an observed event and the explicitly selected objective without inventing completion. Reload and owner disconnect are safe.

The term "first playable" means a limited vertical slice, not a public-ready release. Cooperative visibility must be tested if multiplayer is claimed. A talking observer without an embodied actor is a useful fallback but does not satisfy the original companion goal.

## Non-goals for the initial project

- A general computer-use model pressing keys every frame.
- A trained-from-scratch foundation model.
- An unrestricted agent with shell access, arbitrary game-memory writes, or free-form code execution.
- Public-server deployment without the host's authorization.
- Headless protocol emulation or a guaranteed account-free player bot.
- Complete quest solving, omniscient map knowledge, or spoilers by default.
- Save editing as a shortcut to safe live construction.
- Multiple companions, complex relationship systems, or a large configuration UI before the core loop works.

## Planning realism

A dialogue observer is materially easier than a competent embodied companion; a reliable builder adds another substantial subsystem. The complete vision is a multi-month engineering effort if the bridge proves viable, and may remain blocked regardless of available model quality.

The [implementation plan](IMPLEMENTATION_PLAN.md) uses a bounded investigation first. Phase 0 is a proposed 5–10 focused developer-day budget, not a guarantee that reverse engineering will succeed. Later estimates should be produced from measured evidence, not extrapolated from how quickly a chat demo can be built.
