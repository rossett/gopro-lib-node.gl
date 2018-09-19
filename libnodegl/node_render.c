/*
 * Copyright 2016 GoPro Inc.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "glincludes.h"
#include "hmap.h"
#include "log.h"
#include "math_utils.h"
#include "nodegl.h"
#include "nodes.h"
#include "utils.h"

#define TEXTURES_TYPES_LIST (const int[]){NGL_NODE_TEXTURE2D,       \
                                          NGL_NODE_TEXTURE3D,       \
                                          -1}

#define PROGRAMS_TYPES_LIST (const int[]){NGL_NODE_PROGRAM,         \
                                          -1}

#define UNIFORMS_TYPES_LIST (const int[]){NGL_NODE_BUFFERFLOAT,     \
                                          NGL_NODE_BUFFERVEC2,      \
                                          NGL_NODE_BUFFERVEC3,      \
                                          NGL_NODE_BUFFERVEC4,      \
                                          NGL_NODE_UNIFORMFLOAT,    \
                                          NGL_NODE_UNIFORMVEC2,     \
                                          NGL_NODE_UNIFORMVEC3,     \
                                          NGL_NODE_UNIFORMVEC4,     \
                                          NGL_NODE_UNIFORMQUAT,     \
                                          NGL_NODE_UNIFORMINT,      \
                                          NGL_NODE_UNIFORMMAT4,     \
                                          -1}

#define ATTRIBUTES_TYPES_LIST (const int[]){NGL_NODE_BUFFERFLOAT,   \
                                            NGL_NODE_BUFFERVEC2,    \
                                            NGL_NODE_BUFFERVEC3,    \
                                            NGL_NODE_BUFFERVEC4,    \
                                            NGL_NODE_BUFFERMAT4,    \
                                            -1}

#define GEOMETRY_TYPES_LIST (const int[]){NGL_NODE_CIRCLE,          \
                                          NGL_NODE_GEOMETRY,        \
                                          NGL_NODE_QUAD,            \
                                          NGL_NODE_TRIANGLE,        \
                                          -1}

#define BUFFERS_TYPES_LIST (const int[]){NGL_NODE_BUFFERFLOAT,      \
                                         NGL_NODE_BUFFERVEC2,       \
                                         NGL_NODE_BUFFERVEC3,       \
                                         NGL_NODE_BUFFERVEC4,       \
                                         NGL_NODE_BUFFERINT,        \
                                         NGL_NODE_BUFFERIVEC2,      \
                                         NGL_NODE_BUFFERIVEC3,      \
                                         NGL_NODE_BUFFERIVEC4,      \
                                         NGL_NODE_BUFFERUINT,       \
                                         NGL_NODE_BUFFERUIVEC2,     \
                                         NGL_NODE_BUFFERUIVEC3,     \
                                         NGL_NODE_BUFFERUIVEC4,     \
                                         -1}

#define OFFSET(x) offsetof(struct render, x)
static const struct node_param render_params[] = {
    {"geometry", PARAM_TYPE_NODE, OFFSET(geometry), .flags=PARAM_FLAG_CONSTRUCTOR,
                 .node_types=GEOMETRY_TYPES_LIST,
                 .desc=NGLI_DOCSTRING("geometry to be rasterized")},
    {"program",  PARAM_TYPE_NODE, OFFSET(pipeline.program),
                 .node_types=PROGRAMS_TYPES_LIST,
                 .desc=NGLI_DOCSTRING("program to be executed")},
    {"textures", PARAM_TYPE_NODEDICT, OFFSET(pipeline.textures),
                 .node_types=TEXTURES_TYPES_LIST,
                 .desc=NGLI_DOCSTRING("textures made accessible to the `program`")},
    {"uniforms", PARAM_TYPE_NODEDICT, OFFSET(pipeline.uniforms),
                 .node_types=UNIFORMS_TYPES_LIST,
                 .desc=NGLI_DOCSTRING("uniforms made accessible to the `program`")},
    {"buffers",  PARAM_TYPE_NODEDICT, OFFSET(pipeline.buffers),
                 .node_types=BUFFERS_TYPES_LIST,
                 .desc=NGLI_DOCSTRING("buffers made accessible to the `program`")},
    {"attributes", PARAM_TYPE_NODEDICT, OFFSET(attributes),
                 .node_types=ATTRIBUTES_TYPES_LIST,
                 .desc=NGLI_DOCSTRING("extra vertex attributes made accessible to the `program`")},
    {"instance_attributes", PARAM_TYPE_NODEDICT, OFFSET(instance_attributes),
                 .node_types=ATTRIBUTES_TYPES_LIST,
                 .desc=NGLI_DOCSTRING("per instance extra vertex attributes made accessible to the `program`")},
    {"nb_instances", PARAM_TYPE_INT, OFFSET(nb_instances),
                 .desc=NGLI_DOCSTRING("number of instances to draw")},
    {NULL}
};

static int update_geometry_uniforms(struct ngl_node *node)
{
    struct ngl_ctx *ctx = node->ctx;
    struct glcontext *gl = ctx->glcontext;
    struct render *s = node->priv_data;

    const float *modelview_matrix = ngli_darray_tail(&ctx->modelview_matrix_stack);
    const float *projection_matrix = ngli_darray_tail(&ctx->projection_matrix_stack);

    if (s->modelview_matrix_location_id >= 0) {
        ngli_glUniformMatrix4fv(gl, s->modelview_matrix_location_id, 1, GL_FALSE, modelview_matrix);
    }

    if (s->projection_matrix_location_id >= 0) {
        ngli_glUniformMatrix4fv(gl, s->projection_matrix_location_id, 1, GL_FALSE, projection_matrix);
    }

    if (s->normal_matrix_location_id >= 0) {
        float normal_matrix[3*3];
        ngli_mat3_from_mat4(normal_matrix, modelview_matrix);
        ngli_mat3_inverse(normal_matrix, normal_matrix);
        ngli_mat3_transpose(normal_matrix, normal_matrix);
        ngli_glUniformMatrix3fv(gl, s->normal_matrix_location_id, 1, GL_FALSE, normal_matrix);
    }

    return 0;
}

#define GEOMETRY_OFFSET(x) offsetof(struct geometry, x)
static const struct {
    const char *const_name;
    int offset;
} attrib_const_map[] = {
    {"ngl_position", GEOMETRY_OFFSET(vertices_buffer)},
    {"ngl_uvcoord",  GEOMETRY_OFFSET(uvcoords_buffer)},
    {"ngl_normal",   GEOMETRY_OFFSET(normals_buffer)},
};

static int update_vertex_attribs(struct ngl_node *node)
{
    struct ngl_ctx *ctx = node->ctx;
    struct glcontext *gl = ctx->glcontext;
    struct render *s = node->priv_data;

    for (int i = 0; i < s->nb_attribute_pairs; i++) {
        const struct nodeprograminfopair *pair = &s->attribute_pairs[i];
        const struct attributeprograminfo *info = pair->program_info;
        const GLint aid = info->id;
        const struct ngl_node *bnode = pair->node;
        struct buffer *buffer = bnode->priv_data;

        ngli_glBindBuffer(gl, GL_ARRAY_BUFFER, buffer->buffer_id);
        /* TODO: check that attribute type is mat4 */
        if (bnode->class->id == NGL_NODE_BUFFERMAT4) {
            uintptr_t data_stride = buffer->data_stride / 4;
            for (int j = 0; j < 4; j++) {
                ngli_glEnableVertexAttribArray(gl, aid + j);
                ngli_glVertexAttribPointer(gl, aid + j, 4, GL_FLOAT, GL_FALSE, buffer->data_stride, (void *)(j * data_stride));
            }
        } else {
            ngli_glEnableVertexAttribArray(gl, aid);
            ngli_glVertexAttribPointer(gl, aid, buffer->data_comp, GL_FLOAT, GL_FALSE, buffer->data_stride, NULL);
        }

        if (i >= s->first_instance_attribute_index) {
            if (bnode->class->id == NGL_NODE_BUFFERMAT4) {
                for (int j = 0; j < 4; j++) {
                    ngli_glVertexAttribDivisor(gl, aid + j, 1);
                }
            } else {
                ngli_glVertexAttribDivisor(gl, aid, 1);
            }
        }
    }

    return 0;
}

