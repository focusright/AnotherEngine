#pragma once

#include <windows.h>
#include <windowsx.h>
#include "Engine.h"
#include "EditorCamera.h"
#include "EditorContext.h"

struct EditableMesh;
struct RenderMesh;

class App {
public:
    App(EditorCamera& camera);

    EditorContext& Ctx() { return m_ctx; }
    void SetEngine(Engine* engine) { m_engine = engine; }
    void SetMeshes(EditableMesh* editMesh, RenderMesh* renderMesh) { m_editMesh = editMesh; m_renderMesh = renderMesh; }
    void SetWindow(HWND hwnd) { m_hwnd = hwnd; m_ctx.hwnd = hwnd; }

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

        bool mmbDown = false;
        bool mmbPressed = false;
        bool mmbReleased = false;

        bool rmbDown = false;
        bool rmbPressed = false;
        bool rmbReleased = false;

        int wheelDelta = 0;

        bool keys[256] = {};
        bool fPressed = false;
    };

    Engine* m_engine = nullptr;
    EditableMesh* m_editMesh = nullptr;
    RenderMesh* m_renderMesh = nullptr;

    InputState m_input;

    bool m_isDragging = false;
    int m_selectedVertex = -1;
    POINT m_lastMousePos = { 0, 0 };
    HWND m_hwnd = nullptr;

    bool m_colorsInit = false;
    DirectX::XMFLOAT4 m_baseColors[3];

    EditorCamera m_camera;
    POINT m_lastCameraMouse = { 0, 0 };

    EditorContext m_ctx;

    int HitTestVertex(int mouseX, int mouseY);
    bool ScreenToWorldOnZPlane(int screenX, int screenY, float& worldX, float& worldY);
    void UpdateViewProj();
    void FocusCamera();
    EditorCamera* Cam() { return m_ctx.camera; }
};
