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


#ifndef WIN_SPICE_SESSION_H
#define WIN_SPICE_SESSION_H

#include "wspice.h"
#include "options.h"
#include "gui.h"

typedef struct Session {
    Options *options;
    WSpice *wspice;
    GUI *gui;

    char *app_path;

    gboolean running;

    /// display
    gboolean update_thread_running;
    Display *display;
} Session;

Session *session_new(int argc, char **argv);
void session_destroy(Session *session);

void session_start(Session *session);
void session_stop(Session *session);

void session_disconnect_client(Session *session);
void session_client_connected(Session *session);
void session_client_disconnected(Session *session);

#endif  /* WIN_SPICE_SESSION_H */

