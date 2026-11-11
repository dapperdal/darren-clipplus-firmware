/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Home carousel and settings for the Clip+ focus shell.
 * Project-original fork code. */
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <string-extra.h>
#include "action.h"
#include "rtc.h"
#include "audio.h"
#include "kernel.h"
#include "file.h"
#include "lcd.h"
#include "misc.h"
#include "playback.h"
#include "playlist.h"
#include "powermgmt.h"
#include "settings.h"
#include "timefuncs.h"
#include "version.h"
#include "focus_ui.h"

/* ------------------------------------------------------------- home ---- */

enum { HOME_MUSIC, HOME_PODCASTS, HOME_RADIO, HOME_RECORDINGS, HOME_PLAYING,
       HOME_SETTINGS, HOME_COUNT };

static const struct { const char *label; enum focus_icon_id icon; }
home_items[HOME_COUNT] = {
    { "Music", FOCUS_ICON_MUSIC },
    { "Podcasts", FOCUS_ICON_PODCAST },
    { "Radio", FOCUS_ICON_RADIO },
    { "Recordings", FOCUS_ICON_RECORD },
    { "Now Playing", FOCUS_ICON_PLAYING },
    { "Settings", FOCUS_ICON_SETTINGS },
};

static void home_item(int index, int dy)
{
    focus_icon_draw(home_items[index].icon, 52, 19 + dy);
    focus_text_center(&focus_font_title, 45 + dy, home_items[index].label);
}

static void home_frame(int selected, int other, int offset)
{
    int i;

    focus_clear();
    home_item(selected, offset);
    if (other >= 0)
        home_item(other, offset > 0 ? offset - FOCUS_BODY_H
                                    : offset + FOCUS_BODY_H);
    for (i = 0; i < HOME_COUNT; i++)
    {
        int y = 21 + i * 7;
        if (i == selected)
            lcd_fillrect(123, y - 1, 4, 4);
        else
            lcd_fillrect(124, y, 2, 2);
    }
    {
        char title[MAX_PATH];
        bool on = false;
#if CONFIG_TUNER
        on = focus_radio_title(title, sizeof(title));
#endif
        if (!on)
            on = focus_playing_title(title, sizeof(title));
        focus_header(on ? title : "DARREN");
        if (on)
        {
            /* small "playing" pip under the title */
            lcd_hline(1, 8, 15);
        }
    }
    focus_flush();
}

static void home_slide(int from, int to, int direction)
{
    static const signed char steps[] = { 34, 20, 9 };
    unsigned i;

    if (!focus_display_on())
        return;
    for (i = 0; i < sizeof(steps); i++)
    {
        home_frame(to, from, direction * steps[i]);
        sleep(HZ / 50);
    }
}

static void home_open(int item)
{
    switch (item)
    {
    case HOME_MUSIC:
        focus_browse("/MUSIC", "Music", false);
        break;
    case HOME_PODCASTS:
        focus_browse("/PODCASTS", "Podcasts", false);
        break;
    case HOME_RADIO:
#if CONFIG_TUNER
        focus_radio();
#endif
        break;
    case HOME_RECORDINGS:
        focus_browse(global_settings.rec_directory[0]
                     ? global_settings.rec_directory : "/Recordings",
                     "Recordings", true);
        break;
    case HOME_PLAYING:
        focus_now_playing();
        break;
    case HOME_SETTINGS:
        focus_settings();
        break;
    }
}

void focus_menu(void)
{
    int selected = HOME_MUSIC;

    while (true)
    {
        int action, next;

        home_frame(selected, -1, 0);
        action = focus_get_action(FOCUS_CTX_NAV);
        switch (action)
        {
        case ACTION_STD_PREV:
        case ACTION_STD_PREVREPEAT:
            next = (selected + HOME_COUNT - 1) % HOME_COUNT;
            home_slide(selected, next, -1);
            selected = next;
            break;
        case ACTION_STD_NEXT:
        case ACTION_STD_NEXTREPEAT:
            next = (selected + 1) % HOME_COUNT;
            home_slide(selected, next, 1);
            selected = next;
            break;
        case ACTION_STD_OK:
            home_open(selected);
            break;
        case ACTION_STD_CONTEXT:
            /* hold Select anywhere on Home: straight to what is playing */
            if (focus_playback_active() || focus_radio_active())
                focus_now_playing();
            break;
        default:
            break;
        }
    }
}

