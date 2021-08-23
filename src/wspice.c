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
 * @file   wspice.c
 * @brief  Window spice server module
 */

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <spice.h>
#include <unistd.h>
#include "wspice.h"
#include "session.h"
#include "memory.h"

static void create_primary_surface(WSpice *wspice)
{
    QXLDevSurfaceCreate surface;
    int current_surface_size;

    g_assert(wspice->primary_width > 0);
    g_assert(wspice->primary_height > 0);

    current_surface_size = wspice->primary_width * wspice->primary_height * 4;
    if (wspice->primary_surface_size < current_surface_size) {
        wspice->primary_surface_size  = current_surface_size;
        if (wspice->primary_surface) {
            w_free(wspice->primary_surface);
        }
        wspice->primary_surface = (uint8_t *)w_malloc(wspice->primary_surface_size);
    }

    memset(&surface, 0, sizeof(surface));
    surface.format     = SPICE_SURFACE_FMT_32_xRGB;
    surface.width      = wspice->primary_width;
    surface.height     = wspice->primary_height;
    surface.stride     = -4 * wspice->primary_width; /* negative? */
    surface.mouse_mode = TRUE;  /* unused by red_worker */
    surface.flags      = 0;
    surface.type       = 0;     /* unused by red_worker */
    surface.position   = 0;     /* unused by red_worker */
    surface.mem        = (uintptr_t)wspice->primary_surface;
    surface.group_id   = 0;

    spice_qxl_create_primary_surface(&wspice->qxl, 0, &surface);
}

static void attache_worker(QXLInstance *qin, QXLWorker *_qxl_worker)
{
    static QXLWorker *qxl_worker = NULL;
    WSpice *wspice = SPICE_CONTAINEROF(qin, WSpice, qxl);

    g_print("attach new one\n");

    if (qxl_worker) {
        if (qxl_worker != _qxl_worker)
            printf("%s ignored, %p is set, ignoring new %p\n", __func__,
                   qxl_worker, _qxl_worker);
        else
            printf("%s ignored, redundant\n", __func__);
        return;
    } else {
        qxl_worker = _qxl_worker;
    }
    static QXLDevMemSlot slot = {
        .slot_group_id = 0,
        .slot_id = 0,
        .generation = 0,
        .virt_start = 0,
        .virt_end = ~0,
        .addr_delta = 0,
        .qxl_ram_size = ~0,
    };
    spice_qxl_add_memslot(qin, &slot);
    create_primary_surface(wspice);
    spice_server_vm_start(wspice->server);
}

static void set_compression_level(QXLInstance *qin G_GNUC_UNUSED, int level G_GNUC_UNUSED)
{
    printf("FIXME! UNIMPLEMENTED! %s\n", __func__);
}

static void get_init_info(QXLInstance *qin G_GNUC_UNUSED, QXLDevInitInfo *info)
{
    memset(info, 0, sizeof(*info));
    info->num_memslots = 1;
    info->num_memslots_groups = 1;
    info->memslot_id_bits = 1;
    info->memslot_gen_bits = 1;
    info->n_surfaces = 1;
}

static int get_command(QXLInstance *qin G_GNUC_UNUSED, struct QXLCommandExt *ext)
{
    WSpice *wspice = SPICE_CONTAINEROF(qin, WSpice, qxl);
    SimpleSpiceUpdate *update;

    update = g_async_queue_try_pop(wspice->drawable_queue);
    if (!update) {
        return false;
    }

    *ext = update->ext;
    return true;
}

static int req_cmd_notification(QXLInstance *qin G_GNUC_UNUSED)
{
    WSpice *wspice = SPICE_CONTAINEROF(qin, WSpice, qxl);
    if (g_async_queue_length(wspice->drawable_queue) > 0) {
        return 0;
    }
    return 1;
}

static void release_resource(QXLInstance *qin G_GNUC_UNUSED,
                             struct QXLReleaseInfoExt release_info)
{
    SimpleSpiceUpdate *update;
    SimpleSpiceCursor *cursor;
    QXLCommandExt *ext;

