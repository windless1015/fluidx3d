#include "lbm_graphics.hpp"
#include <thread>

#ifdef GRAPHICS
void LBM_Domain_Graphics::allocate(Device& device) {
	bitmap = Memory<int>(device, camera.width*camera.height);
	zbuffer = Memory<int>(device, camera.width*camera.height, 1u, lbm->get_D()>1u); // if there are multiple domains, allocate zbuffer also on host side
	camera_parameters = Memory<float>(device, 15u);
	kernel_clear = Kernel(device, bitmap.length(), "graphics_clear", bitmap, zbuffer);

	kernel_graphics_flags = Kernel(device, lbm->get_N(), "graphics_flags", camera_parameters, bitmap, zbuffer, lbm->flags);
	kernel_graphics_flags_mc = Kernel(device, lbm->get_N(), "graphics_flags_mc", camera_parameters, bitmap, zbuffer, lbm->flags);
	kernel_graphics_field = Kernel(device, lbm->get_D()==1u ? camera.width*camera.height : lbm->get_N(), lbm->get_D()==1u ? "graphics_field_rt" : "graphics_field", camera_parameters, bitmap, zbuffer, 0, lbm->rho, lbm->u, lbm->flags); // raytraced field visualization only works for single-GPU
	kernel_graphics_field_slice = Kernel(device, lbm->get_N(), "graphics_field_slice", camera_parameters, bitmap, zbuffer, 0, 0, 0, 0, 0, lbm->rho, lbm->u, lbm->flags);
	kernel_graphics_streamline = Kernel(device, (lbm->get_Nx()/GRAPHICS_STREAMLINE_SPARSE)*(lbm->get_Ny()/GRAPHICS_STREAMLINE_SPARSE)*(lbm->get_Nz()/GRAPHICS_STREAMLINE_SPARSE), "graphics_streamline", camera_parameters, bitmap, zbuffer, 0, 0, 0, 0, 0, lbm->rho, lbm->u, lbm->flags); // 3D
	kernel_graphics_q = Kernel(device, lbm->get_N(), "graphics_q", camera_parameters, bitmap, zbuffer, 0, lbm->rho, lbm->u);

#ifdef SURFACE
	skybox = Memory<int>(device, skybox_image->width()*skybox_image->height(), 1u, skybox_image->data());
	kernel_graphics_rasterize_phi = Kernel(device, lbm->get_N(), "graphics_rasterize_phi", camera_parameters, bitmap, zbuffer, lbm->phi);
	kernel_graphics_raytrace_phi = Kernel(device, bitmap.length(), "graphics_raytrace_phi", camera_parameters, bitmap, skybox, lbm->phi, lbm->flags);
	kernel_graphics_q.add_parameters(lbm->flags);
#endif // SURFACE
}

