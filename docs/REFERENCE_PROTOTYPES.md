# Reference Prototypes

Updated: 2026-05-19

This document records lessons from Cursor/Codex reference prototypes created around Another Engine. These prototypes are **study artifacts and idea mines**, not merge candidates, unless explicitly promoted later.

The purpose of this document is to preserve durable lessons across chats and development sessions so AE can harvest useful ideas without accidentally absorbing toy-code shortcuts.

## Rules for Using Reference Prototypes

1. **Do not port toy code directly by default.** Translate ideas manually into AE’s real architecture.
2. **Extract concepts before implementation.** The valuable output is usually a data model, ownership boundary, UI flow, validation rule, or sequencing pattern.
3. **Separate durable lessons from disposable glue.** A toy may demonstrate a correct architectural shape while still using unsafe shortcuts to stay small.
4. **Keep AE’s source of truth clean.** Reference prototypes may live in an experiments repo or under `prototypes/`, but AE’s official systems should evolve from deliberate manual implementation.
5. **Record “do not port directly” notes.** Prototype mistakes are useful if they prevent similar mistakes in AE.
6. **Prefer repo docs over memory for exact findings.** Memory should preserve durable direction; this file should preserve concrete findings, source artifacts, risks, and reading order.

---

# Prototype Index

| Prototype | Source Artifact | Primary Lesson Area | Status |
|---|---|---|---|
| AETDP1 | `AE_CURSOR_01.7z` | Top-down editor architecture, command spine, viewport/gizmo/timeline/serialization boundaries, AI-legible editor surfaces | Reference only |
| AERIGP1 | `AE_CURSOR_02.7z` | Rigging, skeletons, skinning, skeletal animation, IK, rig validation | Reference only |

---

# AETDP1 — Another Engine Top-Down Reference Prototype 1

AETDP1 stands for **Another Engine Top-Down Reference Prototype 1**.

## Status

AETDP1 is a generated reference prototype created from an AI prompt. It is not the official Another Engine codebase, and it should not become the source of truth for AEDD development.

Use AETDP1 as a study artifact and pattern library. Translate ideas manually into AE only when they fit the current baseline, current milestone, and current code structure.

## Purpose

AETDP1 is a standalone D3D12 + Dear ImGui top-down/editor reference prototype. It is useful for studying how a small editor-like application can separate scene data, selection, viewport interaction, gizmos, commands, animation tracks, timeline UI, and serialization.

It demonstrates several editor-shaped boundaries that are relevant to AE:

```text
stable object identity
scene data separated from selection state
command-shaped editor operations
undo/redo boundaries
viewport picking as a query service
gizmo interaction separated from scene mutation policy
serialization isolated from UI and rendering
animation data separated from timeline UI and evaluator logic
documentation that explains ownership, interaction traces, and reading order
```

These ideas support AE's long-term goal of being legible, controllable, and verifiable by external AI tools.

The long-term AI direction is:

```text
human prompt
-> AI gameplay/storyline plan
-> structured AE command JSON
-> editor-applied changes
-> validation report
-> human review
-> playable sequence
```

AETDP1 does not implement that full pipeline, but it gives AE useful low-level patterns for building toward it.

## Core Decision

AETDP1 should influence AE architecture, not replace AE architecture.

When a useful pattern exists in AETDP1, the process should be:

```text
study pattern
-> compare against current AE baseline
-> design smallest AE-native version
-> implement as a surgical diff
-> test locally
-> update AE docs/state
```

Do not bulk-port AETDP1 files into AE.

## Recommended Placement

If the generated prototype itself is committed, place it somewhere clearly separated from official engine code, for example:

```text
experiments/AETDP1/
```

or:

```text
reference/AETDP1/
```

Official AE development should continue from the current AE baseline.

The useful lessons from AETDP1 belong in this document, not in a separate long-term usage-plan document.

## Noteworthy Files

