/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "clipplus_ui.h"
#include "clipplus_boot.h"
#include "clipplus_runtime.h"
#include "clipplus_startup.h"
#include "clipplus_alarm.h"
#include "clipplus_playback.h"

#include <stdio.h>
#include <string.h>

static void write_pgm(const char *path,
                      const uint8_t fb[CLIPPLUS_LCD_HEIGHT][CLIPPLUS_LCD_WIDTH])
{
    FILE *out = fopen(path, "wb");
    if (!out) return;
    fprintf(out, "P5\n128 64\n255\n");
    for (size_t y = 0; y < CLIPPLUS_LCD_HEIGHT; ++y)
        for (size_t x = 0; x < CLIPPLUS_LCD_WIDTH; ++x) fputc(fb[y][x] ? 255 : 0, out);
    fclose(out);
}

static clipplus_button_t button_for(const char *word, bool *long_press)
{
    *long_press = strncmp(word, "hold-", 5) == 0;
    if (*long_press) word += 5;
    if (strcmp(word, "up") == 0) return CLIPPLUS_BUTTON_UP;
    if (strcmp(word, "down") == 0) return CLIPPLUS_BUTTON_DOWN;
    if (strcmp(word, "left") == 0) return CLIPPLUS_BUTTON_LEFT;
    if (strcmp(word, "right") == 0) return CLIPPLUS_BUTTON_RIGHT;
    if (strcmp(word, "home") == 0) return CLIPPLUS_BUTTON_HOME;
    if (strcmp(word, "select") == 0) return CLIPPLUS_BUTTON_SELECT;
    if (strcmp(word, "power") == 0) return CLIPPLUS_BUTTON_POWER;
    if (strcmp(word, "play") == 0) return CLIPPLUS_BUTTON_PLAY;
    if (strcmp(word, "vol-up") == 0) return CLIPPLUS_BUTTON_VOL_UP;
    if (strcmp(word, "vol-down") == 0) return CLIPPLUS_BUTTON_VOL_DOWN;
    return CLIPPLUS_BUTTON_NONE;
}

static void print_state(const clipplus_ui_state_t *s)
{
    static const char *const names[] = {"HOME", "MUSIC", "RADIO", "RECORDINGS", "EQUALISER", "ALARM", "PODCASTS", "NOW_PLAYING"};
    printf("screen=%s home_index=%u playing=%s elapsed=%lus\n", names[s->screen],
           s->home_index, s->playing ? "yes" : "no", (unsigned long)s->elapsed_seconds);
}

