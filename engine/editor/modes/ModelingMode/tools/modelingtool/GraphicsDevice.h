#pragma once

#include <cstdint>
#include <wrl/client.h>
#include <dxgi1_6.h>
#include <d3d12.h>

#pragma warning(push)
#pragma warning(disable : 6001)
#include "d3dx12.h"
#pragma warning(pop)

class GraphicsDevice {
public:
    ID3D12Device* Device() const { return m_device.Get(); }
    ID3D12CommandQueue* Queue() const { return m_queue.Get(); }
    IDXGISwapChain3* SwapChain() const { return m_swapChain.Get(); }

    bool CreateCommandQueue(ID3D12Device* device);
    bool CreateSwapChain(IDXGIFactory6* factory, HWND hwnd, uint32_t width, uint32_t height, DXGI_FORMAT format, uint32_t bufferCount);
    bool CreateRTVs(ID3D12Device* device, DXGI_FORMAT rtvFormat, uint32_t bufferCount);

    D3D12_CPU_DESCRIPTOR_HANDLE RTV(uint32_t frameIndex) const;
    ID3D12Resource* BackBuffer(uint32_t frameIndex) const;

    void SetFrameIndexFromSwapChain();
    uint32_t FrameIndex() const { return m_frameIndex; }

    D3D12_CPU_DESCRIPTOR_HANDLE CurrentRTV() const;
    ID3D12Resource* CurrentBackBuffer() const;

    private:
    Microsoft::WRL::ComPtr<ID3D12Device> m_device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_queue;
    Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_renderTargets[2];
    uint32_t m_rtvDescriptorSize = 0;

    uint32_t m_frameIndex = 0;
};
