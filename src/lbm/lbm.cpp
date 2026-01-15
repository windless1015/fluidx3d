#include "lbm.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>
#ifdef GRAPHICS
#include "lbm_graphics.hpp"
#endif // GRAPHICS



Units units; // for unit conversion


const uint velocity_set = 19u;
const uint dimensions = 3u;
uint bytes_per_cell_host() { // returns the number of Bytes per cell allocated in host memory
	uint bytes_per_cell = 17u; // rho, u, flags
#ifdef SURFACE
	bytes_per_cell += 4u; // phi
#endif // SURFACE
	return bytes_per_cell;
}
uint bytes_per_cell_device() { // returns the number of Bytes per cell allocated in device memory
	uint bytes_per_cell = velocity_set*sizeof(fpxx)+17u; // fi, rho, u, flags
#ifdef SURFACE
	bytes_per_cell += 12u; // phi, mass, flags
#endif // SURFACE

	return bytes_per_cell;
}
uint bandwidth_bytes_per_cell_device() { // returns the bandwidth in Bytes per cell per time step from/to device memory
	uint bandwidth_bytes_per_cell = velocity_set*2u*sizeof(fpxx)+1u; // lattice.set()*2*fi, flags
#ifdef UPDATE_FIELDS
	bandwidth_bytes_per_cell += 16u; // rho, u

#endif // UPDATE_FIELDS
	bandwidth_bytes_per_cell += (velocity_set-1u)*1u; // neighbor flags have to be loaded
#ifdef SURFACE
	bandwidth_bytes_per_cell += (1u+(2u*velocity_set-1u)*sizeof(fpxx)+8u+(velocity_set-1u)*4u) + 1u + 1u + (4u+velocity_set+4u+4u+4u); // surface_0 (flags, fi, mass, massex), surface_1 (flags), surface_2 (flags), surface_3 (rho, flags, mass, massex, phi)
#endif // SURFACE

	return bandwidth_bytes_per_cell;
}
uint3 resolution(const float3 box_aspect_ratio, const uint memory) { // input: simulation box aspect ratio and VRAM occupation in MB, output: grid resolution
	float memory_required = (box_aspect_ratio.x*box_aspect_ratio.y*box_aspect_ratio.z)*(float)bytes_per_cell_device()/1048576.0f; // in MB
	float scaling = cbrt((float)memory/memory_required);
	return uint3(to_uint(scaling*box_aspect_ratio.x), to_uint(scaling*box_aspect_ratio.y), to_uint(scaling*box_aspect_ratio.z));
}

string default_filename(const string& path, const string& name, const string& extension, const ulong t) { // generate a default filename with timestamp
	string time = "00000000"+to_string(t);
	time = substring(time, length(time)-9u, 9u);
	return (path=="" ? get_exe_path()+"export/" : path)+create_file_extension((name=="" ? "file" : name)+"-"+time, extension);
}
string default_filename(const string& name, const string& extension, const ulong t) { // generate a default filename with timestamp at exe_path/export/
	return default_filename("", name, extension, t);
}



LBM_Domain::LBM_Domain(const Device_Info& device_info, const uint Nx, const uint Ny, const uint Nz, const uint Dx, const uint Dy, const uint Dz, const int Ox, const int Oy, const int Oz, const float nu, const float fx, const float fy, const float fz, const float sigma) { // constructor with manual device selection and domain offset
	this->Nx = Nx; this->Ny = Ny; this->Nz = Nz;
	this->Dx = Dx; this->Dy = Dy; this->Dz = Dz;
	this->Ox = Ox; this->Oy = Oy; this->Oz = Oz;
	this->nu = nu;
	this->fx = fx; this->fy = fy; this->fz = fz;
	this->sigma = sigma;
	string opencl_c_code;
#ifdef GRAPHICS
	opencl_c_code = device_defines()+lbm_graphics_device_defines(this)+get_opencl_c_code();
#else // GRAPHICS
	opencl_c_code = device_defines()+get_opencl_c_code();
#endif // GRAPHICS
	this->device = Device(device_info, opencl_c_code);
	print_info("Allocating memory. This may take a few seconds.");
	allocate(device); // lbm first
#ifdef USE_CUDA_LBM
	{
		CudaLBMParams params;
		params.Nx = Nx;
		params.Ny = Ny;
		params.Nz = Nz;
		params.N = get_N();
		params.nu = nu;
		params.fx = fx;
		params.fy = fy;
		params.fz = fz;
		params.sigma = sigma;
		params.w = 1.0f/(3.0f*nu+0.5f);
		params.def_6_sigma = 6.0f*sigma;
		cuda_backend = new CudaLBMBackend();
		cuda_backend->initialize(params);
	}
#endif // USE_CUDA_LBM
}

LBM_Domain::~LBM_Domain() {
#ifdef USE_CUDA_LBM
	delete cuda_backend;
	cuda_backend = nullptr;
#endif // USE_CUDA_LBM
}

