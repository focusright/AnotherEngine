# AGENTS.md — AnotherEngine Operational Contract

This file defines how AI agents (Codex) must operate inside this repository.

## 1. Mission

AnotherEngine is a solo-built engine and game project inspired by:
- Another World
- Fade to Black
- Silent Hill 2
- The Last of Us
- Alone in the Dark (1992 character models)
- 4D Sports Boxing (character + ring only)

The goal is to build everything from scratch in code (Eric Chahi model).

The current focus is the Modeling Tool prototype.

---

## 2. Non-Negotiable Rules

### No Hallucinated Code
- NEVER invent APIs, classes, methods, or structure.
- Only use symbols that exist in the repository.
- If unsure, search the codebase first.

### Small Surgical Changes Only
- No large refactors.
- No architecture rewrites.
- Every change must be incremental and minimal.

### Always Provide:
- Exact file names modified.
- Clear explanation of what changed and why.
- No "zip only" responses.
- No massive rewrites.

### Code Style
- Function parameter lists must remain on a single line.
- Match existing formatting and patterns.
- Do not introduce new architectural layers unless explicitly requested.

---

## 3. Current Development Phase

Version: v0.0.2

Locked Scope:
- Scene Save/Load (.aem format)
- Minimal Dear ImGui integration
- Mouse-driven world-space translate gizmo (X/Y/Z axis only)

Immediate Priority:
Fix behavioral bugs in the translate gizmo before adding any new features.

DO NOT add new features until gizmo behavior is stable.

---

## 4. Rendering Constraints (Aesthetic Spine)

The engine must preserve the following aesthetic:

- No textures
- No normal maps
- No specular highlights
- Large flat-shaded color regions
- Minimal diffuse (single directional)
- Strong ambient dominance
- Visible polygon facets
- Lighting subordinate to authored color
- 24 FPS target (cinematic feel)

---

## 5. Engine Constraints

- D3D12-first architecture
- WebGPU seam later
- Instant play (zero-loading start)
- Background streaming allowed with placeholder visuals
- Editor and runtime separated
- Single executable editor
- VR support must remain architecturally possible later

---

## 6. Modeling Tool Direction

Modeling tool must:
- Produce instantiable assets
- Support primitive-first workflow
- Maintain Unreal-style instancing workflow
- Use tetrahedron as first primitive

Current work target:
Fix translate gizmo bugs.
