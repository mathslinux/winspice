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

WinSpiceSession *session_new(int argc, char **argv)
{
    WinSpiceSession *session = NULL;

    session = g_malloc0(sizeof(WinSpiceSession));
    if (!session) {
        goto failed;
    }

    session->app_path = g_strdup(argv[0]);

    session->options = winspice_options_new();
    if (!session->options) {
        printf("Failed to create winspice option\n");
        goto failed;
    }

    return session;

failed:
    session_destroy(session);
    return NULL;
}

void session_start(WinSpiceSession *session)
{
    session->server = win_spice_server_new(session->options);
    if (!session->server) {
        /// TODO: handle it.
        return ;
    }
    session->server->start(session->server);
}

void session_destroy(WinSpiceSession *session)
{
    if (session) {
        g_free(session->app_path);
        if (session->options) {
            winspice_options_free(session->options);
        }
        if (session->server) {
            win_spice_server_free(session->server);
        }
        g_free(session);
    } 
}
