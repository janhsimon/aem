# What is *AEM*?

*AEM* is ***a**n **e**xcellent **m**odel* file format. :wink:

It describes game-ready 3D models with everything they need:
- Vertex normals and tangents for lighting and normal mapping, and UV coordinates for texture mapping
- Separate triangle data to use with an index buffer
- Separate meshes with material information
- Skeletal animation support

This repository contains a Blender add-on that exports your models to *AEM* in a single click.


# Specification

## Definitions

- All bytes in *AEM* files are always in big-endian order. 
- All matrices in *AEM* files are always stored in column-major order.


## File Header

| Offset | Size | Description         | Data Type        |
| ------ | ---- | ------------------- | ---------------- |
| 0      | 3    | Magic number        | String           |
| 3      | 1    | Version number      | Unsigned integer |
| 4      | 8    | Number of vertices  | Unsigned integer |
| 12     | 8    | Number of triangles | Unsigned integer |
| 20     | 4    | Number of meshes    | Unsigned integer |
| 24     | 4    | Number of bones     | Unsigned integer |

The magic number is always "AEM" in ASCII (`0x41 45 4D`). This specification describes the version 1 of the file format.


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

The bone indices index into the [bone section](#bone-section). The sum of all bone weights is always 1.


## Triangle Section

| Offset | Size | Description      | Data Type        |
| ------ | ---- | ---------------- | ---------------- |
| 0      | 8    | Triangle index 1 | Unsigned integer |
| 8      | 8    | Triangle index 2 | Unsigned integer |
| 16     | 8    | Triangle index 3 | Unsigned integer |
| ...    | ...  | ...              | ...              |

(The fields above are repeated for each triangle in the model.)

The triangle indices index into the [vertex section](#vertex-section).


## Mesh Section

| Offset | Size | Description                | Data Type        |
| ------ | ---- | -------------------------- | ---------------- |
| 0      | 8    | First triangle index       | Unsigned integer |
| 8      | 8    | Number of triangle indices | Unsigned integer |
| ...    | ...  | ...                        | ...              |

(The fields above are repeated for each mesh in the model.)

The first triangle indices index into the [triangle section](#triangle-section).


## Bone Section

| Offset | Size | Description              | Data Type  |
| ------ | ---- | ------------------------ | ---------- |
| 0      | 64   | Inverse bind pose matrix | Matrix 4x4 |
| ...    | ...  | ...                      | ...        |

(The field above is repeated for each bone in the model.)