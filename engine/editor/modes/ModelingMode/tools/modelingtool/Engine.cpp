#include "Engine.h"
#include "GraphicsDevice.h"
#include "RenderMesh.h"
#include "EditableMesh.h"
#include <DirectXMath.h>

void Engine::SetRenderObjects(ID3D12CommandAllocator* commandAllocator, ID3D12GraphicsCommandList* commandList, ID3D12RootSignature* rootSignature, ID3D12PipelineState* pipelineState, ID3D12Fence* fence, HANDLE fenceEvent, UINT64* fenceValue, ID3D12Resource* vertexBuffer, uint32_t width, uint32_t height) {
    m_commandAllocator = commandAllocator;
    m_commandList = commandList;
    m_rootSignature = rootSignature;
    m_pipelineState = pipelineState;
    m_fence = fence;
    m_fenceEvent = fenceEvent;
    m_fenceValue = fenceValue;
    m_vertexBuffer = vertexBuffer;
    m_width = width;
    m_height = height;
}

void Engine::UpdateVertexBuffer(const EditableMesh* editMesh, RenderMesh* renderMesh, HWND hwnd) {
    if (!m_vertexBuffer) { return; }

    for (uint32_t i = 0; i < 3; ++i) {
        renderMesh->drawVertices[i].position = editMesh->GetVertex(i);
    }

    UINT8* pVertexDataBegin = nullptr;
    D3D12_RANGE readRange = { 0, 0 };

    HRESULT hr = m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
    if (FAILED(hr) || !pVertexDataBegin) {
        wchar_t buf[256];
        swprintf_s(buf, L"VertexBuffer Map failed. hr=0x%08X", (unsigned)hr);
        MessageBox(hwnd, buf, L"Error", MB_OK);
        return;
    }

    memcpy(pVertexDataBegin, renderMesh->drawVertices, sizeof(renderMesh->drawVertices));
    m_vertexBuffer->Unmap(0, nullptr);
}

void Engine::PopulateCommandList() {
    m_commandAllocator->Reset();
    m_commandList->Reset(m_commandAllocator, m_pipelineState);

    m_commandList->SetGraphicsRootSignature(m_rootSignature);
    m_commandList->SetGraphicsRoot32BitConstants(0, 16, &m_viewProj, 0);

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

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_gfx->CurrentRTV();
    m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    const float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView{
        m_vertexBuffer->GetGPUVirtualAddress(),
        sizeof(Vertex) * 3,
        sizeof(Vertex)
    };
    m_commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
    m_commandList->DrawInstanced(3, 1, 0, 0);

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
