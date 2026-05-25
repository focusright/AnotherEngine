#pragma once

#include <windows.h>
#include <cstdint>

struct HostFrame {
    float dt = 0.0f;
    float rawDt = 0.0f;
    float totalTime = 0.0f;
    uint64_t frameIndex = 0;
};

class IHostApp {
public:
    virtual ~IHostApp() {}

    virtual void BeginFrame() = 0;
    virtual void PumpMessages(MSG& msg) = 0;
    virtual void Update(const HostFrame& frame) = 0;
    virtual void BuildFrameUI() = 0;
    virtual void Render() = 0;
};