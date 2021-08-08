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
 * @brief  The main program for winspice
 */

#include <glib.h>
#include <stdio.h>
#include "server.h"

int main(int argc, char *argv[])
{
    WinSpiceServer *server = NULL;
    WinSpiceOption *options = NULL;

    /* TODO: log_init */
    printf("winspice init\n");

    /* TODO: program arguments */

    options = winspice_options_new();
    if (!options) {
        printf("Failed to create winspice option\n");
        return -1;
    }

    server = win_spice_server_new(options);
    if (!server) {
        printf("failed to create windows spice server\n");
        return -1;
    }

    server->start(server);

    win_spice_server_free(server);

    return 0;
}

