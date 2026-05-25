#pragma once

#include <windows.h>

class IHostApp {
public:
    virtual ~IHostApp() {}

    virtual void BeginFrame() = 0;
    virtual void PumpMessages(MSG& msg) = 0;
    virtual void Update(float dt) = 0;
    virtual void BuildFrameUI() = 0;
    virtual void Render() = 0;
};