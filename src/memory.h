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

#ifndef WIN_SPICE_MEMORY_H
#define WIN_SPICE_MEMORY_H

#include <glib.h>

void *_w_malloc(gsize n_bytes, const char *file, int line, const char*func);
void *_w_malloc0(gsize n_bytes, const char *file, int line, const char*func);
void _w_free(void *mem, const char *file, int line, const char*func);
char *_w_strdup(const char *str, const char *file, int line, const char*func);

#define w_malloc(x)        _w_malloc(x, __FILE__, __LINE__, __FUNCTION__)
#define w_malloc0(x)       _w_malloc0(x, __FILE__, __LINE__, __FUNCTION__)
#define w_free(x)          _w_free(x, __FILE__, __LINE__, __FUNCTION__)
#define w_strdup(x)        _w_strdup(x, __FILE__, __LINE__, __FUNCTION__)
    
#endif //WIN_SPICE_MEMORY_H
