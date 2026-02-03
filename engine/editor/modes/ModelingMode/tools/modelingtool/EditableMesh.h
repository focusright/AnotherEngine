#pragma once
#include <DirectXMath.h>
#include <cstdint>

using namespace DirectX;

using VertexID = uint32_t;

// Authoritative mesh data for editing (CPU-side truth)
struct EditableMesh {
    // For now: fixed triangle (we will generalize later)
    XMFLOAT3 positions[3];

    void SetVertex(VertexID v, const XMFLOAT3& p) {
        positions[v] = p;
    }

    XMFLOAT3 GetVertex(VertexID v) const {
        return positions[v];
    }
};
