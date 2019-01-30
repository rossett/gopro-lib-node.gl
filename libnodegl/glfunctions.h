/* DO NOT EDIT - This file is autogenerated */

#ifndef NGL_GLFUNCS_H
#define NGL_GLFUNCS_H

#include "glincludes.h"

#ifdef _WIN32
#define NGLI_GL_APIENTRY WINAPI
#else
#define NGLI_GL_APIENTRY
#endif

struct glfunctions {
    NGLI_GL_APIENTRY void (*ActiveTexture)(GLenum texture);
    NGLI_GL_APIENTRY void (*AttachShader)(GLuint program, GLuint shader);
    NGLI_GL_APIENTRY void (*BeginQuery)(GLenum target, GLuint id);
    NGLI_GL_APIENTRY void (*BeginQueryEXT)(GLenum target, GLuint id);
    NGLI_GL_APIENTRY void (*BindAttribLocation)(GLuint program, GLuint index, const GLchar * name);
    NGLI_GL_APIENTRY void (*BindBuffer)(GLenum target, GLuint buffer);
    NGLI_GL_APIENTRY void (*BindBufferBase)(GLenum target, GLuint index, GLuint buffer);
    NGLI_GL_APIENTRY void (*BindBufferRange)(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);
    NGLI_GL_APIENTRY void (*BindFramebuffer)(GLenum target, GLuint framebuffer);
    NGLI_GL_APIENTRY void (*BindImageTexture)(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format);
    NGLI_GL_APIENTRY void (*BindRenderbuffer)(GLenum target, GLuint renderbuffer);
    NGLI_GL_APIENTRY void (*BindTexture)(GLenum target, GLuint texture);
    NGLI_GL_APIENTRY void (*BindVertexArray)(GLuint array);
    NGLI_GL_APIENTRY void (*BlendColor)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
    NGLI_GL_APIENTRY void (*BlendEquation)(GLenum mode);
    NGLI_GL_APIENTRY void (*BlendEquationSeparate)(GLenum modeRGB, GLenum modeAlpha);
    NGLI_GL_APIENTRY void (*BlendFunc)(GLenum sfactor, GLenum dfactor);
    NGLI_GL_APIENTRY void (*BlendFuncSeparate)(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);
    NGLI_GL_APIENTRY void (*BlitFramebuffer)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
    NGLI_GL_APIENTRY void (*BufferData)(GLenum target, GLsizeiptr size, const void * data, GLenum usage);
    NGLI_GL_APIENTRY void (*BufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, const void * data);
    NGLI_GL_APIENTRY GLenum (*CheckFramebufferStatus)(GLenum target);
    NGLI_GL_APIENTRY void (*Clear)(GLbitfield mask);
    NGLI_GL_APIENTRY void (*ClearColor)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
    NGLI_GL_APIENTRY GLenum (*ClientWaitSync)(GLsync sync, GLbitfield flags, GLuint64 timeout);
    NGLI_GL_APIENTRY void (*ColorMask)(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
    NGLI_GL_APIENTRY void (*CompileShader)(GLuint shader);
    NGLI_GL_APIENTRY GLuint (*CreateProgram)();
    NGLI_GL_APIENTRY GLuint (*CreateShader)(GLenum type);
    NGLI_GL_APIENTRY void (*CullFace)(GLenum mode);
    NGLI_GL_APIENTRY void (*DeleteBuffers)(GLsizei n, const GLuint * buffers);
    NGLI_GL_APIENTRY void (*DeleteFramebuffers)(GLsizei n, const GLuint * framebuffers);
    NGLI_GL_APIENTRY void (*DeleteProgram)(GLuint program);
    NGLI_GL_APIENTRY void (*DeleteQueries)(GLsizei n, const GLuint * ids);
    NGLI_GL_APIENTRY void (*DeleteQueriesEXT)(GLsizei n, const GLuint * ids);
    NGLI_GL_APIENTRY void (*DeleteRenderbuffers)(GLsizei n, const GLuint * renderbuffers);
    NGLI_GL_APIENTRY void (*DeleteShader)(GLuint shader);
    NGLI_GL_APIENTRY void (*DeleteTextures)(GLsizei n, const GLuint * textures);
    NGLI_GL_APIENTRY void (*DeleteVertexArrays)(GLsizei n, const GLuint * arrays);
    NGLI_GL_APIENTRY void (*DepthFunc)(GLenum func);
    NGLI_GL_APIENTRY void (*DepthMask)(GLboolean flag);
    NGLI_GL_APIENTRY void (*DetachShader)(GLuint program, GLuint shader);
    NGLI_GL_APIENTRY void (*Disable)(GLenum cap);
    NGLI_GL_APIENTRY void (*DisableVertexAttribArray)(GLuint index);
    NGLI_GL_APIENTRY void (*DispatchCompute)(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z);
    NGLI_GL_APIENTRY void (*DrawArrays)(GLenum mode, GLint first, GLsizei count);
    NGLI_GL_APIENTRY void (*DrawArraysInstanced)(GLenum mode, GLint first, GLsizei count, GLsizei instancecount);
    NGLI_GL_APIENTRY void (*DrawElements)(GLenum mode, GLsizei count, GLenum type, const void * indices);
    NGLI_GL_APIENTRY void (*DrawElementsInstanced)(GLenum mode, GLsizei count, GLenum type, const void * indices, GLsizei instancecount);
    NGLI_GL_APIENTRY void (*EGLImageTargetTexture2DOES)(GLenum target, GLeglImageOES image);
    NGLI_GL_APIENTRY void (*Enable)(GLenum cap);
    NGLI_GL_APIENTRY void (*EnableVertexAttribArray)(GLuint index);
    NGLI_GL_APIENTRY void (*EndQuery)(GLenum target);
    NGLI_GL_APIENTRY void (*EndQueryEXT)(GLenum target);
    NGLI_GL_APIENTRY GLsync (*FenceSync)(GLenum condition, GLbitfield flags);
    NGLI_GL_APIENTRY void (*FramebufferRenderbuffer)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
    NGLI_GL_APIENTRY void (*FramebufferTexture2D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
    NGLI_GL_APIENTRY void (*GenBuffers)(GLsizei n, GLuint * buffers);
    NGLI_GL_APIENTRY void (*GenFramebuffers)(GLsizei n, GLuint * framebuffers);
    NGLI_GL_APIENTRY void (*GenQueries)(GLsizei n, GLuint * ids);
    NGLI_GL_APIENTRY void (*GenQueriesEXT)(GLsizei n, GLuint * ids);
    NGLI_GL_APIENTRY void (*GenRenderbuffers)(GLsizei n, GLuint * renderbuffers);
    NGLI_GL_APIENTRY void (*GenTextures)(GLsizei n, GLuint * textures);
    NGLI_GL_APIENTRY void (*GenVertexArrays)(GLsizei n, GLuint * arrays);
    NGLI_GL_APIENTRY void (*GenerateMipmap)(GLenum target);
    NGLI_GL_APIENTRY void (*GetActiveAttrib)(GLuint program, GLuint index, GLsizei bufSize, GLsizei * length, GLint * size, GLenum * type, GLchar * name);
    NGLI_GL_APIENTRY void (*GetActiveUniform)(GLuint program, GLuint index, GLsizei bufSize, GLsizei * length, GLint * size, GLenum * type, GLchar * name);
    NGLI_GL_APIENTRY void (*GetActiveUniformBlockName)(GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei * length, GLchar * uniformBlockName);
    NGLI_GL_APIENTRY void (*GetActiveUniformBlockiv)(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint * params);
    NGLI_GL_APIENTRY void (*GetAttachedShaders)(GLuint program, GLsizei maxCount, GLsizei * count, GLuint * shaders);
    NGLI_GL_APIENTRY GLint (*GetAttribLocation)(GLuint program, const GLchar * name);
    NGLI_GL_APIENTRY void (*GetBooleanv)(GLenum pname, GLboolean * data);
    NGLI_GL_APIENTRY GLenum (*GetError)();
    NGLI_GL_APIENTRY void (*GetIntegeri_v)(GLenum target, GLuint index, GLint * data);
    NGLI_GL_APIENTRY void (*GetIntegerv)(GLenum pname, GLint * data);
    NGLI_GL_APIENTRY void (*GetInternalformativ)(GLenum target, GLenum internalformat, GLenum pname, GLsizei bufSize, GLint * params);
    NGLI_GL_APIENTRY void (*GetProgramInfoLog)(GLuint program, GLsizei bufSize, GLsizei * length, GLchar * infoLog);
    NGLI_GL_APIENTRY void (*GetProgramInterfaceiv)(GLuint program, GLenum programInterface, GLenum pname, GLint * params);
    NGLI_GL_APIENTRY GLuint (*GetProgramResourceIndex)(GLuint program, GLenum programInterface, const GLchar * name);
    NGLI_GL_APIENTRY GLint (*GetProgramResourceLocation)(GLuint program, GLenum programInterface, const GLchar * name);
    NGLI_GL_APIENTRY void (*GetProgramResourceName)(GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei * length, GLchar * name);
    NGLI_GL_APIENTRY void (*GetProgramResourceiv)(GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum * props, GLsizei bufSize, GLsizei * length, GLint * params);
    NGLI_GL_APIENTRY void (*GetProgramiv)(GLuint program, GLenum pname, GLint * params);
    NGLI_GL_APIENTRY void (*GetQueryObjectui64v)(GLuint id, GLenum pname, GLuint64 * params);
    NGLI_GL_APIENTRY void (*GetQueryObjectui64vEXT)(GLuint id, GLenum pname, GLuint64 * params);
    NGLI_GL_APIENTRY void (*GetRenderbufferParameteriv)(GLenum target, GLenum pname, GLint * params);
    NGLI_GL_APIENTRY void (*GetShaderInfoLog)(GLuint shader, GLsizei bufSize, GLsizei * length, GLchar * infoLog);
    NGLI_GL_APIENTRY void (*GetShaderSource)(GLuint shader, GLsizei bufSize, GLsizei * length, GLchar * source);
    NGLI_GL_APIENTRY void (*GetShaderiv)(GLuint shader, GLenum pname, GLint * params);
    NGLI_GL_APIENTRY const GLubyte * (*GetString)(GLenum name);
    NGLI_GL_APIENTRY const GLubyte * (*GetStringi)(GLenum name, GLuint index);
    NGLI_GL_APIENTRY GLuint (*GetUniformBlockIndex)(GLuint program, const GLchar * uniformBlockName);
    NGLI_GL_APIENTRY GLint (*GetUniformLocation)(GLuint program, const GLchar * name);
    NGLI_GL_APIENTRY void (*GetUniformiv)(GLuint program, GLint location, GLint * params);
    NGLI_GL_APIENTRY void (*InvalidateFramebuffer)(GLenum target, GLsizei numAttachments, const GLenum * attachments);
    NGLI_GL_APIENTRY void (*LinkProgram)(GLuint program);
    NGLI_GL_APIENTRY void (*MemoryBarrier)(GLbitfield barriers);
    NGLI_GL_APIENTRY void (*PolygonMode)(GLenum face, GLenum mode);
    NGLI_GL_APIENTRY void (*ReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void * pixels);
    NGLI_GL_APIENTRY void (*ReleaseShaderCompiler)();
    NGLI_GL_APIENTRY void (*RenderbufferStorage)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
    NGLI_GL_APIENTRY void (*RenderbufferStorageMultisample)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
    NGLI_GL_APIENTRY void (*ShaderBinary)(GLsizei count, const GLuint * shaders, GLenum binaryformat, const void * binary, GLsizei length);
    NGLI_GL_APIENTRY void (*ShaderSource)(GLuint shader, GLsizei count, const GLchar *const* string, const GLint * length);
    NGLI_GL_APIENTRY void (*StencilFunc)(GLenum func, GLint ref, GLuint mask);
    NGLI_GL_APIENTRY void (*StencilFuncSeparate)(GLenum face, GLenum func, GLint ref, GLuint mask);
    NGLI_GL_APIENTRY void (*StencilMask)(GLuint mask);
    NGLI_GL_APIENTRY void (*StencilMaskSeparate)(GLenum face, GLuint mask);
    NGLI_GL_APIENTRY void (*StencilOp)(GLenum fail, GLenum zfail, GLenum zpass);
    NGLI_GL_APIENTRY void (*StencilOpSeparate)(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass);
    NGLI_GL_APIENTRY void (*TexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void * pixels);
    NGLI_GL_APIENTRY void (*TexImage3D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void * pixels);
    NGLI_GL_APIENTRY void (*TexParameteri)(GLenum target, GLenum pname, GLint param);
    NGLI_GL_APIENTRY void (*TexStorage2D)(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
    NGLI_GL_APIENTRY void (*TexStorage3D)(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);
    NGLI_GL_APIENTRY void (*TexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void * pixels);
    NGLI_GL_APIENTRY void (*TexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void * pixels);
    NGLI_GL_APIENTRY void (*Uniform1f)(GLint location, GLfloat v0);
    NGLI_GL_APIENTRY void (*Uniform1fv)(GLint location, GLsizei count, const GLfloat * value);
    NGLI_GL_APIENTRY void (*Uniform1i)(GLint location, GLint v0);
    NGLI_GL_APIENTRY void (*Uniform1iv)(GLint location, GLsizei count, const GLint * value);
    NGLI_GL_APIENTRY void (*Uniform2f)(GLint location, GLfloat v0, GLfloat v1);
    NGLI_GL_APIENTRY void (*Uniform2fv)(GLint location, GLsizei count, const GLfloat * value);
    NGLI_GL_APIENTRY void (*Uniform2i)(GLint location, GLint v0, GLint v1);
    NGLI_GL_APIENTRY void (*Uniform2iv)(GLint location, GLsizei count, const GLint * value);
    NGLI_GL_APIENTRY void (*Uniform3f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
    NGLI_GL_APIENTRY void (*Uniform3fv)(GLint location, GLsizei count, const GLfloat * value);
    NGLI_GL_APIENTRY void (*Uniform3i)(GLint location, GLint v0, GLint v1, GLint v2);
    NGLI_GL_APIENTRY void (*Uniform3iv)(GLint location, GLsizei count, const GLint * value);
    NGLI_GL_APIENTRY void (*Uniform4f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
    NGLI_GL_APIENTRY void (*Uniform4fv)(GLint location, GLsizei count, const GLfloat * value);
    NGLI_GL_APIENTRY void (*Uniform4i)(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
    NGLI_GL_APIENTRY void (*Uniform4iv)(GLint location, GLsizei count, const GLint * value);
    NGLI_GL_APIENTRY void (*UniformBlockBinding)(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding);
    NGLI_GL_APIENTRY void (*UniformMatrix2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value);
    NGLI_GL_APIENTRY void (*UniformMatrix3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value);
    NGLI_GL_APIENTRY void (*UniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value);
    NGLI_GL_APIENTRY void (*UseProgram)(GLuint program);
    NGLI_GL_APIENTRY void (*VertexAttribDivisor)(GLuint index, GLuint divisor);
    NGLI_GL_APIENTRY void (*VertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void * pointer);
    NGLI_GL_APIENTRY void (*Viewport)(GLint x, GLint y, GLsizei width, GLsizei height);
    NGLI_GL_APIENTRY void (*WaitSync)(GLsync sync, GLbitfield flags, GLuint64 timeout);
};

#endif
