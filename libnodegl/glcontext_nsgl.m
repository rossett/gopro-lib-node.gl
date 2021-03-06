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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <OpenGL/OpenGL.h>
#include <CoreFoundation/CFBundle.h>
#include <Cocoa/Cocoa.h>

#include "fbo.h"
#include "format.h"
#include "glcontext.h"
#include "log.h"
#include "nodegl.h"

struct nsgl_priv {
    NSOpenGLContext *handle;
    NSView *view;
    CFBundleRef framework;
    CVDisplayLinkRef display_link;
    int swap_interval;
    int swap_event;
    NSCondition *swap_condition;
    struct fbo fbo;
    struct texture fbo_color;
    struct texture fbo_depth;
};

static CVReturn display_link_cb(CVDisplayLinkRef display_link,
                                const CVTimeStamp *now,
                                const CVTimeStamp *output_time,
                                CVOptionFlags flags_in,
                                CVOptionFlags *flags_out,
                                void *user_info)
{
    struct glcontext *ctx = user_info;
    struct nsgl_priv *nsgl = ctx->priv_data;

    if (nsgl->swap_interval > 0) {
        [nsgl->swap_condition lock];
        nsgl->swap_event++;
        [nsgl->swap_condition signal];
        [nsgl->swap_condition unlock];
    }

    return kCVReturnSuccess;
}

static int nsgl_init(struct glcontext *ctx, uintptr_t display, uintptr_t window, uintptr_t other)
{
    struct nsgl_priv *nsgl = ctx->priv_data;

    CFBundleRef framework = CFBundleGetBundleWithIdentifier(CFSTR("com.apple.opengl"));
    if (!framework) {
        LOG(ERROR, "could not retrieve OpenGL framework");
        return -1;
    }

    nsgl->framework = (CFBundleRef)CFRetain(framework);
    if (!nsgl->framework) {
        LOG(ERROR, "could not retain OpenGL framework object");
        return -1;
    }

    const NSOpenGLPixelFormatAttribute pixelAttrs[] = {
        NSOpenGLPFAAccelerated,
        NSOpenGLPFAClosestPolicy,
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion4_1Core,
        NSOpenGLPFAColorSize, 24,
        NSOpenGLPFAAlphaSize, 8,
        NSOpenGLPFADepthSize, 24,
        NSOpenGLPFAStencilSize, 8,
        NSOpenGLPFASampleBuffers, ctx->offscreen ? 0 : (ctx->samples > 0),
        NSOpenGLPFASamples, ctx->offscreen ? 0 : ctx->samples,
        0,
    };

    NSOpenGLPixelFormat* pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:pixelAttrs];
    if (!pixelFormat) {
        LOG(ERROR, "could not allocate pixel format");
        return -1;
    }

    NSOpenGLContext *shared_context = other ? (NSOpenGLContext *)other : NULL;
    nsgl->handle = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:shared_context];
    if (!nsgl->handle) {
        LOG(ERROR, "could not create NSGL context");
        return -1;
    }

    if (!ctx->offscreen) {
        nsgl->view = (NSView *)window;
        if (!nsgl->view) {
            LOG(ERROR, "could not retrieve NS view");
            return -1;
        }
        [nsgl->handle setView:nsgl->view];

        CVReturn ret = CVDisplayLinkCreateWithActiveCGDisplays(&nsgl->display_link);
        if (ret != kCVReturnSuccess) {
            LOG(ERROR, "could not create CVDisplayLink");
            return -1;
        }

        ret = CVDisplayLinkSetOutputCallback(nsgl->display_link, display_link_cb, ctx);
        if (ret != kCVReturnSuccess) {
            LOG(ERROR, "could not set CVDisplayLink callback");
            return -1;
        }

        nsgl->swap_condition = [NSCondition new];
        if (!nsgl->swap_condition) {
            LOG(ERROR, "could not allocate swap condition");
            return -1;
        }
    }

    return 0;
}