bool LBM_Domain_Graphics::update_camera() {
	camera.update_matrix();
	bool change = false;
	for(uint i=0u; i<15u; i++) {
		const float data = camera.data(i);
		change |= (camera_parameters[i]!=data);
		camera_parameters[i] = data;
	}
	return change; // return false if camera parameters remain unchanged
}
bool LBM_Domain_Graphics::enqueue_draw_frame(const int visualization_modes, const int field_mode, const int slice_mode, const int slice_x, const int slice_y, const int slice_z, const bool visualization_change) {
	const bool camera_update = update_camera();
#if defined(INTERACTIVE_GRAPHICS)||defined(INTERACTIVE_GRAPHICS_ASCII)
	if(!visualization_change&&!camera_update&&lbm->get_t()==t_last_rendered_frame) return false; // don't render a new frame if the scene hasn't changed since last frame
#endif // INTERACTIVE_GRAPHICS||INTERACTIVE_GRAPHICS_ASCII
	t_last_rendered_frame = lbm->get_t();
	if(camera_update) camera_parameters.enqueue_write_to_device(); // camera_parameters PCIe transfer and kernel_clear execution can happen simulataneously
	kernel_clear.enqueue_run();
	const int sx=slice_x-lbm->get_Ox(), sy=slice_y-lbm->get_Oy(), sz=slice_z-lbm->get_Oz(); // subtract domain offsets
#ifdef SURFACE
	if((visualization_modes&VIS_PHI_RAYTRACE)&&lbm->get_D()==1u) kernel_graphics_raytrace_phi.enqueue_run(); // disable raytracing for multi-GPU (domain decomposition rendering doesn't work for raytracing)
	if(visualization_modes&VIS_PHI_RASTERIZE) kernel_graphics_rasterize_phi.enqueue_run();
#endif // SURFACE
	if(visualization_modes&VIS_FLAG_LATTICE) kernel_graphics_flags.enqueue_run();
	if(visualization_modes&VIS_FLAG_SURFACE) kernel_graphics_flags_mc.enqueue_run();
	if(visualization_modes&VIS_STREAMLINES) kernel_graphics_streamline.set_parameters(3u, field_mode, slice_mode, sx, sy, sz).enqueue_run();
	if(visualization_modes&VIS_Q_CRITERION) kernel_graphics_q.set_parameters(3u, field_mode).enqueue_run();
	if(visualization_modes&VIS_FIELD) {
		switch(slice_mode) { // 0 (no slice), 1 (x), 2 (y), 3 (z), 4 (xz), 5 (xyz), 6 (yz), 7 (xy)
			case 0: // no slice
				kernel_graphics_field.set_parameters(3u, field_mode).enqueue_run();
				break;
			case 1: case 2: case 3: // x/y/z
				kernel_graphics_field_slice.set_ranges(lbm->get_area((uint)clamp(slice_mode-1, 0, 2))).set_parameters(3u, field_mode, slice_mode, sx, sy, sz).enqueue_run();
				break;
			case 4: // xz
				kernel_graphics_field_slice.set_ranges(lbm->get_area(0u)).set_parameters(3u, field_mode, 0u+1u, sx, sy, sz).enqueue_run();
				kernel_graphics_field_slice.set_ranges(lbm->get_area(2u)).set_parameters(3u, field_mode, 2u+1u, sx, sy, sz).enqueue_run();
				break;
			case 5: // xyz
				kernel_graphics_field_slice.set_ranges(lbm->get_area(0u)).set_parameters(3u, field_mode, 0u+1u, sx, sy, sz).enqueue_run();
				kernel_graphics_field_slice.set_ranges(lbm->get_area(1u)).set_parameters(3u, field_mode, 1u+1u, sx, sy, sz).enqueue_run();
				kernel_graphics_field_slice.set_ranges(lbm->get_area(2u)).set_parameters(3u, field_mode, 2u+1u, sx, sy, sz).enqueue_run();
				break;
			case 6: // yz
				kernel_graphics_field_slice.set_ranges(lbm->get_area(1u)).set_parameters(3u, field_mode, 1u+1u, sx, sy, sz).enqueue_run();
				kernel_graphics_field_slice.set_ranges(lbm->get_area(2u)).set_parameters(3u, field_mode, 2u+1u, sx, sy, sz).enqueue_run();
				break;
			case 7: // xy
				kernel_graphics_field_slice.set_ranges(lbm->get_area(0u)).set_parameters(3u, field_mode, 0u+1u, sx, sy, sz).enqueue_run();
				kernel_graphics_field_slice.set_ranges(lbm->get_area(1u)).set_parameters(3u, field_mode, 1u+1u, sx, sy, sz).enqueue_run();
				break;
		}
	}
	bitmap.enqueue_read_from_device();
	if(lbm->get_D()>1u) zbuffer.enqueue_read_from_device();
	return true; // new frame has been rendered
}
int* LBM_Domain_Graphics::get_bitmap() { // returns pointer to zbuffer
	return bitmap.data();
}
int* LBM_Domain_Graphics::get_zbuffer() { // returns pointer to zbuffer
	return zbuffer.data();
}

