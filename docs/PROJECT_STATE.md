# PROJECT_STATE.md

## Project
AEDD — Another Engine Dark Defiance

## Current Version
v0.0.2

## Session Summary
This session continued the v0.0.2 Editor command spine work against the updated baseline.

The baseline already contained the first real command-spine slice:
- `EditorCommands.h`
- `App::ExecuteCommand(...)`
- hotkeys routed through the command spine
- ImGui buttons routed through the command spine

This session did not introduce new code changes into the baseline. The main outcome was re-anchoring to the updated codebase and identifying the next smallest surgical step for the command spine.

## Current Confirmed State

### Completed in v0.0.2
- Tetrahedron primitive
- Object selection
- Mouse object selection
- Minimal Dear ImGui integration
- Object list
- Add / Duplicate / Delete
- Scene save/load (`.aem`)
- Translate gizmo foundation
- Initial Editor command spine slice:
  - `EditorCommands.h`
  - `App::ExecuteCommand(...)`
  - hotkeys routed through the command spine
  - ImGui action buttons routed through the command spine

### Remaining v0.0.2 Tasks
1. Editor command spine
2. Object rotate/scale gizmo

These remain the only two locked v0.0.2 tasks.

## Exact Next Step
Continue the Editor command spine by routing mouse object selection through `ExecuteCommand(...)` instead of calling `SetActiveObject(...)` directly.

The next surgical changes are:

1. In `App.cpp`, replace direct mouse-pick object selection:
   - from direct `SetActiveObject((uint32_t)hitObject);`
   - to an `EditorCommand` with `EditorCommandType::SetActiveObject` sent through `ExecuteCommand(...)`

2. Add tiny last-command debug state:
   - store last executed command name in `App`
   - expose `LastCommandName()`
   - display it in ImGui

This is intended only as a debug/readout aid to verify the command spine is being used consistently.

## Important Decisions Reconfirmed
- The locked source of truth for remaining v0.0.2 work is:
  1. Editor command spine
  2. Object rotate/scale gizmo
- Older internal docs in previous baselines that referenced translate gizmo bug fixing are stale and no longer the source of truth.
- `enum class EditorCommandType` remains the correct internal representation for the command spine at this stage.
- Do not expand the command spine into undo/redo, automation scripting, registries, or string-driven systems during v0.0.2.
- Keep all work surgical and grounded in the actual baseline.

## Known Non-v0.0.2 Note
A shutdown bug was identified earlier:
- ImGui DX12 backend shutdown was being called twice on exit
- fix: keep ImGui shutdown ownership in one place only, rather than both `ShutdownImGui()` and `ShutdownD3D()`

This is not one of the two remaining v0.0.2 tasks, but it is a known cleanup/fix item already discussed.

## Next Session Start Point
Resume with the next Editor command spine step:
- route mouse object selection through `ExecuteCommand(...)`
- add the tiny last-command debug readout