| File / Folder | Why It Matters |
|---|---|
| `src/app/bootstrap_main.cpp` | Shows bootstrap/main-loop ordering for Win32, D3D12, ImGui, playback tick, UI, and rendering. |
| `src/app/app_state.h` | Shows a compact session object tying the toy together. Useful as a study object, but too god-object-like for AE. |
| `src/scene/scene_data.*` | Small authoring-time scene object model: id, name, transform, color, local box data. |
| `src/selection/selection_state.*` | Separates selection set from active object. This is a strong AE lesson. |
| `src/viewport/viewport_interaction.*` | Keeps camera and ray picking as a viewport/geometric concern. |
| `src/gizmo/translate_gizmo.*` | Separates gizmo hover/active state and mouse-delta-to-axis-motion mapping from command history. |
| `src/commands/command_undo.*` | Demonstrates a semantic command boundary for create/delete/duplicate/select/transform/key edits. |
| `src/animation/animation_data.*` | Demonstrates transform tracks and sorted keyframe storage. |
| `src/animation/animation_eval.*` | Demonstrates evaluation separate from animation storage, but with prototype shortcuts. |
| `src/timeline/timeline_ui.*` | Shows timeline transport/scrub/key UI separate from evaluator and data storage. |
| `src/serialization/serialization.*` | Keeps save/load grammar isolated from UI, viewport, and renderer. |
| `ARCHITECTURE_NOTES.md` | Best high-level explanation of subsystem ownership and prototype glue. |
| `EXTRACTABLE_PATTERNS.md` | Good summary of patterns worth studying and risks not to copy blindly. |
| `TOP_10_PORTING_IDEAS.md` | Useful ranking of what to manually translate later. |
| `INTERACTION_TRACES.md` | Useful for studying editor action flow from input to state change. |
| `READING_ORDER.md` | Good guide for studying the prototype without getting lost. |

## Highest-Value Patterns to Carry Forward

### 1. Stable Scene Object Identity

AETDP1's `SceneData` uses objects with stable ids and names.

AE should move toward scene objects that have:

```cpp
uint32_t id;
std::string name;
Transform transform;
```

Future fields can include:

```cpp
std::vector<std::string> tags;
std::string role;
std::string sourceType;
```

This matters because future AI commands must refer to objects reliably.

Good future command shape:

```json
{
  "type": "SetObjectTransform",
  "object": "weak_wall_01",
  "position": [3.0, 0.0, 8.0]
}
```

Fragile command shape:

```json
{
  "type": "SetObjectTransform",
  "objectIndex": 3,
  "position": [3.0, 0.0, 8.0]
}
```

Indices are useful internally, but object identity should not depend on array position.

### 2. Scene Data, Selection State, and Editor State Separation

AETDP1 separates:

```text
SceneData
SelectionState
EditorState
```

AE should preserve this separation as the editor grows.

Scene data should own objects and authoring data.

Selection state should own selected ids and active object.

Editor state should own current tool mode, UI toggles, viewport/editor-only flags, and temporary interaction state.

This separation will help future systems query the editor cleanly.

Example future AI query:

```json
{
  "type": "GetSelection"
}
```

Possible response:

```json
{
  "selected": ["door_01", "switch_01"],
  "active": "door_01"
}
```

### 3. Command-Shaped Editor Operations

AETDP1 routes core edits through semantic commands.

AE should gradually move editor actions toward this shape:

```text
UI/input
-> command-shaped operation
-> scene mutation
-> undo/redo later
-> JSON import/export later
```

Near-term AE command candidates:

```text
CreateObject
DuplicateObject
DeleteObject
SelectObject
SetObjectTransform
SetObjectColor
CreatePrimitive
```

Later modeling command candidates:

```text
SelectFace
SelectEdge
MoveVertex
MoveFace
ExtrudeFace
DeleteFace
SplitFace
```

The important design rule is:

```text
one meaningful editor action should have one meaningful operation name
```

That operation name is what later becomes usable by undo/redo, scripting, tests, and AI command artifacts.

