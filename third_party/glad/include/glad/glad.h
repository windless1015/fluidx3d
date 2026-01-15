#pragma once

#include <stddef.h>
#include "../KHR/khrplatform.h"

#if defined(_WIN32)
#ifndef APIENTRY
#define APIENTRY KHRONOS_APIENTRY
#endif
#endif

#if defined(__cplusplus)
extern "C" {
#endif

typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef void GLvoid;
typedef signed char GLbyte;
typedef short GLshort;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned int GLuint;
typedef float GLfloat;
typedef float GLclampf;
typedef double GLdouble;
typedef double GLclampd;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;

#define GL_FALSE 0
#define GL_TRUE 1

#define GL_ARRAY_BUFFER 0x8892
#define GL_STATIC_DRAW 0x88E4
#define GL_FLOAT 0x1406
#define GL_TRIANGLES 0x0004
#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_TEXTURE_2D 0x0DE1
#define GL_TEXTURE0 0x84C0
#define GL_RGBA 0x1908
#define GL_RGBA8 0x8058
#define GL_UNSIGNED_BYTE 0x1401
#define GL_VERTEX_SHADER 0x8B31
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_TEXTURE_WRAP_R 0x8072
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_LINEAR 0x2601
#define GL_UNPACK_ALIGNMENT 0x0CF5
#define GL_DEPTH_TEST 0x0B71
#define GL_TEXTURE_3D 0x806F
#define GL_RED 0x1903
#define GL_R32F 0x822E

typedef void* (*GLADloadproc)(const char* name);
int gladLoadGLLoader(GLADloadproc load);

void glActiveTexture(GLenum texture);
void glAttachShader(GLuint program, GLuint shader);
void glBindBuffer(GLenum target, GLuint buffer);
void glBindTexture(GLenum target, GLuint texture);
void glBindVertexArray(GLuint array);
void glBufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage);
void glClear(GLbitfield mask);
void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void glCompileShader(GLuint shader);
GLuint glCreateProgram(void);
GLuint glCreateShader(GLenum type);
void glDeleteBuffers(GLsizei n, const GLuint* buffers);
void glDeleteProgram(GLuint program);
void glDeleteShader(GLuint shader);
void glDeleteTextures(GLsizei n, const GLuint* textures);
void glDeleteVertexArrays(GLsizei n, const GLuint* arrays);
void glDrawArrays(GLenum mode, GLint first, GLsizei count);
void glEnableVertexAttribArray(GLuint index);
void glGenBuffers(GLsizei n, GLuint* buffers);
void glGenTextures(GLsizei n, GLuint* textures);
void glGenVertexArrays(GLsizei n, GLuint* arrays);
void glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei* length, char* infoLog);
void glGetProgramiv(GLuint program, GLenum pname, GLint* params);
void glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei* length, char* infoLog);
void glGetShaderiv(GLuint shader, GLenum pname, GLint* params);
GLint glGetUniformLocation(GLuint program, const char* name);
void glLinkProgram(GLuint program);
void glPixelStorei(GLenum pname, GLint param);
void glShaderSource(GLuint shader, GLsizei count, const char* const* string, const GLint* length);
void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels);
void glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void* pixels);
void glTexParameteri(GLenum target, GLenum pname, GLint param);
void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels);
void glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* pixels);
void glUseProgram(GLuint program);
void glUniform1i(GLint location, GLint v0);
void glUniform1f(GLint location, GLfloat v0);
void glUniform2f(GLint location, GLfloat v0, GLfloat v1);
void glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
void glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);
void glViewport(GLint x, GLint y, GLsizei width, GLsizei height);
void glDisable(GLenum cap);

#if defined(__cplusplus)
}
#endif
