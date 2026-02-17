# Development Setup — AnotherEngine Modeling Tool

## Environment

- Windows 10/11
- Visual Studio 2022
- Desktop development with C++
- Windows 10/11 SDK
- x64 build

## Opening the Project

Open:

prototypes/modelingtool/modelingtool.sln

Select:
- Configuration: Debug
- Platform: x64

Build → Run (F5)

---

## Controls (Current)

- RMB + WASD → Fly camera
- Mouse → Select
- Gizmo drag → Translate along axis
- Ctrl+S → Save scene (.aem)
- Ctrl+O → Load scene (.aem)

---

## Project Structure

prototypes/modelingtool/
    main.cpp
    App.*
    Engine.*
    EditableMesh.*
    RenderMesh.*
    EditorCamera.*
    GraphicsDevice.*

Scene files use .aem custom text format.

---

## Current Version

v0.0.2

Primary task:
Fix translate gizmo behavior bugs.
