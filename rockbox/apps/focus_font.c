/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Project-original 2x 5x7 bitmap lettering for the 128x64 focus screens. */
#include "config.h"
#include <stdio.h>
#include "font.h"
#include "lcd.h"
#include "kernel.h"
#include "power.h"
#include "powermgmt.h"

#define FOCUS_SCALE 2
#define FOCUS_GLYPH_W 5
#define FOCUS_GLYPH_H 7
#define FOCUS_CELL_W 12

static const unsigned char glyphs[37][7] = {
    {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30},
    {14,17,16,16,16,17,14}, {30,17,17,17,17,17,30},
    {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16},
    {14,17,16,23,17,17,14}, {17,17,17,31,17,17,17},
    {31,4,4,4,4,4,31}, {7,2,2,2,18,18,12},
    {17,18,20,24,20,18,17}, {16,16,16,16,16,16,31},
    {17,27,21,21,17,17,17}, {17,25,21,19,17,17,17},
    {14,17,17,17,17,17,14}, {30,17,17,30,16,16,16},
    {14,17,17,17,21,18,13}, {30,17,17,30,20,18,17},
    {15,16,16,14,1,1,30}, {31,4,4,4,4,4,4},
    {17,17,17,17,17,17,14}, {17,17,17,17,17,10,4},
    {17,17,17,21,21,27,17}, {17,17,10,4,10,17,17},
    {17,17,10,4,4,4,4}, {31,1,2,4,8,16,31},
    {14,17,19,21,25,17,14}, {4,12,4,4,4,4,14},
    {14,17,1,2,4,8,31}, {30,1,1,14,1,1,30},
    {2,6,10,18,31,2,2}, {31,16,16,30,1,1,30},
    {14,16,16,30,17,17,14}, {31,1,2,4,8,8,8},
    {14,17,17,14,17,17,14}, {14,17,17,15,1,1,14},
    {0,0,0,0,0,0,0}
};

static const unsigned char *focus_glyph(char c)
{
    if (c >= 'A' && c <= 'Z')
        return glyphs[c - 'A'];
    if (c >= '0' && c <= '9')
        return glyphs[26 + c - '0'];
    return glyphs[36];
}

int focus_big_text_width(const char *text)
{
    int width = 0;
    int line = 0;
    for (; *text; text++)
    {
        if (*text == '\n')
        {
            if (line > width)
                width = line;
            line = 0;
        }
        else
            line += FOCUS_CELL_W;
    }
    return line > width ? line : width;
}

void focus_big_text_centered(const char *text, int y)
{
    int x = (LCD_WIDTH - focus_big_text_width(text)) / 2;
    int line_x = x;

    lcd_set_drawmode(DRMODE_SOLID);
    for (; *text; text++)
    {
        int row, col;
        if (*text == '\n')
        {
            y += FOCUS_GLYPH_H * FOCUS_SCALE + 2;
            line_x = x;
            continue;
        }
        const unsigned char *glyph = focus_glyph(*text);
        for (row = 0; row < FOCUS_GLYPH_H; row++)
            for (col = 0; col < FOCUS_GLYPH_W; col++)
                if (glyph[row] & (1 << (FOCUS_GLYPH_W - 1 - col)))
                    lcd_fillrect(line_x + col * FOCUS_SCALE,
                                 y + row * FOCUS_SCALE,
                                 FOCUS_SCALE, FOCUS_SCALE);
        line_x += FOCUS_CELL_W;
    }
}

void focus_screen_header(const char *section)
{
    int width = 0;
    lcd_clear_display();
    lcd_setfont(FONT_SYSFIXED);
    lcd_putsxy(4, 1, "DARREN");
    if (section)
    {
        lcd_getstringsize((const unsigned char *)section, &width, NULL);
        lcd_putsxy(LCD_WIDTH - width - 4, 1, section);
    }
    lcd_hline(4, LCD_WIDTH - 5, 12);
}

void focus_screen_battery(int x, int y)
{
    static long refresh_tick;
    static int cached_minutes = -1;
    static bool cached_charging;
    char text[16];

    /* Power readings are intentionally refreshed at most twice per minute;
     * the estimate is guidance, not a precision timer. */
    if (cached_minutes < 0 || current_tick >= refresh_tick) {
        cached_minutes = battery_time();
        cached_charging = charging_state();
        refresh_tick = current_tick + HZ * 30;
    }
    if (cached_charging)
        snprintf(text, sizeof(text), "CHG");
    else if (cached_minutes >= 0)
        snprintf(text, sizeof(text), "BAT %dH%02d", cached_minutes / 60,
                 cached_minutes % 60);
    else
        snprintf(text, sizeof(text), "BAT --");
    lcd_setfont(FONT_SYSFIXED);
    lcd_putsxy(x, y, text);
}

void focus_screen_navigation_cues(void)
{
    /* Centered chevrons make the physical Up/Down contract explicit without
     * suggesting Left/Right navigation. */
    lcd_drawpixel(64, 14);
    lcd_drawpixel(63, 15);
    lcd_drawpixel(65, 15);
    lcd_drawpixel(62, 16);
    lcd_drawpixel(66, 16);
    lcd_drawpixel(62, 55);
    lcd_drawpixel(66, 55);
    lcd_drawpixel(63, 56);
    lcd_drawpixel(65, 56);
    lcd_drawpixel(64, 57);
}

void focus_screen_position(int selected, int total)
{
    char position[16];
    int width = 0;
    if (selected < 0) selected = 0;
    if (total < 0) total = 0;
    snprintf(position, sizeof(position), "%d OF %d", total ? selected + 1 : 0, total);
    lcd_setfont(FONT_SYSFIXED);
    lcd_getstringsize((const unsigned char *)position, &width, NULL);
    lcd_putsxy(LCD_WIDTH - width - 4, 55, position);
}
