#ifndef CLIPPLUS_AS3525V2_TARGET_CONFIG_H
#define CLIPPLUS_AS3525V2_TARGET_CONFIG_H

#include "clipplus_target.h"

/* Porting checklist, not a verified register map. */
#define CLIPPLUS_TARGET_LCD_CONTROLLER "SSD1303-compatible reference path"
#define CLIPPLUS_TARGET_LCD_SPLIT_LINE 16u
#define CLIPPLUS_TARGET_STORAGE "SD protocol internal flash"
#define CLIPPLUS_TARGET_TUNER "SI4702-compatible reference path"
#define CLIPPLUS_TARGET_HAS_MICROPHONE 1

#endif
