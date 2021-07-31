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

#ifndef WIN_SPICE_WSPICE_H
#define WIN_SPICE_WSPICE_H

#include <spice.h>
#include <glib.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

typedef struct SimpleSpiceCursor {
    QXLCursorCmd cmd;
    QXLCommandExt ext;
    QXLCursor cursor;
} SimpleSpiceCursor;

typedef struct WinSpice {
    SpiceServer *server;
    QXLInstance qxl;
    SpiceTabletInstance tablet;

    bool emul0;
    SpiceKbdInstance kbd;

    uint32_t port;
    GAsyncQueue *drawable_queue;
    SimpleSpiceCursor *ptr_define;
    SimpleSpiceCursor *ptr_move;
    int ptr_x, ptr_y;
    int hot_x, hot_y;
    int ptr_type;

    uint32_t last_bmask;

    pthread_mutex_t lock;

    uint8_t *primary_surface;
    int primary_surface_size;
    int primary_width;
    int primary_height;

    /* spice_context */
    void (*start)(struct WinSpice *wspice);
    void (*stop)(struct WinSpice *wspice);
    void (*wakeup)(struct WinSpice *wspice);
} WinSpice;

/* cursor data format is 32bit RGBA */
typedef struct WinSpiceCursor {
    int                 width, height;
    int                 hot_x, hot_y;
    int                 ptr_type;
    uint32_t            data[];
} WinSpiceCursor;

void *bitmaps_to_drawable(uint8_t *bitmaps, QXLRect *rect, int bytes_per_line);
void *create_cursor_update(WinSpice *wspice, WinSpiceCursor *c, int on);

WinSpice *win_spice_new(GAsyncQueue *drawable_queue, int primary_width, int primary_height);

void win_spice_free(WinSpice *wspice);

#endif  /* WIN_SPICE_WSPICE_H */
