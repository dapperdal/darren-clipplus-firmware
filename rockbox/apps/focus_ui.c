/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Drawing, input and list helpers shared by every Clip+ focus screen.
 * Project-original fork code; it only uses Rockbox's public LCD, action,
 * power and sound APIs so the proven target drivers stay untouched. */
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <string-extra.h>
#include "action.h"
#include "audio.h"
#include "backlight.h"
#include "kernel.h"
#include "lcd.h"
#include "scroll_engine.h"
#include "misc.h"
#include "power.h"
#include "powermgmt.h"
#include "settings.h"
#include "sound.h"
#include "timefuncs.h"
#include "focus_ui.h"

int focus_storage_generation;
long focus_last_input_tick;

static char toast_text[40];
static long toast_until;
static long volume_until;
static long header_since;

/* ------------------------------------------------------------- text ---- */

static unsigned next_codepoint(const char **text)
{
    const unsigned char *s = (const unsigned char *)*text;
    unsigned cp = *s;
    int extra = 0;

    if (!cp)
        return 0;
    s++;
    if (cp >= 0xf0) { cp &= 0x07; extra = 3; }
    else if (cp >= 0xe0) { cp &= 0x0f; extra = 2; }
    else if (cp >= 0xc0) { cp &= 0x1f; extra = 1; }
    else if (cp >= 0x80) cp = '?';
    while (extra-- > 0 && (*s & 0xc0) == 0x80)
        cp = (cp << 6) | (*s++ & 0x3f);
    *text = (const char *)s;

    if (cp > 255)
    {
        switch (cp)
        {
        case 0x2018: case 0x2019: cp = '\''; break;
        case 0x201c: case 0x201d: cp = '"'; break;
        case 0x2010: case 0x2011: case 0x2012: case 0x2013: case 0x2014:
            cp = '-'; break;
        case 0x2026: cp = '.'; break;
        default: cp = '?'; break;
        }
    }
    return cp;
}

static int glyph_for(const struct focus_font *f, unsigned cp)
{
    int g = f->index[cp & 0xff];
    if (g == 255)
        g = f->index['?'];
    return g == 255 ? -1 : g;
}

int focus_text_width(const struct focus_font *f, const char *s)
{
    int width = 0;
    unsigned cp;
    while ((cp = next_codepoint(&s)))
    {
        int g = glyph_for(f, cp);
        if (g >= 0)
            width += f->widths[g];
    }
    return width;
}

void focus_text_clip(const struct focus_font *f, int x, int y, const char *s,
                     int clip_x0, int clip_x1)
{
    unsigned cp;

    if (clip_x0 < 0) clip_x0 = 0;
    if (clip_x1 > LCD_WIDTH) clip_x1 = LCD_WIDTH;
    lcd_set_drawmode(DRMODE_FG);
    while (x < clip_x1 && (cp = next_codepoint(&s)))
    {
        int g = glyph_for(f, cp);
        int w, x0, x1;
        if (g < 0)
            continue;
        w = f->widths[g];
        x0 = x < clip_x0 ? clip_x0 : x;
        x1 = x + w > clip_x1 ? clip_x1 : x + w;
        if (x1 > x0)
            lcd_mono_bitmap_part(f->bits + f->offsets[g], x0 - x, 0, w,
                                 x0, y, x1 - x0, f->height);
        x += w;
    }
    lcd_set_drawmode(DRMODE_SOLID);
}

void focus_text(const struct focus_font *f, int x, int y, const char *s)
{
    focus_text_clip(f, x, y, s, 0, LCD_WIDTH);
}

void focus_text_center(const struct focus_font *f, int y, const char *s)
{
    focus_text(f, (LCD_WIDTH - focus_text_width(f, s)) / 2, y, s);
}

void focus_text_right(const struct focus_font *f, int x_right, int y,
                      const char *s)
{
    focus_text(f, x_right - focus_text_width(f, s), y, s);
}

