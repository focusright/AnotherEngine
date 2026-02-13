#include "EditorCamera.h"
#include <algorithm>
#include <cmath>

using namespace DirectX;

static float Clamp(float v, float a, float b) { return (v < a) ? a : (v > b) ? b : v; }

void EditorCamera::SetLens(float fovYRadians, float nearZ, float farZ) {
    m_fovY = fovYRadians;
    m_nearZ = nearZ;
    m_farZ = farZ;
}

void EditorCamera::SetViewport(float width, float height) {
    m_viewW = (width > 1.0f) ? width : 1.0f;
    m_viewH = (height > 1.0f) ? height : 1.0f;
}

void EditorCamera::AddYawPitch(float yawDelta, float pitchDelta) {
    m_yaw += yawDelta;
    m_pitch += pitchDelta;

    // Prevent flip.
    const float limit = DirectX::XM_PIDIV2 - 0.001f;
    m_pitch = Clamp(m_pitch, -limit, limit);
}

void EditorCamera::MoveLocal(float forward, float right, float up) {
    XMVECTOR f;
    XMVECTOR r;
    XMVECTOR u;

    {
        const XMVECTOR forwardLocal = XMVectorSet(0, 0, 1, 0);
        const XMVECTOR rightLocal = XMVectorSet(1, 0, 0, 0);
        const XMVECTOR upLocal = XMVectorSet(0, 1, 0, 0);

        XMMATRIX rot = XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, 0.0f);
        f = XMVector3TransformNormal(forwardLocal, rot);
        r = XMVector3TransformNormal(rightLocal, rot);
        u = upLocal; // keep "world up" for editor feel
    }

    XMVECTOR pos = XMLoadFloat3(&m_position);
    pos = XMVectorAdd(pos, XMVectorScale(f, forward));
    pos = XMVectorAdd(pos, XMVectorScale(r, right));
    pos = XMVectorAdd(pos, XMVectorScale(u, up));

    XMStoreFloat3(&m_position, pos);
}

void EditorCamera::Pan(float dxPixels, float dyPixels) {
    // Pan in view plane: right and world-up.
    XMMATRIX rot = XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, 0.0f);
    XMVECTOR rightV = XMVector3TransformNormal(XMVectorSet(1, 0, 0, 0), rot);
    XMVECTOR upV = XMVectorSet(0, 1, 0, 0);

    // NOTE: Use (std::max)(...) to avoid Windows.h max macro collisions.
    float distance = (std::max)(1.0f, fabsf(m_position.z));
    float scale = panSpeed * distance;

    XMVECTOR delta = XMVectorScale(rightV, -dxPixels * scale);
    delta = XMVectorAdd(delta, XMVectorScale(upV, dyPixels * scale));

    XMVECTOR pos = XMLoadFloat3(&m_position);
    pos = XMVectorAdd(pos, delta);
    XMStoreFloat3(&m_position, pos);
}

void EditorCamera::Dolly(float wheelSteps) {
    XMMATRIX rot = XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, 0.0f);
    XMVECTOR forwardV = XMVector3TransformNormal(XMVectorSet(0, 0, 1, 0), rot);

    XMVECTOR pos = XMLoadFloat3(&m_position);
    pos = XMVectorAdd(pos, XMVectorScale(forwardV, wheelSteps * dollySpeed));
    XMStoreFloat3(&m_position, pos);
}

void EditorCamera::FocusOnPoints(const XMFLOAT3* points, int count) {
    if (!points || count <= 0) return;

    XMFLOAT3 c = { 0, 0, 0 };
    for (int i = 0; i < count; ++i) {
        c.x += points[i].x;
        c.y += points[i].y;
        c.z += points[i].z;
    }
    c.x /= count; c.y /= count; c.z /= count;

    float r = 0.0f;
    for (int i = 0; i < count; ++i) {
        float dx = points[i].x - c.x;
        float dy = points[i].y - c.y;
        float dz = points[i].z - c.z;
        r = (std::max)(r, std::sqrt(dx * dx + dy * dy + dz * dz));
    }
    r = (std::max)(r, 0.5f);

    // Distance so object fits in view vertically.
    float dist = r / std::tan(m_fovY * 0.5f);
    dist = (std::max)(dist, 1.0f);

    XMMATRIX rot = XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, 0.0f);
    XMVECTOR forwardV = XMVector3TransformNormal(XMVectorSet(0, 0, 1, 0), rot);

    XMVECTOR target = XMLoadFloat3(&c);
    XMVECTOR pos = XMVectorSubtract(target, XMVectorScale(forwardV, dist));

    XMStoreFloat3(&m_position, pos);
}

void EditorCamera::UpdateMatrices() {
    float aspect = (m_viewH > 0.0f) ? (m_viewW / m_viewH) : 1.0f;

    XMMATRIX rot = XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, 0.0f);
    XMVECTOR forwardV = XMVector3TransformNormal(XMVectorSet(0, 0, 1, 0), rot);
    XMVECTOR upV = XMVectorSet(0, 1, 0, 0);

    XMVECTOR pos = XMLoadFloat3(&m_position);
    XMMATRIX view = XMMatrixLookToLH(pos, forwardV, upV);
    XMMATRIX proj = XMMatrixPerspectiveFovLH(m_fovY, aspect, m_nearZ, m_farZ);
    XMMATRIX vp = XMMatrixMultiply(view, proj);

    XMStoreFloat4x4(&m_view, view);
    XMStoreFloat4x4(&m_proj, proj);
    XMStoreFloat4x4(&m_viewProj, vp);
}

XMFLOAT3 EditorCamera::Forward() const {
    XMMATRIX rot = XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, 0.0f);
    XMVECTOR forwardV = XMVector3TransformNormal(XMVectorSet(0, 0, 1, 0), rot);
    XMFLOAT3 f;
    XMStoreFloat3(&f, forwardV);
    return f;
}

XMFLOAT3 EditorCamera::Right() const {
    XMMATRIX rot = XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, 0.0f);
    XMVECTOR rightV = XMVector3TransformNormal(XMVectorSet(1, 0, 0, 0), rot);
    XMFLOAT3 r;
    XMStoreFloat3(&r, rightV);
    return r;
}

void EditorCamera::BuildRayFromScreen(float screenX, float screenY, XMFLOAT3& outOrigin, XMFLOAT3& outDir) const {
    float x = (2.0f * screenX / m_viewW) - 1.0f;
    float y = 1.0f - (2.0f * screenY / m_viewH);

    XMMATRIX invVP = XMMatrixInverse(nullptr, XMLoadFloat4x4(&m_viewProj));

    XMVECTOR nearP = XMVector3TransformCoord(XMVectorSet(x, y, 0.0f, 1.0f), invVP);
    XMVECTOR farP = XMVector3TransformCoord(XMVectorSet(x, y, 1.0f, 1.0f), invVP);

    XMVECTOR dir = XMVector3Normalize(XMVectorSubtract(farP, nearP));

    XMStoreFloat3(&outOrigin, nearP);
    XMStoreFloat3(&outDir, dir);
}
