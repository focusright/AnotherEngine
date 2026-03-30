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

struct GizmoTarget {
    EditableMesh* editMesh = nullptr;
    uint32_t activeObject = UINT32_MAX;
    int selectedVertex = -1;
    DirectX::XMFLOAT3 objectPos = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 objectRot = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 objectScale = { 1.0f, 1.0f, 1.0f };
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

    void Update(Engine* engine, HWND hwnd, EditorCamera& camera, EditableMesh* editMesh, RenderMesh* renderMesh, uint32_t activeObject, int selectedVertex, const DirectX::XMFLOAT3& objectPos, const DirectX::XMFLOAT3& objectRot, const DirectX::XMFLOAT3& objectScale, bool rmbDown, bool rmbPressed, bool lmbDown, bool lmbPressed, bool lmbReleased, int mouseX, int mouseY, DirectX::XMFLOAT3& inOutObjectPos, bool& outRenderMeshDirty);

private:
    GizmoMode m_mode = GizmoMode::Translate;
    int m_activeAxis = -1;
    int m_hotAxis = -1;
    bool m_dragging = false;
    float m_dragT0 = 0.0f;
    DirectX::XMFLOAT3 m_startPos = { 0, 0, 0 };

    DirectX::XMFLOAT3 LocalVertexToWorld(const DirectX::XMFLOAT3& point, const DirectX::XMFLOAT3& objectPos, const DirectX::XMFLOAT3& objectRot, const DirectX::XMFLOAT3& objectScale) const;
    DirectX::XMFLOAT3 WorldPointToLocal(const DirectX::XMFLOAT3& point, const DirectX::XMFLOAT3& objectPos, const DirectX::XMFLOAT3& objectRot, const DirectX::XMFLOAT3& objectScale) const;
    DirectX::XMFLOAT3 GetOrigin(const GizmoTarget& target) const;
    void BuildVertices(Vertex* gizmoVerts, HWND hwnd, EditorCamera& camera, const GizmoTarget& target);
};