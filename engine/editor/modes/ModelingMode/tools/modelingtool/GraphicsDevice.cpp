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
