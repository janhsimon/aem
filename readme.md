# What is *AEM*?

*AEM* is ***a**n **e**xcellent **m**odel* file format. :wink:

It describes game-ready 3D models with everything they need - no more, no less:
- Vertex normals, tangents and bitangents for lighting and normal mapping
- Single-channel UV coordinates for texture mapping
- Separate index data to use with an index buffer
- Standard PBR material system with base color, normal, occlusion, roughness and metalness maps
- Skeletal animations

This repository contains:
- `libaem`: A minimal and dependency-free C library that can load and animate *AEM* models efficiently
- `converter`: A command-line tool that can convert GLB files into *AEM*s
- `viewer`: A viewer application that illustrates how to load and render *AEM* models with `libaem` and OpenGL 3.3 and can be used to inspect *AEM* models


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
| 24     | 4    | Number of joints     | Unsigned integer |
| 28     | 4    | Number of animations | Unsigned integer |
| 32     | 4    | Number of tracks     | Unsigned integer |
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
| 56     | 4    | Joint index 1    | Signed integer |
| 60     | 4    | Joint index 2    | Signed integer |
| 64     | 4    | Joint index 3    | Signed integer |
| 68     | 4    | Joint index 4    | Signed integer |
| 72     | 4    | Joint weight 1   | Float          |
| 76     | 4    | Joint weight 2   | Float          |
| 80     | 4    | Joint weight 3   | Float          |
| 84     | 4    | Joint weight 4   | Float          |
| ...    | ...  | (repeat)         | ...            |

(The fields above are repeated for each vertex in the file.)

The joint indices 1-4 index into the [joint section](#joint-section). The sum of all joint weights is 1.0 if the vertex is affected by at least 1 joint. Otherwise the sum of joint weights is 0.0. If a vertex is affected by less than 4 joints, the additional joint weights are 0.0, and the corresponding joint indices are -1.


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


## Joint Section

| Offset | Size | Description          | Data Type      |
| ------ | ---- | -------------------- | -------------- |
| 0      | 128  | Name                 | String         |
| 128    | 64   | Inverse bind matrix  | 4x4 matrix     |
| 192    | 4    | Parent joint index   | Signed integer |
| ...    | ...  | (repeat)             | ...            |

(The fields above are repeated for each joint in the file.)

The joint name is supplied with a null-terminated string of at most 128 characters (including the null terminator). Longer names are not supported. The parent joint indices index into this same joint section, and is -1 for root joints without parent.


## Animation Section

| Offset | Size | Description                  | Data Type        |
| ------ | ---- | ---------------------------- | ---------------- |
| 0      | 128  | Name                         | String           |
| 128    | 4    | Duration                     | Float            |
| ...    | ...  | (repeat)                     | ...              |

(The fields above are repeated for each animation in the file.)

The animation name is supplied as a null-terminated string of at most 128 characters (including the null terminator). Longer names are not supported. The duration is defined in seconds.


## Track Section

| Offset | Size | Description                      | Data Type        |
| ------ | ---- | -------------------------------- | ---------------- |
| 0      | 4    | First keyframe index             | Unsigned integer |
| 4      | 4    | Number of translation keyframes  | Unsigned integer |
| 8      | 4    | Number of rotation keyframes     | Unsigned integer |
| 12     | 4    | Number of scale keyframes        | Unsigned integer |
| ...    | ...  | (repeat)                         | ...              |

(The fields above are repeated for each track in the file.)

Tracks index into the [keyframe section](#keyframe-section). There is a track for each joint in each animation and the keyframes are always in the order: Translation keyframes, then rotation keyframes, then scale keyframes. The layout is as follows: Track for joint 1/2 in animation 1/2, track for joint 2/2 in animation 1/2, track for joint 1/2 in animation 2/2, track for joint 2/2 in animation 2/2 and so on.


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

The time of the keyframe is defined in seconds. Keyframes are generic, they can represent position, rotation or scale keyframes, depending on how the track using the keyframe is indexing it. For position and scale keyframes, the last component W is 0. For rotation keyframes, the X, Y, Z, W values define a quaternion that expresses the rotation of the keyframe.