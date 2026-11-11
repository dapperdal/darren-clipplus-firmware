/* SPDX-License-Identifier: GPL-2.0-or-later */
/* FM radio screen for the Clip+ focus shell. Project-original fork code.
 *
 * The tuner is driven through exactly the calls Rockbox's own radio screen
 * uses (audio_set_input_source, next_station, preset_next, tuner_set), in
 * the same order, so the proven Si4702 path is unchanged. Only the drawing,
 * the key handling and the preset bookkeeping are new. */
#include "config.h"
#if CONFIG_TUNER
#include <stdio.h>
#include <string.h>
#include <string-extra.h>
#include "action.h"
#include "audio.h"
#include "kernel.h"
#include "lcd.h"
#include "misc.h"
#include "pcm.h"
#include "settings.h"
#include "system.h"
#include "tuner.h"
#include "radio.h"
#include "presets.h"
#include "focus_ui.h"

/* small additions in apps/radio/radio.c and presets.c */
int get_radio_status(void);
void remember_frequency(void);
void focus_radio_set_in_screen(bool in_screen);
void focus_presets_load(void);
void focus_presets_save(void);
void focus_presets_clear(void);
int focus_preset_add(int freq, const char *name);
void focus_preset_remove(int preset);

static long name_since;

bool focus_radio_active(void)
{
    return get_radio_status() != FMRADIO_OFF;
}

bool focus_radio_title(char *buf, size_t size)
{
    char freq[16];
    int whole, frac;
    if (!focus_radio_active())
        return false;
    whole = radio_get_current_frequency() / 1000000;
    frac = (radio_get_current_frequency() % 1000000) / 100000;
    snprintf(freq, sizeof(freq), "%d.%d", whole, frac);
    snprintf(buf, size, "FM %s", freq);
    return true;
}

static const struct fm_region_data *region(void)
{
    return &fm_region_data[global_settings.fm_region];
}

static int step_freq(int freq, int direction)
{
    const struct fm_region_data *fmr = region();
    freq += direction * fmr->freq_step;
    if (freq > fmr->freq_max) freq = fmr->freq_min;
    if (freq < fmr->freq_min) freq = fmr->freq_max;
    return freq;
}

static void freq_text(char *buf, size_t size, int freq)
{
    int whole = freq / 1000000;
    int frac = (freq % 1000000) / 10000;
    if (region()->freq_step % 100000)
        snprintf(buf, size, "%d.%02d", whole, frac);
    else
        snprintf(buf, size, "%d.%d", whole, frac / 10);
}

static void station_name(char *buf, size_t size, int preset)
{
    buf[0] = '\0';
    if (preset >= 0)
        radio_get_preset_name(preset, buf, size);
#ifdef HAVE_RDS_CAP
    if (!buf[0] && get_radio_status() == FMRADIO_PLAYING)
        tuner_get_rds_info(RADIO_RDS_NAME, buf, size);
#endif
}

static void radio_draw(bool seeking)
{
    char text[32], name[40], title[24];
    const struct fm_region_data *fmr = region();
    int freq = radio_get_current_frequency();
    int preset = radio_current_preset();
    int status = get_radio_status();
    int count = radio_preset_count();
    int w, x, i;

    if (radio_get_mode() == RADIO_PRESET_MODE && preset >= 0)
        snprintf(title, sizeof(title), "FM  %d / %d", preset + 1, count);
    else
        strlcpy(title, "FM Radio", sizeof(title));
    focus_begin(title);

    freq_text(text, sizeof(text), freq);
    w = focus_text_width(&focus_font_huge, text);
    x = (LCD_WIDTH - w - 24) / 2;
    focus_text(&focus_font_huge, x, 16, text);
    focus_text(&focus_font_small, x + w + 3, 29, "MHz");
    if (status == FMRADIO_PLAYING && radio_is_stereo())
        focus_icon_draw(FOCUS_ICON_S_STEREO, x + w + 5, 20);
    if (preset >= 0)
        focus_icon_draw(FOCUS_ICON_S_STAR, 2, 19);

    if (seeking)
        strlcpy(name, "Seeking...", sizeof(name));
    else if (status != FMRADIO_PLAYING)
        strlcpy(name, "Muted", sizeof(name));
    else
    {
        station_name(name, sizeof(name), preset);
        if (!name[0] && count == 0)
            strlcpy(name, "Hold < > to seek", sizeof(name));
    }
    focus_text_marquee(&focus_font_small, 2, 43, 124, name, name_since, true);

    /* band dial with a tick per saved station */
    lcd_hline(4, 123, 61);
    for (i = 0; i < count; i++)
    {
        int f = radio_get_preset_freq(i);
        if (f >= fmr->freq_min && f <= fmr->freq_max)
            lcd_vline(4 + (int)((long long)(f - fmr->freq_min) * 119 /
                                (fmr->freq_max - fmr->freq_min)), 58, 60);
    }
    x = 4 + (int)((long long)(freq - fmr->freq_min) * 119 /
                  (fmr->freq_max - fmr->freq_min));
    lcd_fillrect(x - 1, 55, 3, 8);
    focus_flush();
}

