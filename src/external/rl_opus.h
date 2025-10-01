#ifndef RL_OPUS_H
#define RL_OPUS_H
//#pragma once
#include <opusfile.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct rl_opus {
    OggOpusFile *file;
    int sampleRate;
    int channels;
} rl_opus;


// Checks if the given file contains a valid Opus stream
static int IsOpusValid(const char* fileName)
{
    FILE* opusFile = fopen(fileName, "rb");
    if (!opusFile) {
        perror("Error opening file");
        return 1;  // Could not open file
    }

    /************************************************************************************************************************************************************
    op_test() tries to determine if the data provided is an Opus stream, based on the initial Opus ID header packet.
    This header is typically located very early in the file, usually within the first few hundred bytes.
    but there's no fixed guaranteed size — sometimes it needs a bit more than just the first page to confirm it's a valid Opus stream. 
    Opus_header will be assigned a Commonly used size of 1024 bytes, while 512 bytes can be a minimum safe size and 4096 bytes (4 KB) being a robust upper bound
    
    *************************************************************************************************************************************************************/
    unsigned char Opus_header[1024];
    /************************************************************************************************************************************************************/

    size_t bytesRead = fread(Opus_header, 1, sizeof(Opus_header), opusFile);
    fclose(opusFile);  

    if (bytesRead == 0) {
        fprintf(stderr, "Error: Failed to read from file or file is empty.\n");
        return 1;
    }

    
    OpusHead head;
    int result = op_test(&head, Opus_header, bytesRead);

    switch (result) {
        case 0:
            printf("Valid Opus stream.\n");
            return 0;

        case OP_FALSE:
            printf("Not enough data to determine if it's an Opus stream.\n");
            return OP_FALSE;

        case OP_EFAULT:
            printf("Internal memory allocation failed.\n");
            return OP_EFAULT;

        case OP_EIMPL:
            printf("Stream uses unsupported Opus features.\n");
            return OP_EIMPL;

        case OP_ENOTFORMAT:
            printf("File is not a recognizable Opus stream.\n");
            return OP_ENOTFORMAT;

        case OP_EVERSION:
            printf("Unsupported Opus stream version.\n");
            return OP_EVERSION;

        case OP_EBADHEADER:
            printf("Malformed or illegal Opus ID header.\n");
            return OP_EBADHEADER;

        default:
            printf("Unknown error while checking Opus stream: %d\n", result);
            return result;
    }
}



static bool rl_opus_init_file(const char *filename, rl_opus *pOpus) {
    if (!pOpus) return false;

    pOpus->file = op_open_file(filename, NULL);
    if (!pOpus->file) return false;

    pOpus->sampleRate = 48000; // Opus is always 48kHz
    pOpus->channels = op_channel_count(pOpus->file, -1);

    return true;
}

static void rl_opus_uninit(rl_opus *pOpus) {
    if (pOpus && pOpus->file) op_free(pOpus->file);
}

static uint64_t rl_opus_read_pcm_frames_f32(rl_opus *pOpus, uint64_t frameCount, float *pOut) {
    if (!pOpus || !pOut) return 0;

    int totalRead = 0;
    int channels = pOpus->channels;
    short buffer[4096 * 2];  // max stereo

    while (totalRead < frameCount) {
        int framesToRead = (int)(frameCount - totalRead);
        if (framesToRead > 4096) framesToRead = 4096;

        int read = op_read(pOpus->file, buffer, framesToRead * channels, NULL);
        if (read <= 0) break;

        // Convert to float
        for (int i = 0; i < read * channels; i++) {
            pOut[(totalRead * channels) + i] = buffer[i] / 32768.0f;
        }

        totalRead += read;
    }

    return totalRead;
}

static uint64_t rl_opus_get_total_pcm_frame_count(rl_opus *pOpus) {
    if (!pOpus || !pOpus->file) return 0;
    return op_pcm_total(pOpus->file, -1);
}

#endif 
