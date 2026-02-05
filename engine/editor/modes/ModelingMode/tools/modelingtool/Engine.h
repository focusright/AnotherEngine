#pragma once

#include "GraphicsDevice.h"

class Engine {
public:
    void SetGraphicsDevice(GraphicsDevice* gfx) { m_gfx = gfx; }
    GraphicsDevice* Gfx() const { return m_gfx; }

private:
    GraphicsDevice* m_gfx = nullptr; // owned elsewhere for now (main.cpp baseline)
};
