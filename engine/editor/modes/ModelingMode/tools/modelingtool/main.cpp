#include <windows.h>
#include <windowsx.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <cwchar>

#include "d3dx12.h"
#include "EditableMesh.h"
#include "RenderMesh.h"
#include "GraphicsDevice.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using Microsoft::WRL::ComPtr;
using namespace DirectX;

// Window dimensions
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

// Global variables
HWND g_hwnd = nullptr;
ComPtr<ID3D12Device> g_device;
ComPtr<ID3D12CommandAllocator> g_commandAllocator;
ComPtr<ID3D12GraphicsCommandList> g_commandList;
ComPtr<ID3D12RootSignature> g_rootSignature;
ComPtr<ID3D12PipelineState> g_pipelineState;
ComPtr<ID3D12Resource> g_vertexBuffer;
UINT g_frameIndex;
ComPtr<ID3D12Fence> g_fence;
UINT64 g_fenceValue;
HANDLE g_fenceEvent;

GraphicsDevice g_gfx;

// Vertex structure
struct Vertex {
    XMFLOAT3 position;
    XMFLOAT4 color;
};

struct InputState {
    int mouseX = 0;
    int mouseY = 0;

    bool lmbDown = false;      // current state
    bool lmbPressed = false;   // edge: went down this frame
    bool lmbReleased = false;  // edge: went up this frame
};

InputState g_input;


// Mouse interaction variables
bool g_isDragging = false;
int g_selectedVertex = -1;
POINT g_lastMousePos = {0, 0};
//Vertex g_vertices[3];  // Store vertices for easy manipulation

EditableMesh g_editMesh;
RenderMesh   g_renderMesh;
Vertex       g_drawVertices[3]; // staging array used only for upload (positions+colors)


// Forward declarations
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void InitializeWindow(HINSTANCE hInstance);
void InitializeDirect3D();
void CreatePipelineState();
void CreateVertexBuffer();
void PopulateCommandList();
void WaitForGpu();
void MoveToNextFrame();
void UpdateVertexBuffer();
int HitTestVertex(int mouseX, int mouseY);
void ScreenToNDC(int screenX, int screenY, float& ndcX, float& ndcY);

