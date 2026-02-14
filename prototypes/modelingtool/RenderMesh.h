#pragma once

#include <DirectXMath.h>
#include <cstdint>

// Matches the existing vertex layout used by CreateVertexBuffer/Draw.
struct Vertex {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT4 color;
};

struct RenderMesh {
    static constexpr uint32_t kDrawVertexCount = 12; // 4 faces * 3 verts

    bool dirty = true;

    // Each draw vertex references an EditableMesh vertex index (0..3 for tetra)
    uint32_t drawToEdit[kDrawVertexCount] = {};

    Vertex drawVertices[kDrawVertexCount] = {};
};
