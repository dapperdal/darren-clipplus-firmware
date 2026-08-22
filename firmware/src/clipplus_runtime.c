#include "clipplus_runtime.h"
#include "clipplus_memory.h"

void clipplus_events_init(clipplus_event_queue_t *queue)
{
    clipplus_memset(queue, 0, sizeof(*queue));
}

bool clipplus_events_post(clipplus_event_queue_t *queue, clipplus_event_t event)
{
    if (queue->count == CLIPPLUS_EVENT_QUEUE_CAPACITY) {
        ++queue->dropped_events;
        return false;
    }
    queue->items[queue->write_index] = event;
    queue->write_index = (uint8_t)((queue->write_index + 1u) % CLIPPLUS_EVENT_QUEUE_CAPACITY);
    ++queue->count;
    return true;
}

bool clipplus_events_pop(clipplus_event_queue_t *queue, clipplus_event_t *event)
{
    if (!queue->count || !event) return false;
    *event = queue->items[queue->read_index];
    queue->read_index = (uint8_t)((queue->read_index + 1u) % CLIPPLUS_EVENT_QUEUE_CAPACITY);
    --queue->count;
    return true;
}

static char lower_ascii(char c)
{
    return c >= 'A' && c <= 'Z' ? (char)(c + ('a' - 'A')) : c;
}

clipplus_media_format_t clipplus_media_validate_path(const char *path)
{
    if (!path || path[0] != '/' || clipplus_strlen(path) >= CLIPPLUS_MEDIA_PATH_MAX) return CLIPPLUS_MEDIA_INVALID;
    bool component_dot = false;
    for (size_t i = 1; path[i]; ++i) {
        unsigned char c = (unsigned char)path[i];
        if (c < 0x20 || c == ':' || c == '\\') return CLIPPLUS_MEDIA_INVALID;
        if (path[i] == '/') {
            if (component_dot) return CLIPPLUS_MEDIA_INVALID;
            component_dot = false;
        } else {
            component_dot = component_dot && path[i] == '.';
            if (path[i] == '.' && (i == 1 || path[i - 1] == '/')) component_dot = true;
        }
    }
    const char *dot = clipplus_strrchr(path, '.');
    if (!dot || dot == path) return CLIPPLUS_MEDIA_INVALID;
    char e0 = lower_ascii(dot[1]), e1 = lower_ascii(dot[2]), e2 = lower_ascii(dot[3]);
    if (e0 == 'm' && e1 == 'p' && e2 == '3' && dot[4] == '\0') return CLIPPLUS_MEDIA_MP3;
    if (e0 == 'f' && e1 == 'l' && e2 == 'a' && lower_ascii(dot[4]) == 'c' && dot[5] == '\0')
        return CLIPPLUS_MEDIA_FLAC_PLANNED;
    return CLIPPLUS_MEDIA_INVALID;
}

void clipplus_media_index_init(clipplus_media_index_t *index)
{
    clipplus_memset(index, 0, sizeof(*index));
}

size_t clipplus_media_scan_step(clipplus_media_index_t *index,
                                const clipplus_scan_entry_t *entries,
                                size_t entry_count, size_t budget)
{
    if (!index || !entries || index->complete || budget == 0) return 0;
    size_t examined = 0;
    while (index->cursor < entry_count && examined < budget) {
        const clipplus_scan_entry_t *entry = &entries[index->cursor++];
        ++examined;
        clipplus_media_format_t format = clipplus_media_validate_path(entry->path);
        if (format == CLIPPLUS_MEDIA_INVALID || index->count == CLIPPLUS_MEDIA_INDEX_CAPACITY) {
            ++index->rejected;
            continue;
        }
        clipplus_media_item_t *item = &index->items[index->count++];
        clipplus_strncpy(item->path, entry->path, sizeof(item->path) - 1u);
        item->format = format;
        item->duration_ms = entry->duration_ms;
    }
    index->complete = index->cursor >= entry_count;
    return examined;
}

static bool resume_valid(const clipplus_resume_slot_t *slot)
{
    if (slot->magic != CLIPPLUS_RESUME_MAGIC || slot->version != CLIPPLUS_RESUME_VERSION) return false;
    clipplus_resume_slot_t copy = *slot;
    copy.crc32 = 0;
    return clipplus_crc32(&copy, (uint32_t)sizeof(copy)) == slot->crc32;
}

void clipplus_resume_init(clipplus_resume_store_t *store)
{
    clipplus_memset(store, 0, sizeof(*store));
}

bool clipplus_resume_commit(clipplus_resume_store_t *store, uint32_t media_id,
                            uint32_t position_ms, uint32_t duration_ms)
{
    if (!store || media_id == 0 || position_ms > duration_ms) return false;
    bool a_valid = resume_valid(&store->slots[0]);
    bool b_valid = resume_valid(&store->slots[1]);
    unsigned target = !a_valid ? 0u : (!b_valid ? 1u :
        (store->slots[0].generation <= store->slots[1].generation ? 0u : 1u));
    uint32_t generation = 1u;
    if (a_valid && store->slots[0].generation >= generation) generation = store->slots[0].generation + 1u;
    if (b_valid && store->slots[1].generation >= generation) generation = store->slots[1].generation + 1u;
    clipplus_resume_slot_t next = {CLIPPLUS_RESUME_MAGIC, CLIPPLUS_RESUME_VERSION,
        generation, media_id, position_ms, duration_ms, 0u, 0u};
    next.crc32 = clipplus_crc32(&next, (uint32_t)sizeof(next));
    store->slots[target] = next;
    return true;
}

bool clipplus_resume_load(const clipplus_resume_store_t *store, uint32_t media_id,
                          uint32_t *position_ms)
{
    if (!store || !position_ms || media_id == 0) return false;
    const clipplus_resume_slot_t *a = &store->slots[0], *b = &store->slots[1];
    bool av = resume_valid(a) && a->media_id == media_id;
    bool bv = resume_valid(b) && b->media_id == media_id;
    if (!av && !bv) return false;
    const clipplus_resume_slot_t *latest = !bv || (av && a->generation >= b->generation) ? a : b;
    *position_ms = latest->position_ms;
    return true;
}

void clipplus_watchdog_init(clipplus_watchdog_policy_t *policy, uint8_t threshold)
{
    clipplus_memset(policy, 0, sizeof(*policy));
    policy->recovery_threshold = threshold ? threshold : 1u;
}

void clipplus_watchdog_kick(clipplus_watchdog_policy_t *policy, uint32_t now_ms)
{
    policy->last_kick_ms = now_ms;
}

bool clipplus_watchdog_expired(const clipplus_watchdog_policy_t *policy,
                               uint32_t now_ms, uint32_t timeout_ms)
{
    return now_ms - policy->last_kick_ms > timeout_ms;
}

bool clipplus_watchdog_record(clipplus_watchdog_policy_t *policy, clipplus_fault_t fault)
{
    if (fault == CLIPPLUS_FAULT_NONE) return false;
    if (policy->consecutive_faults < UINT8_MAX) ++policy->consecutive_faults;
    return policy->consecutive_faults >= policy->recovery_threshold;
}