/* --------------------------------------------------------- stations ---- */

static const char *preset_row_name(int index, void *ctx, char *buf, size_t size)
{
    (void)ctx;
    radio_get_preset_name(index, buf, size);
    if (!buf[0])
        strlcpy(buf, "Station", size);
    return buf;
}

static const char *preset_row_value(int index, void *ctx, char *buf, size_t size)
{
    (void)ctx;
    freq_text(buf, size, radio_get_preset_freq(index));
    return buf;
}

static void tune_to_preset(int preset)
{
    radio_set_current_frequency(radio_get_preset_freq(preset));
    radio_set_mode(RADIO_SCAN_MODE);
    next_station(0); /* retune, find matching preset, remember frequency */
    radio_set_mode(RADIO_PRESET_MODE);
    name_since = current_tick;
}

static void radio_stations(void)
{
    struct focus_list list;

    focus_list_init(&list, "Stations");
    list.name = preset_row_name;
    list.value = preset_row_value;
    list.empty = "No saved stations";
    if (radio_current_preset() >= 0)
        list.selected = radio_current_preset();
    while (true)
    {
        int action;
        list.count = radio_preset_count();
        focus_list_draw(&list);
        focus_flush();
        action = focus_get_action(FOCUS_CTX_NAV);
        if (focus_list_input(&list, action))
            continue;
        if (action == ACTION_STD_MENU)
            return;
        if (action == ACTION_STD_OK && list.count > 0)
        {
            tune_to_preset(list.selected);
            return;
        }
    }
}

static void radio_autoscan(void)
{
    const struct fm_region_data *fmr = region();
    int freq = fmr->freq_min;
    int found = 0;
    char text[32];

    focus_presets_clear();
    tuner_set(RADIO_MUTE, 1);
    while (freq <= fmr->freq_max && found < 64)
    {
        int action;

        focus_begin("Auto scan");
        freq_text(text, sizeof(text), freq);
        focus_text_center(&focus_font_title, 18, text);
        focus_bar(8, 38, 112, 7, freq - fmr->freq_min,
                  fmr->freq_max - fmr->freq_min);
        snprintf(text, sizeof(text), "%d found   HOME stops", found);
        focus_text_center(&focus_font_small, 50, text);
        focus_flush();

        action = focus_get_action_timeout(FOCUS_CTX_NAV, TIMEOUT_NOBLOCK);
        if (action == ACTION_STD_MENU)
            break;
        if (tuner_set(RADIO_SCAN_FREQUENCY, freq) &&
            focus_preset_add(freq, "") >= 0)
            found++;
        freq += fmr->freq_step;
        yield();
    }
    if (get_radio_status() == FMRADIO_PLAYING)
        tuner_set(RADIO_MUTE, 0);
    focus_presets_save();
    if (found > 0)
        tune_to_preset(0);
    else
    {
        radio_set_current_frequency(fmr->freq_min);
        radio_set_mode(RADIO_SCAN_MODE);
        next_station(0);
    }
    snprintf(text, sizeof(text), "%d stations saved", found);
    focus_toast(text);
}

