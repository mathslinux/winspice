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

#ifndef WIN_SPICE_GUI_H
#define WIN_SPICE_GUI_H

#include <gtk/gtk.h>
#include "options.h"

typedef struct Session session;

typedef struct GUI {
    GtkWidget *window;
    GtkWidget *main_box;
    GtkWidget *arguments_grid;
    GtkWidget *port_label;
    GtkWidget *port_entry;
    GtkWidget *password_label;
    GtkWidget *password_entry;
    GtkWidget *compression_label;
    GtkWidget *compression_entry;
    GtkWidget *status_label;
    GtkWidget *start_button;
    GtkWidget *disconnect_button;
    GtkWidget *button_box;
} GUI;

GUI *gui_new(struct Session *session);
void gui_run(GUI *gui);
void gui_destroy(GUI *gui);

#endif  /* WIN_SPICE_GUI_H */
