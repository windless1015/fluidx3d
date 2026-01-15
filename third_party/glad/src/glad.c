#include "../include/glad/glad.h"

#ifdef _WIN32
#define GLAD_API_CALL APIENTRY
#else
#define GLAD_API_CALL
#endif

#define LOAD_GL_FUNC(name)                           \
	do {                                              \
		glad_##name = (PFN_##name)load(#name);         \
		if(glad_##name == NULL) return 0;              \
	} while(0)

typedef void (GLAD_API_CALL *PFN_glActiveTexture)(GLenum);
typedef void (GLAD_API_CALL *PFN_glAttachShader)(GLuint, GLuint);
typedef void (GLAD_API_CALL *PFN_glBindBuffer)(GLenum, GLuint);
typedef void (GLAD_API_CALL *PFN_glBindTexture)(GLenum, GLuint);
typedef void (GLAD_API_CALL *PFN_glBindVertexArray)(GLuint);
typedef void (GLAD_API_CALL *PFN_glBufferData)(GLenum, GLsizeiptr, const void*, GLenum);
typedef void (GLAD_API_CALL *PFN_glClear)(GLbitfield);
typedef void (GLAD_API_CALL *PFN_glClearColor)(GLclampf, GLclampf, GLclampf, GLclampf);
typedef void (GLAD_API_CALL *PFN_glCompileShader)(GLuint);
typedef GLuint (GLAD_API_CALL *PFN_glCreateProgram)(void);
typedef GLuint (GLAD_API_CALL *PFN_glCreateShader)(GLenum);
typedef void (GLAD_API_CALL *PFN_glDeleteBuffers)(GLsizei, const GLuint*);
typedef void (GLAD_API_CALL *PFN_glDeleteProgram)(GLuint);
typedef void (GLAD_API_CALL *PFN_glDeleteShader)(GLuint);
typedef void (GLAD_API_CALL *PFN_glDeleteTextures)(GLsizei, const GLuint*);
typedef void (GLAD_API_CALL *PFN_glDeleteVertexArrays)(GLsizei, const GLuint*);
typedef void (GLAD_API_CALL *PFN_glDrawArrays)(GLenum, GLint, GLsizei);
typedef void (GLAD_API_CALL *PFN_glEnableVertexAttribArray)(GLuint);
typedef void (GLAD_API_CALL *PFN_glGenBuffers)(GLsizei, GLuint*);
typedef void (GLAD_API_CALL *PFN_glGenTextures)(GLsizei, GLuint*);
typedef void (GLAD_API_CALL *PFN_glGenVertexArrays)(GLsizei, GLuint*);
typedef void (GLAD_API_CALL *PFN_glGetProgramInfoLog)(GLuint, GLsizei, GLsizei*, char*);
typedef void (GLAD_API_CALL *PFN_glGetProgramiv)(GLuint, GLenum, GLint*);
typedef void (GLAD_API_CALL *PFN_glGetShaderInfoLog)(GLuint, GLsizei, GLsizei*, char*);
typedef void (GLAD_API_CALL *PFN_glGetShaderiv)(GLuint, GLenum, GLint*);
typedef GLint (GLAD_API_CALL *PFN_glGetUniformLocation)(GLuint, const char*);
typedef void (GLAD_API_CALL *PFN_glLinkProgram)(GLuint);
typedef void (GLAD_API_CALL *PFN_glPixelStorei)(GLenum, GLint);
typedef void (GLAD_API_CALL *PFN_glShaderSource)(GLuint, GLsizei, const char* const*, const GLint*);
typedef void (GLAD_API_CALL *PFN_glTexImage2D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*);
typedef void (GLAD_API_CALL *PFN_glTexImage3D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*);
typedef void (GLAD_API_CALL *PFN_glTexParameteri)(GLenum, GLenum, GLint);
typedef void (GLAD_API_CALL *PFN_glTexSubImage2D)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const void*);
typedef void (GLAD_API_CALL *PFN_glTexSubImage3D)(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const void*);
typedef void (GLAD_API_CALL *PFN_glUseProgram)(GLuint);
typedef void (GLAD_API_CALL *PFN_glUniform1i)(GLint, GLint);
typedef void (GLAD_API_CALL *PFN_glUniform1f)(GLint, GLfloat);
typedef void (GLAD_API_CALL *PFN_glUniform2f)(GLint, GLfloat, GLfloat);
typedef void (GLAD_API_CALL *PFN_glUniform3f)(GLint, GLfloat, GLfloat, GLfloat);
typedef void (GLAD_API_CALL *PFN_glUniformMatrix3fv)(GLint, GLsizei, GLboolean, const GLfloat*);
typedef void (GLAD_API_CALL *PFN_glVertexAttribPointer)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);
typedef void (GLAD_API_CALL *PFN_glViewport)(GLint, GLint, GLsizei, GLsizei);
typedef void (GLAD_API_CALL *PFN_glDisable)(GLenum);

