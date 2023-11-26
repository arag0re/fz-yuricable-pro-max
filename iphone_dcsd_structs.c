#include <gui/gui.h>

typedef enum {
    EventTypeKey,
} EventType;

typedef struct {
    EventType type; // The reason for this event.
    InputEvent input; // This data is specific to keypress data.
} Event;
