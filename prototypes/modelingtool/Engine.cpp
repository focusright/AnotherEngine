#include "Engine.h"
#include "GraphicsDevice.h"
#include "RenderMesh.h"
#include "EditableMesh.h"
#include <DirectXMath.h>

void Engine::SetRenderObjects(ID3D12CommandAllocator* commandAllocator, ID3D12GraphicsCommandList* commandList, ID3D12RootSignature* rootSignature, ID3D12PipelineState* pipelineStateTriangles, ID3D12PipelineState* pipelineStateLines, ID3D12Fence* fence, HANDLE fenceEvent, UINT64* fenceValue, ID3D12Resource* vertexBufferTetra, ID3D12Resource* vertexBufferGrid, uint32_t gridVertexCount, uint32_t width, uint32_t height) {
    m_commandAllocator = commandAllocator;
    m_commandList = commandList;
    m_rootSignature = rootSignature;
    m_pipelineStateTriangles = pipelineStateTriangles;
    m_pipelineStateLines = pipelineStateLines;
    m_fence = fence;
    m_fenceEvent = fenceEvent;
    m_fenceValue = fenceValue;
    m_vertexBufferTetra = vertexBufferTetra;
    m_vertexBufferGrid = vertexBufferGrid;
    m_gridVertexCount = gridVertexCount;
    m_gizmoVertexCount = 6;
    m_gridBaseVertexCount = (gridVertexCount >= m_gizmoVertexCount) ? (gridVertexCount - m_gizmoVertexCount) : gridVertexCount;
    m_width = width;
    m_height = height;
}

void Engine::UpdateVertexBuffer(const EditableMesh* editMesh, RenderMesh* renderMesh, HWND hwnd) {
    if (!m_vertexBufferTetra) { return; }

    for (uint32_t i = 0; i < RenderMesh::kDrawVertexCount; ++i) {
        uint32_t ev = renderMesh->drawToEdit[i];
        renderMesh->drawVertices[i].position = editMesh->GetVertex(ev);
    }

    UINT8* pVertexDataBegin = nullptr;
    D3D12_RANGE readRange = { 0, 0 };

    HRESULT hr = m_vertexBufferTetra->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
    if (FAILED(hr) || !pVertexDataBegin) {
        wchar_t buf[256];
        swprintf_s(buf, L"VertexBuffer Map failed. hr=0x%08X", (unsigned)hr);
        MessageBox(hwnd, buf, L"Error", MB_OK);
        return;
    }

    memcpy(pVertexDataBegin, renderMesh->drawVertices, sizeof(renderMesh->drawVertices));
    m_vertexBufferTetra->Unmap(0, nullptr);
}

void Engine::PopulateCommandList() {
    m_commandAllocator->Reset();
    m_commandList->Reset(m_commandAllocator, m_pipelineStateTriangles);

    m_commandList->SetGraphicsRootSignature(m_rootSignature);

    D3D12_VIEWPORT viewport{
        0.0f,
        0.0f,
        static_cast<float>(m_width),
        static_cast<float>(m_height),
        0.0f,
        1.0f
    };
    D3D12_RECT scissorRect{
        0,
        0,
        (LONG)m_width,
        (LONG)m_height
    };
    m_commandList->RSSetViewports(1, &viewport);
    m_commandList->RSSetScissorRects(1, &scissorRect);

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_gfx->CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_commandList->ResourceBarrier(1, &barrier);

    const float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_gfx->CurrentRTV();
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_gfx->DSV();
    m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
    m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // Draw grid first (lines). This mimics Blender's startup grid and helps anchor scale.
    // D3D12 layers: fixed-function pipeline state switch + new vertex buffer binding.
    if (m_pipelineStateLines && m_vertexBufferGrid && m_gridVertexCount >= 2) {
        m_commandList->SetPipelineState(m_pipelineStateLines);
        m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

        D3D12_VERTEX_BUFFER_VIEW gridVBV{
            m_vertexBufferGrid->GetGPUVirtualAddress(),
            sizeof(Vertex) * m_gridVertexCount,
            sizeof(Vertex)
        };
        m_commandList->IASetVertexBuffers(0, 1, &gridVBV);

        // World = identity so WVP = VP.
        DirectX::XMMATRIX VP = DirectX::XMLoadFloat4x4(&m_viewProj);
        DirectX::XMFLOAT4X4 wvp;
        DirectX::XMStoreFloat4x4(&wvp, VP);
        m_commandList->SetGraphicsRoot32BitConstants(0, 16, &wvp, 0);

        // 1) Base grid (dim)
        DirectX::XMFLOAT4 tint = DirectX::XMFLOAT4(0.35f, 0.35f, 0.35f, 1.0f);
        m_commandList->SetGraphicsRoot32BitConstants(1, 4, &tint, 0);

        uint32_t baseCount = (m_gridBaseVertexCount > 0 && m_gridBaseVertexCount <= m_gridVertexCount) ? m_gridBaseVertexCount : m_gridVertexCount;
        m_commandList->DrawInstanced(baseCount, 1, 0, 0);

        // 2) Gizmo tail (full brightness)
        if (m_gizmoVertexCount > 0 && (baseCount + m_gizmoVertexCount) <= m_gridVertexCount) {
            tint = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
            m_commandList->SetGraphicsRoot32BitConstants(1, 4, &tint, 0);
            m_commandList->DrawInstanced(m_gizmoVertexCount, 1, baseCount, 0);
        }
    }

    // Now draw the objects (triangles).
    m_commandList->SetPipelineState(m_pipelineStateTriangles);
    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView{
        m_vertexBufferTetra->GetGPUVirtualAddress(),
        sizeof(Vertex) * RenderMesh::kDrawVertexCount,
        sizeof(Vertex)
    };
    m_commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

    // Draw each object with its own world transform.
    // D3D12 layer note: this is fixed-function pipeline setup + programmable (root constants) per draw.
    uint32_t count = (m_objectCount == 0) ? 1 : m_objectCount;
    if (count > kMaxObjects) count = kMaxObjects;

    for (uint32_t i = 0; i < count; ++i) {
        DirectX::XMMATRIX W = DirectX::XMLoadFloat4x4(&m_world[i]);
        DirectX::XMMATRIX VP = DirectX::XMLoadFloat4x4(&m_viewProj);
        DirectX::XMMATRIX WVP = DirectX::XMMatrixMultiply(W, VP);

        DirectX::XMFLOAT4X4 wvp;
        DirectX::XMStoreFloat4x4(&wvp, WVP);
        m_commandList->SetGraphicsRoot32BitConstants(0, 16, &wvp, 0);
        DirectX::XMFLOAT4 tint = m_tint[i];
        if (tint.w == 0.0f) { tint = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); }
        if (i == m_debugPivotIndex) {
            tint = DirectX::XMFLOAT4(0.25f, 1.0f, 1.0f, 1.0f);
        }
        if (i == m_selectedObject) {
            tint = DirectX::XMFLOAT4(1.0f, 0.35f, 0.35f, 1.0f);
        }
        m_commandList->SetGraphicsRoot32BitConstants(1, 4, &tint, 0);

        m_commandList->DrawInstanced(RenderMesh::kDrawVertexCount, 1, 0, 0);
    }

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_gfx->CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    m_commandList->ResourceBarrier(1, &barrier);

    m_commandList->Close();

    ID3D12CommandList* ppCommandLists[] = { m_commandList };
    m_gfx->Queue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
}

