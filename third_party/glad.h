#ifndef GLAD_H
#define GLAD_H

#include <SDL3/SDL.h>
#include <stddef.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <GL/gl.h>

// Missing types
typedef char GLchar;
typedef ptrdiff_t GLintptr;
typedef size_t GLsizeiptr;

// Constants
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER                   0x8892
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_STATIC_DRAW                    0x88E4
#define GL_DYNAMIC_DRAW                    0x88E8
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82
#define GL_INFO_LOG_LENGTH                0x8B84
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void (APIENTRY * PFNGLGENVERTEXARRAYSPROC) (GLsizei n, GLuint *arrays);
typedef void (APIENTRY * PFNGLBINDVERTEXARRAYPROC) (GLuint array);
typedef void (APIENTRY * PFNGLGENBUFFERSPROC) (GLsizei n, GLuint *buffers);
typedef void (APIENTRY * PFNGLBINDBUFFERPROC) (GLenum target, GLuint buffer);
typedef void (APIENTRY * PFNGLBUFFERDATAPROC) (GLenum target, GLsizeiptr size, const void *data, GLenum usage);
typedef void (APIENTRY * PFNGLBUFFERSUBDATAPROC) (GLenum target, GLintptr offset, GLsizeiptr size, const void *data);
typedef void (APIENTRY * PFNGLENABLEVERTEXATTRIBARRAYPROC) (GLuint index);
typedef void (APIENTRY * PFNGLVERTEXATTRIBPOINTERPROC) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
typedef GLuint (APIENTRY * PFNGLCREATESHADERPROC) (GLenum type);
typedef void (APIENTRY * PFNGLSHADERSOURCEPROC) (GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
typedef void (APIENTRY * PFNGLCOMPILESHADERPROC) (GLuint shader);
typedef void (APIENTRY * PFNGLGETSHADERIVPROC) (GLuint shader, GLenum pname, GLint *params);
typedef void (APIENTRY * PFNGLGETSHADERINFOLOGPROC) (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef GLuint (APIENTRY * PFNGLCREATEPROGRAMPROC) (void);
typedef void (APIENTRY * PFNGLATTACHSHADERPROC) (GLuint program, GLuint shader);
typedef void (APIENTRY * PFNGLLINKPROGRAMPROC) (GLuint program);
typedef void (APIENTRY * PFNGLGETPROGRAMIVPROC) (GLuint program, GLenum pname, GLint *params);
typedef void (APIENTRY * PFNGLGETPROGRAMINFOLOGPROC) (GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (APIENTRY * PFNGLUSEPROGRAMPROC) (GLuint program);
typedef void (APIENTRY * PFNGLDELETESHADERPROC) (GLuint shader);
typedef void (APIENTRY * PFNGLDELETEPROGRAMPROC) (GLuint program);
typedef GLint (APIENTRY * PFNGLGETUNIFORMLOCATIONPROC) (GLuint program, const GLchar *name);
typedef void (APIENTRY * PFNGLUNIFORM1IPROC) (GLint location, GLint v0);
typedef void (APIENTRY * PFNGLUNIFORM1FPROC) (GLint location, GLfloat v0);
typedef void (APIENTRY * PFNGLUNIFORMMATRIX4FVPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRY * PFNGLDELETEVERTEXARRAYSPROC) (GLsizei n, const GLuint *arrays);
typedef void (APIENTRY * PFNGLDELETEBUFFERSPROC) (GLsizei n, const GLuint *buffers);
typedef void (APIENTRY * PFNGLACTIVETEXTUREPROC) (GLenum texture);

extern PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
extern PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
extern PFNGLGENBUFFERSPROC glGenBuffers;
extern PFNGLBINDBUFFERPROC glBindBuffer;
extern PFNGLBUFFERDATAPROC glBufferData;
extern PFNGLBUFFERSUBDATAPROC glBufferSubData;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
extern PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
extern PFNGLCREATESHADERPROC glCreateShader;
extern PFNGLSHADERSOURCEPROC glShaderSource;
extern PFNGLCOMPILESHADERPROC glCompileShader;
extern PFNGLGETSHADERIVPROC glGetShaderiv;
extern PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
extern PFNGLCREATEPROGRAMPROC glCreateProgram;
extern PFNGLATTACHSHADERPROC glAttachShader;
extern PFNGLLINKPROGRAMPROC glLinkProgram;
extern PFNGLGETPROGRAMIVPROC glGetProgramiv;
extern PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
extern PFNGLUSEPROGRAMPROC glUseProgram;
extern PFNGLDELETESHADERPROC glDeleteShader;
extern PFNGLDELETEPROGRAMPROC glDeleteProgram;
extern PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
extern PFNGLUNIFORM1IPROC glUniform1i;
extern PFNGLUNIFORM1FPROC glUniform1f;
extern PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
extern PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;
extern PFNGLDELETEBUFFERSPROC glDeleteBuffers;
extern PFNGLACTIVETEXTUREPROC glActiveTexture;

int gladLoadGLLoader(void* (*load)(const char* name));

#ifdef __cplusplus
}
#endif

#endif