    ext = (void *)(uintptr_t)(release_info.info->id);
    switch (ext->cmd.type) {
    case QXL_CMD_DRAW:
        update = SPICE_CONTAINEROF(ext, SimpleSpiceUpdate, ext);
        w_free(update->bitmaps);
        w_free(update);
        break;
    case QXL_CMD_CURSOR:
        cursor = SPICE_CONTAINEROF(ext, SimpleSpiceCursor, ext);
        w_free(cursor);
        break;
    default:
        g_assert_not_reached();
    }
}

static int get_cursor_command(QXLInstance *qin G_GNUC_UNUSED, struct QXLCommandExt *ext G_GNUC_UNUSED)
{
    //printf("FIXME! UNIMPLEMENTED! %s\n", __func__);
    WSpice *wspice = SPICE_CONTAINEROF(qin, WSpice, qxl);
    bool ret;

    pthread_mutex_lock(&wspice->lock);
    if (wspice->ptr_define) {
        *ext = wspice->ptr_define->ext;
        wspice->ptr_define = NULL;
        ret = true;
    } else if (wspice->ptr_move) {
        *ext = wspice->ptr_move->ext;
        wspice->ptr_move = NULL;
        ret = true;
    } else {
        ret = false;
    }
    pthread_mutex_unlock(&wspice->lock);

    return ret;
}

static int req_cursor_notification(QXLInstance *qin G_GNUC_UNUSED)
{
    /* FIXME: if cursor do not change ? */
    return 1;                /* 必须返回 1，否则出错 primary 创建不了*/
}

static void notify_update(QXLInstance *qin G_GNUC_UNUSED, uint32_t update_id G_GNUC_UNUSED)
{
    printf("FIXME! UNIMPLEMENTED! %s\n", __func__);
}

static int flush_resources(QXLInstance *qin G_GNUC_UNUSED)
{
    printf("FIXME! UNIMPLEMENTED! %s\n", __func__);
    return 1;
}

static void async_complete(QXLInstance *qin G_GNUC_UNUSED, uint64_t cookie)
{
    /* FIXME: handle all case */
    QXLMonitorsConfig *monitors = (QXLMonitorsConfig *)(uintptr_t)cookie;
    w_free(monitors);
}

static void update_area_complete(QXLInstance *qin G_GNUC_UNUSED,
                                 uint32_t surface_id G_GNUC_UNUSED,
                                 struct QXLRect *updated_rects G_GNUC_UNUSED,
                                 uint32_t num_updated_rects G_GNUC_UNUSED)
{
    printf("FIXME! UNIMPLEMENTED! %s\n", __func__);
}

static int client_monitors_config(QXLInstance *qin G_GNUC_UNUSED,
                                  VDAgentMonitorsConfig *monitors_config G_GNUC_UNUSED)
{
    printf("FIXME! UNIMPLEMENTED! %s\n", __func__);
    return 0;
}

static void set_client_capabilities(QXLInstance *qin G_GNUC_UNUSED,
                                    uint8_t client_present G_GNUC_UNUSED,
                                    uint8_t caps[58] G_GNUC_UNUSED)
{
    printf("FIXME! UNIMPLEMENTED! %s\n", __func__);
}

static QXLInterface dpy_interface = {
    .base = {
        .type = SPICE_INTERFACE_QXL,
        .description = "window spice server",
        .major_version = SPICE_INTERFACE_QXL_MAJOR,
        .minor_version = SPICE_INTERFACE_QXL_MINOR
    },
    .attache_worker = attache_worker,
    .set_compression_level = set_compression_level,
    .get_init_info = get_init_info,

    /* the callbacks below are called from spice server thread context */
    .get_command = get_command,
    .req_cmd_notification = req_cmd_notification,
    .release_resource = release_resource,
    .get_cursor_command = get_cursor_command,
    .req_cursor_notification = req_cursor_notification,
    .notify_update = notify_update,
    .flush_resources = flush_resources,
    .async_complete = async_complete,
    .update_area_complete = update_area_complete,
    .client_monitors_config = client_monitors_config,
//    .set_client_capabilities = set_client_capabilities,
    .set_client_capabilities = set_client_capabilities,
};

struct SpiceTimer {
    SpiceTimerFunc func;
    void *opaque;
    GSource *source;
};
typedef struct SpiceTimer SpiceTimer;

static SpiceTimer* timer_add(SpiceTimerFunc func, void *opaque)
{
    SpiceTimer *timer = g_new0(SpiceTimer, 1);

    timer->func = func;
    timer->opaque = opaque;

    return timer;
}

