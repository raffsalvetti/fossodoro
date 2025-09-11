
#include "sound.h"

void sound_play(sound_play_data_t* data) {
    ao_device *device;
    ao_sample_format sample_format;
    
    mpg123_handle *mh;
    size_t outmemsize, done;
    
    unsigned char *outmemory;
    int driver_id, channels, encoding, error, rate;

    ao_initialize();
    driver_id = ao_default_driver_id();

    mpg123_init();
    mh = mpg123_new(NULL, &error);
    outmemsize = mpg123_outblock(mh);
    outmemory = (unsigned char*) malloc(outmemsize * sizeof(unsigned char));

    mpg123_open(mh, data->audio_file);
    mpg123_getformat(mh, &rate, &channels, &encoding);
    mpg123_volume(mh, data->volume);

    sample_format.channels = channels;
    sample_format.bits = mpg123_encsize(encoding) * SOUND_BITS;
    sample_format.byte_format = AO_FMT_NATIVE;
    sample_format.rate = rate;
    sample_format.matrix = 0;
    
    device = ao_open_live(driver_id, &sample_format, NULL);
    
    while (mpg123_read(mh, outmemory, outmemsize, &done) == MPG123_OK)
        ao_play(device, outmemory, done);

    free(outmemory);
    mpg123_close(mh);
    mpg123_delete(mh);
    mpg123_exit();

    ao_close(device);
    ao_shutdown();
}