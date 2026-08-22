/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Focus-player media screens.  These deliberately do not enter Rockbox's
 * tree or WPS screens: the shell owns the complete first-pass interaction. */
#include "config.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <string-extra.h>
#include "action.h"
#include "audio.h"
#include "dir.h"
#include "filetypes.h"
#include "font.h"
#include "focus_font.h"
#include "lcd.h"
#include "metadata.h"
#include "misc.h"
#include "playlist.h"
#include "kernel.h"
#include "settings.h"
#include "status.h"
#include "sound.h"
#include "screens.h"
#include "recorder/recording.h"
#include "radio/radio.h"
#include "alarm_menu.h"

#define FOCUS_MEDIA_MAX 32

static char media_paths[FOCUS_MEDIA_MAX][MAX_PATH];
static char media_names[FOCUS_MEDIA_MAX][MAX_PATH];
static int media_count;
static int media_selected;

static void upper_copy(char *dst, size_t size, const char *src)
{
    size_t i;
    if (!size) return;
    for (i = 0; i + 1 < size && src[i]; ++i)
        dst[i] = (char)toupper((unsigned char)src[i]);
    dst[i] = '\0';
}

static void short_name(char *dst, size_t size, const char *path)
{
    const char *name = strrchr(path, '/');
    char temp[MAX_PATH];
    char *dot;
    name = name ? name + 1 : path;
    upper_copy(temp, sizeof(temp), name);
    dot = strrchr(temp, '.');
    if (dot) *dot = '\0';
    strlcpy(dst, temp, size);
}

static void draw_title(const char *title, int y)
{
    char first[11], second[11];
    size_t n = strlen(title);
    memset(first, 0, sizeof(first));
    memset(second, 0, sizeof(second));
    strlcpy(first, title, sizeof(first));
    if (n > 10) strlcpy(second, title + 10, sizeof(second));
    focus_big_text_centered(first, y);
    if (second[0]) focus_big_text_centered(second, y + 18);
}

static void media_draw(const char *heading, const char *message)
{
    focus_screen_header(heading);
    if (message) draw_title(message, 23);
    lcd_update();
}

static bool media_scan(const char *folder)
{
    DIR *dir = opendir(folder);
    struct dirent *entry;
    media_count = 0;
    media_selected = 0;
    if (!dir) return false;
    while (media_count < FOCUS_MEDIA_MAX && (entry = readdir(dir)))
    {
        struct dirinfo info = dir_get_info(dir, entry);
        char path[MAX_PATH];
        int attr;
        if (info.attribute & ATTR_DIRECTORY) continue;
        snprintf(path, sizeof(path), "%s/%s", folder, entry->d_name);
        attr = filetype_get_attr(path);
        if (!filetype_supported(attr) || !(attr & FILE_ATTR_AUDIO)) continue;
        strlcpy(media_paths[media_count], path, sizeof(media_paths[0]));
        short_name(media_names[media_count], sizeof(media_names[0]), path);
        ++media_count;
    }
    closedir(dir);
    return true;
}

static void media_list_draw(const char *heading)
{
    focus_screen_header(heading);
    if (!media_count)
        draw_title("EMPTY", 25);
    else
        draw_title(media_names[media_selected], 22);
    lcd_set_drawmode(DRMODE_SOLID);
    lcd_vline(3, 30, 35);
    lcd_vline(LCD_WIDTH - 4, 30, 35);
    focus_screen_position(media_selected, media_count);
    lcd_update();
}