static int disable_vertex_attribs(struct ngl_node *node)
{
    struct ngl_ctx *ctx = node->ctx;
    struct glcontext *gl = ctx->glcontext;
    struct render *s = node->priv_data;

    for (int i = 0; i < s->nb_attribute_pairs; i++) {
        const struct nodeprograminfopair *pair = &s->attribute_pairs[i];
        const struct attributeprograminfo *info = pair->program_info;
        const GLint aid = info->id;

        ngli_glDisableVertexAttribArray(gl, aid);
    }

    return 0;
}

static int get_uniform_location(struct hmap *uniforms, const char *name)
{
    const struct uniformprograminfo *info = ngli_hmap_get(uniforms, name);
    return info ? info->id : -1;
}

static int pair_node_to_attribinfo(struct render *s, const char *name,
                                   struct ngl_node *anode)
{
    const struct ngl_node *pnode = s->pipeline.program;
    const struct program *program = pnode->priv_data;

    const struct attributeprograminfo *active_attribute =
        ngli_hmap_get(program->active_attributes, name);
    if (!active_attribute)
        return 1;

    if (active_attribute->id < 0)
        return 0;

    struct nodeprograminfopair pair = {
        .node = anode,
        .program_info = (void *)active_attribute,
    };
    snprintf(pair.name, sizeof(pair.name), "%s", name);
    s->attribute_pairs[s->nb_attribute_pairs++] = pair;
    return 0;
}