void LBM_Domain::allocate(Device& device) {
	const ulong N = get_N();
	fi = Memory<fpxx>(device, N, velocity_set, false);
	rho = Memory<float>(device, N, 1u, true, true, 1.0f);
	u = Memory<float>(device, N, 3u);
	flags = Memory<uchar>(device, N);
	kernel_initialize = Kernel(device, N, "initialize", fi, rho, u, flags);
	kernel_stream_collide = Kernel(device, N, "stream_collide", fi, rho, u, flags, t, fx, fy, fz);
	kernel_update_fields = Kernel(device, N, "update_fields", fi, rho, u, flags, t, fx, fy, fz);

#ifdef SURFACE
	phi = Memory<float>(device, N);
	mass = Memory<float>(device, N, 1u, false);
	massex = Memory<float>(device, N, 1u, false);
	kernel_initialize.add_parameters(mass, massex, phi);
	kernel_stream_collide.add_parameters(mass);
	kernel_surface_capture_outgoing = Kernel(device, N, "surface_0", fi, rho, u, flags, mass, massex, phi, t, fx, fy, fz);
	kernel_surface_mass_exchange = Kernel(device, N, "surface_1", flags);
	kernel_surface_flag_transition = Kernel(device, N, "surface_2", fi, rho, u, flags, t);
	kernel_surface_phi_recompute = Kernel(device, N, "surface_3", rho, flags, mass, massex, phi);
#endif // SURFACE
}

void LBM_Domain::enqueue_initialize() { // call kernel_initialize
#ifdef USE_CUDA_LBM
	cuda_backend->upload_host_fields(rho.data(), u.data(), flags.data(), phi.data());
	cuda_backend->kernel_initialize();
#else
	kernel_initialize.enqueue_run();
#endif // USE_CUDA_LBM
}
void LBM_Domain::enqueue_stream_collide() { // call kernel_stream_collide to perform one LBM time step
#ifdef USE_CUDA_LBM
	cuda_backend->kernel_stream_collide(t);
#else
	kernel_stream_collide.set_parameters(4u, t, fx, fy, fz).enqueue_run();
#endif // USE_CUDA_LBM
}
void LBM_Domain::enqueue_update_fields() { // update fields (rho, u, T) manually
#ifndef UPDATE_FIELDS
	if(t!=t_last_update_fields) { // only run kernel_update_fields if the time step has changed since last update
	#ifdef USE_CUDA_LBM
		cuda_backend->kernel_update_fields(t);
	#else
		kernel_update_fields.set_parameters(4u, t, fx, fy, fz).enqueue_run();
	#endif // USE_CUDA_LBM
		t_last_update_fields = t;
	}
#endif // UPDATE_FIELDS
}
#ifdef SURFACE
void LBM_Domain::enqueue_surface_capture_outgoing() {
#ifdef USE_CUDA_LBM
	cuda_backend->kernel_surface_capture_outgoing(t);
#else
	kernel_surface_capture_outgoing.set_parameters(7u, t, fx, fy, fz).enqueue_run();
#endif // USE_CUDA_LBM
}
void LBM_Domain::enqueue_surface_mass_exchange() {
#ifdef USE_CUDA_LBM
	cuda_backend->kernel_surface_mass_exchange();
#else
	kernel_surface_mass_exchange.enqueue_run();
#endif // USE_CUDA_LBM
}
void LBM_Domain::enqueue_surface_flag_transition() {
#ifdef USE_CUDA_LBM
	cuda_backend->kernel_surface_flag_transition(t);
#else
	kernel_surface_flag_transition.set_parameters(4u, t).enqueue_run();
#endif // USE_CUDA_LBM
}
void LBM_Domain::enqueue_surface_phi_recompute() {
#ifdef USE_CUDA_LBM
	cuda_backend->kernel_surface_phi_recompute();
#else
	kernel_surface_phi_recompute.enqueue_run();
#endif // USE_CUDA_LBM
}
#endif // SURFACE

void LBM_Domain::increment_time_step(const uint steps) {
	t += (ulong)steps; // increment time step
#ifdef UPDATE_FIELDS
	t_last_update_fields = t;
#endif // UPDATE_FIELDS
}
void LBM_Domain::reset_time_step() {
	t = 0ull; // increment time step
#ifdef UPDATE_FIELDS
	t_last_update_fields = t;
#endif // UPDATE_FIELDS
}
void LBM_Domain::finish_queue() {
#ifdef USE_CUDA_LBM
	cuda_backend->synchronize();
#else
	device.finish_queue();
#endif // USE_CUDA_LBM
}

void LBM_Domain::sync_cuda_to_opencl_render() {
#ifdef USE_CUDA_LBM
	cuda_backend->download_fields(rho.data(), u.data(), flags.data(), phi.data());
	rho.enqueue_write_to_device();
	u.enqueue_write_to_device();
	flags.enqueue_write_to_device();
#ifdef SURFACE
	phi.enqueue_write_to_device();
#endif // SURFACE
	device.finish_queue();
#endif // USE_CUDA_LBM
}

