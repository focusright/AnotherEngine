# DECISIONS.md

This document records **intentional architectural and design decisions**.
Purpose:
- Preserve *why* choices were made
- Prevent accidental refactors that violate core goals
- Give future-me (or Codex) context when the code alone is ambiguous

Entries should be **short and factual**, not essays.

---

## 2026-02-03 — Instant Play Is Mandatory

**Decision**  
The game must start immediately on Play, with no apparent loading screens.

**Reason**  
Flow and immersion are higher priority than initial visual fidelity. Streaming + placeholders are acceptable; waiting is not.

**Implication**  
- All systems must tolerate missing/partial data
- Assets must stream asynchronously
- Levels store references, not raw data blobs

---

## 2026-02-03 — Modeling Tool Comes First

**Decision**  
Start engine development from the **modeling tool**, not gameplay or renderer abstractions.

**Reason**  
Games need content immediately; tools define the asset pipeline and constrain architecture early in a good way.

**Implication**  
- EditableMesh → RenderMesh separation is foundational
- Editor code is not “throwaway”

---

## 2026-02-03 — EditableMesh / RenderMesh Hard Boundary

**Decision**  
Editing logic and GPU logic must be strictly separated.

**Reason**  
Prevents editor concerns from leaking into runtime and simplifies future ports (WebGPU).

**Implication**  
- EditableMesh contains topology + edit state only
- RenderMesh owns GPU buffers only
- RenderMesh never clears dirty flags

---

## 2026-02-03 — Unreal-Style Static Mesh Instancing

**Decision**  
Models created in the editor become **MeshAssets** that can be instanced easily in-game (drag-and-drop mental model).

**Reason**  
Proven workflow; avoids copying geometry into levels; scales naturally to streaming and instancing.

**Implication**  
- Levels reference MeshAsset handles + transforms
- Assets have stable IDs and paths
- Rendering supports many instances per mesh

---

## 2026-02-03 — Single Editor Executable, Multi-Mode

**Decision**  
Use one editor executable with mode switching (Modeling → Rigging → Animation).

**Reason**  
Shared viewport, shared context, easier VR support later, simpler mental model.

**Implication**  
- Central EditorContext
- ModeManager + IMode interface
- Input/UI routed by active mode

---

## 2026-02-03 — VR Is Editor-Only (For Now)

**Decision**  
VR support is planned only for the editor, not required for the shipped game.

**Reason**  
VR modeling is valuable; VR gameplay is not a goal.

**Implication**  
- View/input abstraction must allow alternate devices
- Runtime/gameplay code must not depend on VR

---

## 2026-02-03 — Unified Animation Tracks

**Decision**  
Keyframe and procedural animation are treated as the same abstraction: time-parameterized tracks.

**Reason**  
Avoids parallel animation systems and allows layering, blending, and LOD naturally.

**Implication**  
- glTF animation compiles into track evaluators
- Procedural animation plugs into the same runtime path

---

## 2026-02-03 — Differential Geometry & ML Are Optional Hooks

**Decision**  
DG and ML features are planned but **must not block core engine progress**.

**Reason**  
They add uniqueness and power, but shipping comes first.

**Implication**  
- Architect extension seams early
- Implement DG/ML as optional modules later
- Prefer editor/offline ML over runtime dependence

---

## 2026-02-03 — D3D12 First, WebGPU Later

**Decision**  
Target D3D12 initially with a deliberate seam for WebGPU.

**Reason**  
Native performance + learning goals now; portability later.

**Implication**  
- Avoid leaking D3D12 concepts into gameplay/editor logic
- Keep rendering interface narrow and explicit

---

## 2026-02-03 — Small Prototypes Feed the Engine

**Decision**  
Develop features in small prototype apps before integrating into th