static gboolean timer_func(gpointer user_data)
{
    SpiceTimer *timer = user_data;

    timer->func(timer->opaque);
    /* timer might be free after func(), don't touch */

    return FALSE;
}

static void timer_cancel(SpiceTimer *timer)
{
    if (timer->source) {
        g_source_destroy(timer->source);
        g_source_unref(timer->source);
        timer->source = NULL;
    }
}

static void timer_start(SpiceTimer *timer, uint32_t ms)
{
    timer_cancel(timer);

    timer->source = g_timeout_source_new(ms);

    g_source_set_callback(timer->source, timer_func, timer, NULL);

    g_source_attach(timer->source, g_main_context_default());
}

static void timer_remove(SpiceTimer *timer)
{
    timer_cancel(timer);
    w_free(timer);
}

struct SpiceWatch {
    void *opaque;
    GSource *source;
    GIOChannel *channel;
    SpiceWatchFunc func;
};

static GIOCondition spice_event_to_giocondition(int event_mask)
{
    GIOCondition condition = 0;

    if (event_mask & SPICE_WATCH_EVENT_READ)
        condition |= G_IO_IN;
    if (event_mask & SPICE_WATCH_EVENT_WRITE)
        condition |= G_IO_OUT;

    return condition;
}

static int giocondition_to_spice_event(GIOCondition condition)
{
    int event = 0;

    if (condition & G_IO_IN)
        event |= SPICE_WATCH_EVENT_READ;
    if (condition & G_IO_OUT)
        event |= SPICE_WATCH_EVENT_WRITE;

    return event;
}

static gboolean watch_func(GIOChannel *source, GIOCondition condition,
                           gpointer data)
{
    SpiceWatch *watch = data;
    // this works also under Windows despite the name
    int fd = g_io_channel_unix_get_fd(source);

    watch->func(fd, giocondition_to_spice_event(condition), watch->opaque);

    return TRUE;
}

static void watch_update_mask(SpiceWatch *watch, int event_mask)
{
    if (watch->source) {
        g_source_destroy(watch->source);
        g_source_unref(watch->source);
        watch->source = NULL;
    }

    if (!event_mask)
        return;

    watch->source = g_io_create_watch(watch->channel, spice_event_to_giocondition(event_mask));
    /* g_source_set_callback() documentation says:
     * "The exact type of func depends on the type of source; ie. you should
     *  not count on func being called with data as its first parameter."
     * In this case it is a GIOFunc. First cast to GIOFunc to make sure it is the right type.
     * The other casts silence the warning from gcc */
    g_source_set_callback(watch->source, (GSourceFunc)(void*)(GIOFunc)watch_func, watch, NULL);
    g_source_attach(watch->source, g_main_context_default());
}

static SpiceWatch *watch_add(int fd, int event_mask, SpiceWatchFunc func, void *opaque)
{
    SpiceWatch *watch;

    watch = g_new0(SpiceWatch, 1);
    watch->channel = g_io_channel_win32_new_socket(fd);
    watch->func = func;
    watch->opaque = opaque;

    watch_update_mask(watch, event_mask);

    return watch;
}

static void watch_remove(SpiceWatch *watch)
{
    watch_update_mask(watch, 0);

    g_io_channel_unref(watch->channel);
    w_free(watch);
}

static void channel_event(int event G_GNUC_UNUSED, SpiceChannelEventInfo *info G_GNUC_UNUSED)
{
    printf("FIXME! UNIMPLEMENTED! %s\n", __func__);
}

static SpiceCoreInterface core_interface = {
    .base.type          = SPICE_INTERFACE_CORE,
    .base.description   = "window spice service",
    .base.major_version = SPICE_INTERFACE_CORE_MAJOR,
    .base.minor_version = SPICE_INTERFACE_CORE_MINOR,

    .timer_add          = timer_add,
    .timer_start        = timer_start,
    .timer_cancel       = timer_cancel,
    .timer_remove       = timer_remove,

    .watch_add          = watch_add,
    .watch_update_mask  = watch_update_mask,
    .watch_remove       = watch_remove,

    .channel_event      = channel_event,
};