int main(int argc, char **argv)
{
    clipplus_ui_state_t state;
    uint8_t framebuffer[CLIPPLUS_LCD_HEIGHT][CLIPPLUS_LCD_WIDTH];
    clipplus_ui_init(&state);
    if (argc > 1 && strcmp(argv[1], "--self-test") == 0) {
        clipplus_event_queue_t events;
        clipplus_events_init(&events);
        for (uint16_t i = 0; i < CLIPPLUS_EVENT_QUEUE_CAPACITY; ++i)
            if (!clipplus_events_post(&events, (clipplus_event_t){CLIPPLUS_EVENT_TICK, i, i})) return 13;
        if (clipplus_events_post(&events, (clipplus_event_t){CLIPPLUS_EVENT_TICK, 99, 99}) ||
            events.dropped_events != 1u) return 14;
        clipplus_event_t event;
        if (!clipplus_events_pop(&events, &event) || event.value != 0u) return 15;

        const clipplus_scan_entry_t scan_entries[] = {
            {"/Music/track.mp3", 180000u}, {"/Podcasts/episode.flac", 3600000u},
            {"/bad/../escape.mp3", 1000u}, {"/notes.txt", 1000u}
        };
        clipplus_media_index_t media;
        clipplus_media_index_init(&media);
        if (clipplus_media_scan_step(&media, scan_entries, 4u, 2u) != 2u || media.count != 2u || media.complete)
            return 16;
        if (clipplus_media_scan_step(&media, scan_entries, 4u, 2u) != 2u ||
            media.count != 2u || media.rejected != 2u || !media.complete ||
            media.items[1].format != CLIPPLUS_MEDIA_FLAC_PLANNED) return 17;

        clipplus_resume_store_t resumes;
        uint32_t resume_ms;
        clipplus_resume_init(&resumes);
        if (!clipplus_resume_commit(&resumes, 42u, 5000u, 10000u) ||
            !clipplus_resume_commit(&resumes, 42u, 8000u, 10000u) ||
            !clipplus_resume_load(&resumes, 42u, &resume_ms) || resume_ms != 8000u) return 18;
        resumes.slots[1].crc32 ^= 1u;
        if (!clipplus_resume_load(&resumes, 42u, &resume_ms) || resume_ms != 5000u) return 19;

        clipplus_watchdog_policy_t watchdog;
        clipplus_watchdog_init(&watchdog, 2u);
        clipplus_watchdog_kick(&watchdog, 0u);
        if (!clipplus_watchdog_expired(&watchdog, 1001u, 1000u) ||
            clipplus_watchdog_record(&watchdog, CLIPPLUS_FAULT_STORAGE) ||
            !clipplus_watchdog_record(&watchdog, CLIPPLUS_FAULT_WATCHDOG)) return 20;

        clipplus_startup_context_t startup = {0};
        startup.reset_reason = 1u;
        startup.storage_read_only_ready = true;
        if (clipplus_startup_run(&clipplus_host_board, &startup) ||
            startup.phase != CLIPPLUS_STARTUP_RECOVERY) return 21;
        startup.reset_reason = 0u;
        startup.storage_read_only_ready = false;
        if (clipplus_startup_run(&clipplus_host_board, &startup) ||
            startup.phase != CLIPPLUS_STARTUP_HALTED) return 22;
        startup.storage_read_only_ready = true;
        if (!clipplus_startup_run(&clipplus_host_board, &startup) ||
            startup.phase != CLIPPLUS_STARTUP_APPLICATION) return 23;

        clipplus_alarm_config_t alarm;
        clipplus_alarm_state_t alarm_state;
        clipplus_alarm_init(&alarm, &alarm_state);
        if (clipplus_alarm_service(&alarm, &alarm_state, 0u, CLIPPLUS_POWER_ACTIVE, false) != CLIPPLUS_ALARM_NONE)
            return 24;
        alarm.enabled = true;
        alarm.weekdays = 1u << 4; /* Thursday, day zero of the Unix epoch. */
        alarm.hour = 0u;
        alarm.minute = 1u;
        if (!clipplus_alarm_validate(&alarm) ||
            clipplus_alarm_service(&alarm, &alarm_state, 59u, CLIPPLUS_POWER_ACTIVE, false) != CLIPPLUS_ALARM_NONE ||
            clipplus_alarm_service(&alarm, &alarm_state, 60u, CLIPPLUS_POWER_ACTIVE, false) != CLIPPLUS_ALARM_NOTIFY ||
            clipplus_alarm_service(&alarm, &alarm_state, 120u, CLIPPLUS_POWER_ACTIVE, false) != CLIPPLUS_ALARM_NONE)
            return 25;
        alarm_state.last_fired_day = UINT32_MAX;
        if (clipplus_alarm_service(&alarm, &alarm_state, 60u, CLIPPLUS_POWER_OFF, false) != CLIPPLUS_ALARM_BLOCKED_UNVERIFIED_WAKE ||
            clipplus_alarm_service(&alarm, &alarm_state, 60u, CLIPPLUS_POWER_OFF, true) != CLIPPLUS_ALARM_WAKE_REQUEST)
            return 26;
        alarm.minute = 60u;
        if (clipplus_alarm_validate(&alarm)) return 27;

        const uint8_t boot_payload[] = {0x43, 0x50, 0x4C, 0x55};
        clipplus_image_header_t boot_header = {0};
        boot_header.magic = CLIPPLUS_IMAGE_MAGIC;
        boot_header.format_version = CLIPPLUS_IMAGE_FORMAT_VERSION;
        boot_header.image_size = (uint32_t)sizeof(boot_payload);
        boot_header.image_crc32 = clipplus_crc32(boot_payload, sizeof(boot_payload));
        clipplus_image_header_t header_for_crc = boot_header;
        boot_header.header_crc32 = clipplus_crc32(&header_for_crc, sizeof(header_for_crc));
        if (clipplus_boot_preflight(&boot_header, boot_payload, sizeof(boot_payload)) != CLIPPLUS_BOOT_OK)
            return 11;
        if (!clipplus_boot_recovery_requested(1u, 0u) ||
            !clipplus_boot_recovery_requested(0u, 0x03u) ||
            clipplus_boot_recovery_requested(0u, 0u)) return 12;
        clipplus_ui_input(&state, CLIPPLUS_BUTTON_RIGHT, false);
        if (state.home_index != 1) return 1;
        clipplus_ui_input(&state, CLIPPLUS_BUTTON_SELECT, false);
        if (state.screen != CLIPPLUS_SCREEN_RADIO) return 2;
        clipplus_ui_input(&state, CLIPPLUS_BUTTON_SELECT, true);
        if (state.screen != CLIPPLUS_SCREEN_NOW_PLAYING) return 3;
        clipplus_ui_input(&state, CLIPPLUS_BUTTON_SELECT, false);
        if (!state.playing) return 34;
        clipplus_ui_input(&state, CLIPPLUS_BUTTON_PLAY, false);
        if (state.playing) return 35;
        clipplus_ui_input(&state, CLIPPLUS_BUTTON_PLAY, false);
        if (!state.playing) return 36;
        clipplus_ui_input(&state, CLIPPLUS_BUTTON_VOL_UP, false);
        if (state.volume != 61 || state.volume_overlay_ms != 2000) return 37;
        clipplus_ui_input(&state, CLIPPLUS_BUTTON_VOL_DOWN, false);
        if (state.volume != 60 || state.volume_overlay_ms != 2000) return 38;
        state.volume = 100;
        clipplus_ui_input(&state, CLIPPLUS_BUTTON_VOL_DOWN, false);
        if (state.volume != 99) return 42;
        state.volume = 0;
        clipplus_ui_input(&state, CLIPPLUS_BUTTON_VOL_UP, false);
        if (state.volume != 1) return 43;
        clipplus_ui_tick(&state, 2000);
        if (state.volume_overlay_ms != 0) return 44;
        clipplus_ui_input(&state, CLIPPLUS_BUTTON_LEFT, false);
        if (state.elapsed_seconds != 0) return 39;
        clipplus_ui_input(&state, CLIPPLUS_BUTTON_RIGHT, false);
        if (state.elapsed_seconds != 5) return 40;
        clipplus_ui_tick(&state, 1200);
        if (!state.playing || state.elapsed_seconds != 6) return 4;
        clipplus_ui_input(&state, CLIPPLUS_BUTTON_HOME, false);
        clipplus_ui_render(&state, framebuffer);
        for (size_t y = 56; y < CLIPPLUS_LCD_HEIGHT; ++y)
            for (size_t x = 0; x < CLIPPLUS_LCD_WIDTH; ++x)
                if (framebuffer[y][x]) return 5;
        clipplus_ui_input(&state, CLIPPLUS_BUTTON_HOME, true);
        if (!state.screen_locked) return 46;
        clipplus_ui_render(&state, framebuffer);
        if (!framebuffer[21][28]) return 49;
        if (!state.playing) return 50;
        clipplus_ui_input(&state, CLIPPLUS_BUTTON_DOWN, false);
        if (state.home_index != 1) return 47;
        clipplus_ui_tick(&state, 2000);
        clipplus_ui_render(&state, framebuffer);
        if (state.lock_overlay_ms != 0) return 51;
        clipplus_ui_input(&state, CLIPPLUS_BUTTON_HOME, true);
        if (state.screen_locked) return 48;
        clipplus_ui_render_list(&state, framebuffer);
        if (!framebuffer[11 + state.home_index * 7][4]) return 6;
        for (size_t y = 60; y < CLIPPLUS_LCD_HEIGHT; ++y)
            for (size_t x = 0; x < CLIPPLUS_LCD_WIDTH; ++x)
                if (framebuffer[y][x]) return 7;
        clipplus_ui_render_single(&state, framebuffer);
        if (!framebuffer[57][54 + state.home_index * 4]) return 8;
        if (!framebuffer[13][64] || !framebuffer[53][64]) return 9;
        for (size_t y = 0; y < 20; ++y)
            for (size_t x = 0; x < CLIPPLUS_LCD_WIDTH; ++x)
                if (framebuffer[y][x] && y > 9 && !(x >= 62 && x <= 66 && y <= 17)) return 10;
        clipplus_ui_render_stock_inspired(&state, framebuffer);
        if (!framebuffer[12][4] || !framebuffer[53][49]) return 28;
        clipplus_ui_render_media(&state, framebuffer, false, false, 1, 17);
        if (!framebuffer[12][4] || !framebuffer[30][9]) return 29;
        clipplus_ui_render_media(&state, framebuffer, false, true, 0, 0);
        if (!framebuffer[23][28]) return 30;
        clipplus_ui_render_media(&state, framebuffer, false, false, 0, 1);
        bool has_position = false;
        for (size_t y = 55; y < CLIPPLUS_LCD_HEIGHT; ++y)
            for (size_t x = 0; x < CLIPPLUS_LCD_WIDTH; ++x)
                if (framebuffer[y][x]) has_position = true;
        if (!has_position) return 41;
        clipplus_ui_render_now_playing(&state, framebuffer);
        if (!framebuffer[12][4] || !framebuffer[57][14]) return 31;
        state.battery_minutes = 135;
        clipplus_ui_render_now_playing(&state, framebuffer);
        bool has_battery = false;
        for (size_t y = 0; y < 10; ++y)
            for (size_t x = 82; x < CLIPPLUS_LCD_WIDTH; ++x)
                if (framebuffer[y][x]) has_battery = true;
        if (!has_battery) return 52;
        state.battery_charging = true;
        clipplus_ui_render_now_playing(&state, framebuffer);
        has_battery = false;
        for (size_t y = 0; y < 10; ++y)
            for (size_t x = 100; x < CLIPPLUS_LCD_WIDTH; ++x)
                if (framebuffer[y][x]) has_battery = true;
        if (!has_battery) return 53;
        clipplus_ui_render_volume(&state, framebuffer);
        if (!framebuffer[12][4] || !framebuffer[56][7]) return 45;
        clipplus_mock_playback_t playback;
        const char *sequential_tracks[] = {
            "/MUSIC/Focus Mix.mp3", "/MUSIC/Second Track.mp3",
            "/PODCASTS/Episode One.mp3", "/MUSIC/Final Track.mp3"
        };
        for (size_t i = 0; i < sizeof(sequential_tracks) / sizeof(sequential_tracks[0]); ++i)
        {
            if (!clipplus_mock_playback_start(&playback, sequential_tracks[i]) ||
                !playback.playlist_created || !playback.current_track_ready ||
                !playback.playback_started || strcmp(playback.current_path, sequential_tracks[i])) return 32;
        }
        if (clipplus_mock_playback_start(&playback, "MUSIC/no-root.mp3")) return 33;
        puts("self-test: ok");
        return 0;
    }
    clipplus_ui_render(&state, framebuffer);
    write_pgm("build/clipplus-home.pgm", framebuffer);
    clipplus_ui_render_list(&state, framebuffer);
    write_pgm("build/clipplus-list.pgm", framebuffer);
    clipplus_ui_render_single(&state, framebuffer);
    write_pgm("build/clipplus-single.pgm", framebuffer);
    clipplus_ui_render_stock_inspired(&state, framebuffer);
    write_pgm("build/clipplus-stock-inspired.pgm", framebuffer);
    clipplus_ui_render_media(&state, framebuffer, false, false, 1, 17);
    write_pgm("build/clipplus-media.pgm", framebuffer);
    clipplus_ui_render_media(&state, framebuffer, false, true, 0, 0);
    write_pgm("build/clipplus-media-error.pgm", framebuffer);
    clipplus_ui_render_now_playing(&state, framebuffer);
    write_pgm("build/clipplus-now-playing.pgm", framebuffer);
    clipplus_ui_render_volume(&state, framebuffer);
    write_pgm("build/clipplus-volume.pgm", framebuffer);
    clipplus_ui_render(&state, framebuffer); print_state(&state);
    puts("commands: up down left right select home power play vol-up vol-down hold-select hold-home quit");
    char word[32];
    while (scanf("%31s", word) == 1 && strcmp(word, "quit") != 0) {
        bool long_press;
        clipplus_button_t button = button_for(word, &long_press);
        if (button != CLIPPLUS_BUTTON_NONE) clipplus_ui_input(&state, button, long_press);
        clipplus_ui_tick(&state, 120);
        clipplus_ui_render(&state, framebuffer);
        write_pgm("build/clipplus-home.pgm", framebuffer);
        clipplus_ui_render_list(&state, framebuffer);
        write_pgm("build/clipplus-list.pgm", framebuffer);
        clipplus_ui_render_single(&state, framebuffer);
        write_pgm("build/clipplus-single.pgm", framebuffer);
        clipplus_ui_render_stock_inspired(&state, framebuffer);
        write_pgm("build/clipplus-stock-inspired.pgm", framebuffer);
        clipplus_ui_render_media(&state, framebuffer, false, false, 1, 17);
        write_pgm("build/clipplus-media.pgm", framebuffer);
        clipplus_ui_render_now_playing(&state, framebuffer);
        write_pgm("build/clipplus-now-playing.pgm", framebuffer);
        clipplus_ui_render_volume(&state, framebuffer);
        write_pgm("build/clipplus-volume.pgm", framebuffer);
        clipplus_ui_render(&state, framebuffer); print_state(&state);
    }
    return 0;
}
