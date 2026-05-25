# AEDD PROJECT STATE

## Current Version
v0.0.2.2 - runtime spine bootstrap

## Status
Complete.

## Current Focus
AE now has a shared host runner that owns the outer editor frame loop and high-resolution timing. The AE editor updates with variable `dt`; fixed 24 FPS stepping remains a future DD runtime policy.

## v0.0.2.2 Outcome
At the end of v0.0.2.2:

- `engine/core/HostApp.h` defines the neutral host-facing app interface
- `engine/core/HostRunner.h/.cpp` own the shared outer editor loop
- `EditorMain.cpp` delegates loop ownership to `HostRunner`
- `EditorApp` implements the hosted-app interface
- per-frame editor UI build logic lives in `EditorApp`
- AE editor continues to update with variable frame delta
- the host spine leaves room for a future DD runtime timing policy, including fixed 24 FPS gameplay stepping

## Canonical Editor Frame Order
The v0.0.2.2 host runner uses this order:

1. clear transient per-frame input edges
2. pump Windows messages
3. measure elapsed real time
4. update editor app with variable `dt`
5. build editor UI
6. render

## Next Planned Step
v0.0.2.3 - continue runtime/session preparation, then return to v0.0.3 modeling topology work