### 4. Viewport Picking as a Pure Query

AETDP1 treats picking as a service that reads scene/camera data and returns a result.

AE should keep picking separate from mutation.

Good shape:

```text
mouse position + camera + scene
-> pick query
-> pick result
-> editor decides command
```

This matters because future systems may need the same query without a mouse:

```json
{
  "type": "PickNearestObject",
  "rayOrigin": [0.0, 1.5, -5.0],
  "rayDirection": [0.0, 0.0, 1.0]
}
```

Near-term AE can still use simple picking. Pixel-perfect picking is deferred.

### 5. Gizmo Interaction Split

AETDP1 separates gizmo interaction from command policy.

AE should keep this direction:

```text
Gizmo math:
- hover axis
- active axis
- ray/axis hit testing
- drag delta calculation

Editor command policy:
- when to start preview
- when to commit transform
- how undo should capture before/after state
```

This will matter when AE has:

```text
simple vertex/edge gizmo
full object transform gizmo
rotate gizmo
scale gizmo
VR/mobile/editor input variants
```

### 6. Serialization Boundary

AETDP1 has serialization as its own module.

AE should keep save/load knowledge out of UI, renderer, and gizmo code.

Near-term `.aem` text save/load is still fine for v0.0.2-era development, but it should be metadata-friendly:

```text
object id
object name
primitive/source type
transform
color
future tags
future components
```

For AI-facing artifacts, AE should use JSON as the canonical structured command format.

There can eventually be multiple artifact types:

```text
.aem scene files
.json command batches
.json validation reports
.json gameplay sequence descriptions
```

### 7. Documentation as an AI/Learning Surface

AETDP1 includes useful documentation types:

```text
ARCHITECTURE_NOTES.md
FILE_OWNERSHIP.md
FUNCTION_INDEX.md
INTERACTION_TRACES.md
READING_ORDER.md
EXTRACTABLE_PATTERNS.md
```

AE should eventually maintain similar docs for its real systems.

These documents help the user understand the code, and they also make the engine easier for AI tools to reason about without guessing.

Recommended AE docs over time:

```text
docs/PROJECT_STATE.md
docs/ARCHITECTURE_NOTES.md
docs/FILE_OWNERSHIP.md
docs/COMMANDS.md
docs/SCENE_FORMAT.md
docs/INTERACTION_TRACES.md
docs/AI_AUTHORING_SURFACE.md
docs/REFERENCE_PROTOTYPES.md
```

## How AETDP1 Affects Current AE Development

AETDP1 should change AE development pressure in one specific way:

```text
Build current features so they can later be commanded, queried, serialized, validated, and reviewed.
```

For current v0.0.2-era and v0.0.2.2-era work, this means:

1. Keep current AE baseline as source of truth.
2. Avoid broad refactors.
3. Prefer stable object identity when `SceneObject` structure is touched.
4. Avoid making UI directly own permanent scene mutation policy.
5. Keep editor-only state separate from runtime/game state.
6. Keep save/load and scene mutation logic isolated enough to evolve later.
7. Keep the runtime host compatible with future PIE-style preview and validation runs.

## Near-Term AE Plan Influenced by AETDP1

### Phase 1 — Current Baseline Continuation

Continue current AE development against the existing codebase.

Use AETDP1 only as a reference while implementing:

```text
runtime host spine
editor/runtime separation
scene object structure cleanup
save/load support
transform gizmo behavior
```

### Phase 2 — SceneObject Identity

When AE's `SceneObject` structure is formalized, include stable identity early:

```cpp
struct SceneObject {
    uint32_t id;
    std::string name;
    Transform transform;
    EditableMesh editableMesh;
    RenderMesh renderMesh;
    DirectX::XMFLOAT3 color;
};
```

Names can start simple:

```text
Tetra_001
Tetra_002
Tetra_003
```

IDs should be generated by the scene, not by UI code.

### Phase 3 — Command Spine

Introduce a small AE-native command shape when it fits naturally.

