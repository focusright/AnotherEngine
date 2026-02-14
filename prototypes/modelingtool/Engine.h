#pragma once

#include <cstdio>
#include <cstdint>
#include <windows.h>
#include <DirectXMath.h>
#include <d3d12.h>

#pragma warning(push)
#pragma warning(disable : 6001)
#include "d3dx12.h"
#pragma warning(pop)

class GraphicsDevice;
struct EditableMesh;
struct RenderMesh;

class Engine {
public:
    void SetGraphicsDevice(GraphicsDevice* gfx) { m_gfx = gfx; }
    GraphicsDevice* Gfx() const { return m_gfx; }

    void SetRenderObjects(ID3D12CommandAllocator* commandAllocator, ID3D12GraphicsCommandList* commandList, ID3D12RootSignature* rootSignature, ID3D12PipelineState* pipelineStateTriangles, ID3D12PipelineState* pipelineStateLines, ID3D12Fence* fence, HANDLE fenceEvent, UINT64* fenceValue, ID3D12Resource* vertexBufferTetra, ID3D12Resource* vertexBufferGrid, uint32_t gridVertexCount, uint32_t width, uint32_t height);
    void UpdateVertexBuffer(const EditableMesh* editMesh, RenderMesh* renderMesh, HWND hwnd);

    void PopulateCommandList();
    void WaitForGpu();
    void MoveToNextFrame();
    void RenderFrame();

    void SetViewProj(const DirectX::XMFLOAT4X4& viewProj);

    // Simple scene support (multiple objects drawn with different world transforms).
    void SetObjectCount(uint32_t count);
    void SetObjectWorld(uint32_t index, const DirectX::XMFLOAT4X4& world);
    void SetSelectedObject(uint32_t index) { m_selectedObject = index; }

    // Debug helpers (editor-only visualization).
    // If set to a valid index, that object will be tinted cyan to visualize a pivot/marker.
    void SetDebugPivotIndex(uint32_t index) { m_debugPivotIndex = index; }

private:
    GraphicsDevice* m_gfx = nullptr;

    ID3D12CommandAllocator* m_commandAllocator = nullptr;
    ID3D12GraphicsCommandList* m_commandList = nullptr;
    ID3D12RootSignature* m_rootSignature = nullptr;
    ID3D12PipelineState* m_pipelineStateTriangles = nullptr;
    ID3D12PipelineState* m_pipelineStateLines = nullptr;
    ID3D12Resource* m_vertexBufferTetra = nullptr;
    ID3D12Resource* m_vertexBufferGrid = nullptr;
    uint32_t m_gridVertexCount = 0;

    ID3D12Fence* m_fence = nullptr;
    HANDLE m_fenceEvent = nullptr;
    UINT64* m_fenceValue = nullptr;

    uint32_t m_width = 0;
    uint32_t m_height = 0;

    DirectX::XMFLOAT4X4 m_viewProj = {};

    static const uint32_t kMaxObjects = 16;
    uint32_t m_objectCount = 1;
    DirectX::XMFLOAT4X4 m_world[kMaxObjects] = {};

    uint32_t m_selectedObject = 0;

    uint32_t m_debugPivotIndex = 0xFFFFFFFFu;
};
