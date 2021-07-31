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

typedef struct WinDisplay {
    uint32_t width;
    uint32_t height;
    uint32_t accumulated_frames;
    uint32_t total_metadata_buffer_size;
    RECT invalid;
    DXGI_OUTDUPL_FRAME_INFO FrameInfo;
    PTR_INFO *PtrInfo;
    GAsyncQueue *drawable_queue;
    int (*update_changes)(struct WinDisplay *wdisplay);
    void (*release_update_frame)(struct WinDisplay *wdisplay);
    bool (*display_have_updates)(struct WinDisplay *wdisplay);
    int (*find_invalid_region)(struct WinDisplay *wdisplay);
    int (*get_screen_bitmap)(struct WinDisplay *wdisplay, uint8_t **bitmap, int *pitch);

    /// mouse
    bool (*mouse_have_updates)(struct WinDisplay *wdisplay);
    bool (*mouse_have_new_shape)(struct WinDisplay *wdisplay);
    int (*mouse_get_new_shape)(struct WinDisplay *wdisplay, WinSpiceCursor **cursor);
} WinDisplay;

WinDisplay *win_display_new(GAsyncQueue *drawable_queue);
void win_display_free(WinDisplay *wdisplay);

#endif  /* WIN_SPCIE_DISPLAY_H */
