# PROJECT_STATE.md

## Current Version: v0.0.2

## Completed

- Tetrahedron primitive
- Object selection
- Face highlight
- Mouse-driven translate gizmo (world space)
- Scene save/load (.aem)
- Minimal Dear ImGui integration
- Object list
- Add/Duplicate/Delete
- Transform editing
- Baseline object transform workflow for translation

---

## Remaining Tasks for v0.0.2

1. Editor command spine
2. Object rotate/scale gizmo

---

## Current Focus

Implement the remaining two v0.0.2 tasks only:

- Editor command spine
- Object rotate/scale gizmo

Do not expand scope beyond these until v0.0.2 is complete.

---

## Notes

- The older note about “fix behavioral bugs in mouse-driven world-space translate gizmo” is no longer the source of truth.
- The locked source of truth is the later dev agreement that only two tasks remain in v0.0.2.
- Keep all work surgical and grounded in the current baseline.
- No large refactors unless directly required to support one of the two remaining tasks.

---

## Next Recommended Step

Start with the editor command spine, since it gives a clean control surface for editor actions and will help support the rotate/scale gizmo cleanly.