static void *bitmaps_to_drawable(uint8_t *bitmaps, QXLRect *rect, int pitch)
{
    SimpleSpiceUpdate *update;
    QXLDrawable *drawable;
    QXLImage *qxl_image;
    QXLCommand *cmd;
    int bw, bh;

    update    = w_malloc0(sizeof(*update));
    drawable  = &update->drawable;
    qxl_image = &update->image;
    cmd       = &update->ext.cmd;

    bw        = rect->right - rect->left;
    bh        = rect->bottom - rect->top;

    update->bitmaps = bitmaps;

    drawable->bbox            = *rect;
    drawable->clip.type       = SPICE_CLIP_TYPE_NONE;
    drawable->effect          = QXL_EFFECT_OPAQUE;
    drawable->release_info.id = (uintptr_t)(&update->ext);
    drawable->type            = QXL_DRAW_COPY;

    drawable->surfaces_dest[0] = -1;
    drawable->surfaces_dest[1] = -1;
    drawable->surfaces_dest[2] = -1;
    drawable->surface_id       = 0;

    drawable->u.copy.rop_descriptor = SPICE_ROPD_OP_PUT;
    drawable->u.copy.src_bitmap = (uintptr_t)qxl_image;
    drawable->u.copy.src_area.left = 0;
    drawable->u.copy.src_area.top = 0;
    drawable->u.copy.src_area.right = bw;
    drawable->u.copy.src_area.bottom = bh;

    qxl_image->descriptor.type = SPICE_IMAGE_TYPE_BITMAP;
    qxl_image->bitmap.flags = SPICE_BITMAP_FLAGS_TOP_DOWN | QXL_BITMAP_DIRECT;
    qxl_image->bitmap.stride = pitch;
    qxl_image->descriptor.width = qxl_image->bitmap.x = bw;
    qxl_image->descriptor.height = qxl_image->bitmap.y = bh;
    qxl_image->bitmap.data = (uintptr_t)bitmaps;
    qxl_image->bitmap.palette = 0;
    qxl_image->bitmap.format = SPICE_BITMAP_FMT_RGBA;

    cmd->type = QXL_CMD_DRAW;
    cmd->data = (uintptr_t)drawable;

    return update;
}

static void handle_invalid_bitmaps(struct WSpice *wspice, WinSpiceInvalid *invalid)
{
    void *drawable;

    drawable = bitmaps_to_drawable(invalid->bitmaps, &invalid->rect, invalid->pitch);
    if (drawable) {
        g_async_queue_push(wspice->drawable_queue, drawable);
        wspice->wakeup(wspice);
    } else {
        w_free(invalid->bitmaps);
    }
}

void wakeup(struct WSpice *wspice)
{
    spice_qxl_wakeup(&wspice->qxl);
}

#define INPUT_BUTTON__MAX 5
typedef enum WinspiceMouseButton {
    WINSPICE_MOUSE_BUTTON_LEFT,
    WINSPICE_MOUSE_BUTTON_MIDDLE,
    WINSPICE_MOUSE_BUTTON_RIGHT,
    WINSPICE_MOUSE_BUTTON_UP,
    WINSPICE_MOUSE_BUTTON_DOWN,
} WinspiceMouseButton;

static uint32_t button_map[INPUT_BUTTON__MAX] = {
    [WINSPICE_MOUSE_BUTTON_LEFT]   = 0x01,
    [WINSPICE_MOUSE_BUTTON_MIDDLE] = 0x04,
    [WINSPICE_MOUSE_BUTTON_RIGHT]  = 0x02,
    [WINSPICE_MOUSE_BUTTON_UP]     = 0x10,
    [WINSPICE_MOUSE_BUTTON_DOWN]   = 0x20,
};

static const unsigned short button_map_down[INPUT_BUTTON__MAX] = {
    [WINSPICE_MOUSE_BUTTON_LEFT]   = MOUSEEVENTF_LEFTDOWN,
    [WINSPICE_MOUSE_BUTTON_MIDDLE] = MOUSEEVENTF_MIDDLEDOWN,
    [WINSPICE_MOUSE_BUTTON_RIGHT]  = MOUSEEVENTF_RIGHTDOWN,
    [WINSPICE_MOUSE_BUTTON_UP]     = MOUSEEVENTF_WHEEL,
    [WINSPICE_MOUSE_BUTTON_DOWN]   = MOUSEEVENTF_WHEEL,
};

