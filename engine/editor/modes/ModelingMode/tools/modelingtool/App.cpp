#include "App.h"
#include "EditableMesh.h"
#include "RenderMesh.h"
#include <DirectXMath.h>

using namespace DirectX;

void App::BeginFrameInput() {
    m_input.lmbPressed = false;
    m_input.lmbReleased = false;
    m_input.rmbPressed = false;
    m_input.rmbReleased = false;
    m_input.mmbPressed = false;
    m_input.mmbReleased = false;
    m_input.wheelDelta = 0;
    m_input.fPressed = false;
}

void App::Update(float dt) {
    if (!m_editMesh || !m_renderMesh || !m_engine) return;

    if (!m_colorsInit) {
        for (int i = 0; i < 3; ++i) { m_baseColors[i] = m_renderMesh->drawVertices[i].color; }
        m_colorsInit = true;
    }

    // Update viewport for camera.
    RECT rc;
    GetClientRect(m_hwnd, &rc);
    float width = float(rc.right - rc.left);
    float height = float(rc.bottom - rc.top);
    m_camera.SetViewport(width, height);
    m_camera.SetLens(DirectX::XM_PIDIV4, 0.1f, 1000.0f);

    // Mouse-look (RMB) - only if not dragging a vertex.
    if (m_input.rmbPressed) {
        m_lastCameraMouse = { m_input.mouseX, m_input.mouseY };
    }
    if (m_input.rmbDown && !m_isDragging) {
        int dx = m_input.mouseX - m_lastCameraMouse.x;
        int dy = m_input.mouseY - m_lastCameraMouse.y;

        m_camera.AddYawPitch(dx * m_camera.mouseSensitivity, dy * m_camera.mouseSensitivity);

        m_lastCameraMouse = { m_input.mouseX, m_input.mouseY };
    }

    // Pan (MMB) - per your requirement.
    if (m_input.mmbPressed) {
        m_lastCameraMouse = { m_input.mouseX, m_input.mouseY };
    }
    if (m_input.mmbDown && !m_isDragging) {
        int dx = m_input.mouseX - m_lastCameraMouse.x;
        int dy = m_input.mouseY - m_lastCameraMouse.y;

        m_camera.Pan(float(dx), float(dy));

        m_lastCameraMouse = { m_input.mouseX, m_input.mouseY };
    }

    // Dolly (wheel).
    if (m_input.wheelDelta != 0 && !m_isDragging) {
        float steps = float(m_input.wheelDelta) / 120.0f;
        m_camera.Dolly(steps);
    }

    // WASD move only while RMB is held (Unreal-ish).
    if (m_input.rmbDown && !m_isDragging) {
        float f = 0.0f;
        float r = 0.0f;

        if (m_input.keys['W']) f += 1.0f;
        if (m_input.keys['S']) f -= 1.0f;
        if (m_input.keys['D']) r += 1.0f;
        if (m_input.keys['A']) r -= 1.0f;

        if (f != 0.0f || r != 0.0f) {
            float step = m_camera.moveSpeed * dt;
            m_camera.MoveLocal(f * step, r * step, 0.0f);
        }
    }

    // Focus.
    if (m_input.fPressed && !m_isDragging) {
        FocusCamera();
    }

    // 1) On press: select a vertex
    if (m_input.lmbPressed) {
        m_selectedVertex = HitTestVertex(m_input.mouseX, m_input.mouseY);
        m_isDragging = (m_selectedVertex != -1);

        for (int i = 0; i < 3; ++i) { m_renderMesh->drawVertices[i].color = m_baseColors[i]; }
        if (m_selectedVertex != -1) { m_renderMesh->drawVertices[m_selectedVertex].color = DirectX::XMFLOAT4(1, 1, 1, 1); }
        m_renderMesh->dirty = true;
    }

    // 2) On release: stop dragging
    if (m_input.lmbReleased) {
        m_isDragging = false;
        m_selectedVertex = -1;

        for (int i = 0; i < 3; ++i) { m_renderMesh->drawVertices[i].color = m_baseColors[i]; }
        m_renderMesh->dirty = true;
    }

    // 3) While dragging: move the vertex on Z=0 plane
    if (m_input.lmbDown && m_isDragging && m_selectedVertex != -1) {
        float worldX = 0.0f, worldY = 0.0f;
        if (ScreenToWorldOnZPlane(m_input.mouseX, m_input.mouseY, worldX, worldY)) {
            DirectX::XMFLOAT3 p = m_editMesh->GetVertex((VertexID)m_selectedVertex);
            p.x = worldX;
            p.y = worldY;
            m_editMesh->SetVertex((VertexID)m_selectedVertex, p);
            m_renderMesh->dirty = true;
        }
    }

    UpdateViewProj();

    if (m_renderMesh->dirty) {
        m_engine->UpdateVertexBuffer(m_editMesh, m_renderMesh, m_hwnd);
        m_renderMesh->dirty = false;
    }
}

