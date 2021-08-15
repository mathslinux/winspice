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
#include "wspice.h"
#include "gui.h"
#include "session.h"

/// FIXME: ugly hack
/// save application path globally

int main(int argc, char *argv[])
{
    GUI *gui = NULL;
    int rc = 0;
    Session *session = NULL;

    /* TODO: log_init */
    printf("winspice init\n");

    session = session_new(argc, argv);
    if (!session) {
        printf("Failed to createsession\n");
        rc = -1;
        goto quit;
    }

    gui = gui_new(session);
    if (!gui) {
        printf("Failed to create gui\n");
        rc = -1;
        goto quit;
    }

    gui_run(gui);

quit:
    if (session) {
        session_destroy(session);
    }
    if (gui) {
        gui_destroy(gui);
    }

    return rc;
}

