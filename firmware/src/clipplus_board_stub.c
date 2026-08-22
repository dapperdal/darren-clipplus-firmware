#include "clipplus_target.h"

/* Host-safe board adapter. Target register code must replace these functions
 * only after AS3525v2 clock/GPIO/LCD/button validation on real hardware. */
static void board_noop(void) {}
static void board_present(const uint8_t *framebuffer, uint32_t bytes)
{
    (void)framebuffer;
    (void)bytes;
}
static clipplus_key_t board_button(void) { return CLIPPLUS_KEY_NONE; }
static void board_recovery(uint32_t reason) { (void)reason; }
static bool board_storage_read_only(void) { return true; }
static bool board_audio_start(void) { return false; }
static void board_audio_stop(void) {}

const clipplus_board_ops_t clipplus_host_board = {
    board_noop, board_noop, board_present, board_button, board_noop, board_recovery,
    board_storage_read_only, board_audio_start, board_audio_stop
};
