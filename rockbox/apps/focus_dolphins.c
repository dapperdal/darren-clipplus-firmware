/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Original dolphin standby scene for the Clip+ blue OLED band. */
#include "config.h"
#include "action.h"
#include "audio.h"
#include "kernel.h"
#include "lcd.h"
#include "settings.h"
#include "focus_ui.h"

static const signed char swell[] = {
    0, 1, 2, 3, 4, 4, 4, 3, 2, 1, 0, -1, -2, -3, -4, -4, -4, -3, -2, -1
};

/* Scale the foreground animal as it approaches. Integer-only, no heap and
 * fewer than 3,500 pixel reads per frame on the real device. */
static void dolphin_draw(enum focus_icon_id id, int x, int y, int scale)
{
    const struct focus_icon *sprite = &focus_icons[id];
    int w = sprite->w * scale / 100, h = sprite->h * scale / 100;
    for (int dy = 0; dy < h; dy++)
    {
        int sy = dy * sprite->h / h;
        for (int dx = 0; dx < w; dx++)
        {
            int sx = dx * sprite->w / w;
            if (x + dx < 0 || x + dx >= LCD_WIDTH ||
                y + dy < FOCUS_BODY_Y || y + dy >= LCD_HEIGHT)
                continue;
            if (sprite->bits[(sy / 8) * sprite->w + sx] & (1 << (sy & 7)))
                lcd_drawpixel(x + dx, y + dy);
        }
    }
}

static void dolphins_draw(unsigned frame)
{
    static const enum focus_icon_id swim[] = {
        FOCUS_ICON_DOLPHIN_0, FOCUS_ICON_DOLPHIN_1,
        FOCUS_ICON_DOLPHIN_2, FOCUS_ICON_DOLPHIN_1
    };
    static const enum focus_icon_id small[] = {
        FOCUS_ICON_DOLPHIN_SMALL_0, FOCUS_ICON_DOLPHIN_SMALL_1,
        FOCUS_ICON_DOLPHIN_SMALL_2, FOCUS_ICON_DOLPHIN_SMALL_1
    };
    char title[MAX_PATH];
    unsigned phase = frame % 200; /* 14 s underwater, then a 6 s jump */
    bool on = focus_playing_title(title, sizeof(title));
#if CONFIG_TUNER
    if (!on)
        on = focus_radio_title(title, sizeof(title));
#endif
    focus_begin(on ? title : "Dolphins");

    if (phase < 140)
    {
        /* Sparse light from the surface and bubbles drifting upward. */
        for (int i = 0; i < 7; i++)
        {
            int x = (i * 23 + frame / 4) % 140 - 6;
            lcd_drawline(x, 17, x - 4, 21 + i % 3);
        }
        for (int i = 0; i < 9; i++)
        {
            int x = (i * 37 + frame / 3) % 128;
            int y = 19 + (i * 13 + 44 - frame % 44) % 44;
            if (i % 3 == 0)
                lcd_drawrect(x, y, 2, 2);
            else
                lcd_drawpixel(x, y);
        }
        /* Distant pair, with independent tail beats and gentle vertical drift. */
        dolphin_draw(small[(frame / 3 + 1) % 4],
                     (frame / 2 + 18) % 160 - 22,
                     23 + swell[(frame / 3) % 20] / 2, 100);
        dolphin_draw(small[(frame / 4 + 2) % 4],
                     (frame / 3 + 92) % 160 - 22,
                     40 + swell[(frame / 4 + 7) % 20] / 2, 100);
        /* A larger dolphin glides past the viewer, growing as it comes near. */
        int near = phase < 70 ? phase : 140 - phase;
        dolphin_draw(swim[(frame / 2) % 4], (int)phase * 3 / 2 - 60,
                     26 + swell[(frame / 2) % 20], 120 + near);
    }
    else
    {
        int t = phase - 140;
        int height = t * (60 - t) * 24 / 900;
        int x = -36 + t * 3;
        enum focus_icon_id sprite = t < 23 ? FOCUS_ICON_DOLPHIN_RISE :
                                   t > 37 ? FOCUS_ICON_DOLPHIN_DIVE :
                                   swim[(frame / 2) % 4];
        /* Broken waterline, with a smaller dolphin following underneath. */
        for (int i = -16; i < LCD_WIDTH; i += 16)
        {
            int wx = i + frame % 16;
            int wy = 51 + swell[(i + 16) / 4 % 20] / 3;
            lcd_hline(wx, wx + 8, wy);
        }
        dolphin_draw(small[(frame / 3) % 4], (t * 2 + 12) % 160 - 22, 51, 100);
        dolphin_draw(sprite, x, 43 - height, 105);
        if (t < 12 || t > 48)
        {
            int splash = t < 12 ? t : 60 - t;
            int sx = t < 12 ? 12 : 115;
            lcd_drawpixel(sx - splash, 49 - splash / 2);
            lcd_drawpixel(sx + splash, 47 - splash / 2);
        }
    }
    focus_flush();
}

void focus_dolphins(void)
{
    int generation = focus_storage_generation;
    long started = current_tick;

    while (true)
    {
        int action;
        if (focus_display_on())
            dolphins_draw((unsigned)(current_tick - started) / (HZ / 10));
        /* Keep using the normal input path: USB, shutdown, lock and volume
         * still work here. Sleep the renderer when the OLED times out. */
        action = focus_get_action(FOCUS_CTX_NAV);
        if (action != ACTION_NONE || generation != focus_storage_generation)
        {
            action_wait_for_release();
            focus_last_input_tick = current_tick;
            return; /* wake-up key never pauses or skips the playing track */
        }
    }
}

void focus_standby_check(int action)
{
    /* Eight seconds lets the scene appear before the normal 15 s OLED
     * timeout. It never overrides the user's screen-off or sleep timer. */
    if (action == ACTION_NONE && global_settings.focus_dolphin_standby &&
        focus_display_on() && !(audio_status() & AUDIO_STATUS_RECORD) &&
        TIME_AFTER(current_tick, focus_last_input_tick + 8 * HZ))
        focus_dolphins();
}
