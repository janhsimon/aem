# What is *AEM*?

*AEM* is ***a**n **e**xcellent **m**odel* file format. :wink:

It describes game-ready 3D models with everything they need - no more, no less:
- Vertex normals, tangents and bitangents for lighting and normal mapping
- Single-channel UV coordinates for texture mapping
- Separate index data to use with an index buffer
- Standard PBR material system with base color, normal, occlusion, roughness and metalness maps
- Mesh and skeletal animations

This repository contains:
- `libaem`: A minimal and dependency-free C library that can load and animate *AEM* models efficiently
- `exporter`: An export tool that can convert virtually any 3D model into an *AEM* model
- `viewer`: A simple viewer application that shows how to load and render *AEM* models with `libaem` and OpenGL 3.3 and can be used to inspect exported *AEM* models


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
| 8      | 4    | Number of indices    | Unsigned integer |
| 12     | 4    | Number of textures   | Unsigned integer |
| 16     | 4    | Number of meshes     | Unsigned integer |
| 20     | 4    | Number of materials  | Unsigned integer |
| 24     | 4    | Number of bones      | Unsigned integer |
| 28     | 4    | Number of animations | Unsigned integer |
| 32     | 4    | Number of sequences  | Unsigned integer |
| 36     | 4    | Number of keyframes  | Unsigned integer |

The magic number is always "AEM" in ASCII (`0x41 45 4D`). This specification describes version 1 of the file format.


## Vertex Section

| Offset | Size | Description      | Data Type      |
| ------ | ---- | ---------------- | -------------- |
| 0      | 4    | Position X       | Float          |
| 4      | 4    | Position Y       | Float          |
| 8      | 4    | Position Z       | Float          |
| 12     | 4    | Normal X         | Float          |
| 16     | 4    | Normal Y         | Float          |
| 20     | 4    | Normal Z         | Float          |
| 24     | 4    | Tangent X        | Float          |
| 28     | 4    | Tangent Y        | Float          |
| 32     | 4    | Tangent Z        | Float          |
| 36     | 4    | Bitangent X      | Float          |
| 40     | 4    | Bitangent Y      | Float          |
| 44     | 4    | Bitangent Z      | Float          |
| 48     | 4    | Texture U        | Float          |
| 52     | 4    | Texture V        | Float          |
| 56     | 4    | Bone index 1     | Signed integer |
| 60     | 4    | Bone index 2     | Signed integer |
| 64     | 4    | Bone index 3     | Signed integer |
| 68     | 4    | Bone index 4     | Signed integer |
| 72     | 4    | Bone weight 1    | Float          |
| 76     | 4    | Bone weight 2    | Float          |
| 80     | 4    | Bone weight 3    | Float          |
| 84     | 4    | Bone weight 4    | Float          |
| 88     | 4    | Extra bone index | Signed integer |
| ...    | ...  | (repeat)         | ...            |

(The fields above are repeated for each vertex in the file.)

