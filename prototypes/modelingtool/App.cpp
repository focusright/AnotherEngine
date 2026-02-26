#define _CRT_SECURE_NO_WARNINGS

#include "App.h"
#include "EditableMesh.h"
#include "RenderMesh.h"
#include <DirectXMath.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

using namespace DirectX;

App::App(EditorCamera& camera) : m_camera(camera) {
    m_ctx.camera = &m_camera;

    // Startup camera framing: look at the origin (Blender-like default scene framing).
    m_viewPivot = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
    m_camera.LookAt(m_viewPivot);
    {
        char buf[256];
        DirectX::XMFLOAT3 fwd = m_camera.Forward();
        std::snprintf(buf, sizeof(buf), "Startup LookAt: yaw=%.3f pitch=%.3f fwd=(%.3f,%.3f,%.3f) pos=(%.3f,%.3f,%.3f) pivot=(%.3f,%.3f,%.3f)\n",
            m_camera.Yaw(), m_camera.Pitch(), fwd.x, fwd.y, fwd.z,
            m_camera.Position().x, m_camera.Position().y, m_camera.Position().z,
            m_viewPivot.x, m_viewPivot.y, m_viewPivot.z);
        OutputDebugStringA(buf);
    }
    {
        DirectX::XMFLOAT3 pos = m_camera.Position();
        float dx = pos.x - m_viewPivot.x;
        float dy = pos.y - m_viewPivot.y;
        float dz = pos.z - m_viewPivot.z;
        m_orbitDistance = (std::max)(0.25f, std::sqrt(dx * dx + dy * dy + dz * dz));
    }


    // Initialize per-object transform components.
for (uint32_t i = 0; i < kMaxObjects; ++i) {
    m_objectRot[i] = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
    m_objectScale[i] = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
    m_objectColor[i] = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
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
            m_viewPivot = DirectX::XMFLOAT3(
                pos.x + fwd.x * m_orbitDistance,
                pos.y + fwd.y * m_orbitDistance,
                pos.z + fwd.z * m_orbitDistance
            );
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
    if (m_input.mmbDown && !m_isDragging) { //Continue orbit/pan only while holding MMB, do not orbit while dragging a gizmo
        int dx = m_input.mouseX - m_lastCameraMouse.x;
        int dy = m_input.mouseY - m_lastCameraMouse.y;

        bool shiftDown = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        if (shiftDown) { //Pan
            DirectX::XMFLOAT3 right = m_camera.Right();
            DirectX::XMFLOAT3 up = m_camera.Up();

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
        } else { //Orbit
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
    if (m_input.fPressed && !m_isDragging) { FocusCamera(); }

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
        XMMATRIX S = XMMatrixScaling(m_objectScale[i].x, m_objectScale[i].y, m_objectScale[i].z);
        XMMATRIX R = XMMatrixRotationRollPitchYaw(m_objectRot[i].x, m_objectRot[i].y, m_objectRot[i].z);
        XMMATRIX T = XMMatrixTranslation(m_objectPos[i].x, m_objectPos[i].y, m_objectPos[i].z);
        XMMATRIX W = S * R * T; //W = World matrix for this object
        XMFLOAT4X4 world;
        XMStoreFloat4x4(&world, W);
        m_engine->SetObjectWorld(i, world);
        m_engine->SetObjectTint(i, m_objectColor[i]);
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

    UpdateGizmo();

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

                // Scene save/load (v0.0.2)
                if (!wasDown && wParam == 'S' && (GetAsyncKeyState(VK_CONTROL) & 0x8000)) {
                    SaveSceneAem(L"scene.aem");
                }
                if (!wasDown && wParam == 'O' && (GetAsyncKeyState(VK_CONTROL) & 0x8000)) {
                    LoadSceneAem(L"scene.aem");
                }


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


bool App::SaveSceneAem(const wchar_t* path) {
    // File format:
    //   AE_MODEL 1
    //   objects N
    //   active I
    //   obj tetra px py pz rx ry rz sx sy sz cr cg cb ca
    FILE* f = nullptr;
    if (_wfopen_s(&f, path, L"wb") != 0 || !f) return false;

    std::fprintf(f, "AE_MODEL 1\n");
    std::fprintf(f, "objects %u\n", (unsigned)m_objectCount);
    std::fprintf(f, "active %u\n", (unsigned)m_activeObject);

    for (uint32_t i = 0; i < m_objectCount; ++i) {
        std::fprintf(f, "obj tetra %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g\n",
            m_objectPos[i].x, m_objectPos[i].y, m_objectPos[i].z,
            m_objectRot[i].x, m_objectRot[i].y, m_objectRot[i].z,
            m_objectScale[i].x, m_objectScale[i].y, m_objectScale[i].z,
            m_objectColor[i].x, m_objectColor[i].y, m_objectColor[i].z, m_objectColor[i].w
        );
    }

    std::fclose(f);
    return true;
}

bool App::LoadSceneAem(const wchar_t* path) {
    FILE* f = nullptr;
    if (_wfopen_s(&f, path, L"rb") != 0 || !f) return false;

    char header[64] = {};
    int ver = 0;
    if (std::fscanf(f, "%63s %d", header, &ver) != 2) { std::fclose(f); return false; }
    if (std::strcmp(header, "AE_MODEL") != 0 || ver != 1) { std::fclose(f); return false; }

    char key[64] = {};
    unsigned n = 0;
    if (std::fscanf(f, "%63s %u", key, &n) != 2) { std::fclose(f); return false; }
    if (std::strcmp(key, "objects") != 0) { std::fclose(f); return false; }

    unsigned active = 0;
    if (std::fscanf(f, "%63s %u", key, &active) != 2) { std::fclose(f); return false; }
    if (std::strcmp(key, "active") != 0) { std::fclose(f); return false; }

    if (n == 0) n = 1;
    if (n > kMaxObjects) n = kMaxObjects;

    m_objectCount = (uint32_t)n;
    m_activeObject = (active < m_objectCount) ? (uint32_t)active : 0;

    for (uint32_t i = 0; i < m_objectCount; ++i) {
        char objKey[16] = {};
        char prim[16] = {};
        if (std::fscanf(f, "%15s %15s", objKey, prim) != 2) { std::fclose(f); return false; }
        if (std::strcmp(objKey, "obj") != 0) { std::fclose(f); return false; }
        if (std::strcmp(prim, "tetra") != 0) { std::fclose(f); return false; }

        if (std::fscanf(f, "%f %f %f %f %f %f %f %f %f %f %f %f %f",
            &m_objectPos[i].x, &m_objectPos[i].y, &m_objectPos[i].z,
            &m_objectRot[i].x, &m_objectRot[i].y, &m_objectRot[i].z,
            &m_objectScale[i].x, &m_objectScale[i].y, &m_objectScale[i].z,
            &m_objectColor[i].x, &m_objectColor[i].y, &m_objectColor[i].z, &m_objectColor[i].w
        ) != 13) { std::fclose(f); return false; }
    }

    std::fclose(f);
    return true;
}

bool App::GizmoPickAxis(int mouseX, int mouseY, int& outAxis, float& outTOnAxis) {
    // Pick the closest axis by ray-to-segment distance.
    // We use a conservative threshold in world units.
    DirectX::XMFLOAT3 ro, rd;
    m_camera.BuildRayFromScreen(float(mouseX), float(mouseY), ro, rd);

    DirectX::XMFLOAT3 o = m_objectPos[m_activeObject];
    const float axisLen = kGizmoAxisLen;

    auto testAxis = [&](int axis, const DirectX::XMFLOAT3& dir, float& outTRaw, float& outDistSq) {
        // Segment [o, o + dir*axisLen]
        DirectX::XMVECTOR RO = DirectX::XMLoadFloat3(&ro);
        DirectX::XMVECTOR RD = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&rd));
        DirectX::XMVECTOR O = DirectX::XMLoadFloat3(&o);
        DirectX::XMVECTOR D = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&dir));
        DirectX::XMVECTOR P1 = O;
        DirectX::XMVECTOR P2 = DirectX::XMVectorAdd(O, DirectX::XMVectorScale(D, axisLen));

        // Compute closest points between ray (RO + u*RD, u>=0) and infinite line (P1 + t*D).
        DirectX::XMVECTOR w0 = DirectX::XMVectorSubtract(RO, P1);
        float a = DirectX::XMVectorGetX(DirectX::XMVector3Dot(RD, RD));
        float b = DirectX::XMVectorGetX(DirectX::XMVector3Dot(RD, D));
        float c = DirectX::XMVectorGetX(DirectX::XMVector3Dot(D, D));
        float d = DirectX::XMVectorGetX(DirectX::XMVector3Dot(RD, w0));
        float e = DirectX::XMVectorGetX(DirectX::XMVector3Dot(D, w0));
        float denom = a * c - b * b;

        float u = 0.0f;
        float t = 0.0f;
        if (fabsf(denom) > 1e-6f) {
            u = (b * e - c * d) / denom;
            t = (a * e - b * d) / denom;
        }

        if (u < 0.0f) u = 0.0f;

        // Keep the raw t (infinite line) so drag doesn't "jump" when leaving the handle segment.
        outTRaw = t;

        // Clamp t to segment.
        if (t < 0.0f) t = 0.0f;
        if (t > axisLen) t = axisLen;

        DirectX::XMVECTOR C0 = DirectX::XMVectorAdd(RO, DirectX::XMVectorScale(RD, u));
        DirectX::XMVECTOR C1 = DirectX::XMVectorAdd(P1, DirectX::XMVectorScale(D, t));
        DirectX::XMVECTOR diff = DirectX::XMVectorSubtract(C0, C1);
        outDistSq = DirectX::XMVectorGetX(DirectX::XMVector3Dot(diff, diff));
    };

    float bestDistSq = 1e30f;
    int bestAxis = -1;

    float tRaw, dsq;
    float bestTRaw = 0.0f;

    testAxis(0, DirectX::XMFLOAT3(1, 0, 0), tRaw, dsq);
    if (dsq < bestDistSq) { bestDistSq = dsq; bestAxis = 0; bestTRaw = tRaw; }

    testAxis(1, DirectX::XMFLOAT3(0, 1, 0), tRaw, dsq);
    if (dsq < bestDistSq) { bestDistSq = dsq; bestAxis = 1; bestTRaw = tRaw; }

    testAxis(2, DirectX::XMFLOAT3(0, 0, 1), tRaw, dsq);
    if (dsq < bestDistSq) { bestDistSq = dsq; bestAxis = 2; bestTRaw = tRaw; }

    // Threshold: tune for your world scale (tetra size ~0.8)
    const float thresh = kGizmoPickThresh;
    if (bestDistSq > thresh * thresh) return false;

    outAxis = bestAxis;
    outTOnAxis = bestTRaw;
    return true;
}

