#include "App.h"
#include "EditableMesh.h"
#include "RenderMesh.h"
#include <DirectXMath.h>
#include <algorithm>
#include <cmath>

using namespace DirectX;

App::App(EditorCamera& camera) : m_camera(camera) {
    m_ctx.camera = &m_camera;

    // View pivot starts in front of the camera so orbit has a stable center.
    {
        DirectX::XMFLOAT3 pos = m_camera.Position();
        DirectX::XMFLOAT3 f = m_camera.Forward();
        //m_viewPivot = DirectX::XMFLOAT3(pos.x + f.x * m_orbitDistance, pos.y + f.y * m_orbitDistance, pos.z + f.z * m_orbitDistance);
    }

    m_objectCount = 2;
    m_objectPos[0] = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
    m_objectPos[1] = DirectX::XMFLOAT3(2.0f, 0.0f, 0.0f);
}

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
    if (!m_engine || !m_editMesh || !m_renderMesh) return;

    EditorCamera* cam = m_ctx.camera;
    if (!cam) return;

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

        // In RMB-fly look mode, treat the view center as "straight ahead at orbitDistance".
        // This keeps orbit behavior predictable after you look around in fly mode.
        {
            DirectX::XMFLOAT3 pos = m_camera.Position();
            DirectX::XMFLOAT3 fwd = m_camera.Forward();
            m_orbitDistance = (std::max)(0.25f, m_orbitDistance);
            m_viewPivot = DirectX::XMFLOAT3(pos.x + fwd.x * m_orbitDistance, pos.y + fwd.y * m_orbitDistance, pos.z + fwd.z * m_orbitDistance);
        }

    }
    if (m_input.rmbDown && !m_isDragging) {
        int dx = m_input.mouseX - m_lastCameraMouse.x;
        int dy = m_input.mouseY - m_lastCameraMouse.y;

        m_camera.AddYawPitch(dx * m_camera.mouseSensitivity, dy * m_camera.mouseSensitivity);

        m_lastCameraMouse = { m_input.mouseX, m_input.mouseY };
    }

    // Orbit/Pan (MMB):
    //   MMB drag           = orbit around view center
    //   Shift + MMB drag   = pan (moves camera and pivot together)
    // This is app/input + camera math (no D3D12 layer).
    if (m_input.mmbPressed) {
        m_lastCameraMouse = { m_input.mouseX, m_input.mouseY };

        // Blender-like: orbit center does NOT depend on cursor position.
        // We lock onto the current persistent view center (m_viewPivot) and just recompute distance.
        DirectX::XMFLOAT3 pos = m_camera.Position();
        float dxp = pos.x - m_viewPivot.x;
        float dyp = pos.y - m_viewPivot.y;
        float dzp = pos.z - m_viewPivot.z;
        m_orbitDistance = (std::max)(0.25f, std::sqrt(dxp * dxp + dyp * dyp + dzp * dzp));
    }
    if (m_input.mmbDown && !m_isDragging) {
        int dx = m_input.mouseX - m_lastCameraMouse.x;
        int dy = m_input.mouseY - m_lastCameraMouse.y;

        bool shiftDown = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        if (shiftDown) {
            DirectX::XMFLOAT3 right = m_camera.Right();
            DirectX::XMFLOAT3 up = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);

            float scale = m_camera.panSpeed * m_orbitDistance;
            DirectX::XMFLOAT3 delta = DirectX::XMFLOAT3(
                (-float(dx) * scale) * right.x + (float(dy) * scale) * up.x,
                (-float(dx) * scale) * right.y + (float(dy) * scale) * up.y,
                (-float(dx) * scale) * right.z + (float(dy) * scale) * up.z
            );

            DirectX::XMFLOAT3 pos = m_camera.Position();
            pos.x += delta.x; pos.y += delta.y; pos.z += delta.z;
            m_camera.SetPosition(pos);

            m_viewPivot.x += delta.x; m_viewPivot.y += delta.y; m_viewPivot.z += delta.z;
        } else {
            m_camera.AddYawPitch(dx * m_camera.mouseSensitivity, dy * m_camera.mouseSensitivity);

            DirectX::XMFLOAT3 f = m_camera.Forward();
            DirectX::XMFLOAT3 pos = DirectX::XMFLOAT3(
                m_viewPivot.x - f.x * m_orbitDistance,
                m_viewPivot.y - f.y * m_orbitDistance,
                m_viewPivot.z - f.z * m_orbitDistance
            );
            m_camera.SetPosition(pos);
        }

        m_lastCameraMouse = { m_input.mouseX, m_input.mouseY };
    }

    // Dolly (wheel).
    // - While RMB fly is held: dolly along camera forward (current behavior).
    // - Otherwise: dolly toward/away from the view pivot (Blender-like zoom).
    if (m_input.wheelDelta != 0 && !m_isDragging) {
        float steps = float(m_input.wheelDelta) / 120.0f;
        if (m_input.rmbDown) {
            m_camera.Dolly(steps);
        } else {
            m_orbitDistance = (std::max)(0.25f, m_orbitDistance - steps * m_camera.dollySpeed);

            DirectX::XMFLOAT3 f = m_camera.Forward();
            DirectX::XMFLOAT3 pos = DirectX::XMFLOAT3(
                m_viewPivot.x - f.x * m_orbitDistance,
                m_viewPivot.y - f.y * m_orbitDistance,
                m_viewPivot.z - f.z * m_orbitDistance
            );
            m_camera.SetPosition(pos);
        }
    }

    // WASD move only while RMB is held (Unreal-ish).
    if (m_input.rmbDown && !m_isDragging) {
        float f = 0.0f;
        float r = 0.0f;

        if (GetAsyncKeyState('W') & 0x8000 || GetAsyncKeyState(VK_UP)    & 0x8000) f += 1.0f;
        if (GetAsyncKeyState('S') & 0x8000 || GetAsyncKeyState(VK_DOWN)  & 0x8000) f -= 1.0f;
        if (GetAsyncKeyState('D') & 0x8000 || GetAsyncKeyState(VK_RIGHT) & 0x8000) r += 1.0f;
        if (GetAsyncKeyState('A') & 0x8000 || GetAsyncKeyState(VK_LEFT)  & 0x8000) r -= 1.0f;

        if (f != 0.0f || r != 0.0f) {
            float step = m_camera.moveSpeed * dt;
            m_camera.MoveLocal(f * step, r * step, 0.0f);
        }
    
        // Update view center pivot in fly move mode (see RMB look block comment).
        {
            DirectX::XMFLOAT3 pos = m_camera.Position();
            DirectX::XMFLOAT3 fwd = m_camera.Forward();
            m_orbitDistance = (std::max)(0.25f, m_orbitDistance);
            m_viewPivot = DirectX::XMFLOAT3(pos.x + fwd.x * m_orbitDistance, pos.y + fwd.y * m_orbitDistance, pos.z + fwd.z * m_orbitDistance);
        }
    }

    // Focus.
    if (m_input.fPressed && !m_isDragging) {
        FocusCamera();
    }



    // Object move (when not in RMB-fly mode).
    // Layer note: this is "app/input"... no D3D12 layer yet; we just update transforms.
    if (!m_input.rmbDown && !m_isDragging) {
        float dx = 0.0f;
        float dy = 0.0f;

        if (GetAsyncKeyState(VK_LEFT)  & 0x8000) dx -= 1.0f;
        if (GetAsyncKeyState(VK_RIGHT) & 0x8000) dx += 1.0f;
        if (GetAsyncKeyState(VK_UP)    & 0x8000) dy += 1.0f;
        if (GetAsyncKeyState(VK_DOWN)  & 0x8000) dy -= 1.0f;

        if ((dx != 0.0f || dy != 0.0f) && (m_activeObject < m_objectCount)) {
            float step = 2.0f * dt;
            m_objectPos[m_activeObject].x += dx * step;
            m_objectPos[m_activeObject].y += dy * step;
        }
    }

