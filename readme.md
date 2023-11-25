# What is *AEM*?

*AEM* is ***a**n **e**xcellent **m**odel* file format. :wink:

It describes game-ready 3D models with everything they need - no more, no less:
- Vertex normals, tangents and bitangents for lighting and normal mapping
- Single-channel UV coordinates for texture mapping
- Separate index data to use with an index buffer
- Standard PBR material system with base color, normal, occlusion, roughness and metalness maps
- Mesh and skeletal animations

This repository contains a simple export tool that can convert virtually any 3D model into an AEM file. It also comes with a simple viewer application that shows how to load and render *AEM* models with OpenGL. Both applications are written in C.


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
| 12     | 4    | Number of meshes     | Unsigned integer |
| 16     | 4    | Number of materials  | Unsigned integer |
| 20     | 4    | Number of textures   | Unsigned integer |
| 24     | 4    | Number of bones      | Unsigned integer |
| 28     | 4    | Number of animations | Unsigned integer |

The magic number is always "AEM" in ASCII (`0x41 45 4D`). This specification describes version 1 of the file format.


## Vertex Section

| Offset | Size | Description      | Data Type        |
| ------ | ---- | ---------------- | ---------------- |
| 0      | 4    | Position X       | Float            |
| 4      | 4    | Position Y       | Float            |
| 8      | 4    | Position Z       | Float            |
| 12     | 4    | Normal X         | Float            |
| 16     | 4    | Normal Y         | Float            |
| 20     | 4    | Normal Z         | Float            |
| 24     | 4    | Tangent X        | Float            |
| 28     | 4    | Tangent Y        | Float            |
| 32     | 4    | Tangent Z        | Float            |
| 36     | 4    | Bitangent X      | Float            |
| 40     | 4    | Bitangent Y      | Float            |
| 44     | 4    | Bitangent Z      | Float            |
| 48     | 4    | Texture U        | Float            |
| 52     | 4    | Texture V        | Float            |
| 56     | 4    | Bone index 1     | Unsigned integer |
| 60     | 4    | Bone index 2     | Unsigned integer |
| 64     | 4    | Bone index 3     | Unsigned integer |
| 68     | 4    | Bone index 4     | Unsigned integer |
| 72     | 4    | Bone weight 1    | Float            |
| 76     | 4    | Bone weight 2    | Float            |
| 80     | 4    | Bone weight 3    | Float            |
| 84     | 4    | Bone weight 4    | Float            |
| 88     | 4    | Extra bone index | Unsigned integer |
| ...    | ...  | ...              | ...              |

(The fields above are repeated for each vertex in the file.)

The bone indices 1-4 and extra bone indices index into the [bone section](#bone-section). The sum of all bone weights is always 1. If a vertex is affected by less than 4 bones, the additional bone weights are 0.0, and the corresponding bone indices are undefined. The bone indices 1-4 express skeletal animation, while the extra bone index is always applied with 100% weight to all vertices in a mesh, and expresses mesh animation.


## Index Section

| Offset | Size | Description | Data Type        |
| ------ | ---- | ----------- | ---------------- |
| 0      | 4    | Index       | Unsigned integer |
| ...    | ...  | ...         | ...              |

(The field above is repeated for each index in the file.)

The indices index into the [vertex section](#vertex-section).


## Mesh Section

| Offset | Size | Description         | Data Type        |
| ------ | ---- | ------------------- | ---------------- |
| 0      | 4    | Number of indices   | Unsigned integer |
| 4      | 1    | Material index      | Unsigned integer |
| ...    | ...  | ...                 | ...              |

(The field above is repeated for each mesh in the file.)

Meshes consist of sequential indices in the [index section](#index-section), and are themselves sequential. The material indices index into the [material section](#material-section).


## Material Section

| Offset | Size | Description              | Data Type        |
| ------ | ---- | ------------------------ | ---------------- |
| 0      | 1    | Base color texture index | Unsigned integer |
| 1      | 1    | Normal texture index     | Unsigned integer |
| 2      | 1    | ORM texture index        | Unsigned integer |
| ...    | ...  | ...                      | ...              |

(The fields above are repeated for each material in the file.)

The texture indices index into the [material section](#material-section). An index of 255 indicates that the respective texture is not defined. In that case, your application should fall back to an appropriate default texture. (For example a solid 127/127/255 texture to use when a mesh in the model does not come with an actual normal map.) ORM stands for Occlusion, Roughness and Metalness: A texture image where red represents occlusion, green represents roughness and the blue channel represents metalness.


## Texture Section

| Offset | Size | Description | Data Type |
| ------ | ---- | ----------- | --------- |
| 0      | 128  | Filename    | String    |
| ...    | ...  | ...         | ...       |

(The field above is repeated for each texture in the file.)

The filename for a texture is supplied with a null-terminated string of at most 128 characters (including the null terminator). Longer filenames are not supported.


## Bone Section

| Offset | Size | Description         | Data Type      |
| ------ | ---- | ------------------- | -------------- |
| 0      | 64   | Inverse bind matrix | 4x4 Matrix     |
| 64     | 4    | Parent bone index   | Signed integer |
| 68     | 12   | Padding             | None           |
| ...    | ...  | ...                 | ...            |

(The fields above are repeated for each bone in the file.)

The parent bone indices index into this same bone section, and is -1 for root bones without parent.


## Animation Section

| Offset | Size | Description                  | Data Type        |
| ------ | ---- | ---------------------------- | ---------------- |
| 0      | 128  | Name                         | String           |
| 128    | 4    | Duration                     | Float            |
| 132    | 4    | Number of position keyframes | Unsigned integer |
| 136    | 4    | Number of rotation keyframes | Unsigned integer |
| 140    | 4    | Number of scale keyframes    | Unsigned integer |
| ...    | ...  | ...                          | ...              |

(The fields above are repeated for each animation in the file. Additionally, the keyframe fields above are repeated for each bone in the file.)

The animation name is supplied as a null-terminated string of at most 128 characters (including the null terminator). The duration is defined in seconds. Animations consist of sequential indices in the keyframe sections ([position keyframe section](#position-keyframe-section), [rotation keyframe section](#rotation-keyframe-section), and [scale keyframe section](#scale-keyframe-section)), and are themselves sequential.


## Position Keyframe Section

| Offset | Size | Description                  | Data Type |
| ------ | ---- | ---------------------------- | --------- |
| 0      | 4    | Time                         | Float     |
| 4      | 4    | Position X                   | Float     |
| 8      | 4    | Position Y                   | Float     |
| 12     | 4    | Position Z                   | Float     |
| 16     | 4    | Padding                      | None      |

The time of the keyframe is defined in seconds.


## Rotation Keyframe Section

| Offset | Size | Description | Data Type |
| ------ | ---- | ----------- | --------- |
| 0      | 4    | Time        | Float     |
| 4      | 4    | Rotation X  | Float     |
| 8      | 4    | Rotation Y  | Float     |
| 12     | 4    | Rotation Z  | Float     |
| 16     | 4    | Rotation W  | Float     |

The time of the keyframe is defined in seconds. The Rotation X, Y, Z, W values define a quaternion that expresses the rotation of the keyframe.


## Scale Keyframe Section

| Offset | Size | Description | Data Type |
| ------ | ---- | ----------- | --------- |
| 0      | 4    | Time        | Float     |
| 4      | 4    | Scale X     | Float     |
| 8      | 4    | Scale Y     | Float     |
| 12     | 4    | Scale Z     | Float     |
| 16     | 4    | Padding     | None      |

The time of the keyframe is defined in seconds.