/* --------------------------------------------------------- settings ---- */

enum { SET_EQ, SET_SHUFFLE, SET_REPEAT, SET_SLEEP, SET_BRIGHTNESS, SET_ALARM,
       SET_CLOCK, SET_ABOUT };

static const unsigned char settings_all[] = {
    SET_EQ, SET_SHUFFLE, SET_REPEAT, SET_SLEEP, SET_BRIGHTNESS, SET_ALARM,
    SET_CLOCK, SET_ABOUT
};
static const unsigned char settings_playback[] = {
    SET_SHUFFLE, SET_REPEAT, SET_SLEEP, SET_EQ
};
#ifdef HAVE_RTC_ALARM
static bool alarm_on;
#endif
static const unsigned char sleep_steps[] = { 0, 15, 30, 45, 60, 90 };
static const unsigned char brightness_steps[] = { 12, 30, 50 };
static const char * const brightness_names[] = { "Low", "Medium", "High" };

static int brightness_index(void)
{
    if (global_settings.contrast < 21) return 0;
    if (global_settings.contrast < 40) return 1;
    return 2;
}

static const char *settings_name(int index, void *ctx, char *buf, size_t size)
{
    static const char * const names[] = {
        "Equaliser", "Shuffle", "Repeat", "Sleep timer", "Brightness",
        "Alarm", "Set clock", "About"
    };
    (void)buf; (void)size;
    return names[((const unsigned char *)ctx)[index]];
}

static const char *settings_value(int index, void *ctx, char *buf, size_t size)
{
    switch (((const unsigned char *)ctx)[index])
    {
    case SET_SHUFFLE:
        return global_settings.playlist_shuffle ? "On" : "Off";
    case SET_REPEAT:
        switch (global_settings.repeat_mode)
        {
        case REPEAT_ALL: return "All";
        case REPEAT_ONE: return "One";
        case REPEAT_OFF: return "Off";
        default: return "Other";
        }
    case SET_SLEEP:
        if (!get_sleep_timer_active())
            return "Off";
        snprintf(buf, size, "%d min", (get_sleep_timer() + 59) / 60);
        return buf;
    case SET_BRIGHTNESS:
        return brightness_names[brightness_index()];
    case SET_EQ:
        return global_settings.eq_enabled ? "On" : "Off";
#ifdef HAVE_RTC_ALARM
    case SET_ALARM:
    {
        int hour, minute;
        if (!alarm_on)
            return "Off";
        rtc_get_alarm(&hour, &minute);
        snprintf(buf, size, "%02d:%02d", hour, minute);
        return buf;
    }
#endif
    default:
        return NULL;
    }
}

/* Big HH:MM editor. Left/Right pick the field, Up/Down change it, Select
 * saves, Home cancels. */
static bool edit_time(const char *title, int *hour, int *minute)
{
    int field = 0;

    while (true)
    {
        char text[8];
        int action, x, w, *value = field ? minute : hour;
        int limit = field ? 60 : 24;

        focus_begin(title);
        snprintf(text, sizeof(text), "%02d:%02d", *hour, *minute);
        w = focus_text_width(&focus_font_huge, text);
        x = (LCD_WIDTH - w) / 2;
        focus_text(&focus_font_huge, x, 17, text);
        lcd_fillrect(field ? x + w - 32 : x, 46, 32, 3);
        focus_text_center(&focus_font_small, 53, "Select saves");
        focus_flush();

        action = focus_get_action(FOCUS_CTX_EQ);
        switch (action)
        {
        case ACTION_SETTINGS_INC:
        case ACTION_SETTINGS_INCREPEAT:
            *value = (*value + 1) % limit;
            break;
        case ACTION_SETTINGS_DEC:
        case ACTION_SETTINGS_DECREPEAT:
            *value = (*value + limit - 1) % limit;
            break;
        case ACTION_STD_PREV:
        case ACTION_STD_NEXT:
            field = !field;
            break;
        case ACTION_STD_OK:
            return true;
        case ACTION_STD_MENU:
            return false;
        default:
            break;
        }
    }
}