string lbm_graphics_device_defines(const LBM_Domain* lbm_domain) {
	string defines =
		"\n	#define GRAPHICS"
		"\n	#define def_background_color " +to_string(GRAPHICS_BACKGROUND_COLOR)+""
		"\n	#define def_screen_width "     +to_string(camera.width)+"u"
		"\n	#define def_screen_height "    +to_string(camera.height)+"u"
		"\n	#define def_scale_u "          +to_string(1.0f/(0.57735027f*(GRAPHICS_U_MAX)))+"f"
		"\n	#define def_scale_rho "        +to_string(0.5f/(GRAPHICS_RHO_DELTA))+"f"
		"\n	#define def_scale_Q_min "      +to_string(GRAPHICS_Q_CRITERION)+"f"
		"\n	#define def_streamline_sparse "+to_string(GRAPHICS_STREAMLINE_SPARSE)+"u"
		"\n	#define def_streamline_length "+to_string(GRAPHICS_STREAMLINE_LENGTH)+"u"
		"\n	#define def_n "                +to_string(1.333f)+"f" // refractive index of water for raytracing graphics
		"\n	#define def_attenuation "      +to_string(ln(clamp(GRAPHICS_RAYTRACING_TRANSMITTANCE, 1E-9f, 1.0f))/(float)max(max(lbm_domain->get_Nx(), lbm_domain->get_Ny()), lbm_domain->get_Nz()))+"f" // (negative) attenuation parameter for raytracing graphics
		"\n	#define def_absorption_color " +to_string(GRAPHICS_RAYTRACING_COLOR)+"" // absorption color of fluid for raytracing graphics
		"\n	#define COLOR_S (127<<16|127<<8|127)" // (stationary or moving) solid boundary
		"\n	#define COLOR_F (  0<<16|  0<<8|255)" // fluid
		"\n	#define COLOR_I (  0<<16|255<<8|255)" // interface
		"\n	#define COLOR_0 (127<<16|127<<8|127)" // regular cell or gas
		"\n	#define COLOR_X (255<<16|127<<8|  0)" // reserved type X
		"\n	#define COLOR_Y (255<<16|255<<8|  0)" // reserved type Y
	;

#ifdef GRAPHICS_TRANSPARENCY
	defines += "\n	#define GRAPHICS_TRANSPARENCY "+to_string(GRAPHICS_TRANSPARENCY)+"f";
#endif // GRAPHICS_TRANSPARENCY

#ifndef SURFACE
	defines += "\n	#define def_skybox_width 1u";
	defines += "\n	#define def_skybox_height 1u";
#else // SURFACE
	const string path_skybox = get_exe_path()+"../skybox/skybox8k.png";
	Image* skybox_image = read_png(path_skybox);
	if(skybox_image==nullptr) {
		print_info("Skybox image not found at \""+path_skybox+"\"; using 1x1 fallback.");
		defines += "\n	#define def_skybox_width 1u";
		defines += "\n	#define def_skybox_height 1u";
	} else {
		defines += "\n	#define def_skybox_width " +to_string(skybox_image->width() )+"u";
		defines += "\n	#define def_skybox_height "+to_string(skybox_image->height())+"u";
		delete skybox_image;
	}
#endif // SURFACE
	return defines+"\n";
}

LBM_Graphics::LBM_Graphics(LBM* lbm) {
	print_info("Initializing graphics.");
	this->lbm = lbm;
	camera.set_zoom(0.5f*(float)fmax(fmax(lbm->get_Nx(), lbm->get_Ny()), lbm->get_Nz()));
	slice_x = (int)lbm->get_Nx()/2;
	slice_y = (int)lbm->get_Ny()/2;
	slice_z = (int)lbm->get_Nz()/2;
	default_settings();
	domain_graphics.reserve(lbm->get_D());
	for(uint d=0u; d<lbm->get_D(); d++) {
		print_info("Allocating graphics for domain "+to_string(d)+".");
		domain_graphics.emplace_back(std::make_unique<LBM_Domain_Graphics>(lbm->lbm_domain[d]));
		domain_graphics.back()->allocate(lbm->lbm_domain[d]->get_device());
	}
	print_info("Graphics initialization complete.");
}
LBM_Graphics::~LBM_Graphics() { // destructor must wait for all encoder threads to finish
	int last_value = running_encoders.load();
	while(last_value>0) {
		const int current_value = running_encoders.load();
		if(last_value!=current_value) {
			print_info("Finishing encoder threads: "+to_string(current_value));
			last_value = current_value;
		}
		sleep(0.016);
	}
}
LBM_Graphics& LBM_Graphics::operator=(const LBM_Graphics& graphics) { // copy assignment
	lbm = graphics.lbm;
	visualization_modes = graphics.visualization_modes;
	field_mode = graphics.field_mode;
	slice_mode = graphics.slice_mode;
	slice_x = graphics.slice_x;
	slice_y = graphics.slice_y;
	slice_z = graphics.slice_z;
	return *this;
}

