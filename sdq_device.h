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

enum TRISTAR_REQUESTS {
    TRISTAR_POLL = 0x74,
    TRISTAR_POWER = 0x70,
    TRISTAR_UNKNOWN_76 = 0x76,
};

typedef struct {
    uint8_t DFU[7];
    uint8_t RESET_DEVICE[7];
    uint8_t USB_UART_JTAG[7];
    uint8_t USB_SPAM_JTAG[7];
    uint8_t USB_UART[7];
    uint8_t USB_A_CHARGING_CABLE[7];
} TRISTART_RESPONSES;

typedef enum {
    SDQDeviceCommand_NONE = 0,
    SDQDeviceCommand_DFU,
    SDQDeviceCommand_RESET_DEVICE,
    SDQDeviceCommand_USB_UART_JTAG,
    SDQDeviceCommand_USB_SPAM_JTAG,
    SDQDeviceCommand_USB_UART,
    SDQDeviceCommand_USB_A_CHARGING_CABLE,
} SDQDeviceCommand;

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

typedef enum {
    SDQDeviceErrorNone = 0,
    SDQDeviceErrorResetInProgress,
    SDQDeviceErrorNotConnected,
    SDQDeviceErrorInvalidCommand,
    SDQDeviceErrorBitReadTiming,
    SDQDeviceErrorTimeout,
} SDQDeviceError;

typedef struct SDQDevice SDQDevice;

typedef bool (*SDQDeviceCommandCallback)(uint8_t* command, void* context);

struct SDQDevice {
    const GpioPin* gpio_pin;
    SDQTimings timings;
    SDQDeviceError error;
    SDQDeviceCommand runCommand;
    bool listening;
    bool connected;

    const FuriMessageQueue* eventQueue;

    SDQDeviceCommandCallback command_callback;
    void* command_callback_context;
};

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
