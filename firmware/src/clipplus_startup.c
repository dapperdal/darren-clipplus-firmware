#include "clipplus_startup.h"

#include "clipplus_boot.h"

bool clipplus_startup_run(const clipplus_board_ops_t *board,
                          clipplus_startup_context_t *context)
{
    if (!board || !context) return false;
    context->phase = CLIPPLUS_STARTUP_RESET;
    context->failure_reason = 0;
    if (clipplus_boot_recovery_requested(context->reset_reason, context->button_mask)) {
        context->phase = CLIPPLUS_STARTUP_RECOVERY;
        if (board->enter_recovery) board->enter_recovery(context->reset_reason);
        return false;
    }
    if (!board->lcd_init || !board->lcd_clear || !board->button_read ||
        !board->watchdog_kick || !board->enter_recovery ||
        !board->storage_mount_read_only) {
        context->failure_reason = 1u;
        context->phase = CLIPPLUS_STARTUP_HALTED;
        return false;
    }
    context->phase = CLIPPLUS_STARTUP_LCD;
    board->lcd_init();
    board->lcd_clear();
    context->phase = CLIPPLUS_STARTUP_INPUT;
    (void)board->button_read();
    board->watchdog_kick();
    context->phase = CLIPPLUS_STARTUP_STORAGE_READ_ONLY;
    if (!context->storage_read_only_ready || !board->storage_mount_read_only()) {
        context->failure_reason = 2u;
        context->phase = CLIPPLUS_STARTUP_HALTED;
        board->enter_recovery(context->failure_reason);
        return false;
    }
    context->phase = CLIPPLUS_STARTUP_APPLICATION;
    return true;
}