static void radio_toggle_saved(void)
{
    char name[MAX_FMPRESET_LEN + 1] = "";
    int preset = radio_current_preset();

    if (preset >= 0)
    {
        focus_preset_remove(preset);
        preset_set_current(-1);
        if (radio_preset_count() == 0)
            radio_set_mode(RADIO_SCAN_MODE);
        focus_toast("Station removed");
    }
    else
    {
#ifdef HAVE_RDS_CAP
        tuner_get_rds_info(RADIO_RDS_NAME, name, sizeof(name));
#endif
        preset = focus_preset_add(radio_get_current_frequency(), name);
        if (preset < 0)
        {
            focus_toast("Station list full");
            return;
        }
        preset_set_current(preset);
        radio_set_mode(RADIO_PRESET_MODE);
        focus_toast("Station saved");
    }
    focus_presets_save();
}

enum { RM_SAVE, RM_STATIONS, RM_SCAN, RM_MODE, RM_SOUND, RM_OFF, RM_COUNT };

static const char *menu_name(int index, void *ctx, char *buf, size_t size)
{
    (void)ctx; (void)buf; (void)size;
    switch (index)
    {
    case RM_SAVE:
        return radio_current_preset() >= 0 ? "Remove station" : "Save station";
    case RM_STATIONS: return "Stations";
    case RM_SCAN: return "Auto scan";
    case RM_MODE: return "Left / Right";
    case RM_SOUND: return "Sound";
    default: return "Radio off";
    }
}

static const char *menu_value(int index, void *ctx, char *buf, size_t size)
{
    (void)ctx;
    switch (index)
    {
    case RM_STATIONS:
        snprintf(buf, size, "%d", radio_preset_count());
        return buf;
    case RM_MODE:
        return radio_get_mode() == RADIO_PRESET_MODE ? "Stations" : "Tune";
    case RM_SOUND:
        return global_settings.fm_force_mono ? "Mono" : "Stereo";
    default:
        return NULL;
    }
}

/* Returns true when the radio should be switched off. */
static bool radio_menu(void)
{
    struct focus_list list;

    focus_list_init(&list, "Radio");
    list.name = menu_name;
    list.value = menu_value;
    list.count = RM_COUNT;
    while (true)
    {
        int action;
        focus_list_draw(&list);
        focus_flush();
        action = focus_get_action(FOCUS_CTX_NAV);
        if (focus_list_input(&list, action))
            continue;
        if (action == ACTION_STD_MENU)
            return false;
        if (action != ACTION_STD_OK)
            continue;
        switch (list.selected)
        {
        case RM_SAVE:
            radio_toggle_saved();
            return false;
        case RM_STATIONS:
            radio_stations();
            return false;
        case RM_SCAN:
            radio_autoscan();
            return false;
        case RM_MODE:
            if (radio_get_mode() == RADIO_PRESET_MODE)
                radio_set_mode(RADIO_SCAN_MODE);
            else if (radio_preset_count() > 0)
                radio_set_mode(RADIO_PRESET_MODE);
            else
                focus_toast("No saved stations");
            break;
        case RM_SOUND:
            global_settings.fm_force_mono = !global_settings.fm_force_mono;
            toggle_mono_mode(global_settings.fm_force_mono);
            settings_save();
            break;
        case RM_OFF:
            return true;
        }
    }
}

/* ----------------------------------------------------------- screen ---- */

