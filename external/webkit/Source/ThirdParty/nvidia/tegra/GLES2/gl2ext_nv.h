/*
 * Copyright (c) 2010 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef __gl2ext_nv_h_
#define __gl2ext_nv_h_

#ifdef __cplusplus
extern "C" {
#endif

/* GL_NV_platform_binary */

#ifndef GL_NV_platform_binary
#define GL_NV_platform_binary 1
#define GL_NVIDIA_PLATFORM_BINARY_NV                            0x890B
#endif

/* GL_EXT_Cg_shader */

#ifndef GL_EXT_Cg_shader
#define GL_EXT_Cg_shader 1
#define GL_CG_VERTEX_SHADER_EXT                                 0x890E
#define GL_CG_FRAGMENT_SHADER_EXT                               0x890F
#endif

/* GL_EXT_packed_float */

#ifndef GL_EXT_packed_float
#define GL_EXT_packed_float 1
#define GL_R11F_G11F_B10F_EXT                           0x8C3A
#define GL_UNSIGNED_INT_10F_11F_11F_REV_EXT             0x8C3B
#define GL_RGBA_SIGNED_COMPONENTS_EXT                   0x8C3C
#endif

/* GL_EXT_texture_array */

#ifndef GL_EXT_texture_array
#define GL_EXT_texture_array 1
#define GL_TEXTURE_2D_ARRAY_EXT           0x8C1A
#define GL_SAMPLER_2D_ARRAY_EXT           0x8DC1
#define GL_TEXTURE_BINDING_2D_ARRAY_EXT   0x8C1D
#define GL_MAX_ARRAY_TEXTURE_LAYERS_EXT   0x88FF
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER_EXT 0x8CD4
#ifdef GL_GLEXT_PROTOTYPES
GL_APICALL void GL_APIENTRY glFramebufferTextureLayerEXT(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer);
#endif
typedef void (GL_APIENTRYP PFNGLFRAMEBUFFERTEXTURELAYEREXTPROC) (GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer);
#endif

/* GL_EXT_texture_compression_dxt1 */

#ifndef GL_EXT_texture_compression_dxt1
#define GL_EXT_texture_compression_dxt1 1
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT   0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT  0x83F1
#endif

/* GL_EXT_texture_compression_latc */

#ifndef GL_EXT_texture_compression_latc
#define GL_EXT_texture_compression_latc 1
#define GL_COMPRESSED_LUMINANCE_LATC1_EXT               0x8C70
#define GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT        0x8C71
#define GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT         0x8C72
#define GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT  0x8C73
#endif

/* GL_EXT_texture_compression_s3tc */

#ifndef GL_EXT_texture_compression_s3tc
#define GL_EXT_texture_compression_s3tc 1
/* GL_COMPRESSED_RGB_S3TC_DXT1_EXT defined in GL_EXT_texture_compression_dxt1 already. */
/* GL_COMPRESSED_RGBA_S3TC_DXT1_EXT defined in GL_EXT_texture_compression_dxt1 already. */
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT  0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT  0x83F3
#endif

/* GL_NV_coverage_sample */

#ifndef GL_NV_coverage_sample
#define GL_NV_coverage_sample 1
#define GL_COVERAGE_COMPONENT_NV          0x8ED0
#define GL_COVERAGE_COMPONENT4_NV         0x8ED1
#define GL_COVERAGE_ATTACHMENT_NV         0x8ED2
#define GL_COVERAGE_BUFFERS_NV            0x8ED3
#define GL_COVERAGE_SAMPLES_NV            0x8ED4
#define GL_COVERAGE_ALL_FRAGMENTS_NV      0x8ED5
#define GL_COVERAGE_EDGE_FRAGMENTS_NV     0x8ED6
#define GL_COVERAGE_AUTOMATIC_NV          0x8ED7
#define GL_COVERAGE_BUFFER_BIT_NV         0x8000
#ifdef GL_GLEXT_PROTOTYPES
GL_APICALL void GL_APIENTRY glCoverageMaskNV(GLboolean mask);
GL_APICALL void GL_APIENTRY glCoverageOperationNV(GLenum operation);
#endif
typedef void (GL_APIENTRYP PFNGLCOVERAGEMASKNVPROC) (GLboolean mask);
typedef void (GL_APIENTRYP PFNGLCOVERAGEOPERATIONNVPROC) (GLenum operation);
#endif

