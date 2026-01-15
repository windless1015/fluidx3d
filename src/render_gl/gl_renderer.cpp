#include "gl_renderer.hpp"
#include "../core/utilities.hpp"
#include <glad/glad.h>

static unsigned int compile_shader(const unsigned int type, const char* source) {
	const unsigned int shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, nullptr);
	glCompileShader(shader);
	int success = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if(!success) {
		char log[1024];
		glGetShaderInfoLog(shader, 1024, nullptr, log);
		print_error("OpenGL shader compile failed: "+string(log));
	}
	return shader;
}

bool GLRenderer::create_program() {
	const char* vertex_source =
		"#version 330 core\n"
		"layout (location = 0) in vec2 aPos;\n"
		"layout (location = 1) in vec2 aUV;\n"
		"out vec2 vUV;\n"
		"void main() {\n"
		"    vUV = aUV;\n"
		"    gl_Position = vec4(aPos, 0.0, 1.0);\n"
		"}\n";
	const char* fragment_source =
		"#version 330 core\n"
		"in vec2 vUV;\n"
		"out vec4 FragColor;\n"
		"uniform sampler2D uTex;\n"
		"void main() {\n"
		"    FragColor = texture(uTex, vUV);\n"
		"}\n";
	const unsigned int vs = compile_shader(GL_VERTEX_SHADER, vertex_source);
	const unsigned int fs = compile_shader(GL_FRAGMENT_SHADER, fragment_source);
	program = glCreateProgram();
	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);
	int success = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if(!success) {
		char log[1024];
		glGetProgramInfoLog(program, 1024, nullptr, log);
		print_error("OpenGL program link failed: "+string(log));
	}
	glDeleteShader(vs);
	glDeleteShader(fs);
	return success != 0;
}

void GLRenderer::create_quad() {
	const float quad[] = {
		-1.0f, -1.0f, 0.0f, 1.0f,
		 1.0f, -1.0f, 1.0f, 1.0f,
		 1.0f,  1.0f, 1.0f, 0.0f,
		-1.0f, -1.0f, 0.0f, 1.0f,
		 1.0f,  1.0f, 1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f, 0.0f
	};
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float)));
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void GLRenderer::create_texture(const unsigned int width, const unsigned int height) {
	tex_width = width;
	tex_height = height;
	if(texture == 0u) glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (int)width, (int)height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glBindTexture(GL_TEXTURE_2D, 0);
	rgba.resize((size_t)width*(size_t)height*4u);
}

bool GLRenderer::initialize(const unsigned int width, const unsigned int height) {
	if(!create_program()) return false;
	create_quad();
	create_texture(width, height);
	glDisable(GL_DEPTH_TEST);
	return true;
}

void GLRenderer::upload_frame(const int* bitmap, const unsigned int width, const unsigned int height) {
	if(width != tex_width || height != tex_height) {
		create_texture(width, height);
	}
	const size_t pixels = (size_t)width*(size_t)height;
	if(rgba.size() < pixels*4u) rgba.resize(pixels*4u);
	for(size_t i = 0; i < pixels; i++) {
		const unsigned int c = (unsigned int)bitmap[i];
		rgba[i*4u+0u] = (unsigned char)((c>>16) & 0xFFu);
		rgba[i*4u+1u] = (unsigned char)((c>>8) & 0xFFu);
		rgba[i*4u+2u] = (unsigned char)(c & 0xFFu);
		rgba[i*4u+3u] = 0xFFu;
	}
	glBindTexture(GL_TEXTURE_2D, texture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (int)width, (int)height, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
	glBindTexture(GL_TEXTURE_2D, 0);
}

void GLRenderer::render() {
	glViewport(0, 0, (int)tex_width, (int)tex_height);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(program);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);
}

void GLRenderer::shutdown() {
	if(vbo != 0u) glDeleteBuffers(1, &vbo);
	if(vao != 0u) glDeleteVertexArrays(1, &vao);
	if(texture != 0u) glDeleteTextures(1, &texture);
	if(program != 0u) glDeleteProgram(program);
	vbo = 0u;
	vao = 0u;
	texture = 0u;
	program = 0u;
}