ulong LBM_Domain::get_area(const uint direction) {
	const ulong A[3] = { (ulong)Ny*(ulong)Nz, (ulong)Nz*(ulong)Nx, (ulong)Nx*(ulong)Ny };
	return A[direction];
}

uint LBM_Domain::get_velocity_set() const {
	return velocity_set;
}

void LBM_Domain::voxelize_mesh_on_device(const Mesh* mesh, const uchar flag, const float3& rotation_center, const float3& linear_velocity, const float3& rotational_velocity) { // voxelize triangle mesh
	Memory<float3> p0(device, mesh->triangle_number, 1u, mesh->p0);
	Memory<float3> p1(device, mesh->triangle_number, 1u, mesh->p1);
	Memory<float3> p2(device, mesh->triangle_number, 1u, mesh->p2);
	Memory<float> bounding_box_and_velocity(device, 16u);
	const float x0=mesh->pmin.x-2.0f, y0=mesh->pmin.y-2.0f, z0=mesh->pmin.z-2.0f, x1=mesh->pmax.x+2.0f, y1=mesh->pmax.y+2.0f, z1=mesh->pmax.z+2.0f; // use bounding box of mesh to speed up voxelization; add tolerance of 2 cells for re-voxelization of moving objects
	bounding_box_and_velocity[ 0] = as_float(mesh->triangle_number);
	bounding_box_and_velocity[ 1] = x0;
	bounding_box_and_velocity[ 2] = y0;
	bounding_box_and_velocity[ 3] = z0;
	bounding_box_and_velocity[ 4] = x1;
	bounding_box_and_velocity[ 5] = y1;
	bounding_box_and_velocity[ 6] = z1;
	bounding_box_and_velocity[ 7] = rotation_center.x;
	bounding_box_and_velocity[ 8] = rotation_center.y;
	bounding_box_and_velocity[ 9] = rotation_center.z;
	bounding_box_and_velocity[10] = linear_velocity.x;
	bounding_box_and_velocity[11] = linear_velocity.y;
	bounding_box_and_velocity[12] = linear_velocity.z;
	bounding_box_and_velocity[13] = rotational_velocity.x;
	bounding_box_and_velocity[14] = rotational_velocity.y;
	bounding_box_and_velocity[15] = rotational_velocity.z;
	uint direction = 0u;
	if(length(rotational_velocity)==0.0f) { // choose direction of minimum bounding-box cross-section area
		float v[3] = { (y1-y0)*(z1-z0), (z1-z0)*(x1-x0), (x1-x0)*(y1-y0) };
		float vmin = v[0];
		for(uint i=1u; i<3u; i++) {
			if(v[i]<vmin) {
				vmin = v[i];
				direction = i;
			}
		}
	} else { // choose direction closest to rotation axis
		float v[3] = { fabsf(rotational_velocity.x), fabsf(rotational_velocity.y), fabsf(rotational_velocity.z) };
		float vmax = v[0];
		for(uint i=1u; i<3u; i++) {
			if(v[i]>vmax) {
				vmax = v[i];
				direction = i; // find direction of minimum bounding-box cross-section area
			}
		}
	}
	const ulong A[3] = { (ulong)Ny*(ulong)Nz, (ulong)Nz*(ulong)Nx, (ulong)Nx*(ulong)Ny };
	Kernel kernel_voxelize_mesh(device, A[direction], "voxelize_mesh", direction, fi, u, flags, t+1ull, flag, p0, p1, p2, bounding_box_and_velocity);
#ifdef SURFACE
	kernel_voxelize_mesh.add_parameters(mass, massex);
#endif // SURFACE
	p0.write_to_device();
	p1.write_to_device();
	p2.write_to_device();
	bounding_box_and_velocity.write_to_device();
	kernel_voxelize_mesh.run();
}
void LBM_Domain::enqueue_unvoxelize_mesh_on_device(const Mesh* mesh, const uchar flag) { // remove voxelized triangle mesh from LBM grid
	const float x0=mesh->pmin.x, y0=mesh->pmin.y, z0=mesh->pmin.z, x1=mesh->pmax.x, y1=mesh->pmax.y, z1=mesh->pmax.z; // remove all flags in bounding box of mesh
	Kernel kernel_unvoxelize_mesh(device, get_N(), "unvoxelize_mesh", flags, flag, x0, y0, z0, x1, y1, z1);
	kernel_unvoxelize_mesh.run();
}

