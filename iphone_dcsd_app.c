#include <furi.h>
#include <furi_hal.h>
#include <furi_hal_gpio.h>
#include <furi_hal_interrupt.h>
#include <furi_hal_resources.h>
#include <furi_hal_bus.h>
#include <furi_hal_cortex.h>
#include <gui/gui.h>
#include <locale/locale.h>
#include <iphone_dcsd_structs.c>

#define TAG "iphone_dcsd_app"
#define SDQ_PIN gpio_ext_pa7 // GPIO 2

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

SDQTimings timings = { // microseconds
    .BREAK_meaningful_min = 12,
    .BREAK_meaningful_max = 16,
    .BREAK_meaningful = 14,
    .BREAK_recovery = 4,
    .WAKE_meaningful_min = 22,
    .WAKE_meaningful_max = 27,
    .WAKE_meaningful = 24,
    .WAKE_recovery = 1100,
    .ZERO_meaningful_min = 6,
    .ZERO_meaningful_max = 8,
    .ZERO_meaningful = 7,
    .ZERO_recovery = 3,
    .ONE_meaningful_min = 1,
    .ONE_meaningful_max = 3,
    .ONE_meaningful = 2,
    .ONE_recovery = 8,
    .ZERO_STOP_recovery = 16,
    .ONE_STOP_recovery = 21};

typedef enum {
    DemoEventTypeKey,
} DemoEventType;

typedef struct {
    DemoEventType type; // The reason for this event.
    InputEvent input; // This data is specific to keypress data.
    // You can add additional data that is helpful for your events.
} DemoEvent;

typedef struct {
    FuriString* buffer;
    // You can add additional state here.
} DemoData;

typedef struct {
    FuriMessageQueue* queue; // Message queue (DemoEvent items to process).
    FuriMutex* mutex; // Used to provide thread safe access to data.
    DemoData* data; // Data accessed by multiple threads (acquire the mutex before accessing!)
} DemoContext;

enum TRISTAR_REQUESTS {
    TRISTAR_POLL = 0x74,
    TRISTAR_POWER = 0x70,
    TRISTAR_UNKNOWN_76 = 0x76,
};

static bool sdq_slave_wait_while_gpio_is(GpioPin* pin, uint32_t time_us, const bool pin_value) {
    const uint32_t time_start = DWT->CYCCNT;
    const uint32_t time_ticks = time_us * furi_hal_cortex_instructions_per_microsecond();
    uint32_t time_elapsed;
    do { //-V1044
        time_elapsed = DWT->CYCCNT - time_start;
        if(furi_hal_gpio_read(pin) != pin_value) {
            return time_ticks >= time_elapsed;
        }
    } while(time_elapsed < time_ticks);
    return false;
}

void init_timer_rx() {
    furi_hal_bus_enable(FuriHalBusTIM17);
    LL_TIM_SetPrescaler(TIM17, SystemCoreClock / 10000000 - 1);
    LL_TIM_SetCounterMode(TIM17, LL_TIM_COUNTERMODE_UP);
    LL_TIM_SetAutoReload(TIM17, 0xFFFF);
    LL_TIM_SetClockDivision(TIM17, LL_TIM_CLOCKDIVISION_DIV1);
}

void init_rx(void) {
    furi_hal_gpio_init(&SDQ_PIN, GpioModeInput, GpioPullNo, GpioSpeedVeryHigh);
    init_timer_rx();
}

void init_tx(void) {
    furi_hal_gpio_init(&SDQ_PIN, GpioModeOutputPushPull, GpioPullNo, GpioSpeedVeryHigh);
    furi_hal_gpio_write(&SDQ_PIN, false);
}

