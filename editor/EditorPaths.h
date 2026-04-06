#pragma once

#include <windows.h>
#include <string>

inline std::wstring GetExecutableDirectory() {
    wchar_t modulePath[MAX_PATH] = {};
    DWORD len = GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        return L".";
    }

    std::wstring path(modulePath, len);
    size_t slash = path.find_last_of(L"\\/");
    if (slash == std::wstring::npos) {
        return L".";
    }

    path.resize(slash);
    return path;
}

inline std::wstring GetDefaultSceneDirectory() {
    std::wstring exeDir = GetExecutableDirectory();
    return exeDir + L"\\assets\\scenes";
}

inline std::wstring GetDefaultScenePath() {
    return GetDefaultSceneDirectory() + L"\\scene.aem";
}

inline void EnsureDefaultSceneDirectoryExists() {
    std::wstring exeDir = GetExecutableDirectory();
    std::wstring assetsDir = exeDir + L"\\assets";
    std::wstring scenesDir = assetsDir + L"\\scenes";

    CreateDirectoryW(assetsDir.c_str(), nullptr);
    CreateDirectoryW(scenesDir.c_str(), nullptr);
}