static const unsigned short button_map_up[INPUT_BUTTON__MAX] = {
    [WINSPICE_MOUSE_BUTTON_LEFT]   = MOUSEEVENTF_LEFTUP,
    [WINSPICE_MOUSE_BUTTON_MIDDLE] = MOUSEEVENTF_MIDDLEUP,
    [WINSPICE_MOUSE_BUTTON_RIGHT]  = MOUSEEVENTF_RIGHTUP,
};

static void send_mouse_event(WinspiceMouseButton button, bool down)
{
    INPUT mouse_event;
    ZeroMemory(&mouse_event, sizeof(INPUT));
    mouse_event.type = INPUT_MOUSE;
    if (down) {
        mouse_event.mi.dwFlags |= button_map_down[button];
        if (button == WINSPICE_MOUSE_BUTTON_UP) {
            mouse_event.mi.mouseData = WHEEL_DELTA;
        } else if (button == WINSPICE_MOUSE_BUTTON_DOWN) {
            mouse_event.mi.mouseData = WHEEL_DELTA;
            mouse_event.mi.mouseData *= -1;
        }
    } else {
        mouse_event.mi.dwFlags |= button_map_up[button];
    }
    if (SendInput(1, &mouse_event, sizeof(INPUT)) == 0) {
        DWORD errCode = GetLastError();
        printf("error: %s:%d: %lX\n", __FUNCTION__, __LINE__, errCode);
    }
}

/// copy from qemu/ui/spice-input.c
static void spice_update_buttons(WSpice *wspice,
                                 int wheel, uint32_t button_mask)
{
    uint32_t btn;
    uint32_t mask;

    if (wheel < 0) {
        button_mask |= 0x10;
    }
    if (wheel > 0) {
        button_mask |= 0x20;
    }

    if (wspice->last_bmask == button_mask) {
        return;
    }

    for (btn = 0; btn < INPUT_BUTTON__MAX; btn++) {
        mask = button_map[btn];
        if ((wspice->last_bmask & mask) == (button_mask & mask)) {
            continue;
        }
        // qemu_input_queue_btn(src, btn, button_mask & mask);
        send_mouse_event(btn, button_mask & mask);
    }

    wspice->last_bmask = button_mask;
}

static void tablet_set_logical_size(SpiceTabletInstance* sin, int width, int height)
{
    printf("FIXME! UNIMPLEMENTED! %s\n", __func__);
}

static void tablet_position(SpiceTabletInstance* sin, int x, int y,
                            uint32_t buttons_state)
{
    WSpice *wspice = SPICE_CONTAINEROF(sin, WSpice, tablet);

    //spice_update_buttons(server, 0, buttons_state);

    INPUT mouse_event;
	ZeroMemory(&mouse_event, sizeof(INPUT));
	mouse_event.type = INPUT_MOUSE;

    mouse_event.mi.dx = (LONG)((float) x * (65535.0f / wspice->primary_width));
    mouse_event.mi.dy = (LONG)((float) y * (65535.0f / wspice->primary_height));
    mouse_event.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;

    if (SendInput(1, &mouse_event, sizeof(INPUT)) == 0) {
        DWORD errCode = GetLastError();
        printf("error: %s:%d: %lX\n", __FUNCTION__, __LINE__, errCode);
    }
}

static void tablet_wheel(SpiceTabletInstance* sin, int wheel,
                         uint32_t buttons_state)
{
    WSpice *wspice = SPICE_CONTAINEROF(sin, WSpice, tablet);
    spice_update_buttons(wspice, wheel, buttons_state);
}

static void tablet_buttons(SpiceTabletInstance *sin,
                           uint32_t buttons_state)
{
    WSpice *wspice = SPICE_CONTAINEROF(sin, WSpice, tablet);
    spice_update_buttons(wspice, 0, buttons_state);
}

static const SpiceTabletInterface tablet_interface = {
    .base.type          = SPICE_INTERFACE_TABLET,
    .base.description   = "tablet",
    .base.major_version = SPICE_INTERFACE_TABLET_MAJOR,
    .base.minor_version = SPICE_INTERFACE_TABLET_MINOR,
    .set_logical_size   = tablet_set_logical_size,
    .position           = tablet_position,
    .wheel              = tablet_wheel,
    .buttons            = tablet_buttons,
};

