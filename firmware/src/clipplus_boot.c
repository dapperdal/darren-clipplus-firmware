#include "clipplus_boot.h"

#include <stddef.h>

static uint32_t crc32_update(uint32_t crc, const uint8_t *bytes, uint32_t size)
{
    for (uint32_t i = 0; i < size; ++i) {
        crc ^= bytes[i];
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ (0xEDB88320u & (uint32_t)-(int)(crc & 1u));
    }
    return crc;
}

uint32_t clipplus_crc32(const void *data, uint32_t size)
{
    return ~crc32_update(0xFFFFFFFFu, (const uint8_t *)data, size);
}

clipplus_boot_result_t clipplus_boot_validate_header(
    const clipplus_image_header_t *header, uint32_t available_bytes)
{
    if (!header) return CLIPPLUS_BOOT_BAD_ARGUMENT;
    if (header->magic == CLIPPLUS_RECOVERY_MAGIC)
        return CLIPPLUS_BOOT_RECOVERY_REQUESTED;
    if (header->magic != CLIPPLUS_IMAGE_MAGIC)
        return CLIPPLUS_BOOT_BAD_MAGIC;
    if (header->format_version != CLIPPLUS_IMAGE_FORMAT_VERSION)
        return CLIPPLUS_BOOT_BAD_VERSION;
    if (header->image_size == 0 || header->image_size > available_bytes)
        return CLIPPLUS_BOOT_BAD_SIZE;
    clipplus_image_header_t copy = *header;
    copy.header_crc32 = 0;
    if (clipplus_crc32(&copy, (uint32_t)sizeof(copy)) != header->header_crc32)
        return CLIPPLUS_BOOT_BAD_CHECKSUM;
    return CLIPPLUS_BOOT_OK;
}

clipplus_boot_result_t clipplus_boot_preflight(
    const clipplus_image_header_t *header, const void *image, uint32_t image_bytes)
{
    clipplus_boot_result_t result = clipplus_boot_validate_header(header, image_bytes);
    if (result != CLIPPLUS_BOOT_OK) return result;
    if (!image) return CLIPPLUS_BOOT_BAD_ARGUMENT;
    if (clipplus_crc32(image, header->image_size) != header->image_crc32)
        return CLIPPLUS_BOOT_BAD_CHECKSUM;
    return CLIPPLUS_BOOT_OK;
}

bool clipplus_boot_recovery_requested(uint32_t reset_reason, uint32_t button_mask)
{
    /* Conservative policy: a watchdog/panic reset or held Home+Select enters
     * recovery before media or the normal application are touched. */
    return reset_reason != 0u || (button_mask & 0x03u) == 0x03u;
}
