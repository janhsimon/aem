# What is *aem*?

*aem* is ***a**n **e**xcellent **m**odel* file format. :wink:

It describes game-ready 3D models with everything they need:
- Normals and tangents for lighting and tangent-space normal mapping
- UV coordinates for texture mapping
- Index buffer to reuse identical vertices
- Separate meshes with material information
- Skeletal animation support


# Specification

## File Header

| Offset | Size | Description                   | Data Type        |
| ------ | ---- | ----------------------------- | ---------------- |
| 0      | 3    | Magic number: AEM             | String           |
| 3      | 1    | File format version number: 1 | Unsigned integer |
| 4      | 8    | Number of vertices            | Unsigned integer |
| 12     | 8    | Number of indices             | Unsigned integer |
| 20     | 4    | Number of meshes              | Unsigned integer |
| 24     | 4    | Number of bones               | Unsigned integer |

The magic number is always "AEX" in ASCII. This specification describes the file format version number 1.

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

The fields above are repeated for each vertex in the model.

The position, normal and tangent vectors are in model-space. The bone indices index into the [bone section](#bone-section). The sum of all bone weights is always 1.


## Index Section

| Offset | Size | Description         | Data Type        |
| ------ | ---- | ------------------- | ---------------- |
| 0      | 4    | Index into vertices | Unsigned integer |
| ...    | ...  | ...                 | ...              |

The fields above are repeated for each index in the model.

The indices index into the [vertex section](#vertex-section) to form triangles.


## Mesh Section

| Offset | Size | Descripti                 | Data Type        |
| ------ | ---- | ------------------------- | ---------------- |
| 0      | 4    | Beginning of mesh indices | Unsigned integer |
| 4      | 4    | Number of mesh indices    | Unsigned integer |
| ...    | ...  | ...                       | ...              |

The fields above are repeated for each mesh in the model.

The beginning and number of mesh indices index into the [index section](#index-section).


## Bone Section

| Offset | Size | Descripti                | Data Type  |
| ------ | ---- | ------------------------ | ---------- |
| 0      | 64   | Inverse bind pose matrix | Matrix 4x4 |
| ...    | ...  | ...                      | ...        |

The field above is repeated for each bone in the model.