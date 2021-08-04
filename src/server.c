/**
 * winspice - windows spice server
 *
 * Copyright 2019 Dunrong Huang <riegamaths@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file   main.c
 * @brief  Window spice server module
 */

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <spice.h>
#include <unistd.h>
#include "server.h"

static bool mouse_server_mode = true;
static guint32 fps = 30;

static inline glong get_tick_count()
{
    gint64 tv;
    tv = g_get_real_time();
    return tv / 1000;
}

static void display_update(WinSpiceServer *server)
{
    void *drawable;
    uint8_t *bitmaps = NULL;
    int bytes_per_line = 0;
    int ret;
    WinSpice *wspice = server->wspice;
    WinDisplay *wdisplay = server->wdisplay;

    if (!wdisplay->display_have_updates(wdisplay)) {
        return ;
    }

    if (wdisplay->find_invalid_region(wdisplay) != 0) {
        return ;
    }

    QXLRect invalid = {
        .left   = wdisplay->invalid.left,
        .top    = wdisplay->invalid.top,
        .right  = wdisplay->invalid.right,
        .bottom = wdisplay->invalid.bottom,
    };

    ret = wdisplay->get_screen_bitmap(wdisplay, &bitmaps, &bytes_per_line);
    if (ret != 0) {
        return ;
    }
    drawable = bitmaps_to_drawable(bitmaps, &invalid, bytes_per_line);
    if (drawable) {
        g_async_queue_push(server->drawable_queue, drawable);
        wspice->wakeup(wspice);
    }
    wdisplay->clear_invalid_region(wdisplay);
}

static void mouse_update(WinSpiceServer *server)
{
    WinSpice *wspice = server->wspice;
    WinDisplay *wdisplay = server->wdisplay;

    if (!wdisplay->mouse_have_updates(wdisplay)) {
        return ;
    }

    if (wdisplay->mouse_have_new_shape(wdisplay)) {
        /// define mouse point
        WinSpiceCursor *cursor = NULL;
        if (wdisplay->mouse_get_new_shape(wdisplay, &cursor) != 0) {
            return ;
        }
        pthread_mutex_lock(&wspice->lock);
        wspice->hot_x = cursor->hot_x;
        wspice->hot_y = cursor->hot_y;
        if (cursor->ptr_type == DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MONOCHROME) {
            /* FIXME: bug if type if DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MONOCHROME */
            wspice->ptr_type = SPICE_CURSOR_TYPE_MONO;
        } else {
            wspice->ptr_type = SPICE_CURSOR_TYPE_ALPHA;
        }
        g_free(wspice->ptr_move);
        wspice->ptr_move = NULL;
        wspice->ptr_define = create_cursor_update(wspice, cursor, 0);
        pthread_mutex_unlock(&wspice->lock);
        free(cursor);
    } else if (!mouse_server_mode) {
        /// mouse client mode
        PTR_INFO *PtrInfo = wdisplay->PtrInfo; /* FIXME: use struct */
        pthread_mutex_lock(&wspice->lock);
        wspice->ptr_x = PtrInfo->Position.x;
        wspice->ptr_y = PtrInfo->Position.y;
        g_free(wspice->ptr_move);
        wspice->ptr_move = create_cursor_update(wspice, NULL, wdisplay->FrameInfo.PointerPosition.Visible);
        pthread_mutex_unlock(&wspice->lock);
    }
}

static void *win_spice_update_thread(void *arg)
{
    WinSpiceServer *server;
    WinDisplay *wdisplay;
    glong begin, end, diff;
    guint32 rate = 1000 / fps;

    server = (WinSpiceServer *)arg;
    wdisplay = server->wdisplay;
    while (1) {
        int ret;
        begin = get_tick_count();

        ret = wdisplay->update_changes(wdisplay);
        if (ret == 0) {
            display_update(server);
            mouse_update(server);
            wdisplay->release_update_frame(wdisplay);
        }

        end = get_tick_count();
        diff = end - begin;
        if (diff < rate) {
            g_usleep(1000 * (rate - diff));
        }
    }

    return NULL;
}

static void start(WinSpiceServer *server)
{
    GMainLoop *loop;
    pthread_t pid;
    pthread_create(&pid, NULL, win_spice_update_thread, server);

    loop = g_main_loop_new(server->main_context, FALSE);
    g_main_loop_run(loop);
}

WinSpiceServer *win_spice_server_new()
{
    WinSpiceServer *server = (WinSpiceServer *)malloc(sizeof(WinSpiceServer));
    memset(server, 0, sizeof(*server));
    if (!server) {
        printf("failed to alloc memory for winspiceserver\n");
        goto failed;
    }
    server->port = 5900;        /* TODO: */

    server->drawable_queue = g_async_queue_new();

    server->main_context = g_main_context_default();

    /// display init
    server->wdisplay = win_display_new(server->drawable_queue);
    if (!server->wdisplay) {
        printf("Failed to new display\n");
        goto failed;
    }

    /// spice init
    /* TODO: set primary dynamically */
    server->wspice = win_spice_new(server->drawable_queue, server->wdisplay->width, server->wdisplay->height);
    if (!server->wspice) {
        goto failed;
    }

    server->start = start;
    server->stop = NULL;

    return server;

failed:
    if (server) {
        if (server->wdisplay) {
            win_display_free(server->wdisplay);
        }
        if (server->wspice) {
            win_spice_free(server->wspice);
        }
        win_spice_server_free(server);
    }
    return NULL;
}

void win_spice_server_free(WinSpiceServer *server)
{
    if (!server) {
        return;
    }
    free(server);
    printf("FIXME! %s UNIMPLEMENTED!\n", __FUNCTION__);
    return;
}
