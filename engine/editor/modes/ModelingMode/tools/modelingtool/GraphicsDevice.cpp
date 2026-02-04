#include "GraphicsDevice.h"
#include <stdexcept>

using Microsoft::WRL::ComPtr;

static void ThrowIfFailed(HRESULT hr, const char* file, int line) {
    if (FAILED(hr)) {
        char buf[256];
        sprintf_s(buf, "D3D12 call failed. hr=0x%08X at %s:%d\n", (unsigned)hr, file, line);
        OutputDebugStringA(buf);
        throw std::runtime_error(buf);
    }
}

#define THROW_IF_FAILED(x) ThrowIfFailed((x), __FILE__, __LINE__)

bool GraphicsDevice::Init(HWND hwnd, uint32_t width, uint32_t height) {
    m_width = width;
    m_height = height;

    UINT factoryFlags = 0;
#if defined(_DEBUG)
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
            debugController->EnableDebugLayer();
            factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif

    THROW_IF_FAILED(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&m_factory)));

    if (!CreateDeviceAndQueue()) { return false; }
    if (!CreateCommandObjects()) { return false; }
    if (!CreateSwapChain(hwnd, width, height)) { return false; }
    if (!CreateRTVs()) { return false; }
    if (!CreateSyncObjects()) { return false; }

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
    return true;
}

void GraphicsDevice::Shutdown() {
    WaitForGpu();

    if (m_fenceEvent) {
        CloseHandle(m_fenceEvent);
        m_fenceEvent = nullptr;
    }

    ReleaseSwapChainResources();
    m_swapChain.Reset();
    m_cmdList.Reset();
    for (uint32_t i = 0; i < FrameCount; i++) { m_cmdAlloc[i].Reset(); }
    m_rtvHeap.Reset();
    m_cmdQueue.Reset();
    m_fence.Reset();
    m_device.Reset();
    m_factory.Reset();
}

bool GraphicsDevice::CreateDeviceAndQueue() {
    ComPtr<IDXGIAdapter1> adapter;
    for (UINT i = 0; m_factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; i++) {
        DXGI_ADAPTER_DESC1 desc = {};
        adapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) { continue; }

        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)))) {
            break;
        }
    }

    if (!m_device) {
        ComPtr<IDXGIAdapter> warpAdapter;
        THROW_IF_FAILED(m_factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));
        THROW_IF_FAILED(D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)));
    }

    D3D12_COMMAND_QUEUE_DESC qdesc = {};
    qdesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    qdesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    THROW_IF_FAILED(m_device->CreateCommandQueue(&qdesc, IID_PPV_ARGS(&m_cmdQueue)));

    return true;
}

bool GraphicsDevice::CreateCommandObjects() {
    for (uint32_t i = 0; i < FrameCount; i++) {
        THROW_IF_FAILED(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_cmdAlloc[i])));
    }

    THROW_IF_FAILED(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_cmdAlloc[0].Get(), nullptr, IID_PPV_ARGS(&m_cmdList)));
    THROW_IF_FAILED(m_cmdList->Close());

    return true;
}

bool GraphicsDevice::CreateSwapChain(HWND hwnd, uint32_t width, uint32_t height) {
    DXGI_SWAP_CHAIN_DESC1 scDesc = {};
    scDesc.BufferCount = FrameCount;
    scDesc.Width = width;
    scDesc.Height = height;
    scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain1;
    THROW_IF_FAILED(m_factory->CreateSwapChainForHwnd(
        m_cmdQueue.Get(),
        hwnd,
        &scDesc,
        nullptr,
        nullptr,
        &swapChain1));

    THROW_IF_FAILED(m_factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
    THROW_IF_FAILED(swapChain1.As(&m_swapChain));

    return true;
}

bool GraphicsDevice::CreateRTVs() {
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = FrameCount;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    THROW_IF_FAILED(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_rtvHeap)));

    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
    for (uint32_t i = 0; i < FrameCount; i++) {
        THROW_IF_FAILED(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])));
        m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHandle);
        rtvHandle.Offset(1, m_rtvDescriptorSize);
    }

    return true;
}

