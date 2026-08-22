#ifndef CLIPPLUS_STARTUP_H
#define CLIPPLUS_STARTUP_H

#include "clipplus_target.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    CLIPPLUS_STARTUP_RESET = 0,
    CLIPPLUS_STARTUP_RECOVERY,
    CLIPPLUS_STARTUP_LCD,
    CLIPPLUS_STARTUP_INPUT,
    CLIPPLUS_STARTUP_STORAGE_READ_ONLY,
    CLIPPLUS_STARTUP_APPLICATION,
    CLIPPLUS_STARTUP_HALTED
} clipplus_startup_phase_t;

typedef struct {
    uint32_t reset_reason;
    uint32_t button_mask;
    bool storage_read_only_ready;
    clipplus_startup_phase_t phase;
    uint32_t failure_reason;
} clipplus_startup_context_t;

/* Returns true only when the normal application may run. Recovery and any
 * failed prerequisite stop before media/index writes or application start. */
bool clipplus_startup_run(const clipplus_board_ops_t *board,
                          clipplus_startup_context_t *context);

#endif
