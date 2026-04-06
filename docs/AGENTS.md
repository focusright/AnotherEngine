# AEDD AGENTS GUIDE

This document describes how automated helpers, coding assistants, and future tool-facing workflows should operate inside this repository.

## Project Identity
AEDD means **Another Engine Dark Defiance**.

Another Engine is the engine/editor/tooling codebase.
Dark Defiance is the game that will later depend on it.

## Primary Mission
Build the engine and game stack in a way that preserves deep understanding of the whole system.

This repo is not just about getting output quickly. It is an apprenticeship engine. Changes should protect clarity, legibility, and architectural understanding.

## Core Development Rules
- Prefer small, surgical diffs over broad churn.
- Preserve current behavior unless behavior change is explicitly requested.
- Do not invent APIs, files, structures, or methods that do not exist in the actual baseline.
- Do not silently move the project into a larger refactor than requested.
- Do not introduce speculative abstractions early.
- Do not widen scope when the current version has a defined objective.

## Versioning Discipline
- v0.0.2.1 is the AE structure migration pass.
- v0.0.2.2 is the runtime spine bootstrap pass.
- v0.0.3 is reserved for later modeling and structural refactor work already discussed elsewhere.

Do not blur these phases together.

## Repository Structure Intent
- `editor/` contains editor-facing application code.
- `editor/modes/modeling/` contains current modeling mode data structures and mode-specific pieces.
- `engine/core/` contains core runtime-side engine code.
- `engine/gfx/` contains graphics-device and D3D12 setup code.
- `third_party/` contains vendored dependencies.
- `build/vs2022/` contains Visual Studio build metadata.
- `assets/` contains content files used by the executable.
- `docs/` contains developer-facing project documentation.

## Build and Runtime Path Rule
Do not rely on the process working directory for runtime asset paths.

Default asset and scene paths should resolve relative to the executable directory.

For the current baseline, the default scene should resolve to:

```text
<exe folder>/assets/scenes/scene.aem
```

When adding new default asset paths, follow the same rule.

## Documentation Rule
Whenever files are moved, renamed, or rehomed, update docs that mention paths.

At minimum, check:
- `docs/README_DEV.md`
- `docs/PROJECT_STATE.md`
- `docs/AGENTS.md`

Do not leave documentation referencing obsolete locations after a migration pass.

## Behavior-Safe Refactoring Rule
During structure passes:
- allow compile fixes
- allow build metadata fixes
- allow asset path fixes
- avoid behavior changes beyond what is necessary to preserve the existing baseline

## Windows-Only Rule
This engine is Windows-only.

Do not introduce cross-platform abstractions unless explicitly requested for a later reason. Toolchain separation such as `build/vs2022/` exists to separate source from Visual Studio metadata, not to imply multi-platform support.

## Asset Handling Rule
Content files do not belong under `prototypes/`.

When deciding where files belong:
- code goes under `editor/`, `engine/`, or `third_party/`
- build metadata goes under `build/`
- content goes under `assets/`
- documentation goes under `docs/`

## Assistant Conduct Rule
When helping with code changes:
- provide exact manual edits
- align with the current repo baseline
- preserve existing comments where reasonable
- keep code style consistent with the current project
- avoid zip-only answers
- explain changes clearly enough that the user can apply them manually

## Completion Rule For Migration Passes
A structure migration pass is not complete until:
- file locations match their intended ownership
- build files live in a real build folder
- asset paths are stable
- Visual Studio project metadata matches the real tree
- docs match reality
