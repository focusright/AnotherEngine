#pragma once

#include <windows.h>

class IHostApp {
public:
    virtual ~IHostApp() {}

    virtual void ClearFrameInputEdges() = 0;
    virtual void PumpMessages(MSG& msg) = 0;
    virtual void BuildFrameUI() = 0;
    virtual void FixedUpdate(float dt) = 0;
    virtual void Render() = 0;
};