/* GL_NV_depth_nonlinear */

#ifndef GL_NV_depth_nonlinear
#define GL_NV_depth_nonlinear 1
#define GL_DEPTH_COMPONENT16_NONLINEAR_NV 0x8E2C
#endif

/* GL_NV_draw_path */

#ifndef GL_NV_draw_path
#define GL_NV_draw_path 1
#define GL_PATH_QUALITY_NV          0x8ED8
#define GL_FILL_RULE_NV             0x8ED9
#define GL_STROKE_CAP0_STYLE_NV     0x8EE0
#define GL_STROKE_CAP1_STYLE_NV     0x8EE1
#define GL_STROKE_CAP2_STYLE_NV     0x8EE2
#define GL_STROKE_CAP3_STYLE_NV     0x8EE3
#define GL_STROKE_JOIN_STYLE_NV     0x8EE8
#define GL_STROKE_MITER_LIMIT_NV    0x8EE9
#define GL_EVEN_ODD_NV              0x8EF0
#define GL_NON_ZERO_NV              0x8EF1
#define GL_CAP_BUTT_NV              0x8EF4
#define GL_CAP_ROUND_NV             0x8EF5
#define GL_CAP_SQUARE_NV            0x8EF6
#define GL_CAP_TRIANGLE_NV          0x8EF7
#define GL_JOIN_MITER_NV            0x8EFC
#define GL_JOIN_ROUND_NV            0x8EFD
#define GL_JOIN_BEVEL_NV            0x8EFE
#define GL_JOIN_CLIPPED_MITER_NV    0x8EFF
#define GL_MATRIX_PATH_TO_CLIP_NV   0x8F04
#define GL_MATRIX_STROKE_TO_PATH_NV 0x8F05
#define GL_MATRIX_PATH_COORD0_NV    0x8F08
#define GL_MATRIX_PATH_COORD1_NV    0x8F09
#define GL_MATRIX_PATH_COORD2_NV    0x8F0A
#define GL_MATRIX_PATH_COORD3_NV    0x8F0B
#define GL_FILL_PATH_NV             0x8F18
#define GL_STROKE_PATH_NV           0x8F19
#define GL_MOVE_TO_NV               0x00
#define GL_LINE_TO_NV               0x01
#define GL_QUADRATIC_BEZIER_TO_NV   0x02
#define GL_CUBIC_BEZIER_TO_NV       0x03
#define GL_START_MARKER_NV          0x20
#define GL_CLOSE_NV                 0x21
#define GL_CLOSE_FILL_NV            0x22
#define GL_STROKE_CAP0_NV           0x40
#define GL_STROKE_CAP1_NV           0x41
#define GL_STROKE_CAP2_NV           0x42
#define GL_STROKE_CAP3_NV           0x43
#ifdef GL_GLEXT_PROTOTYPES
GL_APICALL GLuint GL_APIENTRY glCreatePathNV(GLenum datatype, GLsizei numCommands, const GLubyte* commands);
GL_APICALL void GL_APIENTRY glDeletePathNV(GLuint path);
GL_APICALL void GL_APIENTRY glPathVerticesNV(GLuint path, const void* vertices);
GL_APICALL void GL_APIENTRY glPathParameterfNV(GLuint path, GLenum paramType, GLfloat param);
GL_APICALL void GL_APIENTRY glPathParameteriNV(GLuint path, GLenum paramType, GLint param);
GL_APICALL GLuint GL_APIENTRY glCreatePathProgramNV(void);
GL_APICALL void GL_APIENTRY glPathMatrixNV(GLenum target, const GLfloat* value);
GL_APICALL void GL_APIENTRY glDrawPathNV(GLuint path, GLenum mode);
GL_APICALL GLuint GL_APIENTRY glCreatePathbufferNV(GLsizei capacity);
GL_APICALL void GL_APIENTRY glDeletePathbufferNV(GLuint buffer);
GL_APICALL void GL_APIENTRY glPathbufferPathNV(GLuint buffer, GLint index, GLuint path);
GL_APICALL void GL_APIENTRY glPathbufferPositionNV(GLuint buffer, GLint index, GLfloat x, GLfloat y);
GL_APICALL void GL_APIENTRY glDrawPathbufferNV(GLuint buffer, GLenum mode);
#endif
typedef GLuint (GL_APIENTRYP PFNGLCREATEPATHNVPROC) (GLenum datatype, GLsizei numCommands, const GLubyte* commands);
typedef void (GL_APIENTRYP PFNGLDELETEPATHNVPROC) (GLuint path);
typedef void (GL_APIENTRYP PFNGLPATHVERTICESNVPROC) (GLuint path, const void* vertices);
typedef void (GL_APIENTRYP PFNGLPATHPARAMETERFNVPROC) (GLuint path, GLenum paramType, GLfloat param);
typedef void (GL_APIENTRYP PFNGLPATHPARAMETERINVPROC) (GLuint path, GLenum paramType, GLint param);
typedef GLuint (GL_APIENTRYP PFNGLCREATEPATHPROGRAMNVPROC) (void);
typedef void (GL_APIENTRYP PFNGLPATHMATRIXNVPROC) (GLenum target, const GLfloat* value);
typedef void (GL_APIENTRYP PFNGLDRAWPATHNVPROC) (GLuint path, GLenum mode);
typedef GLuint (GL_APIENTRYP PFNGLCREATEPATHBUFFERNVPROC) (GLsizei capacity);
typedef void (GL_APIENTRYP PFNGLDELETEPATHBUFFERNVPROC) (GLuint buffer);
typedef void (GL_APIENTRYP PFNGLPATHBUFFERPATHNVPROC) (GLuint buffer, GLint index, GLuint path);
typedef void (GL_APIENTRYP PFNGLPATHBUFFERPOSITIONNVPROC) (GLuint buffer, GLint index, GLfloat x, GLfloat y);
typedef void (GL_APIENTRYP PFNGLDRAWPATHBUFFERPROC) (GLuint buffer, GLenum mode);
#endif