string LBM_Domain::device_defines() const { return
	"\n	#define def_Nx "+to_string(Nx)+"u"
	"\n	#define def_Ny "+to_string(Ny)+"u"
	"\n	#define def_Nz "+to_string(Nz)+"u"
	"\n	#define def_N "+to_string(get_N())+"ul"
	"\n	#define uxx "+(get_N()<=(ulong)max_uint ? "uint" : "ulong")+"" // switchable data type for index calculation (32-bit uint / 64-bit ulong)

	"\n	#define def_Dx "+to_string(Dx)+"u"
	"\n	#define def_Dy "+to_string(Dy)+"u"
	"\n	#define def_Dz "+to_string(Dz)+"u"

	"\n	#define def_Ox "+to_string(Ox)+"" // offsets are signed integer!
	"\n	#define def_Oy "+to_string(Oy)+""
	"\n	#define def_Oz "+to_string(Oz)+""

	"\n	#define def_Ax "+to_string(Ny*Nz)+"u"
	"\n	#define def_Ay "+to_string(Nz*Nx)+"u"
	"\n	#define def_Az "+to_string(Nx*Ny)+"u"

	"\n	#define def_domain_offset_x "+to_string(0.5f*(float)((int)Nx+2*Ox+(int)Dx*(2*(int)(Dx>1u)-(int)Nx)))+"f"
	"\n	#define def_domain_offset_y "+to_string(0.5f*(float)((int)Ny+2*Oy+(int)Dy*(2*(int)(Dy>1u)-(int)Ny)))+"f"
	"\n	#define def_domain_offset_z "+to_string(0.5f*(float)((int)Nz+2*Oz+(int)Dz*(2*(int)(Dz>1u)-(int)Nz)))+"f"

	"\n	#define D"+to_string(dimensions)+"Q"+to_string(velocity_set)+"" // D3Q19
	"\n	#define def_velocity_set "+to_string(velocity_set)+"u" // LBM velocity set (D3Q19)
	"\n	#define def_dimensions "+to_string(dimensions)+"u" // number spatial dimensions (2D or 3D)
	"\n	#define def_c 0.57735027f" // lattice speed of sound c = 1/sqrt(3)*dt
	"\n	#define def_w " +to_string(1.0f/get_tau())+"f" // relaxation rate w = dt/tau = dt/(nu/c^2+dt/2) = 1/(3*nu+1/2)
	"\n	#define def_w0 (1.0f/3.0f)" // center (0)
	"\n	#define def_ws (1.0f/18.0f)" // straight (1-6)
	"\n	#define def_we (1.0f/36.0f)" // edge (7-18)

#if defined(SRT)
	"\n	#define SRT"
#endif // SRT

	"\n	#define TYPE_S 0x01" // 0b00000001 // (stationary or moving) solid boundary
	"\n	#define TYPE_F 0x08" // 0b00001000 // fluid
	"\n	#define TYPE_I 0x10" // 0b00010000 // interface
	"\n	#define TYPE_G 0x20" // 0b00100000 // gas
	"\n	#define TYPE_X 0x40" // 0b01000000 // reserved type X
	"\n	#define TYPE_Y 0x80" // 0b10000000 // reserved type Y

	"\n	#define TYPE_BO 0x01" // 0b00000001 // any flag bit used for boundaries (temperature excluded)
	"\n	#define TYPE_IF 0x18" // 0b00011000 // change from interface to fluid
	"\n	#define TYPE_IG 0x30" // 0b00110000 // change from interface to gas
	"\n	#define TYPE_GI 0x38" // 0b00111000 // change from gas to interface
	"\n	#define TYPE_SU 0x38" // 0b00111000 // any flag bit used for SURFACE

	"\n	#define fpxx half" // switchable data type (scaled IEEE-754 16-bit floating-point format: 1-5-10, exp-30, +-1.99902344, +-1.86446416E-9, +-1.81898936E-12, 3.311 digits)
	"\n	#define fpxx_copy ushort" // switchable data type for direct copying (scaled IEEE-754 16-bit floating-point format: 1-5-10, exp-30, +-1.99902344, +-1.86446416E-9, +-1.81898936E-12, 3.311 digits)
	"\n	#define load(p,o) vload_half(o,p)*3.0517578E-5f" // special function for loading half
	"\n	#define store(p,o,x) vstore_half_rte((x)*32768.0f,o,p)" // special function for storing half

#ifdef UPDATE_FIELDS
	"\n	#define UPDATE_FIELDS"
#endif // UPDATE_FIELDS

#ifdef VOLUME_FORCE
	"\n	#define VOLUME_FORCE"
#endif // VOLUME_FORCE


#ifdef SURFACE
	"\n	#define SURFACE"
	"\n	#define def_6_sigma "+to_string(6.0f*sigma)+"f" // rho_laplace = 2*o*K, rho = 1-rho_laplace/c^2 = 1-(6*o)*K
#endif // SURFACE


;}




