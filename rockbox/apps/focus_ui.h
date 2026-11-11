/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Shared drawing, input and list helpers for the Clip+ focus shell.
 * Project-original fork code. */
#ifndef FOCUS_UI_H
#define FOCUS_UI_H

#include "config.h"
#include <stdbool.h>
#include <stddef.h>
#include "gcc_extensions.h"
#include "action.h"
#include "focus_icons.h"

/* The Clip+ OLED is yellow for the top 16 rows and blue below. The header
 * lives in the yellow band; everything else is "body". */
#define FOCUS_HEADER_H 16
#define FOCUS_BODY_Y   16
#define FOCUS_BODY_H   48
#define FOCUS_W        128
#define FOCUS_H        64

/* Key contexts (see apps/keymaps/keymap-clip.c). */
#define FOCUS_CTX_PLAY  (CONTEXT_CUSTOM | 0x100)
#define FOCUS_CTX_NAV   (CONTEXT_CUSTOM | 0x101)
#define FOCUS_CTX_RADIO (CONTEXT_CUSTOM | 0x102)
#define FOCUS_CTX_EQ    (CONTEXT_CUSTOM | 0x103)

struct focus_font
{
    unsigned char height;
    const unsigned char *index;    /* 256 entries, 255 = no glyph */
    const unsigned char *widths;
    const unsigned short *offsets;
    const unsigned char *bits;
};

struct focus_icon
{
    unsigned char w, h;
    const unsigned char *bits;
};

extern const struct focus_font focus_font_huge;  /* 27 px, digits only */
extern const struct focus_font focus_font_title; /* 16 px bold */
extern const struct focus_font focus_font_text;  /* 12 px bold */
extern const struct focus_font focus_font_small; /* 10 px regular */
extern const struct focus_icon focus_icons[];

/* ---- text and shapes ---- */
int  focus_text_width(const struct focus_font *f, const char *s);
/* Draw at (x, y), only touching columns clip_x0 <= col < clip_x1. */
void focus_text_clip(const struct focus_font *f, int x, int y, const char *s,
                     int clip_x0, int clip_x1);
void focus_text(const struct focus_font *f, int x, int y, const char *s);
void focus_text_center(const struct focus_font *f, int y, const char *s);
void focus_text_right(const struct focus_font *f, int x_right, int y,
                      const char *s);
/* Text in a box: left aligned, scrolling back and forth when too wide.
 * `since` is the tick the text became visible. */
void focus_text_marquee(const struct focus_font *f, int x, int y, int w,
                        const char *s, long since, bool center);
void focus_icon_draw(enum focus_icon_id id, int x, int y);
void focus_round_rect(int x, int y, int w, int h, bool fill);
void focus_invert_round(int x, int y, int w, int h);
void focus_bar(int x, int y, int w, int h, long value, long max);

/* ---- frame ---- */
/* Clear the screen and draw the header (title left, status right). */
void focus_begin(const char *title);
/* The two halves of focus_begin, for screens that animate the body. */
void focus_clear(void);
void focus_header(const char *title);
/* Draw transient overlays (volume, toast) and push the frame to the LCD. */
void focus_flush(void);
void focus_toast(const char *message);
/* Simple full-body message with an optional second line. */
void focus_message(const char *title, const char *line1, const char *line2);
void focus_splash(const char *word);

/* ---- input ---- */
/* get_action() plus the things every focus screen wants: softlock notice,
 * volume keys, system events. Returns ACTION_NONE for anything consumed. */
int  focus_get_action(int context);
int  focus_get_action_timeout(int context, int timeout);
/* Bumped whenever storage may have changed (USB session). */
extern int focus_storage_generation;
bool focus_display_on(void);

/* ---- list ---- */
struct focus_list
{
    const char *title;
    int count;
    int selected;
    int top;
    long since;
    /* name is required; value and icon may be NULL. */
    const char *(*name)(int index, void *ctx, char *buf, size_t size);
    const char *(*value)(int index, void *ctx, char *buf, size_t size);
    int (*icon)(int index, void *ctx); /* focus_icon_id or -1 */
    void *ctx;
    const char *empty; /* shown when count == 0 */
};

void focus_list_init(struct focus_list *list, const char *title);
void focus_list_draw(struct focus_list *list);
/* Handles up/down; returns true when the action was used. */
bool focus_list_input(struct focus_list *list, int action);

/* ---- screens ---- */
void focus_menu(void) NORETURN_ATTR;
void focus_browse(const char *root, const char *title, bool recordings);
void focus_now_playing(void);
bool focus_playback_active(void);
void focus_radio(void);
bool focus_radio_active(void);
void focus_eq(void);
void focus_settings(void);
void focus_playback_options(void);
bool focus_playing_title(char *buf, size_t size);
bool focus_radio_title(char *buf, size_t size);
void focus_pretty_name(char *dst, size_t size, const char *filename);

#endif /* FOCUS_UI_H */
