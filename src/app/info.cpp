#include "info.hpp"

Info info;

void Info::append(const ulong steps, const ulong total_steps, const ulong t) {
	if(total_steps==max_ulong) { // total_steps is not provided/used
		this->steps = steps; // has to be executed before info.print_initialize()
		this->steps_last = t; // reset last step count if multiple run() commands are executed consecutively
		this->runtime_total_last = this->runtime_total; // reset last runtime if multiple run() commands are executed consecutively
		this->runtime_total = clock.stop();
	} else { // total_steps has been specified
		this->steps = total_steps; // has to be executed before info.print_initialize()
	}
}
void Info::update(const double dt) {
	this->runtime_lbm_timestep_last = dt; // exact dt
	this->runtime_lbm_timestep_smooth = (dt+0.3)/(0.3/runtime_lbm_timestep_smooth+1.0); // smoothed dt
	this->runtime_lbm += dt; // skip first step since it is likely slower than average
	this->runtime_total = clock.stop();
}
double Info::time() const { // returns either elapsed time or remaining time
	if(core==nullptr) return 0.0;
	return steps==max_ulong ? runtime_total : ((double)steps/(double)max(core->get_t()-steps_last, 1ull)-1.0)*(runtime_total-runtime_total_last); // time estimation on average so far
}
void Info::print_logo() const {
	const int a=color_light_blue, b=color_orange, c=color_pink;
	print(".-----------------------------------------------------------------------------.\n");
	print("|                      "); print(  " ______________  ", a);                  print(" ______________ ", b); print("                      |\n");
	print("|                       "); print( "\\   ________  | ", a);                  print("|  ________   /", b); print("                       |\n");
	print("|                        "); print("\\  \\       | | ", a);                  print("| |       /  /", b); print("                        |\n");
	print("|                         "); print("\\  \\      | | ", a);                  print("| |      /  /", b); print("                         |\n");
	print("|                          "); print("\\  \\     | | ", a);                  print("| |     /  /", b); print("                          |\n");
	print("|                           "); print("\\  \\_.-\"  | ", a);                print("|  \"-._/  /", b); print("                           |\n");
	print("|                            "); print("\\    _.-\" ", a);  print("_ ", c);  print("\"-._    /", b); print("                            |\n");
	print("|                             "); print("\\.-\" ", a); print("_.-\" \"-._ ", c); print("\"-./", b); print("                             |\n");
	print("|                              ");                 print(" .-\"  .-\"-.  \"-. ", c);               print("                              |\n");
	print("|                               ");                 print("\\  v\"     \"v  /", c);               print("                               |\n");
	print("|                                ");                 print("\\  \\     /  /", c);                print("                                |\n");
	print("|                                 ");                 print("\\  \\   /  /", c);                print("                                 |\n");
	print("|                                  ");                 print("\\  \\ /  /", c);                print("                                  |\n");
	print("|                                   ");                 print("\\  '  /", c);                 print("                                   |\n");
	print("|                                    ");                 print("\\   /", c);                 print("                                    |\n");
	print("|                                     ");                 print("\\ /", c);                 print("                FluidX3D Version 3.2 |\n");
	print("|                                      ");                 print( "'", c);                 print("     Copyright (c) Dr. Moritz Lehmann |\n");
	print("|-----------------------------------------------------------------------------|\n");
}
void Info::print_initialize(lbm::LBMCore* core) {
	info.allow_printing.lock(); // disable print_update() until print_initialize() has finished
	this->core = core;
#if defined(SRT)
	collision = "SRT";
#endif // SRT
#if defined(FP16S)
	collision += " (FP32/FP16S)";
#endif // FP16S
	cpu_mem_required = 0u;
	gpu_mem_required = 0u;
	const float Re = 0.0f;
	println("|-----------------.-----------------------------------------------------------|");
	println("| Grid Resolution | "+alignr(57u, to_string(core->get_Nx())+" x "+to_string(core->get_Ny())+" x "+to_string(core->get_Nz())+" = "+to_string(core->nCellsU()))+" |");
	println("| Grid Domains    | "+alignr(57u, to_string(core->get_Dx())+" x "+to_string(core->get_Dy())+" x "+to_string(core->get_Dz())+" = "+to_string(core->get_D()))+" |");
	println("| LBM Type        | "+alignr(57u, /***************/ "D3Q19 "+collision)+" |");
	println("| Memory Usage    | "+alignr(54u, /*******/ "CPU "+to_string(cpu_mem_required)+" MB, GPU "+to_string(core->get_D())+"x "+to_string(gpu_mem_required))+" MB |");
	println("| Max Alloc Size  | "+alignr(54u, /*************/ "0 MB")+" |");
	println("| Time Steps      | "+alignr(57u, /***************************************************************/ (steps==max_ulong ? "infinite" : to_string(steps)))+" |");
	println("| Kin. Viscosity  | "+alignr(57u, /*************************************************************************************/ to_string(core->get_nu(), 8u))+" |");
	println("| Relaxation Time | "+alignr(57u, /************************************************************************************/ to_string(core->get_tau(), 8u))+" |");
	println("| Reynolds Number | "+alignr(57u, /******************************************/ "Re < "+string(Re>=100.0f ? to_string(to_uint(Re)) : to_string(Re, 6u)))+" |");
#ifdef VOLUME_FORCE
	println("| Volume Force    | "+alignr(57u, alignr(15u, to_string(core->get_fx(), 8u))+","+alignr(15u, to_string(core->get_fy(), 8u))+","+alignr(15u, to_string(core->get_fz(), 8u)))+" |");
#endif // VOLUME_FORCE
#ifdef SURFACE
	println("| Surface Tension | "+alignr(57u, /**********************************************************************************/ to_string(core->get_sigma(), 8u))+" |");
#endif // SURFACE
#ifndef INTERACTIVE_GRAPHICS_ASCII
	println("|---------.-------'-----.-----------.-------------------.---------------------|");
	println("| MLUPs   | Bandwidth   | Steps/s   | Current Step      | "+string(steps==max_ulong?"Elapsed Time  ":"Time Remaining")+"      |");
#else // INTERACTIVE_GRAPHICS_ASCII
	println("'-----------------'-----------------------------------------------------------'");
#endif // INTERACTIVE_GRAPHICS_ASCII
	clock.start();
	info.allow_printing.unlock();
}
void Info::print_update() const {
	if(core==nullptr) return;
	info.allow_printing.lock();
	reprint(
		"|"+alignr(8, to_uint((double)core->nCellsU()*1E-6/runtime_lbm_timestep_smooth))+" |"+ // MLUPs
		alignr(7, to_uint(0.0))+" GB/s |"+ // memory bandwidth
		alignr(10, to_uint(1.0/runtime_lbm_timestep_smooth))+" | "+ // steps/s
		(steps==max_ulong ? alignr(17, core->get_t()) : alignr(12, core->get_t())+" "+print_percentage((float)(core->get_t()-steps_last)/(float)steps))+" | "+ // current step
		alignr(19, print_time(time()))+" |" // either elapsed time or remaining time
	);
#ifdef GRAPHICS
	if(key_G) { // print camera settings
		const string camera_position = "float3("+alignr(9u, to_string(camera.pos.x/(float)core->get_Nx(), 6u))+"f*(float)Nx, "+alignr(9u, to_string(camera.pos.y/(float)core->get_Ny(), 6u))+"f*(float)Ny, "+alignr(9u, to_string(camera.pos.z/(float)core->get_Nz(), 6u))+"f*(float)Nz)";
		const string camera_rx_ry_fov = alignr(6u, to_string(degrees(camera.rx)-90.0, 1u))+"f, "+alignr(5u, to_string(180.0-degrees(camera.ry), 1u))+"f, "+alignr(5u, to_string(camera.fov, 1u))+"f";
		const string camera_zoom = alignr(8u, to_string(camera.zoom*(float)fmax(fmax(core->get_Nx(), core->get_Ny()), core->get_Nz())/(float)min(camera.width, camera.height), 6u))+"f";
		if(camera.free) println("\rCamera free: pos="+camera_position+", rot/fov="+camera_rx_ry_fov+";");
		else println("\rCamera centered: rot/fov="+camera_rx_ry_fov+", zoom="+camera_zoom+";          ");
		key_G = false;
	}
#endif // GRAPHICS
	info.allow_printing.unlock();
}
void Info::print_finalize() {
	core = nullptr;
	println("\n|---------'-------------'-----------'-------------------'---------------------|");
}
