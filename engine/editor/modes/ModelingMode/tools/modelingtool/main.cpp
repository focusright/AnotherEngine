#include <windows.h>
#include <windowsx.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <exception>
#include <vector>

#include "d3dx12.h"  // For CD3DX12 helper classes
#include "EditableMesh.h"
#include "RenderMesh.h"
#include "GraphicsDevice.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")

using Microsoft::WRL::ComPtr;
using namespace DirectX;

// Window dimensions
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

// Global variables
HWND g_hwnd = nullptr;
ComPtr<ID3D12RootSignature> g_rootSignature;
ComPtr<ID3D12PipelineState> g_pipelineState;
ComPtr<ID3D12Resource> g_vertexBuffer;

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
void UpdateVertexBuffer();
int HitTestVertex(int mouseX, int mouseY);
void ScreenToNDC(int screenX, int screenY, float& ndcX, float& ndcY);

void BeginFrameInput();   // clears edge flags
void Update(float dt);    // runs tool logic


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    try {
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

            // Skip render when minimized, not visible, or zero-sized to avoid DXGI_ERROR_INVALID_CALL at Present
            RECT clientRect = {};
            if (IsWindowVisible(g_hwnd) && !IsIconic(g_hwnd) &&
                GetClientRect(g_hwnd, &clientRect) &&
                (clientRect.right > clientRect.left) && (clientRect.bottom > clientRect.top)) {
                PopulateCommandList();
            } else {
                Sleep(10);
            }
        }

        g_gfx.Shutdown();
        return static_cast<int>(msg.wParam);
    } catch (const std::exception& e) {
        OutputDebugStringA("ModelingTool: ");
        OutputDebugStringA(e.what());
        int len = MultiByteToWideChar(CP_ACP, 0, e.what(), -1, nullptr, 0);
        if (len > 0) {
            std::vector<wchar_t> buf(static_cast<size_t>(len));
            MultiByteToWideChar(CP_ACP, 0, e.what(), -1, buf.data(), len);
            MessageBoxW(nullptr, buf.data(), L"ModelingTool Error", MB_OK | MB_ICONERROR);
        } else {
            MessageBoxW(nullptr, L"A fatal error occurred. See Output/Debug for details.", L"ModelingTool Error", MB_OK | MB_ICONERROR);
        }
        g_gfx.Shutdown();
        return 1;
    }
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
        case WM_SIZE: {
            RECT rc = {};
            if (GetClientRect(hwnd, &rc)) {
                UINT w = (UINT)(rc.right - rc.left);
                UINT h = (UINT)(rc.bottom - rc.top);
                if (g_gfx.Device())
                    g_gfx.Resize(w, h);
            }
            return 0;
        }
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
    if (!g_gfx.Init(g_hwnd, WINDOW_WIDTH, WINDOW_HEIGHT)) {
        MessageBox(g_hwnd, L"GraphicsDevice::Init failed.", L"Error", MB_OK);
        PostQuitMessage(0);
    }
}

void CreatePipelineState() {
    auto* device = g_gfx.Device();

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
    device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&g_rootSignature));

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
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

    // RasterizerState (defaults + your overrides)
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
    psoDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    psoDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    psoDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    psoDesc.RasterizerState.DepthClipEnable = TRUE;
    psoDesc.RasterizerState.MultisampleEnable = FALSE;
    psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
    psoDesc.RasterizerState.ForcedSampleCount = 0;
    psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    // BlendState (defaults)
    psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
    psoDesc.BlendState.IndependentBlendEnable = FALSE;

    D3D12_RENDER_TARGET_BLEND_DESC rtBlend = {};
    rtBlend.BlendEnable = FALSE;
    rtBlend.LogicOpEnable = FALSE;
    rtBlend.SrcBlend = D3D12_BLEND_ONE;
    rtBlend.DestBlend = D3D12_BLEND_ZERO;
    rtBlend.BlendOp = D3D12_BLEND_OP_ADD;
    rtBlend.SrcBlendAlpha = D3D12_BLEND_ONE;
    rtBlend.DestBlendAlpha = D3D12_BLEND_ZERO;
    rtBlend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    rtBlend.LogicOp = D3D12_LOGIC_OP_NOOP;
    rtBlend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    for (int i = 0; i < 8; i++) { psoDesc.BlendState.RenderTarget[i] = rtBlend; }

    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;

    if (FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&g_pipelineState)))) { return; }
}

void CreateVertexBuffer() {
    auto* device = g_gfx.Device();

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
    device->CreateCommittedResource(
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
    g_gfx.BeginFrame(g_pipelineState.Get());

    auto* cmdList = g_gfx.CmdList();
    auto* backBuffer = g_gfx.CurrentBackBuffer();
    auto rtvHandle = g_gfx.CurrentBackBufferRTV();

    cmdList->SetGraphicsRootSignature(g_rootSignature.Get());
    
    UINT w = g_gfx.Width(), h = g_gfx.Height();
    if (w == 0 || h == 0) return;
    D3D12_VIEWPORT viewport{
        0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h), 0.0f, 1.0f
    };
    D3D12_RECT scissorRect{ 0, 0, (LONG)w, (LONG)h };
    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissorRect);

    // Present -> RenderTarget
    {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            backBuffer,
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET
        );
        cmdList->ResourceBarrier(1, &barrier);
    }

    cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    const float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    D3D12_VERTEX_BUFFER_VIEW vertexBufferView{
        g_vertexBuffer->GetGPUVirtualAddress(),  // BufferLocation
        sizeof(Vertex) * 3,                      // SizeInBytes
        sizeof(Vertex)                           // StrideInBytes
    };
    cmdList->IASetVertexBuffers(0, 1, &vertexBufferView);
    cmdList->DrawInstanced(3, 1, 0, 0);

    // RenderTarget -> Present
    {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            backBuffer,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT
        );
        cmdList->ResourceBarrier(1, &barrier);
    }

    g_gfx.EndFrameAndPresent(true);
}

void UpdateVertexBuffer() {
    // Rebuild staging vertices from EditableMesh (truth)
    for (uint32_t i = 0; i < 3; ++i) {
        g_drawVertices[i].position = g_editMesh.GetVertex(i);
    }

    UINT8* pVertexDataBegin = nullptr;
    D3D12_RANGE readRange = { 0, 0 };
    g_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
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
    // Screen coordinates: (0,0) at top-left, (Width, Height) at bottom-right
    // NDC coordinates: (-1,-1) at bottom-left, (1,1) at top-right
    
    uint32_t w = g_gfx.Width(), h = g_gfx.Height();
    if (w == 0 || h == 0) { ndcX = ndcY = 0.0f; return; }
    ndcX = (2.0f * screenX / (float)w) - 1.0f;
    ndcY = 1.0f - (2.0f * screenY / (float)h); // Flip Y axis
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
