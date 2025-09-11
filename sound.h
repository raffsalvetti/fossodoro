#ifndef SOUNDDORO_SOUND_H
#define SOUNDDORO_SOUND_H

#include <ao/ao.h>
#include <mpg123.h>

#ifndef SOUND_BITS
#define SOUND_BITS 8
#endif

typedef struct {
    const char* audio_file;
    double volume;
} sound_play_data_t;

void sound_play(sound_play_data_t* data);

#endif // SOUNDDORO_SOUND_H