void sdq_read_word(void) {
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

void sdq_write_word(uint32_t meaningful, uint32_t recovery) {
    furi_hal_gpio_write(&SDQ_PIN, false);
    furi_delay_us(meaningful);
    furi_hal_gpio_write(&SDQ_PIN, true);
    furi_delay_us(recovery);
}

void sdq_write_byte(uint8_t data) {
    for(int i = 0; i < 8; i++) {
        uint8_t bit = (data >> i) & 0x01;
        uint32_t meaningful, recovery;
        meaningful = timings.ZERO_meaningful;
        recovery = timings.ZERO_recovery;
        if(i == 7) {
            meaningful = (bit == 0) ? timings.ZERO_meaningful : timings.ONE_meaningful;
            recovery = (bit == 0) ? timings.ZERO_STOP_recovery : timings.ONE_STOP_recovery;
        } else {
            meaningful = (bit == 0) ? timings.ZERO_meaningful : timings.ONE_meaningful;
            recovery = (bit == 0) ? timings.ZERO_recovery : timings.ONE_recovery;
        }
        sdq_write_word(meaningful, recovery);
    }
}

void sdq_write(const uint8_t data[], size_t size) {
    sdq_write_word(timings.BREAK_meaningful, timings.BREAK_recovery);
    for(size_t byteIndex = 0; byteIndex < size; byteIndex++) {
        sdq_write_byte(data[byteIndex]);
    }
    sdq_write_word(timings.BREAK_meaningful, timings.BREAK_recovery);
}

static void demo_input_callback(InputEvent* input_event, FuriMessageQueue* queue) {
    furi_assert(queue);
    DemoEvent event = {.type = DemoEventTypeKey, .input = *input_event};
    furi_message_queue_put(queue, &event, FuriWaitForever);
}

static void demo_render_callback(Canvas* canvas, void* ctx) {
    DemoContext* demo_context = ctx;
    if(furi_mutex_acquire(demo_context->mutex, 200) != FuriStatusOk) {
        return;
    }
    DemoData* data = demo_context->data;
    furi_string_printf(data->buffer, "yay");
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(
        canvas, 10, 20, AlignLeft, AlignTop, furi_string_get_cstr(data->buffer));
    furi_mutex_release(demo_context->mutex);
}

static void sdq_rise_interrupt_handler(FuriMessageQueue* queue) {
    UNUSED(queue);
    furi_hal_gpio_remove_int_callback(&SDQ_PIN);
    furi_hal_gpio_init(&SDQ_PIN, GpioModeInput, GpioPullNo, GpioSpeedVeryHigh);
}

int32_t iphone_dcsd_app(void* p) {
    UNUSED(p);
    FURI_LOG_I("YURIAPP", "Starting the SDQ Listener on GPIO 2!");
    // Configure our initial data.
    DemoContext* demo_context = malloc(sizeof(DemoContext));
    demo_context->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    demo_context->data = malloc(sizeof(DemoData));
    demo_context->data->buffer = furi_string_alloc();
    // Queue for events (tick or input)
    demo_context->queue = furi_message_queue_alloc(8, sizeof(DemoEvent));
    init_rx();
    //furi_hal_gpio_init(&SDQ_PIN, GpioModeInterruptRise, GpioPullDown, GpioSpeedVeryHigh);
    //furi_hal_gpio_add_int_callback(&SDQ_PIN, sdq_rise_interrupt_handler, NULL);
    // Set ViewPort callbacks
    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, demo_render_callback, demo_context);
    view_port_input_callback_set(view_port, demo_input_callback, demo_context->queue);

    // Open GUI and register view_port
    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    DemoEvent event;
    bool processing = true;
    do {
        if(furi_message_queue_get(demo_context->queue, &event, 1000) == FuriStatusOk) {
            FURI_LOG_T(TAG, "Got event type: %d", event.type);
            switch(event.type) {
            case DemoEventTypeKey:
                // Short press of back button exits the program.
                if(event.input.type == InputTypeShort && event.input.key == InputKeyBack) {
                    FURI_LOG_I(TAG, "Short-Back pressed. Exiting program.");
                    processing = false;
                } else if(event.input.type == InputTypeShort && event.input.key == InputKeyOk) {
                    FURI_LOG_I("INFO", "Pressed Enter Key");
                    while(1) {
                    }
                    //furi_hal_gpio_write(&SDQ_PIN, true);
                    //uint8_t data[] = {0x74, 0x00, 0x02, 0x1f};
                    //sdq_write(data, 4);
                }
                break;
            default:
                break;
            }
        }
    } while(processing);
    // furi_hal_gpio_remove_int_callback(&SDQ_PIN);

    furi_hal_gpio_init(&SDQ_PIN, GpioModeOutputOpenDrain, GpioPullNo, GpioSpeedLow);
    furi_hal_gpio_write(&SDQ_PIN, true);
    view_port_enabled_set(view_port, false);
    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_record_close(RECORD_GUI);

    //while(1) {
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
    //}
    return 0;
}