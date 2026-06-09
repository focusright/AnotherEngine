#pragma once

#include <DirectXMath.h>
#include <cstdint>
#include "EditableMesh.h"

// Matches the existing vertex layout used by CreateVertexBuffer/Draw.
struct Vertex {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT4 color;
};

struct RenderMesh {
    static constexpr uint32_t kMaxDrawVertexCount = EditableMesh::kMaxTriangles * 3;

    bool dirty = true;
    uint32_t drawVertexCount = 0;

    // Each draw vertex references an EditableMesh vertex index and triangle index.
    uint32_t drawToEdit[kMaxDrawVertexCount] = {};
    uint32_t drawToTriangle[kMaxDrawVertexCount] = {};

    Vertex drawVertices[kMaxDrawVertexCount] = {};

    static DirectX::XMFLOAT4 FaceColor(uint32_t face) {
        switch (face % 4) {
        case 0: return DirectX::XMFLOAT4(1, 0, 0, 1);
        case 1: return DirectX::XMFLOAT4(0, 1, 0, 1);
        case 2: return DirectX::XMFLOAT4(0, 0, 1, 1);
        default: return DirectX::XMFLOAT4(1, 1, 0, 1);
        }
    }

    void Clear() {
        dirty = true;
        drawVertexCount = 0;

        for (uint32_t i = 0; i < kMaxDrawVertexCount; ++i) {
            drawToEdit[i] = 0;
            drawToTriangle[i] = 0;
            drawVertices[i] = Vertex{};
        }
    }

    bool BuildFromEditable(const EditableMesh& mesh) {
        Clear();

        uint32_t needed = mesh.triangleCount * 3;
        if (needed > kMaxDrawVertexCount) { return false; }

        for (uint32_t face = 0; face < mesh.triangleCount; ++face) {
            DirectX::XMFLOAT4 color = FaceColor(face);

            for (uint32_t corner = 0; corner < 3; ++corner) {
                uint32_t draw = face * 3 + corner;
                VertexID vertex = mesh.GetTriangleVertex(face, corner);

                drawToEdit[draw] = vertex;
                drawToTriangle[draw] = face;
                drawVertices[draw].position = mesh.GetVertex(vertex);
                drawVertices[draw].color = color;
            }
        }

        drawVertexCount = needed;
        dirty = true;
        return true;
    }
};