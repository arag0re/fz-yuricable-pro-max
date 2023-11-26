#include <furi.h>
#include <furi_hal.h>
#include <sdq_slave.h>

#define TH_TIMEOUT_MAX 15000 /* Maximum time before general timeout */

typedef struct {
    const GpioPin* gpio_pin;
    SDQTimings sdq_timings;
    SDQSlaveError error;
    bool is_first_reset;
    bool is_short_reset;
    SDQSlaveResetCallback reset_callback;
    SDQSlaveCommandCallback command_callback;
    SDQSlaveResultCallback result_callback;
    void* reset_callback_context;
    void* result_callback_context;
    void* command_callback_context;
} SDQSlave;

static bool
    onewire_slave_wait_while_gpio_is(SDQSlave* bus, uint32_t time_us, const bool pin_value) {
    const uint32_t time_start = DWT->CYCCNT;
    const uint32_t time_ticks = time_us * furi_hal_cortex_instructions_per_microsecond();
    uint32_t time_elapsed;
    do { //-V1044
        time_elapsed = DWT->CYCCNT - time_start;
        if(furi_hal_gpio_read(bus->gpio_pin) != pin_value) {
            return time_ticks >= time_elapsed;
        }
    } while(time_elapsed < time_ticks);
    return false;
}

static inline bool sdq_slave_receive_and_process_command(SDQSlave* sdq_slave) {
    /* Reset condition detected, send a presence pulse and reset protocol state */
    if(sdq_slave->error == SDQSlaveErrorResetInProgress) {
        if(!sdq_slave->is_first_reset) {
            /* Guess the reset type */
            sdq_slave->is_short_reset = sdq_slave_wait_while_gpio_is(
                sdq_slave,
                sdq_slave->sdq_timings.BREAK_meaningful_max -
                    sdq_slave->sdq_timings.BREAK_meaningful_min,
                false);
        } else {
            sdq_slave->is_first_reset = false;
        }

        // Replace with the correct assertion function in your environment
        furi_assert(sdq_slave->reset_callback);

        if(sdq_slave->reset_callback(
               sdq_slave->is_short_reset, sdq_slave->reset_callback_context)) {
            if(sdq_slave_show_presence(sdq_slave)) {
                sdq_slave->error = SDQSlaveErrorNone;
                return true;
            }
        }
    } else if(sdq_slave->error == SDQSlaveErrorNone) {
        uint8_t command;
        if(sdq_slave_receive(sdq_slave, &command, sizeof(command))) {
            // Replace with the correct assertion function in your environment
            furi_assert(sdq_slave->command_callback);

            if(sdq_slave->command_callback(command, sdq_slave->command_callback_context)) {
                return true;
            }
        }

        return (sdq_slave->error == SDQSlaveErrorResetInProgress);
    }

    return false;
}

static inline bool sdq_slave_bus_start(SDQSlave* sdq_slave) {
    FURI_CRITICAL_ENTER();
    furi_hal_gpio_init(sdq_slave->gpio_pin, GpioModeOutputOpenDrain, GpioPullNo, GpioSpeedLow);

    while(sdq_slave_receive_and_process_command(sdq_slave))
        ;

    const bool result = (sdq_slave->error == SDQSlaveErrorNone);

    furi_hal_gpio_init(sdq_slave->gpio_pin, GpioModeInterruptRiseFall, GpioPullNo, GpioSpeedLow);
    FURI_CRITICAL_EXIT();

    return result;
}

static void sdq_slave_exti_callback(void* context) {
    SDQSlave* sdq_slave = context;

    const volatile bool input_state = furi_hal_gpio_read(sdq_slave->gpio_pin);
    static uint32_t pulse_start = 0;

    if(input_state) {
        const uint32_t pulse_length =
            (DWT->CYCCNT - pulse_start) / furi_hal_cortex_instructions_per_microsecond();

        if((pulse_length >= sdq_slave->sdq_timings.BREAK_meaningful_min) &&
           (pulse_length <= sdq_slave->sdq_timings.BREAK_meaningful_max)) {
            sdq_slave->error = SDQSlaveErrorResetInProgress;
            /* Determine reset type (chooses speed mode if supported by the device) */
            sdq_slave->is_short_reset = pulse_length <=
                                        sdq_slave->sdq_timings.BREAK_meaningful_max;
            /* Initial reset allows going directly into overdrive mode */
            sdq_slave->is_first_reset = true;

            const bool result = sdq_slave_bus_start(sdq_slave);

            if(result && sdq_slave->result_callback != NULL) {
                sdq_slave->result_callback(sdq_slave->result_callback_context);
            }
        }

    } else {
        pulse_start = DWT->CYCCNT;
    }
}

