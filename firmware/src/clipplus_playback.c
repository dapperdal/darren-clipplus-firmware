#include "clipplus_playback.h"
#include <string.h>

bool clipplus_mock_playback_start(clipplus_mock_playback_t *state,
                                  const char *absolute_path)
{
    const char *slash;
    size_t dir_len;
    size_t file_len;
    if (!state || !absolute_path || absolute_path[0] != '/') return false;
    slash = strrchr(absolute_path, '/');
    if (!slash || slash == absolute_path || !slash[1]) return false;
    dir_len = (size_t)(slash - absolute_path);
    file_len = strlen(slash + 1);
    if (dir_len >= sizeof(state->directory) || file_len >= sizeof(state->filename) ||
        strlen(absolute_path) >= sizeof(state->current_path)) return false;
    memset(state, 0, sizeof(*state));
    memcpy(state->directory, absolute_path, dir_len);
    state->directory[dir_len] = '\0';
    memcpy(state->filename, slash + 1, file_len + 1);
    strcpy(state->current_path, absolute_path);
    state->playlist_created = true;
    state->current_track_ready = true;
    state->playback_started = true;
    return true;
}
