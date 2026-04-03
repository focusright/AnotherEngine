#include "Gizmo.h"

#include <cmath>

#include "EditableMesh.h"
#include "EditorCamera.h"
#include "RenderMesh.h"

using namespace DirectX;

void Gizmo::Reset() {
    m_activeAxis = -1;
    m_hotAxis = -1;
    m_dragging = false;
    m_dragT0 = 0.0f;
    m_startPos = XMFLOAT3(0.0f, 0.0f, 0.0f);
    m_startScale = XMFLOAT3(1.0f, 1.0f, 1.0f);
    m_startRot = XMFLOAT3(0.0f, 0.0f, 0.0f);
    m_dragStartMouseX = 0;
    m_dragStartMouseY = 0;
    m_rotateAxisScreenDir = XMFLOAT2(1.0f, 0.0f);
}

void Gizmo::SetMode(GizmoMode mode) {
    if (m_mode == mode)
        return;

    Reset();
    m_mode = mode;
}

const char* Gizmo::GetModeName() const {
    switch (m_mode) {
    case GizmoMode::Translate: return "Translate";
    case GizmoMode::Scale: return "Scale";
    case GizmoMode::Rotate: return "Rotate";
    default: return "Unknown";
    }
}

XMFLOAT3 Gizmo::LocalVertexToWorld(const XMFLOAT3& point, const XMFLOAT3& objectPos, const XMFLOAT3& objectRot, const XMFLOAT3& objectScale) const {
    XMMATRIX scaleMat = XMMatrixScaling(objectScale.x, objectScale.y, objectScale.z);
    XMMATRIX rotMat = XMMatrixRotationRollPitchYaw(objectRot.x, objectRot.y, objectRot.z);
    XMMATRIX transMat = XMMatrixTranslation(objectPos.x, objectPos.y, objectPos.z);

    XMMATRIX worldMat = scaleMat * rotMat * transMat;

    XMVECTOR local = XMVectorSet(point.x, point.y, point.z, 1.0f);
    XMVECTOR world = XMVector3TransformCoord(local, worldMat);

    XMFLOAT3 out;
    XMStoreFloat3(&out, world);
    return out;
}

XMFLOAT3 Gizmo::WorldPointToLocal(const XMFLOAT3& point, const XMFLOAT3& objectPos, const XMFLOAT3& objectRot, const XMFLOAT3& objectScale) const {
    XMMATRIX scaleMat = XMMatrixScaling(objectScale.x, objectScale.y, objectScale.z);
    XMMATRIX rotMat = XMMatrixRotationRollPitchYaw(objectRot.x, objectRot.y, objectRot.z);
    XMMATRIX transMat = XMMatrixTranslation(objectPos.x, objectPos.y, objectPos.z);

    XMMATRIX worldMat = scaleMat * rotMat * transMat;
    XMMATRIX invWorldMat = XMMatrixInverse(nullptr, worldMat);

    XMVECTOR world = XMVectorSet(point.x, point.y, point.z, 1.0f);
    XMVECTOR local = XMVector3TransformCoord(world, invWorldMat);

    XMFLOAT3 out;
    XMStoreFloat3(&out, local);
    return out;
}

XMFLOAT3 Gizmo::GetOrigin(const GizmoTarget& target) const {
    if (target.selectedVertex != -1) {
        XMFLOAT3 localPos = target.editMesh->GetVertex((VertexID)target.selectedVertex);
        return LocalVertexToWorld(localPos, *target.transform.pos, *target.transform.rot, *target.transform.scale);
    }

    return *target.transform.pos;
}

