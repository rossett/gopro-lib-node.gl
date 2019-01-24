/*
 * Copyright 2017 GoPro Inc.
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

#include <float.h>
#include <stddef.h>
#include <string.h>
#include "log.h"
#include "memory.h"
#include "nodegl.h"
#include "nodes.h"

#define OFFSET(x) offsetof(struct buffer_priv, x)
static const struct node_param animatedbuffer_params[] = {
    {"keyframes", PARAM_TYPE_NODELIST, OFFSET(animkf),
                  .node_types=(const int[]){NGL_NODE_ANIMKEYFRAMEBUFFER, -1},
                  .flags=PARAM_FLAG_DOT_DISPLAY_PACKED,
                  .desc=NGLI_DOCSTRING("key frame buffers to interpolate from")},
    {NULL}
};

static int get_kf_id(struct ngl_node **animkf, int nb_animkf, int start, double t)
{
    int ret = -1;

    for (int i = start; i < nb_animkf; i++) {
        const struct animkeyframe_priv *kf = animkf[i]->priv_data;
        if (kf->time > t)
            break;
        ret = i;
    }
    return ret;
}

#define MIX(x, y, a) ((x)*(1.-(a)) + (y)*(a))

static int animatedbuffer_update(struct ngl_node *node, double t)
{
    struct buffer_priv *s = node->priv_data;
    struct ngl_node **animkf = s->animkf;
    const int nb_animkf = s->nb_animkf;
    float *dst = (float *)s->data;

    if (!nb_animkf)
        return 0;
    int kf_id = get_kf_id(animkf, nb_animkf, s->current_kf, t);
    if (kf_id < 0)
        kf_id = get_kf_id(animkf, nb_animkf, 0, t);
    if (kf_id >= 0 && kf_id < nb_animkf - 1) {
        const struct animkeyframe_priv *kf0 = animkf[kf_id    ]->priv_data;
        const struct animkeyframe_priv *kf1 = animkf[kf_id + 1]->priv_data;
        const double t0 = kf0->time;
        const double t1 = kf1->time;

        double tnorm = (t - t0) / (t1 - t0);
        if (kf1->scale_boundaries)
            tnorm = (kf1->offsets[1] - kf1->offsets[0]) * tnorm + kf1->offsets[0];
        double ratio = kf1->function(tnorm, kf1->nb_args, kf1->args);
        if (kf1->scale_boundaries)
            ratio = (ratio - kf1->boundaries[0]) / (kf1->boundaries[1] - kf1->boundaries[0]);

        s->current_kf = kf_id;

        const float *d1 = (const float *)kf0->data;
        const float *d2 = (const float *)kf1->data;
        for (int k = 0; k < s->count; k++)
            for (int i = 0; i < s->data_comp; i++)
                dst[k*s->data_comp + i] = MIX(d1[k*s->data_comp + i], d2[k*s->data_comp + i], ratio);
    } else {
        const struct animkeyframe_priv *kf0 = animkf[            0]->priv_data;
        const struct animkeyframe_priv *kfn = animkf[nb_animkf - 1]->priv_data;
        const struct animkeyframe_priv *kf  = t < kf0->time ? kf0 : kfn;

        memcpy(dst, kf->data, s->data_size);
    }

    return 0;
}

static int animatedbuffer_init(struct ngl_node *node)
{
    struct buffer_priv *s = node->priv_data;
    double prev_time = -DBL_MAX;

    int nb_comp;
    int format;

    switch (node->class->id) {
    case NGL_NODE_ANIMATEDBUFFERFLOAT: nb_comp = 1; format = NGLI_FORMAT_R32_SFLOAT;          break;
    case NGL_NODE_ANIMATEDBUFFERVEC2:  nb_comp = 2; format = NGLI_FORMAT_R32G32_SFLOAT;       break;
    case NGL_NODE_ANIMATEDBUFFERVEC3:  nb_comp = 3; format = NGLI_FORMAT_R32G32B32_SFLOAT;    break;
    case NGL_NODE_ANIMATEDBUFFERVEC4:  nb_comp = 4; format = NGLI_FORMAT_R32G32B32A32_SFLOAT; break;
    default:
        ngli_assert(0);
    }

    s->dynamic = 1;
    s->usage = GL_DYNAMIC_DRAW;
    s->data_comp = nb_comp;
    s->data_format = format;
    s->data_stride = s->data_comp * sizeof(float);

    for (int i = 0; i < s->nb_animkf; i++) {
        const struct animkeyframe_priv *kf = s->animkf[i]->priv_data;
        const int data_count = kf->data_size / s->data_stride;
        const int data_pad   = kf->data_size % s->data_stride;

        if (kf->time < prev_time) {
            LOG(ERROR, "key frames must be monotically increasing: %g < %g",
                kf->time, prev_time);
            return -1;
        }
        prev_time = kf->time;

        if (s->count && s->count != data_count) {
            static const char *types[] = {"float", "vec2", "vec3", "vec4"};
            LOG(ERROR, "the number of %s in buffer key frame %d "
                "does not match the previous ones (%d vs %d)",
                types[s->data_comp - 1], i, data_count, s->count);
            return -1;
        }

        if (data_pad)
            LOG(WARNING, "the data buffer has %d trailing bytes", data_pad);

        s->count = data_count;
    }

    if (!s->count)
        return -1;

    s->data = s->data_chunk = ngli_calloc(s->count, s->data_stride);
    if (!s->data)
        return -1;
    s->data_size = s->data_chunk_size = s->count * s->data_stride;

    return 0;
}

static void animatedbuffer_uninit(struct ngl_node *node)
{
    struct buffer_priv *s = node->priv_data;

    ngli_free(s->data);
    s->data = NULL;
}

const struct node_class ngli_animatedbufferfloat_class = {
    .id        = NGL_NODE_ANIMATEDBUFFERFLOAT,
    .name      = "AnimatedBufferFloat",
    .init      = animatedbuffer_init,
    .update    = animatedbuffer_update,
    .uninit    = animatedbuffer_uninit,
    .priv_size = sizeof(struct buffer_priv),
    .params    = animatedbuffer_params,
    .params_id = "AnimatedBuffer",
    .file      = __FILE__,
};
const struct node_class ngli_animatedbuffervec2_class = {
    .id        = NGL_NODE_ANIMATEDBUFFERVEC2,
    .name      = "AnimatedBufferVec2",
    .init      = animatedbuffer_init,
    .update    = animatedbuffer_update,
    .uninit    = animatedbuffer_uninit,
    .priv_size = sizeof(struct buffer_priv),
    .params    = animatedbuffer_params,
    .params_id = "AnimatedBuffer",
    .file      = __FILE__,
};
const struct node_class ngli_animatedbuffervec3_class = {
    .id        = NGL_NODE_ANIMATEDBUFFERVEC3,
    .name      = "AnimatedBufferVec3",
    .init      = animatedbuffer_init,
    .update    = animatedbuffer_update,
    .uninit    = animatedbuffer_uninit,
    .priv_size = sizeof(struct buffer_priv),
    .params    = animatedbuffer_params,
    .params_id = "AnimatedBuffer",
    .file      = __FILE__,
};
const struct node_class ngli_animatedbuffervec4_class = {
    .id        = NGL_NODE_ANIMATEDBUFFERVEC4,
    .name      = "AnimatedBufferVec4",
    .init      = animatedbuffer_init,
    .update    = animatedbuffer_update,
    .uninit    = animatedbuffer_uninit,
    .priv_size = sizeof(struct buffer_priv),
    .params    = animatedbuffer_params,
    .params_id = "AnimatedBuffer",
    .file      = __FILE__,
};
