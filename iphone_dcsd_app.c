#include <furi.h>
#include <furi_hal.h>
#include <furi_hal_gpio.h>
#include <furi_hal_interrupt.h>
#include <furi_hal_resources.h>
#include <furi_hal_bus.h>
#include <stm32wbxx_ll_dma.h>
#include <furi_hal_cortex.h>

#define SDQ_PIN gpio_ext_pc3 // GPIO 2
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
    furi_hal_gpio_init(&SDQ_PIN, GpioModeInput, GpioPullNo, GpioSpeedVeryHigh);
    furi_hal_bus_enable(FuriHalBusTIM17);
    // Configure TIM
    LL_TIM_SetPrescaler(TIM17, SystemCoreClock / 1000000 - 1);
    LL_TIM_SetCounterMode(TIM17, LL_TIM_COUNTERMODE_UP);
    LL_TIM_SetAutoReload(TIM17, 0xFFFF);
    LL_TIM_SetClockDivision(TIM17, LL_TIM_CLOCKDIVISION_DIV1);
}

void SDQ_Init_tx(void) {
    // Initialize the SDQ pin as an output
    furi_hal_gpio_init_simple(&SDQ_PIN, GpioModeOutputPushPull);
}

void IntCallBackTest() {
    FURI_LOG_I("INFO", "Hello From Callback");
}

void setup() {
    furi_hal_gpio_init(&SDQ_PIN, GpioModeInterruptFall, GpioPullUp, GpioSpeedVeryHigh);
    furi_hal_gpio_add_int_callback(&SDQ_PIN, IntCallBackTest, NULL);
    //furi_hal_gpio_enable_int_callback(&SDQ_PIN);
}

void SDQ_Read_Byte(void) {
    // Read the state of the SDQ pin
    LL_TIM_SetCounter(TIM17, 0);
    while(furi_hal_gpio_read(&SDQ_PIN)) {
        if(!furi_hal_gpio_read(&SDQ_PIN)) {
            LL_TIM_EnableCounter(TIM17);
            break;
        }
    }
    uint32_t timer;
    while(!furi_hal_gpio_read(&SDQ_PIN)) {
        if(furi_hal_gpio_read(&SDQ_PIN)) {
            timer = LL_TIM_GetCounter(TIM17);
            FURI_LOG_I("INFO", "SDQ bit: %lu", timer);
            break;
        }
    }
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
    //SDQ_Init_rx();
    setup();
    while(1) {
        //SDQ_Read_Byte();
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
