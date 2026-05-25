#pragma once
#include <DirectXMath.h>
#include <cstdint>

using namespace DirectX;

using VertexID = uint32_t;
using TriangleID = uint32_t;

struct EditTriangle {
    VertexID a = 0;
    VertexID b = 0;
    VertexID c = 0;
};

// Authoritative mesh data for editing (CPU-side truth)
struct EditableMesh {
    static constexpr uint32_t kVertexCount = 4;   // tetrahedron vertices
    static constexpr uint32_t kTriangleCount = 4; // tetrahedron faces

    XMFLOAT3 positions[kVertexCount];
    EditTriangle triangles[kTriangleCount];

    void SetVertex(VertexID v, const XMFLOAT3& p) {
        positions[v] = p;
    }

    XMFLOAT3 GetVertex(VertexID v) const {
        return positions[v];
    }

    void SetTriangle(TriangleID t, VertexID a, VertexID b, VertexID c) {
        triangles[t].a = a;
        triangles[t].b = b;
        triangles[t].c = c;
    }

    EditTriangle GetTriangle(TriangleID t) const {
        return triangles[t];
    }

    VertexID GetTriangleVertex(TriangleID t, uint32_t corner) const {
        const EditTriangle& tri = triangles[t];
        if (corner == 0) { return tri.a; }
        if (corner == 1) { return tri.b; }
        return tri.c;
    }

    void BuildTetrahedron(float s) {
        positions[0] = XMFLOAT3(0.0f, 0.0f, s);
        positions[1] = XMFLOAT3(0.9428f * s, 0.0f, -0.3333f * s);
        positions[2] = XMFLOAT3(-0.4714f * s, 0.8165f * s, -0.3333f * s);
        positions[3] = XMFLOAT3(-0.4714f * s, -0.8165f * s, -0.3333f * s);

        SetTriangle(0, 0, 1, 2);
        SetTriangle(1, 0, 2, 3);
        SetTriangle(2, 0, 3, 1);
        SetTriangle(3, 1, 3, 2);
    }
};