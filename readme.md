# What is *AEM*?

*AEM* is ***a**n **e**xcellent **m**odel* file format. :wink:

It describes game-ready 3D models with everything they need:
- Vertex normals and tangents for lighting and normal mapping, and UV coordinates for texture mapping
- Separate triangle data to use with an index buffer
- Separate meshes with material information
- Skeletal animation support

This repository contains a Blender add-on that exports your models to *AEM* in a single click. It also comes with a simple viewer application written in C++ that shows how to load and render *AEM* models with modern OpenGL.


# Specification

## Definitions

- All offsets and sizes in this specification are given in bytes.
- All bytes in *AEM* files are stored in little-endian order.
- All matrices in *AEM* files are stored in column-major order.


## File Header

| Offset | Size | Description          | Data Type        |
| ------ | ---- | -------------------- | ---------------- |
| 0      | 3    | Magic number         | String           |
| 3      | 1    | Version number       | Unsigned integer |
| 4      | 4    | Number of vertices   | Unsigned integer |
| 8      | 4    | Number of triangles  | Unsigned integer |
| 12     | 4    | Number of meshes     | Unsigned integer |
| 16     | 4    | Number of bones      | Unsigned integer |
| 20     | 4    | Number of keyframes  | Unsigned integer |

The magic number is always "AEM" in ASCII (`0x41 45 4D`). This specification describes version 1 of the file format.


## Vertex Section

| Offset | Size | Description   | Data Type        |
| ------ | ---- | ------------- | ---------------- |
| 0      | 4    | Position X    | Float            |
| 4      | 4    | Position Y    | Float            |
| 8      | 4    | Position Z    | Float            |
| 12     | 4    | Normal X      | Float            |
| 16     | 4    | Normal Y      | Float            |
| 20     | 4    | Normal Z      | Float            |
| 24     | 4    | Tangent X     | Float            |
| 28     | 4    | Tangent Y     | Float            |
| 32     | 4    | Tangent Z     | Float            |
| 36     | 4    | Texture U     | Float            |
| 40     | 4    | Texture V     | Float            |
| 44     | 4    | Bone index 1  | Unsigned integer |
| 48     | 4    | Bone index 2  | Unsigned integer |
| 52     | 4    | Bone index 3  | Unsigned integer |
| 56     | 4    | Bone index 4  | Unsigned integer |
| 60     | 4    | Bone weight 1 | Float            |
| 64     | 4    | Bone weight 2 | Float            |
| 68     | 4    | Bone weight 3 | Float            |
| 72     | 4    | Bone weight 4 | Float            |
| ...    | ...  | ...           | ...              |

(The fields above are repeated for each vertex in the model.)

The bone indices index into the [bone section](#bone-section). The sum of all bone weights is always 1. If a vertex is affected by less than 4 bones, the additional bone weights are 0.0, and the corresponding bone indices are undefined.


## Triangle Section

| Offset | Size | Description      | Data Type        |
| ------ | ---- | ---------------- | ---------------- |
| 0      | 4    | Triangle index 1 | Unsigned integer |
| 4      | 4    | Triangle index 2 | Unsigned integer |
| 8      | 4    | Triangle index 3 | Unsigned integer |
| ...    | ...  | ...              | ...              |

(The fields above are repeated for each triangle in the model.)

The triangle indices index into the [vertex section](#vertex-section).


## Mesh Section

| Offset | Size | Description         | Data Type        |
| ------ | ---- | ------------------- | ---------------- |
| 0      | 4    | Number of triangles | Unsigned integer |
| ...    | ...  | ...                 | ...              |

(The field above is repeated for each mesh in the model.)

Meshes consist of sequential triangles in the [triangle section](#triangle-section), and are themselves sequential.


## Bone Section

| Offset | Size | Description         | Data Type      |
| ------ | ---- | ------------------- | -------------- |
| 0      | 64   | Inverse bind matrix | 4x4 Matrix     |
| 64     | 4    | Parent bone index   | Signed integer |
| ...    | ...  | ...                 | ...            |

(The fields above are repeated for each bone in the model.)

The parent bone indices index into this same bone section, and will be -1 for root bones without parent.


## Keyframe Section

| Offset | Size | Description         | Data Type        |
| ------ | ---- | ------------------- | ---------------- |
| 0      | 4    | Keyframe time       | Float            |
| 4      | 64   | Posed bone matrix   | 4x4 Matrix       |
| ...    | ...  | ...                 | ...              |

(The fields above are repeated for each keyframe in the model. Additionally, the field Posed bone matrix is repeated for each bone in the model.)

Keyframes are sorted chronologically, and the first keyframe is always at time 0.0.