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
#include "audio.h"
#include "debug.h"
#include "dir.h"
#include "file.h"
#include "kernel.h"
#include "lcd.h"
#include "system.h"
#include "thread.h"
#include "settings.h"
#include "focus_ui.h"

void sim_do_exit(void) NORETURN_ATTR;
void sim_trigger_usb(bool inserted);

static char script[8192];
static int script_pos = -1, script_len, wait_frames;
static struct event_queue usb_test_queue;
static long usb_test_stack[DEFAULT_STACK_SIZE / sizeof(long)];

static void usb_test_thread(void)
{
    struct queue_event ev;
    int delay = TIMEOUT_BLOCK;
    while (true)
    {
        queue_wait_w_tmo(&usb_test_queue, &ev, delay);
        if (ev.id == SYS_TIMEOUT)
        {
            sim_trigger_usb(false);
            delay = TIMEOUT_BLOCK;
        }
        else
            delay = ev.data > 0 ? ev.data : TIMEOUT_BLOCK;
    }
}

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
    DEBUGF("SIMSHOT %s audio=%d volume=%d storage=%d display=%d\n", name,
           audio_status(), global_status.volume, focus_storage_generation,
           focus_display_on());
}

void focus_sim_step(void)
{
    if (script_pos < 0)
    {
        queue_init(&usb_test_queue, false);
        create_thread(usb_test_thread, usb_test_stack, sizeof(usb_test_stack),
                      0, "usb-test" IF_PRIO(, PRIORITY_BACKGROUND) IF_COP(, CPU));
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
        case 'U':
            sim_trigger_usb(atoi(arg) != 0);
            /* U 1 N unplugs after N tenths of a second even while the
             * main thread is blocked inside the stock USB screen. */
            queue_post(&usb_test_queue, 1,
                       atoi(arg) != 0 && count > 0 ? count * HZ / 10 : 0);
            wait_frames = 1;
            return;
        case 'q':
            sim_do_exit();
        default:
            break;
        }
    }
}