bool App::GizmoComputeTOnAxis(int axis, int mouseX, int mouseY, float& outTOnAxis) {
    DirectX::XMFLOAT3 ro, rd;
    m_camera.BuildRayFromScreen(float(mouseX), float(mouseY), ro, rd);

    DirectX::XMFLOAT3 o = m_objectPos[m_activeObject];
    DirectX::XMFLOAT3 dir = (axis == 0) ? DirectX::XMFLOAT3(1, 0, 0) : (axis == 1) ? DirectX::XMFLOAT3(0, 1, 0) : DirectX::XMFLOAT3(0, 0, 1);

    // Stable drag: intersect the mouse ray with a plane that contains the axis line and is as
    // perpendicular to the camera view direction as possible.
    DirectX::XMVECTOR RO = DirectX::XMLoadFloat3(&ro);
    DirectX::XMVECTOR RD = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&rd));
    DirectX::XMVECTOR O = DirectX::XMLoadFloat3(&o);
    DirectX::XMVECTOR A = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&dir));

    DirectX::XMFLOAT3 camF = m_camera.Forward();
    DirectX::XMVECTOR V = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&camF));

    // n = axis x (view x axis)  == view projected perpendicular to axis (up to scale).
    DirectX::XMVECTOR n = DirectX::XMVector3Cross(A, DirectX::XMVector3Cross(V, A));
    float nLenSq = DirectX::XMVectorGetX(DirectX::XMVector3Dot(n, n));

    if (nLenSq < 1e-8f) {
        // View almost parallel to axis. Pick a fallback vector not parallel to the axis.
        DirectX::XMVECTOR fallback = (fabsf(DirectX::XMVectorGetY(A)) < 0.9f) ? DirectX::XMVectorSet(0, 1, 0, 0) : DirectX::XMVectorSet(1, 0, 0, 0);
        n = DirectX::XMVector3Cross(A, DirectX::XMVector3Cross(fallback, A));
        nLenSq = DirectX::XMVectorGetX(DirectX::XMVector3Dot(n, n));
        if (nLenSq < 1e-8f) return false;
    }

    n = DirectX::XMVector3Normalize(n);

    float denom = DirectX::XMVectorGetX(DirectX::XMVector3Dot(RD, n));
    if (fabsf(denom) < 1e-6f) return false;

    float u = DirectX::XMVectorGetX(DirectX::XMVector3Dot(DirectX::XMVectorSubtract(O, RO), n)) / denom;
    if (u < 0.0f) u = 0.0f;

    DirectX::XMVECTOR P = DirectX::XMVectorAdd(RO, DirectX::XMVectorScale(RD, u));
    float t = DirectX::XMVectorGetX(DirectX::XMVector3Dot(DirectX::XMVectorSubtract(P, O), A));
    outTOnAxis = t;
    return true;
}

