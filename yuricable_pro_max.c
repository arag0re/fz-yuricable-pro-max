#include <yuricable_pro_max.h>
#include <yuricable_pro_max_icons.h>

#define TAG "YURICABLE_PRO_MAX"
#define SDQ_PIN gpio_ext_pa7 // GPIO 2

FuriString* yuricable_command_callback(char* command, void* ctx) {
    App* yuricable_context = ctx;
    UNUSED(yuricable_context);
    if(strcmp(command, "start") == 0) {
        if(yuricable_context->data->sdq->listening) {
            return furi_string_alloc_printf("already listening");
        }
        sdq_device_start(yuricable_context->data->sdq);
        icon_animation_start(yuricable_context->data->listeningAnimation);
        return furi_string_alloc_printf("started");
    }
    if(strcmp(command, "stop") == 0) {
        if(!yuricable_context->data->sdq->listening) {
            return furi_string_alloc_printf("already stopped");
        }
        sdq_device_stop(yuricable_context->data->sdq);
        icon_animation_stop(yuricable_context->data->listeningAnimation);
        return furi_string_alloc_printf("stopped");
    }
    if(strncmp(command, "mode", 4) == 0) {
        if(command[4] == ' ') {
            char* mode = command + 5;
            if(strcmp(mode, "dfu") == 0) {
                yuricable_context->data->sdq->runCommand = SDQDeviceCommand_DFU;
                yuricable_context->data->sdq->commandExecuted = false;
                return furi_string_alloc_printf("set mode dfu");
            }
            if(strcmp(mode, "reset") == 0) {
                yuricable_context->data->sdq->runCommand = SDQDeviceCommand_RESET;
                yuricable_context->data->sdq->commandExecuted = false;
                return furi_string_alloc_printf("set mode reset");
            }
            if(strcmp(mode, "dcsd") == 0) {
                yuricable_context->data->sdq->runCommand = SDQDeviceCommand_DCSD;
                yuricable_context->data->sdq->commandExecuted = false;
                return furi_string_alloc_printf("set mode dcsd");
            }
        }
        return furi_string_alloc_printf("use: /mode <dfu | reset | dcsd>");
    }
    if(strncmp(command, "help", 4) == 0) {
        return furi_string_alloc_printf(
            "commands:\r\n/start\r\n/stop\r\n/mode <dfu | reset | dcsd>");
    }
    return furi_string_alloc_printf("%s is no valid command", command);
}

// Implement the LED sequence worker thread function
static int32_t led_sequence_worker(void* ctx) {
    App* app = (App*)ctx;
    while(1) {
        uint32_t events = furi_thread_flags_get();
        if(events & WorkerEvtStop) {
            break;
        }
        if(app->data->sdq->listening) {
            furi_hal_light_sequence("rgb B.b.B");
        } else if(app->data->sdq->commandExecuted) {
            furi_hal_light_set(LightRed, 0);
            furi_hal_light_set(LightGreen, 255);
            furi_hal_light_set(LightBlue, 0);
        } else if(app->data->ledMainMenu) {
            furi_hal_light_set(LightRed, 0);
            furi_hal_light_set(LightGreen, 0);
            furi_hal_light_set(LightBlue, 255);
        } else if(app->data->sdq->error) {
            furi_hal_light_set(LightRed, 255);
            furi_hal_light_set(LightGreen, 0);
            furi_hal_light_set(LightBlue, 0);
        } else if(app->data->ledOff) {
            furi_hal_light_set(LightRed, 0);
            furi_hal_light_set(LightBlue, 0);
            furi_hal_light_set(LightGreen, 0);
        }
    }
    return 0;
}

void yuricable_pro_max_menu_callback(void* context, uint32_t index) {
    App* app = context;
    switch(index) {
    case YuriCableProMaxMainMenuSceneDCSD:
        scene_manager_handle_custom_event(
            app->scene_manager, YuriCableProMaxMainMenuSceneDCSDModeEvent);
        break;
    case YuriCableProMaxMainMenuSceneReset:
        scene_manager_handle_custom_event(
            app->scene_manager, YuriCableProMaxMainMenuSceneResetModeEvent);
        break;
    case YuriCableProMaxMainMenuSceneDFU:
        scene_manager_handle_custom_event(
            app->scene_manager, YuriCableProMaxMainMenuSceneDFUModeEvent);
        break;
    case YuriCableProMaxMainMenuSceneCharging:
        scene_manager_handle_custom_event(
            app->scene_manager, YuriCableProMaxMainMenuSceneChargingModeEvent);
        break;
    }
}

