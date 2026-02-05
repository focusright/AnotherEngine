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
    m_input.wheelDelta = 0;
}

void App::Update(float /*dt*/) {
    if (!m_editMesh || !m_renderMesh || !m_engine) return;

    if (!m_colorsInit) {
        for (int i = 0; i < 3; ++i) { m_baseColors[i] = m_renderMesh->drawVertices[i].color; }
        m_colorsInit = true;
    }

    // Zoom (mouse wheel)
    if (m_input.wheelDelta != 0) {
        float steps = float(m_input.wheelDelta) / 120.0f;
        m_zoom *= powf(1.1f, steps);
        if (m_zoom < 0.05f) m_zoom = 0.05f;
        if (m_zoom > 50.0f) m_zoom = 50.0f;
    }

    // Pan (RMB drag) - only if we aren't currently dragging a vertex
    if (m_input.rmbPressed) {
        m_lastMousePos = { m_input.mouseX, m_input.mouseY };
    }
    if (m_input.rmbDown && !m_isDragging) {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        float width = float(rc.right - rc.left);
        float height = float(rc.bottom - rc.top);
        float aspect = (height > 0.0f) ? (width / height) : 1.0f;

        int dx = m_input.mouseX - m_lastMousePos.x;
        int dy = m_input.mouseY - m_lastMousePos.y;

        float ndcDx = (2.0f * dx) / width;
        float ndcDy = (2.0f * dy) / height;

        m_panX -= ndcDx * (aspect / m_zoom);
        m_panY += ndcDy * (1.0f / m_zoom);

        m_lastMousePos = { m_input.mouseX, m_input.mouseY };
    }

    // 1) On press: select a vertex
    if (m_input.lmbPressed) {
        m_selectedVertex = HitTestVertex(m_input.mouseX, m_input.mouseY);
        m_isDragging = (m_selectedVertex != -1);
        m_lastMousePos = { m_input.mouseX, m_input.mouseY };

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

    // 3) While dragging: move the vertex in world space
    if (m_input.lmbDown && m_isDragging && m_selectedVertex != -1) {
        float worldX = 0.0f, worldY = 0.0f;
        ScreenToWorld(m_input.mouseX, m_input.mouseY, worldX, worldY);

        DirectX::XMFLOAT3 p = m_editMesh->GetVertex((VertexID)m_selectedVertex);
        p.x = worldX;
        p.y = worldY;
        m_editMesh->SetVertex((VertexID)m_selectedVertex, p);

        m_renderMesh->dirty = true;
    }

    // Always update the view/proj constants (cheap and keeps it simple)
    UpdateViewProj();

    // 4) Upload once per frame if dirty
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
    }

    return 0;
}

int App::HitTestVertex(int mouseX, int mouseY) {
    if (!m_hwnd) return -1;

    float worldX = 0.0f, worldY = 0.0f;
    ScreenToWorld(mouseX, mouseY, worldX, worldY);

    RECT rc;
    GetClientRect(m_hwnd, &rc);
    float width = float(rc.right - rc.left);
    float height = float(rc.bottom - rc.top);
    float aspect = (height > 0.0f) ? (width / height) : 1.0f;

    // ~10px radius in world units
    const float px = 10.0f;
    float ndcRadius = (2.0f * px) / width;
    float hitRadius = ndcRadius * (aspect / m_zoom);

    for (int i = 0; i < 3; ++i) {
        DirectX::XMFLOAT3 p = m_editMesh->GetVertex((VertexID)i);
        float dx = worldX - p.x;
        float dy = worldY - p.y;
        float d = sqrtf(dx * dx + dy * dy);
        if (d < hitRadius) return i;
    }

    return -1;
}

void App::ScreenToNDC(int screenX, int screenY, float& ndcX, float& ndcY) {
    if (!m_hwnd) { ndcX = 0.0f; ndcY = 0.0f; return; }

    RECT rc;
    GetClientRect(m_hwnd, &rc);

    float width = float(rc.right - rc.left);
    float height = float(rc.bottom - rc.top);

    ndcX = (2.0f * screenX / width) - 1.0f;
    ndcY = 1.0f - (2.0f * screenY / height);
}

void App::ScreenToWorld(int screenX, int screenY, float& worldX, float& worldY) {
    if (!m_hwnd) { worldX = 0.0f; worldY = 0.0f; return; }

    RECT rc;
    GetClientRect(m_hwnd, &rc);

    float width = float(rc.right - rc.left);
    float height = float(rc.bottom - rc.top);
    float aspect = (height > 0.0f) ? (width / height) : 1.0f;

    float ndcX = (2.0f * screenX / width) - 1.0f;
    float ndcY = 1.0f - (2.0f * screenY / height);

    worldX = (ndcX * (aspect / m_zoom)) + m_panX;
    worldY = (ndcY * (1.0f / m_zoom)) + m_panY;
}

void App::UpdateViewProj() {
    if (!m_engine || !m_hwnd) return;

    RECT rc;
    GetClientRect(m_hwnd, &rc);

    float width = float(rc.right - rc.left);
    float height = float(rc.bottom - rc.top);
    float aspect = (height > 0.0f) ? (width / height) : 1.0f;

    float sx = m_zoom / aspect;
    float sy = m_zoom;

    DirectX::XMFLOAT4X4 vp;
    vp._11 = sx;  vp._12 = 0;   vp._13 = 0; vp._14 = 0;
    vp._21 = 0;   vp._22 = sy;  vp._23 = 0; vp._24 = 0;
    vp._31 = 0;   vp._32 = 0;   vp._33 = 1; vp._34 = 0;
    vp._41 = -m_panX * sx; vp._42 = -m_panY * sy; vp._43 = 0; vp._44 = 1;

    m_engine->SetViewProj(vp);
}