void focus_text_marquee(const struct focus_font *f, int x, int y, int w,
                        const char *s, long since, bool center)
{
    int tw = focus_text_width(f, s);
    int offset = 0;

    if (tw <= w)
    {
        if (center)
            x += (w - tw) / 2;
        focus_text_clip(f, x, y, s, x, x + w);
        return;
    }
    {
        const int hold = HZ * 6 / 5, speed = 28; /* pixels per second */
        int max = tw - w;
        int travel = max * HZ / speed;
        long phase = (current_tick - since) % (hold * 2 + travel);
        if (phase < hold)
            offset = 0;
        else if (phase < hold + travel)
            offset = (int)((phase - hold) * speed / HZ);
        else
            offset = max;
    }
    focus_text_clip(f, x - offset, y, s, x, x + w);
}

void focus_icon_draw(enum focus_icon_id id, int x, int y)
{
    const struct focus_icon *icon = &focus_icons[id];
    lcd_set_drawmode(DRMODE_FG);
    lcd_mono_bitmap(icon->bits, x, y, icon->w, icon->h);
    lcd_set_drawmode(DRMODE_SOLID);
}

void focus_round_rect(int x, int y, int w, int h, bool fill)
{
    if (w < 3 || h < 3)
        return;
    lcd_set_drawmode(DRMODE_SOLID);
    if (fill)
    {
        lcd_fillrect(x + 1, y, w - 2, h);
        lcd_vline(x, y + 1, y + h - 2);
        lcd_vline(x + w - 1, y + 1, y + h - 2);
    }
    else
    {
        lcd_hline(x + 1, x + w - 2, y);
        lcd_hline(x + 1, x + w - 2, y + h - 1);
        lcd_vline(x, y + 1, y + h - 2);
        lcd_vline(x + w - 1, y + 1, y + h - 2);
    }
}

void focus_invert_round(int x, int y, int w, int h)
{
    lcd_set_drawmode(DRMODE_COMPLEMENT);
    lcd_fillrect(x, y, w, h);
    /* knock the corners back out for a soft pill shape */
    lcd_drawpixel(x, y);
    lcd_drawpixel(x + w - 1, y);
    lcd_drawpixel(x, y + h - 1);
    lcd_drawpixel(x + w - 1, y + h - 1);
    lcd_set_drawmode(DRMODE_SOLID);
}

void focus_bar(int x, int y, int w, int h, long value, long max)
{
    int inner = w - 4;
    int fill;

    if (max <= 0) { max = 1; value = 0; }
    if (value < 0) value = 0;
    if (value > max) value = max;
    fill = (int)(inner * value / max);
    focus_round_rect(x, y, w, h, false);
    if (fill > 0)
        lcd_fillrect(x + 2, y + 2, fill, h - 4);
}

/* ------------------------------------------------------------ frame ---- */

bool focus_display_on(void)
{
    return is_backlight_on(false);
}

static void header_battery(void)
{
    static long refresh_tick;
    static int level = -1;
    static bool charging;
    const int x = 113, y = 4;

    /* Power readings are refreshed at most every ten seconds. */
    if (level < 0 || TIME_AFTER(current_tick, refresh_tick))
    {
        level = battery_level();
        charging = charging_state() || charger_inserted();
        refresh_tick = current_tick + HZ * 10;
    }
    lcd_drawrect(x, y, 12, 8);
    lcd_vline(x + 12, y + 2, y + 5);
    if (level > 0)
    {
        int fill = (level * 8 + 50) / 100;
        if (fill < 1) fill = 1;
        lcd_fillrect(x + 2, y + 2, fill, 4);
    }
    if (charging)
        focus_icon_draw(FOCUS_ICON_S_BOLT, x - 6, y + 1);
}

void focus_header(const char *title)
{
    char clock[8];
    int right = 106;
    struct tm *tm = get_time();

    lcd_set_drawmode(DRMODE_SOLID | DRMODE_INVERSEVID);
    lcd_fillrect(0, 0, LCD_WIDTH, FOCUS_HEADER_H);
    lcd_set_drawmode(DRMODE_SOLID);

    header_battery();
    if (tm && valid_time(tm) && tm->tm_year >= 124)
    {
        snprintf(clock, sizeof(clock), "%d:%02d", tm->tm_hour, tm->tm_min);
        right -= focus_text_width(&focus_font_small, clock);
        focus_text(&focus_font_small, right, 3, clock);
        right -= 4;
    }
    if (is_keys_locked())
    {
        right -= 6;
        focus_icon_draw(FOCUS_ICON_S_LOCK, right, 4);
        right -= 3;
    }
    if (get_sleep_timer_active())
    {
        right -= 7;
        focus_icon_draw(FOCUS_ICON_S_MOON, right, 5);
        right -= 3;
    }
    if (title)
        focus_text_marquee(&focus_font_text, 1, 2, right - 3, title,
                           header_since, false);
}

