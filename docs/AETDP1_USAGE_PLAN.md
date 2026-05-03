# AETDP1 Usage Plan for Another Engine

AETDP1 stands for **Another Engine Top-Down Reference Prototype 1**.

## Status

AETDP1, short for **Another Engine Top-Down Reference Prototype 1**, is a generated reference prototype created from an AI prompt. It is not the official Another Engine codebase, and it should not become the source of truth for AEDD development.

Use AETDP1 as a study artifact and pattern library. Translate ideas manually into AE only when they fit the current baseline, current milestone, and current code structure.

## Purpose

AETDP1 is useful because it demonstrates several editor-shaped boundaries that are relevant to Another Engine:

- stable object identity
- scene data separated from selection state
- command-shaped editor operations
- undo/redo boundaries
- viewport picking as a query service
- gizmo interaction separated from scene mutation policy
- serialization isolated from UI and rendering
- animation data separated from timeline UI and evaluator logic
- documentation that explains ownership, interaction traces, and reading order

These ideas support AE's long-term goal of being legible, controllable, and verifiable by external AI tools.

The long-term AI direction is:

```text
Human prompt
-> AI gameplay/storyline plan
-> structured AE command JSON
-> editor-applied changes
-> validation report
-> human review
-> playable sequence
```

AETDP1 does not implement that full pipeline, but it gives us several useful low-level patterns for building toward it.

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

## Recommended Repo Placement

Keep AETDP1 material in a clearly marked reference area, for example:

```text
docs/experiments/AETDP1_USAGE_PLAN.md
```

If the generated prototype itself is committed, place it somewhere clearly separated from official engine code, for example:

```text
experiments/AETDP1/
```

or:

```text
reference/AETDP1/
```

Official AE development should continue from the current AE baseline.

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

Scene data should own objects and their authoring data.

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
One meaningful editor action should have one meaningful operation name.
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
```

## How AETDP1 Affects Current v0.0.2.2 Development

The current AE v0.0.2.2 path should remain focused on the runtime host spine and current editor/runtime separation work.

AETDP1 changes the pressure slightly:

```text
Build the current features in a way that can later be commanded, queried, serialized, validated, and reviewed.
```

For v0.0.2.2, this means:

1. Keep current AE baseline as source of truth.
2. Avoid broad refactors.
3. Prefer stable object identity when SceneObject structure is touched.
4. Avoid making UI directly own permanent scene mutation policy.
5. Keep editor-only state separate from runtime/game state.
6. Keep save/load and scene mutation logic isolated enough to evolve later.
7. Keep the runtime host compatible with future PIE-style preview and validation runs.

## Near-Term AE Plan Influenced by AETDP1

### Phase 1 — Current Baseline Continuation

Continue v0.0.2.2 against the existing AE codebase.

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

## AETDP1 Files Worth Studying

Highest value reading order:

```text
Cursor/AE_CURSOR_01/ARCHITECTURE_NOTES.md
Cursor/AE_CURSOR_01/EXTRACTABLE_PATTERNS.md
Cursor/AE_CURSOR_01/TOP_10_PORTING_IDEAS.md
Cursor/AE_CURSOR_01/INTERACTION_TRACES.md
Cursor/AE_CURSOR_01/READING_ORDER.md
Cursor/AE_CURSOR_01/src/core/types.h
Cursor/AE_CURSOR_01/src/scene/scene_data.h
Cursor/AE_CURSOR_01/src/selection/selection_state.h
Cursor/AE_CURSOR_01/src/modeling/modeling_commands.h
Cursor/AE_CURSOR_01/src/commands/command_undo.h
Cursor/AE_CURSOR_01/src/viewport/viewport_interaction.h
Cursor/AE_CURSOR_01/src/gizmo/translate_gizmo.h
Cursor/AE_CURSOR_01/src/serialization/serialization.h
```

Study these for concepts, ownership boundaries, and interaction flow.

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

## Practical Rule for Future Dev Sessions

When continuing AE development, AETDP1 should be referenced like this:

```text
AETDP1 suggests a useful pattern here. We will translate the pattern manually into the current AE baseline with a small explicit diff.
```

AETDP1 should not be referenced like this:

```text
Copy these files into AE.
Replace current AE architecture.
Adopt the generated project wholesale.
```

The value is the architecture signal, not the generated code itself.

## Summary

AETDP1 is useful because it gives AE a concrete reference for editor architecture that supports the long-term AI-authoring direction.

The strongest ideas to carry forward are:

```text
stable object identity
command-shaped edits
selection/active-object split
pure picking queries
gizmo-command separation
serialization boundary
documentation for ownership and interaction flow
```

Use these ideas slowly and surgically as AE grows.

The official path remains:

```text
current AE baseline
-> v0.0.2.2 runtime/editor spine
-> stronger SceneObject identity
-> command-shaped editor operations
-> metadata/tags
-> JSON command artifacts
-> query surface
-> validation reports
-> AI-assisted gameplay/storyline composition
```