/* GL_NV_shader_framebuffer_fetch */

#ifndef GL_NV_shader_framebuffer_fetch
#define GL_NV_shader_framebuffer_fetch 1
#endif

/* GL_NV_get_tex_image */

#ifndef GL_NV_get_tex_image
#define GL_NV_get_tex_image 1
// those enums are the same as the big gl.h
#define GL_TEXTURE_WIDTH_NV                  0x1000
#define GL_TEXTURE_HEIGHT_NV                 0x1001
#define GL_TEXTURE_INTERNAL_FORMAT_NV        0x1003
#define GL_TEXTURE_COMPONENTS_NV             GL_TEXTURE_INTERNAL_FORMAT_NV
#define GL_TEXTURE_BORDER_NV                 0x1005
#define GL_TEXTURE_RED_SIZE_NV               0x805C
#define GL_TEXTURE_GREEN_SIZE_NV             0x805D
#define GL_TEXTURE_BLUE_SIZE_NV              0x805E
#define GL_TEXTURE_ALPHA_SIZE_NV             0x805F
#define GL_TEXTURE_LUMINANCE_SIZE_NV         0x8060
#define GL_TEXTURE_INTENSITY_SIZE_NV         0x8061
#define GL_TEXTURE_DEPTH_NV                  0x8071
#define GL_TEXTURE_COMPRESSED_IMAGE_SIZE_NV  0x86A0
#define GL_TEXTURE_COMPRESSED_NV             0x86A1
#define GL_TEXTURE_DEPTH_SIZE_NV             0x884A
#ifdef GL_GLEXT_PROTOTYPES
GL_APICALL void GL_APIENTRY glGetTexImageNV(GLenum target, GLint level, GLenum format, GLenum type, GLvoid* img);
GL_APICALL void GL_APIENTRY glGetCompressedTexImageNV(GLenum target, GLint level, GLvoid* img);
GL_APICALL void GL_APIENTRY glGetTexLevelParameterfvNV(GLenum target, GLint level, GLenum pname, GLfloat* params);
GL_APICALL void GL_APIENTRY glGetTexLevelParameterivNV(GLenum target, GLint level, GLenum pname, GLint* params);
#endif
typedef void (GL_APIENTRYP PFNGLGETTEXIMAGENVPROC) (GLenum target, GLint level, GLenum format, GLenum type, GLvoid* img);
typedef void (GL_APIENTRYP PFNGLGETCOMPRESSEDTEXIMAGENVPROC) (GLenum target, GLint level, GLvoid* img);
typedef void (GL_APIENTRYP PFNGLGETTEXLEVELPARAMETERFVNVPROC) (GLenum target, GLint level, GLenum pname, GLfloat* params);
typedef void (GL_APIENTRYP PFNGLGETTEXLEVELPARAMETERiVNVPROC) (GLenum target, GLint level, GLenum pname, GLint* params);
#endif

