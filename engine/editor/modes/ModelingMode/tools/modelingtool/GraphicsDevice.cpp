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

bool GraphicsDevice::CreateRTVs(ID3D12Device* device, DXGI_FORMAT rtvFormat, uint32_t bufferCount) {
    if (!device || !m_swapChain) { return false; }

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = bufferCount;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    HRESULT hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_rtvHeap));
    if (FAILED(hr)) { return false; }

    m_rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

    for (uint32_t i = 0; i < bufferCount; ++i) {
        hr = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i]));
        if (FAILED(hr)) { return false; }

        device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, handle);
        handle.ptr += m_rtvDescriptorSize;
    }

    return true;
}

D3D12_CPU_DESCRIPTOR_HANDLE GraphicsDevice::RTV(uint32_t frameIndex) const {
    D3D12_CPU_DESCRIPTOR_HANDLE handle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += frameIndex * m_rtvDescriptorSize;
    return handle;
}

ID3D12Resource* GraphicsDevice::BackBuffer(uint32_t frameIndex) const {
    return m_renderTargets[frameIndex].Get();
}

void GraphicsDevice::SetFrameIndexFromSwapChain() {
    if (!m_swapChain) { return; }
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

D3D12_CPU_DESCRIPTOR_HANDLE GraphicsDevice::CurrentRTV() const {
    return RTV(m_frameIndex);
}

ID3D12Resource* GraphicsDevice::CurrentBackBuffer() const {
    return BackBuffer(m_frameIndex);
}
