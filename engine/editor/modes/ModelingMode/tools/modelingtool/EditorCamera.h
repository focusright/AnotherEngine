#pragma once

#include <DirectXMath.h>

class EditorCamera {
public:
    void SetLens(float fovYRadians, float nearZ, float farZ);
    void SetViewport(float width, float height);

    void SetPosition(const DirectX::XMFLOAT3& p) { m_position = p; }
    DirectX::XMFLOAT3 Position() const { return m_position; }

    void AddYawPitch(float yawDelta, float pitchDelta);
    void MoveLocal(float forward, float right, float up);

    // Set yaw/pitch so the camera looks at a world-space target.
    void LookAt(const DirectX::XMFLOAT3& target);
    void Pan(float dxPixels, float dyPixels);
    void Dolly(float wheelSteps);

    void FocusOnPoints(const DirectX::XMFLOAT3* points, int count);

    void UpdateMatrices();
    const DirectX::XMFLOAT4X4& ViewProj() const { return m_viewProj; }

    DirectX::XMFLOAT3 Forward() const;
    DirectX::XMFLOAT3 Right() const;

    float Yaw() const { return m_yaw; }
    float Pitch() const { return m_pitch; }

    // Ray from screen pixel into world (for picking/dragging).
    void BuildRayFromScreen(float screenX, float screenY, DirectX::XMFLOAT3& outOrigin, DirectX::XMFLOAT3& outDir) const;

    // Tunables
    float moveSpeed = 2.0f;          // units/sec
    float mouseSensitivity = 0.003f; // radians per pixel
    float panSpeed = 0.002f;         // units per pixel (scaled)
    float dollySpeed = 1.0f;         // units per wheel step

private:
    DirectX::XMFLOAT3 m_position = { 0.0f, 2.0f, -3.0f };
    float m_yaw = 0.0f;
    float m_pitch = 0.0f;

    float m_fovY = DirectX::XM_PIDIV4;
    float m_nearZ = 0.1f;
    float m_farZ = 1000.0f;

    float m_viewW = 1280.0f;
    float m_viewH = 720.0f;

    DirectX::XMFLOAT4X4 m_view = {};
    DirectX::XMFLOAT4X4 m_proj = {};
    DirectX::XMFLOAT4X4 m_viewProj = {};
};
