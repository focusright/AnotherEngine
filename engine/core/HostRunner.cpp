#include "engine/core/HostRunner.h"
#include "engine/core/HostApp.h"

#include <windows.h>

int HostRunner::Run(IHostApp& app) {
    const float fixedDt = 1.0f / 24.0f;
    const float maxFrameDt = 0.25f;

    LARGE_INTEGER freq = {};
    LARGE_INTEGER prev = {};
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&prev);

    float accumulator = fixedDt; // Force one initial tick so render state is seeded before the first visible frame.
    MSG msg = {};

    app.ClearFrameInputEdges();

    while (msg.message != WM_QUIT) {
        app.PumpMessages(msg);

        if (msg.message == WM_QUIT) { break; }

        LARGE_INTEGER now = {};
        QueryPerformanceCounter(&now);

        float frameDt = float(double(now.QuadPart - prev.QuadPart) / double(freq.QuadPart));
        prev = now;

        if (frameDt > maxFrameDt) { frameDt = maxFrameDt; }

        accumulator += frameDt;

        app.BuildFrameUI();

        while (accumulator >= fixedDt && msg.message != WM_QUIT) {
            app.FixedUpdate(fixedDt);
            app.ClearFrameInputEdges();
            accumulator -= fixedDt;
        }

        app.Render();
    }

    return static_cast<int>(msg.wParam);
}