/*
 * Copyright 2018 GoPro Inc.
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

#include <string.h>

#include "log.h"
#include "nodes.h"
#include "backend.h"
#include "glcontext.h"
#include "memory.h"

#if defined(HAVE_VAAPI_X11)
#include "vaapi.h"
#endif

static int capture_init(struct ngl_ctx *s)
{
    struct glcontext *gl = s->glcontext;
    struct ngl_config *config = &s->config;
    if (!config->capture_buffer)
        return 0;

    if (gl->features & NGLI_FEATURE_FRAMEBUFFER_OBJECT) {
        struct texture_params attachment_params = NGLI_TEXTURE_PARAM_DEFAULTS;
        attachment_params.format = NGLI_FORMAT_R8G8B8A8_UNORM;
        attachment_params.width = config->width;
        attachment_params.height = config->height;
        attachment_params.usage = NGLI_TEXTURE_USAGE_ATTACHMENT_ONLY;
        int ret = ngli_texture_init(&s->capture_fbo_color, gl, &attachment_params);
        if (ret < 0)
            return ret;

        const struct texture *attachments[] = {&s->capture_fbo_color};
        const int nb_attachments = NGLI_ARRAY_NB(attachments);

        struct fbo_params fbo_params = {
            .width = config->width,
            .height = config->height,
            .nb_attachments = nb_attachments,
            .attachments = attachments,
        };
        ret = ngli_fbo_init(&s->capture_fbo, gl, &fbo_params);
        if (ret < 0)
            return ret;
    } else {
        s->capture_buffer = ngli_calloc(4 /* RGBA */, config->width * config->height);
        if (!s->capture_buffer)
            return -1;
    }

    return 0;
}

static int capture(struct ngl_ctx *s)
{
    struct glcontext *gl = s->glcontext;
    struct ngl_config *config = &s->config;
    if (!config->capture_buffer)
        return 0;

    struct fbo *main_fbo = ngli_glcontext_get_framebuffer(gl);
    if (!main_fbo)
        return -1;

    if (gl->features & NGLI_FEATURE_FRAMEBUFFER_OBJECT) {
        struct fbo *capture_fbo = &s->capture_fbo;
        ngli_fbo_blit(main_fbo, capture_fbo, 1);
        ngli_fbo_bind(capture_fbo);
        ngli_fbo_read_pixels(capture_fbo, config->capture_buffer);
        ngli_fbo_unbind(capture_fbo);
    } else {
        ngli_fbo_read_pixels(main_fbo, s->capture_buffer);
        const int linesize = config->width * 4;
        for (int i = 0; i < config->height; i++) {
            const int line = config->height - i - 1;
            const uint8_t *src = s->capture_buffer + line * linesize;
            uint8_t *dst = config->capture_buffer + i * linesize;
            memcpy(dst, src, linesize);
        }
    }
    return 0;
}

static void capture_reset(struct ngl_ctx *s)
{
    ngli_fbo_reset(&s->capture_fbo);
    ngli_texture_reset(&s->capture_fbo_color);
}

static int gl_reconfigure(struct ngl_ctx *s, const struct ngl_config *config)
{
    int ret = ngli_glcontext_resize(s->glcontext, config->width, config->height);
    if (ret < 0)
        return ret;

    struct ngl_config *current_config = &s->config;
    current_config->width = config->width;
    current_config->height = config->height;

    const int *viewport = config->viewport;
    if (viewport[2] > 0 && viewport[3] > 0) {
        ngli_glViewport(s->glcontext, viewport[0], viewport[1], viewport[2], viewport[3]);
        memcpy(current_config->viewport, config->viewport, sizeof(config->viewport));
    }

    const float *rgba = config->clear_color;
    ngli_glClearColor(s->glcontext, rgba[0], rgba[1], rgba[2], rgba[3]);
    memcpy(current_config->clear_color, config->clear_color, sizeof(config->clear_color));

    return 0;
}

static int gl_configure(struct ngl_ctx *s, const struct ngl_config *config)
{
    memcpy(&s->config, config, sizeof(s->config));

    if (!config->offscreen && config->capture_buffer) {
        LOG(ERROR, "capture_buffer is only supported with offscreen rendering");
        return -1;
    }

    s->glcontext = ngli_glcontext_new(&s->config);
    if (!s->glcontext)
        return -1;

    if (s->config.swap_interval >= 0)
        ngli_glcontext_set_swap_interval(s->glcontext, s->config.swap_interval);

    ngli_glstate_probe(s->glcontext, &s->glstate);

    const int *viewport = config->viewport;
    if (viewport[2] > 0 && viewport[3] > 0)
        ngli_glViewport(s->glcontext, viewport[0], viewport[1], viewport[2], viewport[3]);

    const float *rgba = config->clear_color;
    ngli_glClearColor(s->glcontext, rgba[0], rgba[1], rgba[2], rgba[3]);

    int ret;
#if defined(HAVE_VAAPI_X11)
    ret = ngli_vaapi_init(s);
    if (ret < 0)
        LOG(WARNING, "could not initialize vaapi");
#endif

    ret = capture_init(s);
    if (ret < 0)
        return ret;

    return 0;
}

static int gl_pre_draw(struct ngl_ctx *s, double t)
{
    const struct glcontext *gl = s->glcontext;

    ngli_glClear(gl, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    return 0;
}

static int gl_post_draw(struct ngl_ctx *s, double t)
{
    struct glcontext *gl = s->glcontext;

    int ret = capture(s);
    if (ret < 0)
        LOG(ERROR, "could not capture framebuffer");

    if (ngli_glcontext_check_gl_error(gl, __FUNCTION__))
        ret = -1;

    if (gl->set_surface_pts)
        ngli_glcontext_set_surface_pts(gl, t);

    ngli_glcontext_swap_buffers(gl);

    return ret;
}

static void gl_destroy(struct ngl_ctx *s)
{
    capture_reset(s);
#if defined(HAVE_VAAPI_X11)
    ngli_vaapi_reset(s);
#endif
    ngli_glcontext_freep(&s->glcontext);
}

const struct backend ngli_backend_gl = {
    .name         = "OpenGL",
    .reconfigure  = gl_reconfigure,
    .configure    = gl_configure,
    .pre_draw     = gl_pre_draw,
    .post_draw    = gl_post_draw,
    .destroy      = gl_destroy,
};

const struct backend ngli_backend_gles = {
    .name         = "OpenGL ES",
    .reconfigure  = gl_reconfigure,
    .configure    = gl_configure,
    .pre_draw     = gl_pre_draw,
    .post_draw    = gl_post_draw,
    .destroy      = gl_destroy,
};
