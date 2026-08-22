/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "clipplus_ui.h"
#include "clipplus_memory.h"

static const char *const home_labels[] = {
    "MUSIC", "RADIO", "RECORD", "EQUALISER", "ALARM", "PODCASTS", "NOW PLAYING"
};

static const clipplus_screen_t home_screens[] = {
    CLIPPLUS_SCREEN_MUSIC, CLIPPLUS_SCREEN_RADIO,
    CLIPPLUS_SCREEN_RECORDINGS, CLIPPLUS_SCREEN_SETTINGS,
    CLIPPLUS_SCREEN_ALARM,
    CLIPPLUS_SCREEN_PODCASTS,
    CLIPPLUS_SCREEN_NOW_PLAYING
};

static char *append_uint(char *dst, unsigned value)
{
    char reverse[10];
    unsigned count = 0;
    if (!value) { *dst++ = '0'; return dst; }
    while (value && count < sizeof(reverse)) {
        reverse[count++] = (char)('0' + value % 10u);
        value /= 10u;
    }
    while (count) *dst++ = reverse[--count];
    return dst;
}

static void pixel(uint8_t fb[CLIPPLUS_LCD_HEIGHT][CLIPPLUS_LCD_WIDTH], int x, int y,
                  bool on)
{
    if (x >= 0 && x < (int)CLIPPLUS_LCD_WIDTH && y >= 0 && y < (int)CLIPPLUS_LCD_HEIGHT)
        fb[y][x] = (uint8_t)on;
}

static void rect(uint8_t fb[CLIPPLUS_LCD_HEIGHT][CLIPPLUS_LCD_WIDTH], int x, int y,
                 int w, int h, bool fill)
{
    for (int yy = y; yy < y + h; ++yy)
        for (int xx = x; xx < x + w; ++xx)
            if (fill || yy == y || yy == y + h - 1 || xx == x || xx == x + w - 1)
                pixel(fb, xx, yy, true);
}

