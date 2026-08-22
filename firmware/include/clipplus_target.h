#ifndef CLIPPLUS_TARGET_H
#define CLIPPLUS_TARGET_H

#include <stdbool.h>
#include <stdint.h>

/* Hardware facts from the authorized Clip+ reference configuration. These
 * constants describe the target; they do not prove that this project boots. */
#define CLIPPLUS_TARGET_NAME "SanDisk Sansa Clip+"
#define CLIPPLUS_TARGET_CPU "AS3525v2"
#define CLIPPLUS_TARGET_CPU_HZ 192000000u
#define CLIPPLUS_LCD_WIDTH 128u
#define CLIPPLUS_LCD_HEIGHT 64u
#define CLIPPLUS_LCD_BITS_PER_PIXEL 1u
#define CLIPPLUS_REFERENCE_CODEC_BYTES 0xF8000u
#define CLIPPLUS_REFERENCE_PLUGIN_BYTES 0x80000u
#define CLIPPLUS_INTERNAL_STORAGE_BYTES (8ull * 1024ull * 1024ull * 1024ull)

typedef enum {
    CLIPPLUS_KEY_NONE = 0,
    CLIPPLUS_KEY_UP,
    CLIPPLUS_KEY_DOWN,
    CLIPPLUS_KEY_LEFT,
    CLIPPLUS_KEY_RIGHT,
    CLIPPLUS_KEY_HOME,
    CLIPPLUS_KEY_SELECT,
    CLIPPLUS_KEY_POWER,
    CLIPPLUS_KEY_VOLUME_UP,
    CLIPPLUS_KEY_VOLUME_DOWN
} clipplus_key_t;

typedef struct {
    void (*lcd_init)(void);
    void (*lcd_clear)(void);
    void (*lcd_present)(const uint8_t *framebuffer, uint32_t bytes);
    clipplus_key_t (*button_read)(void);
    void (*watchdog_kick)(void);
    void (*enter_recovery)(uint32_t reason);
    bool (*storage_mount_read_only)(void);
    bool (*audio_start_mp3)(void);
    void (*audio_stop)(void);
} clipplus_board_ops_t;

extern const clipplus_board_ops_t clipplus_host_board;

#endif