/* GL_EXT_bgra */

#ifndef GL_EXT_bgra
#define GL_EXT_bgra 1
#define GL_BGR_EXT                                              0x80E0
#define GL_BGRA_EXT                                             0x80E1
#endif


/* GL_NV_framebuffer_vertex_attrib_array */

#ifndef GL_NV_framebuffer_vertex_attrib_array
#define GL_NV_framebuffer_vertex_attrib_array 1
#define GL_FRAMEBUFFER_ATTACHABLE_NV                                  0x852A
#define GL_VERTEX_ATTRIB_ARRAY_NV                                     0x852B
#define GL_FRAMEBUFFER_ATTACHMENT_VERTEX_ATTRIB_ARRAY_SIZE_NV         0x852C
#define GL_FRAMEBUFFER_ATTACHMENT_VERTEX_ATTRIB_ARRAY_TYPE_NV         0x852D
#define GL_FRAMEBUFFER_ATTACHMENT_VERTEX_ATTRIB_ARRAY_NORMALIZED_NV   0x852E
#define GL_FRAMEBUFFER_ATTACHMENT_VERTEX_ATTRIB_ARRAY_OFFSET_NV       0x852F
#define GL_FRAMEBUFFER_ATTACHMENT_VERTEX_ATTRIB_ARRAY_WIDTH_NV        0x8530
#define GL_FRAMEBUFFER_ATTACHMENT_VERTEX_ATTRIB_ARRAY_STRIDE_NV       0x8531
#define GL_FRAMEBUFFER_ATTACHMENT_VERTEX_ATTRIB_ARRAY_HEIGHT_NV       0x8532
#ifdef GL_GLEXT_PROTOTYPES
GL_APICALL void GL_APIENTRY glFramebufferVertexAttribArrayNV(GLenum target, GLenum attachment, GLenum buffertarget, GLuint bufferobject, GLint size, GLenum type, GLboolean normalized, GLintptr offset, GLsizeiptr width, GLsizeiptr height, GLsizei stride);
#endif
typedef void (GL_APIENTRYP PFNGLFRAMEBUFFERVERTEXATTRIBARRAYNVPROC) (GLenum target, GLenum attachment, GLenum buffertarget, GLuint bufferobject, GLint size, GLenum type, GLboolean normalized, GLintptr offset, GLsizeiptr width, GLsizeiptr height, GLsizei stride);
#endif


/* GL_NV_draw_buffers */

