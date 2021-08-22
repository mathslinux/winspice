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

/**
 * @file   memory.c
 * @brief  Memory Management
 */

#include <glib.h>
#include <stdio.h>
#include "memory.h"

#ifdef WIN_SPICE_DEBUG
extern FILE *fp_dbg;
#endif // WIN_SPICE_DEBUG

static void memory_track(
    void *pointer, gsize n_bytes, const char *file, int line, const char *func)
{
#ifdef WIN_SPICE_DEBUG
    if (fp_dbg) {
        static char buf[256];
        snprintf(buf, sizeof(buf), "[%s:%d %s]: %p:%d\n",
                 __FILE__, __LINE__, __FUNCTION__, pointer, n_bytes);
        fwrite(&buf, strlen(buf), 1, fp_dbg);
    }
#endif // WIN_SPICE_DEBUG
}

void *_w_malloc(gsize n_bytes, const char *file, int line, const char*func)
{
    void *pointer = g_malloc(n_bytes);
    memory_track(pointer, n_bytes, file, line, func);
    return pointer;
}

void *_w_malloc0(gsize n_bytes, const char *file, int line, const char*func)
{
    void *pointer = g_malloc0(n_bytes);
    memory_track(pointer, n_bytes, file, line, func);
    return pointer;
}

void _w_free(void *mem, const char *file, int line, const char*func)
{
    memory_track(mem, 0, file, line, func);
    g_free(mem);
}

char *_w_strdup(const char *str, const char *file, int line, const char*func)
{
    char *pointer = g_strdup(str);
    memory_track(pointer, 0, file, line, func);
    return pointer;
}
