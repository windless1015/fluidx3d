#pragma once

#include "../core/defines.hpp"
#include <vector>

struct GLRenderer {
	bool initialize(const unsigned int width, const unsigned int height);
	void upload_frame(const int* bitmap, const unsigned int width, const unsigned int height);
	void render();
	void shutdown();

private:
	unsigned int program = 0u;
	unsigned int vao = 0u;
	unsigned int vbo = 0u;
	unsigned int texture = 0u;
	unsigned int tex_width = 0u;
	unsigned int tex_height = 0u;
	std::vector<unsigned char> rgba;
	bool create_program();
	void create_quad();
	void create_texture(const unsigned int width, const unsigned int height);
};