bool Gizmo::ComputeAxisScreenDirection(HWND hwnd, EditorCamera& camera, const GizmoTarget& target, int axis, XMFLOAT2& outDir) const {
    if (target.activeObject == UINT32_MAX)
        return false;

    RECT rc;
    GetClientRect(hwnd, &rc);

    float width = float(rc.right - rc.left);
    float height = float(rc.bottom - rc.top);

    if (width <= 0.0f || height <= 0.0f)
        return false;

    XMFLOAT3 origin = GetOrigin(target);
    XMFLOAT3 axisDir = (axis == 0) ? XMFLOAT3(1.0f, 0.0f, 0.0f) : (axis == 1) ? XMFLOAT3(0.0f, 1.0f, 0.0f) : XMFLOAT3(0.0f, 0.0f, 1.0f);

    XMFLOAT3 end(
        origin.x + axisDir.x * kAxisLen,
        origin.y + axisDir.y * kAxisLen,
        origin.z + axisDir.z * kAxisLen
    );

    XMMATRIX viewProj = XMLoadFloat4x4(&camera.ViewProj());

    auto worldToScreen = [&](const XMFLOAT3& point, float& outX, float& outY) {
        XMVECTOR p = XMVectorSet(point.x, point.y, point.z, 1.0f);
        XMVECTOR clip = XMVector4Transform(p, viewProj);

        float w = XMVectorGetW(clip);
        if (fabsf(w) < 1e-6f)
            return false;
        if (w <= 0.0f)
            return false;

        float ndcX = XMVectorGetX(clip) / w;
        float ndcY = XMVectorGetY(clip) / w;

        outX = (ndcX * 0.5f + 0.5f) * width;
        outY = (-ndcY * 0.5f + 0.5f) * height;
        return true;
        };

    float x0 = 0.0f, y0 = 0.0f;
    float x1 = 0.0f, y1 = 0.0f;

    if (!worldToScreen(origin, x0, y0))
        return false;
    if (!worldToScreen(end, x1, y1))
        return false;

    float dx = x1 - x0;
    float dy = y1 - y0;
    float len = std::sqrt(dx * dx + dy * dy);

    if (len < 1e-4f)
        return false;

    outDir.x = dx / len;
    outDir.y = dy / len;
    return true;
}

bool Gizmo::PickAxis(EditorCamera& camera, const GizmoTarget& target, int mouseX, int mouseY, int& outAxis, float& outTOnAxis) {
    if (target.activeObject == UINT32_MAX)
        return false;

    XMFLOAT3 rayOrigin, rayDir;
    camera.BuildRayFromScreen(float(mouseX), float(mouseY), rayOrigin, rayDir);

    XMFLOAT3 origin = GetOrigin(target);
    const float axisLen = kAxisLen;

    auto testAxis = [&](const XMFLOAT3& axisDir, float& outTRaw, float& outDistSq) {
        XMVECTOR rayOriginVec = XMLoadFloat3(&rayOrigin);
        XMVECTOR rayDirVec = XMVector3Normalize(XMLoadFloat3(&rayDir));
        XMVECTOR originVec = XMLoadFloat3(&origin);
        XMVECTOR axisDirVec = XMVector3Normalize(XMLoadFloat3(&axisDir));

        XMVECTOR segStart = originVec;
        XMVECTOR segEnd = XMVectorAdd(originVec, XMVectorScale(axisDirVec, axisLen));

        XMVECTOR w0 = XMVectorSubtract(rayOriginVec, segStart);
        float a = XMVectorGetX(XMVector3Dot(rayDirVec, rayDirVec));
        float b = XMVectorGetX(XMVector3Dot(rayDirVec, axisDirVec));
        float c = XMVectorGetX(XMVector3Dot(axisDirVec, axisDirVec));
        float d = XMVectorGetX(XMVector3Dot(rayDirVec, w0));
        float e = XMVectorGetX(XMVector3Dot(axisDirVec, w0));
        float denom = a * c - b * b;

        float rayT = 0.0f;
        float axisT = 0.0f;

        if (fabsf(denom) > 1e-6f) {
            rayT = (b * e - c * d) / denom;
            axisT = (a * e - b * d) / denom;
        }

        if (rayT < 0.0f)
            rayT = 0.0f;

        outTRaw = axisT;

        if (axisT < 0.0f)
            axisT = 0.0f;
        if (axisT > axisLen)
            axisT = axisLen;

        XMVECTOR closestRay = XMVectorAdd(rayOriginVec, XMVectorScale(rayDirVec, rayT));
        XMVECTOR closestAxis = XMVectorAdd(segStart, XMVectorScale(axisDirVec, axisT));
        XMVECTOR diff = XMVectorSubtract(closestRay, closestAxis);

        outDistSq = XMVectorGetX(XMVector3Dot(diff, diff));
        };

    float bestDistSq = 1e30f;
    int bestAxis = -1;
    float bestTRaw = 0.0f;

    float tRaw = 0.0f;
    float distSq = 0.0f;

    testAxis(XMFLOAT3(1.0f, 0.0f, 0.0f), tRaw, distSq);
    if (distSq < bestDistSq) {
        bestDistSq = distSq;
        bestAxis = 0;
        bestTRaw = tRaw;
    }

    testAxis(XMFLOAT3(0.0f, 1.0f, 0.0f), tRaw, distSq);
    if (distSq < bestDistSq) {
        bestDistSq = distSq;
        bestAxis = 1;
        bestTRaw = tRaw;
    }

    testAxis(XMFLOAT3(0.0f, 0.0f, 1.0f), tRaw, distSq);
    if (distSq < bestDistSq) {
        bestDistSq = distSq;
        bestAxis = 2;
        bestTRaw = tRaw;
    }

    const float thresh = kPickThresh;
    if (bestDistSq > thresh * thresh)
        return false;

    outAxis = bestAxis;
    outTOnAxis = bestTRaw;
    return true;
}