#ifndef GL_NV_draw_buffers
#define GL_NV_draw_buffers 1
#define GL_MAX_DRAW_BUFFERS_NV                     0x8824
#define GL_DRAW_BUFFER0_NV                         0x8825
#define GL_DRAW_BUFFER1_NV                         0x8826
#define GL_DRAW_BUFFER2_NV                         0x8827
#define GL_DRAW_BUFFER3_NV                         0x8828
#define GL_DRAW_BUFFER4_NV                         0x8829
#define GL_DRAW_BUFFER5_NV                         0x882A
#define GL_DRAW_BUFFER6_NV                         0x882B
#define GL_DRAW_BUFFER7_NV                         0x882C
#define GL_DRAW_BUFFER8_NV                         0x882D
#define GL_DRAW_BUFFER9_NV                         0x882E
#define GL_DRAW_BUFFER10_NV                        0x882F
#define GL_DRAW_BUFFER11_NV                        0x8830
#define GL_DRAW_BUFFER12_NV                        0x8831
#define GL_DRAW_BUFFER13_NV                        0x8832
#define GL_DRAW_BUFFER14_NV                        0x8833
#define GL_DRAW_BUFFER15_NV                        0x8834
#ifdef GL_GLEXT_PROTOTYPES
GL_APICALL void GL_APIENTRY glDrawBuffersNV(GLsizei n, const GLenum *bufs);
#endif
typedef void (GL_APIENTRYP PFNGLDRAWBUFFERSNVPROC) (GLsizei n, const GLenum *bufs);
#endif

/* GL_NV_multiview_draw_buffers */

#ifndef GL_NV_multiview_draw_buffers
#define GL_NV_multiview_draw_buffers 1
#define GL_MAX_MULTIVIEW_BUFFERS_NV                   0x90F2
#define GL_COLOR_ATTACHMENT_NV                        0x90F0
#define GL_MULTIVIEW_NV                               0x90F1
#define GL_MULTIVIEW_ALL_NV                           -1
#ifdef GL_GLEXT_PROTOTYPES
GL_APICALL void GL_APIENTRY glDrawBuffersIndexedNV(GLsizei n, const GLenum *locations, const GLint *indices);
GL_APICALL void GL_APIENTRY glReadBufferIndexedNV(GLenum location, GLint index);
#endif
typedef void (GL_APIENTRYP PFNGLDRAWBUFFERSINDEXEDNVPROC) (GLsizei n, const GLenum *locations, const GLint *indices);
typedef void (GL_APIENTRYP PFNGLREADBUFFERINDEXEDNVPROC) (GLenum location, GLint index);
#endif


/* GL_EXT_robustness */

#ifndef GL_EXT_robustness
#define GL_EXT_robustness 1
#define GL_CONTEXT_ROBUST_ACCESS_EXT               0x90F3
#define GL_GUILTY_CONTEXT_RESET_EXT                0x8253
#define GL_INNOCENT_CONTEXT_RESET_EXT              0x8254
#define GL_UNKNOWN_CONTEXT_RESET_EXT               0x8255
#define GL_RESET_NOTIFICATION_STRATEGY_EXT         0x8256
#define GL_LOSE_CONTEXT_ON_RESET_EXT               0x8252
#define GL_NO_RESET_NOTIFICATION_EXT               0x8261
#ifdef GL_GLEXT_PROTOTYPES
GL_APICALL GLenum GL_APIENTRY glGetGraphicsResetStatusEXT(void);
GL_APICALL void GL_APIENTRY glGetnUniformfvEXT(GLuint program, GLint location, GLsizei bufSize, GLfloat *params);
GL_APICALL void GL_APIENTRY glGetnUniformivEXT(GLuint program, GLint location, GLsizei bufSize, GLint *params);
GL_APICALL void GL_APIENTRY glReadnPixelsEXT(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, void* pixels);
#endif
typedef GLenum (GL_APIENTRYP PFNGLGETGRAPHICSRESETSTATUSEXTPROC) (void);
typedef void (GL_APIENTRYP PFNGLGETNUNIFORMFVEXTPROC) (GLuint program, GLint location, GLsizei bufSize, GLfloat *params);
typedef void (GL_APIENTRYP PFNGLGETNUNIFORMIVEXTPROC) (GLuint program, GLint location, GLsizei bufSize, GLint *params);
typedef void (GL_APIENTRYP PFNGLREADNPIXELSEXTPROC) (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, void* pixels);
#endif /* GL_EXT_robustness */