Start with object-level commands:

```text
CreateObjectCommand
DeleteObjectCommand
DuplicateObjectCommand
SetObjectTransformCommand
SelectObjectCommand
```

The first version does not need a perfect undo/redo architecture. The important thing is to give semantic editor operations a stable shape.

### Phase 4 — Metadata and Tags

After object identity and save/load are stable, add metadata support.

Minimum useful concept:

```text
tags: ["obstacle", "mechanic_target", "fog_anchor"]
```

This becomes important for future AI gameplay/storyline composition.

Future AI query examples:

```json
{
  "type": "FindObjectsByTag",
  "tag": "mechanic_target"
}
```

```json
{
  "type": "ListAvailableMechanics"
}
```

### Phase 5 — JSON Command Artifacts

Once commands exist internally, allow command batches to be represented as JSON.

Example:

```json
{
  "commands": [
    {
      "type": "CreateObject",
      "name": "weak_wall_01",
      "primitive": "tetra",
      "position": [4.0, 0.0, 7.0]
    },
    {
      "type": "SetObjectTags",
      "object": "weak_wall_01",
      "tags": ["obstacle", "weak_wall", "mechanic_target"]
    }
  ]
}
```

This becomes the foundation for AI-assisted authoring.

### Phase 6 — Query and Validation Surface

After command artifacts exist, expose query and validation functionality.

Useful future queries:

```text
ListObjects
FindObjectByName
FindObjectsByTag
ListMechanics
ListSequences
GetSelection
GetSceneBounds
FindOpenArea
```

Useful future validation checks:

```text
missing referenced object
duplicate object name
unreachable goal
trigger points to missing sequence
sequence locks player control and never restores it
mechanic requires target but target is missing
camera shot points away from required story beat
```

Validation is what turns AI output from random generation into reviewable authoring.

## Translation Checklist

Before using any AETDP1 idea in AE, check:

```text
Does this serve the current milestone?
Does this fit the current AE baseline?
Can it be implemented as a small diff?
Does it preserve the user's understanding of the code?
Does it avoid broad framework drift?
Does it help future command/query/validation surfaces?
Does it keep official AE code separate from reference prototype code?
```

## AI-Future-Proofing Rule Added by AETDP1

Every new editor/runtime system should be evaluated with this question:

```text
Could an external tool understand, query, command, validate, and review this system without guessing?
```

This does not mean adding AI features now.

It means shaping AE's foundations so that future AI gameplay/storyline authoring becomes possible without rewriting the editor.

## Ideas to Port Later

- Selection set plus active object model.
- Semantic command spine for object/modeling/animation edits.
- Viewport picking as a pure geometric/query service.
- Separate gizmo interaction state from command commit logic.
- Separate animation track data from timeline UI and evaluator.
- Serialization module boundary.
- Stable object identity with ids, names, and eventually tags.
- Metadata/query/validation surfaces for future AI-assisted authoring.
- A reading-order/documentation style for future internal AE prototypes.

## Do Not Port Directly

- `AppState` as a long-term architecture. It is useful toy glue, but AE needs clearer editor/session/core boundaries.
- Evaluator writing directly into `SceneData` transforms. This conflates authored transforms and evaluated/preview transforms.
- CPU-rebuilt mesh upload every frame as a renderer strategy.
- AABB-only picking as the final picking architecture.
- Inspector live-edit plus manual “commit” as the final property transaction model.
- Timeline scrub directly mutating time state as a full undo model.
- The exact text format `AETDP1_TEXT_V1`.

## Integration Risks Revealed

- If animation evaluation writes directly into authored scene data, AE will lose the distinction between authored state, preview state, and runtime/evaluated state.
- If gizmo drag previews are not transaction-aware, undo history can flood or become semantically confusing.
- If selection mutation is mixed into viewport picking, future editor hosts and AI-command surfaces will be harder to support.
- If serialization knowledge leaks into UI, file format changes will become expensive.
- If object identity depends on array index, future command artifacts and AI-authored edits will become fragile.
- If metadata/tags are postponed too long, later validation and AI authoring surfaces will require more refactoring.