//Take the mouse cursor, cast it into 3D, intersect it with a plane that contains the gizmo axis, then measure how far along the axis that hit point lies.
bool Gizmo::ComputeTOnAxis(EditorCamera& camera, const GizmoTarget& target, int axis, int mouseX, int mouseY, float& outTOnAxis) {
    if (target.activeObject == UINT32_MAX)
        return false;

    XMFLOAT3 rayOrigin, rayDir;
    camera.BuildRayFromScreen(float(mouseX), float(mouseY), rayOrigin, rayDir);

    XMFLOAT3 origin = GetOrigin(target);

    if (m_dragging && m_activeAxis == axis) //make sure the gizmo origin doesn't move while dragging
        origin = m_startPos;

    XMFLOAT3 axisDir = (axis == 0) ? XMFLOAT3(1.0f, 0.0f, 0.0f) : (axis == 1) ? XMFLOAT3(0.0f, 1.0f, 0.0f) : XMFLOAT3(0.0f, 0.0f, 1.0f);

    XMVECTOR rayOriginVec = XMLoadFloat3(&rayOrigin);                 //Imagine dragging along X:                                                                   
    XMVECTOR rayDirVec = XMVector3Normalize(XMLoadFloat3(&rayDir));   //draw the X axis through the object                                                            
    XMVECTOR originVec = XMLoadFloat3(&origin);                       //build a sheet of paper containing that X axis                                                  
    XMVECTOR axisDirVec = XMVector3Normalize(XMLoadFloat3(&axisDir)); //rotate that sheet so it faces the camera nicely                                                   
                                                                      //shoot the mouse ray into the sheet                                                               
    XMFLOAT3 camForward = camera.Forward();                           //wherever it lands, drop that point onto the X axis                                                 
    XMVECTOR viewDir = XMVector3Normalize(XMLoadFloat3(&camForward)); //the signed distance along X is t

    XMVECTOR planeNormal = XMVector3Cross(axisDirVec, XMVector3Cross(viewDir, axisDirVec)); //creates the normal of a plane that contains axisDir & faces the camera
    float normalLenSq = XMVectorGetX(XMVector3Dot(planeNormal, planeNormal));

    if (normalLenSq < 1e-8f) {
        XMVECTOR fallback = (fabsf(XMVectorGetY(axisDirVec)) < 0.9f) ? XMVectorSet(0, 1, 0, 0) : XMVectorSet(1, 0, 0, 0);
        planeNormal = XMVector3Cross(axisDirVec, XMVector3Cross(fallback, axisDirVec));
        normalLenSq = XMVectorGetX(XMVector3Dot(planeNormal, planeNormal));
        if (normalLenSq < 1e-8f)
            return false;
    }

    planeNormal = XMVector3Normalize(planeNormal);

    float denom = XMVectorGetX(XMVector3Dot(rayDirVec, planeNormal));
    if (fabsf(denom) < 1e-6f)
        return false;

    float rayT = XMVectorGetX(XMVector3Dot(XMVectorSubtract(originVec, rayOriginVec), planeNormal)) / denom;
    if (rayT < 0.0f) //solving for t: dot((rayOrigin + t * rayDir) - origin, planeNormal) = 0
        rayT = 0.0f;

    XMVECTOR hitPoint = XMVectorAdd(rayOriginVec, XMVectorScale(rayDirVec, rayT));
    float axisT = XMVectorGetX(XMVector3Dot(XMVectorSubtract(hitPoint, originVec), axisDirVec));

    outTOnAxis = axisT;
    return true;
}

