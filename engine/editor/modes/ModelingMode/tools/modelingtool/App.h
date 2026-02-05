#pragma once

#include <windows.h>
#include <windowsx.h>
#include "Engine.h"

struct EditableMesh;
struct RenderMesh;

class App {
public:
    void SetEngine(Engine* engine) { m_engine = engine; }
    void SetMeshes(EditableMesh* editMesh, RenderMesh* renderMesh) { m_editMesh = editMesh; m_renderMesh = renderMesh; }

    void BeginFrameInput();
    void Update(float dt);

    LRESULT HandleWindowMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool& handled);

private:
    struct InputState {
        int mouseX = 0;
        int mouseY = 0;

        bool lmbDown = false;      // current state
        bool lmbPressed = false;   // edge: went down this frame
        bool lmbReleased = false;  // edge: went up this frame
    };

    Engine* m_engine = nullptr;
    EditableMesh* m_editMesh = nullptr;
    RenderMesh* m_renderMesh = nullptr;

    InputState m_input;

    bool m_isDragging = false;
    int m_selectedVertex = -1;
    POINT m_lastMousePos = { 0, 0 };
};
