/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Five-band equaliser for the Clip+ focus shell. Project-original fork code
 * driving Rockbox's existing DSP equaliser; no filter maths lives here. */
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <string-extra.h>
#include "action.h"
#include "kernel.h"
#include "lcd.h"
#include "settings.h"
#include "sound.h"
#include "eq.h"
#include "dsp_proc_settings.h"
#include "focus_ui.h"

#define BANDS 5
#define GAIN_MAX 120 /* tenths of a dB */
#define GAIN_STEP 10

/* The five visible bands map onto five of Rockbox's ten filters. */
static const struct { unsigned char band; short cutoff; unsigned char q;
                      const char *label; } bands[BANDS] = {
    { 0, 80, 7, "80" }, { 3, 250, 10, "250" }, { 5, 1000, 10, "1k" },
    { 7, 4000, 10, "4k" }, { 9, 12000, 7, "12k" },
};

static const struct { const char *name; signed char gain[BANDS]; } presets[] = {
    { "Flat",   { 0, 0, 0, 0, 0 } },
    { "Bass",   { 6, 3, 0, 0, 0 } },
    { "Punchy", { 5, 2, -1, 2, 4 } },
    { "Vocal",  { -2, 0, 3, 3, 0 } },
    { "Bright", { 0, 0, 0, 3, 5 } },
};
#define PRESETS ((int)(sizeof(presets) / sizeof(presets[0])))

static int band_gain(int i)
{
    return global_settings.eq_band_settings[bands[i].band].gain;
}

static int current_preset(void)
{
    int p, i;
    for (p = 0; p < PRESETS; p++)
    {
        for (i = 0; i < BANDS; i++)
            if (band_gain(i) != presets[p].gain[i] * 10)
                break;
        if (i == BANDS)
            return p;
    }
    return -1;
}

static void eq_apply(void)
{
    int i, b, boost = 0;
    bool active = false;

    for (b = 0; b < EQ_NUM_BANDS; b++)
    {
        struct eq_band_setting *s = &global_settings.eq_band_settings[b];
        for (i = 0; i < BANDS; i++)
            if (bands[i].band == b)
                break;
        if (i < BANDS)
        {
            s->cutoff = bands[i].cutoff;
            s->q = bands[i].q;
            if (s->gain > GAIN_MAX) s->gain = GAIN_MAX;
            if (s->gain < -GAIN_MAX) s->gain = -GAIN_MAX;
            if (s->gain > boost) boost = s->gain;
            if (s->gain) active = true;
        }
        else
            s->gain = 0;
        dsp_set_eq_coefs(b, s);
    }
    /* pull the level down by the largest boost so nothing clips */
    global_settings.eq_precut = boost;
    dsp_set_eq_precut(boost);
    global_settings.eq_enabled = active;
    dsp_eq_enable(active);
}

static void eq_draw(int selected)
{
    char title[24], value[16];
    int preset = current_preset();
    int i;

    snprintf(title, sizeof(title), "EQ  %s",
             preset >= 0 ? presets[preset].name : "Custom");
    focus_begin(title);
    for (i = 0; i < BANDS; i++)
    {
        int cx = 13 + i * 25;
        int gain = band_gain(i);
        int y = 34 - gain * 14 / GAIN_MAX;
        const char *label = bands[i].label;
        int w;

        lcd_vline(cx, 19, 49);
        lcd_hline(cx - 2, cx + 2, 34);
        lcd_fillrect(cx - 5, y - 2, 11, 5);
        if (i == selected)
        {
            snprintf(value, sizeof(value), gain ? "%+d" : "%d", gain / 10);
            label = value;
        }
        w = focus_text_width(&focus_font_small, label);
        focus_text(&focus_font_small, cx - w / 2 + 1, 53, label);
        if (i == selected)
            focus_invert_round(cx - 11, 52, 23, 12);
    }
    focus_flush();
}

void focus_eq(void)
{
    int selected = 0;

    while (true)
    {
        int action, gain, preset;

        eq_draw(selected);
        action = focus_get_action(FOCUS_CTX_EQ);
        gain = band_gain(selected);
        switch (action)
        {
        case ACTION_SETTINGS_INC:
        case ACTION_SETTINGS_INCREPEAT:
            if (gain < GAIN_MAX) gain += GAIN_STEP;
            break;
        case ACTION_SETTINGS_DEC:
        case ACTION_SETTINGS_DECREPEAT:
            if (gain > -GAIN_MAX) gain -= GAIN_STEP;
            break;
        case ACTION_STD_PREV:
            selected = (selected + BANDS - 1) % BANDS;
            continue;
        case ACTION_STD_NEXT:
            selected = (selected + 1) % BANDS;
            continue;
        case ACTION_STD_OK:
        {
            int i;
            preset = (current_preset() + 1) % PRESETS;
            for (i = 0; i < BANDS; i++)
                global_settings.eq_band_settings[bands[i].band].gain =
                    presets[preset].gain[i] * 10;
            eq_apply();
            continue;
        }
        case ACTION_STD_MENU:
            settings_save();
            return;
        default:
            continue;
        }
        if (gain != band_gain(selected))
        {
            global_settings.eq_band_settings[bands[selected].band].gain = gain;
            eq_apply();
        }
    }
}