## Practical Rule for Future Dev Sessions

When continuing AE development, AETDP1 should be referenced like this:

```text
AETDP1 suggests a useful pattern here. Translate the pattern manually into the current AE baseline with a small explicit diff.
```

AETDP1 should not be referenced like this:

```text
Copy these files into AE.
Replace current AE architecture.
Adopt the generated project wholesale.
```

The value is the architecture signal, not the generated code itself.

## Memory-Grade Lessons

- AE should keep scene, selection, viewport, gizmo, commands, animation data, animation evaluation, timeline UI, and serialization as distinct concerns.
- AE should separate selected set from active object.
- AE should route meaningful edits through semantic commands.
- AE should treat viewport picking as a query/intent service, not a direct mutation path.
- AE should keep authored state separate from evaluated animation/preview state.
- AE should move toward stable object identity using ids and names, with tags/metadata added later.
- AE should shape current editor/runtime systems so they can later be commanded, queried, serialized, validated, and reviewed.
- AE should not implement AI authoring now, but should keep systems legible enough that AI authoring can be added later without rewriting the editor.

---

# AERIGP1 — Another Engine Rig/Animation Prototype 1

## Purpose

AERIGP1 is a standalone D3D12 + Dear ImGui rigging and skeletal-animation reference prototype. It demonstrates a full small spine for:

```text
procedural character mesh
skeleton hierarchy
bind pose
inverse bind matrices
skin weights
CPU skinning
local-pose editing
24 FPS skeletal animation
keyframe tracks
simple 2-bone IK
rig/skin/animate UI modes
rig and animation save/load
validation
```

It should be treated as a **rigging/animation study toy**, not as AE’s real rig editor implementation.

## Noteworthy Files

| File / Folder | Why It Matters |
|---|---|
| `src/rig_types.h` | Central data model: `RigDocument`, `Mesh`, `Skeleton`, `Joint`, `SkinWeight`, `AnimClip`, `AnimTrack`, `AnimKey`. |
| `src/transform.*` | Transform helpers and Euler/quaternion conversion. |
| `src/skeleton.*` | Hierarchy updates, world pose/bind transforms, inverse bind matrix recomputation, cycle checks. |
| `src/skinning.*` | CPU skinning and auto-weight reference. Best file to study for first-principles skin deformation. |
| `src/animation.*` | 24 FPS clip/keyframe evaluation. Good starting concept for AE animation evaluator. |
| `src/ik_two_bone.*` | Small readable two-bone IK teaching implementation. |
| `src/procedural_character.*` | Procedural low-poly humanoid, default skeleton, sample clip. Useful as future test geometry. |
| `src/rig_validator.*` | Checks hierarchy, weights, and animation tracks. Strong AE lesson. |
| `src/rig_serializer.*` | Separate `.aerig` and `.aeanim` text formats. Useful schema study. |
| `src/rig_ui.*` | ImGui Rig/Skin/Animate panels and workflow ideas. |
| `README_AERIGP1.md` | Explains build, controls, file formats, CPU skinning, and animation evaluation. |
| `INTEGRATION_NOTES.md` | Best high-level summary of what to port conceptually into AE later. |

## Durable Lessons for AE

### 1. Rigging needs distinct data concepts

AERIGP1 usefully separates:

```text
Mesh
Skeleton
Joint
SkinWeight
Pose
AnimClip
AnimTrack
AnimKey
RigDocument
Validator
```

AE should preserve this conceptual separation even if the final asset system uses different names or storage.

### 2. Joint data should carry bind and pose information separately

AERIGP1’s joint model includes:

```text
parent index
local bind transform
local pose transform
world bind transform
world pose transform
inverse bind matrix
```

This is the core skeleton pipeline AE needs. The exact struct layout can change, but AE should keep the conceptual distinction between bind pose and current/evaluated pose.