void yuricable_pro_max_main_menu_scene_on_enter(void* context) {
    App* app = context;
    submenu_reset(app->submenu);
    submenu_set_header(app->submenu, "YuriCable Pro Max");
    submenu_add_item(
        app->submenu,
        "DCSD",
        YuriCableProMaxMainMenuSceneDCSD,
        yuricable_pro_max_menu_callback,
        app);
    submenu_add_item(
        app->submenu,
        "Force Reset",
        YuriCableProMaxMainMenuSceneReset,
        yuricable_pro_max_menu_callback,
        app);
    submenu_add_item(
        app->submenu,
        "Force DFU Mode",
        YuriCableProMaxMainMenuSceneDFU,
        yuricable_pro_max_menu_callback,
        app);
    submenu_add_item(
        app->submenu,
        "5V Charging",
        YuriCableProMaxMainMenuSceneCharging,
        yuricable_pro_max_menu_callback,
        app);
    view_dispatcher_switch_to_view(app->view_dispatcher, YuriCableProMaxSubmenuView);
}

bool yuricable_pro_max_main_menu_scene_on_event(void* context, SceneManagerEvent event) {
    App* app = context;
    bool consumed = false;
    switch(event.type) {
    case SceneManagerEventTypeCustom:
        switch(event.event) {
        case YuriCableProMaxMainMenuSceneDCSDModeEvent:
            scene_manager_next_scene(app->scene_manager, YuriCableProMaxDCSDScene);
            consumed = true;
            break;
        case YuriCableProMaxMainMenuSceneResetModeEvent:
            scene_manager_next_scene(app->scene_manager, YuriCableProMaxResetScene);
            consumed = true;
            break;
        case YuriCableProMaxMainMenuSceneDFUModeEvent:
            scene_manager_next_scene(app->scene_manager, YuriCableProMaxDFUScene);
            consumed = true;
            break;
        case YuriCableProMaxMainMenuSceneChargingModeEvent:
            scene_manager_next_scene(app->scene_manager, YuriCableProMaxCharginScene);
            consumed = true;
            break;
        }
        break;
    default:
        break;
    }
    return consumed;
}

void yuricable_pro_max_toggle_sdq_button_callback(
    GuiButtonType button_type,
    InputType input_type,
    void* context) {
    UNUSED(button_type);
    UNUSED(input_type);
    YuriCableData* data = (YuriCableData*)context;
    if(data->sdq->listening) {
        sdq_device_stop(data->sdq);
        furi_hal_power_disable_otg();
    } else {
        sdq_device_start(data->sdq);
        furi_hal_power_enable_otg();
    }
}

void yuricable_pro_max_dcsd_scene_on_enter(void* context) {
    App* app = context;
    widget_reset(app->widget);
    widget_add_string_element(app->widget, 25, 15, AlignLeft, AlignCenter, FontPrimary, "DCSD");
    app->data->sdq->runCommand = SDQDeviceCommand_DCSD;
    app->data->ledMainMenu = false;
    sdq_device_start(app->data->sdq);
    view_dispatcher_switch_to_view(app->view_dispatcher, YuriCableProMaxWidgetView);
}

void yuricable_pro_max_reset_scene_on_enter(void* context) {
    App* app = context;
    widget_reset(app->widget);
    widget_add_string_element(
        app->widget, 25, 15, AlignLeft, AlignCenter, FontPrimary, "Force Reset");
    app->data->sdq->runCommand = SDQDeviceCommand_RESET;
    app->data->ledMainMenu = false;
    sdq_device_start(app->data->sdq);
    view_dispatcher_switch_to_view(app->view_dispatcher, YuriCableProMaxWidgetView);
}

void yuricable_pro_max_dfu_scene_on_enter(void* context) {
    App* app = context;
    widget_reset(app->widget);
    widget_add_string_element(
        app->widget, 25, 15, AlignLeft, AlignCenter, FontPrimary, "Force DFU Mode");
    app->data->ledMainMenu = false;
    app->data->sdq->runCommand = SDQDeviceCommand_DFU;
    sdq_device_start(app->data->sdq);
    view_dispatcher_switch_to_view(app->view_dispatcher, YuriCableProMaxWidgetView);
}

void yuricable_pro_max_charging_scene_on_enter(void* context) {
    App* app = context;
    widget_reset(app->widget);
    widget_add_string_element(
        app->widget, 25, 15, AlignLeft, AlignCenter, FontPrimary, "5V Charging");
    app->data->ledMainMenu = false;
    app->data->sdq->runCommand = SDQDeviceCommand_CHARGING;
    sdq_device_start(app->data->sdq);
    view_dispatcher_switch_to_view(app->view_dispatcher, YuriCableProMaxWidgetView);
}

bool yuricable_pro_max_sdq_scene_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false; // event not handled.
}

void yuricable_pro_max_sdq_scene_on_exit(void* context) {
    App* app = context;
    if(app->data->sdq->listening) {
        sdq_device_stop(app->data->sdq);
    }
    app->data->sdq->commandExecuted = false;
    app->data->ledMainMenu = true;
    UNUSED(context);
}

void yuricable_pro_max_main_menu_scene_on_exit(void* context) {
    App* app = context;
    submenu_reset(app->submenu);
}

