# Building together

Status: **PROPOSED; native construction capability NOT_VERIFIED**. Building is a separate high-risk feature track, not a prerequisite to learning whether the basic companion works.

## Why building is different

A model can describe a house without being able to place one safely. The hard requirements are exact coordinates/orientation, geometry, collision, permissions, resource consumption, replication and save consistency. An apparently successful placement can still duplicate materials or corrupt persistent state.

The builder must use a verified native placement pathway. A generic memory write, edited save blob, container-placement notification or visual ghost preview is not proof of safe world construction.

## Staged scope

| Stage | Intended capability | Gate |
| --- | --- | --- |
| P0 construction probe | One whitelisted native voxel or prop edit in a disposable test area | Authority, resources, replication, save/reload and recovery proven for that edit type |
| Helper jobs | Fetch approved materials or perform a defined allowed task | Proven inventory/interaction path with limits |
| Blueprint builder | Place a small validated structure in an approved work area | Preflight, immutable approval, bounded steps and job recovery |
| Layout assistant | Propose variants using approved parts and constraints | Deterministic compiler/validator and owner preview |
| Freeform architecture | Open-ended layout generation | Separate later research; not promised by the initial plan |

A prop-only probe proves props, not voxels. Record capabilities separately. If the original building vision requires voxels and only props work, report that limitation explicitly.

## Blueprint contract

A blueprint is structured data, not executable code. It contains:

- Schema version, blueprint ID/version/hash and supported game/adapter capabilities.
- An anchor, orientation, grid convention and bounded work-area volume.
- Allowed block/prop IDs and deterministic transforms.
- Ordered operations and dependencies.
- Expected initial state, collision/support checks and protected regions.
- Material bill and validated source inventory references.
- Operation count, rate and total-job limits.
- Recovery metadata and a preview of expected changes.

The model may propose a layout or select an approved template. A deterministic compiler validates dimensions and IDs, computes materials and generates exact operations. The model does not issue arbitrary coordinates directly to native code.

## Approval and execution

1. Select a site the owner is authorized to edit.
2. Read the current area and inventories through verified paths.
3. Preflight the blueprint: permissions, boundaries, collisions, supported materials and resource sufficiency.
4. Show the exact plan, resource budget and any removal operations.
5. Bind owner approval to the plan hash/version, world/save epoch, site and maximum cost.
6. Revalidate before each native operation, journal intent, dispatch, and verify the observed result.
7. Stop on changed conditions, missing materials, cancellation, conflict or unknown outcome.
8. Reconcile, then report verified completion or a partial/blocked job.

Approval is invalidated if the plan, site, destructive scope or resource budget changes. Do not ask for a click per block when a bounded unchanged plan is already approved, but do not let blanket conversational consent authorize a materially different job.

Start with additive construction only. Demolition, replacing existing structures, terrain removal and bulk modifications remain disabled until separately approved and tested.

## Resources and concurrency

Use normal game material debit and entitlement checks; never emulate a debit only in the companion's private database. Count inventory only from fresh observations. Reservations are advisory unless the game provides real reservation/transaction support: other players may consume or move materials, so recheck at execution.

One executor owns the job. Prevent concurrent workers from editing the same area or spending the same resources. Coordinate with human building through conflict detection; do not overwrite unexpected changes.

Enforce job bounds even if the model requests more. No material spawning, unlimited blocks or duplication loopholes as a hidden fallback.

## Crash and rollback policy

Before the first experimental edit, create a restorable backup of the disposable world and verify the restore procedure. Back up using a supported consistent save point or while stopped; a random copy of actively written files may be unusable.

The job journal records intent, known prior state, resource expectations, dispatch and observed outcome. If a crash happens between placement and acknowledgement, mark UNKNOWN_OUTCOME. Inspect the world and inventory before resuming; never blindly replay that operation.

Rollback is **not assumed to be atomic**. Native undo may be limited, and restoring a whole save may discard other players' progress. Stop all writers and obtain owner approval before any world-wide restore. Inverse edits are allowed only when proven safe and the area still matches the job's expected state. Otherwise preserve evidence and ask for recovery direction.

For a P0 disposable test, successfully restoring the isolated test world can establish a laboratory recovery path; it does not establish production per-job undo. P5 must separately prove safe partial-job reconciliation and a usable production recovery policy.

## Builder acceptance scenarios

- Place the allowed primitive with the correct material cost; insufficient resources make no change.
- Deny another player's or protected area and reject out-of-bounds coordinates.
- A second client sees the same state; the edit survives save/reload.
- Duplicate proposals, delayed responses and reconnects do not repeat material changes.
- Stop halfway through a blueprint, reconcile, and resume without double charging.
- A competing human edit pauses the job instead of being overwritten.
- Deliberately interrupt a disposable job and demonstrate documented recovery.
- Observe no material duplication, orphaned objects or unreported partial results.
- Claims of completion correspond to verified final geometry and resource outcomes.

The first useful target is a very small, repeatable structure—not "build me any castle." See [P5 in the implementation plan](IMPLEMENTATION_PLAN.md).
