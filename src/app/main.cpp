#include "info.hpp"
#include "setup.hpp"
#include "app_model.hpp"
#ifdef GRAPHICS
#include "../core/units.hpp"
#include "../render/graphics.hpp"
#endif

lbm::LBMCore* app_core = nullptr;

#ifdef GRAPHICS
void draw_scale(const int field_mode, const int color) {
	float scale_min=0.0f, scale_max=1.0f;
	string title = "";
	const int label_count = 10;
	switch(field_mode) {
		case 0: // coloring by velocity
			scale_min = 0.0f;
			scale_max = units.si_u(1.0f)==1.0f ? (GRAPHICS_U_MAX) : units.si_u(0.57735027f*(GRAPHICS_U_MAX));
			title = "velocity u / "+string(units.si_u(1.0f)==1.0f ? "c" : "[m/s]");
			break;
		case 1: // coloring by density
			scale_min = units.si_rho(1.0f)-units.si_rho(GRAPHICS_RHO_DELTA);
			scale_max = units.si_rho(1.0f)+units.si_rho(GRAPHICS_RHO_DELTA);
			title = "density rho / "+string(units.si_u(1.0f)==1.0f ? "1" : "[kg/m^3]");
			break;
	}
	const int margin_x=2*(FONT_WIDTH), margin_y=1*(FONT_HEIGHT); // margins in x and y
	const int ox=camera.width-16*(FONT_WIDTH)-margin_x-1, oy=(FONT_HEIGHT)*3/2+margin_y+8; // plot area offset x/y
	const int label_length_max = scale_max>=1000.0f ? (int)length(to_string(to_int(scale_max))) : 5;
	const int w = camera.width-(ox+margin_x+label_length_max*(FONT_WIDTH)+8); // width of color scale
	const int h = camera.height-(FONT_HEIGHT)*3/2-12*(FONT_HEIGHT)-2*margin_y-4; // height of color scale
	const int N = min(((h/(FONT_HEIGHT))/2)*2, label_count); // number of labels on the y-axis
	for(int i=0; i<h; i++) {
		const float v = (float)i/(float)h;
		int c = 0;
		switch(field_mode) {
			case 0: c = colorscale_rainbow(v); break; // coloring by velocity
			case 1: c = colorscale_twocolor(v, GRAPHICS_BACKGROUND_COLOR); break; // coloring by density
			case 2: c = colorscale_iron(v); break; // coloring by temperature
		}
		draw_line_label(ox, oy+h-i, ox+w, oy+h-i, c);
	}
	draw_line_label(ox  , oy+h, ox+w+1, oy+h  , color); // x-axis
	draw_line_label(ox  , oy  , ox    , oy+h+1, color); // y-axis
	draw_line_label(ox  , oy  , ox+w+1, oy    , color); // x-axis mirror
	draw_line_label(ox+w, oy  , ox+w  , oy+h+1, color); // y-axis mirror
	for(int i=0; i<=N; i++) {
		const float f = (float)i/(float)N;
		const float v = scale_min+f*(scale_max-scale_min);
		const string ly = scale_max>=1000.0f ? to_string(to_int(v)) : scale_max>=100.0f ? to_string(v, 1u) : scale_max>=10.0f ? to_string(v, 2u) : to_string(v, 3u);
		const int y = h-to_int(h*f);
		draw_line_label(ox, oy+y, ox+4, oy+y, color); // y-axis tickmarks
		draw_line_label(ox+w-3, oy+y, ox+w+4, oy+y, color); // y-axis mirror tickmarks
		draw_label(ox+w+7, oy+y-(FONT_HEIGHT)/2, ly, color); // y-axis labels
	}
	draw_label(ox+min(0, w+7+label_length_max*(FONT_WIDTH)-(int)length(title)*(FONT_WIDTH)), oy-(FONT_HEIGHT)*3/2-6, title, color); // colorbar title
}
void main_label(const double frametime) {
	if(camera.allow_rendering&&camera.allow_labeling) {
		info.print_update();
		const int c = invert(GRAPHICS_BACKGROUND_COLOR);
		{
			const int ox=camera.width-37*(FONT_WIDTH)-1, oy=camera.height-11*(FONT_HEIGHT)-1;
			int i = 0;
			const float Re = 0.0f;
			const double pn=(double)info.core->nCellsU(), mt=0.0;
			draw_label(ox, oy+i, "Resolution "     +alignr(26u, /********/ to_string(info.core->get_Nx())+"x"+to_string(info.core->get_Ny())+"x"+to_string(info.core->get_Nz())+" = "+to_string(info.core->nCellsU())), c); i+=FONT_HEIGHT;
			//draw_label(ox, oy+i, "Volume Force "   +alignr(16u, /***************************************************/ info.lbm->get_fx())+","+alignr(15, info.lbm->get_fy())+", "+alignr(15, info.lbm->get_fz()), c); i+=FONT_HEIGHT;
			draw_label(ox, oy+i, "Kin. Viscosity " +alignr(22u, /***********************************************************************************************************/ to_string(info.core->get_nu(), 8u)), c); i+=FONT_HEIGHT;
			draw_label(ox, oy+i, "Relaxation Time "+alignr(21u, /**********************************************************************************************************/ to_string(info.core->get_tau(), 8u)), c); i+=FONT_HEIGHT;
			draw_label(ox, oy+i, "Reynolds Number "+alignr(21u, /*********************************************************************/ "Re < "+string(Re>=100.0f ? to_string(to_uint(Re)) : to_string(Re, 6u))), c); i+=FONT_HEIGHT;
			draw_label(ox, oy+i, "LBM Type "       +alignr(28u, /**************************/ "D3Q19 "+info.collision), c); i+=FONT_HEIGHT;
			draw_label(ox, oy+i, "Memory "         +alignr(30u, /****************/ "CPU "+to_string(info.cpu_mem_required)+" MB, GPU "+to_string(info.core->get_D())+"x "+to_string(info.gpu_mem_required)+" MB"), c); i+=FONT_HEIGHT;
			draw_label(ox, oy+i, (info.steps==max_ulong ? "Elapsed Time   " : "Remaining Time ")+alignr(22u, /************************************************************************/ print_time(info.time())), c); i+=FONT_HEIGHT;
			draw_label(ox, oy+i, "Simulation Time "+alignr(21u, /**************************************/ (units.si_t(1ull)==1.0f?to_string(info.core->get_t()):to_string(units.si_t(info.core->get_t()), 6u))+"s"), c); i+=FONT_HEIGHT;
			draw_label(ox, oy+i, "MLUPs "          +alignr(31u, alignr(5u, to_uint(pn*1E-6/info.runtime_lbm_timestep_smooth))+" ("+alignr(5u, to_uint(pn*mt*1E-9/info.runtime_lbm_timestep_smooth))+"    GB/s)"), c); i+=FONT_HEIGHT;
			draw_label(ox, oy+i, "Steps "          +alignr(31u, /************************************/ alignr(10u, info.core->get_t())+" ("+alignr(5, to_uint(1.0/info.runtime_lbm_timestep_smooth))+" Steps/s)"), c); i+=FONT_HEIGHT;
			draw_label(ox, oy+i, "FPS "            +alignr(33u, /************************************************************/ alignr(4u, to_uint(1.0/frametime))+" ("+alignr(5u, camera.fps_limit)+" fps max)"), c);
		}
		draw_label(2, camera.height-1*(FONT_HEIGHT)-1, "FluidX3D v3.2 Copyright (c) Dr. Moritz Lehmann", c);
		if(!key_H) {
			draw_label(camera.width-16*(FONT_WIDTH)-1, 2, "Press H for Help", c);
		} else {
			const int ox=2, oy=2;
			int i = 0;
			draw_label(ox, oy+i, "Keyboard/Mouse Controls: ", c); i+=2*FONT_HEIGHT;
			draw_label(ox, oy+i, "P ("+string(key_P?"running ":" paused ")+"): start/pause simulation", c); i+=FONT_HEIGHT;
			draw_label(ox, oy+i, "H ("+string(key_H?" shown  ":" hidden ")+"): show/hide help", c); i+=2*FONT_HEIGHT;
			draw_label(ox, oy+i, "Mouse drag: orbit camera", c); i+=FONT_HEIGHT;
			draw_label(ox, oy+i, "Right drag: pan camera", c); i+=FONT_HEIGHT;
			draw_label(ox, oy+i, "Scrollwheel: zoom", c); i+=FONT_HEIGHT;
			draw_label(ox, oy+i, "Y/X ("+alignr(3u, to_int(camera.fov))+"): adjust camera field of view", c); i+=FONT_HEIGHT;
			draw_label(ox, oy+i, "F ("+string(camera.free?"  free  ":"centered")+"): toggle centered/free camera mode", c); i+=FONT_HEIGHT;
			draw_label(ox, oy+i, "Esc/Alt+F4: quit", c);
		}
	}
}

void main_graphics() {
	// OpenGL renderer handles drawing in graphics loop.
}
#endif // GRAPHICS

void main_physics() {
	info.print_logo();
	main_setup(); // execute setup
	running = false;
	exit(0); // make sure that the program stops
}

#ifndef GRAPHICS
int main(int argc, char* argv[]) {
	info.allow_printing.lock();
	main_arguments = get_main_arguments(argc, argv);
	thread compute_thread(main_physics);
	info.allow_printing.unlock();
	do { // main console loop
		info.print_update();
		sleep(0.050);
	} while(running);
	compute_thread.join();
	return 0;
}
#endif // GRAPHICS
