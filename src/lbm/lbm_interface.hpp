#pragma once

#include "../core/defines.hpp"
#include "../core/opencl.hpp"
#include "../render/graphics.hpp"
#include "../core/units.hpp"
#include "../app/info.hpp"

uint bytes_per_cell_host(); // returns the number of Bytes per cell allocated in host memory
uint bytes_per_cell_device(); // returns the number of Bytes per cell allocated in device memory
uint bandwidth_bytes_per_cell_device(); // returns the bandwidth in Bytes per cell per time step from/to device memory
uint3 resolution(const float3 box_aspect_ratio, const uint memory); // input: simulation box aspect ratio and VRAM occupation in MB, output: grid resolution

string default_filename(const string& path, const string& name, const string& extension, const ulong t); // generate a default filename with timestamp
string default_filename(const string& name, const string& extension, const ulong t); // generate a default filename with timestamp at exe_path/export/
