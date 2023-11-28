#include <furi.h>
#include <furi_hal.h>
#include <sdq_slave.h>

#define TH_TIMEOUT_MAX 15000 /* Maximum time before general timeout */

struct SDQSlave {
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
};

uint8_t crc_data(const uint8_t* data, size_t len) {
    crc_t crc = crc_init();
    crc = crc_update(crc, data, len);
    crc = crc_finalize(crc);
    return crc;
}

static bool sdq_slave_wait_while_gpio_is(SDQSlave* bus, uint32_t time_us, const bool pin_value) {
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

static inline bool sdq_slave_receive_and_process_command(SDQSlave* bus) {
    /* Reset condition detected, send a presence pulse and reset protocol state */
    if(bus->error == SDQSlaveErrorResetInProgress) {
        if(!bus->is_first_reset) {
            /* Guess the reset type */
            bus->is_short_reset = sdq_slave_wait_while_gpio_is(
                bus,
                bus->sdq_timings.BREAK_meaningful_max - bus->sdq_timings.BREAK_meaningful_min,
                false);
        } else {
            bus->is_first_reset = false;
        }
        // Replace with the correct assertion function in your environment
        furi_assert(bus->reset_callback);
        if(bus->reset_callback(bus->is_short_reset, bus->reset_callback_context)) {
            if(sdq_slave_show_presence(bus)) {
                bus->error = SDQSlaveErrorNone;
                return true;
            }
        }
    } else if(bus->error == SDQSlaveErrorNone) {
        uint8_t command;
        if(sdq_slave_receive(bus, &command, sizeof(command))) {
            // Replace with the correct assertion function in your environment
            furi_assert(bus->command_callback);
            if(bus->command_callback(command, bus->command_callback_context)) {
                return true;
            }
        }
        return (bus->error == SDQSlaveErrorResetInProgress);
    }
    return false;
}

static inline bool sdq_slave_bus_start(SDQSlave* bus) {
    FURI_CRITICAL_ENTER();
    furi_hal_gpio_init(bus->gpio_pin, GpioModeOutputOpenDrain, GpioPullNo, GpioSpeedLow);
    while(sdq_slave_receive_and_process_command(bus))
        ;
    const bool result = (bus->error == SDQSlaveErrorNone);
    furi_hal_gpio_init(bus->gpio_pin, GpioModeInterruptRiseFall, GpioPullNo, GpioSpeedLow);
    FURI_CRITICAL_EXIT();
    return result;
}

static void sdq_slave_exti_callback(void* context) {
    SDQSlave* sdq_slave = context;
    const volatile bool input_state = furi_hal_gpio_read(sdq_slave->gpio_pin);
    static uint32_t pulse_start = 0;
    if(input_state) {
        const uint32_t pulse_length = (DWT->CYCCNT - pulse_start) / furi_hal_cortex_instructions_per_microsecond();
        if((pulse_length >= sdq_slave->sdq_timings.BREAK_meaningful_min) &&
           (pulse_length <= sdq_slave->sdq_timings.BREAK_meaningful_max)) {
            sdq_slave->error = SDQSlaveErrorResetInProgress;
            /* Determine reset type (chooses speed mode if supported by the device) */
            sdq_slave->is_short_reset = pulse_length <= sdq_slave->sdq_timings.BREAK_meaningful_max;
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
    if(!sdq_slave_wait_while_gpio_is(bus, timings->ZERO_meaningful_max, false)) {
        bus->error = SDQSlaveErrorResetInProgress;
        return false;
    }
    // wait while bus is high
    if(!sdq_slave_wait_while_gpio_is(bus, TH_TIMEOUT_MAX, true)) {
        bus->error = SDQSlaveErrorTimeout;
        return false;
    }
    // wait a time of zero
    return sdq_slave_wait_while_gpio_is(bus, timings->ONE_meaningful_max, false);
}

static bool sdq_slave_send_byte(SDQSlave* bus, uint8_t byte) {
    const SDQTimings* timings = &bus->sdq_timings;

    for(uint8_t mask = 0x80; mask != 0; mask >>= 1) {
        if(!sdq_slave_wait_while_gpio_is(bus, timings->ZERO_meaningful_max, false)) {
            bus->error = SDQSlaveErrorResetInProgress;
            return false;
        }

        if(!sdq_slave_wait_while_gpio_is(bus, TH_TIMEOUT_MAX, true)) {
            bus->error = SDQSlaveErrorTimeout;
            return false;
        }

        uint32_t meaningful_time = (mask & byte) ? timings->ONE_meaningful : timings->ZERO_meaningful;
        uint32_t recovery_time = (mask & byte) ? timings->ONE_recovery : timings->ZERO_recovery;
        furi_hal_gpio_write(bus->gpio_pin, false);
        furi_delay_us(meaningful_time);
        furi_hal_gpio_write(bus->gpio_pin, true);

        // Add stop recovery time for the last bit in the byte
        if(mask == 0x01) {
            recovery_time = (mask & byte) ? timings->ONE_STOP_recovery : timings->ZERO_STOP_recovery;
        }

        furi_delay_us(recovery_time);
    }

    return true;
}

bool sdq_slave_send(SDQSlave* bus, const uint8_t* data, size_t data_size) {
    furi_hal_gpio_write(bus->gpio_pin, true);
    for(size_t i = 0; i < data_size; ++i) {
        if(!sdq_slave_send_byte(bus, data[i])) {
            return false;
        }
    }
    // Calculate and send CRC8
    uint8_t crc = crc_data(data, data_size);  // Fix: use crc_data instead of crc8_calculate
    if(!sdq_slave_send_byte(bus, crc)) {
        return false;
    }
    return true;
}

bool sdq_slave_receive(SDQSlave* bus, uint8_t* data, size_t data_size) {
    furi_hal_gpio_write(bus->gpio_pin, true);
    for(size_t i = 0; i < data_size; ++i) {
        uint8_t value = 0;
        for(uint8_t mask = 0x80; mask != 0; mask >>= 1) {
            if(sdq_slave_receive_bit(bus)) {
                value |= mask;
            }
            if(bus->error != SDQSlaveErrorNone) {
                return false;
            }
        }
        data[i] = value;
    }

    // Receive and check CRC8
    uint8_t received_crc = 0;
    if(!sdq_slave_receive_byte(bus, &received_crc)) {
        return false;
    }
    uint8_t calculated_crc = crc_data(data, data_size);
    if(received_crc != calculated_crc) {
        bus->error = SDQSlaveErrorInvalidCommand;
        return false;
    }
    return true;
}