static int pair_nodes_to_attribinfo(struct ngl_node *node, struct hmap *attributes,
                                    int per_instance)
{
    if (!attributes)
        return 0;

    struct render *s = node->priv_data;

    const struct hmap_entry *entry = NULL;
    while ((entry = ngli_hmap_next(attributes, entry))) {
        struct ngl_node *anode = entry->data;
        struct buffer *buffer = anode->priv_data;
        buffer->generate_gl_buffer = 1;

        int ret = ngli_node_init(anode);
        if (ret < 0)
            return ret;
        if (per_instance) {
            if (buffer->count != s->nb_instances) {
                LOG(ERROR,
                    "attribute buffer %s count (%d) does not match instance count (%d)",
                    entry->key,
                    buffer->count,
                    s->nb_instances);
                return -1;
            }
        } else {
            struct geometry *geometry = s->geometry->priv_data;
            struct buffer *vertices = geometry->vertices_buffer->priv_data;
            if (buffer->count != vertices->count) {
                LOG(ERROR,
                    "attribute buffer %s count (%d) does not match vertices count (%d)",
                    entry->key,
                    buffer->count,
                    vertices->count);
                return -1;
            }
        }

        ret = pair_node_to_attribinfo(s, entry->key, anode);
        if (ret < 0)
            return ret;

        if (ret == 1) {
            const struct ngl_node *pnode = s->pipeline.program;
            LOG(WARNING, "attribute %s attached to %s not found in %s",
                entry->key, node->name, pnode->name);
        }
    }
    return 0;
}

static int render_init(struct ngl_node *node)
{
    struct ngl_ctx *ctx = node->ctx;
    struct glcontext *gl = ctx->glcontext;
    struct render *s = node->priv_data;

    int ret = ngli_node_init(s->geometry);
    if (ret < 0)
        return ret;

    if (!s->pipeline.program) {
        s->pipeline.program = ngl_node_create(NGL_NODE_PROGRAM);
        if (!s->pipeline.program)
            return -1;
        ret = ngli_node_attach_ctx(s->pipeline.program, ctx);
        if (ret < 0)
            return ret;
    }

    ret = ngli_pipeline_init(node);
    if (ret < 0)
        return ret;

    struct ngl_node *pnode = s->pipeline.program;
    struct program *program = pnode->priv_data;
    struct hmap *uniforms = program->active_uniforms;

    /* Instancing checks */
    if (s->nb_instances && !(gl->features & NGLI_FEATURE_DRAW_INSTANCED)) {
        LOG(ERROR, "context does not support instanced draws");
        return -1;
    }

    if (s->instance_attributes && !(gl->features & NGLI_FEATURE_INSTANCED_ARRAY)) {
        LOG(ERROR, "context does not support instanced arrays");
        return -1;
    }

    /* Builtin uniforms */
    s->modelview_matrix_location_id  = get_uniform_location(uniforms, "ngl_modelview_matrix");
    s->projection_matrix_location_id = get_uniform_location(uniforms, "ngl_projection_matrix");
    s->normal_matrix_location_id     = get_uniform_location(uniforms, "ngl_normal_matrix");

    /* User and builtin attribute pairs */
    const int max_nb_attributes = NGLI_ARRAY_NB(attrib_const_map)
                                + (s->attributes ? ngli_hmap_count(s->attributes) : 0)
                                + (s->instance_attributes ? ngli_hmap_count(s->instance_attributes) : 0);
    s->attribute_pairs = calloc(max_nb_attributes, sizeof(*s->attribute_pairs));
    if (!s->attribute_pairs)
        return -1;

    /* Builtin vertex attributes */
    struct geometry *geometry = s->geometry->priv_data;
    for (int i = 0; i < NGLI_ARRAY_NB(attrib_const_map); i++) {
        const int offset = attrib_const_map[i].offset;
        const char *const_name = attrib_const_map[i].const_name;
        uint8_t *buffer_node_p = ((uint8_t *)geometry) + offset;
        struct ngl_node *anode = *(struct ngl_node **)buffer_node_p;
        if (!anode)
            continue;

        ret = pair_node_to_attribinfo(s, const_name, anode);
        if (ret < 0)
            return ret;
    }

    /* User vertex attributes */
    ret = pair_nodes_to_attribinfo(node, s->attributes, 0);
    if (ret < 0)
        return ret;

    /* User per instance vertex attributes */
    s->first_instance_attribute_index = s->nb_attribute_pairs;
    ret = pair_nodes_to_attribinfo(node, s->instance_attributes, 1);
    if (ret < 0)
        return ret;

    if (gl->features & NGLI_FEATURE_VERTEX_ARRAY_OBJECT) {
        ngli_glGenVertexArrays(gl, 1, &s->vao_id);
        ngli_glBindVertexArray(gl, s->vao_id);
        update_vertex_attribs(node);
    }

    return 0;
}

