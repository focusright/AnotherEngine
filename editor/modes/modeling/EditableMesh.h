#pragma once
#include <DirectXMath.h>
#include <cstdint>

using namespace DirectX;

using VertexID = uint32_t;
using TriangleID = uint32_t;

static constexpr VertexID kInvalidVertexID = 0xFFFFFFFFu;
static constexpr TriangleID kInvalidTriangleID = 0xFFFFFFFFu;

struct EditTriangle {
    VertexID a = 0;
    VertexID b = 0;
    VertexID c = 0;
};

// Authoritative mesh data for editing (CPU-side truth)
struct EditableMesh {
    static constexpr uint32_t kMaxVertices = 64;
    static constexpr uint32_t kMaxTriangles = 128;

    uint32_t vertexCount = 0;
    uint32_t triangleCount = 0;

    XMFLOAT3 positions[kMaxVertices] = {};
    EditTriangle triangles[kMaxTriangles] = {};

    void Clear() {
        vertexCount = 0;
        triangleCount = 0;
    }

    bool IsValidVertex(VertexID v) const {
        return v < vertexCount;
    }

    bool IsValidTriangle(TriangleID t) const {
        return t < triangleCount;
    }

    VertexID AddVertex(const XMFLOAT3& p) {
        if (vertexCount >= kMaxVertices) { return kInvalidVertexID; }
        VertexID id = vertexCount++;
        positions[id] = p;
        return id;
    }

    TriangleID AddTriangle(VertexID a, VertexID b, VertexID c) {
        if (triangleCount >= kMaxTriangles) { return kInvalidTriangleID; }
        if (!IsValidVertex(a) || !IsValidVertex(b) || !IsValidVertex(c)) { return kInvalidTriangleID; }

        TriangleID id = triangleCount++;
        triangles[id].a = a;
        triangles[id].b = b;
        triangles[id].c = c;
        return id;
    }

    void SetVertex(VertexID v, const XMFLOAT3& p) {
        if (!IsValidVertex(v)) { return; }
        positions[v] = p;
    }

    XMFLOAT3 GetVertex(VertexID v) const {
        if (!IsValidVertex(v)) { return XMFLOAT3(0.0f, 0.0f, 0.0f); }
        return positions[v];
    }

    void SetTriangle(TriangleID t, VertexID a, VertexID b, VertexID c) {
        if (!IsValidTriangle(t)) { return; }
        if (!IsValidVertex(a) || !IsValidVertex(b) || !IsValidVertex(c)) { return; }

        triangles[t].a = a;
        triangles[t].b = b;
        triangles[t].c = c;
    }

    EditTriangle GetTriangle(TriangleID t) const {
        if (!IsValidTriangle(t)) { return EditTriangle{}; }
        return triangles[t];
    }

    VertexID GetTriangleVertex(TriangleID t, uint32_t corner) const {
        if (!IsValidTriangle(t)) { return 0; }

        const EditTriangle& tri = triangles[t];
        if (corner == 0) { return tri.a; }
        if (corner == 1) { return tri.b; }
        return tri.c;
    }

    void BuildTetrahedron(float s) {
        Clear();

        VertexID top = AddVertex(XMFLOAT3(0.0f, 0.0f, s));
        VertexID right = AddVertex(XMFLOAT3(0.9428f * s, 0.0f, -0.3333f * s));
        VertexID back = AddVertex(XMFLOAT3(-0.4714f * s, 0.8165f * s, -0.3333f * s));
        VertexID front = AddVertex(XMFLOAT3(-0.4714f * s, -0.8165f * s, -0.3333f * s));

        AddTriangle(top, right, back);
        AddTriangle(top, back, front);
        AddTriangle(top, front, right);
        AddTriangle(right, front, back);
    }
};