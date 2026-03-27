# PROJECT_STATE.md

## Session
v0.0.2 - Dev End

## Current Scope Status

### v0.0.2 remaining feature tasks
1. Object transform gizmo expansion
   - Rotate gizmo
   - Scale gizmo

### v0.0.2 completed tasks
- Tetrahedron primitive exists in scene
- Minimal Dear ImGui integration
- Object list UI
- Add / Duplicate / Delete object actions
- Scene Save / Load with custom `.aem`
- Mouse object selection
- Vertex selection
- World-space translate gizmo
- Initial command spine
- Command spine completion for current v0.0.2 scope

## What Was Done This Session

### 1. Command spine completion finished
We completed the remaining v0.0.2 command-spine work.

#### Completed command-spine routing
- `F` focus now routes through `ExecuteCommand(...)`
- Mouse viewport object selection now routes through `ExecuteCommand(...)`
- Convenience overload added for payload-free commands:
  - `ExecuteCommand(EditorCommandType type)`

#### Debug visibility
- Last executed command tracking/readout added

### 2. Verified input behavior for focus shortcut
We investigated the `F` shortcut path and confirmed:
- `fPressed` is a one-frame pulse
- It is cleared each frame by `BeginFrameInput()`
- It does not need `WM_KEYUP` handling
- The missing part was making sure `WM_KEYDOWN` for `F` actually sets it

### 3. Pixel-perfect picking deferred
We explicitly decided:
- pixel-perfect object picking is a future feature
- it is deferred for now
- it should not affect current v0.0.2 work

### 4. Translate gizmo extracted into its own class
We performed the first gizmo refactor step:
- current translate gizmo behavior was moved out of `App`
- new files added:
  - `Gizmo.h`
  - `Gizmo.cpp`

Goal of this step was zero behavior change.

### 5. Fixed selection regression caused by gizmo refactor
During gizmo extraction, object and vertex selection stopped working.

Root cause:
- `SetActiveObject(...)` had been reduced to only `m_gizmo.Reset();`
- it no longer assigned `m_activeObject`

Fix:
- restored real `SetActiveObject(...)` behavior
- moved implementation out of `App.h` into `App.cpp`
- this also fixed the `RenderMesh` incomplete-type compile error caused by accessing `m_renderMesh` members in an inline header function

## Current Working State

### Working
- Object selection works
- Vertex selection works
- Translate gizmo works after extraction
- Command spine works at current v0.0.2 scope
- Last-command debug readout works

### Deferred
- Pixel-perfect object picking

## Important Architecture Decisions Locked In

### Rotate gizmo visual style
Rotate rings should be:
- simple
- visually consistent with the current gizmo arms
- 3 axis-color-coded rings
- on the XY, XZ, and YZ planes

### Gizmo architecture direction
We are keeping the current simple gizmo and evolving it into a class hierarchy:
- current gizmo should become the base/foundation class
- fuller object transform gizmo should derive from it
- simple gizmo remains useful later for vertex/edge manipulation

## Next Planned Task

### Next task
Continue gizmo architecture refactor:

1. Turn current gizmo into base/foundation class
2. Add fuller object transform gizmo derived from it
3. Add gizmo mode scaffolding:
   - Translate
   - Scale
   - Rotate
4. Keep behavior translate-only at first
5. After that:
   - implement Scale first
   - implement Rotate second

## Notes / Constraints To Preserve
- Work in small surgical diffs
- Do not invent APIs or structures not present in the actual codebase
- Keep current simple gizmo available for future vertex/edge manipulation
- Rotate gizmo rings must stay visually simple
- Pixel-perfect picking is deferred and should not leak into current work