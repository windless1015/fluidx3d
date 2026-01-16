#pragma once

#include "../core/defines.hpp"
#include "../core/utilities.hpp"
#include <vector>

struct GLRaytraceRenderer {
	bool initialize(const unsigned int width, const unsigned int height);
	void update_volume(const float* phi, const unsigned int nx, const unsigned int ny, const unsigned int nz);
	void render(const float3& cam_pos, const float3x3& cam_R, const float fov_deg, const float3& box_min, const float3& box_max, const unsigned int width, const unsigned int height);
	void shutdown();

private:
	unsigned int program = 0u;
	unsigned int vao = 0u;
	unsigned int vbo = 0u;
	unsigned int volume_tex = 0u;
	unsigned int skybox_tex = 0u;
	unsigned int skybox_w = 0u;
	unsigned int skybox_h = 0u;
	unsigned int vol_nx = 0u;
	unsigned int vol_ny = 0u;
	unsigned int vol_nz = 0u;
	std::vector<float> volume;
	std::vector<unsigned char> skybox_rgba;
	bool create_program();
	void create_quad();
	void create_volume_tex(const unsigned int nx, const unsigned int ny, const unsigned int nz);
	bool load_skybox();
};
