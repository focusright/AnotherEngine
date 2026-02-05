#pragma once

#include "Engine.h"

struct EditableMesh;
struct RenderMesh;

class App {
public:
    void SetEngine(Engine* engine) { m_engine = engine; }
    void SetMeshes(EditableMesh* editMesh, RenderMesh* renderMesh) { m_editMesh = editMesh; m_renderMesh = renderMesh; }

    Engine* GetEngine() const { return m_engine; }
    EditableMesh* EditMesh() const { return m_editMesh; }
    RenderMesh* RenderMeshPtr() const { return m_renderMesh; }

private:
    Engine* m_engine = nullptr;           // owned elsewhere for now (main.cpp baseline)
    EditableMesh* m_editMesh = nullptr;   // owned elsewhere for now
    RenderMesh* m_renderMesh = nullptr;   // owned elsewhere for now
};
