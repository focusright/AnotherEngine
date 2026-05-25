# AEDD PROJECT STATE

## Current Version
v0.0.3 - explicit editable mesh topology

## Status
In progress.

## Current Focus
AE is moving the modeling tool away from hardcoded draw-vertex tetrahedron setup and toward an editable CPU mesh model that owns vertices plus triangle topology.

## v0.0.3 Current Step
The first v0.0.3 step keeps the visual result the same but changes the data ownership:

- `EditableMesh` now stores tetrahedron triangle topology explicitly
- `EditableMesh::BuildTetrahedron` owns the primitive's vertex and face construction
- `CreateVertexBuffer` builds `RenderMesh` draw vertices from `EditableMesh` topology
- vertex picking uses `EditableMesh::kVertexCount` instead of a hardcoded vertex count
- selection color loops use `RenderMesh::kDrawVertexCount` instead of hardcoded draw-vertex counts

## Why This Matters
Extrude, delete face, edge transforms, face splits, and later generated geometry need topology to exist as editable CPU-side data before GPU draw vertices are rebuilt.

## Next Planned Step
Replace fixed tetra-only arrays with bounded editable mesh capacity/counts so the modeling tool can add and remove topology without changing the renderer yet.