void App::UpdateGizmo() {
    if (!m_engine) return;
    if (m_activeObject >= m_objectCount) return;

    // Hover highlight (when not dragging and not in RMB-fly).
    if (!m_gizmoDragging && !m_input.rmbDown) {
        m_gizmoHotAxis = -1;
        int axis = -1;
        float t = 0.0f;
        if (GizmoPickAxis(m_input.mouseX, m_input.mouseY, axis, t)) {
            m_gizmoHotAxis = axis;
        }
    } else {
        m_gizmoHotAxis = -1;
    }

    // 1) Render gizmo (always at active object's position).
    {
        Vertex gizmo[kGizmoVertexCount] = {};

        DirectX::XMFLOAT3 o = m_objectPos[m_activeObject];
        const float len = kGizmoAxisLen;

        auto hotColor = DirectX::XMFLOAT4(1, 1, 1, 1);
        auto activeColor = DirectX::XMFLOAT4(1, 1, 0, 1);

        DirectX::XMFLOAT4 colX = DirectX::XMFLOAT4(1, 0, 0, 1);
        DirectX::XMFLOAT4 colY = DirectX::XMFLOAT4(0, 1, 0, 1);
        DirectX::XMFLOAT4 colZ = DirectX::XMFLOAT4(0.2f, 0.5f, 1, 1);

        if (m_gizmoActiveAxis == 0) colX = activeColor; else if (m_gizmoHotAxis == 0) colX = hotColor;
        if (m_gizmoActiveAxis == 1) colY = activeColor; else if (m_gizmoHotAxis == 1) colY = hotColor;
        if (m_gizmoActiveAxis == 2) colZ = activeColor; else if (m_gizmoHotAxis == 2) colZ = hotColor;

        gizmo[0] = { o, colX };
        gizmo[1] = { DirectX::XMFLOAT3(o.x + len, o.y, o.z), colX };

        gizmo[2] = { o, colY };
        gizmo[3] = { DirectX::XMFLOAT3(o.x, o.y + len, o.z), colY };

        gizmo[4] = { o, colZ };
        gizmo[5] = { DirectX::XMFLOAT3(o.x, o.y, o.z + len), colZ };

        m_engine->UpdateGizmoVertices(gizmo, kGizmoVertexCount, m_hwnd);
    }

    // 2) Input: start drag on LMB press (only when not in RMB-fly mode).
    if (!m_input.rmbDown && m_input.lmbPressed && !m_gizmoDragging) {
        int axis = -1;
        float t0 = 0.0f;
        if (GizmoPickAxis(m_input.mouseX, m_input.mouseY, axis, t0)) {
            m_gizmoActiveAxis = axis;
            m_gizmoDragging = true;
            // Use the same t-computation as the drag loop to avoid a 1-frame jump.
            if (!GizmoComputeTOnAxis(m_gizmoActiveAxis, m_input.mouseX, m_input.mouseY, m_gizmoDragT0))
                m_gizmoDragT0 = t0;
            m_gizmoStartPos = m_objectPos[m_activeObject];
        }
    }

    // 3) While dragging: update object position along axis.
    if (m_gizmoDragging && m_input.lmbDown && m_gizmoActiveAxis != -1) {
        float t = 0.0f;
        if (GizmoComputeTOnAxis(m_gizmoActiveAxis, m_input.mouseX, m_input.mouseY, t)) {
            float dt = t - m_gizmoDragT0;
            DirectX::XMFLOAT3 axisDir = (m_gizmoActiveAxis == 0) ? DirectX::XMFLOAT3(1, 0, 0) : (m_gizmoActiveAxis == 1) ? DirectX::XMFLOAT3(0, 1, 0) : DirectX::XMFLOAT3(0, 0, 1);
            m_objectPos[m_activeObject] = DirectX::XMFLOAT3(
                m_gizmoStartPos.x + axisDir.x * dt,
                m_gizmoStartPos.y + axisDir.y * dt,
                m_gizmoStartPos.z + axisDir.z * dt
            );
        }
    }

    // 4) End drag.
    if (m_gizmoDragging && m_input.lmbReleased) {
        m_gizmoDragging = false;
        m_gizmoActiveAxis = -1;
    }

    // RMB-fly should cancel gizmo drag immediately.
    if (m_gizmoDragging && m_input.rmbPressed) {
        m_gizmoDragging = false;
        m_gizmoActiveAxis = -1;
    }
}