void Gizmo::BuildVertices(Vertex* gizmoVerts, HWND hwnd, EditorCamera& camera, const GizmoTarget& target) {
    XMFLOAT3 origin = GetOrigin(target);
    const float len = kAxisLen;

    XMFLOAT4 hotColor(1.0f, 1.0f, 1.0f, 1.0f);
    XMFLOAT4 activeColor(1.0f, 1.0f, 0.0f, 1.0f);

    XMFLOAT4 colorX(1.0f, 0.0f, 0.0f, 1.0f);
    XMFLOAT4 colorY(0.0f, 1.0f, 0.0f, 1.0f);
    XMFLOAT4 colorZ(0.2f, 0.5f, 1.0f, 1.0f);

    if (m_activeAxis == 0) colorX = activeColor; else if (m_hotAxis == 0) colorX = hotColor;
    if (m_activeAxis == 1) colorY = activeColor; else if (m_hotAxis == 1) colorY = hotColor;
    if (m_activeAxis == 2) colorZ = activeColor; else if (m_hotAxis == 2) colorZ = hotColor;

    const float thicknessPx = 3.0f;

    RECT rc;
    GetClientRect(hwnd, &rc);
    float viewH = float(rc.bottom - rc.top);
    if (viewH < 1.0f)
        viewH = 1.0f;

    XMFLOAT3 camPos = camera.Position();
    XMFLOAT3 camForward = camera.Forward();
    XMFLOAT3 camRight = camera.Right();

    auto addAxisQuad = [&](int baseIndex, const XMFLOAT3& axisDir, const XMFLOAT4& color) {
        XMFLOAT3 p0 = origin;
        XMFLOAT3 p1(
            origin.x + axisDir.x * len,
            origin.y + axisDir.y * len,
            origin.z + axisDir.z * len
        );

        XMFLOAT3 toP0(p0.x - camPos.x, p0.y - camPos.y, p0.z - camPos.z);
        XMFLOAT3 toP1(p1.x - camPos.x, p1.y - camPos.y, p1.z - camPos.z);

        float z0 = toP0.x * camForward.x + toP0.y * camForward.y + toP0.z * camForward.z;
        float z1 = toP1.x * camForward.x + toP1.y * camForward.y + toP1.z * camForward.z;

        if (z0 < 0.05f) z0 = 0.05f;
        if (z1 < 0.05f) z1 = 0.05f;

        float tanHalfFov = std::tan(camera.FovY() * 0.5f);

        float worldThickness0 = thicknessPx * (2.0f * z0 * tanHalfFov) / viewH;
        float worldThickness1 = thicknessPx * (2.0f * z1 * tanHalfFov) / viewH;

        float halfW0 = worldThickness0 * 0.5f;
        float halfW1 = worldThickness1 * 0.5f;

        XMFLOAT3 sideDir(
            axisDir.y * camForward.z - axisDir.z * camForward.y,
            axisDir.z * camForward.x - axisDir.x * camForward.z,
            axisDir.x * camForward.y - axisDir.y * camForward.x
        );

        float sideLen = std::sqrt(sideDir.x * sideDir.x + sideDir.y * sideDir.y + sideDir.z * sideDir.z);

        if (sideLen < 1e-4f) {
            sideDir = camRight;
            sideLen = std::sqrt(sideDir.x * sideDir.x + sideDir.y * sideDir.y + sideDir.z * sideDir.z);
        }

        sideDir.x /= sideLen;
        sideDir.y /= sideLen;
        sideDir.z /= sideLen;

        XMFLOAT3 side0(sideDir.x * halfW0, sideDir.y * halfW0, sideDir.z * halfW0);
        XMFLOAT3 side1(sideDir.x * halfW1, sideDir.y * halfW1, sideDir.z * halfW1);

        XMFLOAT3 a(p0.x + side0.x, p0.y + side0.y, p0.z + side0.z);
        XMFLOAT3 b(p1.x + side1.x, p1.y + side1.y, p1.z + side1.z);
        XMFLOAT3 c(p1.x - side1.x, p1.y - side1.y, p1.z - side1.z);
        XMFLOAT3 d(p0.x - side0.x, p0.y - side0.y, p0.z - side0.z);

        gizmoVerts[baseIndex + 0] = { a, color };
        gizmoVerts[baseIndex + 1] = { b, color };
        gizmoVerts[baseIndex + 2] = { c, color };
        gizmoVerts[baseIndex + 3] = { a, color };
        gizmoVerts[baseIndex + 4] = { c, color };
        gizmoVerts[baseIndex + 5] = { d, color };
        };

    addAxisQuad(0, XMFLOAT3(1.0f, 0.0f, 0.0f), colorX);
    addAxisQuad(6, XMFLOAT3(0.0f, 1.0f, 0.0f), colorY);
    addAxisQuad(12, XMFLOAT3(0.0f, 0.0f, 1.0f), colorZ);
}

