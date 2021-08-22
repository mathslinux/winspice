/**
 * winspice - windows spice server
 *
 * Copyright 2021 Dunrong Huang <riegamaths@gmail.com>
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

#include "options.h"
#include <string.h>
#include <glib.h>
#include <spice.h>
#include "memory.h"

Options *options_new()
{
    Options *options;
    options = (Options *)w_malloc0(sizeof(Options));
    if (!options) {
        return NULL;
    }
    /// TODO: get from config
    options->port = 5900;
    options->ssl = false;

    options->compression_name_list = g_list_append(options->compression_name_list, "auto_glz");
    options->compression_name_list = g_list_append(options->compression_name_list, "auto_lz");
    options->compression_name_list = g_list_append(options->compression_name_list, "quic");
    options->compression_name_list = g_list_append(options->compression_name_list, "glz");
    options->compression_name_list = g_list_append(options->compression_name_list, "lz");
    options->compression_name_list = g_list_append(options->compression_name_list, "lz4");
    options->compression_name_list = g_list_append(options->compression_name_list, "off");

    options->compression_list = g_list_append(options->compression_list, GINT_TO_POINTER(SPICE_IMAGE_COMPRESSION_AUTO_GLZ));
    options->compression_list = g_list_append(options->compression_list, GINT_TO_POINTER(SPICE_IMAGE_COMPRESSION_AUTO_LZ));
    options->compression_list = g_list_append(options->compression_list, GINT_TO_POINTER(SPICE_IMAGE_COMPRESSION_QUIC));
    options->compression_list = g_list_append(options->compression_list, GINT_TO_POINTER(SPICE_IMAGE_COMPRESSION_GLZ));
    options->compression_list = g_list_append(options->compression_list, GINT_TO_POINTER(SPICE_IMAGE_COMPRESSION_LZ));
    options->compression_list = g_list_append(options->compression_list, GINT_TO_POINTER(SPICE_IMAGE_COMPRESSION_LZ4));
    options->compression_list = g_list_append(options->compression_list, GINT_TO_POINTER(SPICE_IMAGE_COMPRESSION_OFF));

    return options;
}

char *options_get_string(Options *options, const char *key)
{
    if (!options || !key) {
        return NULL;
    }

    if (!strcmp(key, "password")) {
        return options->password;
    } else {
        return NULL;
    }
    return NULL;
}

void options_set_string(Options *options, const char *key, const char *value)
{
    if (!options || !key || !value) {
        return ;
    }

    if (!strcmp(key, "password")) {
        options->password = w_strdup(value);
    } else {
        /// TODO: print a warning message
        return ;
    }
}

int options_get_int(Options *options, const char *key)
{
    if (!options || !key) {
        return -1;
    }

    if (!strcmp(key, "port")) {
        return options->port;
    } else if (!strcmp(key, "compression")) {
        return options->compression;
    }
    return -1;
}

void options_set_int(Options *options, const char *key, int value)
{
    if (!options || !key) {
        return ;
    }

    if (!strcmp(key, "port")) {
        options->port = value;
    } else if (!strcmp(key, "compression")) {
        options->compression = value;
    } else {
        /// TODO: print a warning message
    }
}

void options_destroy(Options *options)
{
    if (options) {
        w_free(options->password);
        w_free(options);
    }
}

/* TODO: other options operations */
/*
bool options_get_bool();
*/
