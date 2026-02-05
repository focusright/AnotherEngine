#pragma once

#include <DirectXMath.h>
#include <cstdint>

// Matches the existing vertex layout used by CreateVertexBuffer/Draw.
struct Vertex {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT4 color;
};

// GPU-side cache state derived from EditableMesh
struct RenderMesh {
    bool dirty = true;
    Vertex drawVertices[3];
};