LBM::LBM(const uint Nx, const uint Ny, const uint Nz, const float nu, const float fx, const float fy, const float fz, const float sigma) {
	this->Nx = Nx; this->Ny = Ny; this->Nz = Nz;
	this->Dx = 1u; this->Dy = 1u; this->Dz = 1u;
	const vector<Device_Info> device_infos = { select_device_with_most_flops(get_devices()) };
	sanity_checks_constructor(device_infos, this->Nx, this->Ny, this->Nz, nu, fx, fy, fz, sigma);
	lbm_domain = new LBM_Domain*[1u];
	lbm_domain[0] = new LBM_Domain(device_infos[0], this->Nx, this->Ny, this->Nz, 1u, 1u, 1u, 0, 0, 0, nu, fx, fy, fz, sigma);
	{
		Memory<float>** buffers_rho = new Memory<float>*[1u];
		buffers_rho[0] = &(lbm_domain[0]->rho);
		rho = Memory_Container(this, buffers_rho, "rho");
	} {
		Memory<float>** buffers_u = new Memory<float>*[1u];
		buffers_u[0] = &(lbm_domain[0]->u);
		u = Memory_Container(this, buffers_u, "u");
	} {
		Memory<uchar>** buffers_flags = new Memory<uchar>*[1u];
		buffers_flags[0] = &(lbm_domain[0]->flags);
		flags = Memory_Container(this, buffers_flags, "flags");
	} {
#ifdef SURFACE
		Memory<float>** buffers_phi = new Memory<float>*[1u];
		buffers_phi[0] = &(lbm_domain[0]->phi);
		phi = Memory_Container(this, buffers_phi, "phi");
#endif // SURFACE
	}
}
LBM::LBM(const uint3 N, const float nu, const float fx, const float fy, const float fz, const float sigma)
	:LBM(N.x, N.y, N.z, nu, fx, fy, fz, sigma) {
}
LBM::~LBM() {
#ifdef GRAPHICS
	camera.allow_rendering = false;
#endif // GRAPHICS
	info.print_finalize();
	for(uint d=0u; d<get_D(); d++) delete lbm_domain[d];
	delete[] lbm_domain;
}

void LBM::sanity_checks_constructor(const vector<Device_Info>& device_infos, const uint Nx, const uint Ny, const uint Nz, const float nu, const float fx, const float fy, const float fz, const float sigma) { // sanity checks on grid resolution and extension support
	if((ulong)Nx*(ulong)Ny*(ulong)Nz==0ull) print_error("Grid point number is 0: "+to_string(Nx)+"x"+to_string(Ny)+"x"+to_string(Nz)+" = 0.");
	uint memory_available = max_uint; // in MB
	for(Device_Info device_info : device_infos) memory_available = min(memory_available, device_info.memory);
	uint memory_required = (uint)((ulong)Nx*(ulong)Ny*(ulong)Nz*(ulong)bytes_per_cell_device()/1048576ull); // in MB
	if(memory_required>memory_available) {
		float factor = cbrt((float)memory_available/(float)memory_required);
		const uint maxNx=(uint)(factor*(float)Nx), maxNy=(uint)(factor*(float)Ny), maxNz=(uint)(factor*(float)Nz);
		string message = "Grid resolution ("+to_string(Nx)+", "+to_string(Ny)+", "+to_string(Nz)+") is too large: "+to_string(memory_required)+" MB required, "+to_string(memory_available)+" MB available. Largest possible resolution is ("+to_string(maxNx)+", "+to_string(maxNy)+", "+to_string(maxNz)+"). Restart the simulation with lower resolution or on different device(s) with more memory.";
		print_error(message);
	}
	if(nu==0.0f) print_error("Viscosity cannot be 0. Change it in setup.cpp."); // sanity checks for viscosity
	else if(nu<0.0f) print_error("Viscosity cannot be negative. Remove the \"-\" in setup.cpp.");
#if !defined(SRT)
	print_error("No LBM collision operator selected. Uncomment \"#define SRT\" in defines.hpp");
#endif // SRT
#ifndef VOLUME_FORCE
	if(fx!=0.0f||fy!=0.0f||fz!=0.0f) print_error("Volume force is set in LBM constructor in main_setup(), but VOLUME_FORCE is not enabled. Uncomment \"#define VOLUME_FORCE\" in defines.hpp.");
#endif // VOLUME_FORCE
#ifndef SURFACE
	if(sigma!=0.0f) print_error("Surface tension is set in LBM constructor in main_setup(), but SURFACE is not enabled. Uncomment \"#define SURFACE\" in defines.hpp.");
#endif // SURFACE

}

void LBM::sanity_checks_initialization() { // sanity checks during initialization on used extensions based on used flags
	uchar flags_used = 0u;
	bool surface_used=false; // identify used extensions based used flags
	const uint threads = thread::hardware_concurrency();
	vector<uchar> t_flags_used(threads, 0u);
	parallel_for(get_N(), threads, [&](ulong n, uint t) {
		const uchar flagsn = flags[n];
		t_flags_used[t] = t_flags_used[t]|flagsn;
	});
	for(uint t=0u; t<threads; t++) {
		flags_used = flags_used|t_flags_used[t];
	}
	surface_used = (bool)(flags_used&(TYPE_F|TYPE_I|TYPE_G));
#ifndef SURFACE
	if(surface_used) print_error("Some cells are set as fluid/interface/gas with the TYPE_F/TYPE_I/TYPE_G flags, but SURFACE is not enabled. Uncomment \"#define SURFACE\" in defines.hpp.");
#else // SURFACE
	if(!surface_used) print_error("The SURFACE extension is enabled but no fluid/interface/gas cells (TYPE_F/TYPE_I/TYPE_G flags) are placed in the simulation box. Disable the extension by commenting out \"#define SURFACE\" in defines.hpp.");
#endif // SURFACE

}