void BeginFrameInput();   // clears edge flags
void Update(float dt);    // runs tool logic


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    InitializeWindow(hInstance);
    InitializeDirect3D();
    CreatePipelineState();
    CreateVertexBuffer();

    // Main message loop
    MSG msg = {};
    while (msg.message != WM_QUIT) {
        BeginFrameInput(); // clear edge flags at start of frame

        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        Update(0.0f); // dt later

        PopulateCommandList();
        g_gfx.SwapChain()->Present(1, 0);
        MoveToNextFrame();
    }

    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                PostQuitMessage(0);
                return 0;
            }
            break;
        case WM_LBUTTONDOWN: {
            g_input.lmbDown = true;
            g_input.lmbPressed = true;

            g_input.mouseX = GET_X_LPARAM(lParam);
            g_input.mouseY = GET_Y_LPARAM(lParam);

            SetCapture(hwnd);
            return 0;
        }
        case WM_LBUTTONUP: {
            g_input.lmbDown = false;
            g_input.lmbReleased = true;

            g_input.mouseX = GET_X_LPARAM(lParam);
            g_input.mouseY = GET_Y_LPARAM(lParam);

            ReleaseCapture();
            return 0;
        }
        case WM_MOUSEMOVE: {
            g_input.mouseX = GET_X_LPARAM(lParam);
            g_input.mouseY = GET_Y_LPARAM(lParam);
            return 0;
        }
        case WM_SETCURSOR:
            SetCursor(LoadCursor(nullptr, IDC_ARROW));
            return TRUE;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void InitializeWindow(HINSTANCE hInstance) {
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"ModelingTool";
    RegisterClassEx(&wc);

    RECT windowRect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    // Center the window on the screen
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;
    int x = (screenWidth - windowWidth) / 2;
    int y = (screenHeight - windowHeight) / 2;

    g_hwnd = CreateWindow(
        L"ModelingTool",
        L"Modeling Tool",
        WS_OVERLAPPEDWINDOW,
        x, y, // Centered position
        windowWidth,
        windowHeight,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    ShowWindow(g_hwnd, SW_SHOW);
}

void InitializeDirect3D() {
    UINT dxgiFactoryFlags = 0;
#ifdef _DEBUG
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();
        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }
#endif

    ComPtr<IDXGIFactory6> factory;
    CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory));

    ComPtr<IDXGIAdapter1> hardwareAdapter;
    for (UINT adapterIndex = 0; SUCCEEDED(factory->EnumAdapters1(adapterIndex, &hardwareAdapter)); ++adapterIndex) {
        DXGI_ADAPTER_DESC1 desc;
        hardwareAdapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
        if (SUCCEEDED(D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&g_device)))) break;
    }

    if (!g_gfx.CreateCommandQueue(g_device.Get())) {
        MessageBox(g_hwnd, L"CreateCommandQueue failed.", L"Error", MB_OK);
        PostQuitMessage(0);
        return;
    }
    if (!g_gfx.Queue()) {
        MessageBox(g_hwnd, L"Queue is null after creation.", L"Error", MB_OK);
        PostQuitMessage(0);
        return;
    }

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = 2;
    swapChainDesc.Width = WINDOW_WIDTH;
    swapChainDesc.Height = WINDOW_HEIGHT;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    if (!g_gfx.CreateSwapChain(factory.Get(), g_hwnd, WINDOW_WIDTH, WINDOW_HEIGHT, DXGI_FORMAT_R8G8B8A8_UNORM, 2)) {
        MessageBox(g_hwnd, L"CreateSwapChain failed.", L"Error", MB_OK);
        PostQuitMessage(0);
        return;
    }

    g_frameIndex = g_gfx.SwapChain()->GetCurrentBackBufferIndex();

    if (!g_gfx.CreateRTVs(g_device.Get(), DXGI_FORMAT_R8G8B8A8_UNORM, 2)) {
        MessageBox(g_hwnd, L"CreateRTVs failed.", L"Error", MB_OK);
        PostQuitMessage(0);
        return;
    }

    g_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_commandAllocator));
    g_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&g_commandList));
    g_commandList->Close();

    g_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence));
    g_fenceValue = 1;
    g_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

void CreatePipelineState() {
    // Create root signature
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    Microsoft::WRL::ComPtr<ID3DBlob> signature;
    Microsoft::WRL::ComPtr<ID3DBlob> error;
    if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error))) {
        if (error) {
            OutputDebugStringA(static_cast<char*>(error->GetBufferPointer()));
        }
        return;
    }
    g_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&g_rootSignature));

    // Create pipeline state
    ComPtr<ID3DBlob> vertexShader;
    ComPtr<ID3DBlob> pixelShader;
    UINT compileFlags = 0;
#ifdef _DEBUG
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    const char* vertexShaderSource = R"(
        struct PSInput {
            float4 position : SV_POSITION;
            float4 color : COLOR;
        };

        PSInput VSMain(float3 position : POSITION, float4 color : COLOR) {
            PSInput result;
            result.position = float4(position, 1.0f);
            result.color = color;
            return result;
        }
    )";

    const char* pixelShaderSource = R"(
        struct PSInput {
            float4 position : SV_POSITION;
            float4 color : COLOR;
        };

        float4 PSMain(PSInput input) : SV_TARGET {
            return input.color;
        }
    )";

    if (FAILED(D3DCompile(vertexShaderSource, strlen(vertexShaderSource), nullptr, nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, &error))) {
        if (error) {
            OutputDebugStringA(static_cast<char*>(error->GetBufferPointer()));
        }
        return;
    }

    if (FAILED(D3DCompile(pixelShaderSource, strlen(pixelShaderSource), nullptr, nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, &error))) {
        if (error) {
            OutputDebugStringA(static_cast<char*>(error->GetBufferPointer()));
        }
        return;
    }

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = g_rootSignature.Get();
    psoDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
    psoDesc.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;

    if (FAILED(g_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&g_pipelineState)))) {
        return;
    }
}

