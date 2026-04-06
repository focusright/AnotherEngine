# AEDD / Another Engine Developer Notes

## Current Version
v0.0.2.1 - AE structure migration pass

## Purpose of This Pass
This pass moves the former modeling tool prototype into the real Another Engine folder structure without changing editor/runtime behavior.

The goal is to stop the project structure from hardening around `prototypes/modelingtool/` and prepare for later runtime spine work.

## Current Working Baseline
The project currently compiles and runs.

Behavior is intended to remain the same as the end of v0.0.2, while the source layout, build-file location, asset path handling, and project identity have been cleaned up.

## Current Repo Layout

```text
editor/
  EditorApp.h
  EditorApp.cpp
  EditorCamera.h
  EditorCamera.cpp
  EditorCommands.h
  EditorContext.h
  EditorMain.cpp
  EditorPaths.h
  Gizmo.h
  Gizmo.cpp
  modes/
    modeling/
      EditableMesh.h
      RenderMesh.h

engine/
  core/
    Engine.h
    Engine.cpp
  gfx/
    GraphicsDevice.h
    GraphicsDevice.cpp

third_party/
  d3dx12.h
  imgui/
    imgui.h
    imgui.cpp
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
    imgui_impl_win32.h
    imgui_impl_win32.cpp
    imgui_impl_dx12.h
    imgui_impl_dx12.cpp

build/
  vs2022/
    AnotherEngine.sln
    AnotherEngine.vcxproj
    AnotherEngine.vcxproj.filters

assets/
  scenes/
    scene.aem

docs/
  README_DEV.md
  PROJECT_STATE.md
  AGENTS.md
```

## Build Instructions
Open:

`build/vs2022/AnotherEngine.sln`

Then build and run the `AnotherEngine` project in Visual Studio.

## Include Path Expectations
The Visual Studio project should include these paths for both Debug and Release:

`$(ProjectDir)..\..`  
`$(ProjectDir)..\..\third_party\imgui`

This allows source includes rooted from repo root, such as:

- `editor/...`
- `engine/core/...`
- `engine/gfx/...`
- `third_party/...`

## Scene Path Rule
The default scene path must resolve relative to the executable directory, not the current working directory.

The intended runtime location is:

`<exe folder>/assets/scenes/scene.aem`

Code should use the shared path helper in:

`editor/EditorPaths.h`

to avoid duplicating scene path strings across multiple files.

## Post-Build Asset Copy
Both Debug and Release builds should copy the repo scene file into the output folder so Visual Studio runs and standalone binaries behave consistently.

Expected output structure:

```text
<output folder>/
  AnotherEngine.exe
  assets/
    scenes/
      scene.aem
```

## What v0.0.2 Already Has
- minimal Dear ImGui integration
- scene save/load using `.aem`
- object selection
- object duplication/deletion
- transform gizmo support
- current editor interaction baseline from the end of v0.0.2

## What v0.0.2.1 Did
- moved source files out of `prototypes/modelingtool/`
- renamed the Visual Studio project identity to `AnotherEngine`
- moved build files into `build/vs2022/`
- aligned Solution Explorer filters with the real folder layout
- moved default scene content into `assets/scenes/`
- unified default scene path usage through `EditorPaths.h`
- squared Visual Studio debug behavior and built-binary behavior for scene file location
- preserved runtime/editor behavior during the migration

## What This Pass Did Not Do
- no runtime spine work yet
- no topology refactor yet
- no host-decoupling refactor yet
- no new modeling features
- no behavior changes except compile/build/asset path repair required by the migration

## v0.0.2.1 Completion Summary
This pass is complete when all of the following are true:

- source files live in real engine/editor folders
- Visual Studio solution/project files live in `build/vs2022/`
- Solution Explorer filters match the real folder layout
- Debug and Release include directories are consistent
- Debug and Release both copy `scene.aem` into the output `assets/scenes/` folder
- scene path logic is centralized through `editor/EditorPaths.h`
- docs match the actual repo layout

## Next Planned Phase
v0.0.2.2 - runtime spine bootstrap

That phase begins only after this migration pass is structurally clean and no longer reads like a prototype project.