static void focus_now_playing_draw(struct mp3entry *id3, int status,
                                   const char *feedback,
                                   bool starting)
{
    char shown[MAX_PATH];
    char time_text[32];
    unsigned long elapsed = id3 ? id3->elapsed : 0;
    unsigned long length = id3 ? id3->length : 0;
    int width = 0;
    const char *state = starting ? "STARTING" :
                        ((status & AUDIO_STATUS_PAUSE) ? "PAUSED" : "PLAYING");
    char section[16];

    if (feedback)
        strlcpy(section, feedback, sizeof(section));
    else
    {
        strlcpy(section, "NOW PLAYING", sizeof(section));
    }
    focus_screen_header(section);
    lcd_setfont(FONT_SYSFIXED);
    lcd_putsxy(4, 14, state);
    focus_screen_battery(82, 14);

    {
        const char *title = id3 && id3->title ? id3->title : "STARTING";
        upper_copy(shown, sizeof(shown), title);
    }
    draw_title(shown, 16);
    if (length)
    {
        if (elapsed > length) elapsed = length;
        width = (int)(((LCD_WIDTH - 8) * elapsed) / length);
    }
    lcd_hline(4, LCD_WIDTH - 5, 49);
    if (width > 0)
        lcd_fillrect(4, 48, width, 3);
    snprintf(time_text, sizeof(time_text), "%lu:%02lu -%lu:%02lu /%lu:%02lu",
             elapsed / 60000, (elapsed / 1000) % 60,
             (length > elapsed ? length - elapsed : 0) / 60000,
             ((length > elapsed ? length - elapsed : 0) / 1000) % 60,
             length / 60000, (length / 1000) % 60);
    lcd_setfont(FONT_SYSFIXED);
    lcd_putsxy(4, 55, time_text);
    lcd_update();
}

static void focus_volume_overlay_draw(void)
{
    int min = sound_min(SOUND_VOLUME);
    int max = sound_max(SOUND_VOLUME);
    int value = global_status.volume;
    int level = max > min ? (value - min) * 10 / (max - min) : 0;
    char value_text[20];

    if (level < 0) level = 0;
    if (level > 10) level = 10;
    focus_screen_header("VOLUME");
    focus_big_text_centered("VOLUME", 15);
    snprintf(value_text, sizeof(value_text), "%d", value);
    focus_big_text_centered(value_text, 31);
    for (int i = 0; i < 10; ++i)
    {
        int height = 4 + i * 4;
        int x = 7 + i * 12;
        if (i < level)
            lcd_fillrect(x, 57 - height, 8, height);
        else
            lcd_drawrect(x, 57 - height, 8, height);
    }
    lcd_update();
}

static bool media_play_selected(void)
{
    char playlist_dir[MAX_PATH];

    if (!media_count)
        return false;
    strlcpy(playlist_dir, media_paths[media_selected], sizeof(playlist_dir));
    char *filename = strrchr(playlist_dir, '/');
    if (!filename || filename == playlist_dir || !filename[1])
        return false;
    *filename = '\0';

    /* Match Rockbox's proven audio-file path: replace the current playlist
     * with the selected directory, then insert exactly one absolute track.
     * Passing the MP3 as playlist_create's FILE argument would parse binary
     * audio as a playlist and produce PLAYLIST BUFFER FULL. */
    if (playlist_create(playlist_dir, NULL) < 0 ||
        playlist_insert_track(NULL, media_paths[media_selected],
                              PLAYLIST_INSERT_LAST, false, true) < 0)
        return false;
    playlist_start(0, 0, 0);
    return true;
}

void focus_now_playing(void)
{
    int action;
    int startup_ticks = HZ * 3;
    int volume_overlay_ticks = 0;
    const char *seek_feedback = NULL;
    while (true)
    {
        struct mp3entry *id3 = audio_current_track();
        int status = audio_status();
        bool ready = id3 && id3->path[0];
        if (!ready && startup_ticks <= 0)
        {
            media_draw("NOW PLAYING", "PLAYBACK ERROR");
            sleep(HZ * 2);
            return;
        }
        if (volume_overlay_ticks > 0)
            focus_volume_overlay_draw();
        else
            focus_now_playing_draw(id3, status, seek_feedback, !ready);
        if (volume_overlay_ticks > 0)
            volume_overlay_ticks -= HZ / 4;
        seek_feedback = NULL;
        action = get_action(CONTEXT_CUSTOM | ALLOW_SOFTLOCK, HZ / 4);
        if (action == ACTION_STD_MENU) return;
        if (action == ACTION_STD_CANCEL) { default_event_handler(action); continue; }
        if (!ready && startup_ticks > 0)
            startup_ticks -= HZ / 4;
        if (action == ACTION_STD_PREV) audio_prev();
        else if (action == ACTION_STD_NEXT) audio_next();
        else if (action == ACTION_STD_OK || action == ACTION_WPS_PLAY)
        {
            if (status & AUDIO_STATUS_PAUSE) audio_resume();
            else if (status & AUDIO_STATUS_PLAY) audio_pause();
            else audio_play(0, 0);
        }
        else if (action == ACTION_WPS_VOLUP || action == ACTION_WPS_VOLDOWN)
        {
            adjust_volume(action == ACTION_WPS_VOLUP ? 1 : -1);
            setvol();
            volume_overlay_ticks = HZ * 2;
        }
        else if (action == ACTION_WPS_SEEKBACK || action == ACTION_WPS_SEEKFWD)
        {
            long newpos = id3 ? (long)id3->elapsed : 0;
            long step = 5000;
            newpos += action == ACTION_WPS_SEEKFWD ? step : -step;
            if (newpos < 0) newpos = 0;
            if (id3 && id3->length && (unsigned long)newpos > id3->length)
                newpos = (long)id3->length;
            audio_pre_ff_rewind();
            audio_ff_rewind(newpos);
            seek_feedback = action == ACTION_WPS_SEEKFWD ? "SEEK +5" : "SEEK -5";
        }
        else default_event_handler(action);
    }
}