int* LBM_Graphics::draw_frame() {
#ifndef UPDATE_FIELDS
	if(visualization_modes&(VIS_FIELD|VIS_STREAMLINES|VIS_Q_CRITERION)) {
		for(uint d=0u; d<lbm->get_D(); d++) lbm->lbm_domain[d]->enqueue_update_fields(); // only call update_fields() if the time step has changed since the last rendered frame
	}
#endif // UPDATE_FIELDS
#ifdef USE_CUDA_LBM
	for(uint d=0u; d<lbm->get_D(); d++) lbm->lbm_domain[d]->sync_cuda_to_opencl_render();
#endif // USE_CUDA_LBM
	if(key_1) { visualization_modes = (visualization_modes&~0b11)|(((visualization_modes&0b11)+1)%4); key_1 = false; }
	if(key_2) { visualization_modes ^= VIS_FIELD        ; key_2 = false; }
	if(key_3) { visualization_modes ^= VIS_STREAMLINES  ; key_3 = false; }
	if(key_4) { visualization_modes ^= VIS_Q_CRITERION  ; key_4 = false; }
	if(key_5) { visualization_modes ^= VIS_PHI_RASTERIZE; key_5 = false; }
	if(key_6) { visualization_modes ^= VIS_PHI_RAYTRACE ; key_6 = false; }
	if(key_T) {
		slice_mode = (slice_mode+1)%8; key_T = false;
	}
	if(key_Z) {
		field_mode = (field_mode+1)%2; key_Z = false; // field_mode = { 0 (u), 1 (rho) }
	}
	if(slice_mode==1u) {
		if(key_Q) { slice_x = clamp(slice_x-1, 0, (int)lbm->get_Nx()-1); key_Q = false; }
		if(key_E) { slice_x = clamp(slice_x+1, 0, (int)lbm->get_Nx()-1); key_E = false; }
	}
	if(slice_mode==2u) {
		if(key_Q) { slice_y = clamp(slice_y-1, 0, (int)lbm->get_Ny()-1); key_Q = false; }
		if(key_E) { slice_y = clamp(slice_y+1, 0, (int)lbm->get_Ny()-1); key_E = false; }
	}
	if(slice_mode==3u) {
		if(key_Q) { slice_z = clamp(slice_z-1, 0, (int)lbm->get_Nz()-1); key_Q = false; }
		if(key_E) { slice_z = clamp(slice_z+1, 0, (int)lbm->get_Nz()-1); key_E = false; }
	}
	const bool visualization_change = camera.key_update||last_visualization_modes!=visualization_modes||last_field_mode!=field_mode||last_slice_mode!=slice_mode||last_slice_x!=slice_x||last_slice_y!=slice_y||last_slice_z!=slice_z;
	camera.key_update = false;
	last_visualization_modes = visualization_modes;
	last_field_mode = field_mode;
	last_slice_mode = slice_mode;
	last_slice_x = slice_x;
	last_slice_y = slice_y;
	last_slice_z = slice_z;
	bool new_frame = true;
	for(uint d=0u; d<lbm->get_D(); d++) new_frame = new_frame && domain_graphics[d]->enqueue_draw_frame(visualization_modes, field_mode, slice_mode, slice_x, slice_y, slice_z, visualization_change);
	for(uint d=0u; d<lbm->get_D(); d++) lbm->lbm_domain[d]->finish_queue();
	int* bitmap = domain_graphics[0]->get_bitmap();
	int* zbuffer = domain_graphics[0]->get_zbuffer();
	for(uint d=1u; d<lbm->get_D()&&new_frame; d++) {
		const int* bitmap_d = domain_graphics[d]->get_bitmap(); // each domain renders its own frame
		const int* zbuffer_d = domain_graphics[d]->get_zbuffer();
		for(uint i=0u; i<camera.width*camera.height; i++) {
#ifndef GRAPHICS_TRANSPARENCY
			const int zdi = zbuffer_d[i];
			if(zdi>zbuffer[i]) {
				bitmap[i] = bitmap_d[i]; // overlay frames using their z-buffers
				zbuffer[i] = zdi;
			}
#else // GRAPHICS_TRANSPARENCY
			bitmap[i] = color_add(bitmap[i], bitmap_d[i]);
#endif // GRAPHICS_TRANSPARENCY
		}
	}
	camera.allow_labeling = new_frame; // only print new label on frame if a new frame has been rendered
	return bitmap;
}

