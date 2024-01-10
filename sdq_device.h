#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <crc.h>
#include <furi_hal_gpio.h>
#include <furi_hal.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t BREAK_meaningful_min;
    uint32_t BREAK_meaningful_max;
    uint32_t BREAK_meaningful;
    uint32_t BREAK_recovery;
    uint32_t WAKE_meaningful_min;
    uint32_t WAKE_meaningful_max;
    uint32_t WAKE_meaningful;
    uint32_t WAKE_recovery;
    uint32_t ZERO_meaningful_min;
    uint32_t ZERO_meaningful_max;
    uint32_t ZERO_meaningful;
    uint32_t ZERO_recovery;
    uint32_t ONE_meaningful_min;
    uint32_t ONE_meaningful_max;
    uint32_t ONE_meaningful;
    uint32_t ONE_recovery;
    uint32_t ZERO_STOP_recovery;
    uint32_t ONE_STOP_recovery;
} SDQTimings;

extern const SDQTimings sdq_timings;
typedef enum {
    SDQDeviceErrorNone = 0,
    SDQDeviceErrorResetInProgress,
    SDQDeviceErrorPresenceConflict,
    SDQDeviceErrorInvalidCommand,
    SDQDeviceErrorBitReadTiming,
    SDQDeviceErrorTimeout,
} SDQDeviceError;

typedef struct SDQDevice SDQDevice;

typedef bool (*SDQDeviceCommandCallback)(uint8_t* command, void* context);

struct SDQDevice* sdq_device_alloc(const GpioPin* gpio_pin);
void sdq_device_free(SDQDevice* bus);

void sdq_device_start(SDQDevice* bus);
void sdq_device_stop(SDQDevice* bus);

void sdq_device_set_command_callback(SDQDevice* bus, SDQDeviceCommandCallback callback, void* context);

bool sdq_device_send(SDQDevice* bus, const uint8_t data[], size_t data_size);
bool sdq_device_receive(SDQDevice* bus, uint8_t data[], size_t data_size);

#ifdef __cplusplus
}
#endif