static int nsgl_init_framebuffer(struct glcontext *ctx)
{
    struct nsgl_priv *nsgl = ctx->priv_data;

    if (!ctx->offscreen)
        return 0;

    struct texture_params attachment_params = NGLI_TEXTURE_PARAM_DEFAULTS;
    attachment_params.format = NGLI_FORMAT_R8G8B8A8_UNORM;
    attachment_params.width = ctx->width;
    attachment_params.height = ctx->height;
    attachment_params.samples = ctx->samples;
    attachment_params.usage = NGLI_TEXTURE_USAGE_ATTACHMENT_ONLY;
    int ret = ngli_texture_init(&nsgl->fbo_color, ctx, &attachment_params);
    if (ret < 0)
        return ret;

    attachment_params.format = NGLI_FORMAT_D24_UNORM_S8_UINT;
    ret = ngli_texture_init(&nsgl->fbo_depth, ctx, &attachment_params);
    if (ret < 0)
        return ret;

    const struct texture *attachments[] = {&nsgl->fbo_color, &nsgl->fbo_depth};
    struct fbo_params fbo_params = {
        .width = ctx->width,
        .height = ctx->height,
        .attachments = attachments,
        .nb_attachments = NGLI_ARRAY_NB(attachments),
    };
    ret = ngli_fbo_init(&nsgl->fbo, ctx, &fbo_params);
    if (ret < 0)
        return ret;

    ret = ngli_fbo_bind(&nsgl->fbo);
    if (ret < 0)
        return ret;

    ngli_glViewport(ctx, 0, 0, ctx->width, ctx->height);

    return 0;
}

static int nsgl_resize(struct glcontext *ctx, int width, int height)
{
    struct nsgl_priv *nsgl = ctx->priv_data;

    [nsgl->handle update];

    NSRect bounds = [nsgl->view bounds];
    ctx->width = bounds.size.width;
    ctx->height = bounds.size.height;

    return 0;
}

static int nsgl_make_current(struct glcontext *ctx, int current)
{
    struct nsgl_priv *nsgl = ctx->priv_data;

    if (current) {
        [nsgl->handle makeCurrentContext];
    } else {
        [NSOpenGLContext clearCurrentContext];
    }

    return 0;
}

static void nsgl_swap_buffers(struct glcontext *ctx)
{
    struct nsgl_priv *nsgl = ctx->priv_data;

    if (nsgl->swap_interval > 0) {
        [nsgl->swap_condition lock];
        do {
            [nsgl->swap_condition wait];
        } while (!nsgl->swap_event);
        nsgl->swap_event = 0;
        [nsgl->swap_condition unlock];
    }

    [nsgl->handle flushBuffer];
}

static int nsgl_set_swap_interval(struct glcontext *ctx, int interval)
{
    struct nsgl_priv *nsgl = ctx->priv_data;

    if (!nsgl->display_link)
        return 0;

    nsgl->swap_interval = interval;
    if (nsgl->swap_interval > 0) {
        CVReturn ret = CVDisplayLinkStart(nsgl->display_link);
        if (ret != kCVReturnSuccess) {
            LOG(ERROR, "could not start display link");
            return -1;
        }
    }

    return 0;
}

static void *nsgl_get_proc_address(struct glcontext *ctx, const char *name)
{
    struct nsgl_priv *nsgl = ctx->priv_data;

    CFStringRef symbol_name = CFStringCreateWithCString(kCFAllocatorDefault, name, kCFStringEncodingASCII);
    if (!symbol_name) {
        return NULL;
    }

    void *symbol_address = CFBundleGetFunctionPointerForName(nsgl->framework, symbol_name);
    CFRelease(symbol_name);

    return symbol_address;
}

static uintptr_t nsgl_get_handle(struct glcontext *ctx)
{
    struct nsgl_priv *nsgl = ctx->priv_data;
    return (uintptr_t)nsgl->handle;
}

static void nsgl_uninit(struct glcontext *ctx)
{
    struct nsgl_priv *nsgl = ctx->priv_data;

    if (nsgl->display_link)
        CVDisplayLinkRelease(nsgl->display_link);

    if (nsgl->swap_condition)
        CFRelease(nsgl->swap_condition);

    ngli_fbo_reset(&nsgl->fbo);
    ngli_texture_reset(&nsgl->fbo_color);
    ngli_texture_reset(&nsgl->fbo_depth);

    if (nsgl->framework)
        CFRelease(nsgl->framework);

    if (nsgl->handle)
        CFRelease(nsgl->handle);
}

const struct glcontext_class ngli_glcontext_nsgl_class = {
    .init = nsgl_init,
    .init_framebuffer = nsgl_init_framebuffer,
    .uninit = nsgl_uninit,
    .resize = nsgl_resize,
    .make_current = nsgl_make_current,
    .swap_buffers = nsgl_swap_buffers,
    .set_swap_interval = nsgl_set_swap_interval,
    .get_proc_address = nsgl_get_proc_address,
    .get_handle = nsgl_get_handle,
    .priv_size = sizeof(struct nsgl_priv),
};