void LBM::initialize() { // write all data fields to device and call kernel_initialize
#ifndef BENCHMARK
	sanity_checks_initialization();
#endif // BENCHMARK

	for(uint d=0u; d<get_D(); d++) lbm_domain[d]->rho.enqueue_write_to_device();
	for(uint d=0u; d<get_D(); d++) lbm_domain[d]->u.enqueue_write_to_device();
	for(uint d=0u; d<get_D(); d++) lbm_domain[d]->flags.enqueue_write_to_device();
#ifdef SURFACE
	for(uint d=0u; d<get_D(); d++) lbm_domain[d]->phi.enqueue_write_to_device();
#endif // SURFACE
	for(uint d=0u; d<get_D(); d++) lbm_domain[d]->enqueue_initialize(); // odd time step is baked-in the kernel

	for(uint d=0u; d<get_D(); d++) lbm_domain[d]->finish_queue();
	for(uint d=0u; d<get_D(); d++) lbm_domain[d]->reset_time_step(); // set time step to 0 again
	initialized = true;
}

void LBM::step_stream_collide() {
	for(uint d=0u; d<get_D(); d++) lbm_domain[d]->enqueue_stream_collide(); // run LBM stream_collide kernel after domain communication
}
void LBM::step_exchange_rho_u_flags() {
}
#ifdef SURFACE
void LBM::step_surface_capture_outgoing() {
	for(uint d=0u; d<get_D(); d++) lbm_domain[d]->enqueue_surface_capture_outgoing();
}
void LBM::step_surface_topology_update() {
	step_surface_mass_exchange();
	step_surface_flag_transition();
	step_surface_phi_recompute();
	step_surface_excess_mass_distribute();
}
void LBM::step_surface_mass_exchange() {
	for(uint d=0u; d<get_D(); d++) lbm_domain[d]->enqueue_surface_mass_exchange();
}
void LBM::step_surface_flag_transition() {
	for(uint d=0u; d<get_D(); d++) lbm_domain[d]->enqueue_surface_flag_transition();
}
void LBM::step_surface_phi_recompute() {
	for(uint d=0u; d<get_D(); d++) lbm_domain[d]->enqueue_surface_phi_recompute();
}
void LBM::step_surface_excess_mass_distribute() {
}
#endif // SURFACE
void LBM::step_exchange_fi() {
}
void LBM::step_finalize_time_step() {
	if(get_D()==1u) for(uint d=0u; d<get_D(); d++) lbm_domain[d]->finish_queue(); // this additional domain synchronization barrier is only required in single-GPU, as communication calls already provide all necessary synchronization barriers in multi-GPU
	for(uint d=0u; d<get_D(); d++) lbm_domain[d]->increment_time_step();
}

void LBM::do_time_step() { // call kernel_stream_collide to perform one LBM time step
#ifdef SURFACE
	step_surface_capture_outgoing();
#endif // SURFACE
	step_stream_collide();
	step_exchange_rho_u_flags();
#ifdef SURFACE
	step_surface_topology_update();
#endif // SURFACE
	step_exchange_fi();
	step_finalize_time_step();
}

void LBM::run(const ulong steps, const ulong total_steps) { // initializes the LBM simulation (copies data to device and runs initialize kernel), then runs LBM
	info.append(steps, total_steps, get_t()); // total_steps parameter is just for runtime estimation
	if(!initialized) {
		initialize();
		info.print_initialize(this); // only print setup info if the setup is new (run() was not called before)
#ifdef GRAPHICS
		camera.allow_rendering = true;
#endif // GRAPHICS
	}
	Clock clock;
	for(ulong i=1ull; i<=steps; i++) {
#if defined(INTERACTIVE_GRAPHICS)||defined(INTERACTIVE_GRAPHICS_ASCII)
		while(!key_P&&running) sleep(0.016);
		if(!running) break;
#endif // INTERACTIVE_GRAPHICS_ASCII || INTERACTIVE_GRAPHICS
		clock.start();
		do_time_step();
		info.update(clock.stop());
	}
	if(get_D()>1u) for(uint d=0u; d<get_D(); d++) lbm_domain[d]->finish_queue(); // wait for everything to finish (multi-GPU only)
}

void LBM::update_fields() { // update fields (rho, u, T) manually
	for(uint d=0u; d<get_D(); d++) lbm_domain[d]->enqueue_update_fields();
	for(uint d=0u; d<get_D(); d++) lbm_domain[d]->finish_queue();
}

void LBM::reset() { // reset simulation (takes effect in following run() call)
	initialized = false;
}

