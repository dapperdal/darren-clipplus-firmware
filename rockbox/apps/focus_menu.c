/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Focus-player first screen for the Sansa Clip+.
 *
 * This file is project-original fork code. It deliberately uses Rockbox's
 * existing action and LCD APIs; target button/display drivers stay unchanged.
 */
#include "config.h"
#include "action.h"
#include "font.h"
#include "lcd.h"
#include "screens.h"
#include "splash.h"
#include "kernel.h"
#include "misc.h"
#include "focus_font.h"

void focus_eq(void);
void focus_open_music(void);
void focus_open_podcasts(void);
void focus_now_playing(void);
void focus_open_alarm(void);
#if CONFIG_TUNER
void focus_open_radio(void);
#endif
#ifdef HAVE_RECORDING
void focus_open_recordings(void);
#endif

static const char * const focus_items[] = {
    "MUSIC", "RADIO", "RECORD\nINGS", "EQUALISER", "ALARM", "PODCASTS", "NOW\nPLAYING"
};

#define FOCUS_ITEM_COUNT ((int)(sizeof(focus_items) / sizeof(focus_items[0])))

static void focus_draw(int selected)
{
    const char *item = focus_items[selected];

    focus_screen_header(NULL);
    focus_screen_battery(82, 1);
    /* The item is rendered with a project-original 2x bitmap font. */
    focus_big_text_centered(item, 20);
    focus_screen_navigation_cues();
    lcd_update();
}

/* The destination handoff is intentionally a small second patch: this first
 * shell proves the new boot-facing UX without changing proven drivers. */
void focus_menu(void) NORETURN_ATTR;
void focus_menu(void)
{
    int selected = 0;
    int action;

    focus_draw(selected);
    while (true)
    {
        action = get_action(CONTEXT_STD | ALLOW_SOFTLOCK, HZ / 4);
        switch (action)
        {
            case ACTION_STD_PREV:
                selected = (selected + FOCUS_ITEM_COUNT - 1) % FOCUS_ITEM_COUNT;
                focus_draw(selected);
                break;
            case ACTION_STD_NEXT:
                selected = (selected + 1) % FOCUS_ITEM_COUNT;
                focus_draw(selected);
                break;
            case ACTION_STD_OK:
                switch (selected)
                {
                case 0:
                    focus_open_music();
                    break;
                case 1:
#if CONFIG_TUNER
                    focus_open_radio();
#else
                    splash(HZ / 2, "Radio unavailable");
#endif
                    break;
                case 2:
#ifdef HAVE_RECORDING
                    focus_open_recordings();
#else
                    splash(HZ / 2, "Recording unavailable");
#endif
                    break;
                case 3:
                    focus_eq();
                    break;
                case 4:
                    focus_open_alarm();
                    break;
                case 5:
                    focus_open_podcasts();
                    break;
                case 6:
                    focus_now_playing();
                    break;
                }
                focus_draw(selected);
                break;
            case ACTION_STD_CANCEL:
                /* Power remains an unconsumed standard action; the button
                 * driver owns long-hold shutdown and system events. */
                default_event_handler(action);
                focus_draw(selected);
                break;
            case ACTION_STD_MENU:
                /* Home is back at the top level. */
                focus_draw(selected);
                break;
            default:
                default_event_handler(action);
                break;
            case ACTION_STD_CONTEXT:
                splash(HZ / 2, "Now Playing shortcut");
                focus_draw(selected);
                break;
        }
    }
}