static PFN_glActiveTexture glad_glActiveTexture;
static PFN_glAttachShader glad_glAttachShader;
static PFN_glBindBuffer glad_glBindBuffer;
static PFN_glBindTexture glad_glBindTexture;
static PFN_glBindVertexArray glad_glBindVertexArray;
static PFN_glBufferData glad_glBufferData;
static PFN_glClear glad_glClear;
static PFN_glClearColor glad_glClearColor;
static PFN_glCompileShader glad_glCompileShader;
static PFN_glCreateProgram glad_glCreateProgram;
static PFN_glCreateShader glad_glCreateShader;
static PFN_glDeleteBuffers glad_glDeleteBuffers;
static PFN_glDeleteProgram glad_glDeleteProgram;
static PFN_glDeleteShader glad_glDeleteShader;
static PFN_glDeleteTextures glad_glDeleteTextures;
static PFN_glDeleteVertexArrays glad_glDeleteVertexArrays;
static PFN_glDrawArrays glad_glDrawArrays;
static PFN_glEnableVertexAttribArray glad_glEnableVertexAttribArray;
static PFN_glGenBuffers glad_glGenBuffers;
static PFN_glGenTextures glad_glGenTextures;
static PFN_glGenVertexArrays glad_glGenVertexArrays;
static PFN_glGetProgramInfoLog glad_glGetProgramInfoLog;
static PFN_glGetProgramiv glad_glGetProgramiv;
static PFN_glGetShaderInfoLog glad_glGetShaderInfoLog;
static PFN_glGetShaderiv glad_glGetShaderiv;
static PFN_glGetUniformLocation glad_glGetUniformLocation;
static PFN_glLinkProgram glad_glLinkProgram;
static PFN_glPixelStorei glad_glPixelStorei;
static PFN_glShaderSource glad_glShaderSource;
static PFN_glTexImage2D glad_glTexImage2D;
static PFN_glTexImage3D glad_glTexImage3D;
static PFN_glTexParameteri glad_glTexParameteri;
static PFN_glTexSubImage2D glad_glTexSubImage2D;
static PFN_glTexSubImage3D glad_glTexSubImage3D;
static PFN_glUseProgram glad_glUseProgram;
static PFN_glUniform1i glad_glUniform1i;
static PFN_glUniform1f glad_glUniform1f;
static PFN_glUniform2f glad_glUniform2f;
static PFN_glUniform3f glad_glUniform3f;
static PFN_glUniformMatrix3fv glad_glUniformMatrix3fv;
static PFN_glVertexAttribPointer glad_glVertexAttribPointer;
static PFN_glViewport glad_glViewport;
static PFN_glDisable glad_glDisable;

int gladLoadGLLoader(GLADloadproc load) {
	if(load == NULL) return 0;
	LOAD_GL_FUNC(glActiveTexture);
	LOAD_GL_FUNC(glAttachShader);
	LOAD_GL_FUNC(glBindBuffer);
	LOAD_GL_FUNC(glBindTexture);
	LOAD_GL_FUNC(glBindVertexArray);
	LOAD_GL_FUNC(glBufferData);
	LOAD_GL_FUNC(glClear);
	LOAD_GL_FUNC(glClearColor);
	LOAD_GL_FUNC(glCompileShader);
	LOAD_GL_FUNC(glCreateProgram);
	LOAD_GL_FUNC(glCreateShader);
	LOAD_GL_FUNC(glDeleteBuffers);
	LOAD_GL_FUNC(glDeleteProgram);
	LOAD_GL_FUNC(glDeleteShader);
	LOAD_GL_FUNC(glDeleteTextures);
	LOAD_GL_FUNC(glDeleteVertexArrays);
	LOAD_GL_FUNC(glDrawArrays);
	LOAD_GL_FUNC(glEnableVertexAttribArray);
	LOAD_GL_FUNC(glGenBuffers);
	LOAD_GL_FUNC(glGenTextures);
	LOAD_GL_FUNC(glGenVertexArrays);
	LOAD_GL_FUNC(glGetProgramInfoLog);
	LOAD_GL_FUNC(glGetProgramiv);
	LOAD_GL_FUNC(glGetShaderInfoLog);
	LOAD_GL_FUNC(glGetShaderiv);
	LOAD_GL_FUNC(glGetUniformLocation);
	LOAD_GL_FUNC(glLinkProgram);
	LOAD_GL_FUNC(glPixelStorei);
	LOAD_GL_FUNC(glShaderSource);
	LOAD_GL_FUNC(glTexImage2D);
	LOAD_GL_FUNC(glTexImage3D);
	LOAD_GL_FUNC(glTexParameteri);
	LOAD_GL_FUNC(glTexSubImage2D);
	LOAD_GL_FUNC(glTexSubImage3D);
	LOAD_GL_FUNC(glUseProgram);
	LOAD_GL_FUNC(glUniform1i);
	LOAD_GL_FUNC(glUniform1f);
	LOAD_GL_FUNC(glUniform2f);
	LOAD_GL_FUNC(glUniform3f);
	LOAD_GL_FUNC(glUniformMatrix3fv);
	LOAD_GL_FUNC(glVertexAttribPointer);
	LOAD_GL_FUNC(glViewport);
	LOAD_GL_FUNC(glDisable);
	return 1;
}