static bool edit_date(struct tm *tm)
{
    static const char * const months[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    static const unsigned char days[] = {
        31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };
    int field = 0;

    while (true)
    {
        char part[3][8];
        int action, i, x, total = 0, widths[3];
        int direction = 0;

        snprintf(part[0], sizeof(part[0]), "%d", tm->tm_mday);
        snprintf(part[1], sizeof(part[1]), "%s", months[tm->tm_mon]);
        snprintf(part[2], sizeof(part[2]), "%d", tm->tm_year + 1900);
        for (i = 0; i < 3; i++)
        {
            widths[i] = focus_text_width(&focus_font_title, part[i]);
            total += widths[i] + (i ? 8 : 0);
        }
        focus_begin("Set date");
        x = (LCD_WIDTH - total) / 2;
        for (i = 0; i < 3; i++)
        {
            focus_text(&focus_font_title, x, 24, part[i]);
            if (i == field)
                lcd_fillrect(x, 43, widths[i], 3);
            x += widths[i] + 8;
        }
        focus_text_center(&focus_font_small, 53, "Select saves");
        focus_flush();

        action = focus_get_action(FOCUS_CTX_EQ);
        switch (action)
        {
        case ACTION_SETTINGS_INC:
        case ACTION_SETTINGS_INCREPEAT:
            direction = 1;
            break;
        case ACTION_SETTINGS_DEC:
        case ACTION_SETTINGS_DECREPEAT:
            direction = -1;
            break;
        case ACTION_STD_PREV:
            field = (field + 2) % 3;
            break;
        case ACTION_STD_NEXT:
            field = (field + 1) % 3;
            break;
        case ACTION_STD_OK:
            return true;
        case ACTION_STD_MENU:
            return false;
        default:
            break;
        }
        if (direction)
        {
            if (field == 0)
                tm->tm_mday = (tm->tm_mday - 1 + direction +
                               days[tm->tm_mon]) % days[tm->tm_mon] + 1;
            else if (field == 1)
                tm->tm_mon = (tm->tm_mon + direction + 12) % 12;
            else if (tm->tm_year + direction >= 124 &&
                     tm->tm_year + direction <= 199)
                tm->tm_year += direction;
            if (tm->tm_mday > days[tm->tm_mon])
                tm->tm_mday = days[tm->tm_mon];
        }
    }
}

#ifdef HAVE_RTC_ALARM
static void settings_alarm(void)
{
    int hour, minute, now, togo;
    struct tm *tm = get_time();
    char text[32];

    if (alarm_on)
    {
        rtc_enable_alarm(false);
        alarm_on = false;
        focus_toast("Alarm off");
        return;
    }
    rtc_get_alarm(&hour, &minute);
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59)
    {
        hour = 7;
        minute = 0;
    }
    if (!edit_time("Alarm", &hour, &minute))
        return;
    now = tm->tm_hour * 60 + tm->tm_min;
    togo = (hour * 60 + minute - now + 1440) % 1440;
    /* as in Rockbox: never arm an alarm that would fire during shutdown */
    if (togo <= 1)
    {
        focus_toast("Too soon");
        return;
    }
    rtc_init();
    rtc_set_alarm(hour, minute);
    rtc_enable_alarm(true);
    alarm_on = true;
    snprintf(text, sizeof(text), "In %dh %02dm", togo / 60, togo % 60);
    focus_toast(text);
}
#endif