#define SCANCODE_EMUL0  0xE0
#define SCANCODE_UP     0x80
/// keymap reference: https://www.win.tue.nl/~aeb/linux/kbd/scancodes-1.html
static void kbd_push_key(SpiceKbdInstance *sin, uint8_t scancode)
{
    WSpice *wspice = SPICE_CONTAINEROF(sin, WSpice, kbd);
    bool up;
    int keycode;
    INPUT keyboard_event;

    if (scancode == SCANCODE_EMUL0) {
        wspice->emul0 = true;
        return;
    }

    keycode = scancode & ~SCANCODE_UP;
    up = scancode & SCANCODE_UP;

    keyboard_event.type           = INPUT_KEYBOARD;
    keyboard_event.ki.wVk         = 0;
    keyboard_event.ki.wScan       = keycode;
    keyboard_event.ki.dwFlags     = KEYEVENTF_SCANCODE;
    keyboard_event.ki.dwExtraInfo = 0;
    keyboard_event.ki.time        = 0;
    if (up) {
        keyboard_event.ki.dwFlags |= KEYEVENTF_KEYUP;
    }
    if (wspice->emul0) {
        keyboard_event.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
        wspice->emul0 = false;
    }

    if (SendInput(1, &keyboard_event, sizeof(INPUT)) == 0) {
        DWORD errCode = GetLastError();
        printf("error: %s:%d: %lX\n", __FUNCTION__, __LINE__, errCode);
    }
}

static uint8_t kbd_get_leds(SpiceKbdInstance *sin)
{
    uint8_t ret = 0;

    if (GetKeyState(VK_CAPITAL) & 1) {
        printf("cap\n");
        ret |= SPICE_KEYBOARD_MODIFIER_FLAGS_CAPS_LOCK;
    }
    if (GetKeyState(VK_SCROLL) & 1) {
        printf("scroll\n");
        ret |= SPICE_KEYBOARD_MODIFIER_FLAGS_SCROLL_LOCK;
    }
    if (GetKeyState(VK_NUMLOCK) & 1) {
        printf("cap\n");
        ret |= SPICE_KEYBOARD_MODIFIER_FLAGS_NUM_LOCK;
    }

    return ret;
}

static const SpiceKbdInterface kbd_interface = {
    .base.type          = SPICE_INTERFACE_KEYBOARD,
    .base.description   = "qemu keyboard",
    .base.major_version = SPICE_INTERFACE_KEYBOARD_MAJOR,
    .base.minor_version = SPICE_INTERFACE_KEYBOARD_MINOR,
    .push_scan_freg     = kbd_push_key,
    .get_leds           = kbd_get_leds,
};

void *create_cursor_update(WSpice *wspice, WinSpiceCursor *c, int on)
{
    size_t size = 0;
    SimpleSpiceCursor *update;
    QXLCursorCmd *ccmd;
    QXLCursor *cursor;
    QXLCommand *cmd;

    if (c) {
        if (wspice->ptr_type == SPICE_CURSOR_TYPE_MONO) {
            /**
             * cursor height is equal to AND mask heigh, but actual cursor
             * data contains two parts: AND mask bitmaps, and XOR mask bitmaps
             * so datasize should recalculation here.
             */
            int bpl = (c->width + 7) / 8;
            size = bpl * c->height * 2;
        } else {
            size = c->width * c->height * 4;
        }
    }

    update   = w_malloc0(sizeof(*update) + size);
    ccmd     = &update->cmd;
    cursor   = &update->cursor;
    cmd      = &update->ext.cmd;

    if (c) {
        ccmd->type = QXL_CURSOR_SET;
        ccmd->u.set.position.x = wspice->ptr_x + wspice->hot_x;
        ccmd->u.set.position.y = wspice->ptr_y + wspice->hot_y;
        ccmd->u.set.visible    = true;
        ccmd->u.set.shape      = (uintptr_t)cursor;
        cursor->header.type       = wspice->ptr_type;
        cursor->header.unique     = 0;
        cursor->header.width      = c->width;
        cursor->header.height     = c->height;
        cursor->header.hot_spot_x = c->hot_x;
        cursor->header.hot_spot_y = c->hot_y;
        cursor->data_size         = size;
        cursor->chunk.data_size   = size;
        memcpy(cursor->chunk.data, c->data, size);
    } else if (!on) {
        ccmd->type = QXL_CURSOR_HIDE;
    } else {
        ccmd->type = QXL_CURSOR_MOVE;
        ccmd->u.position.x = wspice->ptr_x + wspice->hot_x;
        ccmd->u.position.y = wspice->ptr_y + wspice->hot_y;
    }
    ccmd->release_info.id = (uintptr_t)(&update->ext);

    cmd->type = QXL_CMD_CURSOR;
    cmd->data = (uintptr_t)ccmd;

    return update;
}