### 3. CPU skinning is the correct first reference path

AERIGP1’s CPU skinning path is valuable as a correctness reference:

```text
skin_matrix[j] = inverse_bind[j] * world_pose[j]
vertex_position = weighted sum of skin_matrix[j] * bind_position
vertex_normal = weighted sum of skin_matrix[j] * bind_normal
```

For AE, CPU skinning should be the first implementation or at least the golden reference for later GPU skinning tests.

### 4. Animation clips should key local joint transforms

The prototype stores animation as per-joint tracks with frame keys containing local transforms. This is the right simple starting point:

```text
clip
  track per joint
    key frame -> local transform
```

AE’s first animation evaluator should keep this simple before adding curves, layers, masks, retargeting, compression, or blend trees.

### 5. AE should use 24 FPS as a first-class animation/editor assumption

AERIGP1 follows the project requirement that Dark Defiance runs at 24 FPS. AE’s first animation tools should make 24 FPS visible and intentional rather than an afterthought.

### 6. Validation should be first-class in rigging

AERIGP1 validates things like:

```text
invalid parent indices
hierarchy cycles
missing root
bad or unnormalized weights
vertices with no weights
animation tracks targeting invalid joints
```

AE should build rig validation early. Rigging tools silently corrupt data if validation is delayed.

### 7. Split rig assets from animation assets

AERIGP1 uses separate conceptual files:

```text
.aerig  -> mesh + skeleton + bind pose + skin weights
.aeanim -> clip + frame rate + frame count + joint tracks + keys
```

This split is worth preserving conceptually. AE’s actual format may become JSON or another AE-owned asset format, but rig and animation data should not be unnecessarily fused.

### 8. A simple 2-bone IK toy is useful for understanding, not final tooling

The two-bone IK implementation is good for learning arm/leg chain solving. AE will eventually need cleaner constraints, pole vectors, editor handles, undo, and validation. The toy is still valuable because it removes the aura around IK.

## Ideas to Port Later

- Skeleton data model with parent indices, local bind, local pose, world bind, world pose, inverse bind.
- CPU skinning as a correctness reference.
- Per-joint local-transform animation tracks at 24 FPS.
- Separate `.rig`-like and `.anim`-like asset concepts.
- Rig validation panel and save-time validation.
- Procedural humanoid as test geometry for early rigging unit tests.
- Weight debug coloring as an editor visualization mode.
- Simple 2-bone IK as a learning/reference implementation.

## Do Not Port Directly

- Single `RigDocument` god-object as final AE architecture. AE should split assets, scene instances, editor session state, and runtime evaluation state.
- ImGui panel layout as final UI architecture.
- Standalone D3D12 renderer path. AE should use its own renderer/debug-draw infrastructure.
- Auto-weighting as final skinning workflow. It is a seed, not a production weight-painting system.
- Text file grammar as the final asset format.
- Toy deletion/reparent behavior without stronger command and validation rules.

## Specific Caveats Found During Review

These do not need to be fixed if AERIGP1 remains a reference toy, but they are worth remembering so AE avoids the same mistakes.

### Skeleton update ordering

The skeleton world-transform update appears to assume parents appear before children in the joint array. This works for the default skeleton but becomes fragile after arbitrary reparenting. AE should use a hierarchy traversal or topological update.

### Joint deletion can corrupt dependent data

Deleting joints must update or invalidate:

```text
skin weights
animation tracks
IK chain references
selection state
child parent indices
```

AE should not implement joint deletion as a raw vector erase without a full dependency update/validation step.

### Animation evaluation should reset unkeyed joints

A safe animation evaluator should start from bind/reference pose, then apply keyed tracks. Otherwise old manual pose edits can leak into playback for unkeyed joints.

### Bind pose versus current pose must be explicit

The prototype exposes pose editing and bind-pose recomputation in a way that can confuse “current pose” with “new bind pose.” AE should make this an explicit command/workflow:

