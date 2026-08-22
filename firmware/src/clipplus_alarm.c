#include "clipplus_alarm.h"

#include <stddef.h>

#define SECONDS_PER_DAY 86400u

bool clipplus_alarm_validate(const clipplus_alarm_config_t *config)
{
    if (!config) return false;
    if (!config->enabled) return true;
    return config->weekdays != 0u && config->hour < 24u && config->minute < 60u;
}

void clipplus_alarm_init(clipplus_alarm_config_t *config, clipplus_alarm_state_t *state)
{
    if (config) {
        config->enabled = false;
        config->weekdays = 0u;
        config->hour = 0u;
        config->minute = 0u;
    }
    if (state) state->last_fired_day = UINT32_MAX;
}

clipplus_alarm_action_t clipplus_alarm_service(
    const clipplus_alarm_config_t *config, clipplus_alarm_state_t *state,
    uint32_t epoch_seconds, clipplus_power_state_t power,
    bool rtc_and_wake_verified)
{
    if (!config || !state || !clipplus_alarm_validate(config) || !config->enabled)
        return CLIPPLUS_ALARM_NONE;
    uint32_t day = epoch_seconds / SECONDS_PER_DAY;
    uint32_t seconds = epoch_seconds % SECONDS_PER_DAY;
    uint32_t weekday = (day + 4u) % 7u; /* Unix epoch began on Thursday. */
    uint32_t alarm_seconds = (uint32_t)config->hour * 3600u + (uint32_t)config->minute * 60u;
    if ((config->weekdays & (uint8_t)(1u << weekday)) == 0u || seconds < alarm_seconds ||
        state->last_fired_day == day)
        return CLIPPLUS_ALARM_NONE;
    if (power != CLIPPLUS_POWER_ACTIVE && !rtc_and_wake_verified)
        return CLIPPLUS_ALARM_BLOCKED_UNVERIFIED_WAKE;
    state->last_fired_day = day;
    return power == CLIPPLUS_POWER_ACTIVE ? CLIPPLUS_ALARM_NOTIFY : CLIPPLUS_ALARM_WAKE_REQUEST;
}