static void start(WSpice *wspice)
{
    int port;
    const char *password;

    port = options_get_int(wspice->options, "port");
    password = options_get_string(wspice->options, "password");
    /// server
    wspice->server = spice_server_new();
    spice_server_set_port(wspice->server, port);
    if (password) {
        spice_server_set_ticket(wspice->server, password, 0, 0, 0);
    } else {
        /// If no password is set, the server does not need to set up auth
        spice_server_set_noauth(wspice->server);
    }
    spice_server_set_addr(wspice->server, "0.0.0.0", 0);   /* FIXME:  */
    //spice_server_set_image_compression(wspice->server, SPICE_IMAGE_COMPRESSION_AUTO_GLZ);  /* FIXME:  */

    /// qxl
    wspice->qxl.base.sif = &dpy_interface.base;
    wspice->qxl.id = 0;

    /// tablet
    wspice->tablet.base.sif = &tablet_interface.base;

    /// keyboard
    wspice->kbd.base.sif = &kbd_interface.base;

    /// from spice_server_init, spice server start to socket
    /// socket, listen ...
    printf("Start spice server with port: %d, compression: %s\n",
           options_get_int(wspice->options, "port"),
           wspice->options->compression_text);

    if (spice_server_init(wspice->server, &core_interface) != 0) {
        printf("failed to initialize spice server\n");
        exit(1);
    }

    if (spice_server_add_interface(wspice->server, &wspice->qxl.base)) {
        spice_server_destroy(wspice->server);
        printf("failed to add display interface\n");
        exit(1);
    }

    if (spice_server_add_interface(wspice->server, &wspice->tablet.base)) {
        spice_server_destroy(wspice->server);
        printf("failed to add tablet interface\n");
        exit(1);
    }

    if (spice_server_add_interface(wspice->server, &wspice->kbd.base)) {
        spice_server_destroy(wspice->server);
        printf("failed to add kbd interface\n");
        exit(1);
    }
}

static void stop(WSpice *wspice)
{
    while (1) {
        /**
         * display update thread has exited, just remove all data in
         * drawable_queue.
         */
        void *data;
        data = g_async_queue_try_pop(wspice->drawable_queue);
        if (data) {
            w_free(data);
        } else {
            break;
        }
    }

    spice_server_destroy(wspice->server);

    wspice->server = NULL;
}

WSpice *wspice_new(struct Session *session)
{
    WSpice *wspice = (WSpice *)w_malloc0(sizeof(WSpice));
    if (!wspice) {
        printf("failed to alloc memory for winspiceserver\n");
        goto failed;
    }

    wspice->session = session;
    wspice->options = session->options;

    /// drawable_queue init
    wspice->drawable_queue = g_async_queue_new();

    pthread_mutex_init(&wspice->lock, NULL);

    /// primary_surface
    wspice->primary_surface = NULL;
    wspice->primary_surface_size = 0;
    wspice->primary_width = session->display->width;
    wspice->primary_height = session->display->height;

    /// function
    wspice->start = start;
    wspice->stop = stop;
    wspice->wakeup = wakeup;
    wspice->handle_invalid_bitmaps = handle_invalid_bitmaps;

    return wspice;

failed:
    if (wspice) {
        wspice_destroy(wspice);
    }
    return NULL;
}

void wspice_destroy(WSpice *wspice)
{
    if (wspice) {
        /// destroy lock
        pthread_mutex_destroy(&wspice->lock);

        /// destroy resources allocated for drawable_queue
        if (wspice->drawable_queue) {
            g_async_queue_unref(wspice->drawable_queue);
        }

        /// TODO: free ptr_define and ptr_move

        /// destroy wspice object
        w_free(wspice);
    }
}
