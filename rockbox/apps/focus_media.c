/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Folder browser and Now Playing for the Clip+ focus shell.
 * Project-original fork code. Playback uses the same playlist calls as
 * Rockbox's own file browser (create, insert context, shuffle, start). */
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string-extra.h>
#include "action.h"
#include "audio.h"
#include "dir.h"
#include "filetypes.h"
#include "kernel.h"
#include "lcd.h"
#include "metadata.h"
#include "misc.h"
#include "playback.h"
#include "playlist.h"
#include "settings.h"
#include "strnatcmp.h"
#include "recorder/recording.h"
#include "focus_ui.h"

bool focus_recording_screen(void); /* apps/recorder/recording.c */

#define MEDIA_MAX   400
#define MEDIA_POOL  16384
#define MEDIA_DEPTH 8

static char media_pool[MEDIA_POOL];
static unsigned short media_off[MEDIA_MAX];
static unsigned char media_is_dir[MEDIA_MAX];
static short media_order[MEDIA_MAX];
static int media_count, media_files;
static char media_dir[MAX_PATH];
static bool media_recordings;

/* ------------------------------------------------------------ names ---- */

void focus_pretty_name(char *dst, size_t size, const char *filename)
{
    const char *base = strrchr(filename, '/');
    char *dot, *p, *out;
    bool has_space;

    base = base ? base + 1 : filename;
    strlcpy(dst, base, size);
    dot = strrchr(dst, '.');
    if (dot && dot != dst)
        *dot = '\0';
    has_space = strchr(dst, ' ') != NULL;
    for (p = dst; *p; p++)
        if (*p == '_' || (*p == '-' && !has_space))
            *p = ' ';
    /* collapse runs of spaces */
    for (p = out = dst; *p; p++)
        if (*p != ' ' || (out > dst && out[-1] != ' '))
            *out++ = *p;
    *out = '\0';
}

static const char *entry_name(int entry)
{
    return media_pool + media_off[entry];
}

static void entry_path(int entry, char *buf, size_t size)
{
    strlcpy(buf, media_dir, size);
    strlcat(buf, "/", size);
    strlcat(buf, entry_name(entry), size);
}

static int media_compare(const void *a, const void *b)
{
    int ea = *(const short *)a, eb = *(const short *)b;
    if (media_is_dir[ea] != media_is_dir[eb])
        return media_is_dir[eb] - media_is_dir[ea];
    return strnatcasecmp(entry_name(ea), entry_name(eb));
}

static bool media_scan(void)
{
    DIR *dir = opendir(media_dir);
    struct dirent *entry;
    size_t used = 0;

    media_count = media_files = 0;
    if (!dir)
        return false;
    while (media_count < MEDIA_MAX && (entry = readdir(dir)))
    {
        struct dirinfo info = dir_get_info(dir, entry);
        size_t len = strlen(entry->d_name) + 1;
        bool is_dir = (info.attribute & ATTR_DIRECTORY) != 0;

        if (entry->d_name[0] == '.' || used + len > MEDIA_POOL)
            continue;
        if (!is_dir)
        {
            int attr = filetype_get_attr(entry->d_name);
            if ((attr & FILE_ATTR_MASK) != FILE_ATTR_AUDIO)
                continue;
            media_files++;
        }
        memcpy(media_pool + used, entry->d_name, len);
        media_off[media_count] = (unsigned short)used;
        media_is_dir[media_count] = is_dir;
        media_order[media_count] = (short)media_count;
        used += len;
        media_count++;
    }
    closedir(dir);
    qsort(media_order, media_count, sizeof(media_order[0]), media_compare);
    return true;
}

/* --------------------------------------------------------- playback ---- */

bool focus_playback_active(void)
{
    return (audio_status() & AUDIO_STATUS_PLAY) != 0;
}