static void focus_media_list(const char *folder, const char *heading)
{
    int action;
    if (!media_scan(folder))
    {
        media_draw(heading, "STORAGE ERROR");
        while ((action = get_action(CONTEXT_STD | ALLOW_SOFTLOCK, HZ / 4)) != ACTION_STD_MENU)
            if (action == ACTION_STD_CANCEL) default_event_handler(action);
        return;
    }
    while (true)
    {
        media_list_draw(heading);
        action = get_action(CONTEXT_STD | ALLOW_SOFTLOCK, HZ / 4);
        if (action == ACTION_STD_MENU) return;
        if (action == ACTION_STD_CANCEL) { default_event_handler(action); continue; }
        if (action == ACTION_STD_PREV && media_count)
        {
            media_selected = (media_selected + media_count - 1) % media_count;
        }
        else if (action == ACTION_STD_NEXT && media_count)
        {
            media_selected = (media_selected + 1) % media_count;
        }
        else if (action == ACTION_STD_OK && media_count)
        {
            if (media_play_selected())
                focus_now_playing();
            else
            {
                media_draw(heading, "PLAYBACK ERROR");
                sleep(HZ * 2);
            }
        }
        else default_event_handler(action);
    }
}

void focus_open_music(void) { focus_media_list("/MUSIC", "MUSIC"); }
void focus_open_podcasts(void) { focus_media_list("/PODCASTS", "PODCASTS"); }

void focus_open_alarm(void)
{
    int action;
#ifdef HAVE_RTC_ALARM
    do {
        media_draw("ALARM", "SET REMINDER");
        action = get_action(CONTEXT_STD | ALLOW_SOFTLOCK, HZ / 4);
        if (action == ACTION_STD_CANCEL) default_event_handler(action);
        else if (action == ACTION_STD_OK)
            alarm_screen();
    } while (action != ACTION_STD_MENU);
#else
    media_draw("ALARM", "UNAVAILABLE");
    sleep(HZ * 2);
#endif
}

void focus_open_radio(void)
{
    int action;
    do {
        media_draw("RADIO", "FM READY");
        action = get_action(CONTEXT_STD | ALLOW_SOFTLOCK, HZ / 4);
        if (action == ACTION_STD_CANCEL) default_event_handler(action);
        else if (action == ACTION_STD_OK)
            /* Controlled entry to the proven tuner implementation. The
             * custom wrapper owns the destination and returns to focus after
             * the tuner exits; no root/plugin browser is exposed. */
            radio_screen();
    } while (action != ACTION_STD_MENU);
}

void focus_open_recordings(void)
{
    int action;
    do {
        media_draw("RECORDINGS", "SELECT TO OPEN");
        action = get_action(CONTEXT_STD | ALLOW_SOFTLOCK, HZ / 4);
        if (action == ACTION_STD_CANCEL) default_event_handler(action);
        else if (action == ACTION_STD_OK)
            recording_screen(false);
    } while (action != ACTION_STD_MENU);
}
