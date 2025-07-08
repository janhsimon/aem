#pragma once

// Compress textures for faster load times, but takes significantly longer to convert
#define COMPRESS_TEXTURES

// Skip optional steps to improve performance
// #define SKIP_INPUT_VALIDATION // Provided by cgltf

// Print model information to the console for debugging purposes
#define PRINT_HEADER
#define PRINT_VERTEX_BUFFER
// #define PRINT_INDEX_BUFFER
// #define PRINT_IMAGE_BUFFER
// #define PRINT_TEXTURES
#define PRINT_MESHES
// #define PRINT_MATERIALS
// #define PRINT_NODES // GLB nodes from the input file
// #define PRINT_JOINTS
// #define PRINT_ANIMATIONS
// #define PRINT_TRACKS
// #define PRINT_KEYFRAMES

// Dump files to the disk for debugging purposes
// #define DUMP_TEXTURES

// Limit output that tends to spam, set to 0 to disable limit
#define PRINT_VERTEX_BUFFER_COUNT 50
#define PRINT_TRACK_COUNT 500
#define PRINT_KEYFRAME_COUNT 500
#define DUMP_TEXTURE_COUNT 0
#define DUMP_TEXTURE_LEVEL_COUNT 1 // How many levels of each texture are dumped