/* Short "what is on" text for the Home header; false when idle. */
bool focus_playing_title(char *buf, size_t size)
{
    struct mp3entry *id3;
    if (!focus_playback_active())
        return false;
    id3 = audio_current_track();
    if (!id3 || !id3->path[0])
        return false;
    if (id3->title && id3->title[0])
        strlcpy(buf, id3->title, size);
    else
        focus_pretty_name(buf, size, id3->path);
    return true;
}

/* Queue every audio file of the current folder, in list order, and start at
 * `row` (an index into media_order) or shuffled when row < 0. */
static bool media_play(int row)
{
    struct playlist_insert_context context;
    char path[MAX_PATH];
    int i, start = 0, index = 0;
    bool ok = true;

    if (media_files <= 0 || playlist_create(media_dir, NULL) < 0)
        return false;
    if (playlist_insert_context_create(NULL, &context, PLAYLIST_REPLACE,
                                       false, false) < 0)
        return false;
    for (i = 0; i < media_count; i++)
    {
        int entry = media_order[i];
        if (media_is_dir[entry])
            continue;
        if (i == row)
            start = index;
        entry_path(entry, path, sizeof(path));
        if (playlist_insert_context_add(&context, path) < 0)
        {
            ok = false;
            break;
        }
        index++;
    }
    playlist_insert_context_release(&context);
    if (!ok || index == 0)
        return false;
    if (row < 0)
        start = playlist_shuffle(current_tick, -1);
    else if (global_settings.playlist_shuffle)
        start = playlist_shuffle(current_tick, start);
    if (start < 0)
        start = 0;
    playlist_start(start, 0, 0);
    return true;
}

/* Pick up the last session where it stopped (long mixes and podcasts). */
static bool media_resume(void)
{
    if (global_status.resume_index == -1 || playlist_resume() == -1)
        return false;
    playlist_resume_track(global_status.resume_index,
                          global_status.resume_crc32,
                          global_status.resume_elapsed,
                          global_status.resume_offset);
    return true;
}

/* ------------------------------------------------------ now playing ---- */

static void time_text(char *buf, size_t size, long ms, bool negative)
{
    long s = ms / 1000;
    if (s >= 3600)
        snprintf(buf, size, "%s%ld:%02ld:%02ld", negative ? "-" : "",
                 s / 3600, (s / 60) % 60, s % 60);
    else
        snprintf(buf, size, "%s%ld:%02ld", negative ? "-" : "", s / 60, s % 60);
}

static void now_playing_draw(struct mp3entry *id3, int status, long position,
                             long since, bool loading)
{
    char title[MAX_PATH], header[24], text[40];
    const char *artist = NULL;
    long length = id3 ? (long)id3->length : 0;
    int amount = playlist_amount();
    int x;

    if (amount > 1)
        snprintf(header, sizeof(header), "%d / %d",
                 playlist_get_display_index(), amount);
    else
        strlcpy(header, "Now Playing", sizeof(header));
    focus_begin(header);

    if (loading || !id3)
        strlcpy(title, "Loading...", sizeof(title));
    else if (id3->title && id3->title[0])
        strlcpy(title, id3->title, sizeof(title));
    else
        focus_pretty_name(title, sizeof(title), id3->path);
    if (id3 && !loading)
        artist = (id3->artist && id3->artist[0]) ? id3->artist :
                 (id3->album && id3->album[0]) ? id3->album : NULL;

    focus_text_marquee(&focus_font_title, 2, artist ? 17 : 22, 124, title,
                       since, true);
    if (artist)
        focus_text_marquee(&focus_font_small, 2, 35, 124, artist, since, true);

    if (position > length) position = length;
    focus_bar(2, 47, 124, 6, position, length);
    time_text(text, sizeof(text), position, false);
    focus_text(&focus_font_small, 2, 54, text);
    time_text(text, sizeof(text), length - position, true);
    focus_text_right(&focus_font_small, 126, 54, text);

    x = 64 - 4;
    if (global_settings.playlist_shuffle)
        focus_icon_draw(FOCUS_ICON_S_SHUFFLE, x - 14, 56);
    focus_icon_draw((status & AUDIO_STATUS_PAUSE) || !status
                    ? FOCUS_ICON_S_PAUSE : FOCUS_ICON_S_PLAY, x, 56);
    if (global_settings.repeat_mode != REPEAT_OFF)
    {
        focus_icon_draw(FOCUS_ICON_S_REPEAT, x + 13, 56);
        if (global_settings.repeat_mode == REPEAT_ONE)
            focus_icon_draw(FOCUS_ICON_S_ONE, x + 23, 57);
    }
    focus_flush();
}

