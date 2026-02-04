#include "GraphicsDevice.h"

bool GraphicsDevice::CreateCommandQueue(ID3D12Device* device) {
    if (!device) { return false; }

    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    m_queue.Reset();
    HRESULT hr = device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_queue));
    if (FAILED(hr) || !m_queue) { return false; }

    return true;
}

bool GraphicsDevice::CreateSwapChain(IDXGIFactory6* factory, HWND hwnd, uint32_t width, uint32_t height, DXGI_FORMAT format, uint32_t bufferCount) {
    if (!factory || !m_queue) { return false; }

    DXGI_SWAP_CHAIN_DESC1 desc = {};
    desc.BufferCount = bufferCount;
    desc.Width = width;
    desc.Height = height;
    desc.Format = format;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.SampleDesc.Count = 1;

    Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain1;
    HRESULT hr = factory->CreateSwapChainForHwnd(m_queue.Get(), hwnd, &desc, nullptr, nullptr, &swapChain1);

    if (FAILED(hr)) { return false; }

    hr = swapChain1.As(&m_swapChain);
    if (FAILED(hr)) { return false; }

    return true;
}