static const uint8_t glyphs[26][7] = {
    {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
    {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
    {14,17,16,23,17,17,14},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},
    {7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
    {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
    {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
    {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
    {17,17,17,17,17,10,4},{17,17,17,21,21,27,17},{17,17,10,4,10,17,17},
    {17,17,10,4,4,4,4},{31,1,2,4,8,16,31}
};

static void text(uint8_t fb[CLIPPLUS_LCD_HEIGHT][CLIPPLUS_LCD_WIDTH], int x, int y,
                 const char *s, int scale)
{
    for (; *s; ++s, x += 6 * scale) {
        unsigned index;
        if (*s >= 'A' && *s <= 'Z') index = (unsigned)(*s - 'A');
        else if (*s >= '0' && *s <= '9') index = 26u + (unsigned)(*s - '0');
        else continue;
        const uint8_t *g = glyphs[index];
        for (int row = 0; row < 7; ++row)
            for (int col = 0; col < 5; ++col)
                if (g[row] & (1u << (4 - col)))
                    for (int dy = 0; dy < scale; ++dy)
                        for (int dx = 0; dx < scale; ++dx)
                            pixel(fb, x + col * scale + dx, y + row * scale + dy, true);
    }
}

static bool render_lock_notice(const clipplus_ui_state_t *state,
                               uint8_t framebuffer[CLIPPLUS_LCD_HEIGHT][CLIPPLUS_LCD_WIDTH])
{
    if (!state->screen_locked || !state->lock_overlay_ms)
        return false;
    text(framebuffer, 5, 3, "DARREN", 1);
    rect(framebuffer, 4, 12, 120, 1, true);
    text(framebuffer, 28, 21, "LOCKED", 2);
    text(framebuffer, 37, 43, "HOLD HOME", 1);
    return true;
}

static void render_battery(const clipplus_ui_state_t *state,
                            uint8_t framebuffer[CLIPPLUS_LCD_HEIGHT][CLIPPLUS_LCD_WIDTH])
{
    char label[8] = "BAT--";
    int x = 98;
    if (state->battery_charging) {
        label[0] = 'C'; label[1] = 'H'; label[2] = 'G'; label[3] = '\0';
        x = 106;
    } else if (state->battery_minutes >= 0) {
        unsigned hours = (unsigned)state->battery_minutes / 60u;
        unsigned minutes = (unsigned)state->battery_minutes % 60u;
        char *p = label;
        p = append_uint(p, hours);
        *p++ = 'H';
        *p++ = (char)('0' + minutes / 10u);
        *p++ = (char)('0' + minutes % 10u);
        *p = '\0';
        x = 92;
    }
    text(framebuffer, x, 3, label, 1);
}

static void icon(uint8_t fb[CLIPPLUS_LCD_HEIGHT][CLIPPLUS_LCD_WIDTH], int x, int y,
                 uint8_t index, bool selected)
{
    rect(fb, x, y, 28, 25, selected);
    const bool ink = !selected;
    switch (index) {
    case 0: /* note */
        rect(fb, x + 11, y + 4, 3, 15, ink); rect(fb, x + 14, y + 4, 7, 3, ink);
        rect(fb, x + 5, y + 16, 8, 5, ink); rect(fb, x + 14, y + 13, 8, 5, ink); break;
    case 1: /* radio */
        rect(fb, x + 6, y + 9, 16, 12, ink); rect(fb, x + 11, y + 12, 6, 6, ink);
        rect(fb, x + 19, y + 5, 2, 5, ink); break;
    case 2: /* microphone */
        rect(fb, x + 11, y + 4, 6, 12, ink); rect(fb, x + 7, y + 11, 14, 3, ink);
        rect(fb, x + 13, y + 16, 2, 5, ink); rect(fb, x + 9, y + 20, 10, 2, ink); break;
    case 3: /* gear */
        rect(fb, x + 8, y + 8, 12, 12, ink); rect(fb, x + 12, y + 12, 4, 4, !ink); break;
    default: /* play */
        for (int row = 0; row < 15; ++row) {
            int width = row < 7 ? row + 1 : 14 - row;
            for (int col = 0; col < width; ++col) pixel(fb, x + 8 + col, y + 5 + row, ink);
        }
        break;
    }
}

void clipplus_ui_init(clipplus_ui_state_t *state)
{
    clipplus_memset(state, 0, sizeof(*state));
    state->screen = CLIPPLUS_SCREEN_HOME;
    state->volume = 60;
    state->storage_ready = true;
    state->battery_minutes = -1;
    state->battery_percent = -1;
}

void clipplus_ui_tick(clipplus_ui_state_t *state, uint32_t elapsed_ms)
{
    state->animation_frame = (uint8_t)((state->animation_frame + elapsed_ms / 120u) % 8u);
    if (state->playing) state->elapsed_seconds += elapsed_ms / 1000u;
    state->volume_overlay_ms = state->volume_overlay_ms > elapsed_ms ?
                               (uint16_t)(state->volume_overlay_ms - elapsed_ms) : 0;
    state->lock_overlay_ms = state->lock_overlay_ms > elapsed_ms ?
                             (uint16_t)(state->lock_overlay_ms - elapsed_ms) : 0;
}

void clipplus_ui_input(clipplus_ui_state_t *state, clipplus_button_t button,
                       bool long_press)
{
    if (state->screen_locked) {
        if (long_press && button == CLIPPLUS_BUTTON_HOME) {
            state->screen_locked = false;
            state->lock_overlay_ms = 0;
        }
        return;
    }
    if (long_press && button == CLIPPLUS_BUTTON_SELECT) {
        state->screen = CLIPPLUS_SCREEN_NOW_PLAYING;
        return;
    }
    if (long_press && button == CLIPPLUS_BUTTON_HOME) {
        state->screen_locked = true;
        state->lock_overlay_ms = 2000;
        return;
    }
    if (button == CLIPPLUS_BUTTON_HOME || button == CLIPPLUS_BUTTON_POWER) {
        state->screen = CLIPPLUS_SCREEN_HOME;
    } else if (state->screen == CLIPPLUS_SCREEN_HOME && button == CLIPPLUS_BUTTON_LEFT) {
        state->home_index = (uint8_t)((state->home_index + 6u) % 7u);
    } else if (state->screen == CLIPPLUS_SCREEN_HOME && button == CLIPPLUS_BUTTON_RIGHT) {
        state->home_index = (uint8_t)((state->home_index + 1u) % 7u);
    } else if (state->screen == CLIPPLUS_SCREEN_HOME && button == CLIPPLUS_BUTTON_UP) {
        state->home_index = (uint8_t)((state->home_index + 6u) % 7u);
    } else if (state->screen == CLIPPLUS_SCREEN_HOME && button == CLIPPLUS_BUTTON_DOWN) {
        state->home_index = (uint8_t)((state->home_index + 1u) % 7u);
    } else if (state->screen == CLIPPLUS_SCREEN_HOME && button == CLIPPLUS_BUTTON_SELECT) {
        state->screen = home_screens[state->home_index];
    } else if (state->screen == CLIPPLUS_SCREEN_NOW_PLAYING &&
               button == CLIPPLUS_BUTTON_SELECT) {
        state->playing = !state->playing;
    } else if (button == CLIPPLUS_BUTTON_VOL_UP) {
        if (state->volume < 100u) state->volume++;
        state->volume_overlay_ms = 2000;
    } else if (button == CLIPPLUS_BUTTON_VOL_DOWN) {
        if (state->volume > 0u) state->volume--;
        state->volume_overlay_ms = 2000;
    } else if (button == CLIPPLUS_BUTTON_PLAY) {
        state->playing = !state->playing;
    } else if (button == CLIPPLUS_BUTTON_LEFT) {
        state->elapsed_seconds = state->elapsed_seconds > 5 ? state->elapsed_seconds - 5 : 0;
    } else if (button == CLIPPLUS_BUTTON_RIGHT) {
        state->elapsed_seconds += 5;
    }
}

void clipplus_ui_render(const clipplus_ui_state_t *state,
                        uint8_t framebuffer[CLIPPLUS_LCD_HEIGHT][CLIPPLUS_LCD_WIDTH])
{
    clipplus_memset(framebuffer, 0, CLIPPLUS_LCD_HEIGHT * CLIPPLUS_LCD_WIDTH);
    if (render_lock_notice(state, framebuffer)) return;
    render_battery(state, framebuffer);
    if (state->screen == CLIPPLUS_SCREEN_HOME) {
        text(framebuffer, 4, 3, "CLIPPLUS", 1);
        const int center = 64;
        for (int i = -1; i <= 1; ++i) {
            int item = (int)state->home_index + i;
            while (item < 0) item += 6;
            while (item >= 6) item -= 6;
            int x = center + i * 34;
            icon(framebuffer, x - 14, 15, (uint8_t)item, i == 0);
        }
        size_t label_width = clipplus_strlen(home_labels[state->home_index]) * 6u;
        int label_x = (int)((CLIPPLUS_LCD_WIDTH - label_width) / 2u);
        text(framebuffer, label_x, 43, home_labels[state->home_index], 1);
        for (int i = 0; i < 7; ++i) pixel(framebuffer, 51 + i * 4, 53, i == state->home_index);
        /* Keep the idle animation in the content area; the home screen has no
         * cramped playback/status strip. Playback remains on the Play key and
         * in the Now Playing screen. */
        pixel(framebuffer, 77 + state->animation_frame, 15, true);
    } else {
        text(framebuffer, 4, 4, state->screen == CLIPPLUS_SCREEN_NOW_PLAYING ? "NOW PLAYING" : home_labels[state->home_index], 1);
        rect(framebuffer, 4, 17, 120, 19, false);
        text(framebuffer, 10, 22, state->playing ? "PLAYING" : "PAUSED", 1);
        text(framebuffer, 10, 30, "SAMPLE TRACK", 1);
        rect(framebuffer, 0, 56, 128, 8, false);
        text(framebuffer, 4, 58, state->playing ? "PLAY" : "STOP", 1);
        for (int i = 0; i < (int)(state->volume / 10u); ++i) pixel(framebuffer, 89 + i * 3, 61, true);
        pixel(framebuffer, 77 + state->animation_frame, 57, true);
    }
}

void clipplus_ui_render_list(const clipplus_ui_state_t *state,
                             uint8_t framebuffer[CLIPPLUS_LCD_HEIGHT][CLIPPLUS_LCD_WIDTH])
{
    clipplus_memset(framebuffer, 0, CLIPPLUS_LCD_HEIGHT * CLIPPLUS_LCD_WIDTH);
    if (render_lock_notice(state, framebuffer)) return;
    render_battery(state, framebuffer);
    text(framebuffer, 4, 3, "CLIPPLUS", 1);
    for (int i = 0; i < 7; ++i) {
        int y = 10 + i * 7;
        bool selected = i == state->home_index;
        if (selected) rect(framebuffer, 4, y + 1, 2, 7, true);
        text(framebuffer, 13, y + 1, home_labels[i], 1);
    }
}

static void centered_text(uint8_t framebuffer[CLIPPLUS_LCD_HEIGHT][CLIPPLUS_LCD_WIDTH],
                          const char *label, int y, int scale)
{
    size_t width = clipplus_strlen(label) * (size_t)(6 * scale);
    int x = (int)((CLIPPLUS_LCD_WIDTH - width) / 2u);
    text(framebuffer, x, y, label, scale);
}

static void direction_arrow(uint8_t framebuffer[CLIPPLUS_LCD_HEIGHT][CLIPPLUS_LCD_WIDTH],
                            int x, int y, bool up)
{
    for (int row = 0; row < 5; ++row) {
        int width;
        int offset;
        if (up) {
            width = row < 3 ? row * 2 + 1 : 1;
            offset = row < 3 ? row : 2;
        } else {
            width = row < 3 ? (4 - row) * 2 + 1 : 1;
            offset = row < 3 ? 4 - row : 0;
        }
        int yy = y + row;
        for (int col = 0; col < width; ++col)
            pixel(framebuffer, x - offset + col, yy, true);
    }
}

void clipplus_ui_render_single(const clipplus_ui_state_t *state,
                               uint8_t framebuffer[CLIPPLUS_LCD_HEIGHT][CLIPPLUS_LCD_WIDTH])
{
    clipplus_memset(framebuffer, 0, CLIPPLUS_LCD_HEIGHT * CLIPPLUS_LCD_WIDTH);
    if (render_lock_notice(state, framebuffer)) return;
    render_battery(state, framebuffer);
    text(framebuffer, 4, 3, "CLIPPLUS", 1);
    direction_arrow(framebuffer, 64, 13, true);
    if (state->home_index == 5) {
        centered_text(framebuffer, "NOW", 21, 2);
        centered_text(framebuffer, "PLAYING", 36, 2);
    } else {
        centered_text(framebuffer, home_labels[state->home_index], 29, 2);
    }
    direction_arrow(framebuffer, 64, 49, false);
    /* Six quiet dots provide orientation without turning the view into a
     * list or adding a status/control strip. */
    for (int i = 0; i < 7; ++i)
        pixel(framebuffer, 54 + i * 4, 57, i == state->home_index);
}

void clipplus_ui_render_stock_inspired(const clipplus_ui_state_t *state,
                                       uint8_t framebuffer[CLIPPLUS_LCD_HEIGHT][CLIPPLUS_LCD_WIDTH])
{
    clipplus_memset(framebuffer, 0, CLIPPLUS_LCD_HEIGHT * CLIPPLUS_LCD_WIDTH);
    if (render_lock_notice(state, framebuffer)) return;
    render_battery(state, framebuffer);
    text(framebuffer, 5, 2, "DARREN", 1);
    rect(framebuffer, 4, 12, 120, 1, true);

    /* A quiet, stock-like framed selection with the agreed single-item flow. */
    rect(framebuffer, 17, 20, 94, 25, false);
    direction_arrow(framebuffer, 64, 14, true);
    direction_arrow(framebuffer, 64, 53, false);
    if (state->home_index == 2) {
        centered_text(framebuffer, "RECORD", 25, 2);
        centered_text(framebuffer, "INGS", 35, 2);
    } else if (state->home_index == 5) {
        centered_text(framebuffer, "NOW", 25, 2);
        centered_text(framebuffer, "PLAYING", 35, 2);
    } else {
        centered_text(framebuffer, home_labels[state->home_index], 29, 2);
    }
    for (int i = 0; i < 7; ++i)
        rect(framebuffer, 49 + i * 5, 53, 3, 3, i == state->home_index);
    pixel(framebuffer, 112 + state->animation_frame, 6, true);
}

void clipplus_ui_render_media(const clipplus_ui_state_t *state,
                              uint8_t framebuffer[CLIPPLUS_LCD_HEIGHT][CLIPPLUS_LCD_WIDTH],
                              bool podcasts, bool playback_error,
                              unsigned selected, unsigned total)
{
    clipplus_memset(framebuffer, 0, CLIPPLUS_LCD_HEIGHT * CLIPPLUS_LCD_WIDTH);
    if (render_lock_notice(state, framebuffer)) return;
    render_battery(state, framebuffer);
    text(framebuffer, 5, 2, podcasts ? "PODCASTS" : "MUSIC", 1);
    rect(framebuffer, 4, 12, 120, 1, true);
    direction_arrow(framebuffer, 9, 30, true);
    direction_arrow(framebuffer, 119, 30, false);
    if (playback_error) {
        centered_text(framebuffer, "PLAYBACK", 23, 2);
        centered_text(framebuffer, "ERROR", 37, 2);
    } else if (!state->storage_ready) {
        centered_text(framebuffer, "STORAGE", 23, 2);
        centered_text(framebuffer, "ERROR", 37, 2);
    } else {
        centered_text(framebuffer, podcasts ? "EPISODE" : "TRACK", 29, 2);
    }
    char position[16];
    char *position_end = append_uint(position, total ? selected + 1u : 0u);
    *position_end++ = ' ';
    *position_end++ = 'O';
    *position_end++ = 'F';
    *position_end++ = ' ';
    position_end = append_uint(position_end, total);
    *position_end = '\0';
    text(framebuffer, 4, 55, position, 1);
}

void clipplus_ui_render_now_playing(const clipplus_ui_state_t *state,
                                    uint8_t framebuffer[CLIPPLUS_LCD_HEIGHT][CLIPPLUS_LCD_WIDTH])
{
    clipplus_memset(framebuffer, 0, CLIPPLUS_LCD_HEIGHT * CLIPPLUS_LCD_WIDTH);
    if (render_lock_notice(state, framebuffer)) return;
    render_battery(state, framebuffer);
    text(framebuffer, 5, 2, "NOW PLAYING", 1);
    rect(framebuffer, 4, 12, 120, 1, true);
    centered_text(framebuffer, state->playing ? "PLAYING" : "PAUSED", 22, 2);
    centered_text(framebuffer, "SAMPLE", 38, 2);
    pixel(framebuffer, 14, 57, true);
    pixel(framebuffer, 114, 57, true);
}

void clipplus_ui_render_volume(const clipplus_ui_state_t *state,
                               uint8_t framebuffer[CLIPPLUS_LCD_HEIGHT][CLIPPLUS_LCD_WIDTH])
{
    char value[4];
    clipplus_memset(framebuffer, 0, CLIPPLUS_LCD_HEIGHT * CLIPPLUS_LCD_WIDTH);
    if (render_lock_notice(state, framebuffer)) return;
    render_battery(state, framebuffer);
    text(framebuffer, 5, 2, "DARREN", 1);
    rect(framebuffer, 4, 12, 120, 1, true);
    centered_text(framebuffer, "VOLUME", 17, 2);
    value[0] = (char)('0' + (state->volume / 100u) % 10u);
    value[1] = (char)('0' + (state->volume / 10u) % 10u);
    value[2] = (char)('0' + state->volume % 10u);
    value[3] = '\0';
    centered_text(framebuffer, value, 32, 2);
    for (int i = 0; i < 10; ++i) {
        int height = 4 + i * 4;
        int x = 7 + i * 12;
        if ((unsigned)(i + 1) * 10u <= state->volume)
            rect(framebuffer, x, 57 - height, 8, height, true);
        else
            rect(framebuffer, x, 57 - height, 8, height, false);
    }
}
