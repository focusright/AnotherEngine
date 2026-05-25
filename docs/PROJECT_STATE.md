# AEDD PROJECT STATE

## Current Version
v0.0.3 - bounded editable mesh capacity/counts

## Status
In progress.

## Current Focus
AE is turning the modeling mesh from fixed tetra-only arrays into bounded editable topology with active counts.

## v0.0.3 Current Step
The second v0.0.3 step keeps the visible tetrahedron the same but changes the mesh storage model:

- `EditableMesh` now has `kMaxVertices` and `kMaxTriangles`
- `EditableMesh` now tracks active `vertexCount` and `triangleCount`
- vertices and triangles are added through `AddVertex` and `AddTriangle`
- `BuildTetrahedron` now builds through the same add path future modeling operations will use
- `RenderMesh` now has `kMaxDrawVertexCount`
- `RenderMesh` now tracks active `drawVertexCount`
- `RenderMesh::BuildFromEditable` converts editable topology into GPU draw vertices
- the renderer draws `m_meshDrawVertexCount` instead of a hardcoded tetra draw count

## Why This Matters
Topology editing needs room to grow and shrink the mesh. This step gives AE bounded mesh capacity and active counts before adding operations such as extrude, delete face, and face split.

## Next Planned Step
v0.0.3 Task 3 - add triangle/face picking as a modeling selection primitive.