bool App::AddObject(const DirectX::XMFLOAT3& pos) {
    if (m_objectCount >= kMaxObjects)
        return false;

    m_objectPos[m_objectCount] = pos;
    m_objectRot[m_objectCount] = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
    m_objectScale[m_objectCount] = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
    m_objectColor[m_objectCount] = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    m_objectCount++;
    m_activeObject = m_objectCount - 1;
    return true;
}

bool App::DuplicateActiveObject() {
    if (m_activeObject >= m_objectCount)
        return false;

    DirectX::XMFLOAT3 p = m_objectPos[m_activeObject];
    DirectX::XMFLOAT3 r = m_objectRot[m_activeObject];
    DirectX::XMFLOAT3 s = m_objectScale[m_activeObject];
    DirectX::XMFLOAT4 c = m_objectColor[m_activeObject];
    p.x += 0.5f; // small offset so it’s visible
    if (!AddObject(p)) return false;
    m_objectRot[m_activeObject] = r;
    m_objectScale[m_activeObject] = s;
    m_objectColor[m_activeObject] = c;
    return true;
}

bool App::DeleteActiveObject() {
    if (m_objectCount <= 1)
        return false;

    if (m_activeObject >= m_objectCount)
        return false;

    for (uint32_t i = m_activeObject + 1; i < m_objectCount; ++i) {
        m_objectPos[i - 1] = m_objectPos[i];
        m_objectRot[i - 1] = m_objectRot[i];
        m_objectScale[i - 1] = m_objectScale[i];
        m_objectColor[i - 1] = m_objectColor[i];
    }

    m_objectCount--;

    if (m_activeObject >= m_objectCount)
        m_activeObject = m_objectCount - 1;

    return true;
}