void LBM::write_status(const string& path) { // write LBM status report to a .txt file
	string status = "";
	status += "Grid Resolution = "+to_string(Nx)+" x "+to_string(Ny)+" x "+to_string(Nz)+" = "+to_string(get_N())+"\n";
	status += "Grid Domains = "+to_string(Dx)+" x "+to_string(Dy)+" x "+to_string(Dz)+" = "+to_string(get_D())+"\n";
	status += "LBM Type = D"+string(get_velocity_set()==9 ? "2" : "3")+"Q"+to_string(get_velocity_set())+" "+info.collision+"\n";
	status += "Memory Usage = CPU "+to_string(info.cpu_mem_required)+" MB, GPU "+to_string(get_D())+"x "+to_string(info.gpu_mem_required)+" MB\n";
	status += "Maximum Allocation Size = "+to_string((uint)(get_N()/(ulong)get_D()*(ulong)(get_velocity_set()*sizeof(fpxx))/1048576ull))+" MB\n";
	status += "Time Steps = "+to_string(get_t())+" / "+(info.steps==max_ulong ? "infinite" : to_string(info.steps))+"\n";
	status += "Runtime = "+print_time(info.runtime_total)+" (total) = "+print_time(info.runtime_lbm)+" (LBM) + "+print_time(info.runtime_total-info.runtime_lbm)+" (rendering and data evaluation)\n";
	status += "Average MLUPs/s = "+to_string(to_uint(1E-6*(double)get_N()*(double)get_t()/info.runtime_lbm))+"\n";
	status += "Kinematic Viscosity = "+to_string(get_nu())+"\n";
	status += "Relaxation Time = "+to_string(get_tau())+"\n";
	status += "Maximum Reynolds Number = "+to_string(get_Re_max())+"\n";
#ifdef VOLUME_FORCE
	status += "Volume Force = ("+to_string(get_fx())+", "+to_string(get_fy())+", "+to_string(get_fz())+")\n";
#endif // VOLUME_FORCE
#ifdef SURFACE
	status += "Surface Tension Coefficient = "+to_string(get_sigma())+"\n";
#endif // SURFACE

	const string filename = default_filename(path, "status", ".txt", get_t());
	write_file(filename, status);
}

void LBM::voxelize_mesh_on_device(const Mesh* mesh, const uchar flag, const float3& rotation_center, const float3& linear_velocity, const float3& rotational_velocity) { // voxelize triangle mesh
	if(get_D()==1u) {
		lbm_domain[0]->voxelize_mesh_on_device(mesh, flag, rotation_center, linear_velocity, rotational_velocity); // if this crashes on Windows, create a TdrDelay 32-bit DWORD with decimal value 300 in Computer\HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\GraphicsDrivers
	} else {
		parallel_for((ulong)get_D(), get_D(), [&](ulong d) {
			lbm_domain[d]->voxelize_mesh_on_device(mesh, flag, rotation_center, linear_velocity, rotational_velocity);
		});
	}
	if(!initialized) {
		flags.read_from_device();
		u.read_from_device();
	}
}
void LBM::unvoxelize_mesh_on_device(const Mesh* mesh, const uchar flag) { // remove voxelized triangle mesh from LBM grid by removing all flags in mesh bounding box (only required when bounding box size changes during re-voxelization)
	for(uint d=0u; d<get_D(); d++) lbm_domain[d]->enqueue_unvoxelize_mesh_on_device(mesh, flag);
	for(uint d=0u; d<get_D(); d++) lbm_domain[d]->finish_queue();
}
void LBM::write_mesh_to_vtk(const Mesh* mesh, const string& path, const bool convert_to_si_units) const { // write mesh to binary .vtk file
	const string header_1 = "# vtk DataFile Version 3.0\nData\nBINARY\nDATASET POLYDATA\nPOINTS "+to_string(3u*mesh->triangle_number)+" float\n";
	const string header_2 = "POLYGONS "+to_string(mesh->triangle_number)+" "+to_string(4u*mesh->triangle_number)+"\n";
	float* points = new float[9u*mesh->triangle_number];
	int* triangles = new int[4u*mesh->triangle_number];
	const float spacing = convert_to_si_units ? units.si_x(1.0f) : 1.0f;
	const float3 offset = center();
	parallel_for(mesh->triangle_number, [&](uint i) {
		points[9u*i   ] = reverse_bytes(spacing*(mesh->p0[i].x-offset.x));
		points[9u*i+1u] = reverse_bytes(spacing*(mesh->p0[i].y-offset.y));
		points[9u*i+2u] = reverse_bytes(spacing*(mesh->p0[i].z-offset.z));
		points[9u*i+3u] = reverse_bytes(spacing*(mesh->p1[i].x-offset.x));
		points[9u*i+4u] = reverse_bytes(spacing*(mesh->p1[i].y-offset.y));
		points[9u*i+5u] = reverse_bytes(spacing*(mesh->p1[i].z-offset.z));
		points[9u*i+6u] = reverse_bytes(spacing*(mesh->p2[i].x-offset.x));
		points[9u*i+7u] = reverse_bytes(spacing*(mesh->p2[i].y-offset.y));
		points[9u*i+8u] = reverse_bytes(spacing*(mesh->p2[i].z-offset.z));
		triangles[4u*i   ] = reverse_bytes(3); // 3 vertices per triangle
		triangles[4u*i+1u] = reverse_bytes(3*(int)i  ); // vertex 0
		triangles[4u*i+2u] = reverse_bytes(3*(int)i+1); // vertex 1
		triangles[4u*i+3u] = reverse_bytes(3*(int)i+2); // vertex 2
	});
	const string filename = default_filename(path, "mesh", ".vtk", get_t());
	create_folder(filename);
	std::ofstream file(filename, std::ios::out|std::ios::binary);
	file.write(header_1.c_str(), header_1.length()); // write non-binary file header
	file.write((char*)points, 4u*9u*mesh->triangle_number); // write binary data
	file.write(header_2.c_str(), header_2.length()); // write non-binary file header
	file.write((char*)triangles, 4u*4u*mesh->triangle_number); // write binary data
	file.close();
	delete[] points;
	delete[] triangles;
	info.allow_printing.lock();
	print_info("File \""+filename+"\" saved.");
	info.allow_printing.unlock();
}
void LBM::voxelize_stl(const string& path, const float3& center, const float3x3& rotation, const float size, const uchar flag) { // voxelize triangle mesh
	const Mesh* mesh = read_stl(path, this->size(), center, rotation, size);
	flags.write_to_device();
	voxelize_mesh_on_device(mesh, flag);
	delete mesh;
	flags.read_from_device();
}
void LBM::voxelize_stl(const string& path, const float3x3& rotation, const float size, const uchar flag) { // read and voxelize binary .stl file (place in box center)
	voxelize_stl(path, center(), rotation, size, flag);
}
void LBM::voxelize_stl(const string& path, const float3& center, const float size, const uchar flag) { // read and voxelize binary .stl file (no rotation)
	voxelize_stl(path, center, float3x3(1.0f), size, flag);
}
void LBM::voxelize_stl(const string& path, const float size, const uchar flag) { // read and voxelize binary .stl file (place in box center, no rotation)
	voxelize_stl(path, center(), float3x3(1.0f), size, flag);
}




