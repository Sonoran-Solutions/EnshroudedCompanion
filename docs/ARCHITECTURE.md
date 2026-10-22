# Architecture

Status: **PROPOSED**. No component or API in this document is implemented. Action names and JSON fields below are this project's intended contracts, not verified Enshrouded APIs. The [Phase 0 gate](PHASE_0_HANDOFF.md) decides what can actually be supported.

## Core separation

The model chooses intent and dialogue. Deterministic controllers execute game behavior. The game remains authoritative about what happened.

~~~mermaid
flowchart TD
  G["Authoritative game runtime"] --> B["Versioned native bridge"]
  B --> W["Observed world state"]
  W --> C["Context and scoped memory"]
  C --> M["Dialogue and intent providers"]
  M --> V["Schema and live policy checks"]
  O["Authenticated owner controls"] --> V
  V --> A["Deterministic action controllers"]
  A --> B
  B --> R["Observed results and journal"]
  R --> W
  R --> C
~~~

The bridge is a thin, version-specific adapter. It exports bounded snapshots/events and executes a small allowlisted command set on the correct engine thread. It does not run inference, block for HTTP, or trust a model to supply function pointers, offsets or code.

The companion service runs outside the game process. It owns provider configuration, prompts, persona, memory, budgets, planning and telemetry. A client-side overlay or chat UI can be added after the bridge is proven; text commands are sufficient for the first slice. Do not assume a server-only mod can automatically draw a custom client UI.

## Proposed components

| Component | Responsibility | Explicit non-responsibility |
| --- | --- | --- |
| Bridge adapter | Build validation, observations, ownership, safe engine calls, results | Provider credentials, lore generation, arbitrary remote commands |
| World-state store | Current snapshot, event sequence, provenance, capability availability | Filling missing fields with model guesses |
| Context builder | Relevant observed facts, owner-selected goals, scoped memories | Sending every frame or all chat history |
| Dialogue provider | In-character text, streaming where useful | Declaring an action succeeded |
| Intent provider | Choose from typed allowed actions or return no action | Direct writes or per-frame motor control |
| Controller/executor | Follow/assist/retreat behavior, preconditions, native dispatch | Free-form generated code |
| Memory store | Character identity, confirmed shared events, user-managed preferences | Authoritative quest/save progress |
| Owner UI | Commands, consent, budgets, stop/disable, status | Delegating authority to arbitrary in-game text |

One model can implement both logical provider roles initially. Separate interfaces do not require loading two models or paying for two calls on every event.

## Candidate implementation stack

Prefer a native bridge in the language required by the proven loader: for example Rust for an Ember-based experiment or C++ for another native adapter. Keep that decision open until P0 selects a tested path.

A small Python service with typed validation, an async HTTP client and SQLite is a reasonable proposed backend for iteration and local-model integration. JSON Schema is the cross-language contract. Avoid a large web application, vector database, multi-agent framework or message broker until a measured need appears.

Start with local authenticated IPC, such as a loopback socket with a per-session token and restrictive access. LAN access is a separate deployment mode requiring explicit binding, authentication, encrypted transport or an equivalently protected tunnel, and host firewall configuration. Do not expose an unauthenticated game-control endpoint.

## Observations

Each snapshot must include:

- Contract/schema version and bridge capability version.
- Exact game build/fingerprint and adapter version.
- World/profile ID, connection-session ID, save/load epoch, owner ID, companion ID and entity generation where available.
- Bridge-issued snapshot sequence and observation time/age semantics.
- Field-level availability/provenance, including stale or unknown values.
- Relevant position, health, nearby entities, supported hazards and current controller state.
- Quest records with stable identifiers, scope and affected player/world; selected objective recorded separately.
- A bounded event sequence, gap detection, and a resynchronization path after missed events.

Identity must be proven, not derived from a display name. If no stable world ID can be read, require an explicit owner-selected profile mapping or keep memory session-only. Unknown association blocks persistent-memory reuse.

The bridge owns freshness calculations. Do not compare unrelated monotonic clocks across processes or machines. A decision echoes a bridge-issued snapshot token; the bridge checks the token's identity, age and relevant preconditions using its own clock and current state.

