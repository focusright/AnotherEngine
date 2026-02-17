# AGENTS.md — Modeling Tool Subtree Rules

This file overrides or tightens root AGENTS.md rules
for everything inside prototypes/modelingtool/.

This subtree is the active development surface.

---

# 1. Scope of Work

Only modify files inside:

prototypes/modelingtool/

Do NOT:
- Modify root-level architecture
- Introduce new modules outside this subtree
- Refactor engine-wide systems

All changes must be local and surgical.

---

# 2. Current Baseline

Baseline is the currently compiling Visual Studio solution:

modelingtool.sln

Files known to exist:

- main.cpp
- App.*
- Engine.*
- EditableMesh.*
- RenderMesh.*
- EditorCamera.*
- GraphicsDevice.*

Do NOT assume additional systems exist.

---

# 3. D3D12 Layer Discipline

Whenever modifying rendering code, explicitly identify which D3D12 mental layer is being touched:

1. Execution / Synchronization
2. Resource & Memory
3. Fixed Function Pipeline
4. Programmable / Shader

Never mix concerns without justification.

---

# 4. Modeling Tool Goals

This tool must eventually support:

- Primitive-first workflow (tetrahedron first)
- Selection (face/object)
- World-space translation gizmo
- Scene save/load (.aem)
- Instantiable asset output

Current immediate priority:

Fix translate gizmo behavior bugs.

No new modeling features allowed until:
- Axis selection stable
- Drag projection stable
- Movement scaling consistent

---

# 5. Code Change Policy

All modifications must:

- Be incremental
- Compile immediately
- Avoid wide rewrites
- Avoid renaming large structures
- Avoid formatting churn

When presenting changes:
- Specify file name
- Show exact changed block
- Explain why change is needed
- Identify affected D3D12 layer (if applicable)

Never present only a zip.
Never rewrite entire files.

---

# 6. Camera + Gizmo Constraints

- Gizmo is world-space only (no local space yet)
- X/Y/Z axis only
- No rotate
- No scale
- No multi-select

Do not expand scope.

---

# 7. Stability Over Features

If a feature works but is messy,
do NOT refactor unless explicitly requested.

The priority is forward progress with stability,
not architectural elegance.