void sdq_slave_start(SDQSlave* bus) {
    furi_hal_gpio_add_int_callback(bus->gpio_pin, sdq_slave_exti_callback, bus);
    furi_hal_gpio_write(bus->gpio_pin, true);
    furi_hal_gpio_init(bus->gpio_pin, GpioModeInterruptRiseFall, GpioPullNo, GpioSpeedLow);
}

void sdq_slave_stop(SDQSlave* bus) {
    furi_hal_gpio_write(bus->gpio_pin, true);
    furi_hal_gpio_init(bus->gpio_pin, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
    furi_hal_gpio_remove_int_callback(bus->gpio_pin);
}

void sdq_slave_set_reset_callback(SDQSlave* bus, SDQSlaveResetCallback callback, void* context) {
    bus->reset_callback = callback;
    bus->reset_callback_context = context;
}

void sdq_slave_set_command_callback(SDQSlave* bus, SDQSlaveCommandCallback callback, void* context) {
    bus->command_callback = callback;
    bus->command_callback_context = context;
}

void sdq_slave_set_result_callback(SDQSlave* bus, SDQSlaveResultCallback result_cb, void* context) {
    bus->result_callback = result_cb;
    bus->result_callback_context = context;
}

bool sdq_slave_receive_bit(SDQSlave* bus) {
    const SDQTimings* timings = &bus->sdq_timings;
    // wait while bus is low
    if(!onewire_slave_wait_while_gpio_is(bus, timings->ZERO_meaningful_max, false)) {
        bus->error = SDQSlaveErrorResetInProgress;
        return false;
    }
    // wait while bus is high
    if(!onewire_slave_wait_while_gpio_is(bus, TH_TIMEOUT_MAX, true)) {
        bus->error = SDQSlaveErrorTimeout;
        return false;
    }
    // wait a time of zero
    return onewire_slave_wait_while_gpio_is(bus, timings->ONE_meaningful_max, false);
}

bool onewire_slave_send_bit(SDQSlave* bus, bool value, bool isLastOfByte) {
    const SDQTimings* timings = &bus->sdq_timings;
    // wait while bus is low
    if(!onewire_slave_wait_while_gpio_is(bus, timings->ZERO_meaningful_max, false)) {
        bus->error = SDQSlaveErrorResetInProgress;
        return false;
    }
    // wait while bus is high
    if(!onewire_slave_wait_while_gpio_is(bus, TH_TIMEOUT_MAX, true)) {
        bus->error = SDQSlaveErrorTimeout;
        return false;
    }
    // choose write time
    uint32_t meaningful_time;
    uint32_t recovery_time;
    furi_hal_gpio_write(bus->gpio_pin, false);
    if(isLastOfByte) {
        meaningful_time = value ? timings->ZERO_meaningful : timings->ONE_meaningful;
        recovery_time = value ? timings->ZERO_STOP_recovery : timings->ONE_STOP_recovery;
    } else {
        meaningful_time = value ? timings->ZERO_meaningful : timings->ONE_meaningful;
        recovery_time = value ? timings->ZERO_recovery : timings->ONE_recovery;
    }
    furi_delay_us(meaningful_time);
    furi_hal_gpio_write(bus->gpio_pin, true);
    furi_delay_us(recovery_time);
}

bool sdq_slave_send(SDQSlave* bus, const uint8_t* data, size_t data_size) {
    furi_hal_gpio_write(bus->gpio_pin, true);
    size_t bytes_sent = 0;
    for(; bytes_sent < data_size; ++bytes_sent) {
        const uint8_t data_byte = data[bytes_sent];
        for(uint8_t bit_mask = 0x01; bit_mask != 0; bit_mask <<= 1) {
            bool isLastOfByte = (bit_mask == 0x80);
            if(!sdq_slave_send_bit(bus, bit_mask & data_byte, isLastOfByte)) {
                return false;
            }
        }
    }
    return true;
}

bool onewire_slave_receive(SDQSlave* bus, uint8_t* data, size_t data_size) {
    furi_hal_gpio_write(bus->gpio_pin, true);
    size_t bytes_received = 0;
    for(; bytes_received < data_size; ++bytes_received) {
        uint8_t value = 0;
        for(uint8_t bit_mask = 0x01; bit_mask != 0; bit_mask <<= 1) {
            if(onewire_slave_receive_bit(bus)) {
                value |= bit_mask;
            }
            if(bus->error != SDQSlaveErrorNone) {
                return false;
            }
        }
        data[bytes_received] = value;
    }
    return true;
}