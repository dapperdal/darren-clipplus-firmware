#ifndef CLIPPLUS_PLAYBACK_H
#define CLIPPLUS_PLAYBACK_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    bool playlist_created;
    bool current_track_ready;
    bool playback_started;
    char directory[128];
    char filename[128];
    char current_path[256];
} clipplus_mock_playback_t;

bool clipplus_mock_playback_start(clipplus_mock_playback_t *state,
                                  const char *absolute_path);

#endif
