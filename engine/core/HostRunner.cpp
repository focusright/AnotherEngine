#include "engine/core/HostRunner.h"
#include "engine/core/HostApp.h"

#include <windows.h>

int HostRunner::Run(IHostApp& app) {
    const float maxFrameDt = 0.25f;

    LARGE_INTEGER freq = {};
    LARGE_INTEGER prev = {};
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&prev);

    MSG msg = {};

    while (msg.message != WM_QUIT) {
        app.BeginFrame();
        app.PumpMessages(msg);

        if (msg.message == WM_QUIT) { break; }

        LARGE_INTEGER now = {};
        QueryPerformanceCounter(&now);

        float dt = float(double(now.QuadPart - prev.QuadPart) / double(freq.QuadPart));
        prev = now;

        if (dt > maxFrameDt) { dt = maxFrameDt; }

        app.Update(dt);
        app.BuildFrameUI();
        app.Render();
    }

    return static_cast<int>(msg.wParam);
}