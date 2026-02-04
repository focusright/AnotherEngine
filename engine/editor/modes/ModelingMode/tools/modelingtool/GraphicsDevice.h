#pragma once

#include <wrl/client.h>
#include <dxgi1_6.h>
#include <d3d12.h>

class GraphicsDevice {
public:
    void SetDevice(const Microsoft::WRL::ComPtr<ID3D12Device>& device) { m_device = device; }
    void SetSwapChain(const Microsoft::WRL::ComPtr<IDXGISwapChain3>& swapChain) { m_swapChain = swapChain; }

    ID3D12Device* Device() const { return m_device.Get(); }
    ID3D12CommandQueue* Queue() const { return m_queue.Get(); }
    IDXGISwapChain3* SwapChain() const { return m_swapChain.Get(); }

    bool CreateCommandQueue(ID3D12Device* device);

private:
    Microsoft::WRL::ComPtr<ID3D12Device> m_device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_queue;
    Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
};