void App::PumpMessages(MSG& msg) {
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

LRESULT App::HandleWindowMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool& handled) {
    handled = false;

    switch (uMsg) {
        case WM_LBUTTONDOWN: {
            m_input.lmbDown = true;
            m_input.lmbPressed = true;
            m_input.mouseX = GET_X_LPARAM(lParam);
            m_input.mouseY = GET_Y_LPARAM(lParam);
            SetCapture(hwnd);
            handled = true;
            return 0;
        }
        case WM_LBUTTONUP: {
            m_input.lmbDown = false;
            m_input.lmbReleased = true;
            m_input.mouseX = GET_X_LPARAM(lParam);
            m_input.mouseY = GET_Y_LPARAM(lParam);
            ReleaseCapture();
            handled = true;
            return 0;
        }
        case WM_MOUSEMOVE: {
            m_input.mouseX = GET_X_LPARAM(lParam);
            m_input.mouseY = GET_Y_LPARAM(lParam);
            handled = true;
            return 0;
        }
        case WM_SETCURSOR: {
            SetCursor(LoadCursor(nullptr, IDC_ARROW));
            handled = true;
            return TRUE;
        }
        case WM_RBUTTONDOWN: {
            m_input.rmbDown = true;
            m_input.rmbPressed = true;
            m_input.mouseX = GET_X_LPARAM(lParam);
            m_input.mouseY = GET_Y_LPARAM(lParam);
            SetCapture(hwnd);
            handled = true;
            return 0;
        }
        case WM_RBUTTONUP: {
            m_input.rmbDown = false;
            m_input.rmbReleased = true;
            m_input.mouseX = GET_X_LPARAM(lParam);
            m_input.mouseY = GET_Y_LPARAM(lParam);
            ReleaseCapture();
            handled = true;
            return 0;
        }
        case WM_MOUSEWHEEL: {
            m_input.wheelDelta += GET_WHEEL_DELTA_WPARAM(wParam);
            handled = true;
            return 0;
        }
        case WM_MBUTTONDOWN: {
            m_input.mmbDown = true;
            m_input.mmbPressed = true;
            m_input.mouseX = GET_X_LPARAM(lParam);
            m_input.mouseY = GET_Y_LPARAM(lParam);
            SetCapture(hwnd);
            handled = true;
            return 0;
        }
        case WM_MBUTTONUP: {
            m_input.mmbDown = false;
            m_input.mmbReleased = true;
            m_input.mouseX = GET_X_LPARAM(lParam);
            m_input.mouseY = GET_Y_LPARAM(lParam);
            ReleaseCapture();
            handled = true;
            return 0;
        }
        case WM_KEYDOWN: {
            if (wParam < 256) {
                bool wasDown = (lParam & (1 << 30)) != 0;
                m_input.keys[wParam] = true;
                if (!wasDown && (wParam == 'F')) { m_input.fPressed = true; }
            }
            handled = true;
            return 0;
        }
        case WM_KEYUP: {
            if (wParam < 256) { m_input.keys[wParam] = false; }
            handled = true;
            return 0;
        }
    }

    return 0;
}

int App::HitTestVertex(int mouseX, int mouseY) {
    if (!m_hwnd) return -1;

    float wx0 = 0.0f, wy0 = 0.0f;
    float wx1 = 0.0f, wy1 = 0.0f;

    if (!ScreenToWorldOnZPlane(mouseX, mouseY, wx0, wy0)) return -1;
    if (!ScreenToWorldOnZPlane(mouseX + 10, mouseY, wx1, wy1)) return -1;

    float hitRadius = sqrtf((wx1 - wx0) * (wx1 - wx0) + (wy1 - wy0) * (wy1 - wy0));

    for (int i = 0; i < 3; ++i) {
        DirectX::XMFLOAT3 p = m_editMesh->GetVertex((VertexID)i);
        float dx = wx0 - p.x;
        float dy = wy0 - p.y;
        float d = sqrtf(dx * dx + dy * dy);
        if (d < hitRadius) return i;
    }

    return -1;
}

bool App::ScreenToWorldOnZPlane(int screenX, int screenY, float& worldX, float& worldY) {
    DirectX::XMFLOAT3 ro, rd;
    m_camera.BuildRayFromScreen(float(screenX), float(screenY), ro, rd);

    // Intersect with plane Z=0: ro.z + t*rd.z = 0
    if (fabsf(rd.z) < 1e-6f) return false;
    float t = -ro.z / rd.z;
    if (t < 0.0f) return false;

    worldX = ro.x + rd.x * t;
    worldY = ro.y + rd.y * t;
    return true;
}

void App::UpdateViewProj() {
    m_camera.UpdateMatrices();
    m_engine->SetViewProj(m_camera.ViewProj());
}

void App::FocusCamera() {
    DirectX::XMFLOAT3 pts[3] = {
        m_editMesh->GetVertex(0),
        m_editMesh->GetVertex(1),
        m_editMesh->GetVertex(2),
    };
    m_camera.FocusOnPoints(pts, 3);
}