```text
Edit pose
Reset pose
Apply current pose as bind pose
Recompute inverse bind matrices
```

### Auto-weighting should use bind pose

Weight generation should usually use bind/reference skeleton positions, not current animated or posed positions.

### Weight debug rendering should preserve per-vertex information

Weight debug views should preserve per-vertex colors or another precise visualization. Triangle-level flattening can hide useful weight information.

## Integration Risks Revealed

- Rigging systems become fragile if bind pose, current edit pose, evaluated animation pose, and runtime pose are not distinct.
- Skeleton hierarchy edits require dependency-aware commands, not raw array mutation.
- Skin weights and animation tracks are tightly coupled to joint identity; joint renaming/reordering/deletion must be handled carefully.
- Animation tools should be built on a clean evaluator before adding UI complexity.
- GPU skinning should wait until CPU skinning proves the math and gives AE a test oracle.

## Memory-Grade Lessons

- AE rigging should separate Skeleton, Pose, Skin, AnimClip, and Validator concepts.
- AE should use CPU skinning as a first correctness reference before GPU skinning.
- AE should make rig validation first-class early.
- AE should store skeletal animation as local joint transform tracks at 24 FPS first.
- AE must keep bind pose, authored pose, evaluated animation pose, and runtime pose conceptually separate.

---

# Cross-Prototype Lessons

## 1. Reference toys are most valuable when they expose boundaries

Both AETDP1 and AERIGP1 are useful because they expose system boundaries:

```text
AETDP1: scene / selection / commands / viewport / gizmo / animation / serialization / AI-legible editor surfaces
AERIGP1: skeleton / pose / skin / animation clip / evaluator / validator
```

The exact code matters less than the separation of responsibilities.

## 2. AE should prefer legible command/query surfaces

Both prototypes point toward an AE architecture where editor actions are expressed as meaningful operations rather than hidden UI mutations. This supports:

```text
undo/redo
human review
AI-generated command artifacts
validation
serialization
future automation
```

## 3. Authoring state and evaluated state must be separate

Both prototypes reveal the same risk in different ways:

```text
AETDP1: animation evaluator writes into scene transforms
AERIGP1: pose/bind/evaluation can blur if not made explicit
```

AE should deliberately separate authored asset data, editor preview state, evaluated pose/state, and runtime state.

## 4. Validation should grow alongside tools

Reference prototypes are small enough to tolerate corrupt states. AE is not. As AE adds Modeling, Rigging, Animation, and Sequencer features, each mode should add validation early.

## 5. Documentation is part of the artifact

AETDP1’s architecture notes, reading order, file ownership notes, and extractable-patterns docs are almost as valuable as the code. For future prototypes, request these docs intentionally.

## 6. AI-future-proofing means legibility first

The near-term goal is not to build AI features into AE immediately. The goal is to keep AE systems easy for an external tool or future AI workflow to understand, query, command, validate, and review.

---

# Future Prototype Review Template

Use this template whenever a new Cursor/Codex toy is reviewed.

```text
Prototype:
Source artifact:
Purpose:
Status:

Useful lessons:
- 

Files worth studying:
- 

Ideas to port later:
- 

Ideas not to port:
- 

Risks / incorrect assumptions:
- 

How it affects AE architecture:
- 

Memory-grade lessons:
- 
```

---

# Current Standing Decision

AETDP1 and AERIGP1 should remain **reference prototypes**. They are useful for studying and extracting lessons, but they should not become AE baselines.

For AE development, use these prototypes to inform manual implementation when the real project reaches the relevant subsystem:

```text
AETDP1 -> editor command/selection/viewport/gizmo/timeline/serialization architecture and AI-legible command/query/validation surfaces
AERIGP1 -> rigging/skinning/skeletal-animation/IK/validation architecture
```

The practical rule remains:

```text
reference prototype
-> extract lesson
-> compare to current AE baseline
-> implement smallest AE-native version
-> validate
-> document
```
