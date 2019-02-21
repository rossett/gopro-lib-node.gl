/*
 * Copyright 2019 GoPro Inc.
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
#include <string.h>

#include "memory.h"
#include "nodes.h"
#include "drawutils.h"
#include "log.h"
#include "math_utils.h"
#include "program.h"

#define VALIGN_CENTER 0
#define VALIGN_TOP    1
#define VALIGN_BOTTOM 2

#define HALIGN_CENTER 0
#define HALIGN_RIGHT  1
#define HALIGN_LEFT   2

static const struct param_choices valign_choices = {
    .name = "valign",
    .consts = {
        {"center", VALIGN_CENTER, .desc=NGLI_DOCSTRING("vertically centered")},
        {"bottom", VALIGN_BOTTOM, .desc=NGLI_DOCSTRING("bottom positioned")},
        {"top",    VALIGN_TOP,    .desc=NGLI_DOCSTRING("top positioned")},
        {NULL}
    }
};

static const struct param_choices halign_choices = {
    .name = "halign",
    .consts = {
        {"center", HALIGN_CENTER, .desc=NGLI_DOCSTRING("horizontally centered")},
        {"right",  HALIGN_RIGHT,  .desc=NGLI_DOCSTRING("right positioned")},
        {"left",   HALIGN_LEFT,   .desc=NGLI_DOCSTRING("left positioned")},
        {NULL}
    }
};

#define OFFSET(x) offsetof(struct text_priv, x)
static const struct node_param text_params[] = {
    {"text",        PARAM_TYPE_STR, OFFSET(text), .flags=PARAM_FLAG_CONSTRUCTOR,
                    .desc=NGLI_DOCSTRING("text string to rasterize")},
    {"fg_color",    PARAM_TYPE_VEC4, OFFSET(fg_color), {.vec={1.0, 1.0, 1.0, 1.0}},
                    .desc=NGLI_DOCSTRING("foreground text color")},
    {"bg_color",    PARAM_TYPE_VEC4, OFFSET(bg_color), {.vec={0.0, 0.0, 0.0, 0.8}},
                    .desc=NGLI_DOCSTRING("background text color")},
    {"box_corner",  PARAM_TYPE_VEC3, OFFSET(box_corner), {.vec={-1.0, -1.0, 0.0}},
                    .desc=NGLI_DOCSTRING("origin coordinates of `box_width` and `box_height` vectors")},
    {"box_width",   PARAM_TYPE_VEC3, OFFSET(box_width), {.vec={2.0, 0.0, 0.0}},
                    .desc=NGLI_DOCSTRING("box width vector")},
    {"box_height",  PARAM_TYPE_VEC3, OFFSET(box_height), {.vec={0.0, 2.0, 0.0}},
                    .desc=NGLI_DOCSTRING("box height vector")},
    {"padding",     PARAM_TYPE_INT, OFFSET(padding), {.i64=3},
                    .desc=NGLI_DOCSTRING("pixel padding around the text")},
    {"valign",      PARAM_TYPE_SELECT, OFFSET(valign), {.i64=VALIGN_CENTER},
                    .choices=&valign_choices,
                    .desc=NGLI_DOCSTRING("vertical alignment of the text in the box")},
    {"halign",      PARAM_TYPE_SELECT, OFFSET(halign), {.i64=HALIGN_CENTER},
                    .choices=&halign_choices,
                    .desc=NGLI_DOCSTRING("horizontal alignment of the text in the box")},
    {NULL}
};

static void set_canvas_dimensions(struct canvas *canvas, const char *s)
{
    canvas->w = 0;
    canvas->h = NGLI_FONT_H;
    int cur_w = 0;
    for (int i = 0; s[i]; i++) {
        if (s[i] == '\n') {
            cur_w = 0;
            canvas->h += NGLI_FONT_H;
        } else {
            cur_w += NGLI_FONT_W;
            canvas->w = NGLI_MAX(canvas->w, cur_w);
        }
    }
}

static int prepare_canvas(struct text_priv *s)
{
    set_canvas_dimensions(&s->canvas, s->text);
    s->canvas.w += 2 * s->padding;
    s->canvas.h += 2 * s->padding;
    s->canvas.buf = ngli_calloc(s->canvas.w * s->canvas.h, sizeof(*s->canvas.buf) * 4);
    if (!s->canvas.buf)
        return -1;

    const uint32_t fg = NGLI_COLOR_VEC4_TO_U32(s->fg_color);
    const uint32_t bg = NGLI_COLOR_VEC4_TO_U32(s->bg_color);
    struct rect rect = {.w = s->canvas.w, .h = s->canvas.h};
    ngli_drawutils_draw_rect(&s->canvas, &rect, bg);
    ngli_drawutils_print(&s->canvas, s->padding, s->padding, s->text, fg);
    return 0;
}

static const char * const vertex_data =
    "#version 100"                                                          "\n"
    "precision highp float;"                                                "\n"
    "attribute vec4 position;"                                              "\n"
    "attribute vec2 uvcoord;"                                               "\n"
    "uniform mat4 modelview_matrix;"                                        "\n"
    "uniform mat4 projection_matrix;"                                       "\n"
    "varying vec2 var_tex_coord;"                                           "\n"
    "void main()"                                                           "\n"
    "{"                                                                     "\n"
    "    gl_Position = projection_matrix * modelview_matrix * position;"    "\n"
    "    var_tex_coord = uvcoord;"                                          "\n"
    "}";

static const char * const fragment_data =
    "#version 100"                                                          "\n"
    "precision highp float;"                                                "\n"
    "uniform sampler2D tex;"                                                "\n"
    "varying vec2 var_tex_coord;"                                           "\n"
    "void main(void)"                                                       "\n"
    "{"                                                                     "\n"
    "    gl_FragColor = texture2D(tex, var_tex_coord);"                     "\n"
    "}";

static void enable_vertex_attribs(struct ngl_node *node)
{
    struct ngl_ctx *ctx = node->ctx;
    struct glcontext *gl = ctx->glcontext;
    struct text_priv *s = node->priv_data;

    ngli_glEnableVertexAttribArray(gl, s->position_location);
    ngli_glBindBuffer(gl, GL_ARRAY_BUFFER, s->vertices_id);
    ngli_glVertexAttribPointer(gl, s->position_location, 3, GL_FLOAT, GL_FALSE, 3 * 4, NULL);

    ngli_glEnableVertexAttribArray(gl, s->uvcoord_location);
    ngli_glBindBuffer(gl, GL_ARRAY_BUFFER, s->uvcoord_id);
    ngli_glVertexAttribPointer(gl, s->uvcoord_location, 2, GL_FLOAT, GL_FALSE, 2 * 4, NULL);
}

#define C(index) corner[index]
#define W(index) width[index]
#define H(index) height[index]

static void vec3_scale(float *v, const float scale)
{
    v[0] *= scale;
    v[1] *= scale;
    v[2] *= scale;
}

static int text_init(struct ngl_node *node)
{
    struct ngl_ctx *ctx = node->ctx;
    struct glcontext *gl = ctx->glcontext;
    struct text_priv *s = node->priv_data;

    int ret = prepare_canvas(s);
    if (ret < 0)
        return ret;

    float corner[3], width[3], height[3];
    memcpy(corner, s->box_corner, sizeof(corner));
    memcpy(width,  s->box_width,  sizeof(width));
    memcpy(height, s->box_height, sizeof(height));

    const float quad_width_len  = ngli_vec3_length(width);
    const float quad_height_len = ngli_vec3_length(height);
    const float quad_ratio = quad_width_len / quad_height_len;
    const float tex_ratio = s->canvas.w / (float)s->canvas.h;

    //LOG(ERROR, "quad:%g/%g=%g tex:%g/%g=%g",
    //    quad_width_len, quad_height_len, quad_ratio,
    //    (float)s->canvas.w, (float)s->canvas.h, tex_ratio);

    const float scale_ratio = tex_ratio / quad_ratio;
    if (scale_ratio < 1.0)
        vec3_scale(width, scale_ratio);
    else
        vec3_scale(height, 1. / scale_ratio);

    float diff_width[3], diff_height[3];
    //LOG(ERROR, "quad: w=(%g,%g,%g) h=(%g,%g,%g) -> w=(%g,%g,%g) h=(%g,%g,%g)",
    //    NGLI_ARG_VEC3(s->box_width),
    //    NGLI_ARG_VEC3(s->box_height),
    //    NGLI_ARG_VEC3(width),
    //    NGLI_ARG_VEC3(height));

    //LOG(ERROR, "quad scaled down to: %g/%g=%g",
    //    ngli_vec3_length(width),
    //    ngli_vec3_length(height),
    //    ngli_vec3_length(width) / ngli_vec3_length(height));

    ngli_vec3_sub(diff_width,  s->box_width,  width);
    ngli_vec3_sub(diff_height, s->box_height, height);

    //LOG(ERROR, "diff_width:(%g,%g,%g) diff_height:(%g,%g,%g)",
    //    NGLI_ARG_VEC3(diff_width),
    //    NGLI_ARG_VEC3(diff_height));

    switch (s->valign) {
        case VALIGN_CENTER:
            vec3_scale(diff_height, .5);
            /* Fall through */
        case VALIGN_TOP:
            ngli_vec3_add(corner, corner, diff_height);
            break;
    }

    switch (s->halign) {
        case HALIGN_CENTER:
            vec3_scale(diff_width, .5);
            /* Fall through */
        case HALIGN_RIGHT:
            ngli_vec3_add(corner, corner, diff_width);
            break;
    }

    const float vertices[] = {
        C(0),               C(1),               C(2),
        C(0) + W(0),        C(1) + W(1),        C(2) + W(2),
        C(0) + H(0) + W(0), C(1) + H(1) + W(1), C(2) + H(2) + W(2),
        C(0) + H(0),        C(1) + H(1),        C(2) + H(2),
    };

    static const float uvs[] = {0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0};

    s->program_id = ngli_program_load(gl, vertex_data, fragment_data);
    if (!s->program_id)
        return -1;

    s->position_location = ngli_glGetAttribLocation(gl, s->program_id, "position");
    s->uvcoord_location  = ngli_glGetAttribLocation(gl, s->program_id, "uvcoord");
    s->texture_location  = ngli_glGetUniformLocation(gl, s->program_id, "tex");

    s->modelview_matrix_location  = ngli_glGetUniformLocation(gl, s->program_id, "modelview_matrix");
    s->projection_matrix_location = ngli_glGetUniformLocation(gl, s->program_id, "projection_matrix");

    if (s->position_location < 0 ||
        s->uvcoord_location < 0 ||
        s->texture_location < 0 ||
        s->modelview_matrix_location < 0 ||
        s->projection_matrix_location < 0)
        return -1;

    ngli_glUseProgram(gl, s->program_id);

    ngli_glUniform1i(gl, s->texture_location, 0);

    ngli_glGenBuffers(gl, 1, &s->vertices_id);
    ngli_glBindBuffer(gl, GL_ARRAY_BUFFER, s->vertices_id);
    ngli_glBufferData(gl, GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    ngli_glGenBuffers(gl, 1, &s->uvcoord_id);
    ngli_glBindBuffer(gl, GL_ARRAY_BUFFER, s->uvcoord_id);
    ngli_glBufferData(gl, GL_ARRAY_BUFFER, sizeof(uvs), uvs, GL_STATIC_DRAW);

    if (gl->features & NGLI_FEATURE_VERTEX_ARRAY_OBJECT) {
        ngli_glGenVertexArrays(gl, 1, &s->vao_id);
        ngli_glBindVertexArray(gl, s->vao_id);
        enable_vertex_attribs(node);
    }

    struct texture_params tex_params = NGLI_TEXTURE_PARAM_DEFAULTS;
    tex_params.width = s->canvas.w;
    tex_params.height = s->canvas.h;
    tex_params.format = NGLI_FORMAT_R8G8B8A8_UNORM;
    tex_params.min_filter = tex_params.mag_filter = GL_LINEAR;
    ret = ngli_texture_init(&s->texture, gl, &tex_params);
    if (ret < 0)
        return ret;

    return ngli_texture_upload(&s->texture, s->canvas.buf);
}