void focus_clear(void)
{
    lcd_set_viewport(NULL);
    lcd_scroll_stop();
    lcd_set_drawmode(DRMODE_SOLID);
    lcd_clear_display();
}

void focus_begin(const char *title)
{
    focus_clear();
    focus_header(title);
}

static void draw_volume_overlay(void)
{
    char text[8];
    int min = sound_min(SOUND_VOLUME);
    int max = sound_max(SOUND_VOLUME);
    int value = global_status.volume;
    int percent = max > min ? (value - min) * 100 / (max - min) : 0;

    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    lcd_set_drawmode(DRMODE_SOLID | DRMODE_INVERSEVID);
    lcd_fillrect(0, FOCUS_BODY_Y, LCD_WIDTH, FOCUS_BODY_H);
    lcd_set_drawmode(DRMODE_SOLID);
    snprintf(text, sizeof(text), "%d", percent);
    focus_text_center(&focus_font_huge, 20, text);
    focus_bar(14, 53, 100, 8, percent, 100);
}

static void draw_toast(void)
{
    int w = focus_text_width(&focus_font_text, toast_text) + 14;
    int x;
    if (w > LCD_WIDTH - 4) w = LCD_WIDTH - 4;
    x = (LCD_WIDTH - w) / 2;
    lcd_set_drawmode(DRMODE_SOLID | DRMODE_INVERSEVID);
    lcd_fillrect(x - 2, 42, w + 4, 22);
    lcd_set_drawmode(DRMODE_SOLID);
    focus_round_rect(x, 44, w, 18, false);
    focus_text_clip(&focus_font_text, x + 7, 47, toast_text, x + 2, x + w - 2);
}

void focus_flush(void)
{
    if (volume_until && TIME_BEFORE(current_tick, volume_until))
        draw_volume_overlay();
    else
        volume_until = 0;
    if (toast_until && TIME_BEFORE(current_tick, toast_until))
        draw_toast();
    else
        toast_until = 0;
    lcd_update();
}

void focus_toast(const char *message)
{
    strlcpy(toast_text, message, sizeof(toast_text));
    toast_until = current_tick + HZ * 3 / 2;
}

void focus_message(const char *title, const char *line1, const char *line2)
{
    focus_begin(title);
    if (line1)
        focus_text_marquee(&focus_font_title, 2, line2 ? 22 : 30, 124, line1,
                           current_tick, true);
    if (line2)
        focus_text_marquee(&focus_font_small, 2, 44, 124, line2,
                           current_tick, true);
    focus_flush();
}

void focus_splash(const char *word)
{
    int w = focus_text_width(&focus_font_title, word);
    lcd_set_viewport(NULL);
    lcd_set_drawmode(DRMODE_SOLID);
    lcd_clear_display();
    focus_text_center(&focus_font_title, 26, word);
    lcd_hline((LCD_WIDTH - w) / 2, (LCD_WIDTH + w) / 2 - 1, 45);
    lcd_update();
}

/* ------------------------------------------------------------ input ---- */

static void lock_notice(bool locked)
{
    focus_begin(NULL);
    focus_icon_draw(FOCUS_ICON_LOCK, 10, 26);
    focus_text(&focus_font_title, 44, 22, locked ? "Locked" : "Unlocked");
    focus_text(&focus_font_small, 44, 42, locked ? "Hold HOME" : "Buttons on");
    lcd_update();
    sleep(HZ);
}

int focus_get_action(int context)
{
    return focus_get_action_timeout(context,
                                    focus_display_on() ? HZ / 10 : HZ / 2);
}

