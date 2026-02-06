#pragma once

#include <windows.h>

// Forward declare to avoid include tangles.
class EditorCamera;

struct EditorContext {
    EditorCamera* camera = nullptr;   // points at the one shared camera instance
    HWND hwnd = nullptr;

    // You can grow this later:
    // - active tool/mode
    // - selection set
    // - gizmo state
    // - grid settings
};

