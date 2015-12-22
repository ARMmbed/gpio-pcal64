/* mbed Microcontroller Library
 * Copyright (c) 2006-2015 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __GPIO_PCAL64_H__
#define __GPIO_PCAL64_H__

#include "mbed-drivers/mbed.h"

using namespace mbed::util;


class PCAL64
{
public:
    typedef enum {
        PRIMARY_ADDRESS   = 0x40,
        SECONDARY_ADDRESS = 0x42
    } address_t;

    typedef enum {
        P0_0 = 0,
        P0_1 = 1,
        P0_2 = 2,
        P0_3 = 3,
        P0_4 = 4,
        P0_5 = 5,
        P0_6 = 6,
        P0_7 = 7,

        P1_0 = 8,
        P1_1 = 9,
        P1_2 = 10,
        P1_3 = 11,
        P1_4 = 12,
        P1_5 = 13,
        P1_6 = 14,
        P1_7 = 15,
        Pin_End
    } pin_t;

    typedef enum {
        Output = 0,
        Input  = 1
    } direction_t;

    typedef enum {
        Low    = 0,
        High   = 1
    } value_t;

    PCAL64(PinName _sda, PinName _scl, address_t _address, PinName _irq);

    bool mode(pin_t pin, direction_t direction, FunctionPointer0<void> callback);
    bool write(pin_t pin, int value, FunctionPointer0<void> callback);
    bool read(pin_t pin, FunctionPointer1<void, int> callback);
    bool toggle(pin_t pin, FunctionPointer0<void> callback);

    /*************************************************************************/

    class PinActionAdder
    {
        friend PCAL64;
    public:
        PinActionAdder& set(pin_t pin, direction_t direction);
        PinActionAdder& set(pin_t pin, value_t value);
        PinActionAdder& callback(FunctionPointer0<void> callback);
        ~PinActionAdder();
    private:
        PinActionAdder(PCAL64* owner, pin_t pin, direction_t direction);
        PinActionAdder(PCAL64* owner, pin_t pin, value_t value);
        const PinActionAdder& operator=(const PinActionAdder& a);
        PinActionAdder(const PinActionAdder& a);

        uint16_t pins;
        uint16_t directions;
        uint16_t values;
        FunctionPointer0<void> _callback;
        bool ready;
        PCAL64* owner;
    };

    PinActionAdder set(pin_t pin, direction_t direction);
    PinActionAdder set(pin_t pin, value_t value);

    bool bulkSet(uint16_t pins, uint16_t directions, uint16_t values, FunctionPointer0<void> callback);

private:

    typedef enum {
        INPUT_PORT_0                    = 0x00,
        INPUT_PORT_1                    = 0x01,
        OUTPUT_PORT_0                   = 0x02,
        OUTPUT_PORT_1                   = 0x03,
        POLARITY_INVERSION_PORT_0       = 0x04,
        POLARITY_INVERSION_PORT_1       = 0x05,
        CONFIGURATION_PORT_0            = 0x06,
        CONFIGURATION_PORT_1            = 0x07,

        OUTPUT_DRIVE_STRENGTH_0_LOW     = 0x40,
        OUTPUT_DRIVE_STRENGTH_0_HIGH    = 0x41,
        OUTPUT_DRIVE_STRENGTH_1_LOW     = 0x42,
        OUTPUT_DRIVE_STRENGTH_1_HIGH    = 0x43,
        INPUT_LATCH_0                   = 0x44,
        INPUT_LATCH_1                   = 0x45,
        PULL_UP_DOWN_ENABLE_0           = 0x46,
        PULL_UP_DOWN_ENABLE_1           = 0x47,
        PULL_UP_DOWN_SELECTION_0        = 0x48,
        PULL_UP_DOWN_SELECTION_1        = 0x49,
        INTERRUPT_MASK_0                = 0x4A,
        INTERRUPT_MASK_1                = 0x4B,
        INTERRUPT_STATUS_0              = 0x4C,
        INTERRUPT_STATUS_1              = 0x4D,
        OUTPUT_PORT_CONFIGURATION       = 0x4F
    } register_t;

    typedef enum {
        STATE_IDLE,
        STATE_DIRECTION_GET,
        STATE_WRITE_GET,
        STATE_COMMAND_DONE,
        STATE_READ_GET,
        STATE_TOGGLE_GET,
        STATE_BULK_VALUE_GET,
        STATE_BULK_VALUE_SET,
        STATE_BULK_DIRECTION_GET,
        STATE_BULK_DIRECTION_SET
    } state_t;

    typedef union {
        direction_t direction;
        int         value;
    } parameter_t;

    int getRegister(register_t reg);
    void getRegisterDone(Buffer txBuffer, Buffer rxBuffer, int code);

    int setRegister(register_t reg, uint16_t value);
    void setRegisterDone(Buffer txBuffer, Buffer rxBuffer, int code);

    void eventHandler(uint16_t value);
    void interruptISR(void);
    void interruptTask(void);

    I2C i2c;
    address_t address;
    InterruptIn irq;

    state_t     state;
    pin_t       currentPin;
    parameter_t current;

    uint16_t bulkPins;
    uint16_t bulkDirections;
    uint16_t bulkValues;

    char memoryWrite[3];
    char memoryRead[2];

    FunctionPointer0<void>      commandCallback;
    FunctionPointer1<void, int> readCallback;
};

#endif // __GPIO_PCAL64_H__
