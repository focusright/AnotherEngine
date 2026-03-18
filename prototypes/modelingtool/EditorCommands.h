#pragma once

#include <cstdint>
#include <DirectXMath.h>

enum class EditorCommandType {
    None = 0,

    // Object commands
    AddObject,
    DuplicateActiveObject,
    DeleteActiveObject,
    SetActiveObject,

    // Scene commands
    SaveScene,
    LoadScene,

    // Transform commands
    SetActiveTransform,
    FocusCamera
};

struct EditorCommand {
    EditorCommandType type = EditorCommandType::None;

    uint32_t objectIndex = 0;
    const wchar_t* path = nullptr;

    DirectX::XMFLOAT3 pos = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 rot = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };
};
