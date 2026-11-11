/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Simulator-only test driver for the focus shell: replays button presses
 * from /focus_script.txt and writes LCD dumps to /shots/NAME.pgm so every
 * screen can be checked on the host. Never built for the player. */
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string-extra.h>
#include "button.h"
#include "dir.h"
#include "file.h"
#include "kernel.h"
#include "lcd.h"
#include "system.h"
#include "focus_ui.h"

void sim_do_exit(void) NORETURN_ATTR;

static char script[8192];
static int script_pos = -1, script_len, wait_frames;

static long button_code(const char *name)
{
    static const struct { const char *name; long code; } map[] = {
        { "UP", BUTTON_UP }, { "DOWN", BUTTON_DOWN }, { "LEFT", BUTTON_LEFT },
        { "RIGHT", BUTTON_RIGHT }, { "SELECT", BUTTON_SELECT },
        { "HOME", BUTTON_HOME }, { "VOLUP", BUTTON_VOL_UP },
        { "VOLDOWN", BUTTON_VOL_DOWN }, { "POWER", BUTTON_POWER },
    };
    unsigned i;
    for (i = 0; i < sizeof(map) / sizeof(map[0]); i++)
        if (!strcmp(name, map[i].name))
            return map[i].code;
    return BUTTON_NONE;
}

static void dump(const char *name)
{
    char path[64];
    unsigned char row[LCD_WIDTH];
    int fd, x, y;

    mkdir("/shots");
    snprintf(path, sizeof(path), "/shots/%s.pgm", name);
    fd = creat(path, 0666);
    if (fd < 0)
        return;
    fdprintf(fd, "P5\n%d %d\n255\n", LCD_WIDTH, LCD_HEIGHT);
    for (y = 0; y < LCD_HEIGHT; y++)
    {
        for (x = 0; x < LCD_WIDTH; x++)
            row[x] = (*FBADDR(x, y >> 3) & (1 << (y & 7))) ? 255 : 0;
        write(fd, row, sizeof(row));
    }
    close(fd);
}

void focus_sim_step(void)
{
    if (script_pos < 0)
    {
        int fd = open("/focus_script.txt", O_RDONLY);
        script_pos = 0;
        if (fd >= 0)
        {
            script_len = read(fd, script, sizeof(script) - 1);
            if (script_len < 0) script_len = 0;
            close(fd);
        }
    }
    if (wait_frames > 0)
    {
        wait_frames--;
        return;
    }
    while (script_pos < script_len)
    {
        char line[64], arg[32] = "";
        int n = 0, count = 0;
        char cmd = 0;

        while (script_pos < script_len && script[script_pos] != '\n' &&
               n < (int)sizeof(line) - 1)
            line[n++] = script[script_pos++];
        line[n] = '\0';
        if (script_pos < script_len)
            script_pos++;
        if (sscanf(line, "%c %31s %d", &cmd, arg, &count) < 1)
            continue;
        switch (cmd)
        {
        case 'k':
            button_queue_post(button_code(arg), 0);
            button_queue_post(button_code(arg) | BUTTON_REL, 0);
            wait_frames = 1;
            return;
        case 'h':
            button_queue_post(button_code(arg), 0);
            while (count-- > 0)
                button_queue_post(button_code(arg) | BUTTON_REPEAT, 0);
            button_queue_post(button_code(arg) | BUTTON_REL, 0);
            wait_frames = 1;
            return;
        case 'w':
            wait_frames = atoi(arg);
            return;
        case 's':
            dump(arg);
            break;
        case 'q':
            sim_do_exit();
        default:
            break;
        }
    }
}
