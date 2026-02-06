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
#include "Engine.h"
#include "App.h"

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
ComPtr<ID3D12Fence> g_fence;
UINT64 g_fenceValue;
HANDLE g_fenceEvent;
GraphicsDevice g_gfx;
Engine g_engine;
App g_app;

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

// Forward declarations
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void InitializeWindow(HINSTANCE hInstance);
void InitializeDirect3D();
void CreatePipelineState();
void CreateVertexBuffer();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    InitializeWindow(hInstance);
    g_app.SetWindow(g_hwnd);
    
    InitializeDirect3D();
    CreatePipelineState();
    CreateVertexBuffer();

    g_engine.SetGraphicsDevice(&g_gfx);
    g_app.SetEngine(&g_engine);
    g_app.SetMeshes(&g_editMesh, &g_renderMesh);
    g_engine.SetRenderObjects(g_commandAllocator.Get(), g_commandList.Get(), g_rootSignature.Get(), g_pipelineState.Get(), g_fence.Get(), g_fenceEvent, &g_fenceValue, g_vertexBuffer.Get(), WINDOW_WIDTH, WINDOW_HEIGHT);

    LARGE_INTEGER freq;
    LARGE_INTEGER prev;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&prev);

    MSG msg = {}; // Main message loop
    while (msg.message != WM_QUIT) {
        g_app.BeginFrameInput();
        g_app.PumpMessages(msg);

        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        float dt = float(double(now.QuadPart - prev.QuadPart) / double(freq.QuadPart));
        prev = now;

        g_app.Update(0.0f);
        g_engine.RenderFrame();
    }

    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    bool handled = false;
    LRESULT appResult = g_app.HandleWindowMessage(hwnd, uMsg, wParam, lParam, handled);
    if (handled) return appResult;

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

    g_gfx.SetFrameIndexFromSwapChain();

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
    // Create root signature (1 root param: 16 32-bit constants for a 4x4 matrix at b0)
    CD3DX12_ROOT_PARAMETER rootParams[1];
    rootParams[0].InitAsConstants(16, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(_countof(rootParams), rootParams, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    Microsoft::WRL::ComPtr<ID3DBlob> signature;
    Microsoft::WRL::ComPtr<ID3DBlob> error;
    if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error))) {
        if (error) { OutputDebugStringA(static_cast<char*>(error->GetBufferPointer())); }
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
        cbuffer CameraCB : register(b0) {
            row_major float4x4 uViewProj;
        };

        struct PSInput {
            float4 position : SV_POSITION;
            float4 color : COLOR;
        };

        PSInput VSMain(float3 position : POSITION, float4 color : COLOR) {
            PSInput result;
            result.position = mul(float4(position, 1.0f), uViewProj);
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

    for (uint32_t i = 0; i < 3; ++i) {
        g_renderMesh.drawVertices[i] = triangleVertices[i];
        g_renderMesh.drawVertices[i].position = g_editMesh.GetVertex(i);
    }

    const UINT vertexBufferSize = sizeof(g_renderMesh.drawVertices);

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
    memcpy(pVertexDataBegin, g_renderMesh.drawVertices, sizeof(g_renderMesh.drawVertices));
    g_vertexBuffer->Unmap(0, nullptr);

    g_renderMesh.dirty = false;
}
