#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

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

SDQTimings sdq_timings = { // microseconds
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
    SDQSlaveErrorNone = 0,
    SDQSlaveErrorResetInProgress,
    SDQSlaveErrorPresenceConflict,
    SDQSlaveErrorInvalidCommand,
    SDQSlaveErrorTimeout,
} SDQSlaveError;

typedef struct SDQDevice SDQDevice;
typedef struct SDQSlave SDQSlave;

typedef bool (*SDQSlaveResetCallback)(bool is_short, void* context);
typedef bool (*SDQSlaveCommandCallback)(uint8_t command, void* context);
typedef void (*SDQSlaveResultCallback)(void* context);

#ifdef __cplusplus
}
#endif