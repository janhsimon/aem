#pragma once

// Dump model information to the console for debugging purposes
#define DUMP_HEADER
// #define DUMP_NODES DOES NOT EXIST LONGER
// #define DUMP_MESHES
// #define DUMP_MATERIALS
// #define DUMP_MATERIAL_PROPERTIES
#define DUMP_TEXTURES
// #define DUMP_BONES
// #define DUMP_ANIMATIONS
// #define DUMP_SEQUENCES

// Skip the texture export to greatly improve export performance
// #define SKIP_TEXTURE_EXPORT

// Export the individual mip level images of textures as PNG files for debugging purposes
//#define EXPORT_TEXTURE_MIP_LEVELS

#define MAX_BONE_WEIGHT_COUNT 4 // Maximum number of bone weights that can influence a single vertex