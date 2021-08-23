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

#include "session.h"
#include "memory.h"

static bool mouse_server_mode = true;
static guint32 fps = 30;

static inline glong get_tick_count()
{
    gint64 tv;
    tv = g_get_real_time();
    return tv / 1000;
}

Session *session_new(int argc, char **argv)
{
    Session *session = NULL;

    session = w_malloc0(sizeof(Session));
    if (!session) {
        goto failed;
    }

    session->app_path = w_strdup(argv[0]);

    /// options init
    session->options = options_new();
    if (!session->options) {
        printf("Failed to create winspice option\n");
        goto failed;
    }

    /// display init
    session->update_thread_running = FALSE;
    session->display = display_new();
    if (!session->display) {
        printf("Failed to new display\n");
        goto failed;
    }

    /**
     * NOTE: wpice can only be initialized after the display is initialized,
     * because it needs to get some data from the display when it is
     * initialized, such as primary_width.
     */
    /// wspice init:
    session->wspice = wspice_new(session);
    if (!session->wspice) {
        printf("Failed to new wspice\n");
        goto failed;
    }

    session->running = FALSE;
    return session;

failed:
    if (session) {
        session_destroy(session);
    }
    return NULL;
}

static void display_update(Session *session)
{
    //void *drawable;
    uint8_t *bitmaps = NULL;
    int pitch = 0;
    WSpice *wspice = session->wspice;
    Display *display = session->display;
    WinSpiceInvalid invalid;

    /**
     * NOTE: In order to improve performance, bitmaps will be freed
     * in wspice context
     */
    if (!display->get_invalid_bitmap(display, &bitmaps, &pitch)) {
        return ;
    }

    memset(&invalid, 0, sizeof(invalid));
    invalid.rect.left   = display->invalid.left;
    invalid.rect.top    = display->invalid.top;
    invalid.rect.right  = display->invalid.right;
    invalid.rect.bottom = display->invalid.bottom;
    invalid.bitmaps     = bitmaps;
    invalid.pitch       = pitch;
    wspice->handle_invalid_bitmaps(wspice, &invalid);

    display->clear_invalid_region(display);
}

static void mouse_update(Session *session)
{
    WSpice *wspice = session->wspice;
    Display *display = session->display;

    if (!display->mouse_have_updates(display)) {
        return ;
    }

    if (display->mouse_have_new_shape(display)) {
        /// define mouse point
        WinSpiceCursor *cursor = NULL;
        if (display->mouse_get_new_shape(display, &cursor) != 0) {
            return ;
        }
        /// TODO: 在 wspice 模块封装函数处理
        pthread_mutex_lock(&wspice->lock);
        wspice->hot_x = cursor->hot_x;
        wspice->hot_y = cursor->hot_y;
        if (cursor->ptr_type == DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MONOCHROME) {
            /* FIXME: bug if type if DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MONOCHROME */
            wspice->ptr_type = SPICE_CURSOR_TYPE_MONO;
        } else {
            wspice->ptr_type = SPICE_CURSOR_TYPE_ALPHA;
        }
        w_free(wspice->ptr_move);
        wspice->ptr_move = NULL;
        wspice->ptr_define = create_cursor_update(wspice, cursor, 0);
        pthread_mutex_unlock(&wspice->lock);
        w_free(cursor);
    } else if (!mouse_server_mode) {
        /// mouse client mode
        PTR_INFO *PtrInfo = display->PtrInfo; /* FIXME: use struct */
        pthread_mutex_lock(&wspice->lock);
        wspice->ptr_x = PtrInfo->Position.x;
        wspice->ptr_y = PtrInfo->Position.y;
        w_free(wspice->ptr_move);
        wspice->ptr_move = create_cursor_update(wspice, NULL, display->FrameInfo.PointerPosition.Visible);
        pthread_mutex_unlock(&wspice->lock);
    }
}

static void *display_update_thread(void *arg)
{
    Session *session;
    Display *display;
    glong begin, end, diff;
    guint32 rate = 1000 / fps;

    session = (Session *)arg;
    display = session->display;
    session->update_thread_running = TRUE;
    while (session->running) {
        int ret;
        begin = get_tick_count();

        ret = display->update_changes(display);
        if (ret == 0) {
            display_update(session);
            mouse_update(session);
            display->release_update_frame(display);
        }

        end = get_tick_count();
        diff = end - begin;
        if (diff < rate) {
            g_usleep(1000 * (rate - diff));
        }
    }

    session->update_thread_running = FALSE;

    return NULL;
}

void session_start(Session *session)
{
    pthread_t pid;

    session->running = TRUE;
    /// start spice server
    /// note: wspice must run before display thread since display need to
    /// wakeup spice server
    session->wspice->start(session->wspice);

    /// start display thread
    pthread_create(&pid, NULL, display_update_thread, session);
}

void session_stop(Session *session)
{
    /// stop display thread
    session->running = FALSE;
    if (session->update_thread_running) {
        /**
         * For convenience, only a bool variable update_thread_running
         * is used here to control the start or stop of the update thread,
         * instead of using the pthread_cancel related function.
         * We must ensure that the thread has exited here.
         */
        g_usleep(1000 * 10);
    }

    /// stop wspice thread
    session->wspice->stop(session->wspice);
}

void session_destroy(Session *session)
{
    if (session) {
        session_stop(session);

        if (session->display) {
            display_destroy(session->display);
        }

        if (session->wspice) {
            wspice_destroy(session->wspice);
        }

        if (session->app_path) {
            w_free(session->app_path);
        }

        if (session->options) {
            options_destroy(session->options);
        }

        w_free(session);
    }
}
