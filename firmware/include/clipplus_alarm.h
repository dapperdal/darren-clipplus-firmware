#ifndef CLIPPLUS_ALARM_H
#define CLIPPLUS_ALARM_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    CLIPPLUS_POWER_ACTIVE = 0,
    CLIPPLUS_POWER_SLEEP,
    CLIPPLUS_POWER_OFF
} clipplus_power_state_t;

typedef struct {
    bool enabled;
    uint8_t weekdays; /* bit 0 Sunday through bit 6 Saturday; zero is invalid when enabled */
    uint8_t hour;
    uint8_t minute;
} clipplus_alarm_config_t;

typedef struct {
    uint32_t last_fired_day;
} clipplus_alarm_state_t;

typedef enum {
    CLIPPLUS_ALARM_NONE = 0,
    CLIPPLUS_ALARM_NOTIFY,
    CLIPPLUS_ALARM_WAKE_REQUEST,
    CLIPPLUS_ALARM_BLOCKED_UNVERIFIED_WAKE
} clipplus_alarm_action_t;

bool clipplus_alarm_validate(const clipplus_alarm_config_t *config);
void clipplus_alarm_init(clipplus_alarm_config_t *config, clipplus_alarm_state_t *state);
clipplus_alarm_action_t clipplus_alarm_service(
    const clipplus_alarm_config_t *config, clipplus_alarm_state_t *state,
    uint32_t epoch_seconds, clipplus_power_state_t power,
    bool rtc_and_wake_verified);

#endif
