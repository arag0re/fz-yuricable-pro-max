#pragma once

#include <furi.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/scene_manager.h>
#include <gui/modules/widget.h>
#include <gui/modules/submenu.h>
#include <gui/modules/text_input.h>
#include <furi_hal_light.h>
#include "lib/sdq/sdq_device.c"

typedef enum { EventTypeKey } EventType;

typedef enum {
    YuriCableProMaxMainMenuScene,
    YuriCableProMaxDCSDScene,
    YuriCableProMaxResetScene,
    YuriCableProMaxDFUScene,
    YuriCableProMaxCharginScene,
    YuriCableProMaxSceneCount
} YuriCableProMaxScene;

typedef enum {
    YuriCableProMaxSubmenuView,
    YuriCableProMaxWidgetView,
} YuriCableProMaxView;

typedef enum {
    YuriCableProMaxMainMenuSceneDCSD,
    YuriCableProMaxMainMenuSceneReset,
    YuriCableProMaxMainMenuSceneDFU,
    YuriCableProMaxMainMenuSceneCharging,
} YuriCableProMaxMainMenuSceneIndex;

typedef struct {
    SDQDevice* sdq;
    IconAnimation* listeningAnimation;
    bool ledMainMenu;
    bool ledOff;
} YuriCableData;

typedef struct App {
    SceneManager* scene_manager;
    ViewDispatcher* view_dispatcher;
    Submenu* submenu;
    Widget* widget;
    FuriMessageQueue* queue;
    FuriMutex* mutex;
    YuriCableData* data;
    FuriThread* led_thread;
} App;

typedef enum {
    YuriCableProMaxMainMenuSceneDCSDModeEvent,
    YuriCableProMaxMainMenuSceneResetModeEvent,
    YuriCableProMaxMainMenuSceneDFUModeEvent,
    YuriCableProMaxMainMenuSceneChargingModeEvent
} YuriCableProMaxMainMenuSceneEvent;

typedef struct {
    EventType type; 
    InputEvent input;
} Event;

typedef enum {
    LedEvtStop = (1 << 0),
    LedEvtStart = (1 << 1),
} LedEvtFlags;