# AEDD PROJECT STATE

## Current Version
v0.0.2.1 - AE structure migration pass

## Current Focus
Finish moving the former modeling tool prototype into the real Another Engine folder structure without changing runtime behavior.

## Completed Before v0.0.2.1
- v0.0.2 completed
- minimal ImGui integration exists
- scene save/load exists
- object transform gizmo exists
- current editor remains functional as the working baseline

## v0.0.2.1 Goals
- migrate code out of `prototypes/modelingtool` into real engine/editor folders
- move Visual Studio solution/project files into a real build folder
- move default scene/content files out of `prototypes/`
- preserve behavior while improving structure
- prepare AE to become the engine dependency for Dark Defiance later
- avoid feature work during this pass

## Current Structure
- `editor/` - editor app, camera, gizmo, commands, main entry
- `editor/modes/modeling/` - current modeling mode data structures
- `engine/core/` - core engine runtime-side code
- `engine/gfx/` - graphics device and D3D12 setup code
- `third_party/` - shared third-party dependencies
- `build/vs2022/` - Visual Studio solution/project files
- `assets/scenes/` - current scene/content files

## Rules For This Pass
- structure changes only
- no behavior changes unless required for compile/build/content path repair
- keep diffs small and surgical
- do not begin runtime spine work yet
- do not begin v0.0.3 topology refactor work yet

## Remaining Tasks To Close v0.0.2.1
- ensure `AnotherEngine.vcxproj.filters` matches the real repo layout
- ensure Debug and Release include paths are consistent
- ensure scene load/save paths use `assets/scenes/scene.aem`
- ensure docs match the real repo layout
- verify build and run after all path updates

## Planned Next Step After This Pass
v0.0.2.2 - runtime spine bootstrap