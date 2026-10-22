# Companion behavior

Status: proposed behavior contract. Every action depends on the proven capability manifest; unavailable behavior must be disabled, not simulated through confident dialogue.

## Priority and control

Recommended priority: owner emergency stop/disable -> immediate tested safety behavior -> explicit owner command -> active approved task -> planner suggestion -> idle behavior.

Use a deterministic state machine or behavior tree for motor/tactical execution. Candidate modes:

| Mode | Entry | Behavior | Exit |
| --- | --- | --- | --- |
| Idle | No task or safe suspension | Do not start new interactions | Owner command or approved intent |
| Follow | Owner requests company | Maintain distance and use verified navigation | Stop, danger, unreachable route or owner loss |
| Guard | Owner selects a bounded position | Stay within radius; use approved defense policy | Recall, stop or unsafe state |
| Assist | Owner engages/marks a valid hostile target | Apply a supported attack routine within leash limits | Target invalid, owner retreats, low health or stop |
| Retreat | Deterministic danger threshold or owner command | Use the verified retreat/safe-hold behavior | Safe condition or owner override |
| Work | Approved gathering/building job | Execute bounded verified steps | Complete, blocked, cancelled or uncertain outcome |
| Suspended | Lost authority, invalid state, unsupported build | No new actions; explicit status | Fresh verified state and authorization |

Emergency behavior must itself be tested. "Retreat" is not a magic safe option if no valid route exists; a bounded hold/disable can be safer than guessing.

## Combat

The first slice supports one controlled encounter and a narrow weapon/ability routine. Use game AI or a deterministic controller where available. The model may choose "assist that target" or "fall back"; it must not decide every dodge, attack frame or cooldown.

Validate faction, owner permissions, range/leash, target lifetime and line-of-action constraints. Do not attack neutral NPCs, unapproved players, buildings or targets outside the allowed engagement policy. Friendly-fire behavior and aggro rules require game-specific tests.

Health, stamina/mana, Shroud exposure and other hazards are only actionable if readable with sufficient reliability. Unknown critical hazard state should narrow or stop the behavior. Do not invent an immortal companion or grant free resources as an implementation shortcut.

Companion death, owner death and resurrection policy are owner decisions. Start by disabling automation and reporting state rather than automatically spending consumables or respawning with duplicated equipment.

## Adventuring and traversal

A short known route is the first navigation acceptance test. Add one traversal type at a time: obstacles, slopes, doors, elevation changes, streamed-area boundaries and later any proven jump/glide/grapple/swim interactions.

Detect lack of progress with bounded retries. Expose "I cannot reach you" rather than repeatedly issuing the same move. Teleport catch-up is an optional gameplay concession requiring owner approval and a tested safe placement rule; it does not count as navigation proof.

On owner disconnect, stop following stale coordinates and suspend new actions. On reconnect, rebind verified identity. On world/load changes, cancel old goals and refresh observations. A late-joining client must not see a duplicate actor.

Gathering and inventory transfers are later features. Define allowed item categories, quantity limits, ownership, recipient capacity, reserved materials and whether pickup consumes shared loot. Never discard valuables, move equipment or loot another player's container without explicit policy.

## Quest awareness and truthfulness

Maintain separate records for:

- Shared world quest progress.
- Personal quest progress and the affected player.
- The objective selected by the owner.
- Discovered lore/map facts.
- Dialogue or memory that is a character's belief, not authoritative state.

The player can select an objective explicitly if a journal-selection hook is unavailable. Mark it OWNER_PROVIDED, not engine-observed. Screenshots/OCR can be an explicitly approved lower-confidence observer fallback, not a substitute for passing the live state gate.

Only verified events update completion state. An accepted attack/build command is not completion; an action result must report the observed outcome. Dialogue should distinguish "I will try," "I cannot reach that," and "I finished it."

Filter knowledge to discovered/approved content by default. Retrieval over the entire wiki or game assets can leak future locations, rewards and plot. A no-spoiler mode is a context-filter policy, not just a sentence in the character prompt.

## Personality and memory

Define a small character card: name, background, voice/style, motives, boundaries and relationship tone. Keep game facts outside the persona prompt.

Use short working context plus selectively retrieved long-term memories. Store confirmed shared events with provenance, timestamp/epoch and source. Keep inferred opinions separate from facts. Summaries must not overwrite authoritative quest records.

Scope memories by verified world/profile and character. Support inspection, correction, export and deletion. Do not mix saves, hosts or other players' private conversations. On a save rollback, reconcile or mark subsequent memories as out-of-timeline instead of treating them as current events.

Banter should be event-driven, interruptible and rate-limited. Prioritize a brief warning during danger over a long monologue. Provide quiet mode and user control over memory persistence.

## Minimum behavior tests

- Stop interrupts follow/assist/work before another prohibited side effect.
- Loss of owner, provider or bridge produces the documented fallback.
- Unknown or recycled entity IDs are rejected.
- Two players with different personal progress receive correctly scoped context.
- No model response can grant itself permissions.
- A failed or uncertain action is not narrated as success.
- Reload/rollback cannot revive stale commands or duplicate the companion.
- Unreachable routes stop after a bounded recovery attempt.
- Memory reset/deletion and no-spoiler filtering work independently of model compliance.

Detailed evidence and process budgets are in [validation and operations](VALIDATION_AND_OPERATIONS.md).