void Gizmo::Update(const GizmoUpdateArgs& args) {
    if (args.outRenderMeshDirty) { *args.outRenderMeshDirty = false; }

    if (!args.engine ||
        !args.camera ||
        !args.outRenderMeshDirty ||
        args.target.activeObject == UINT32_MAX ||
        !args.target.editMesh ||
        !args.target.transform.pos ||
        !args.target.transform.rot ||
        !args.target.transform.scale) { return; }

    EditorCamera& camera = *args.camera;
    EditableMesh* editMesh = args.target.editMesh;

    GizmoTarget target = args.target;
    if (m_mode != GizmoMode::Translate) { target.selectedVertex = -1; }

    DirectX::XMFLOAT3& objectPos = *target.transform.pos;
    DirectX::XMFLOAT3& objectRot = *target.transform.rot;
    DirectX::XMFLOAT3& objectScale = *target.transform.scale;
    int selectedVertex = target.selectedVertex;

    if (!m_dragging && !args.rmbDown) {
        m_hotAxis = -1;
        int axis = -1;
        float axisT = 0.0f;
        if (PickAxis(camera, target, args.mouseX, args.mouseY, axis, axisT)) { m_hotAxis = axis; }
    } else {
        m_hotAxis = -1;
    }

    Vertex gizmoVerts[kVertexCount] = {};
    BuildVertices(gizmoVerts, args.hwnd, camera, target);
    args.engine->UpdateGizmoVertices(gizmoVerts, kVertexCount, args.hwnd);

    if (!args.rmbDown && args.lmbPressed && !m_dragging) {
        int axis = -1;
        float axisT0 = 0.0f;

        if (PickAxis(camera, target, args.mouseX, args.mouseY, axis, axisT0)) {
            m_activeAxis = axis;
            m_dragging = true;

            if (selectedVertex != -1) {
                XMFLOAT3 localPos = editMesh->GetVertex((VertexID)selectedVertex);
                m_startPos = LocalVertexToWorld(localPos, objectPos, objectRot, objectScale);
            } else {
                m_startPos = objectPos;
            }

            m_startScale = objectScale;
            m_startRot = objectRot;

            if (m_mode == GizmoMode::Rotate) {
                m_dragStartMouseX = args.mouseX;
                m_dragStartMouseY = args.mouseY;

                if (!ComputeAxisScreenDirection(args.hwnd, camera, target, m_activeAxis, m_rotateAxisScreenDir)) {
                    m_dragging = false;
                    m_activeAxis = -1;
                }
            } else {
                if (!ComputeTOnAxis(camera, target, m_activeAxis, args.mouseX, args.mouseY, m_dragT0)) { m_dragT0 = axisT0; }
            }
        }
    }

    if (m_dragging && args.lmbDown && m_activeAxis != -1) {
        float axisT = 0.0f;
        XMFLOAT3 axisDir = (m_activeAxis == 0) ? XMFLOAT3(1.0f, 0.0f, 0.0f) : (m_activeAxis == 1) ? XMFLOAT3(0.0f, 1.0f, 0.0f) : XMFLOAT3(0.0f, 0.0f, 1.0f);

        if (m_mode == GizmoMode::Rotate) {
            const float rotateSpeed = 0.01f;
            float mouseDx = float(args.mouseX - m_dragStartMouseX);
            float mouseDy = float(args.mouseY - m_dragStartMouseY);
            float dragAlongAxis = mouseDx * m_rotateAxisScreenDir.x + mouseDy * m_rotateAxisScreenDir.y;
            float deltaAngle = dragAlongAxis * rotateSpeed;

            objectRot = m_startRot;

            if (m_activeAxis == 0) {
                objectRot.x += deltaAngle;
            } else if (m_activeAxis == 1) {
                objectRot.y += deltaAngle;
            } else if (m_activeAxis == 2) {
                objectRot.z += deltaAngle;
            }
        } else {
            float axisT = 0.0f;

            if (ComputeTOnAxis(camera, target, m_activeAxis, args.mouseX, args.mouseY, axisT)) {
                float deltaT = axisT - m_dragT0;

                if (m_mode == GizmoMode::Translate) {
                    XMFLOAT3 newPos(
                        m_startPos.x + axisDir.x * deltaT,
                        m_startPos.y + axisDir.y * deltaT,
                        m_startPos.z + axisDir.z * deltaT
                    );

                    if (selectedVertex != -1) {
                        XMFLOAT3 localPos = WorldPointToLocal(newPos, objectPos, objectRot, objectScale);
                        editMesh->SetVertex((VertexID)selectedVertex, localPos);
                        *args.outRenderMeshDirty = true;
                    } else {
                        objectPos = newPos;
                    }
                } else if (m_mode == GizmoMode::Scale) {
                    const float minScale = 0.05f;

                    if (m_activeAxis == 0) {
                        objectScale.x = m_startScale.x + deltaT;
                        if (objectScale.x < minScale) { objectScale.x = minScale; }
                    } else if (m_activeAxis == 1) {
                        objectScale.y = m_startScale.y + deltaT;
                        if (objectScale.y < minScale) { objectScale.y = minScale; }
                    } else if (m_activeAxis == 2) {
                        objectScale.z = m_startScale.z + deltaT;
                        if (objectScale.z < minScale) { objectScale.z = minScale; }
                    }
                }
            }
        }
    }

    if (m_dragging && args.lmbReleased) {
        m_dragging = false;
        m_activeAxis = -1;
    }

    if (m_dragging && args.rmbPressed) {
        m_dragging = false;
        m_activeAxis = -1;
    }
}