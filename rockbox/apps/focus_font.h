/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef FOCUS_FONT_H
#define FOCUS_FONT_H

int focus_big_text_width(const char *text);
void focus_big_text_centered(const char *text, int y);
void focus_screen_header(const char *section);
void focus_screen_battery(int x, int y);
void focus_screen_navigation_cues(void);
void focus_screen_position(int selected, int total);

#endif