void LBM::write_vtk(const string& filename) { // write simulation data to VTK file (ASCII VTI)
	update_fields(); // ensure macroscopic fields are up-to-date on device
	flags.read_from_device();
#ifdef SURFACE
	phi.read_from_device();
#endif // SURFACE

	const float spacing = units.si_x(1.0f);
	const float3 origin = spacing*float3(0.5f-0.5f*(float)Nx, 0.5f-0.5f*(float)Ny, 0.5f-0.5f*(float)Nz);
	const ulong N = (ulong)Nx*(ulong)Ny*(ulong)Nz;

	const string output = create_file_extension(filename, ".vti");
	create_folder(output);
	std::ofstream file(output);
	if(!file.is_open()) {
		print_error("Could not open file "+output+" for writing.");
		return;
	}

	file << "<?xml version=\"1.0\"?>\n";
	file << "<VTKFile type=\"ImageData\" version=\"0.1\" byte_order=\"LittleEndian\">\n";
	file << "  <ImageData WholeExtent=\"0 " << Nx-1u << " 0 " << Ny-1u << " 0 " << Nz-1u << "\""
	     << " Origin=\"" << origin.x << " " << origin.y << " " << origin.z << "\""
	     << " Spacing=\"" << spacing << " " << spacing << " " << spacing << "\">\n";
	file << "    <Piece Extent=\"0 " << Nx-1u << " 0 " << Ny-1u << " 0 " << Nz-1u << "\">\n";
	file << "      <PointData Scalars=\"fill\">\n";

#ifdef SURFACE
	{ // fill level
		file << "        <DataArray type=\"Float32\" Name=\"fill\" format=\"ascii\">\n";
		for(ulong n=0ull; n<N; ++n) {
			float v = phi[n];
			if(v < 0.0f) v = 0.0f;
			if(v > 1.0f) v = 1.0f;
			file << v << " ";
			if(n%20ull==19ull) file << "\n";
		}
		file << "\n        </DataArray>\n";
	}
#endif // SURFACE

	{ // cell type
		file << "        <DataArray type=\"Int32\" Name=\"cell_type\" format=\"ascii\">\n";
		for(ulong n=0ull; n<N; ++n) {
			file << (int)flags[n] << " ";
			if(n%20ull==19ull) file << "\n";
		}
		file << "\n        </DataArray>\n";
	}

	file << "      </PointData>\n";
	file << "    </Piece>\n";
	file << "  </ImageData>\n";
	file << "</VTKFile>\n";
	file.close();

	info.allow_printing.lock();
	print_info("File \""+output+"\" saved.");
	info.allow_printing.unlock();
}
