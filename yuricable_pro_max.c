#include <furi.h>
#include <furi_hal.h>
#include <furi_hal_gpio.h>
#include <furi_hal_interrupt.h>
#include <furi_hal_resources.h>
#include <furi_hal_bus.h>
#include <furi_hal_cortex.h>
#include <gui/gui.h>
#include <locale/locale.h>
#include <yuricable_pro_max_structs.c>
#define TAG "iphone_dcsd_app"
#define SDQ_PIN gpio_ext_pa7 // GPIO 2

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
    Event event = {.type = EventTypeKey, .input = *input_event};
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
}

int32_t yuricable_pro_max_app(void* p) {
    UNUSED(p);
    FURI_LOG_I("YURIAPP", "Starting the SDQ Listener on GPIO 2!");
    // Configure our initial data.
    DemoContext* demo_context = malloc(sizeof(DemoContext));
    demo_context->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    demo_context->data = malloc(sizeof(DemoData));
    demo_context->data->buffer = furi_string_alloc();
    // Queue for events (tick or input)
    demo_context->queue = furi_message_queue_alloc(8, sizeof(Event));
    //furi_hal_gpio_init(&SDQ_PIN, GpioModeInterruptRise, GpioPullDown, GpioSpeedVeryHigh);
    //furi_hal_gpio_add_int_callback(&SDQ_PIN, sdq_rise_interrupt_handler, NULL);
    // Set ViewPort callbacks
    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, demo_render_callback, demo_context);
    view_port_input_callback_set(view_port, demo_input_callback, demo_context->queue);

    // Open GUI and register view_port
    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    Event event;
    bool processing = true;
    do {
        if(furi_message_queue_get(demo_context->queue, &event, 1000) == FuriStatusOk) {
            FURI_LOG_T(TAG, "Got event type: %d", event.type);
            switch(event.type) {
            case EventTypeKey:
                // Short press of back button exits the program.
                if(event.input.type == InputTypeShort && event.input.key == InputKeyBack) {
                    FURI_LOG_I(TAG, "Short-Back pressed. Exiting program.");
                    processing = false;
                } else if(event.input.type == InputTypeShort && event.input.key == InputKeyOk) {
                    FURI_LOG_I("INFO", "Pressed Enter Key");
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
    return 0;
}