int focus_get_action_timeout(int context, int timeout)
{
    static bool was_locked;
    int action;
#ifdef SIMULATOR
    void focus_sim_step(void);
    focus_sim_step();
#endif
    action = get_action(context | ALLOW_SOFTLOCK, timeout);
    if (action != ACTION_NONE)
        focus_last_input_tick = current_tick;
    bool locked = is_keys_locked();

    if (locked != was_locked)
    {
        was_locked = locked;
        lock_notice(locked);
        return ACTION_REDRAW;
    }
    switch (action)
    {
    case ACTION_WPS_VOLUP:
    case ACTION_WPS_VOLDOWN:
        adjust_volume(action == ACTION_WPS_VOLUP ? 1 : -1);
        volume_until = current_tick + HZ * 3 / 2;
        return ACTION_REDRAW;
    case ACTION_NONE:
    case ACTION_REDRAW:
        return action;
    default:
        break;
    }
    if (IS_SYSEVENT(action))
    {
        if (default_event_handler(action) == SYS_USB_CONNECTED)
            focus_storage_generation++;
        return ACTION_REDRAW;
    }
    return action;
}

/* ------------------------------------------------------------- list ---- */

#define LIST_ROWS  3
#define LIST_ROW_H 16
#define LIST_W     122

void focus_list_init(struct focus_list *list, const char *title)
{
    memset(list, 0, sizeof(*list));
    list->title = title;
    list->since = current_tick;
}

void focus_list_draw(struct focus_list *list)
{
    char name[96], value[24];
    int row;

    focus_begin(list->title);
    if (list->count <= 0)
    {
        focus_text_center(&focus_font_text, 34,
                          list->empty ? list->empty : "Nothing here");
        return;
    }
    if (list->selected >= list->count) list->selected = list->count - 1;
    if (list->selected < 0) list->selected = 0;
    if (list->selected < list->top) list->top = list->selected;
    if (list->selected >= list->top + LIST_ROWS)
        list->top = list->selected - LIST_ROWS + 1;
    if (list->top > list->count - LIST_ROWS) list->top = list->count - LIST_ROWS;
    if (list->top < 0) list->top = 0;

    for (row = 0; row < LIST_ROWS && list->top + row < list->count; row++)
    {
        int index = list->top + row;
        int y = FOCUS_BODY_Y + row * LIST_ROW_H;
        int x = 5, end = LIST_W - 4;
        int icon = list->icon ? list->icon(index, list->ctx) : -1;
        const char *text = list->name(index, list->ctx, name, sizeof(name));
        const char *val = list->value
            ? list->value(index, list->ctx, value, sizeof(value)) : NULL;

        if (icon >= 0)
        {
            const struct focus_icon *ic = &focus_icons[icon];
            focus_icon_draw(icon, 4, y + (LIST_ROW_H - ic->h) / 2);
            x = 17;
        }
        if (val && val[0])
        {
            int vw = focus_text_width(&focus_font_small, val);
            focus_text(&focus_font_small, end - vw, y + 3, val);
            end -= vw + 5;
        }
        if (index == list->selected)
        {
            focus_text_marquee(&focus_font_text, x, y + 2, end - x,
                               text ? text : "", list->since, false);
            focus_invert_round(0, y, LIST_W, LIST_ROW_H);
        }
        else
            focus_text_clip(&focus_font_text, x, y + 2, text ? text : "",
                            x, end);
    }
    if (list->count > LIST_ROWS)
    {
        int h = FOCUS_BODY_H * LIST_ROWS / list->count;
        int y;
        if (h < 5) h = 5;
        y = FOCUS_BODY_Y +
            (FOCUS_BODY_H - h) * list->top / (list->count - LIST_ROWS);
        lcd_fillrect(125, y, 2, h);
    }
}

bool focus_list_input(struct focus_list *list, int action)
{
    int old = list->selected;

    if (list->count <= 0)
        return false;
    switch (action)
    {
    case ACTION_STD_PREV:
        list->selected = (list->selected + list->count - 1) % list->count;
        break;
    case ACTION_STD_NEXT:
        list->selected = (list->selected + 1) % list->count;
        break;
    case ACTION_STD_PREVREPEAT:
        if (list->selected > 0) list->selected--;
        break;
    case ACTION_STD_NEXTREPEAT:
        if (list->selected < list->count - 1) list->selected++;
        break;
    default:
        return false;
    }
    if (list->selected != old)
        list->since = current_tick;
    return true;
}
