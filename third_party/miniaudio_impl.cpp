#include "miniaudio.h"
#include <iostream>

// Implementation of miniaudio mocks for the main application
ma_result ma_device_init(void* pContext, const ma_device_config* pConfig, ma_device* pDevice) {
    std::cout << "Mock Audio: Device Initialized." << std::endl;
    pDevice->pUserData = pConfig->pUserData;
    return MA_SUCCESS;
}

ma_result ma_device_start(ma_device* pDevice) {
    std::cout << "Mock Audio: Device Started." << std::endl;
    return MA_SUCCESS;
}

void ma_device_uninit(ma_device* pDevice) {
    std::cout << "Mock Audio: Device Uninitialized." << std::endl;
}