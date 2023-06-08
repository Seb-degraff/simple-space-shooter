
#include <stdio.h>

typedef struct sound_clip_t
{
    float* samples;
    int frame_count;
}
sound_clip_t;

typedef struct playing_sound_t
{
    double time;
    float amplitude;
    float freq;
    bool white_noise;
    int current_clip_frame;
    sound_clip_t clip;
}
playing_sound_t;

int playing_sound_count = 0;
playing_sound_t playing_sounds[10];

struct span_t
{
    uint8_t* ptr;
    size_t size;
};

span_t load_entire_file(const char* filename)
{
    span_t span = {};

    FILE* f = fopen(filename, "rb");
    fseek(f, 0, SEEK_END);
    span.size = ftell(f);
    fseek(f, 0, SEEK_SET);

    span.ptr = (uint8_t*) malloc(span.size);
    fread(span.ptr, span.size, 1, f);
    fclose(f);

    return span;
}


// nice fmt block struct from cute_sound.h
#pragma pack(push, 1)
typedef struct wav_fmt_block_t
{
    uint16_t wFormatTag;
    uint16_t nChannels;
    uint32_t nSamplesPerSec;
    uint32_t nAvgBytesPerSec;
    uint16_t nBlockAlign;
    uint16_t wBitsPerSample;
    uint16_t cbSize;
    uint16_t wValidBitsPerSample;
    uint32_t dwChannelMask;
    uint8_t SubFormat[18];
} wav_fmt_block_t;
#pragma pack(pop)

sound_clip_t load_sound_clip(const char* filename)
{
    // TODO: check if reading past the end of the file.
    uint8_t* file = load_entire_file(filename).ptr;

    assert(file[0] == 'R');
    assert(file[1] == 'I');
    assert(file[2] == 'F');
    assert(file[3] == 'F');
    file += 4; // RIFF
    file += 4; // size
    assert(file[0] == 'W');
    assert(file[1] == 'A');
    assert(file[2] == 'V');
    assert(file[3] == 'E');
    file += 4; // WAVE

    // TODO: look for the ftm block, it's not always following the first block
    assert(file[0] == 'f');
    assert(file[1] == 'm');
    assert(file[2] == 't');
    assert(file[3] == 0x20);
    file += 4; // FMT
    int32_t* ptr = (int32_t*) file;
    int32_t block_size = *ptr;
    printf("size: %i", block_size);
    file += 4; // size

    wav_fmt_block_t* format_block = (wav_fmt_block_t*) file;
    printf("audio format: %i\n", format_block->wFormatTag);
    printf("nChannels: %i\n", format_block->nChannels);
    printf("samples per sec: %i\n", format_block->nSamplesPerSec);
    printf("bit per sample: %i\n", format_block->wBitsPerSample);
    printf("nBlockAlign: %i\n", format_block->nBlockAlign);
    printf("nSamplesPerSec: %i\n", format_block->nSamplesPerSec);

    file = (uint8_t*) ptr + 4 + 16;

    printf("%c\n", (char) file[0]);
    printf("%c\n", (char) file[1]);
    printf("%c\n", (char) file[2]);
    printf("%c\n", (char) file[3]);

    // for (int delta = 0; delta < 1000; delta++) {
    //     file += 1;
    //     // Read the data block
    //     bool ok = file[0] == 'd' && file[1] == 'a' && file[2] == 't' && file[3] == 'a';

    //     if (ok) {
    //         printf("Delta %i worked!!!!\n", delta);
    //         break;
    //     }
    // }

    file += 4; // data
    uint64_t data_size = *(uint32_t*) file;
    file += 4; // data size
    
    int samples_count = data_size / sizeof(uint16_t) / 2;

    sound_clip_t clip = {
        .samples = (float*) malloc(sizeof(float) * samples_count),
        .frame_count = samples_count,
    };

    for (int i = 0; i < samples_count; i++) {
        int16_t val = ((int) file[0]) | (int) file[1] << 8;
        file += 4;
        clip.samples[i] = ((float) val / INT16_MAX);
    }

    return clip;
}

playing_sound_t* add_playing_sound(playing_sound_t sound)
{
    if (playing_sound_count == ARR_LEN(playing_sounds)) {
        printf("Too many sounds playing at once, skipping one\n");
        return NULL;
    }

    playing_sounds[playing_sound_count++] = sound;

    return &playing_sounds[playing_sound_count-1];
}

playing_sound_t* audio_play_sound_clip(sound_clip_t clip)
{
    return add_playing_sound((playing_sound_t) {
        .clip = clip,
    });
}

playing_sound_t* audio_play_sound(float freq, bool white_noise)
{
    return add_playing_sound((playing_sound_t) {
        .amplitude = 1,
        .freq = freq,
        .white_noise = white_noise,
    });
}



void audio_callback(float* buffer, int num_frames, int num_channels)
{
    assert(num_channels == 1); // TODO: support more channels

    for (int frame = 0; frame < num_frames; frame++) {
        float sample = 0;
        for (int s_idx = 0; s_idx < playing_sound_count; s_idx++) {
            playing_sound_t* s = &playing_sounds[s_idx];

            s->amplitude = MAX(s->amplitude - 0.0001f, 0);

            bool sound_finished = false;

            if (s->white_noise) {
                // white noise
                float random = (rand() / (RAND_MAX-1)) * 2 - 1;
                sample += random * 0.2f * s->amplitude;
                if (s->amplitude <= 0) {
                    sound_finished = true;
                }
            } else if (s->clip.samples) {
                // clip
                sample += s->clip.samples[s->current_clip_frame];
                s->current_clip_frame++;
                if (s->current_clip_frame >= s->clip.frame_count) {
                    sound_finished = true;
                }
            } else {
                // sin wave
                s->freq = MAX(s->freq - 0.1, 0);
                sample += sinf(s->time * 2 * M_PI) * 0.2f * s->amplitude;
                s->time += 1.0/44100 * s->freq; // TODO: remove hardcoded sample rate
                if (s->amplitude <= 0) {
                    sound_finished = true;
                }
            }
            
            // remove sound
            if (sound_finished) {
                playing_sounds[s_idx] = playing_sounds[playing_sound_count-1];
                playing_sound_count--;
                s_idx--;
            }
        }

        buffer[frame] = sample;
    }
}

void audio_init()
{
    // Initialize sokol audio by providing a logger function (the default one that comes with sokol_log)
    // and a callback that will write the samples to a buffer provided by sokol audio (probably grabbed
    // by sokol from the platform's audio system).
    saudio_setup((saudio_desc) {
        .stream_cb = audio_callback,
        .logger = {
            .func = slog_func,
        }
    });
}