bool GraphicsDevice::CreateSyncObjects() {
    THROW_IF_FAILED(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));

    for (uint32_t i = 0; i < FrameCount; i++) { m_fenceValue[i] = 0; }

    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!m_fenceEvent) { return false; }

    return true;
}

void GraphicsDevice::ReleaseSwapChainResources() {
    for (uint32_t i = 0; i < FrameCount; i++) { m_renderTargets[i].Reset(); }
    m_rtvHeap.Reset();
}

void GraphicsDevice::Resize(uint32_t width, uint32_t height) {
    if (!m_swapChain) { return; }
    if (width == 0 || height == 0) { return; }

    WaitForGpu();
    ReleaseSwapChainResources();

    DXGI_SWAP_CHAIN_DESC desc = {};
    THROW_IF_FAILED(m_swapChain->GetDesc(&desc));
    THROW_IF_FAILED(m_swapChain->ResizeBuffers(FrameCount, width, height, desc.BufferDesc.Format, desc.Flags));

    m_width = width;
    m_height = height;
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    CreateRTVs();
}

D3D12_CPU_DESCRIPTOR_HANDLE GraphicsDevice::CurrentBackBufferRTV() const {
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
    rtvHandle.Offset((int)m_frameIndex, m_rtvDescriptorSize);
    return rtvHandle;
}

void GraphicsDevice::BeginFrame(ID3D12PipelineState* initialPSO) {
    THROW_IF_FAILED(m_cmdAlloc[m_frameIndex]->Reset());
    THROW_IF_FAILED(m_cmdList->Reset(m_cmdAlloc[m_frameIndex].Get(), initialPSO));
}

void GraphicsDevice::EndFrameAndPresent(bool vsync) {
    THROW_IF_FAILED(m_cmdList->Close());

    ID3D12CommandList* lists[] = { m_cmdList.Get() };
    m_cmdQueue->ExecuteCommandLists(1, lists);

    // If the device was already removed earlier in the frame, fail with that reason (don't call Present)
    if (m_device) {
        HRESULT removed = m_device->GetDeviceRemovedReason();
        if (removed != S_OK) {
            char buf[320];
            sprintf_s(buf,
                "D3D12 device was removed before Present (reason=0x%08X). Check debug layer for earlier errors. %s:%d\n",
                (unsigned)removed, __FILE__, __LINE__);
            OutputDebugStringA(buf);
            throw std::runtime_error(buf);
        }
    }

    HRESULT hr = m_swapChain->Present(vsync ? 1 : 0, 0);
    if (FAILED(hr)) {
        if (hr == DXGI_ERROR_DEVICE_REMOVED && m_device) {
            HRESULT reason = m_device->GetDeviceRemovedReason();
            char buf[320];
            sprintf_s(buf,
                "D3D12 Present failed. DXGI_ERROR_DEVICE_REMOVED; device reason=0x%08X at %s:%d\n",
                (unsigned)reason, __FILE__, __LINE__);
            OutputDebugStringA(buf);
            throw std::runtime_error(buf);
        }
        ThrowIfFailed(hr, __FILE__, __LINE__);
    }
    MoveToNextFrame();
}

void GraphicsDevice::MoveToNextFrame() {
    const uint64_t currentFence = ++m_fenceValue[m_frameIndex];
    THROW_IF_FAILED(m_cmdQueue->Signal(m_fence.Get(), currentFence));

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    if (m_fence->GetCompletedValue() < m_fenceValue[m_frameIndex]) {
        THROW_IF_FAILED(m_fence->SetEventOnCompletion(m_fenceValue[m_frameIndex], m_fenceEvent));
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }
}

void GraphicsDevice::WaitForGpu() {
    if (!m_cmdQueue || !m_fence) { return; }

    const uint64_t fenceToWaitFor = ++m_fenceValue[m_frameIndex];
    THROW_IF_FAILED(m_cmdQueue->Signal(m_fence.Get(), fenceToWaitFor));
    THROW_IF_FAILED(m_fence->SetEventOnCompletion(fenceToWaitFor, m_fenceEvent));
    WaitForSingleObject(m_fenceEvent, INFINITE);
}