/* GL_EXT_occlusion_query_boolean */

#ifndef GL_EXT_occlusion_query_boolean
#define GL_EXT_occlusion_query_boolean

#define       GL_ANY_SAMPLES_PASSED_EXT                         0x8C2F
#define       GL_ANY_SAMPLES_PASSED_CONSERVATIVE_EXT            0x8D6A

#define       GL_CURRENT_QUERY_EXT                              0x8865

#define       GL_QUERY_RESULT_EXT                               0x8866
#define       GL_QUERY_RESULT_AVAILABLE_EXT                     0x8867

#ifdef GL_GLEXT_PROTOTYPES
GL_APICALL void GL_APIENTRY glGenQueriesEXT(GLsizei n, GLuint *ids);
GL_APICALL void GL_APIENTRY glDeleteQueriesEXT(GLsizei n, const GLuint *ids);
GL_APICALL GLboolean GL_APIENTRY glIsQueryEXT(GLuint id);
GL_APICALL void GL_APIENTRY glBeginQueryEXT(GLenum target, GLuint id);
GL_APICALL void GL_APIENTRY glEndQueryEXT(GLenum target);
GL_APICALL void GL_APIENTRY glGetQueryivEXT(GLenum target, GLenum pname, GLint *params);
GL_APICALL void GL_APIENTRY glGetQueryObjectivEXT(GLuint id, GLenum pname, GLint *params);
GL_APICALL void GL_APIENTRY glGetQueryObjectuivEXT(GLuint id, GLenum pname, GLuint *params);
#endif

typedef void (GL_APIENTRYP PFNGLGENQUERIESEXTPROC) (GLsizei n, GLuint *ids);
typedef void (GL_APIENTRYP PFNGLDELETEQUERIESEXTPROC) (GLsizei n, const GLuint *ids);
typedef GLboolean (GL_APIENTRYP PFNGLISQUERYEXTPROC) (GLuint id);
typedef void (GL_APIENTRYP PFNGLBEGINQUERYEXTPROC) (GLenum target, GLuint id);
typedef void (GL_APIENTRYP PFNGLENDQUERYEXTPROC) (GLenum target);
typedef void (GL_APIENTRYP PFNGLGETQUERYIVEXTPROC) (GLenum target, GLenum pname, GLint *params);
typedef void (GL_APIENTRYP PFNGLGETQUERYOBJECTIVEXTPROC) (GLuint id, GLenum pname, GLint *params);
typedef void (GL_APIENTRYP PFNGLGETQUERYOBJECTUIVEXTPROC) (GLuint id, GLenum pname, GLuint *params);

#endif /* GL_EXT_occlusion_query_boolean */

/* GL_NV_timer_query */
#ifndef GL_NV_timer_query
#define GL_NV_timer_query 1
typedef khronos_int64_t GLint64NV;
typedef khronos_uint64_t GLuint64NV;
#define GL_TIME_ELAPSED_NV                                     0x88BF
#define GL_TIMESTAMP_NV                                        0x8E28
#ifdef GL_GLEXT_PROTOTYPES
GL_APICALL void GL_APIENTRY glQueryCounterNV(GLuint id, GLenum target);
GL_APICALL void GL_APIENTRY glGetQueryObjecti64vNV(GLuint id, GLenum pname, GLint64NV *params);
GL_APICALL void GL_APIENTRY glGetQueryObjectui64vNV(GLuint id, GLenum pname, GLuint64NV *params);
#endif
typedef void (GL_APIENTRYP PFNGLQUERYCOUNTERNVPROC) (GLuint id, GLenum target);
typedef void (GL_APIENTRYP PFNGLGETQUERYOBJECTI64VNVPROC) (GLuint id, GLenum pname, GLint64NV *params);
typedef void (GL_APIENTRYP PFNGLGETQUERYOBJECTUI64VNVPROC) (GLuint id, GLenum pname, GLuint64NV *params);
#endif /* GL_NV_timer_query */

