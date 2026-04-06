#pragma once
#include <DirectXMath.h>
#include <cstdint>

using namespace DirectX;

using VertexID = uint32_t;

// Authoritative mesh data for editing (CPU-side truth)
struct EditableMesh {
    static constexpr uint32_t kVertexCount = 4; // tetrahedron vertices
    XMFLOAT3 positions[kVertexCount];

    void SetVertex(VertexID v, const XMFLOAT3& p) {
        positions[v] = p;
    }

    XMFLOAT3 GetVertex(VertexID v) const {
        return positions[v];
    }
};
