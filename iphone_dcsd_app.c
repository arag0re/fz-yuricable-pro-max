#include <furi.h>
#include <furi_hal.h>
#include <furi_hal_gpio.h>
#include <furi_hal_interrupt.h>
#include <furi_hal_resources.h>
#include <furi_hal_bus.h>
#include <stm32wbxx_ll_dma.h>
#include <furi_hal_cortex.h>
#include <stdio.h>

#define SDQ_PIN gpio_ibutton // GPIO shared with iButton
#define BUFFER_SIZE 4
#define BREAK_MIN_DURATION 12
#define BREAK_MAX_DURATION 16
#define WAKE_MIN_DURATION 22
#define WAKE_MAX_DURATION 27
#define ZERO_MIN_DURATION 6
#define ZERO_MAX_DURATION 8
#define ONE_MIN_DURATION 1
#define ONE_MAX_DURATION 2.5

// Global variables to store interrupt times
volatile uint32_t rise_time = 0;
volatile uint32_t fall_time = 0;

enum TRISTAR_REQUESTS {
    TRISTAR_POLL = 0x74,
    TRISTAR_POWER = 0x70,
    TRISTAR_UNKNOWN_76 = 0x76,
};

void delay_us(uint32_t us) {
    furi_delay_us(us);
}

void SDQ_Init_rx(void) {
    // Initialize the SDQ pin as an input
    furi_hal_gpio_init(&SDQ_PIN, GpioModeAnalog, GpioPullNo, GpioSpeedVeryHigh);
}

void SDQ_Init_tx(void) {
    // Initialize the SDQ pin as an output
    furi_hal_gpio_init_simple(&SDQ_PIN, GpioModeOutputPushPull);
}

bool SDQ_Read(void) {
    // Read the state of the SDQ pin
    return furi_hal_gpio_read(&SDQ_PIN);
}

unsigned char reverse_byte(unsigned char b) {
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

int32_t iphone_dcsd_app(void* p) {
    UNUSED(p);
    FURI_LOG_I("YURIAPP", "Starting the SDQ Listener on GPIO 2!");
    //setup_rx();
    while(1) {
        SDQ_Init_rx();
        bool bit = SDQ_Read();
        FURI_LOG_I("SDQ", "Bit: %d", bit);
        //FURI_LOG_I("SDQ_BUFFER", "0x%02X", received_byte);
        //if(received_byte == TRISTAR_POLL) {
        //    FURI_LOG_I("INFO", "Received TRISTART_POLL 0x%02X", received_byte);
        //} else if(received_byte == TRISTAR_POWER) {
        //    FURI_LOG_I("INFO", "Received TRISTART_POWER 0x%02X", received_byte);
        //} else if(received_byte == TRISTAR_UNKNOWN_76) {
        //    FURI_LOG_I("INFO", "Received TRISTART_UNKNOWN_76 0x%02X", received_byte);
        //} else {
        //    FURI_LOG_E("ERROR", "Tristar >> 0x%02X (unknown, ignoring)", received_byte);
        //}
    }
    return 0;
}