The bone indices 1-4 and extra bone indices index into the [bone section](#bone-section). The sum of all bone weights is always 1. If a vertex is affected by less than 4 bones, the additional bone weights are 0.0, and the corresponding bone indices are -1. The bone indices 1-4 express skeletal animation, while the extra bone index is always applied with 100% weight to all vertices in a mesh and expresses mesh animation.


## Index Section

| Offset | Size | Description | Data Type        |
| ------ | ---- | ----------- | ---------------- |
| 0      | 4    | Index       | Unsigned integer |
| ...    | ...  | (repeat)    | ...              |

(The field above is repeated for each index in the file.)

The indices index into the [vertex section](#vertex-section).


## Texture Section

| Offset | Size | Description | Data Type |
| ------ | ---- | ----------- | --------- |
| 0      | 128  | Filename    | String    |
| ...    | ...  | (repeat)    | ...       |

(The field above is repeated for each texture in the file.)

The filename for a texture is supplied with a null-terminated string of at most 128 characters (including the null terminator). Longer filenames are not supported.


## Mesh Section

| Offset | Size | Description       | Data Type        |
| ------ | ---- | ----------------- | ---------------- |
| 0      | 4    | First index       | Unsigned integer |
| 4      | 4    | Number of indices | Unsigned integer |
| 8      | 4    | Material index    | Unsigned integer |
| ...    | ...  | (repeat)          | ...              |

(The field above is repeated for each mesh in the file.)

Meshes consist of a range of indices in the [index section](#index-section). The material indices index into the [material section](#material-section).


## Material Section

| Offset | Size | Description              | Data Type        |
| ------ | ---- | ------------------------ | ---------------- |
| 0      | 4    | Base color texture index | Signed integer   |
| 4      | 4    | Normal texture index     | Signed integer   |
| 8      | 4    | ORM texture index        | Signed integer   |
| ...    | ...  | (repeat)                 | ...              |

(The fields above are repeated for each material in the file.)

The texture indices index into the [material section](#material-section). An index of 255 indicates that the respective texture is not defined. In that case, your application should fall back to an appropriate default texture. (For example a solid 127/127/255 texture to use when a mesh in the model does not come with an actual normal map.) ORM stands for Occlusion, Roughness and Metalness: A texture image where red represents occlusion, green represents roughness and the blue channel represents metalness.


## Bone Section

| Offset | Size | Description         | Data Type      |
| ------ | ---- | ------------------- | -------------- |
| 0      | 64   | Inverse bind matrix | 4x4 Matrix     |
| 64     | 4    | Parent bone index   | Signed integer |
| 68     | 12   | Padding             | None           |
| ...    | ...  | (repeat)            | ...            |

(The fields above are repeated for each bone in the file.)

The parent bone indices index into this same bone section, and is -1 for root bones without parent.


## Animation Section

| Offset | Size | Description                  | Data Type        |
| ------ | ---- | ---------------------------- | ---------------- |
| 0      | 128  | Name                         | String           |
| 128    | 4    | Duration                     | Float            |
| 132    | 4    | Sequence index               | Unsigned integer |
| ...    | ...  | (repeat)                     | ...              |

(The fields above are repeated for each animation in the file.)

The animation name is supplied as a null-terminated string of at most 128 characters (including the null terminator). The duration is defined in seconds. The sequence indices index into the [sequence section](#sequence-section) and describe where the sequence for the first bone in the [bone section](#bone-section) for this animation is located. The sequences for the other bones in the model are then found following the first one.


## Sequence Section

| Offset | Size | Description                   | Data Type        |
| ------ | ---- | ----------------------------- | ---------------- |
| 0      | 4    | First position keyframe index | Unsigned integer |
| 4      | 4    | Number of position keyframes  | Unsigned integer |
| 8      | 4    | First rotation keyframe index | Unsigned integer |
| 12     | 4    | Number of rotation keyframes  | Unsigned integer |
| 16     | 4    | First scale keyframe index    | Unsigned integer |
| 20     | 4    | Number of scale keyframes     | Unsigned integer |
| ...    | ...  | (repeat)                      | ...              |

(The fields above are repeated for each sequence in the file.)

Sequences index into the [keyframe section](#keyframe-section).


## Keyframe Section

| Offset | Size | Description | Data Type |
| ------ | ---- | ----------- | --------- |
| 0      | 4    | Time        | Float     |
| 4      | 4    | X           | Float     |
| 8      | 4    | Y           | Float     |
| 12     | 4    | Z           | Float     |
| 16     | 4    | W           | Float     |
| ...    | ...  | (repeat)    | ...       |

(The fields above repeated for each keyframe in the file.)

The time of the keyframe is defined in seconds. Keyframes are generic, they can represent position, rotation or scale keyframes, depending on how the sequence using the keyframe is indexing it. For position and scale keyframes, the last component W is 0. For rotation keyframes, the X, Y, Z, W values define a quaternion that expresses the rotation of the keyframe.