static void text_draw(struct ngl_node *node)
{
    struct ngl_ctx *ctx = node->ctx;
    struct glcontext *gl = ctx->glcontext;
    struct text_priv *s = node->priv_data;

    const float *modelview_matrix  = ngli_darray_tail(&ctx->modelview_matrix_stack);
    const float *projection_matrix = ngli_darray_tail(&ctx->projection_matrix_stack);

    ngli_glUseProgram(gl, s->program_id);
    if (gl->features & NGLI_FEATURE_VERTEX_ARRAY_OBJECT)
        ngli_glBindVertexArray(gl, s->vao_id);
    else
        enable_vertex_attribs(node);

    ngli_glUniformMatrix4fv(gl, s->modelview_matrix_location, 1, GL_FALSE, modelview_matrix);
    ngli_glUniformMatrix4fv(gl, s->projection_matrix_location, 1, GL_FALSE, projection_matrix);
    ngli_glActiveTexture(gl, GL_TEXTURE0);
    ngli_glBindTexture(gl, s->texture.target, s->texture.id);
    ngli_glDrawArrays(gl, GL_TRIANGLE_FAN, 0, 4);

    if (!(gl->features & NGLI_FEATURE_VERTEX_ARRAY_OBJECT)) {
        ngli_glDisableVertexAttribArray(gl, s->position_location);
        ngli_glDisableVertexAttribArray(gl, s->uvcoord_location);
    }
}

static void text_uninit(struct ngl_node *node)
{
    struct ngl_ctx *ctx = node->ctx;
    struct glcontext *gl = ctx->glcontext;
    struct text_priv *s = node->priv_data;

    if (gl->features & NGLI_FEATURE_VERTEX_ARRAY_OBJECT)
        ngli_glDeleteVertexArrays(gl, 1, &s->vao_id);
    ngli_glDeleteProgram(gl, s->program_id);
    ngli_glDeleteBuffers(gl, 1, &s->vertices_id);
    ngli_glDeleteBuffers(gl, 1, &s->uvcoord_id);
    ngli_texture_reset(&s->texture);
    ngli_free(s->canvas.buf);
}

const struct node_class ngli_text_class = {
    .id        = NGL_NODE_TEXT,
    .name      = "Text",
    .init      = text_init,
    .draw      = text_draw,
    .uninit    = text_uninit,
    .priv_size = sizeof(struct text_priv),
    .params    = text_params,
    .file      = __FILE__,
};
