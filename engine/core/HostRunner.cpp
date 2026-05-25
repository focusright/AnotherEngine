#include "engine/core/HostRunner.h"
#include "engine/core/HostApp.h"

#include <windows.h>

int HostRunner::Run(IHostApp& app) {
    const float maxFrameDt = 0.25f; // Clamp huge stalls so editor motion/timers do not jump forward by seconds.

    LARGE_INTEGER freq = {};
    LARGE_INTEGER prev = {};
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&prev);

    MSG msg = {};
    float totalTime = 0.0f;
    uint64_t frameIndex = 0;

    while (msg.message != WM_QUIT) {
        app.BeginFrame();
        app.PumpMessages(msg);

        if (msg.message == WM_QUIT) { break; }

        LARGE_INTEGER now = {};
        QueryPerformanceCounter(&now);

        float rawDt = float(double(now.QuadPart - prev.QuadPart) / double(freq.QuadPart));
        prev = now;

        float dt = rawDt;
        if (dt > maxFrameDt) { dt = maxFrameDt; }
        totalTime += dt;

        HostFrame frame = {};
        frame.dt = dt;
        frame.rawDt = rawDt;
        frame.totalTime = totalTime;
        frame.frameIndex = frameIndex++;

        app.Update(frame);
        app.BuildFrameUI();
        app.Render();
    }

    return static_cast<int>(msg.wParam);
}