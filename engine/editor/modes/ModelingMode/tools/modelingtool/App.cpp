#include "App.h"
#include "EditableMesh.h"
#include "RenderMesh.h"
#include <DirectXMath.h>

using namespace DirectX;

extern void UpdateVertexBuffer();

void App::BeginFrameInput() {
    m_input.lmbPressed = false;
    m_input.lmbReleased = false;
}

void App::Update(float /*dt*/) {
    if (!m_editMesh || !m_renderMesh) return;

    // 1) On press: select a vertex
    if (m_input.lmbPressed) {
        m_selectedVertex = HitTestVertex(m_input.mouseX, m_input.mouseY);
        m_isDragging = (m_selectedVertex != -1);
        m_lastMousePos = { m_input.mouseX, m_input.mouseY };
    }

    // 2) On release: stop dragging
    if (m_input.lmbReleased) {
        m_isDragging = false;
        m_selectedVertex = -1;
    }

    // 3) While dragging: edit the mesh (truth), mark dirty
    if (m_input.lmbDown && m_isDragging && m_selectedVertex != -1) {
        float ndcX = 0.0f, ndcY = 0.0f;
        ScreenToNDC(m_input.mouseX, m_input.mouseY, ndcX, ndcY);

        XMFLOAT3 p = m_editMesh->GetVertex((VertexID)m_selectedVertex);
        p.x = ndcX;
        p.y = ndcY;
        m_editMesh->SetVertex((VertexID)m_selectedVertex, p);

        m_renderMesh->dirty = true;
    }

    // 4) Upload once per frame
    if (m_renderMesh->dirty) {
        UpdateVertexBuffer();
        m_renderMesh->dirty = false;
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
    }

    return 0;
}

int App::HitTestVertex(int mouseX, int mouseY) {
    float ndcX = 0.0f, ndcY = 0.0f;
    ScreenToNDC(mouseX, mouseY, ndcX, ndcY);

    const float hitRadius = 0.05f;

    for (int i = 0; i < 3; ++i) {
        DirectX::XMFLOAT3 p = m_editMesh->GetVertex((VertexID)i);
        float dx = ndcX - p.x;
        float dy = ndcY - p.y;
        float distance = sqrtf(dx * dx + dy * dy);
        if (distance < hitRadius) return i;
    }

    return -1;
}

void App::ScreenToNDC(int screenX, int screenY, float& ndcX, float& ndcY) {
    RECT rc;
    GetClientRect(m_hwnd, &rc);

    float width = float(rc.right - rc.left);
    float height = float(rc.bottom - rc.top);

    ndcX = (2.0f * screenX / width) - 1.0f;
    ndcY = 1.0f - (2.0f * screenY / height);
}
