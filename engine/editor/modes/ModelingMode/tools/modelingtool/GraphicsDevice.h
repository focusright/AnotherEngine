#pragma once

#include <windows.h>
#include <wrl/client.h>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <cstdint>

#include "d3dx12.h"

class GraphicsDevice {
public:
    static const uint32_t FrameCount = 2;

    bool Init(HWND hwnd, uint32_t width, uint32_t height);
    void Shutdown();

    void Resize(uint32_t width, uint32_t height);

    ID3D12Device* Device() const { return m_device.Get(); }
    ID3D12GraphicsCommandList* CmdList() const { return m_cmdList.Get(); }
    ID3D12CommandQueue* Queue() const { return m_cmdQueue.Get(); }

    uint32_t CurrentFrameIndex() const { return m_frameIndex; }
    uint32_t Width() const { return m_width; }
    uint32_t Height() const { return m_height; }

    D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferRTV() const;
    ID3D12Resource* CurrentBackBuffer() const { return m_renderTargets[m_frameIndex].Get(); }

    void BeginFrame(ID3D12PipelineState* initialPSO);
    void EndFrameAndPresent(bool vsync);

    void WaitForGpu();

private:
    bool CreateDeviceAndQueue();
    bool CreateSwapChain(HWND hwnd, uint32_t width, uint32_t height);
    bool CreateRTVs();
    bool CreateCommandObjects();
    bool CreateSyncObjects();

    void MoveToNextFrame();
    void ReleaseSwapChainResources();

private:
    uint32_t m_width = 0;
    uint32_t m_height = 0;

    Microsoft::WRL::ComPtr<IDXGIFactory4> m_factory;
    Microsoft::WRL::ComPtr<ID3D12Device> m_device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_cmdQueue;
    Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    uint32_t m_rtvDescriptorSize = 0;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_renderTargets[FrameCount];

    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_cmdAlloc[FrameCount];
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_cmdList;

    Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
    uint64_t m_fenceValue[FrameCount] = {};
    HANDLE m_fenceEvent = nullptr;

    uint32_t m_frameIndex = 0;
    bool m_hasPresentedOnce = false;
};
