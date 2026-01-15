#include "gl_raytrace.hpp"
#include "../core/utilities.hpp"
#include <glad/glad.h>

static unsigned int compile_shader_rt(const unsigned int type, const char* source) {
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

bool GLRaytraceRenderer::create_program() {
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
		"uniform sampler3D uPhi;\n"
		"uniform vec3 uCamPos;\n"
		"uniform mat3 uCamR;\n"
		"uniform float uFov;\n"
		"uniform vec3 uBoxMin;\n"
		"uniform vec3 uBoxMax;\n"
		"uniform vec2 uViewport;\n"
		"uniform vec3 uAbsorbColor;\n"
		"uniform float uAtten;\n"
		"uniform float uIso;\n"
		"uniform float uOpacity;\n"
		"uniform vec3 uLightDir;\n"
		"uniform vec3 uBgColor;\n"
		"uniform int uSteps;\n"
		"vec2 intersect_box(vec3 ro, vec3 rd, vec3 bmin, vec3 bmax) {\n"
		"    vec3 inv = 1.0 / rd;\n"
		"    vec3 t0 = (bmin - ro) * inv;\n"
		"    vec3 t1 = (bmax - ro) * inv;\n"
		"    vec3 tmin = min(t0, t1);\n"
		"    vec3 tmax = max(t0, t1);\n"
		"    float lo = max(max(tmin.x, tmin.y), tmin.z);\n"
		"    float hi = min(min(tmax.x, tmax.y), tmax.z);\n"
		"    return vec2(lo, hi);\n"
		"}\n"
		"vec3 calc_normal(vec3 p, vec3 bmin, vec3 bmax) {\n"
		"    vec3 e = vec3(1.0) / vec3(textureSize(uPhi, 0));\n"
		"    vec3 uv = (p - bmin) / (bmax - bmin);\n"
		"    float dx = texture(uPhi, uv + vec3(e.x, 0.0, 0.0)).r - texture(uPhi, uv - vec3(e.x, 0.0, 0.0)).r;\n"
		"    float dy = texture(uPhi, uv + vec3(0.0, e.y, 0.0)).r - texture(uPhi, uv - vec3(0.0, e.y, 0.0)).r;\n"
		"    float dz = texture(uPhi, uv + vec3(0.0, 0.0, e.z)).r - texture(uPhi, uv - vec3(0.0, 0.0, e.z)).r;\n"
		"    return normalize(vec3(dx, dy, dz));\n"
		"}\n"
		"void main() {\n"
		"    vec2 ndc = vUV * 2.0 - 1.0;\n"
		"    float aspect = uViewport.x / uViewport.y;\n"
		"    float tanf = tan(radians(uFov) * 0.5);\n"
		"    vec3 dir = normalize(uCamR * vec3(ndc.x * aspect * tanf, -ndc.y * tanf, -1.0));\n"
		"    vec2 hit = intersect_box(uCamPos, dir, uBoxMin, uBoxMax);\n"
		"    if(hit.x > hit.y) { FragColor = vec4(uBgColor, 1.0); return; }\n"
		"    float t0 = max(hit.x, 0.0);\n"
		"    float t1 = hit.y;\n"
		"    float dt = (t1 - t0) / float(uSteps);\n"
		"    float t = t0;\n"
		"    float last_phi = 0.0;\n"
		"    bool hit_surface = false;\n"
		"    vec3 hit_pos = vec3(0.0);\n"
		"    vec3 hit_normal = vec3(0.0);\n"
		"    for(int i=0; i<uSteps; i++) {\n"
		"        vec3 p = uCamPos + dir * t;\n"
		"        vec3 uv = (p - uBoxMin) / (uBoxMax - uBoxMin);\n"
		"        float phi = texture(uPhi, uv).r;\n"
		"        if(phi >= uIso && last_phi < uIso) {\n"
		"            hit_surface = true;\n"
		"            hit_pos = p;\n"
		"            hit_normal = calc_normal(p, uBoxMin, uBoxMax);\n"
		"            break;\n"
		"        }\n"
		"        last_phi = phi;\n"
		"        t += dt;\n"
		"    }\n"
		"    if(!hit_surface) { FragColor = vec4(uBgColor, 1.0); return; }\n"
		"    vec3 n = normalize(hit_normal);\n"
		"    float cosi = clamp(dot(-dir, n), 0.0, 1.0);\n"
		"    float n1 = 1.0;\n"
		"    float n2 = 1.333;\n"
		"    float f0 = (n1 - n2) / (n1 + n2);\n"
		"    f0 = f0 * f0;\n"
		"    float fresnel = f0 + (1.0 - f0) * pow(1.0 - cosi, 5.0);\n"
		"    float thickness = max(t1 - t0, 0.0);\n"
		"    float trans = exp(uAtten * thickness);\n"
		"    vec3 light_dir = normalize(uLightDir);\n"
		"    float diff = clamp(dot(n, light_dir), 0.0, 1.0);\n"
		"    vec3 refract_col = mix(vec3(0.02, 0.07, 0.12), uAbsorbColor, diff);\n"
		"    vec3 reflect_col = vec3(0.9) * pow(diff + 0.25, 2.0);\n"
		"    vec3 water_col = mix(refract_col, reflect_col, fresnel);\n"
		"    vec3 col = mix(uBgColor, water_col, 1.0 - trans);\n"
		"    col = mix(uBgColor, col, uOpacity);\n"
		"    FragColor = vec4(col, 1.0);\n"
		"}\n";

	const unsigned int vs = compile_shader_rt(GL_VERTEX_SHADER, vertex_source);
	const unsigned int fs = compile_shader_rt(GL_FRAGMENT_SHADER, fragment_source);
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

void GLRaytraceRenderer::create_quad() {
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

void GLRaytraceRenderer::create_volume_tex(const unsigned int nx, const unsigned int ny, const unsigned int nz) {
	vol_nx = nx;
	vol_ny = ny;
	vol_nz = nz;
	if(volume_tex == 0u) glGenTextures(1, &volume_tex);
	glBindTexture(GL_TEXTURE_3D, volume_tex);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, (int)nx, (int)ny, (int)nz, 0, GL_RED, GL_FLOAT, nullptr);
	glBindTexture(GL_TEXTURE_3D, 0);
	volume.resize((size_t)nx*(size_t)ny*(size_t)nz);
}

bool GLRaytraceRenderer::initialize(const unsigned int width, const unsigned int height) {
	if(!create_program()) return false;
	create_quad();
	glDisable(GL_DEPTH_TEST);
	glUseProgram(program);
	glUniform1i(glGetUniformLocation(program, "uPhi"), 0);
	glUseProgram(0);
	return true;
}

void GLRaytraceRenderer::update_volume(const float* phi, const unsigned int nx, const unsigned int ny, const unsigned int nz) {
	if(nx == 0u || ny == 0u || nz == 0u || phi == nullptr) return;
	if(nx != vol_nx || ny != vol_ny || nz != vol_nz || volume_tex == 0u) {
		create_volume_tex(nx, ny, nz);
	}
	const size_t count = (size_t)nx*(size_t)ny*(size_t)nz;
	for(size_t i = 0; i < count; i++) {
		float v = phi[i];
		if(v < 0.0f) v = 0.0f;
		if(v > 1.0f) v = 1.0f;
		volume[i] = v;
	}
	glBindTexture(GL_TEXTURE_3D, volume_tex);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, (int)nx, (int)ny, (int)nz, GL_RED, GL_FLOAT, volume.data());
	glBindTexture(GL_TEXTURE_3D, 0);
}

void GLRaytraceRenderer::render(const float3& cam_pos, const float3x3& cam_R, const float fov_deg, const float3& box_min, const float3& box_max, const unsigned int width, const unsigned int height) {
	glViewport(0, 0, (int)width, (int)height);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(program);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, volume_tex);
	glUniform3f(glGetUniformLocation(program, "uCamPos"), cam_pos.x, cam_pos.y, cam_pos.z);
	glUniformMatrix3fv(glGetUniformLocation(program, "uCamR"), 1, GL_TRUE, &cam_R.xx);
	glUniform1f(glGetUniformLocation(program, "uFov"), fov_deg);
	glUniform3f(glGetUniformLocation(program, "uBoxMin"), box_min.x, box_min.y, box_min.z);
	glUniform3f(glGetUniformLocation(program, "uBoxMax"), box_max.x, box_max.y, box_max.z);
	glUniform2f(glGetUniformLocation(program, "uViewport"), (float)width, (float)height);
	const float bg_r = (float)((GRAPHICS_BACKGROUND_COLOR>>16)&0xFFu) / 255.0f;
	const float bg_g = (float)((GRAPHICS_BACKGROUND_COLOR>>8)&0xFFu) / 255.0f;
	const float bg_b = (float)(GRAPHICS_BACKGROUND_COLOR&0xFFu) / 255.0f;
	const float ab_r = (float)((GRAPHICS_RAYTRACING_COLOR>>16)&0xFFu) / 255.0f;
	const float ab_g = (float)((GRAPHICS_RAYTRACING_COLOR>>8)&0xFFu) / 255.0f;
	const float ab_b = (float)(GRAPHICS_RAYTRACING_COLOR&0xFFu) / 255.0f;
	const float max_dim = (float)max(max(vol_nx, vol_ny), vol_nz);
	const float atten = (float)ln(clamp(GRAPHICS_RAYTRACING_TRANSMITTANCE, 1.0e-9f, 1.0f)) / max_dim;
	glUniform3f(glGetUniformLocation(program, "uAbsorbColor"), ab_r, ab_g, ab_b);
	glUniform1f(glGetUniformLocation(program, "uAtten"), atten);
	glUniform1f(glGetUniformLocation(program, "uIso"), 0.5f);
	glUniform1f(glGetUniformLocation(program, "uOpacity"), 0.85f);
	glUniform3f(glGetUniformLocation(program, "uLightDir"), 0.3f, 0.7f, 0.6f);
	glUniform3f(glGetUniformLocation(program, "uBgColor"), bg_r, bg_g, bg_b);
	glUniform1i(glGetUniformLocation(program, "uSteps"), 512);
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_3D, 0);
	glUseProgram(0);
}

void GLRaytraceRenderer::shutdown() {
	if(vbo != 0u) glDeleteBuffers(1, &vbo);
	if(vao != 0u) glDeleteVertexArrays(1, &vao);
	if(volume_tex != 0u) glDeleteTextures(1, &volume_tex);
	if(program != 0u) glDeleteProgram(program);
	vbo = 0u;
	vao = 0u;
	volume_tex = 0u;
	program = 0u;
}
