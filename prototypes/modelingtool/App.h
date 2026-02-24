#pragma once

#include <windows.h>
#include <windowsx.h>
#include <DirectXMath.h>
#include <cstdint>
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

    uint32_t ObjectCount() const { return m_objectCount; }
    uint32_t ActiveObject() const { return m_activeObject; }

    void SetActiveObject(uint32_t index) {
        if (index < m_objectCount)
            m_activeObject = index;
    }

    bool AddObject(const DirectX::XMFLOAT3& pos);
    bool DuplicateActiveObject();
    bool DeleteActiveObject();

    bool SaveSceneAem(const wchar_t* path);
    bool LoadSceneAem(const wchar_t* path);

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
    DirectX::XMFLOAT4 m_baseColors[3] = {};

    EditorCamera m_camera;
    POINT m_lastCameraMouse = { 0, 0 };

    DirectX::XMFLOAT3 m_viewPivot = { 0.0f, 0.0f, 0.0f };
    float m_orbitDistance = 10.0f;

    EditorContext m_ctx;

    static const uint32_t kMaxObjects = 16;
    uint32_t m_objectCount = 2;
    uint32_t m_activeObject = 0;
    DirectX::XMFLOAT3 m_objectPos[kMaxObjects] = {};
    DirectX::XMFLOAT3 m_objectRot[kMaxObjects] = {};   // Euler radians (pitch,yaw,roll) for now
    DirectX::XMFLOAT3 m_objectScale[kMaxObjects] = {};
    DirectX::XMFLOAT4 m_objectColor[kMaxObjects] = {}; // per-object tint (saved)

    // Translate gizmo (world-space)
    static const uint32_t kGizmoVertexCount = 6; // 3 axes * 2 verts
    static constexpr float kGizmoAxisLen = 1.25f;
    static constexpr float kGizmoPickThresh = 0.15f;
    int m_gizmoActiveAxis = -1; // 0=X,1=Y,2=Z
    int m_gizmoHotAxis = -1;    // hover highlight (when not dragging)
    bool m_gizmoDragging = false;
    float m_gizmoDragT0 = 0.0f;
    DirectX::XMFLOAT3 m_gizmoStartPos = { 0,0,0 };

    int HitTestVertex(int mouseX, int mouseY);
    void UpdateGizmo();
    bool GizmoPickAxis(int mouseX, int mouseY, int& outAxis, float& outTOnAxis);
    bool GizmoComputeTOnAxis(int axis, int mouseX, int mouseY, float& outTOnAxis);
    bool ScreenToWorldOnZPlane(int screenX, int screenY, float& worldX, float& worldY);
    void UpdateViewProj();
    void FocusCamera();
    EditorCamera* Cam() { return m_ctx.camera; }
};