static void settings_about(void)
{
    char line1[40], line2[40];
    int level = battery_level();
    int minutes = battery_time();

    snprintf(line1, sizeof(line1), "Focus %s", rbversion);
    if (level >= 0 && minutes > 0)
        snprintf(line2, sizeof(line2), "Battery %d%%  ~%dh%02d", level,
                 minutes / 60, minutes % 60);
    else if (level >= 0)
        snprintf(line2, sizeof(line2), "Battery %d%%", level);
    else
        snprintf(line2, sizeof(line2), "Battery unknown");
    while (true)
    {
        int action;
        focus_begin("About");
        focus_text_center(&focus_font_title, 18, "DARREN");
        focus_text_marquee(&focus_font_small, 2, 37, 124, line1, 0, true);
        focus_text_marquee(&focus_font_small, 2, 50, 124, line2, 0, true);
        focus_flush();
        action = focus_get_action(FOCUS_CTX_NAV);
        if (action == ACTION_STD_MENU || action == ACTION_STD_OK)
            return;
    }
}

static void settings_activate(int item)
{
    unsigned i;

    switch (item)
    {
    case SET_EQ:
        focus_eq();
        return;
    case SET_SHUFFLE:
    {
        struct playlist_info *playlist = playlist_get_current();
        global_settings.playlist_shuffle = !global_settings.playlist_shuffle;
        if (playlist && playlist->started &&
            (audio_status() & AUDIO_STATUS_PLAY))
        {
            if (global_settings.playlist_shuffle)
                playlist_randomise(playlist, current_tick, true);
            else
                playlist_sort(playlist, true);
        }
        break;
    }
    case SET_REPEAT:
        global_settings.repeat_mode =
            global_settings.repeat_mode == REPEAT_OFF ? REPEAT_ALL :
            global_settings.repeat_mode == REPEAT_ALL ? REPEAT_ONE : REPEAT_OFF;
        if (audio_status() & AUDIO_STATUS_PLAY)
            audio_flush_and_reload_tracks();
        break;
    case SET_SLEEP:
    {
        int current = get_sleep_timer_active()
            ? (get_sleep_timer() + 59) / 60 : 0;
        int next = 0;
        for (i = 0; i < sizeof(sleep_steps); i++)
            if (sleep_steps[i] > current) { next = sleep_steps[i]; break; }
        if (next)
            global_settings.sleeptimer_duration = next;
        set_sleeptimer_duration(next);
        break;
    }
    case SET_BRIGHTNESS:
        global_settings.contrast =
            brightness_steps[(brightness_index() + 1) % 3];
        lcd_set_contrast(global_settings.contrast);
        break;
    case SET_ALARM:
#ifdef HAVE_RTC_ALARM
        settings_alarm();
#else
        focus_toast("No alarm");
#endif
        return;
    case SET_CLOCK:
    {
        struct tm tm = *get_time();
        if (!valid_time(&tm) || tm.tm_year < 124)
        {
            memset(&tm, 0, sizeof(tm));
            tm.tm_year = 126; tm.tm_mon = 0; tm.tm_mday = 1; tm.tm_hour = 12;
        }
        if (edit_time("Set clock", &tm.tm_hour, &tm.tm_min) && edit_date(&tm))
        {
            tm.tm_sec = 0;
            set_day_of_week(&tm);
            set_time(&tm);
            focus_toast("Clock set");
        }
        return;
    }
    case SET_ABOUT:
        settings_about();
        return;
    }
    settings_save();
}

static void settings_list(const char *title, const unsigned char *items,
                          int count)
{
    struct focus_list list;

    focus_list_init(&list, title);
    list.count = count;
    list.name = settings_name;
    list.value = settings_value;
    list.ctx = (void *)items;
    while (true)
    {
        int action;
        focus_list_draw(&list);
        focus_flush();
        action = focus_get_action(FOCUS_CTX_NAV);
        if (focus_list_input(&list, action))
            continue;
        if (action == ACTION_STD_MENU)
            return;
        if (action == ACTION_STD_OK)
            settings_activate(items[list.selected]);
    }
}

void focus_settings(void)
{
    settings_list("Settings", settings_all, sizeof(settings_all));
}

void focus_playback_options(void)
{
    settings_list("Options", settings_playback, sizeof(settings_playback));
}
