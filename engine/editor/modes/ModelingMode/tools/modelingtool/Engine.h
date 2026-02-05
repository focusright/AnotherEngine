#pragma once

#include <cstdio>
#include <cstdint>
#include <windows.h>
#include <d3d12.h>

class GraphicsDevice;
struct EditableMesh;
struct RenderMesh;

class Engine {
public:
    void SetGraphicsDevice(GraphicsDevice* gfx) { m_gfx = gfx; }
    GraphicsDevice* Gfx() const { return m_gfx; }

    void SetRenderObjects(ID3D12CommandAllocator* commandAllocator, ID3D12GraphicsCommandList* commandList, ID3D12RootSignature* rootSignature, ID3D12PipelineState* pipelineState, ID3D12Fence* fence, HANDLE fenceEvent, UINT64* fenceValue, ID3D12Resource* vertexBuffer, uint32_t width, uint32_t height);
    void UpdateVertexBuffer(const EditableMesh* editMesh, RenderMesh* renderMesh, HWND hwnd);

    void PopulateCommandList();
    void WaitForGpu();
    void MoveToNextFrame();

private:
    GraphicsDevice* m_gfx = nullptr;

    ID3D12CommandAllocator* m_commandAllocator = nullptr;
    ID3D12GraphicsCommandList* m_commandList = nullptr;
    ID3D12RootSignature* m_rootSignature = nullptr;
    ID3D12PipelineState* m_pipelineState = nullptr;
    ID3D12Resource* m_vertexBuffer = nullptr;

    ID3D12Fence* m_fence = nullptr;
    HANDLE m_fenceEvent = nullptr;
    UINT64* m_fenceValue = nullptr;

    uint32_t m_width = 0;
    uint32_t m_height = 0;
};