void Engine::WaitForGpu() {
    const UINT64 fence = *m_fenceValue;
    m_gfx->Queue()->Signal(m_fence, fence);
    (*m_fenceValue)++;

    if (m_fence->GetCompletedValue() < fence) {
        m_fence->SetEventOnCompletion(fence, m_fenceEvent);
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }
}

void Engine::MoveToNextFrame() {
    const UINT64 fenceValue = ++(*m_fenceValue);
    m_gfx->Queue()->Signal(m_fence, fenceValue);

    if (m_fence->GetCompletedValue() < fenceValue) {
        m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent);
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }

    m_gfx->SetFrameIndexFromSwapChain();
}

void Engine::RenderFrame() {
    PopulateCommandList();
    m_gfx->SwapChain()->Present(1, 0);
    MoveToNextFrame();
}

void Engine::SetViewProj(const DirectX::XMFLOAT4X4& viewProj) {
    m_viewProj = viewProj;
}

void Engine::SetObjectCount(uint32_t count) {
    m_objectCount = (count == 0) ? 1 : count;

    // Default worlds to identity so uninitialized objects don't explode.
    uint32_t n = m_objectCount;
    if (n > kMaxObjects) n = kMaxObjects;
    for (uint32_t i = 0; i < n; ++i) {
        DirectX::XMStoreFloat4x4(&m_world[i], DirectX::XMMatrixIdentity());
        m_tint[i] = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    }
}

void Engine::SetObjectWorld(uint32_t index, const DirectX::XMFLOAT4X4& world) {
    if (index >= kMaxObjects) return;
    m_world[index] = world;
}


void Engine::SetObjectTint(uint32_t index, const DirectX::XMFLOAT4& tint) {
    if (index >= kMaxObjects) return;
    m_tint[index] = tint;
}

void Engine::UpdateGizmoVertices(const Vertex* verts, uint32_t count, HWND hwnd) {
    if (!m_vertexBufferGrid || !verts || count == 0) return;

    // Clamp count so we don't write past the allocated tail.
    if (count > m_gizmoVertexCount) count = m_gizmoVertexCount;
    if (m_gridBaseVertexCount + count > m_gridVertexCount) return;

    UINT8* pData = nullptr;
    D3D12_RANGE readRange = { 0, 0 };

    HRESULT hr = m_vertexBufferGrid->Map(0, &readRange, reinterpret_cast<void**>(&pData));
    if (FAILED(hr) || !pData) {
        wchar_t buf[256];
        swprintf_s(buf, L"Grid VertexBuffer Map failed (gizmo). hr=0x%08X", (unsigned)hr);
        MessageBox(hwnd, buf, L"Error", MB_OK);
        return;
    }

    size_t byteOffset = sizeof(Vertex) * size_t(m_gridBaseVertexCount);
    memcpy(pData + byteOffset, verts, sizeof(Vertex) * count);

    m_vertexBufferGrid->Unmap(0, nullptr);
}
