# AEDD PROJECT STATE

## Current Version
v0.0.2.1 - AE structure migration pass

## Current Focus
Move the former modeling tool prototype into the real Another Engine folder structure without changing runtime behavior.

## Completed Before v0.0.2.1
- v0.0.2 completed
- minimal ImGui integration exists
- scene save/load exists
- object transform gizmo exists
- current editor remains functional as the working baseline

## v0.0.2.1 Goals
- migrate code out of `prototypes/modelingtool` into real engine/editor folders
- preserve behavior while improving structure
- prepare AE to become the engine dependency for Dark Defiance later
- avoid feature work during this pass

## Current Structure
- `engine/core/` - core engine runtime-side code
- `engine/gfx/` - graphics device and D3D12 setup code
- `engine/editor/` - editor app, camera, gizmo, commands, main entry
- `engine/editor/modes/modeling/` - current modeling mode data structures
- `third_party/` - shared third-party dependencies
- `prototypes/modelingtool/` - legacy project location still holding the Visual Studio solution/project for now

## Rules For This Pass
- structure changes only
- no behavior changes unless required for compile/build repair
- keep diffs small and surgical
- do not begin runtime spine work yet
- do not begin v0.0.3 topology refactor work yet

## Planned Next Steps After This Pass
1. finish project identity cleanup so the solution no longer reads as a prototype
2. decide whether to rename the Visual Studio project from `modelingtool` to an AE/editor name
3. then begin v0.0.2.2 runtime spine bootstrap