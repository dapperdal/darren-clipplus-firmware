/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Project-original compact EQ surface for the Clip+ focus shell. */
#include "config.h"
#include <stdio.h>
#include "action.h"
#include "font.h"
#include "lcd.h"
#include "kernel.h"
#include "settings.h"
#include "sound.h"
#include "eq.h"
#include "dsp_proc_settings.h"
#include "focus_font.h"
#include "misc.h"

#define FOCUS_EQ_BANDS 5
#define FOCUS_EQ_MIN (-120) /* -12.0 dB */
#define FOCUS_EQ_MAX 120    /* +12.0 dB */
#define FOCUS_EQ_STEP 10    /* 1.0 dB */

static void focus_eq_draw(int band)
{
    int i;
    char value[16];

    focus_screen_header("EQ");
    focus_big_text_centered((const char *[]) { "BASS", "LOW", "MID", "HIGH", "AIR" }[band], 15);

    snprintf(value, sizeof(value), "%d", global_settings.eq_band_settings[band].gain / 10);
    focus_big_text_centered(value, 28);

    for (i = 0; i < FOCUS_EQ_BANDS; i++)
    {
        int x = 8 + i * 23;
        int gain = global_settings.eq_band_settings[i].gain;
        int height = (gain - FOCUS_EQ_MIN) * 20 /
                     (FOCUS_EQ_MAX - FOCUS_EQ_MIN);
        if (height < 1)
            height = 1;
        if (height > 20)
            height = 20;

        lcd_vline(x + 8, 36, 54);
        if (i == band)
            lcd_fillrect(x + 3, 34, 12, 2);
        lcd_fillrect(x + 4, 54 - height, 10, height);
    }
    lcd_putsxy(2, 57, "SEL NEXT   HOME BACK");
    lcd_update();
}

void focus_eq(void)
{
    int band = 0;
    int action;

    focus_eq_draw(band);
    while (true)
    {
        action = get_action(CONTEXT_SETTINGS_EQ | ALLOW_SOFTLOCK, HZ / 4);
        switch (action)
        {
            case ACTION_STD_PREV:
                if (global_settings.eq_band_settings[band].gain < FOCUS_EQ_MAX)
                    global_settings.eq_band_settings[band].gain += FOCUS_EQ_STEP;
                dsp_set_eq_coefs(band, &global_settings.eq_band_settings[band]);
                focus_eq_draw(band);
                break;
            case ACTION_STD_NEXT:
                if (global_settings.eq_band_settings[band].gain > FOCUS_EQ_MIN)
                    global_settings.eq_band_settings[band].gain -= FOCUS_EQ_STEP;
                dsp_set_eq_coefs(band, &global_settings.eq_band_settings[band]);
                focus_eq_draw(band);
                break;
            case ACTION_STD_OK:
                band = (band + 1) % FOCUS_EQ_BANDS;
                focus_eq_draw(band);
                break;
            case ACTION_STD_CANCEL:
                default_event_handler(action);
                return;
            case ACTION_STD_MENU:
                return;
            default:
                default_event_handler(action);
                break;
        }
    }
}
