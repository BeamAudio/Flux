#include "glad.h"

#ifndef NULL
#define NULL 0
#endif

PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = NULL;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray = NULL;
PFNGLGENBUFFERSPROC glGenBuffers = NULL;
PFNGLBINDBUFFERPROC glBindBuffer = NULL;
PFNGLBUFFERDATAPROC glBufferData = NULL;
PFNGLBUFFERSUBDATAPROC glBufferSubData = NULL;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = NULL;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = NULL;
PFNGLCREATESHADERPROC glCreateShader = NULL;
PFNGLSHADERSOURCEPROC glShaderSource = NULL;
PFNGLCOMPILESHADERPROC glCompileShader = NULL;
PFNGLGETSHADERIVPROC glGetShaderiv = NULL;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = NULL;
PFNGLCREATEPROGRAMPROC glCreateProgram = NULL;
PFNGLATTACHSHADERPROC glAttachShader = NULL;
PFNGLLINKPROGRAMPROC glLinkProgram = NULL;
PFNGLGETPROGRAMIVPROC glGetProgramiv = NULL;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = NULL;
PFNGLUSEPROGRAMPROC glUseProgram = NULL;
PFNGLDELETESHADERPROC glDeleteShader = NULL;
PFNGLDELETEPROGRAMPROC glDeleteProgram = NULL;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = NULL;
PFNGLUNIFORM1IPROC glUniform1i = NULL;
PFNGLUNIFORM1FPROC glUniform1f = NULL;
PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv = NULL;
PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays = NULL;
PFNGLDELETEBUFFERSPROC glDeleteBuffers = NULL;

int gladLoadGLLoader(void* (*load)(const char* name)) {
    glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)load("glGenVertexArrays");
    glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)load("glBindVertexArray");
    glGenBuffers = (PFNGLGENBUFFERSPROC)load("glGenBuffers");
    glBindBuffer = (PFNGLBINDBUFFERPROC)load("glBindBuffer");
    glBufferData = (PFNGLBUFFERDATAPROC)load("glBufferData");
    glBufferSubData = (PFNGLBUFFERSUBDATAPROC)load("glBufferSubData");
    glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)load("glEnableVertexAttribArray");
    glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)load("glVertexAttribPointer");
    glCreateShader = (PFNGLCREATESHADERPROC)load("glCreateShader");
    glShaderSource = (PFNGLSHADERSOURCEPROC)load("glShaderSource");
    glCompileShader = (PFNGLCOMPILESHADERPROC)load("glCompileShader");
    glGetShaderiv = (PFNGLGETSHADERIVPROC)load("glGetShaderiv");
    glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)load("glGetShaderInfoLog");
    glCreateProgram = (PFNGLCREATEPROGRAMPROC)load("glCreateProgram");
    glAttachShader = (PFNGLATTACHSHADERPROC)load("glAttachShader");
    glLinkProgram = (PFNGLLINKPROGRAMPROC)load("glLinkProgram");
    glGetProgramiv = (PFNGLGETPROGRAMIVPROC)load("glGetProgramiv");
    glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)load("glGetProgramInfoLog");
    glUseProgram = (PFNGLUSEPROGRAMPROC)load("glUseProgram");
    glDeleteShader = (PFNGLDELETESHADERPROC)load("glDeleteShader");
    glDeleteProgram = (PFNGLDELETEPROGRAMPROC)load("glDeleteProgram");
    glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)load("glGetUniformLocation");
    glUniform1i = (PFNGLUNIFORM1IPROC)load("glUniform1i");
    glUniform1f = (PFNGLUNIFORM1FPROC)load("glUniform1f");
    glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)load("glUniformMatrix4fv");
    glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)load("glDeleteVertexArrays");
    glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)load("glDeleteBuffers");

    return 1;
}