#if 0
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
#endif

    UpdateViewProj();

    // Debug: visualize orbit pivot as a tiny cyan tetra at m_viewPivot.
    // This helps validate what point MMB orbit is rotating around.
    uint32_t renderCount = m_objectCount;
    uint32_t pivotIndex = 0xFFFFFFFFu;
    if (renderCount < kMaxObjects) {
        pivotIndex = renderCount;
        renderCount++;
    }

    m_engine->SetObjectCount(renderCount);

    for (uint32_t i = 0; i < m_objectCount; ++i) {
        XMMATRIX W = XMMatrixTranslation(
            m_objectPos[i].x,
            m_objectPos[i].y,
            m_objectPos[i].z
        );

        XMFLOAT4X4 world;
        XMStoreFloat4x4(&world, W);
        m_engine->SetObjectWorld(i, world);
    }

    if (pivotIndex != 0xFFFFFFFFu) {
        XMMATRIX W = XMMatrixScaling(0.15f, 0.15f, 0.15f) * XMMatrixTranslation(m_viewPivot.x, m_viewPivot.y, m_viewPivot.z);
        XMFLOAT4X4 world;
        XMStoreFloat4x4(&world, W);
        m_engine->SetObjectWorld(pivotIndex, world);
        m_engine->SetDebugPivotIndex(pivotIndex);
    } else {
        m_engine->SetDebugPivotIndex(0xFFFFFFFFu);
    }

    m_engine->SetSelectedObject(m_activeObject);

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
        case WM_MOUSEWHEEL: {
            m_input.wheelDelta += GET_WHEEL_DELTA_WPARAM(wParam);
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
        case WM_KEYDOWN: {
            if (wParam == VK_ESCAPE) { handled = false; break; } // let main.cpp handle quit
            if (wParam < 256) {
                bool wasDown = (lParam & (1 << 30)) != 0;
                m_input.keys[wParam] = true;
                if (!wasDown && (wParam == 'F')) { m_input.fPressed = true; }

                // Add new object at origin
                if (!wasDown && wParam == 'N') {
                    AddObject(DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f));
                }
                // Duplicate active object (Ctrl+D)
                if (!wasDown && wParam == 'D' && (GetAsyncKeyState(VK_CONTROL) & 0x8000)) {
                    DuplicateActiveObject();
                }
                // Delete active object
                if (!wasDown && wParam == VK_DELETE) {
                    DeleteActiveObject();
                }
                // Select object 1–9
                if (!wasDown && wParam >= '1' && wParam <= '9') {
                    uint32_t idx = uint32_t(wParam - '1');
                    if (idx < m_objectCount)
                        m_activeObject = idx;
                }
            }
            handled = true;
            return 0;
        }
        case WM_KEYUP: {
            if (wParam == VK_ESCAPE) { handled = false; break; }
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
    // Compute the same center/radius that EditorCamera::FocusOnPoints uses so we can
    // update our view-center pivot and orbit distance.
    DirectX::XMFLOAT3 c = { 0, 0, 0 };
    for (int i = 0; i < 3; ++i) {
        c.x += pts[i].x;
        c.y += pts[i].y;
        c.z += pts[i].z;
    }
    c.x /= 3.0f; c.y /= 3.0f; c.z /= 3.0f;

    float r = 0.0f;
    for (int i = 0; i < 3; ++i) {
        float dx = pts[i].x - c.x;
        float dy = pts[i].y - c.y;
        float dz = pts[i].z - c.z;
        r = (std::max)(r, std::sqrt(dx * dx + dy * dy + dz * dz));
    }
    r = (std::max)(r, 0.5f);

    float dist = r / std::tan(DirectX::XM_PIDIV4 * 0.5f);
    dist = (std::max)(dist, 1.0f);

    m_camera.FocusOnPoints(pts, 3);
    m_viewPivot = c;
    m_orbitDistance = dist;
}

bool App::AddObject(const DirectX::XMFLOAT3& pos) {
    if (m_objectCount >= kMaxObjects)
        return false;

    m_objectPos[m_objectCount] = pos;
    m_objectCount++;
    m_activeObject = m_objectCount - 1;
    return true;
}

bool App::DuplicateActiveObject() {
    if (m_activeObject >= m_objectCount)
        return false;

    DirectX::XMFLOAT3 p = m_objectPos[m_activeObject];
    p.x += 0.5f; // small offset so it’s visible
    return AddObject(p);
}

bool App::DeleteActiveObject() {
    if (m_objectCount <= 1)
        return false;

    if (m_activeObject >= m_objectCount)
        return false;

    for (uint32_t i = m_activeObject + 1; i < m_objectCount; ++i)
        m_objectPos[i - 1] = m_objectPos[i];

    m_objectCount--;

    if (m_activeObject >= m_objectCount)
        m_activeObject = m_objectCount - 1;

    return true;
}