void CreateVertexBuffer() {
    // Create vertex buffer
    const float scale = 0.5f; // 50% of viewport
    Vertex triangleVertices[] = {
        { XMFLOAT3(0.0f, scale, 0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },      // Top (Red)
        { XMFLOAT3(-scale, -scale, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },   // Bottom left (Green)
        { XMFLOAT3(scale, -scale, 0.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }     // Bottom right (Blue)
    };

    // Truth lives in EditableMesh (positions only)
    for (uint32_t i = 0; i < 3; ++i) {
        g_editMesh.SetVertex(i, triangleVertices[i].position);
    }

    // GPU staging uses colors + positions pulled from edit mesh
    for (uint32_t i = 0; i < 3; ++i) {
        g_drawVertices[i] = triangleVertices[i];
        g_drawVertices[i].position = g_editMesh.GetVertex(i);
    }

    const UINT vertexBufferSize = sizeof(g_drawVertices);

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
    g_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&g_vertexBuffer)
    );

    // initial upload
    UINT8* pVertexDataBegin = nullptr;
    D3D12_RANGE readRange = { 0, 0 };
    g_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
    memcpy(pVertexDataBegin, g_drawVertices, sizeof(g_drawVertices));
    g_vertexBuffer->Unmap(0, nullptr);

    g_renderMesh.dirty = false;
}

void PopulateCommandList() {
    g_commandAllocator->Reset();
    g_commandList->Reset(g_commandAllocator.Get(), g_pipelineState.Get());

    g_commandList->SetGraphicsRootSignature(g_rootSignature.Get());
    
    D3D12_VIEWPORT viewport{
        0.0f,                                    // TopLeftX
        0.0f,                                    // TopLeftY
        static_cast<float>(WINDOW_WIDTH),        // Width
        static_cast<float>(WINDOW_HEIGHT),       // Height
        0.0f,                                    // MinDepth
        1.0f                                     // MaxDepth
    };
    D3D12_RECT scissorRect{
        0,              // left
        0,              // top
        WINDOW_WIDTH,   // right
        WINDOW_HEIGHT   // bottom
    };
    g_commandList->RSSetViewports(1, &viewport);
    g_commandList->RSSetScissorRects(1, &scissorRect);

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(g_gfx.BackBuffer(g_frameIndex), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

    g_commandList->ResourceBarrier(1, &barrier);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = g_gfx.RTV(g_frameIndex);
    g_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    const float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    g_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

    g_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView{
        g_vertexBuffer->GetGPUVirtualAddress(),  // BufferLocation
        sizeof(Vertex) * 3,                      // SizeInBytes
        sizeof(Vertex)                           // StrideInBytes
    };
    g_commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
    g_commandList->DrawInstanced(3, 1, 0, 0);

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(g_gfx.BackBuffer(g_frameIndex), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

    g_commandList->ResourceBarrier(1, &barrier);
    g_commandList->Close();

    ID3D12CommandList* ppCommandLists[] = { g_commandList.Get() };
    g_gfx.Queue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
}

void WaitForGpu() {
    const UINT64 fence = g_fenceValue;
    g_gfx.Queue()->Signal(g_fence.Get(), fence);
    g_fenceValue++;

    if (g_fence->GetCompletedValue() < fence) {
        g_fence->SetEventOnCompletion(fence, g_fenceEvent);
        WaitForSingleObject(g_fenceEvent, INFINITE);
    }
}

void MoveToNextFrame() {
    const UINT64 fenceValue = ++g_fenceValue;
    g_gfx.Queue()->Signal(g_fence.Get(), fenceValue);

    if (g_fence->GetCompletedValue() < fenceValue) {
        g_fence->SetEventOnCompletion(fenceValue, g_fenceEvent);
        WaitForSingleObject(g_fenceEvent, INFINITE);
    }

    g_frameIndex = g_gfx.SwapChain()->GetCurrentBackBufferIndex();
}

void UpdateVertexBuffer() {
    if (!g_vertexBuffer) { return; }

    for (uint32_t i = 0; i < 3; ++i) {
        g_drawVertices[i].position = g_editMesh.GetVertex(i);
    }

    UINT8* pVertexDataBegin = nullptr;
    D3D12_RANGE readRange = { 0, 0 };

    HRESULT hr = g_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
    if (FAILED(hr) || !pVertexDataBegin) {
        wchar_t buf[256];
        swprintf_s(buf, L"VertexBuffer Map failed. hr=0x%08X", (unsigned)hr);
        MessageBox(g_hwnd, buf, L"Error", MB_OK);
        return;
    }

    memcpy(pVertexDataBegin, g_drawVertices, sizeof(g_drawVertices));
    g_vertexBuffer->Unmap(0, nullptr);
}


int HitTestVertex(int mouseX, int mouseY) {
    float ndcX, ndcY;
    ScreenToNDC(mouseX, mouseY, ndcX, ndcY);
    
    const float hitRadius = 0.05f; // Hit radius in NDC space
    
    for (int i = 0; i < 3; i++) {
        XMFLOAT3 p = g_editMesh.GetVertex((VertexID)i);
        float dx = ndcX - p.x;
        float dy = ndcY - p.y;
        float distance = sqrtf(dx * dx + dy * dy);
        if (distance < hitRadius) return i;
    }
    
    return -1; // No vertex hit
}

void ScreenToNDC(int screenX, int screenY, float& ndcX, float& ndcY) {
    // Convert screen coordinates to NDC (Normalized Device Coordinates)
    // Screen coordinates: (0,0) at top-left, (WINDOW_WIDTH, WINDOW_HEIGHT) at bottom-right
    // NDC coordinates: (-1,-1) at bottom-left, (1,1) at top-right
    
    ndcX = (2.0f * screenX / WINDOW_WIDTH) - 1.0f;
    ndcY = 1.0f - (2.0f * screenY / WINDOW_HEIGHT); // Flip Y axis
}

void BeginFrameInput() {
    g_input.lmbPressed = false;
    g_input.lmbReleased = false;
}

void Update(float /*dt*/) {
    // 1) On press: select a vertex
    if (g_input.lmbPressed) {
        g_selectedVertex = HitTestVertex(g_input.mouseX, g_input.mouseY);
        g_isDragging = (g_selectedVertex != -1);
        g_lastMousePos = { g_input.mouseX, g_input.mouseY };
    }

    // 2) On release: stop dragging
    if (g_input.lmbReleased) {
        g_isDragging = false;
        g_selectedVertex = -1;
    }

    // 3) While dragging: edit the mesh (truth), mark dirty
    if (g_input.lmbDown && g_isDragging && g_selectedVertex != -1) {
        float ndcX, ndcY;
        ScreenToNDC(g_input.mouseX, g_input.mouseY, ndcX, ndcY);

        // Authoring mesh is truth
        XMFLOAT3 p = g_editMesh.GetVertex((VertexID)g_selectedVertex);
        p.x = ndcX;
        p.y = ndcY;
        g_editMesh.SetVertex((VertexID)g_selectedVertex, p);

        g_renderMesh.dirty = true;
    }

    // 4) Upload once per frame (not inside WindowProc)
    if (g_renderMesh.dirty) {
        UpdateVertexBuffer();
        g_renderMesh.dirty = false;
    }
}
