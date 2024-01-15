#include <yuricable_pro_max.h>
#include <yuricable_pro_max_icons.h>
#include <lib/dab/dap_v2_usb.h>

#define TAG "YURICABLE_PRO_MAX"
#define SDQ_PIN gpio_ext_pa7 // GPIO 2

static void yuricable_input_callback(InputEvent* input_event, FuriMessageQueue* queue) {
    furi_assert(queue);
    Event event = {.type = EventTypeKey, .input = *input_event};
    furi_message_queue_put(queue, &event, FuriWaitForever);
}

static void yuricable_render_callback(Canvas* canvas, void* ctx) {
    YuriCableContext* yuricable_context = ctx;
    if (furi_mutex_acquire(yuricable_context->mutex, 200) != FuriStatusOk) {
        return;
    }
    YuriCableData* data = yuricable_context->data;
    UNUSED(data);

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 4, 13, "YuriCable Pro Max");

    if (yuricable_context->data->sdq->listening) {
        IconAnimation* animation = icon_animation_alloc(&A_Round_loader_8x8);
        canvas_draw_icon_animation(canvas, 115, 4, animation);
        canvas_draw_box(canvas, 115, 4, 8, 8);
    } else {
        canvas_draw_icon(canvas, 115, 4, &I_Round_empty_8x8);
    }

    canvas_draw_line(canvas, 0, 16, 127, 16);
    canvas_draw_line(canvas, 0, 28, 127, 28);
    canvas_draw_line(canvas, 0, 40, 127, 40);
    canvas_draw_line(canvas, 0, 52, 127, 52);

    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 4, 26, "DCSD - Bootlog");
    canvas_draw_str(canvas, 4, 38, "JTAG / SWD");
    canvas_draw_str(canvas, 4, 50, "DFU");
    canvas_draw_str(canvas, 4, 61, "Recovery");

    canvas_draw_icon(canvas, 120, 7 + 12 * yuricable_context->data->sdq->runCommand, &I_ButtonLeft_4x7);

    furi_mutex_release(yuricable_context->mutex);
}

int32_t yuricable_pro_max_app(void* p) {
    UNUSED(p);
    FURI_LOG_I(TAG, "Starting the SDQ Listener on GPIO 2!");
    // Configure our initial data.
    YuriCableContext* yuricable_context = malloc(sizeof(YuriCableContext));
    yuricable_context->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    yuricable_context->data = malloc(sizeof(YuriCableData));

    // Queue for events (tick or input)
    yuricable_context->queue = furi_message_queue_alloc(8, sizeof(Event));

    UsbUartConfig bridgeConfig = {.vcp_ch = 1, .uart_ch = 0, .baudrate_mode = 0, .baudrate = 115200};
    UsbUartBridge* uartBridge = usb_uart_enable(&bridgeConfig);

    yuricable_context->data->sdq = sdq_device_alloc(&SDQ_PIN, uartBridge);
    yuricable_context->data->sdq->runCommand = SDQDeviceCommand_USB_A_CHARGING_CABLE;

    //  Set ViewPort callbacks
    ViewPort* view_port = view_port_alloc();

    view_port_draw_callback_set(view_port, yuricable_render_callback, yuricable_context);
    view_port_input_callback_set(view_port, yuricable_input_callback, yuricable_context->queue);

    // Open GUI and register view_port
    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    Event event;
    bool processing = true;
    do {
        if (furi_message_queue_get(yuricable_context->queue, &event, 1000) == FuriStatusOk) {
            FURI_LOG_T(TAG, "Got event type: %d", event.type);
            switch (event.type) {
            case EventTypeKey:
                // Short press of back button exits the program.
                if (event.input.type == InputTypeShort && event.input.key == InputKeyBack) {
                    FURI_LOG_I(TAG, "Short-Back pressed. Exiting program.");
                    processing = false;
                } else if (event.input.type == InputTypeShort && event.input.key == InputKeyOk) {
                    FURI_LOG_I(TAG, "Pressed Enter Key");
                    if (!yuricable_context->data->sdq->listening) {
                        sdq_device_start(yuricable_context->data->sdq);
                        view_port_update(view_port);
                        FURI_LOG_I(TAG, "SDQ Device started");
                    } else {
                        sdq_device_stop(yuricable_context->data->sdq);
                        view_port_update(view_port);
                        FURI_LOG_I(TAG, "SDQ Device stopped");
                    }
                } else if (event.input.type == InputTypeShort && event.input.key == InputKeyUp) {
                    if (yuricable_context->data->sdq->runCommand > 1) {
                        yuricable_context->data->sdq->runCommand--;
                        view_port_update(view_port);
                    }
                } else if (event.input.type == InputTypeShort && event.input.key == InputKeyDown) {
                    if (yuricable_context->data->sdq->runCommand < SDQDeviceCommand_RECOVERY) {
                        yuricable_context->data->sdq->runCommand++;
                        view_port_update(view_port);
                    }
                }
                break;
            default:
                break;
            }
        }
    } while (processing);

    usb_uart_disable(uartBridge);
    free(uartBridge);

    sdq_device_free(yuricable_context->data->sdq);
    free(yuricable_context);

    furi_hal_gpio_init(&SDQ_PIN, GpioModeOutputOpenDrain, GpioPullNo, GpioSpeedLow);
    furi_hal_gpio_write(&SDQ_PIN, true);

    view_port_enabled_set(view_port, false);
    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_record_close(RECORD_GUI);
    return 0;
}
