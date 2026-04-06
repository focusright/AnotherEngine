
## `docs/PROJECT_STATE.md`

```md
# AEDD PROJECT STATE

## Current Version
v0.0.2.1 - AE structure migration pass

## Current Focus
Finish and lock the structural migration from prototype layout to real engine/editor/build/content layout without changing runtime behavior.

## Completed Before v0.0.2.1
- v0.0.2 completed
- minimal ImGui integration exists
- scene save/load exists
- object transform gizmo exists
- current editor remains functional as the working baseline

## v0.0.2.1 Goals
- migrate code out of `prototypes/modelingtool` into real editor/engine folders
- move Visual Studio solution/project files into a real build folder
- move default scene/content files out of `prototypes/`
- preserve behavior while improving structure
- prepare AE to become the engine dependency for Dark Defiance later
- avoid feature work during this pass

## Current Structure
- `editor/` - editor app, camera, gizmo, commands, shared editor paths, main entry
- `editor/modes/modeling/` - current modeling mode data structures
- `engine/core/` - core engine runtime-side code
- `engine/gfx/` - graphics device and D3D12 setup code
- `third_party/` - shared third-party dependencies
- `build/vs2022/` - Visual Studio solution/project files
- `assets/scenes/` - current scene/content files

## Rules For This Pass
- structure changes only
- no behavior changes unless required for compile/build/asset path repair
- keep diffs small and surgical
- do not begin runtime spine work yet
- do not begin v0.0.3 topology refactor work yet

## v0.0.2.1 Completion State
This pass is considered complete when all of the following are true:

- code no longer lives under `prototypes/modelingtool/`
- build files no longer live under `prototypes/modelingtool/`
- the Solution Explorer folder structure matches the real repo layout
- Debug and Release use consistent include paths
- Debug and Release both copy `assets/scenes/scene.aem` beside the built executable
- the default scene path is resolved relative to the executable directory
- default scene path usage is centralized rather than duplicated across files
- docs match the real repo layout and current build/runtime behavior

## Current Outcome
At the intended end state of v0.0.2.1:

- source lives in `editor/`, `engine/`, and `third_party/`
- build files live in `build/vs2022/`
- content lives in `assets/scenes/`
- the default scene is addressed through shared path helpers
- Visual Studio debug runs and built binaries follow the same scene-file rule
- the repo no longer presents itself as a prototype-first project

## Next Planned Step After This Pass
v0.0.2.2 - runtime spine bootstrap