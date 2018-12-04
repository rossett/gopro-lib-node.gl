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

#include <stddef.h>
#include <stdlib.h>
#include <pthread.h>

#include "log.h"
#include "nodegl.h"
#include "nodes.h"


#define OFFSET(x) offsetof(struct async, x)
static const struct node_param async_params[] = {
    {"child", PARAM_TYPE_NODE, OFFSET(child),
              .flags=PARAM_FLAG_CONSTRUCTOR,
              .desc=NGLI_DOCSTRING("scene to be render asynchronously")},
    {NULL}
};

cmd_init();
cmd_draw();

static int cmd_stop(struct ngl_ctx *s, void *arg)
{
    return 0;
}

static int dispatch_cmd(struct ngl_ctx *s, cmd_func_type cmd_func, void *arg)
{
    pthread_mutex_lock(&s->lock);
    s->cmd_func = cmd_func;
    s->cmd_arg = arg;
    pthread_cond_signal(&s->cond_wkr);
    while (s->cmd_func)
        pthread_cond_wait(&s->cond_ctl, &s->lock);
    pthread_mutex_unlock(&s->lock);

    return s->cmd_ret;
}

static void *worker_thread(void *arg)
{
    struct ngl_ctx *s = arg;

    ngli_thread_set_name("ngl-thread");

    pthread_mutex_lock(&s->lock);
    for (;;) {
        while (!s->cmd_func)
            pthread_cond_wait(&s->cond_wkr, &s->lock);
        s->cmd_ret = s->cmd_func(s, s->cmd_arg);
        int need_stop = s->cmd_func == cmd_stop;
        s->cmd_func = s->cmd_arg = NULL;
        pthread_cond_signal(&s->cond_ctl);

        if (need_stop)
            break;
    }
    pthread_mutex_unlock(&s->lock);

    return NULL;
}

static int async_init(struct ngl_node *node)
{
    struct async *s = node->priv_data;

    int ret;
    if ((ret = pthread_mutex_init(&s->lock, NULL)) ||
        (ret = pthread_cond_init(&s->cond_ctl, NULL)) ||
        (ret = pthread_cond_init(&s->cond_wkr, NULL)) ||
        (ret = pthread_create(&s->worker_tid, NULL, worker_thread, s))) {
        pthread_cond_destroy(&s->cond_ctl);
        pthread_cond_destroy(&s->cond_wkr);
        pthread_mutex_destroy(&s->lock);
        return ret;
    }


    s->ngl_ctx = ngl_create();
    if (!s->ngl_ctx)
        return -1;

    s->ngl_config.platform = node->ctx->config.platform;
    s->ngl_config.backend = node->ctx->config.backend;
    s->ngl_config.display = node->ctx->config.display;
    s->ngl_config.window = node->ctx->config.window;
    s->ngl_config.handle = node->ctx->config.handle;
    s->ngl_config.swap_interval = 0;
    s->ngl_config.offscreen = 1;
    s->ngl_config.width = 1920;
    s->ngl_config.height = 1080;
    s->ngl_config.samples = node->ctx->config.samples;

    int ret = ngl_configure(s->ngl_ctx, &s->ngl_config);
    if (ret < 0)
        return -1;

    ret = ngl_set_scene(s->ngl_ctx, s->child);
    if (ret < 0)
        return ret;

    return 0;
}

static void async_uninit(struct ngl_node *node)
{
    struct async *s = node->priv_data;

    ngl_freep(&s->ngl_ctx);
}

static int async_visit(struct ngl_node *node, int is_active, double t)
{
    return 0;
}

static int async_update(struct ngl_node *node, double t)
{
    struct async *s = node->priv_data;

    ngl_draw(s->ngl_ctx, t);

    return 0;
}

static void async_draw(struct ngl_node *node)
{
}

const struct node_class ngli_async_class = {
    .id        = NGL_NODE_ASYNC,
    .name      = "Async",
    .init      = async_init,
    .visit     = async_visit,
    .update    = async_update,
    .draw      = async_draw,
    .uninit    = async_uninit,
    .priv_size = sizeof(struct async),
    .params    = async_params,
    .file      = __FILE__,
};
