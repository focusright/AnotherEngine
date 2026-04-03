# AGENTS.md — Modeling Tool Subtree Rules

This file overrides or tightens root AGENTS.md rules for everything inside `prototypes/modelingtool/`.

This subtree is the active development surface.

---

## 1. Scope of Work

Only modify files inside:

`prototypes/modelingtool/`

Do not:
- modify root-level architecture unless explicitly requested
- introduce new modules outside this subtree unless explicitly requested
- perform wide refactors when a small local change is enough

All changes must be local and surgical by default.

---

## 2. Current Baseline

Baseline is the currently compiling Visual Studio solution:

`modelingtool.sln`

Files known to exist include:
- `main.cpp`
- `App.*`
- `Engine.*`
- `EditableMesh.*`
- `RenderMesh.*`
- `EditorCamera.*`
- `GraphicsDevice.*`
- `Gizmo.*`
- `EditorCommands.h`
- `EditorContext.h`

Do not assume additional systems exist unless they are present in the repo.

---

## 3. D3D12 Layer Discipline (DO NOT need to do this anymore)

Whenever modifying rendering code, explicitly identify which D3D12 mental layer is being touched:

1. Resources / Memory
2. Commands / Synchronization
3. Fixed Function Pipeline
4. Programmable / Shader

Do not mix concerns without justification.

---

## 4. Current Tool State

`v0.0.2` is complete in this subtree.

Implemented features include:
- primitive scene with tetra objects
- object selection
- vertex selection
- world-space translate / scale / rotate gizmo
- scene save/load (`.aem`)
- minimal Dear ImGui scene window
- editor command spine
- focus camera

Important note:
- the current rotate behavior uses the existing axis gizmo
- a separate new rotation gizmo is not required for v0.0.2

---

## 5. Code Change Policy

All modifications must:
- be incremental
- compile immediately
- avoid wide rewrites
- avoid renaming large structures unless requested
- avoid formatting churn

When presenting changes:
- specify file name
- show exact changed block
- explain why the change is needed
- identify affected D3D12 layer when relevant

Never present only a zip.

---

## 6. Current Gizmo Constraints

- gizmo is world-space only
- X/Y/Z axis only
- translate, scale, and simplified rotate are present
- no local-space gizmo yet
- no multi-select

Do not expand scope unless explicitly asked.

---

## 7. Next Planned Step

After v0.0.2, the next planned milestone is **v0.0.2.1**, the structure migration pass.

Priority in that pass:
- move the modeling tool toward the actual engine folder structure
- lay structural foundation for the real engine repo shape
- avoid unnecessary feature expansion during migration