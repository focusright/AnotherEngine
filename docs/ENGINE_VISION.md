# ENGINE_VISION.md

> Single-sentence vision:
Build a small, shippable 3D game + engine/toolchain as a solo developer, targeting PS2-level scope/visuals, with **instant play** (no apparent loading) and a clean seam from **D3D12 → WebGPU** later.

---

## 1) North Star

- Primary goal: **ship complete 3D games** (runtime + tools + formats + gameplay), not “engine feature depth for career signaling”.
- Visual/scope target: **PS2-era** (e.g., Another World in 3D / Fade to Black vibes), level-centric continuous flow (Half-Life-style transitions).
- Influences: Silent Hill 2 fog-heavy mood; The Last of Us pacing/grounded tone.

---

## 2) Hard Requirements (Non-Negotiable)

### Instant Play / Zero Loading
- Press Play → **game starts immediately**.
- All content streams in the background.
- Fidelity may degrade temporarily (wireframe / placeholder meshes / low LOD) to guarantee immediate gameplay.

### Platform & Graphics API Path
- **D3D12 first**.
- Keep an intentional seam to support **WebGPU later** without rewriting gameplay/runtime.

### Editor is a First-Class Product
- Tools are not “side scripts”; they’re core to shipping.
- Editor and runtime are separate concerns but designed to work together.

---

## 3) Editor Architecture

### Single Executable, Multi-Mode
- One editor app with a shared `EditorContext`.
- A `ModeManager` routes input/UI/viewport to the active `IMode`.
- Modes (in development order):
  1) Modeling
  2) Rigging
  3) Animation (two sub-modes: Manual + Procedural)

### VR Later (Editor-Only)
- Plan to add VR as **an alternate view/input mode** for the editor (not a forked app).
- The shipped game does not need VR.

---

## 4) Asset Workflow (Unreal-like “Static Mesh” Instancing)

### Core UX Goal
- Models created in the modeling tool become **assets** that can be **instanced in-game easily**:
  - Content browser shows mesh assets
  - Drag/drop (or equivalent) places instances into the level

### Data Model
- **MeshAsset**: serialized asset (identity + geometry + metadata).
- **MeshInstance**: lightweight level entity referencing a MeshAsset handle + transform (and later material).
- The level never “owns” raw vertex arrays; it owns **references**.

### Streaming-Friendly
- Instances can exist before assets are loaded.
- If asset isn’t ready: render placeholder and swap seamlessly when streamed.

---

## 5) Runtime / Rendering Separation

### Editable vs Render Mesh (Strict Boundary)
- `EditableMesh` (CPU/editor):
  - topology + edit state (selection, operations)
  - dirty flags
  - **no D3D12 types**
- `RenderMesh` (GPU/runtime-ish):
  - GPU buffers/resources + update strategy
  - **no editing logic**
- Allowed dependency direction:
  - `EditableMesh → RenderMesh` (via an explicit “sync/update” step)

### Per-Frame Sync Pattern
- Input gathered (WindowProc is dumb)
- Editing happens in `Update()`
- If dirty: `RenderMesh.UpdateFrom(EditableMesh)`
- **Editor decides when to clear dirty**, not RenderMesh

---

## 6) Animation Direction (Unified “Tracks”)

- Keyframe animation and procedural animation are treated the same:
  - time-parameterized function tracks
  - supports blending/layering/LODs
  - can compile glTF animation into minimal runtime evaluators
- Two editor modes:
  - Manual animation (authoring keyframes)
  - Procedural animation (ControlRig-lite)

---

## 7) Differential Geometry & ML Hooks (Later, Minimal Refactors)

### Differential Geometry (DG)
- Keep hooks for:
  - cotangent Laplacian / Laplace–Beltrami
  - geodesics / heat method distances
  - parallel transport frames / stable tangents
  - intrinsic UV-free “spectral texture” ideas (LB eigenfunctions as Fourier-like basis)
- DG features should be optional modules, not core blockers.

### ML
- Prefer offline/editor-first ML, optional tiny runtime.
- Desired gameplay ML feature: telemetry-driven “stuck episode” detector (no-progress + backtracking + idle scanning).

### Multiplayer
- Not required now; keep architecture clean enough to add later without rewrites.

---

## 8) Project Workflow

- Build as small, disposable prototypes that feed reusable modules:
  - e.g., `modelingtool_proto` = thin host app + reusable core module
- Integrate into the main engine only after the core seam is stable.

---

## 9) Coding Conventions (Project Preference)

- Brace style: **left brace on same line** (K&R style)
  - `if (x) { ... }`
  - Not:
    ```
    if (x)
    {
      ...
    }
    ```

---

## 10) Definition of “Done” for v0

- You can:
  - create/edit a low-poly mesh in the modeling mode
  - save it as a MeshAsset
  - see it in a content browser list
  - place multiple instances into a level
  - press Play and **start instantly**
  - watch placeholders stream into final meshes seamlessly
