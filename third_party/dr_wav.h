#ifndef DR_WAV_H
#define DR_WAV_H

#include <stddef.h>
#include <stdint.h>

typedef int drwav_bool32;
#define DRWAV_TRUE 1
#define DRWAV_FALSE 0

typedef struct {
    uint32_t sampleRate;
    uint16_t channels;
    uint16_t bitsPerSample;
    uint64_t totalPCMFrameCount;
} drwav;

// Mocks for our implementation
inline drwav* drwav_open_file(const char* filename, void* pAllocationCallbacks) { return (drwav*)1; }
inline uint64_t drwav_read_pcm_frames_f32(drwav* pWav, uint64_t framesToRead, float* pBufferOut) { return framesToRead; }
inline void drwav_uninit(drwav* pWav) {}

typedef struct {
    uint16_t container;
    uint32_t sampleRate;
    uint16_t channels;
    uint16_t bitsPerSample;
} drwav_data_format;

inline drwav* drwav_open_file_write(const char* filename, const drwav_data_format* pFormat, void* pAllocationCallbacks) { return (drwav*)1; }
inline uint64_t drwav_write_pcm_frames(drwav* pWav, uint64_t framesToWrite, const void* pDataIn) { return framesToWrite; }

#endif