void glActiveTexture(GLenum texture) { glad_glActiveTexture(texture); }
void glAttachShader(GLuint program, GLuint shader) { glad_glAttachShader(program, shader); }
void glBindBuffer(GLenum target, GLuint buffer) { glad_glBindBuffer(target, buffer); }
void glBindTexture(GLenum target, GLuint texture) { glad_glBindTexture(target, texture); }
void glBindVertexArray(GLuint array) { glad_glBindVertexArray(array); }
void glBufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage) { glad_glBufferData(target, size, data, usage); }
void glClear(GLbitfield mask) { glad_glClear(mask); }
void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) { glad_glClearColor(red, green, blue, alpha); }
void glCompileShader(GLuint shader) { glad_glCompileShader(shader); }
GLuint glCreateProgram(void) { return glad_glCreateProgram(); }
GLuint glCreateShader(GLenum type) { return glad_glCreateShader(type); }
void glDeleteBuffers(GLsizei n, const GLuint* buffers) { glad_glDeleteBuffers(n, buffers); }
void glDeleteProgram(GLuint program) { glad_glDeleteProgram(program); }
void glDeleteShader(GLuint shader) { glad_glDeleteShader(shader); }
void glDeleteTextures(GLsizei n, const GLuint* textures) { glad_glDeleteTextures(n, textures); }
void glDeleteVertexArrays(GLsizei n, const GLuint* arrays) { glad_glDeleteVertexArrays(n, arrays); }
void glDrawArrays(GLenum mode, GLint first, GLsizei count) { glad_glDrawArrays(mode, first, count); }
void glEnableVertexAttribArray(GLuint index) { glad_glEnableVertexAttribArray(index); }
void glGenBuffers(GLsizei n, GLuint* buffers) { glad_glGenBuffers(n, buffers); }
void glGenTextures(GLsizei n, GLuint* textures) { glad_glGenTextures(n, textures); }
void glGenVertexArrays(GLsizei n, GLuint* arrays) { glad_glGenVertexArrays(n, arrays); }
void glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei* length, char* infoLog) { glad_glGetProgramInfoLog(program, bufSize, length, infoLog); }
void glGetProgramiv(GLuint program, GLenum pname, GLint* params) { glad_glGetProgramiv(program, pname, params); }
void glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei* length, char* infoLog) { glad_glGetShaderInfoLog(shader, bufSize, length, infoLog); }
void glGetShaderiv(GLuint shader, GLenum pname, GLint* params) { glad_glGetShaderiv(shader, pname, params); }
GLint glGetUniformLocation(GLuint program, const char* name) { return glad_glGetUniformLocation(program, name); }
void glLinkProgram(GLuint program) { glad_glLinkProgram(program); }
void glPixelStorei(GLenum pname, GLint param) { glad_glPixelStorei(pname, param); }
void glShaderSource(GLuint shader, GLsizei count, const char* const* string, const GLint* length) { glad_glShaderSource(shader, count, string, length); }
void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels) { glad_glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels); }
void glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void* pixels) { glad_glTexImage3D(target, level, internalformat, width, height, depth, border, format, type, pixels); }
void glTexParameteri(GLenum target, GLenum pname, GLint param) { glad_glTexParameteri(target, pname, param); }
void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels) { glad_glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels); }
void glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* pixels) { glad_glTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels); }
void glUseProgram(GLuint program) { glad_glUseProgram(program); }
void glUniform1i(GLint location, GLint v0) { glad_glUniform1i(location, v0); }
void glUniform1f(GLint location, GLfloat v0) { glad_glUniform1f(location, v0); }
void glUniform2f(GLint location, GLfloat v0, GLfloat v1) { glad_glUniform2f(location, v0, v1); }
void glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) { glad_glUniform3f(location, v0, v1, v2); }
void glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) { glad_glUniformMatrix3fv(location, count, transpose, value); }
void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer) { glad_glVertexAttribPointer(index, size, type, normalized, stride, pointer); }
void glViewport(GLint x, GLint y, GLsizei width, GLsizei height) { glad_glViewport(x, y, width, height); }
void glDisable(GLenum cap) { glad_glDisable(cap); }
