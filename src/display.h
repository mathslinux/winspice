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

#ifndef WIN_SPICE_DISPLAY_H
#define WIN_SPICE_DISPLAY_H
#include <glib.h>
#include <stdbool.h>
#include <dxgi1_2.h>
#include <stdint.h>
#include <windows.h>

typedef struct _PTR_INFO
{
    _Field_size_bytes_(BufferSize) BYTE* PtrShapeBuffer;
    DXGI_OUTDUPL_POINTER_SHAPE_INFO ShapeInfo;
    POINT Position;
    bool Visible;
    UINT BufferSize;
    UINT WhoUpdatedPositionLast;
    LARGE_INTEGER LastTimeStamp;
} PTR_INFO;

/* cursor data format is 32bit RGBA */
typedef struct WinSpiceCursor {
    int                 width, height;
    int                 hot_x, hot_y;
    int                 ptr_type;
    uint32_t            data[];
} WinSpiceCursor;

typedef struct Display {
    uint32_t width;
    uint32_t height;
    uint32_t accumulated_frames;
    uint32_t total_metadata_buffer_size;
    RECT invalid;
    DXGI_OUTDUPL_FRAME_INFO FrameInfo;
    PTR_INFO *PtrInfo;
    int (*update_changes)(struct Display *display);
    void (*release_update_frame)(struct Display *display);
    bool (*display_have_updates)(struct Display *display);
    bool (*find_invalid_region)(struct Display *display);
    void (*clear_invalid_region)(struct Display *display);
    bool (*get_screen_bitmap)(struct Display *display, uint8_t **bitmap, int *pitch);
    bool (*get_invalid_bitmap)(struct Display *display, uint8_t **bitmap, int *pitch);

    /// mouse
    bool (*mouse_have_updates)(struct Display *display);
    bool (*mouse_have_new_shape)(struct Display *display);
    int (*mouse_get_new_shape)(struct Display *display, WinSpiceCursor **cursor);
} Display;

Display *display_new();
void display_destroy(Display *display);

#endif  /* WIN_SPCIE_DISPLAY_H */
