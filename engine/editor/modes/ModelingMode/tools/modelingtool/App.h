#pragma once

#include <windows.h>
#include <windowsx.h>
#include "Engine.h"

struct EditableMesh;
struct RenderMesh;

class App {
public:
    void SetEngine(Engine* engine) { m_engine = engine; }
    void SetMeshes(EditableMesh* editMesh, RenderMesh* renderMesh) { m_editMesh = editMesh; m_renderMesh = renderMesh; }
    void SetWindow(HWND hwnd) { m_hwnd = hwnd; }

    void BeginFrameInput();
    void Update(float dt);
    void PumpMessages(MSG& msg);

    LRESULT HandleWindowMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool& handled);

private:
    struct InputState {
        int mouseX = 0;
        int mouseY = 0;

        bool lmbDown = false;
        bool lmbPressed = false;
        bool lmbReleased = false;

        bool rmbDown = false;
        bool rmbPressed = false;
        bool rmbReleased = false;

        int wheelDelta = 0;
    };

    Engine* m_engine = nullptr;
    EditableMesh* m_editMesh = nullptr;
    RenderMesh* m_renderMesh = nullptr;

    InputState m_input;

    bool m_isDragging = false;
    int m_selectedVertex = -1;
    POINT m_lastMousePos = { 0, 0 };
    HWND m_hwnd = nullptr;

    float m_panX = 0.0f;
    float m_panY = 0.0f;
    float m_zoom = 1.0f;

    bool m_colorsInit = false;
    DirectX::XMFLOAT4 m_baseColors[3];


    int HitTestVertex(int mouseX, int mouseY);
    void ScreenToNDC(int screenX, int screenY, float& ndcX, float& ndcY);
    void ScreenToWorld(int screenX, int screenY, float& worldX, float& worldY);
    void UpdateViewProj();
};
