# PROJECT_STATE.md

## Project
Another Engine / AEDD

## Current Version
**v0.0.2 complete**

## Current Status Summary
The v0.0.2 modeling tool milestone is functionally complete.

The current prototype now includes:
- scene save/load through the custom `.aem` text format
- minimal Dear ImGui integration
- editor command spine via `EditorCommandType` and `ExecuteCommand(...)`
- focus camera on active object
- object selection
- vertex selection
- direct transform editing
- world-space gizmo support for translate / scale / rotate

The separate new rotation gizmo is no longer needed for v0.0.2. The current axis gizmo is sufficient for this milestone.

---

## Implemented in v0.0.2

### 1. Scene save/load
Scene state can be saved and loaded using the custom `.aem` text format.

Saved state currently includes:
- object count
- active object index
- object position
- object rotation
- object scale
- per-object tint

Available through:
- `Ctrl+S` / `Ctrl+O`
- ImGui `Save` / `Load` buttons

### 2. Minimal Dear ImGui integration
The Scene window currently provides:
- `Add`
- `Duplicate`
- `Delete`
- `Save`
- `Load`
- object list with active-object selection
- last command display
- gizmo mode buttons
- direct editing of position / rotation / scale

### 3. Editor command spine
The editor now routes core actions through `EditorCommandType` and `ExecuteCommand(...)`.

This currently covers:
- add object
- duplicate active object
- delete active object
- set active object
- save scene
- load scene
- set active transform
- set gizmo mode
- focus camera

### 4. Focus camera
Pressing `F` focuses the camera on the active object and updates the persistent orbit pivot.

### 5. Gizmo behavior
The current gizmo supports:
- translate
- scale
- rotate

Rotate behavior note:
- rotate uses the existing axis gizmo, not a separate ring gizmo
- rotate drag is driven by screen-space drag projected onto the selected axis direction on screen
- this replaced the earlier nonlinear rotation approach

### 6. Selection / transform flow
The current tool supports:
- object selection
- vertex selection
- object transform editing
- object/vertex movement through the gizmo

---

## Current Controls

### Camera
- `RMB + Mouse` = fly look
- `RMB + WASD` = fly move
- `MMB + Mouse` = orbit around persistent view pivot
- `Shift + MMB + Mouse` = pan
- Mouse wheel = dolly
- `F` = focus active object

### Scene / Editor
- `LMB` = gizmo interaction / selection
- `Ctrl+S` = save scene
- `Ctrl+O` = load scene
- `N` = add object
- `Ctrl+D` = duplicate active object
- `Delete` = delete active object

---

## Important Current Decisions

### Rotation gizmo decision
A separate new rotation gizmo is **not** required for v0.0.2.

For now, rotation will continue to use the current axis gizmo with the simplified screen-space drag behavior.

### Scope discipline
v0.0.2 should not be re-expanded. It is complete and should now be treated as the finished baseline for the next phase.

---

## Next Version

## v0.0.2.1 — AE structure migration pass
This is the immediate next step.

### Purpose
Move the modeling tool out of its prototype-style placement and lay the structural foundation for the real engine repository layout before runtime-spine bootstrap work begins.

### Main Goals
- move modeling-tool code into the correct long-term engine/editor folder structure
- reduce prototype-specific layout assumptions
- establish the proper structural shape of Another Engine
- prepare the repo for Dark Defiance depending on Another Engine cleanly
- avoid feature creep during migration

### Constraints
- do not expand scope with unrelated new modeling features
- do not destabilize the current v0.0.2 behavior
- preserve the working baseline while improving structure
- keep changes incremental and understandable

### Expected Outcome
At the end of v0.0.2.1:
- the repo layout should look more like the real engine and less like a temporary prototype
- the modeling tool should be living in the correct structural home
- the codebase should be ready for the runtime spine bootstrap phase

---

## Planned After That

## v0.0.2.2 — runtime spine bootstrap
This is the planned phase after the structure migration pass.

### Purpose
Begin the runtime/game-side foundation after the repo and engine structure are in the correct shape.

### Broad Goal
Create the first runtime spine needed for Dark Defiance without mixing that work into the v0.0.2 modeling milestone.

### Notes
This phase should happen **after** v0.0.2.1, not before.

---

## Longer-Term Planned Version

## v0.0.3 — modeling structure refactor + host-decoupling seam refactor
This is a planned later milestone and may still evolve.

### Planned Modeling Goal
Refactor the modeling-tool structure so later real topology editing features are supported cleanly, including:
- extrude
- edge transforms
- face splits
- more serious topology editing

### Planned Host-Decoupling Goal
Refactor desktop-specific assumptions out of shared editor/tool logic so future alternative hosts remain possible, including:
- desktop
- VR
- mobile
- headless / automation-oriented hosts

### Important Note
v0.0.3 is planned, but details may shift after v0.0.2.1 and v0.0.2.2 are complete.

---

## Files Most Relevant to Current Baseline
- `prototypes/modelingtool/main.cpp`
- `prototypes/modelingtool/App.cpp`
- `prototypes/modelingtool/App.h`
- `prototypes/modelingtool/Gizmo.cpp`
- `prototypes/modelingtool/Gizmo.h`
- `prototypes/modelingtool/EditorCommands.h`
- `prototypes/modelingtool/EditorContext.h`

---

## Current Overall Assessment
v0.0.2 is complete.

The correct next action is to move forward into **v0.0.2.1**, not to keep adding more features into v0.0.2.