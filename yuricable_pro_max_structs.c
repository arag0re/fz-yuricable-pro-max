#include <gui/gui.h>

typedef enum {
    EventTypeKey,
} EventType;

typedef struct {
    EventType type; // The reason for this event.
    InputEvent input; // This data is specific to keypress data.
    // You can add additional data that is helpful for your events.
} Event;

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

typedef struct {
    uint8_t DFU[7];
    uint8_t RESET_DEVICE[7];
    uint8_t USB_UART_JTAG[7];
    uint8_t USB_SPAM_JTAG[7];
    uint8_t USB_UART[7];
    uint8_t USB_A_CHARGING_CABLE[7];
} TRISTART_RESPONSES;

