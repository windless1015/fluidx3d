#pragma once

#include "../core/defines.hpp"
#include "../core/opencl.hpp"
#include "../render/graphics.hpp"
#include "../app/info.hpp"
#include "lbm.hpp"
#include <atomic>
#include <memory>
#include <vector>

class LBM_Domain_Graphics {
private:
	Kernel kernel_clear; // reset bitmap and zbuffer
	Memory<int> bitmap; // bitmap for rendering
	Memory<int> zbuffer; // z-buffer for rendering
	Memory<float> camera_parameters; // contains camera position, rotation, field of view etc.

	LBM_Domain* lbm = nullptr;
	Kernel kernel_graphics_flags; // render flag lattice with wireframe
	Kernel kernel_graphics_flags_mc; // render flag lattice with marching-cubes
	Kernel kernel_graphics_field; // render a colored velocity vector for each cell
	Kernel kernel_graphics_field_slice; // render one slice of velocity field according to slics settings
	Kernel kernel_graphics_streamline; // render streamlines
	Kernel kernel_graphics_q; // render vorticity (Q-criterion)

#ifdef SURFACE
	const string path_skybox = get_exe_path()+"../skybox/skybox8k.png";
	Image* skybox_image = nullptr;
	Memory<int> skybox; // skybox for free surface raytracing
	Kernel kernel_graphics_rasterize_phi; // rasterize free surface
	Kernel kernel_graphics_raytrace_phi; // raytrace free surface
	Image* get_skybox_image() const { return skybox_image; }
#endif // SURFACE

	ulong t_last_rendered_frame = max_ulong; // optimization to not call draw_frame() multiple times if camera_parameters and LBM time step are unchanged
	bool update_camera(); // update camera_parameters and return if they are changed from their previous state

public:
	LBM_Domain_Graphics() {} // default constructor
	explicit LBM_Domain_Graphics(LBM_Domain* lbm) {
		this->lbm = lbm;
#ifdef SURFACE
		skybox_image = read_png(path_skybox);
#endif // SURFACE
	}
	LBM_Domain_Graphics& operator=(const LBM_Domain_Graphics& graphics) { // copy assignment
		lbm = graphics.lbm;
#ifdef SURFACE
		skybox_image = graphics.get_skybox_image();
#endif // SURFACE
		return *this;
	}
	void allocate(Device& device); // allocate memory for bitmap and zbuffer
	bool enqueue_draw_frame(const int visualization_modes, const int field_mode=0, const int slice_mode=0, const int slice_x=0, const int slice_y=0, const int slice_z=0, const bool visualization_change=true); // main rendering function, calls rendering kernels, returns true if new frame is rendered, false if old frame is returned when camera has not moved
	int* get_bitmap(); // returns pointer to bitmap
	int* get_zbuffer(); // returns pointer to zbuffer
}; // LBM_Domain_Graphics

string lbm_graphics_device_defines(const LBM_Domain* lbm_domain); // returns preprocessor constants for embedding in OpenCL C code

class LBM_Graphics {
private:
	LBM* lbm = nullptr;
	std::vector<std::unique_ptr<LBM_Domain_Graphics>> domain_graphics;
	std::atomic_int running_encoders = 0;
	uint last_exported_frame = 0u; // for next_frame(...) function
	int last_visualization_modes=0, last_field_mode=0, last_slice_mode=0, last_slice_x=0, last_slice_y=0, last_slice_z=0; // don't render a new frame if the scene hasn't changed since last frame
	void default_settings() {
		visualization_modes |= VIS_FLAG_LATTICE;
	}

public:
	int visualization_modes=0, field_mode=0, slice_mode=0, slice_x=0, slice_y=0, slice_z=0; // field_mode = { 0 (u), 1 (rho) }, slice_mode = { 0 (no slice), 1 (x), 2 (y), 3 (z), 4 (xz), 5 (xyz), 6 (yz), 7 (xy) }, slice_{xyz} = position of slices

	LBM_Graphics() {} // default constructor
	explicit LBM_Graphics(LBM* lbm);
	~LBM_Graphics(); // destructor must wait for all encoder threads to finish
	LBM_Graphics& operator=(const LBM_Graphics& graphics); // copy assignment

	int* draw_frame(); // main rendering function, calls rendering kernels

	void set_camera_centered(const float rx=0.0f, const float ry=0.0f, const float fov=100.0f, const float zoom=1.0f); // set camera centered
	void set_camera_free(const float3& p=float3(0.0f), const float rx=0.0f, const float ry=0.0f, const float fov=100.0f); // set camera free
	bool next_frame(const ulong total_time_steps, const float video_length_seconds); // returns true once simulation time has progressed enough to render the next video frame for a 60fps video of specified length
	void print_frame(); // preview preview of current frame in console
	void write_frame(const string& path="", const string& name="image", const string& extension=".png", bool print_preview=false); // save current frame
	void write_frame(const uint x1, const uint y1, const uint x2, const uint y2, const string& path="", const string& name="image", const string& extension=".png", bool print_preview=false); // save current frame cropped with two corner points (x1,y1) and (x2,y2)
	void write_frame_png(const string& path="", bool print_preview=false); // save current frame as .png file (smallest file size, but slow)
	void write_frame_qoi(const string& path="", bool print_preview=false); // save current frame as .qoi file (small file size, fast)
	void write_frame_bmp(const string& path="", bool print_preview=false); // save current frame as .bmp file (large file size, fast)
	void write_frame_png(const uint x1, const uint y1, const uint x2, const uint y2, const string& path="", bool print_preview=false); // save current frame as .png file (smallest file size, but slow)
	void write_frame_qoi(const uint x1, const uint y1, const uint x2, const uint y2, const string& path="", bool print_preview=false); // save current frame as .qoi file (small file size, fast)
	void write_frame_bmp(const uint x1, const uint y1, const uint x2, const uint y2, const string& path="", bool print_preview=false); // save current frame as .bmp file (large file size, fast)
}; // LBM_Graphics
