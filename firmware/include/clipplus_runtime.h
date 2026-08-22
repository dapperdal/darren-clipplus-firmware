#ifndef CLIPPLUS_RUNTIME_H
#define CLIPPLUS_RUNTIME_H

#include "clipplus_boot.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CLIPPLUS_EVENT_QUEUE_CAPACITY 16u
#define CLIPPLUS_MEDIA_INDEX_CAPACITY 64u
#define CLIPPLUS_MEDIA_PATH_MAX 96u
#define CLIPPLUS_RESUME_MAGIC 0x52534D45u /* "RSME" */
#define CLIPPLUS_RESUME_VERSION 1u

typedef enum {
    CLIPPLUS_EVENT_NONE = 0,
    CLIPPLUS_EVENT_BUTTON,
    CLIPPLUS_EVENT_TICK,
    CLIPPLUS_EVENT_MEDIA_STEP,
    CLIPPLUS_EVENT_RESUME_COMMIT
} clipplus_event_type_t;

typedef struct {
    clipplus_event_type_t type;
    uint16_t value;
    uint32_t timestamp_ms;
} clipplus_event_t;

typedef struct {
    clipplus_event_t items[CLIPPLUS_EVENT_QUEUE_CAPACITY];
    uint8_t read_index;
    uint8_t write_index;
    uint8_t count;
    uint32_t dropped_events;
} clipplus_event_queue_t;

void clipplus_events_init(clipplus_event_queue_t *queue);
bool clipplus_events_post(clipplus_event_queue_t *queue, clipplus_event_t event);
bool clipplus_events_pop(clipplus_event_queue_t *queue, clipplus_event_t *event);

typedef enum {
    CLIPPLUS_MEDIA_INVALID = 0,
    CLIPPLUS_MEDIA_MP3,
    CLIPPLUS_MEDIA_FLAC_PLANNED
} clipplus_media_format_t;

typedef struct {
    char path[CLIPPLUS_MEDIA_PATH_MAX];
    clipplus_media_format_t format;
    uint32_t duration_ms;
    uint32_t resume_ms;
} clipplus_media_item_t;

typedef struct {
    const char *path;
    uint32_t duration_ms;
} clipplus_scan_entry_t;

typedef struct {
    clipplus_media_item_t items[CLIPPLUS_MEDIA_INDEX_CAPACITY];
    uint16_t count;
    uint16_t rejected;
    size_t cursor;
    bool complete;
} clipplus_media_index_t;

void clipplus_media_index_init(clipplus_media_index_t *index);
size_t clipplus_media_scan_step(clipplus_media_index_t *index,
                                const clipplus_scan_entry_t *entries,
                                size_t entry_count, size_t budget);
clipplus_media_format_t clipplus_media_validate_path(const char *path);

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t generation;
    uint32_t media_id;
    uint32_t position_ms;
    uint32_t duration_ms;
    uint32_t flags;
    uint32_t crc32;
} clipplus_resume_slot_t;

typedef struct {
    clipplus_resume_slot_t slots[2];
} clipplus_resume_store_t;

void clipplus_resume_init(clipplus_resume_store_t *store);
bool clipplus_resume_commit(clipplus_resume_store_t *store, uint32_t media_id,
                            uint32_t position_ms, uint32_t duration_ms);
bool clipplus_resume_load(const clipplus_resume_store_t *store, uint32_t media_id,
                          uint32_t *position_ms);

typedef enum {
    CLIPPLUS_FAULT_NONE = 0,
    CLIPPLUS_FAULT_WATCHDOG,
    CLIPPLUS_FAULT_PANIC,
    CLIPPLUS_FAULT_STORAGE
} clipplus_fault_t;

typedef struct {
    uint8_t consecutive_faults;
    uint8_t recovery_threshold;
    uint32_t last_kick_ms;
} clipplus_watchdog_policy_t;

void clipplus_watchdog_init(clipplus_watchdog_policy_t *policy, uint8_t threshold);
void clipplus_watchdog_kick(clipplus_watchdog_policy_t *policy, uint32_t now_ms);
bool clipplus_watchdog_expired(const clipplus_watchdog_policy_t *policy,
                               uint32_t now_ms, uint32_t timeout_ms);
bool clipplus_watchdog_record(clipplus_watchdog_policy_t *policy, clipplus_fault_t fault);

#endif
