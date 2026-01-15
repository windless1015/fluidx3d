/**
 * FluidX3D - Configuration Defines
 * 
 * Minimal configuration for Dam Break 3D with LBM + Free Surface
 */

#pragma once

// ============================================================================
// LBM Velocity Set
// ============================================================================
#define D3Q19  // 3D velocity set with 19 velocities

// ============================================================================
// LBM Collision Operator
// ============================================================================
#define SRT  // Single-relaxation-time collision operator

// ============================================================================
// Memory Compression
// ============================================================================
#define FP16S  // Range-shifted IEEE-754 FP16 for 2x speedup and 2x VRAM reduction

// ============================================================================
// Physical Extensions
// ============================================================================
#define VOLUME_FORCE  // Enable global volume force (gravity)
#define SURFACE       // Enable free surface LBM

// ============================================================================
// Graphics
// ============================================================================
#define INTERACTIVE_GRAPHICS_GLFW  // Enable interactive graphics window (GLFW + GLAD)

// ============================================================================
// Output
// ============================================================================
#define ENABLE_VTK_OUTPUT 0  // 1 = write VTK files, 0 = disable VTK output

#define GRAPHICS_FRAME_WIDTH 1920
#define GRAPHICS_FRAME_HEIGHT 1080
#define GRAPHICS_BACKGROUND_COLOR 0x000000
#define GRAPHICS_U_MAX 0.18f
#define GRAPHICS_RHO_DELTA 0.001f

#define GRAPHICS_Q_CRITERION 0.0001f
#define GRAPHICS_STREAMLINE_SPARSE 8
#define GRAPHICS_STREAMLINE_LENGTH 128
#define GRAPHICS_RAYTRACING_TRANSMITTANCE 0.25f
#define GRAPHICS_RAYTRACING_COLOR 0x005F7F

// ============================================================================
// Cell Type Flags
// ============================================================================
#define TYPE_S 0b00000001  // Solid boundary
#define TYPE_F 0b00001000  // Fluid
#define TYPE_I 0b00010000  // Interface
#define TYPE_G 0b00100000  // Gas
#define TYPE_X 0b01000000  // Reserved
#define TYPE_Y 0b10000000  // Reserved

// ============================================================================
// Visualization Modes
// ============================================================================
#define VIS_FLAG_LATTICE  0b00000001
#define VIS_FLAG_SURFACE  0b00000010
#define VIS_FIELD         0b00000100
#define VIS_STREAMLINES   0b00001000
#define VIS_Q_CRITERION   0b00010000
#define VIS_PHI_RASTERIZE 0b00100000
#define VIS_PHI_RAYTRACE  0b01000000

// ============================================================================
// Internal Definitions (Do not modify)
// ============================================================================
#define fpxx ushort

#ifdef SURFACE
#define UPDATE_FIELDS
#endif

#if defined(INTERACTIVE_GRAPHICS) || defined(INTERACTIVE_GRAPHICS_ASCII) || defined(INTERACTIVE_GRAPHICS_GLFW)
#define GRAPHICS
#define UPDATE_FIELDS
#endif
