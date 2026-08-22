#ifndef CLIPPLUS_BOOT_H
#define CLIPPLUS_BOOT_H

#include <stdbool.h>
#include <stdint.h>

#define CLIPPLUS_IMAGE_MAGIC 0x43504C55u /* "CPLU" */
#define CLIPPLUS_IMAGE_FORMAT_VERSION 1u
#define CLIPPLUS_RECOVERY_MAGIC 0x52454356u /* "RECV" */

typedef enum {
    CLIPPLUS_BOOT_OK = 0,
    CLIPPLUS_BOOT_BAD_ARGUMENT,
    CLIPPLUS_BOOT_BAD_MAGIC,
    CLIPPLUS_BOOT_BAD_VERSION,
    CLIPPLUS_BOOT_BAD_SIZE,
    CLIPPLUS_BOOT_BAD_CHECKSUM,
    CLIPPLUS_BOOT_RECOVERY_REQUESTED
} clipplus_boot_result_t;

typedef struct {
    uint32_t magic;
    uint32_t format_version;
    uint32_t image_size;
    uint32_t image_crc32;
    uint32_t flags;
    uint32_t header_crc32;
} clipplus_image_header_t;

uint32_t clipplus_crc32(const void *data, uint32_t size);
clipplus_boot_result_t clipplus_boot_validate_header(
    const clipplus_image_header_t *header, uint32_t available_bytes);
clipplus_boot_result_t clipplus_boot_preflight(
    const clipplus_image_header_t *header, const void *image, uint32_t image_bytes);
bool clipplus_boot_recovery_requested(uint32_t reset_reason, uint32_t button_mask);

#endif