void focus_radio(void)
{
    const struct fm_region_data *fmr = region();
    int band_steps = (fmr->freq_max - fmr->freq_min) / fmr->freq_step + 1;
    int generation = focus_storage_generation;
    int seek_dir = 0, seek_steps = 0;
    bool keep_playing = true, done = false;
#ifndef HAVE_NOISY_IDLE_MODE
    long button_timeout = current_tick + 2 * HZ;
#endif

    if (!radio_hardware_present())
    {
        focus_message("FM Radio", "No tuner found", NULL);
        sleep(HZ * 2);
        return;
    }
    focus_message("FM Radio", "Starting...", "Earphones are the aerial");

    focus_radio_set_in_screen(true);
    if (radio_preset_count() <= 0)
        focus_presets_load();
    if (get_radio_status() == FMRADIO_OFF)
        audio_stop();
    /* This should be done before touching audio settings */
    while (!pcm_is_initialized())
        sleep(0);
    audio_set_input_source(AUDIO_SRC_FMRADIO,
                           get_radio_status() == FMRADIO_PAUSED
                           ? SRCF_FMRADIO_PAUSED : SRCF_FMRADIO_PLAYING);

    preset_set_current(preset_find(radio_get_current_frequency()));
    radio_set_mode(radio_current_preset() >= 0 ? RADIO_PRESET_MODE
                                               : RADIO_SCAN_MODE);
    name_since = current_tick;
#ifndef HAVE_NOISY_IDLE_MODE
    cpu_idle_mode(true);
#endif

    while (!done)
    {
        int action;

        if (seek_dir)
        {
            int freq = step_freq(radio_get_current_frequency(), seek_dir);
            radio_set_current_frequency(freq);
            if (tuner_set(RADIO_SCAN_FREQUENCY, freq) ||
                ++seek_steps >= band_steps)
            {
                seek_dir = 0;
                preset_set_current(preset_find(freq));
                if (radio_current_preset() >= 0)
                    radio_set_mode(RADIO_PRESET_MODE);
                remember_frequency();
                if (get_radio_status() == FMRADIO_PLAYING)
                    tuner_set(RADIO_MUTE, 0);
                name_since = current_tick;
            }
        }
        radio_draw(seek_dir != 0);

        if (seek_dir)
            action = focus_get_action_timeout(FOCUS_CTX_RADIO, TIMEOUT_NOBLOCK);
        else
            action = focus_get_action_timeout(FOCUS_CTX_RADIO, HZ / 5);
#ifndef HAVE_NOISY_IDLE_MODE
        if (action != ACTION_NONE)
        {
            cpu_idle_mode(false);
            button_timeout = current_tick + 2 * HZ;
        }
#endif
        if (generation != focus_storage_generation ||
            !tuner_get(RADIO_PRESENT))
        {
            keep_playing = false;
            break;
        }
        switch (action)
        {
        case ACTION_STD_PREV:
        case ACTION_STD_NEXT:
            if (seek_dir)
            {
                /* a tap stops a seek where it is */
                seek_dir = 0;
                radio_set_mode(RADIO_SCAN_MODE);
                next_station(0);
            }
            else
                next_station(action == ACTION_STD_PREV ? -1 : 1);
            name_since = current_tick;
            break;
        case ACTION_STD_PREVREPEAT:
        case ACTION_STD_NEXTREPEAT:
            if (!seek_dir)
            {
                seek_dir = action == ACTION_STD_PREVREPEAT ? -1 : 1;
                seek_steps = 0;
                radio_set_mode(RADIO_SCAN_MODE);
                preset_set_current(-1);
                tuner_set(RADIO_MUTE, 1);
            }
            break;
        case ACTION_FM_PLAY:
            if (get_radio_status() == FMRADIO_PLAYING)
                radio_pause();
            else
                radio_start();
            break;
        case ACTION_FM_MODE:
            if (radio_get_mode() == RADIO_PRESET_MODE)
            {
                radio_set_mode(RADIO_SCAN_MODE);
                focus_toast("Tune by hand");
            }
            else if (radio_preset_count() > 0)
            {
                radio_set_mode(RADIO_PRESET_MODE);
                focus_toast("Saved stations");
            }
            else
                focus_toast("No saved stations");
            break;
        case ACTION_FM_PRESET:
        case ACTION_FM_MENU:
            if (seek_dir)
            {
                seek_dir = 0;
                next_station(0);
            }
            if (action == ACTION_FM_MENU)
                radio_toggle_saved();
            else if (radio_menu())
            {
                keep_playing = false;
                done = true;
            }
            break;
        case ACTION_FM_EXIT:
            done = true;
            break;
        default:
            break;
        }
#ifndef HAVE_NOISY_IDLE_MODE
        if (TIME_AFTER(current_tick, button_timeout))
        {
            cpu_idle_mode(true);
        }
#endif
    }

    if (seek_dir && get_radio_status() == FMRADIO_PLAYING)
        tuner_set(RADIO_MUTE, 0);
#ifndef SIMULATOR
    sound_settings_apply();
#endif
    if (!keep_playing)
        audio_set_input_source(AUDIO_SRC_PLAYBACK, SRCF_PLAYBACK);
#ifndef HAVE_NOISY_IDLE_MODE
    cpu_idle_mode(false);
#endif
    focus_radio_set_in_screen(false);
}
#endif /* CONFIG_TUNER */
