#pragma once

#include <windows.h>
#include <DirectXMath.h>
#include "Engine.h"

struct EditableMesh;
struct RenderMesh;
class EditorCamera;

enum class GizmoMode {
    Translate = 0,
    Scale,
    Rotate
};

struct GizmoTransformRef {
    DirectX::XMFLOAT3* pos = nullptr;
    DirectX::XMFLOAT3* rot = nullptr;
    DirectX::XMFLOAT3* scale = nullptr;
};

struct GizmoTarget {
    EditableMesh* editMesh = nullptr;
    uint32_t activeObject = UINT32_MAX;
    int selectedVertex = -1;
    GizmoTransformRef transform = {};
};

struct GizmoUpdateArgs {
    Engine* engine = nullptr;
    HWND hwnd = nullptr;
    EditorCamera* camera = nullptr;
    GizmoTarget target = {};
    bool rmbDown = false;
    bool rmbPressed = false;
    bool lmbDown = false;
    bool lmbPressed = false;
    bool lmbReleased = false;
    int mouseX = 0;
    int mouseY = 0;
    bool* outRenderMeshDirty = nullptr;
};

class Gizmo {
public:
    static const uint32_t kVertexCount = 18;
    static constexpr float kAxisLen = 1.25f;
    static constexpr float kPickThresh = 0.15f;

    void Reset();
    void SetMode(GizmoMode mode);
    GizmoMode GetMode() const { return m_mode; }
    const char* GetModeName() const;

    bool IsDragging() const { return m_dragging; }
    int ActiveAxis() const { return m_activeAxis; }
    int HotAxis() const { return m_hotAxis; }

    bool PickAxis(EditorCamera& camera, const GizmoTarget& target, int mouseX, int mouseY, int& outAxis, float& outTOnAxis);
    bool ComputeTOnAxis(EditorCamera& camera, const GizmoTarget& target, int axis, int mouseX, int mouseY, float& outTOnAxis);

    void Update(const GizmoUpdateArgs& args);

private:
private:
    GizmoMode m_mode = GizmoMode::Translate;
    int m_activeAxis = -1; //0 = X, 1 = Y, 2 = Z, -1 = none
    int m_hotAxis = -1;
    bool m_dragging = false;
    float m_dragT0 = 0.0f;
    DirectX::XMFLOAT3 m_startPos = { 0, 0, 0 };
    DirectX::XMFLOAT3 m_startScale = { 1, 1, 1 };
    DirectX::XMFLOAT3 m_startRot = { 0, 0, 0 }; //The object’s Euler rotation
    int m_dragStartMouseX = 0;

    DirectX::XMFLOAT3 LocalVertexToWorld(const DirectX::XMFLOAT3& point, const DirectX::XMFLOAT3& objectPos, const DirectX::XMFLOAT3& objectRot, const DirectX::XMFLOAT3& objectScale) const;
    DirectX::XMFLOAT3 WorldPointToLocal(const DirectX::XMFLOAT3& point, const DirectX::XMFLOAT3& objectPos, const DirectX::XMFLOAT3& objectRot, const DirectX::XMFLOAT3& objectScale) const;
    DirectX::XMFLOAT3 GetOrigin(const GizmoTarget& target) const;
    void BuildVertices(Vertex* gizmoVerts, HWND hwnd, EditorCamera& camera, const GizmoTarget& target);
};