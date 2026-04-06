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

## Build and Runtime Path Rule
Do not rely on the process working directory for runtime asset paths.

Default asset and scene paths should resolve relative to the executable directory.

For the current baseline, the default scene should resolve to:

```text
<exe folder>/assets/scenes/scene.aem