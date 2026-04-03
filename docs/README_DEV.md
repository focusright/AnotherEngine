# README_DEV.md

## Build

Open:

`prototypes/modelingtool/modelingtool.sln`

Select:
- Configuration: `Debug`
- Platform: `x64`

Build and run with `F5`.

## Current Version

`v0.0.2`

Status: complete.

The current prototype includes:
- scene save/load via `.aem`
- minimal Dear ImGui integration
- command-driven editor actions
- focus camera
- world-space gizmo with translate / scale / rotate

## Controls

### Camera
- `RMB + Mouse` → fly look
- `RMB + WASD` → fly move
- `MMB + Mouse` → orbit around persistent view pivot
- `Shift + MMB + Mouse` → pan
- Mouse wheel → dolly
- `F` → focus active object

### Scene / Editor
- `LMB` → gizmo interaction / selection
- `Ctrl+S` → save scene (`scene.aem`)
- `Ctrl+O` → load scene (`scene.aem`)
- `N` → add object
- `Ctrl+D` → duplicate active object
- `Delete` → delete active object

## ImGui Scene Window

The `Scene` window currently provides:
- `Add`, `Duplicate`, `Delete`
- `Save`, `Load`
- object list selection
- last command display
- gizmo mode buttons: `Translate`, `Scale`, `Rotate`
- direct editing for `Position`, `Rotation`, and `Scale`

## Project Structure

Primary active subtree:

```text
prototypes/modelingtool/
    main.cpp
    App.*
    Engine.*
    EditableMesh.*
    RenderMesh.*
    EditorCamera.*
    GraphicsDevice.*
    Gizmo.*
    EditorCommands.h
    EditorContext.h
    third_party/imgui/