#pragma once

// Skip optional steps to improve performance
// #define SKIP_INPUT_VALIDATION

// Print optional model information to the console for debugging purposes
#define PRINT_HEADER
//#define PRINT_VERTEX_BUFFER
// #define PRINT_INDEX_BUFFER
// #define PRINT_IMAGE_BUFFER
// #define PRINT_LEVELS
// #define PRINT_TEXTURES
// #define PRINT_MESHES
// #define PRINT_MATERIALS
#define PRINT_NODES // GLB nodes from the input file
#define PRINT_JOINTS
#define PRINT_ANIMATIONS
// #define PRINT_SEQUENCES
// #define PRINT_TRANSLATION_KEYFRAMES
// #define PRINT_ROTATION_KEYFRAMES
// #define PRINT_SCALE_KEYFRAMES

// Dump optional files to the disk for debugging purposes
// #define DUMP_TEXTURES // Dump all levels of all textures for visual inspection