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