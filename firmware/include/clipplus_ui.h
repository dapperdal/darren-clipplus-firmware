/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef CLIPPLUS_UI_H
#define CLIPPLUS_UI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CLIPPLUS_LCD_WIDTH 128u
#define CLIPPLUS_LCD_HEIGHT 64u

typedef enum {
    CLIPPLUS_BUTTON_NONE = 0,
    CLIPPLUS_BUTTON_UP,
    CLIPPLUS_BUTTON_DOWN,
    CLIPPLUS_BUTTON_LEFT,
    CLIPPLUS_BUTTON_RIGHT,
    CLIPPLUS_BUTTON_HOME,
    CLIPPLUS_BUTTON_SELECT,
    CLIPPLUS_BUTTON_POWER,
    CLIPPLUS_BUTTON_PLAY,
    CLIPPLUS_BUTTON_VOL_UP,
    CLIPPLUS_BUTTON_VOL_DOWN
} clipplus_button_t;

typedef enum {
    CLIPPLUS_SCREEN_HOME = 0,
    CLIPPLUS_SCREEN_MUSIC,
    CLIPPLUS_SCREEN_RADIO,
    CLIPPLUS_SCREEN_RECORDINGS,
    CLIPPLUS_SCREEN_SETTINGS,
    CLIPPLUS_SCREEN_ALARM,
    CLIPPLUS_SCREEN_PODCASTS,
    CLIPPLUS_SCREEN_NOW_PLAYING
} clipplus_screen_t;

typedef struct {
    clipplus_screen_t screen;
    uint8_t home_index;
    uint8_t volume;
    bool playing;
    bool storage_ready;
    uint8_t animation_frame;
    uint32_t elapsed_seconds;
    uint16_t volume_overlay_ms;
    bool screen_locked;
    uint16_t lock_overlay_ms;
    int battery_minutes;
    int battery_percent;
    bool battery_charging;
} clipplus_ui_state_t;

void clipplus_ui_init(clipplus_ui_state_t *state);
void clipplus_ui_tick(clipplus_ui_state_t *state, uint32_t elapsed_ms);
void clipplus_ui_input(clipplus_ui_state_t *state, clipplus_button_t button,
                       bool long_press);
void clipplus_ui_render(const clipplus_ui_state_t *state,
                        uint8_t framebuffer[CLIPPLUS_LCD_HEIGHT][CLIPPLUS_LCD_WIDTH]);
void clipplus_ui_render_list(const clipplus_ui_state_t *state,
                             uint8_t framebuffer[CLIPPLUS_LCD_HEIGHT][CLIPPLUS_LCD_WIDTH]);
void clipplus_ui_render_single(const clipplus_ui_state_t *state,
                               uint8_t framebuffer[CLIPPLUS_LCD_HEIGHT][CLIPPLUS_LCD_WIDTH]);
void clipplus_ui_render_stock_inspired(const clipplus_ui_state_t *state,
                                       uint8_t framebuffer[CLIPPLUS_LCD_HEIGHT][CLIPPLUS_LCD_WIDTH]);
void clipplus_ui_render_media(const clipplus_ui_state_t *state,
                              uint8_t framebuffer[CLIPPLUS_LCD_HEIGHT][CLIPPLUS_LCD_WIDTH],
                              bool podcasts, bool playback_error,
                              unsigned selected, unsigned total);
void clipplus_ui_render_now_playing(const clipplus_ui_state_t *state,
                                    uint8_t framebuffer[CLIPPLUS_LCD_HEIGHT][CLIPPLUS_LCD_WIDTH]);
void clipplus_ui_render_volume(const clipplus_ui_state_t *state,
                               uint8_t framebuffer[CLIPPLUS_LCD_HEIGHT][CLIPPLUS_LCD_WIDTH]);

#endif
