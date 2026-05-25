# AEDD PROJECT STATE

## Current Version
v0.0.2.3 - host frame data pass

## Status
In progress.

## Current Focus
AE is formalizing the per-frame data passed from the neutral host runner into the editor app. This keeps the current editor on variable frame delta while making frame timing explicit enough for later runtime/session policies.

## v0.0.2.3 Outcome
At the end of v0.0.2.3:

- `engine/core/HostApp.h` defines `HostFrame`
- `HostRunner` builds one `HostFrame` per outer loop iteration
- `HostFrame` carries clamped `dt`, unclamped `rawDt`, accumulated `totalTime`, and `frameIndex`
- `IHostApp::Update` receives `const HostFrame&` instead of a raw float
- `EditorApp` still updates with variable editor `dt` from `frame.dt`
- the Scene ImGui window displays basic frame timing/debug data

## Canonical Editor Frame Order
The host runner uses this order:

1. clear transient per-frame input edges
2. pump Windows messages
3. measure elapsed real time
4. build `HostFrame`
5. update editor app with variable `frame.dt`
6. build editor UI
7. render

## Next Planned Step
v0.0.3 - return to modeling structure/topology work, starting from the current hosted editor baseline