void focus_now_playing(void)
{
    long loading_until = current_tick + HZ * 5;
    long since = current_tick;
    long seek_pos = 0, seek_tick = 0;
    int seek_count = 0;
    bool seeking = false, was_ready = false;
    char last_path[MAX_PATH] = "";
    int generation = focus_storage_generation;

#if CONFIG_TUNER
    if (focus_radio_active())
    {
        focus_radio();
        return;
    }
#endif
    if (!audio_status() && !media_resume())
    {
        focus_message("Now Playing", "Nothing playing",
                      "Pick something from Music");
        sleep(HZ * 3 / 2);
        return;
    }

    while (true)
    {
        struct mp3entry *id3 = audio_current_track();
        int status = audio_status();
        bool ready = status && id3 && id3->path[0];
        int action;

        if (generation != focus_storage_generation)
            return;
        if (ready)
        {
            was_ready = true;
            if (strcmp(last_path, id3->path))
            {
                strlcpy(last_path, id3->path, sizeof(last_path));
                since = current_tick;
            }
        }
        else if (was_ready && !status)
            return; /* playlist finished */
        else if (TIME_AFTER(current_tick, loading_until))
        {
            focus_message("Now Playing", "Can't play this", NULL);
            sleep(HZ * 2);
            return;
        }

        /* a held seek that never saw its release still gets committed */
        if (seeking && TIME_AFTER(current_tick, seek_tick + HZ))
        {
            audio_ff_rewind(seek_pos);
            seeking = false;
        }
        now_playing_draw(id3, status,
                         seeking ? seek_pos : (id3 ? (long)id3->elapsed : 0),
                         since, !ready);

        action = focus_get_action(FOCUS_CTX_PLAY);
        if (!seeking && ready)
            focus_standby_check(action);
        if (generation != focus_storage_generation)
            return;
        if (!ready && action != ACTION_STD_MENU)
            continue;
        switch (action)
        {
        case ACTION_STD_MENU:
            if (seeking)
                audio_ff_rewind(seek_pos);
            return;
        case ACTION_WPS_PLAY:
            if (status & AUDIO_STATUS_PAUSE)
                audio_resume();
            else
                audio_pause();
            break;
        case ACTION_WPS_SKIPNEXT:
            audio_next();
            break;
        case ACTION_WPS_SKIPPREV:
            if (id3->elapsed > 3000)
            {
                audio_pre_ff_rewind();
                audio_ff_rewind(0);
            }
            else
                audio_prev();
            break;
        case ACTION_WPS_SEEKBACK:
        case ACTION_WPS_SEEKFWD:
        {
            long step = seek_count < 10 ? 3000 : seek_count < 25 ? 10000 : 30000;
            if (!seeking)
            {
                seeking = true;
                seek_count = 0;
                seek_pos = (long)id3->elapsed;
                audio_pre_ff_rewind();
            }
            seek_count++;
            seek_pos += action == ACTION_WPS_SEEKFWD ? step : -step;
            if (seek_pos < 0) seek_pos = 0;
            if (id3->length && seek_pos > (long)id3->length - 1000)
                seek_pos = (long)id3->length - 1000;
            if (seek_pos < 0) seek_pos = 0;
            seek_tick = current_tick;
            break;
        }
        case ACTION_WPS_STOPSEEK:
            if (seeking)
            {
                audio_ff_rewind(seek_pos);
                seeking = false;
            }
            break;
        case ACTION_WPS_CONTEXT:
            focus_playback_options();
            break;
        default:
            break;
        }
    }
}