void (*const yuricable_pro_max_scene_on_enter_handlers[])(void*) = {
    yuricable_pro_max_main_menu_scene_on_enter,
    yuricable_pro_max_dcsd_scene_on_enter,
    yuricable_pro_max_reset_scene_on_enter,
    yuricable_pro_max_dfu_scene_on_enter,
    yuricable_pro_max_charging_scene_on_enter};

bool (*const yuricable_pro_max_scene_on_event_handlers[])(void*, SceneManagerEvent) = {
    yuricable_pro_max_main_menu_scene_on_event,
    yuricable_pro_max_sdq_scene_on_event,
    yuricable_pro_max_sdq_scene_on_event,
    yuricable_pro_max_sdq_scene_on_event,
    yuricable_pro_max_sdq_scene_on_event};

void (*const yuricable_pro_max_scene_on_exit_handlers[])(void*) = {
    yuricable_pro_max_main_menu_scene_on_exit,
    yuricable_pro_max_sdq_scene_on_exit,
    yuricable_pro_max_sdq_scene_on_exit,
    yuricable_pro_max_sdq_scene_on_exit,
    yuricable_pro_max_sdq_scene_on_exit};

static const SceneManagerHandlers yuricable_pro_max_scene_manager_handlers = {
    .on_enter_handlers = yuricable_pro_max_scene_on_enter_handlers,
    .on_event_handlers = yuricable_pro_max_scene_on_event_handlers,
    .on_exit_handlers = yuricable_pro_max_scene_on_exit_handlers,
    .scene_num = YuriCableProMaxSceneCount,
};

static bool yuricable_pro_max_custom_callback(void* context, uint32_t custom_event) {
    furi_assert(context);
    App* app = context;
    return scene_manager_handle_custom_event(app->scene_manager, custom_event);
}

bool yuricable_pro_max_back_event_callback(void* context) {
    furi_assert(context);
    App* app = context;
    return scene_manager_handle_back_event(app->scene_manager);
}

App* app_alloc() {
    App* app = malloc(sizeof(App));
    // Initialize LED Sequence
    app->led_thread = furi_thread_alloc_ex("LedSequenceWorker", 1024, led_sequence_worker, app);
    // Initialize YuriCableContext
    app->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    app->data = malloc(sizeof(YuriCableData));
    app->data->listeningAnimation = icon_animation_alloc(&A_Round_loader_8x8);
    app->data->ledMainMenu = true;
    app->data->ledOff = false;
    app->queue = furi_message_queue_alloc(8, sizeof(Event));
    // Initialize USB UART bridge
    UsbUartConfig bridgeConfig = {
        .vcp_ch = 1, .uart_ch = 0, .baudrate_mode = 0, .baudrate = 115200};
    UsbUartBridge* uartBridge = usb_uart_enable(&bridgeConfig);
    usb_uart_set_command_callback(uartBridge, yuricable_command_callback, app);

    // Initialize SDQ
    app->data->sdq = sdq_device_alloc(&SDQ_PIN, uartBridge);
    app->data->sdq->runCommand = SDQDeviceCommand_DCSD;

    //Initialize SceneManager
    app->scene_manager = scene_manager_alloc(&yuricable_pro_max_scene_manager_handlers, app);
    app->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_enable_queue(app->view_dispatcher);
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_custom_event_callback(
        app->view_dispatcher, yuricable_pro_max_custom_callback);
    view_dispatcher_set_navigation_event_callback(
        app->view_dispatcher, yuricable_pro_max_back_event_callback);
    app->submenu = submenu_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, YuriCableProMaxSubmenuView, submenu_get_view(app->submenu));
    app->widget = widget_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, YuriCableProMaxWidgetView, widget_get_view(app->widget));

    return app;
}

void app_free(App* app) {
    furi_assert(app);
    app->data->ledOff = true;
    furi_thread_join(app->led_thread);
    furi_thread_free(app->led_thread);
    view_dispatcher_remove_view(app->view_dispatcher, YuriCableProMaxSubmenuView);
    view_dispatcher_remove_view(app->view_dispatcher, YuriCableProMaxWidgetView);
    scene_manager_free(app->scene_manager);
    view_dispatcher_free(app->view_dispatcher);
    submenu_free(app->submenu);
    widget_free(app->widget);
    sdq_device_free(app->data->sdq);
    icon_animation_free(app->data->listeningAnimation);
    free(app->data);
    furi_mutex_free(app->mutex);
    furi_message_queue_free(app->queue);
    furi_record_close(RECORD_GUI);
    free(app);
}

int32_t yuricable_pro_max_app(void* p) {
    UNUSED(p);
    FURI_LOG_I(TAG, "Starting YuriCable Pro Max");
    App* app = app_alloc();
    furi_thread_start(app->led_thread);
    Gui* gui = furi_record_open(RECORD_GUI);
    view_dispatcher_attach_to_gui(app->view_dispatcher, gui, ViewDispatcherTypeFullscreen);
    scene_manager_next_scene(app->scene_manager, YuriCableProMaxMainMenuScene);
    view_dispatcher_run(app->view_dispatcher);

    app_free(app);
    return 0;
}
