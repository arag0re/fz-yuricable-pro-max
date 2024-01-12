#include <gui/gui.h>
#include <usb_uart_bridge.h>
#include <sdq_device.h>

typedef enum { EventTypeKey, EventTypeUpdateGUI } EventType;

typedef struct {
    EventType type; // The reason for this event.
    InputEvent input; // This data is specific to keypress data.
    // You can add additional data that is helpful for your events.
} Event;

typedef struct {
    SDQDevice* sdq;
    // You can add additional state here.
} YuriCableData;

typedef struct {
    FuriMessageQueue* queue; // Message queue (DemoEvent items to process).
    FuriMutex* mutex; // Used to provide thread safe access to data.
    YuriCableData* data; // Data accessed by multiple threads (acquire the mutex before accessing!)
} YuriCableContext;