static void render_uninit(struct ngl_node *node)
{
    struct ngl_ctx *ctx = node->ctx;
    struct glcontext *gl = ctx->glcontext;

    struct render *s = node->priv_data;

    if (gl->features & NGLI_FEATURE_VERTEX_ARRAY_OBJECT) {
        ngli_glDeleteVertexArrays(gl, 1, &s->vao_id);
    }

    ngli_pipeline_uninit(node);

    free(s->attribute_pairs);
}

static int render_update(struct ngl_node *node, double t)
{
    struct render *s = node->priv_data;

    int ret = ngli_node_update(s->geometry, t);
    if (ret < 0)
        return ret;

    for (int i = 0; i < s->nb_attribute_pairs; i++) {
        const struct nodeprograminfopair *pair = &s->attribute_pairs[i];
        struct ngl_node *bnode = (struct ngl_node *)pair->node;
        int ret = ngli_node_update(bnode, t);
        if (ret < 0)
            return ret;
    }

    return ngli_pipeline_update(node, t);
}

static void render_draw(struct ngl_node *node)
{
    struct ngl_ctx *ctx = node->ctx;
    struct glcontext *gl = ctx->glcontext;

    struct render *s = node->priv_data;

    const struct program *program = s->pipeline.program->priv_data;
    ngli_glUseProgram(gl, program->program_id);

    if (gl->features & NGLI_FEATURE_VERTEX_ARRAY_OBJECT) {
        ngli_glBindVertexArray(gl, s->vao_id);
    } else {
        update_vertex_attribs(node);
    }

    update_geometry_uniforms(node);

    int ret = ngli_pipeline_upload_data(node);
    if (ret < 0) {
        LOG(ERROR, "pipeline upload data error");
    }

    const struct geometry *geometry = s->geometry->priv_data;
    const struct buffer *indices_buffer = geometry->indices_buffer->priv_data;

    GLenum indices_type;
    ngli_format_get_gl_format_type(gl, indices_buffer->data_format, NULL, NULL, &indices_type);

    ngli_glBindBuffer(gl, GL_ELEMENT_ARRAY_BUFFER, indices_buffer->buffer_id);
    if (s->nb_instances > 0) {
        ngli_glDrawElementsInstanced(gl, geometry->topology, indices_buffer->count, indices_type, 0, s->nb_instances);
    } else {
        ngli_glDrawElements(gl, geometry->topology, indices_buffer->count, indices_type, 0);
    }

    if (!(gl->features & NGLI_FEATURE_VERTEX_ARRAY_OBJECT)) {
        disable_vertex_attribs(node);
    }
}

const struct node_class ngli_render_class = {
    .id        = NGL_NODE_RENDER,
    .name      = "Render",
    .init      = render_init,
    .uninit    = render_uninit,
    .update    = render_update,
    .draw      = render_draw,
    .priv_size = sizeof(struct render),
    .params    = render_params,
    .file      = __FILE__,
};
