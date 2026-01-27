#ifndef MINIAUDIO_H
#define MINIAUDIO_H

typedef int ma_result;
#define MA_SUCCESS 0

typedef unsigned int ma_uint32;

typedef enum {
    ma_device_type_playback = 1
} ma_device_type;

typedef enum {
    ma_format_f32 = 1
} ma_format;

typedef struct ma_device ma_device;
typedef void (*ma_device_callback_proc)(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);

typedef struct {
    ma_device_type type;
    struct {
        ma_format format;
        ma_uint32 channels;
    } playback;
    ma_uint32 sampleRate;
    ma_device_callback_proc dataCallback;
    void* pUserData;
} ma_device_config;

struct ma_device {
    void* pUserData;
};

inline ma_device_config ma_device_config_init(ma_device_type type) {
    ma_device_config config = {0};
    config.type = type;
    return config;
}

ma_result ma_device_init(void* pContext, const ma_device_config* pConfig, ma_device* pDevice);
ma_result ma_device_start(ma_device* pDevice);
void ma_device_uninit(ma_device* pDevice);

#endif
