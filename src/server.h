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
#include "wspice.h"
#include "display.h"
#include "options.h"

typedef struct WinSpiceServer {
    WinSpiceOption *options;

    /// drawable queue
    GAsyncQueue *drawable_queue;

    GMainContext *main_context;

    /// display
    WinDisplay *wdisplay;

    /// spice
    /* spice_context */
    WinSpice *wspice;

    /// function
    void (*start)(struct WinSpiceServer *server);
    void (*stop)(struct WinSpiceServer *server);
} WinSpiceServer;

WinSpiceServer *win_spice_server_new(WinSpiceOption *options);
void win_spice_server_free(WinSpiceServer *server);

#endif /* WIN_SPICE_SERVER_H */