Events can arrive twice or out of order. Detect sequence gaps, deduplicate known IDs, and obtain a new snapshot. A lost completion event must not become a fabricated success.

## Proposed intent envelope

Illustrative JSON; schema definitions are a P1 deliverable.

~~~json
{
  "schema_version": 1,
  "capability_version": "probe-1",
  "proposal_id": "proposal-example-17",
  "snapshot_token": "opaque-bridge-token",
  "world_id": "test-world-a",
  "session_id": "connection-4",
  "save_epoch": 2,
  "owner_id": "owner-a",
  "companion_id": "companion-a",
  "action": "follow_owner",
  "arguments": {
    "distance_m": 4
  }
}
~~~

Initial candidate actions are follow_owner, stop, guard_position, assist_target and retreat_to_owner. Later candidates include collect_allowed_resource and execute_blueprint_job. Only actions proven by the active bridge appear in its capability manifest. Never expose unavailable actions to the model.

The executor validates syntax, enum values, bounds, entity identity, capability/schema version, snapshot scope and action-specific freshness. It then checks current permissions, target validity, health/hazards, inventory, cooldowns and work-area limits immediately before dispatch. Unknown critical state means denial. Passing JSON validation is not permission to execute.

## Action lifecycle and crash uncertainty

Use a lifecycle such as PROPOSED -> VALIDATED -> DISPATCHED -> RUNNING -> SUCCEEDED, FAILED, CANCELLED or UNKNOWN_OUTCOME. REJECTED is a terminal result before dispatch. Cancellation of a native operation is not assumed to undo a completed side effect.

Assign one controller owner per actor and serialize conflicting actions. Persist a bounded execution intent/journal before dispatching material side effects. Keep proposal IDs scoped to the world/session/save epoch and reject duplicates in the applicable execution ledger.

A crash can occur after the game applies a change but before acknowledgement is recorded. Therefore **do not promise exactly-once execution**. Mark the result UNKNOWN_OUTCOME, reconcile against the authoritative game state, and stop for operator review if the outcome cannot be established. Never blindly repeat resource consumption, placement, demolition, loot transfer or reward actions.

Prefer setting a controller's desired mode idempotently over repeatedly issuing "move" operations. Material world edits need a separate job ledger and [building recovery policy](BUILDING.md).

## Scheduling and control

- Local controllers react on the game-supported update cadence; initial rates are tunable and must be profiled.
- The planner runs on meaningful events or bounded decision intervals, not every rendered frame.
- Coalesce redundant observations, cap context size, bound queue depth, and permit only a small configured number of in-flight requests.
- Owner stop/disable and deterministic emergency behaviors outrank planner goals.
- A provider timeout, invalid output or disconnect produces no new action. Existing controllers follow their explicitly tested fallback policy.
- Do not dispatch a structured decision until the entire response has been parsed and validated. Stream dialogue separately.
- Revalidate between controller steps and construction operations, not just when a long task begins.

## Authority and hostile inputs

Only an authenticated owner or authorized host can change permissions, enable features, approve building or alter spending limits. NPC text, chat messages, lore, item labels, retrieved content and speech transcripts are data, not instructions that can raise privileges.

Use explicit controls for dangerous permissions. A character saying "I give you permission" in generated dialogue has no effect. Restrict action arguments to typed IDs and bounded numbers; never pass model text into a shell, database query, file path or native-call selector without a safe dedicated interface.

## Persistence and reload

Keep mod metadata separate from the game save unless a tested save-cycle integration safely associates it. Prefer a versioned transactional store. Persist persona and confirmed memories with world/profile and character scope; keep authoritative gameplay facts re-readable from the game.

On load, world switch, rollback or reconnect: rotate the appropriate session/epoch, cancel old requests, clear transient targets, rebind identities, reconcile companion existence and jobs, and refresh state before allowing actions. If rollback cannot be detected reliably, require explicit profile reset/reconciliation rather than assuming the newest memory is true.

Enforce uniqueness so a reload cannot create a second companion for the same configured binding. Prefer safe suspension over deleting an actor whose ownership is uncertain. Test migrations, reset, deletion and recovery; see [validation](VALIDATION_AND_OPERATIONS.md).
