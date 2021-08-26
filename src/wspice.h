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

#ifndef WIN_SPICE_SERVER_H
#define WIN_SPICE_SERVER_H

#include <glib.h>
#include <pthread.h>
#include <stdint.h>
#include <spice.h>
#include "display.h"
#include "options.h"

typedef struct SimpleSpiceCursor {
    QXLCursorCmd cmd;
    QXLCommandExt ext;
    QXLCursor cursor;
} SimpleSpiceCursor;

typedef struct SimpleSpiceUpdate {
    QXLDrawable drawable;
    QXLImage image;
    QXLCommandExt ext;
    uint8_t *bitmaps;
} SimpleSpiceUpdate;

typedef struct WinSpiceInvalid {
    QXLRect rect;
    uint8_t *bitmaps;
    int pitch;
} WinSpiceInvalid;

struct Session;

typedef struct WSpice {
    struct Session *session;

    Options *options;

    /// drawable queue
    GAsyncQueue *drawable_queue;

    // spice interface
    SpiceServer *server;
    QXLInstance qxl;
    SpiceTabletInstance tablet;

    bool emul0;
    SpiceKbdInstance kbd;

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

    /// function
    void (*start)(struct WSpice *wspice);
    void (*stop)(struct WSpice *wspice);
    void (*wakeup)(struct WSpice *wspice);
    void (*handle_invalid_bitmaps)(struct WSpice *wspice, WinSpiceInvalid *invalid);
    void (*disconnect_client)(struct WSpice *wspice);
} WSpice;

WSpice *wspice_new(struct Session *session);
void wspice_destroy(WSpice *server);
void *create_cursor_update(WSpice *wspice, WinSpiceCursor *c, int on);
//void *bitmaps_to_drawable(uint8_t *bitmaps, QXLRect *rect, int pitch);

#endif /* WIN_SPICE_SERVER_H */