void LBM_Graphics::set_camera_centered(const float rx, const float ry, const float fov, const float zoom) {
	camera.free = false;
	camera.rx = 0.5*pi+((double)rx*pi/180.0);
	camera.ry = pi-((double)ry*pi/180.0);
	camera.fov = clamp((float)fov, 1E-6f, 179.0f);
	camera.set_zoom(0.5f*(float)fmax(fmax(lbm->get_Nx(), lbm->get_Ny()), lbm->get_Nz())/zoom);
}
void LBM_Graphics::set_camera_free(const float3& p, const float rx, const float ry, const float fov) {
	camera.free = true;
	camera.rx = 0.5*pi+((double)rx*pi/180.0);
	camera.ry = pi-((double)ry*pi/180.0);
	camera.fov = clamp((float)fov, 1E-6f, 179.0f);
	camera.zoom = 1E16f;
	camera.pos = p;
}
bool LBM_Graphics::next_frame(const ulong total_time_steps, const float video_length_seconds) { // returns true once simulation time has progressed enough to render the next video frame for a 60fps video of specified length
	const uint new_frame = to_uint((float)lbm->get_t()/(float)total_time_steps*video_length_seconds*60.0f);
	if(new_frame!=last_exported_frame) {
		last_exported_frame = new_frame;
		return true;
	} else {
		return false;
	}
}
void LBM_Graphics::print_frame() { // preview current frame in console
#ifndef INTERACTIVE_GRAPHICS_ASCII
	camera.rendring_frame.lock(); // block rendering for other threads until finished
	camera.key_update = true; // force rendering new frame
	int* image_data = draw_frame(); // make sure the frame is fully rendered
	Image* image = new Image(camera.width, camera.height, image_data);
	info.allow_printing.lock();
	println();
	print_image(image);
	info.allow_printing.unlock();
	delete image;
	camera.rendring_frame.unlock();
#endif // INTERACTIVE_GRAPHICS_ASCII
}
void encode_image(Image* image, const string& filename, const string& extension, std::atomic_int* running_encoders) {
	if(extension==".png") write_png(filename, image);
	if(extension==".qoi") write_qoi(filename, image);
	if(extension==".bmp") write_bmp(filename, image);
	delete image; // delete image when done
	(*running_encoders)--;
}
void LBM_Graphics::write_frame(const string& path, const string& name, const string& extension, bool print_preview) { // save current frame as .png file (smallest file size, but slow)
	write_frame(0u, 0u, camera.width, camera.height, path, name, extension, print_preview);
}
void LBM_Graphics::write_frame(const uint x1, const uint y1, const uint x2, const uint y2, const string& path, const string& name, const string& extension, bool print_preview) { // save a cropped current frame with two corner points (x1,y1) and (x2,y2)
	camera.rendring_frame.lock(); // block rendering for other threads until finished
	camera.key_update = true; // force rendering new frame
	int* image_data = draw_frame(); // make sure the frame is fully rendered
	const string filename = default_filename(path, name, extension, lbm->get_t());
	const uint xa=max(min(x1, x2), 0u), xb=min(max(x1, x2), camera.width ); // sort coordinates if necessary
	const uint ya=max(min(y1, y2), 0u), yb=min(max(y1, y2), camera.height);
	Image* image = new Image(xb-xa, yb-ya); // create local copy of frame buffer
	for(uint y=0u; y<image->height(); y++) for(uint x=0u; x<image->width(); x++) image->set_color(x, y, image_data[camera.width*(ya+y)+(xa+x)]);
#ifndef INTERACTIVE_GRAPHICS_ASCII
	if(print_preview) {
		info.allow_printing.lock();
		println();
		print_image(image);
		print_info("Image \""+filename+"\" saved.");
		info.allow_printing.unlock();
	}
#endif // INTERACTIVE_GRAPHICS_ASCII
	running_encoders++;
	thread encoder(encode_image, image, filename, extension, &running_encoders); // the main bottleneck in rendering images to the hard disk is .png encoding, so encode image in new thread
	encoder.detach(); // detatch thread so it can run concurrently
	camera.rendring_frame.unlock();
}
void LBM_Graphics::write_frame_png(const string& path, bool print_preview) { // save current frame as .png file (smallest file size, but slow)
	write_frame(path, "image", ".png", print_preview);
}
void LBM_Graphics::write_frame_qoi(const string& path, bool print_preview) { // save current frame as .qoi file (small file size, fast)
	write_frame(path, "image", ".qoi", print_preview);
}
void LBM_Graphics::write_frame_bmp(const string& path, bool print_preview) { // save current frame as .bmp file (large file size, fast)
	write_frame(path, "image", ".bmp", print_preview);
}
void LBM_Graphics::write_frame_png(const uint x1, const uint y1, const uint x2, const uint y2, const string& path, bool print_preview) { // save current frame as .png file (smallest file size, but slow)
	write_frame(x1, y1, x2, y2, path, "image", ".png", print_preview);
}
void LBM_Graphics::write_frame_qoi(const uint x1, const uint y1, const uint x2, const uint y2, const string& path, bool print_preview) { // save current frame as .qoi file (small file size, fast)
	write_frame(x1, y1, x2, y2, path, "image", ".qoi", print_preview);
}
void LBM_Graphics::write_frame_bmp(const uint x1, const uint y1, const uint x2, const uint y2, const string& path, bool print_preview) { // save current frame as .bmp file (large file size, fast)
	write_frame(x1, y1, x2, y2, path, "image", ".bmp", print_preview);
}
#endif // GRAPHICS
