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

static const char* CommandName(EditorCommandType type) {
    switch (type) {
    case EditorCommandType::AddObject: return "AddObject";
    case EditorCommandType::DuplicateActiveObject: return "DuplicateActiveObject";
    case EditorCommandType::DeleteActiveObject: return "DeleteActiveObject";
    case EditorCommandType::SetActiveObject: return "SetActiveObject";
    case EditorCommandType::SaveScene: return "SaveScene";
    case EditorCommandType::LoadScene: return "LoadScene";
    case EditorCommandType::SetActiveTransform: return "SetActiveTransform";
    case EditorCommandType::SetGizmoMode: return "SetGizmoMode";
    case EditorCommandType::FocusCamera: return "FocusCamera";
    default: return "None";
    }
}

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
        for (int i = 0; i < 12; ++i) {
            m_baseColors[i] = m_renderMesh->drawVertices[i].color;
        }
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
    if (m_input.fPressed && !m_isDragging) {
        ExecuteCommand(EditorCommandType::FocusCamera);
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

    // 1) On press: gizmo first, then active-object vertex, then object selection.
    if (!m_input.rmbDown && m_input.lmbPressed && !m_gizmo.IsDragging()) {
        bool overGizmo = false;

        if (m_activeObject < m_objectCount) {
            int axis = -1;
            float t0 = 0.0f;

            GizmoTarget target = BuildGizmoTarget();

            overGizmo = m_gizmo.PickAxis(
                m_camera,
                target,
                m_input.mouseX,
                m_input.mouseY,
                axis,
                t0
            );
        }

        if (!overGizmo) {
            int hitVertex = HitTestVertex(m_input.mouseX, m_input.mouseY);

            if (hitVertex != -1) {
                m_selectedVertex = hitVertex;
                m_isDragging = false;

                for (int i = 0; i < 12; ++i) {
                    m_renderMesh->drawVertices[i].color = m_baseColors[i];
                }

                for (int i = 0; i < 12; ++i) {
                    if ((int)m_renderMesh->drawToEdit[i] == m_selectedVertex) {
                        m_renderMesh->drawVertices[i].color = DirectX::XMFLOAT4(1, 1, 1, 1);
                    }
                }

                m_renderMesh->dirty = true;
            } else {
                int hitObject = HitTestObject(m_input.mouseX, m_input.mouseY);

                if (hitObject != -1) {
                    EditorCommand command{ EditorCommandType::SetActiveObject };
                    command.objectIndex = (uint32_t)hitObject;
                    ExecuteCommand(command);
                } else {
                    m_selectedVertex = -1;

                    for (int i = 0; i < 12; ++i) {
                        m_renderMesh->drawVertices[i].color = m_baseColors[i];
                    }

                    m_renderMesh->dirty = true;
                }
            }
        }
    }

#if 0
    // Old direct vertex drag on Z=0 plane.
    // Keep disabled. Vertex movement should go through the gizmo now.
    if (m_input.lmbReleased) {
        m_isDragging = false;
        m_selectedVertex = -1;

        for (int i = 0; i < 3; ++i) { m_renderMesh->drawVertices[i].color = m_baseColors[i]; }
        m_renderMesh->dirty = true;
    }

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

    bool renderMeshDirty = false;
    
    GizmoUpdateArgs gizmoArgs = BuildGizmoUpdateArgs(renderMeshDirty);
    m_gizmo.Update(gizmoArgs);

    if (renderMeshDirty)
        m_renderMesh->dirty = true;

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

                if (!wasDown && wParam == 'F') {
                    m_input.fPressed = true;
                }

                if (!wasDown && wParam == 'S' && (GetAsyncKeyState(VK_CONTROL) & 0x8000)) {
                    EditorCommand command = {EditorCommandType::SaveScene};
                    command.path = L"scene.aem";
                    ExecuteCommand(command);
                }

                if (!wasDown && wParam == 'O' && (GetAsyncKeyState(VK_CONTROL) & 0x8000)) {
                    EditorCommand command = {EditorCommandType::LoadScene};
                    command.path = L"scene.aem";
                    ExecuteCommand(command);
                }

                if (!wasDown && wParam == 'N') {
                    EditorCommand command = {EditorCommandType::AddObject};
                    command.pos = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
                    ExecuteCommand(command);
                }

                if (!wasDown && wParam == 'D' && (GetAsyncKeyState(VK_CONTROL) & 0x8000)) {
                    ExecuteCommand(EditorCommandType::DuplicateActiveObject);
                }

                if (!wasDown && wParam == VK_DELETE) {
                    ExecuteCommand(EditorCommandType::DeleteActiveObject);
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

    float bestDistSq = 1.0e30f;
    int bestVertex = -1;
    const float pickRadiusPx = 14.0f;
    const float pickRadiusSq = pickRadiusPx * pickRadiusPx;

    for (int i = 0; i < 4; ++i) {
        DirectX::XMFLOAT3 p = m_editMesh->GetVertex((VertexID)i);
        DirectX::XMFLOAT3 pw = LocalVertexToWorld(p);

        float sx = 0.0f;
        float sy = 0.0f;
        if (!WorldToScreen(pw, sx, sy)) continue;

        float dx = sx - float(mouseX);
        float dy = sy - float(mouseY);
        float distSq = dx * dx + dy * dy;

        if (distSq <= pickRadiusSq && distSq < bestDistSq) {
            bestDistSq = distSq;
            bestVertex = i;
        }
    }

    return bestVertex;
}

int App::HitTestObject(int mouseX, int mouseY) const {
    DirectX::XMFLOAT3 origin, dir;
    m_camera.BuildRayFromScreen(float(mouseX), float(mouseY), origin, dir);

    DirectX::XMVECTOR rayOrigin = DirectX::XMLoadFloat3(&origin);
    DirectX::XMVECTOR rayDir = DirectX::XMLoadFloat3(&dir);

    float bestDist = 1.0e30f;
    int bestIndex = -1;

    for (uint32_t index = 0; index < m_objectCount; ++index) {
        DirectX::XMFLOAT3 center = m_objectPos[index];

        float scale = (std::max)(m_objectScale[index].x, (std::max)(m_objectScale[index].y, m_objectScale[index].z));
        float radius = 1.0f * scale;

        DirectX::XMVECTOR sphereCenter = DirectX::XMLoadFloat3(&center); //Also object center in world space
        DirectX::XMVECTOR offset = DirectX::XMVectorSubtract(rayOrigin, sphereCenter); //points from the sphere center to the ray origin 
        
        /*
        |X - C| = r(Sphere: C is the sphere center, r is the radius, X is any point on the sphere)
        |X - C|^2 = r^2      (Remove the square root on the LHS since it's a distance formula)
        P(t) = O + tD        (Ray: O = origin, D = direction, t = distance along the ray)
        |O + tD - C|^2 = r^2 (plug ray into the sphere equation) 
        m = O - C            (vector from sphere center to ray origin)
        |m + tD|^2 = r^2
        |m + tD|^2 = (m + tD) · (m + tD)
        (m + tD) · (m + tD) = m·m + 2t(m·D) + t^2(D·D)
        t^2(D·D) + 2t(m·D) + (m·m - r^2) = 0
        m·m : The squared distance from the ray origin to the sphere center
        m·D : how the ray direction lines up with the vector from center to origin
        D·D = 1 : The squared length of the direction vector (normalized)
        m·m - r^2 : whether the ray origin starts outside, on, or inside the sphere
            positive: outside
            zero: exactly on surface
            negative: inside
        */

        //proj < 0 means the ray points back toward the sphere
        //proj > 0 means the ray points away from the sphere
        float proj = DirectX::XMVectorGetX(DirectX::XMVector3Dot(offset, rayDir)); //is the sphere center roughly in front of the ray or behind it, and by how much?
        float offsetLenSq = DirectX::XMVectorGetX(DirectX::XMVector3Dot(offset, offset)); //squared distance from the ray origin to the sphere center
        //dist > 0 means the ray origin is outside the sphere
        //dist == 0 means the ray origin is exactly on the sphere
        //dist < 0 means the ray origin is inside the sphere
        float dist = offsetLenSq - radius * radius;

        if (dist > 0.0f && proj > 0.0f) continue; //the ray starts outside the sphere or the ray points away from the sphere, so a hit is impossible

        //ax² + bx + c = 0, x = -b ± sqrt(b² - 4ac) / 2a, discriminant = b² - 4ac

        float disc = proj * proj - dist; //reduced discriminant, obtained because the ray-sphere quadratic was written as:
        //t² + 2proj·t + dist = 0
        //a = 1
        //b = 2proj
        //c = dist
        //disc = 4(proj² - dist)

        //disc < 0 means no real intersection (No real roots)
        //disc == 0 means tangent hit (one real root)
        //disc > 0 means the ray enters and exits the sphere (two distinct real roots)
        if (disc < 0.0f) continue; //So if disc is negative, skip this object

        float hit = -proj - std::sqrt(disc); //t = -proj ± sqrt(disc), using only minus because that is the nearer root
        //Since sqrt(disc) is nonnegative, -proj - sqrt(disc) <= -proj + sqrt(disc)
        //-proj - sqrt(disc) is the smaller root
        //-proj + sqrt(disc) is the larger root
        
        if (hit < 0.0f) hit = 0.0f;

        if (hit < bestDist) {
            bestDist = hit;
            bestIndex = (int)index;
        }
    }

    return bestIndex;
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

DirectX::XMFLOAT3 App::LocalVertexToWorld(const DirectX::XMFLOAT3& p) const {
    if (m_activeObject >= m_objectCount) return p;

    DirectX::XMMATRIX S = DirectX::XMMatrixScaling(
        m_objectScale[m_activeObject].x,
        m_objectScale[m_activeObject].y,
        m_objectScale[m_activeObject].z
    );
    DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(
        m_objectRot[m_activeObject].x,
        m_objectRot[m_activeObject].y,
        m_objectRot[m_activeObject].z
    );
    DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(
        m_objectPos[m_activeObject].x,
        m_objectPos[m_activeObject].y,
        m_objectPos[m_activeObject].z
    );

    DirectX::XMMATRIX W = S * R * T;

    DirectX::XMVECTOR v = DirectX::XMVectorSet(p.x, p.y, p.z, 1.0f);
    DirectX::XMVECTOR r = DirectX::XMVector3TransformCoord(v, W);

    DirectX::XMFLOAT3 out;
    DirectX::XMStoreFloat3(&out, r);
    return out;
}

DirectX::XMFLOAT3 App::WorldPointToLocal(const DirectX::XMFLOAT3& p) const {
    if (m_activeObject >= m_objectCount) return p;

    DirectX::XMMATRIX S = DirectX::XMMatrixScaling(
        m_objectScale[m_activeObject].x,
        m_objectScale[m_activeObject].y,
        m_objectScale[m_activeObject].z
    );
    DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(
        m_objectRot[m_activeObject].x,
        m_objectRot[m_activeObject].y,
        m_objectRot[m_activeObject].z
    );
    DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(
        m_objectPos[m_activeObject].x,
        m_objectPos[m_activeObject].y,
        m_objectPos[m_activeObject].z
    );

    DirectX::XMMATRIX W = S * R * T;
    DirectX::XMMATRIX invW = DirectX::XMMatrixInverse(nullptr, W);

    DirectX::XMVECTOR v = DirectX::XMVectorSet(p.x, p.y, p.z, 1.0f);
    DirectX::XMVECTOR r = DirectX::XMVector3TransformCoord(v, invW);

    DirectX::XMFLOAT3 out;
    DirectX::XMStoreFloat3(&out, r);
    return out;
}

bool App::WorldToScreen(const DirectX::XMFLOAT3& worldPos, float& outScreenX, float& outScreenY) const {
    if (!m_hwnd) return false;

    RECT rc;
    GetClientRect(m_hwnd, &rc);

    float width = float(rc.right - rc.left);
    float height = float(rc.bottom - rc.top);

    if (width <= 0.0f || height <= 0.0f) return false;

    DirectX::XMMATRIX VP = DirectX::XMLoadFloat4x4(&m_camera.ViewProj());
    DirectX::XMVECTOR p = DirectX::XMVectorSet(worldPos.x, worldPos.y, worldPos.z, 1.0f);
    DirectX::XMVECTOR clip = DirectX::XMVector4Transform(p, VP);

    float w = DirectX::XMVectorGetW(clip);
    if (fabsf(w) < 1e-6f) return false;
    if (w <= 0.0f) return false;

    float ndcX = DirectX::XMVectorGetX(clip) / w;
    float ndcY = DirectX::XMVectorGetY(clip) / w;

    outScreenX = (ndcX * 0.5f + 0.5f) * width;
    outScreenY = (-ndcY * 0.5f + 0.5f) * height;
    return true;
}

GizmoTarget App::BuildGizmoTarget() const {
    GizmoTarget target = {};
    target.editMesh = m_editMesh;
    target.activeObject = m_activeObject;
    target.selectedVertex = m_selectedVertex;

    if (m_activeObject < m_objectCount) {
        target.transform.pos = const_cast<DirectX::XMFLOAT3*>(&m_objectPos[m_activeObject]);
        target.transform.rot = &m_objectRot[m_activeObject];
        target.transform.scale = &m_objectScale[m_activeObject];
    }

    return target;
}

GizmoUpdateArgs App::BuildGizmoUpdateArgs(bool& outRenderMeshDirty) {
    GizmoUpdateArgs args = {};
    args.engine = m_engine;
    args.hwnd = m_hwnd;
    args.camera = &m_camera;
    args.target = BuildGizmoTarget();
    args.rmbDown = m_input.rmbDown;
    args.rmbPressed = m_input.rmbPressed;
    args.lmbDown = m_input.lmbDown;
    args.lmbPressed = m_input.lmbPressed;
    args.lmbReleased = m_input.lmbReleased;
    args.mouseX = m_input.mouseX;
    args.mouseY = m_input.mouseY;
    args.outRenderMeshDirty = &outRenderMeshDirty;
    return args;
}

void App::UpdateViewProj() {
    m_camera.UpdateMatrices();
    m_engine->SetViewProj(m_camera.ViewProj());
}

void App::FocusCamera() {
    if (m_activeObject >= m_objectCount)
        return;

    DirectX::XMFLOAT3 center = m_objectPos[m_activeObject];

    DirectX::XMFLOAT3 scale = m_objectScale[m_activeObject];
    float maxScale = (std::max)(scale.x, (std::max)(scale.y, scale.z));

    float baseRadius = 0.75f;
    float radius = baseRadius * maxScale;
    radius = (std::max)(radius, 0.5f);

    float dist = radius / std::tan(m_camera.FovY() * 0.5f);
    dist = (std::max)(dist, 1.0f);

    DirectX::XMFLOAT3 forward = m_camera.Forward();

    DirectX::XMFLOAT3 pos = {
        center.x - forward.x * dist,
        center.y - forward.y * dist,
        center.z - forward.z * dist
    };

    m_camera.SetPosition(pos);
    m_viewPivot = center;
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

bool App::GetActiveObjectTransform(DirectX::XMFLOAT3& pos, DirectX::XMFLOAT3& rot, DirectX::XMFLOAT3& scale) const {
    if (m_activeObject >= m_objectCount)
        return false;

    pos = m_objectPos[m_activeObject];
    rot = m_objectRot[m_activeObject];
    scale = m_objectScale[m_activeObject];
    return true;
}

bool App::SetActiveObjectTransform(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& rot, const DirectX::XMFLOAT3& scale) {
    if (m_activeObject >= m_objectCount)
        return false;

    DirectX::XMFLOAT3 safeScale = scale;
    const float minScale = 0.001f;
    if (fabsf(safeScale.x) < minScale) safeScale.x = (safeScale.x < 0.0f) ? -minScale : minScale;
    if (fabsf(safeScale.y) < minScale) safeScale.y = (safeScale.y < 0.0f) ? -minScale : minScale;
    if (fabsf(safeScale.z) < minScale) safeScale.z = (safeScale.z < 0.0f) ? -minScale : minScale;

    m_objectPos[m_activeObject] = pos;
    m_objectRot[m_activeObject] = rot;
    m_objectScale[m_activeObject] = safeScale;
    return true;
}

void App::SetActiveObject(uint32_t index) {
    if (index >= m_objectCount)
        return;

    m_activeObject = index;
    m_selectedVertex = -1;
    m_isDragging = false;
    m_gizmo.Reset();

    if (m_renderMesh && m_colorsInit) {
        for (int i = 0; i < 12; ++i) {
            m_renderMesh->drawVertices[i].color = m_baseColors[i];
        }

        m_renderMesh->dirty = true;
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

bool App::ExecuteCommand(EditorCommandType type) {
    return ExecuteCommand(EditorCommand{ type });
}

bool App::ExecuteCommand(const EditorCommand& command) {
    bool ok = false;

    switch (command.type) {
    case EditorCommandType::AddObject:
        ok = AddObject(command.pos);
        break;

    case EditorCommandType::DuplicateActiveObject:
        ok = DuplicateActiveObject();
        break;

    case EditorCommandType::DeleteActiveObject:
        ok = DeleteActiveObject();
        break;

    case EditorCommandType::SaveScene:
        if (!command.path) return false;
        ok = SaveSceneAem(command.path);
        break;

    case EditorCommandType::LoadScene:
        if (!command.path) return false;
        ok = LoadSceneAem(command.path);
        break;

    case EditorCommandType::SetActiveObject:
        if (command.objectIndex >= m_objectCount) return false;
        SetActiveObject(command.objectIndex);
        ok = true;
        break;

    case EditorCommandType::SetActiveTransform:
        ok = SetActiveObjectTransform(command.pos, command.rot, command.scale);
        break;

    case EditorCommandType::SetGizmoMode:
        if (command.gizmoMode > (uint32_t)GizmoMode::Rotate) return false;
        SetGizmoMode((GizmoMode)command.gizmoMode);
        ok = true;
        break;

    case EditorCommandType::FocusCamera:
        FocusCamera();
        ok = true;
        break;

    default:
        return false;
    }

    if (ok) {
        m_lastCommandName = CommandName(command.type);
    }

    return ok;
}