/* ---------------------------------------------------------- browser ---- */

static int browse_extra(void)
{
    if (media_recordings)
        return 1;                    /* "New recording" */
    return media_files >= 2 ? 1 : 0; /* "Shuffle all" */
}

static const char *browse_name(int index, void *ctx, char *buf, size_t size)
{
    int extra = browse_extra();
    int entry;
    (void)ctx;
    if (index < extra)
        return media_recordings ? "New recording" : "Shuffle all";
    entry = media_order[index - extra];
    if (media_is_dir[entry])
        return entry_name(entry);
    focus_pretty_name(buf, size, entry_name(entry));
    return buf;
}

static int browse_icon(int index, void *ctx)
{
    int extra = browse_extra();
    int entry;
    char path[MAX_PATH];
    struct mp3entry *id3;
    (void)ctx;
    if (index < extra)
        return media_recordings ? FOCUS_ICON_S_REC : FOCUS_ICON_S_SHUFFLE;
    entry = media_order[index - extra];
    if (media_is_dir[entry])
        return FOCUS_ICON_S_FOLDER;
    id3 = audio_status() ? audio_current_track() : NULL;
    if (id3 && id3->path[0])
    {
        entry_path(entry, path, sizeof(path));
        if (!strcasecmp(path, id3->path))
            return FOCUS_ICON_S_PLAY;
    }
    return FOCUS_ICON_S_NOTE;
}

void focus_browse(const char *root, const char *title, bool recordings)
{
    struct focus_list list;
    int stack[MEDIA_DEPTH];
    int depth = 0;
    int generation = focus_storage_generation;
    bool found;

    strlcpy(media_dir, root, sizeof(media_dir));
    media_recordings = recordings;
    found = media_scan();
    focus_list_init(&list, title);
    list.name = browse_name;
    list.icon = browse_icon;
    list.empty = found ? "No audio files yet" : "Folder not found";

    while (true)
    {
        int action, extra = browse_extra();

        if (generation != focus_storage_generation)
        {
            /* files may have changed over USB: start again from the top */
            generation = focus_storage_generation;
            strlcpy(media_dir, root, sizeof(media_dir));
            depth = 0;
            found = media_scan();
            list.selected = list.top = 0;
            list.empty = found ? "No audio files yet" : "Folder not found";
        }
        list.count = media_count + extra;
        list.title = depth ? strrchr(media_dir, '/') + 1 : title;
        focus_list_draw(&list);
        focus_flush();

        action = focus_get_action(FOCUS_CTX_NAV);
        if (focus_list_input(&list, action))
            continue;
        if (action == ACTION_STD_MENU)
        {
            char *slash;
            if (depth == 0)
                return;
            slash = strrchr(media_dir, '/');
            if (slash)
                *slash = '\0';
            media_scan();
            list.selected = stack[--depth];
            list.top = 0;
            list.since = current_tick;
        }
        else if (action == ACTION_STD_OK && list.count > 0)
        {
            int row = list.selected - extra;
            if (row < 0 && recordings)
            {
#ifdef HAVE_RECORDING
                focus_recording_screen();
                media_scan();
#endif
            }
            else if (row >= 0 && media_is_dir[media_order[row]])
            {
                if (depth < MEDIA_DEPTH &&
                    strlen(media_dir) + strlen(entry_name(media_order[row]))
                        + 2 < sizeof(media_dir))
                {
                    char name[MAX_PATH];
                    strlcpy(name, entry_name(media_order[row]), sizeof(name));
                    strlcat(media_dir, "/", sizeof(media_dir));
                    strlcat(media_dir, name, sizeof(media_dir));
                    stack[depth++] = list.selected;
                    media_scan();
                    list.selected = list.top = 0;
                    list.since = current_tick;
                }
            }
            else if (media_play(row))
                focus_now_playing();
            else
                focus_toast("Can't play this");
        }
    }
}