//
// Deprecated extensions. Do not use.
//

/* GL_NV_read_buffer */

#ifndef GL_NV_read_buffer
#define GL_NV_read_buffer 1
#define GL_READ_BUFFER_NV                          0x0C02
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_NV   0x8CDC
#ifdef GL_GLEXT_PROTOTYPES
GL_APICALL void GL_APIENTRY glReadBufferNV(GLenum mode);
#endif
typedef void (GL_APIENTRYP PFNGLREADBUFFERNVPROC) (GLenum mode);
#endif

/* GL_NV_fbo_color_attachments */

#ifndef GL_NV_fbo_color_attachments
#define GL_NV_fbo_color_attachments 1
#define GL_MAX_COLOR_ATTACHMENTS_NV                     0x8CDF
#define GL_COLOR_ATTACHMENT0_NV                         0x8CE0
#define GL_COLOR_ATTACHMENT1_NV                         0x8CE1
#define GL_COLOR_ATTACHMENT2_NV                         0x8CE2
#define GL_COLOR_ATTACHMENT3_NV                         0x8CE3
#define GL_COLOR_ATTACHMENT4_NV                         0x8CE4
#define GL_COLOR_ATTACHMENT5_NV                         0x8CE5
#define GL_COLOR_ATTACHMENT6_NV                         0x8CE6
#define GL_COLOR_ATTACHMENT7_NV                         0x8CE7
#define GL_COLOR_ATTACHMENT8_NV                         0x8CE8
#define GL_COLOR_ATTACHMENT9_NV                         0x8CE9
#define GL_COLOR_ATTACHMENT10_NV                        0x8CEA
#define GL_COLOR_ATTACHMENT11_NV                        0x8CEB
#define GL_COLOR_ATTACHMENT12_NV                        0x8CEC
#define GL_COLOR_ATTACHMENT13_NV                        0x8CED
#define GL_COLOR_ATTACHMENT14_NV                        0x8CEE
#define GL_COLOR_ATTACHMENT15_NV                        0x8CEF
#endif

//
// Disabled extensions. Do not use.
//

/* GL_ARB_half_float_pixel */

#ifndef GL_ARB_half_float_pixel
#define GL_ARB_half_float_pixel 1
#define GL_HALF_FLOAT_ARB                          0x140B
#endif

/* GL_ARB_texture_rectangle */

#ifndef GL_ARB_texture_rectangle
#define GL_ARB_texture_rectangle 1
#define GL_TEXTURE_RECTANGLE_ARB                0x84F5
#define GL_TEXTURE_BINDING_RECTANGLE_ARB        0x84F6
#define GL_PROXY_TEXTURE_RECTANGLE_ARB          0x84F7
#define GL_MAX_RECTANGLE_TEXTURE_SIZE_ARB       0x84F8
#define GL_SAMPLER_2D_RECT_ARB                  0x8B63
#define GL_SAMPLER_2D_RECT_SHADOW_ARB           0x8B64
#endif

/* GL_EXT_texture_lod_bias */

#ifndef GL_EXT_texture_lod_bias
#define GL_EXT_texture_lod_bias 1
#define GL_MAX_TEXTURE_LOD_BIAS_EXT       0x84FD
#define GL_TEXTURE_FILTER_CONTROL_EXT     0x8500
#define GL_TEXTURE_LOD_BIAS_EXT           0x8501
#endif

/* GL_SGIS_texture_lod */

#ifndef GL_SGIS_texture_lod
#define GL_SGIS_texture_lod 1
#define GL_TEXTURE_MIN_LOD_SGIS           0x813A
#define GL_TEXTURE_MAX_LOD_SGIS           0x813B
#define GL_TEXTURE_BASE_LEVEL_SGIS        0x813C
#define GL_TEXTURE_MAX_LEVEL_SGIS         0x813D
#endif

#ifdef __cplusplus
}
#endif

#endif /* __gl2ext_nv_h_ */


