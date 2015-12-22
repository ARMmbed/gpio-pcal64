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

#include "gpio-pcal64/PCAL64.h"

#if 1
#include "swo/swo.h"
#define printf(...) { swoprintf(__VA_ARGS__); }
#else
#define printf(...)
#endif

PCAL64::PCAL64(PinName _sda, PinName _scl, address_t _address, PinName _irq)
    :   i2c(_sda, _scl),
        address(_address),
        irq(_irq),
        state(STATE_IDLE),
        currentPin(Pin_End),
        bulkPins(0),
        bulkDirections(0),
        bulkValues(0)
{
    i2c.frequency(400000);

    irq.fall(this, &PCAL64::interruptISR);

    printf("PCAL64: %02X\r\n", address);
}

bool PCAL64::mode(pin_t pin, direction_t direction, FunctionPointer0<void> callback)
{
    bool result = false;

    if (pin < Pin_End)
    {
        int retval = getRegister(CONFIGURATION_PORT_0);

        if (retval != -1)
        {
            currentPin = pin;
            current.direction = direction;
            commandCallback = callback;
            state = STATE_DIRECTION_GET;
            result = true;
        }
    }

    return result;
}


bool PCAL64::write(pin_t pin, int value, FunctionPointer0<void> callback)
{
    bool result = false;

    if (pin < Pin_End)
    {
        int retval = getRegister(OUTPUT_PORT_0);

        if (retval != -1)
        {
            currentPin = pin;
            current.value = value;
            commandCallback = callback;
            state = STATE_WRITE_GET;
            result = true;
        }
    }

    return result;
}

bool PCAL64::read(pin_t pin, FunctionPointer1<void, int> callback)
{
    bool result = false;

    if (pin < Pin_End)
    {
        int retval = getRegister(INPUT_PORT_0);

        if (retval != -1)
        {
            currentPin = pin;
            readCallback = callback;
            state = STATE_READ_GET;
            result = true;
        }
    }

    return result;
}

bool PCAL64::toggle(pin_t pin, FunctionPointer0<void> callback)
{
    bool result = false;

    if (pin < Pin_End)
    {
        int retval = getRegister(OUTPUT_PORT_0);

        if (retval != -1)
        {
            currentPin = pin;
            commandCallback = callback;
            state = STATE_TOGGLE_GET;
            result = true;
        }
    }

    return result;
}

PCAL64::PinActionAdder PCAL64::set(pin_t pin, direction_t direction)
{
    PinActionAdder adder(this, pin, direction);

    return adder;
}

PCAL64::PinActionAdder PCAL64::set(pin_t pin, value_t value)
{
    PinActionAdder adder(this, pin, value);

    return adder;
}

bool PCAL64::bulkSet(uint16_t pins, uint16_t directions, uint16_t values, FunctionPointer0<void> callback)
{
    bool result = false;

    int retval = getRegister(OUTPUT_PORT_0);

    if (retval != -1)
    {
        bulkPins = pins;
        bulkDirections = directions;
        bulkValues = values;
        commandCallback = callback;

        state = STATE_BULK_VALUE_GET;
        result = true;
    }

    return result;
}


/*****************************************************************************/
/*  Generic functions for reading and writing registers.                     */
/*****************************************************************************/

int PCAL64::getRegister(register_t reg)
{
    memoryWrite[0] = reg;

    I2C::event_callback_t callback(this, &PCAL64::getRegisterDone);

    return i2c.transfer(address, memoryWrite, 1, memoryRead, 2, callback);
}

void PCAL64::getRegisterDone(Buffer txBuffer, Buffer rxBuffer, int code)
{
    (void) txBuffer;
    (void) rxBuffer;
    (void) code;

    uint16_t value = ((uint16_t) memoryRead[1] << 8) | memoryRead[0];

    FunctionPointer1<void, uint16_t> fp(this, &PCAL64::eventHandler);
    minar::Scheduler::postCallback(fp.bind(value));
}

int PCAL64::setRegister(register_t reg, uint16_t value)
{
    memoryWrite[0] = reg;
    memoryWrite[1] = value;
    memoryWrite[2] = value >> 8;

    I2C::event_callback_t callback(this, &PCAL64::setRegisterDone);

    return i2c.transfer(address, memoryWrite, 3, memoryRead, 0, callback);
}

void PCAL64::setRegisterDone(Buffer txBuffer, Buffer rxBuffer, int code)
{
    (void) txBuffer;
    (void) rxBuffer;
    (void) code;

    FunctionPointer1<void, uint16_t> fp(this, &PCAL64::eventHandler);
    minar::Scheduler::postCallback(fp.bind(0));
}

/*
    Main event handler
*/
void PCAL64::eventHandler(uint16_t value)
{
    printf("event: %02X\r\n", value);

    switch (state)
    {
        case STATE_DIRECTION_GET:
            {
                uint16_t pinBit = 1 << currentPin;
                uint16_t config = value;

                // compare change
                if ((current.direction == PCAL64::Input) && !(config & pinBit))
                {
                    config |= pinBit;
                }
                else if ((current.direction == PCAL64::Output) && (config & pinBit))
                {
                    config &= ~pinBit;
                }

                // only send changes
                if (config != value)
                {
                    setRegister(CONFIGURATION_PORT_0, config);
                    state = STATE_COMMAND_DONE;
                }
                else
                {
                    // no changes made, schedule callback
                    if (commandCallback)
                    {
                        minar::Scheduler::postCallback(commandCallback);
                    }
                    state = STATE_IDLE;
                }
            }
            break;

        case STATE_WRITE_GET:
            {
                uint16_t pinBit = 1 << currentPin;
                uint16_t output = value;

                // compare change
                if ((current.value == 1) && !(output & pinBit))
                {
                    output |= pinBit;
                }
                else if ((current.value == 0) && (output & pinBit))
                {
                    output &= ~pinBit;
                }

                // only send changes
                if (output != value)
                {
                    setRegister(OUTPUT_PORT_0, output);
                    state = STATE_COMMAND_DONE;
                }
                else
                {
                    // no changes made, schedule callback
                    if (commandCallback)
                    {
                        minar::Scheduler::postCallback(commandCallback);
                    }
                    state = STATE_IDLE;
                }
            }
            break;

        case STATE_COMMAND_DONE:
            {
                if (commandCallback)
                {
                    minar::Scheduler::postCallback(commandCallback);
                }
                state = STATE_IDLE;
            }
            break;

        case STATE_READ_GET:
            {
                if (readCallback)
                {
                    int pin = (value & (1 << currentPin)) ? 1 : 0;
                    minar::Scheduler::postCallback(readCallback.bind(pin));
                }
                state = STATE_IDLE;
            }
            break;

        case STATE_TOGGLE_GET:
            {
                uint16_t pinBit = 1 << currentPin;

                if (value & pinBit)
                {
                    value &= ~pinBit;
                }
                else
                {
                    value |= pinBit;
                }

                setRegister(OUTPUT_PORT_0, value);
                state = STATE_COMMAND_DONE;
            }
            break;

        case STATE_BULK_VALUE_GET:
            {
                // set setted pins
                value |= bulkPins & bulkValues;

                // clear cleared pins
                value &= ~(bulkPins & ~bulkValues);

                setRegister(OUTPUT_PORT_0, value);
                state = STATE_BULK_VALUE_SET;
            }
            break;

        case STATE_BULK_VALUE_SET:
            {
                getRegister(CONFIGURATION_PORT_0);
                state = STATE_BULK_DIRECTION_GET;
            }
            break;

        case STATE_BULK_DIRECTION_GET:
            {
                // set setted pins
                value |= bulkPins & bulkDirections;

                // clear cleared pins
                value &= ~(bulkPins & ~bulkDirections);

                setRegister(CONFIGURATION_PORT_0, value);
                state = STATE_COMMAND_DONE;
            }
            break;

        default:
            break;
    }
}

/*
    Interrupts
*/
void PCAL64::interruptISR()
{

}

void PCAL64::interruptTask()
{
    printf("interrupt\r\n");
}

/*****************************************************************************/

PCAL64::PinActionAdder::PinActionAdder(PCAL64* _owner, pin_t pin, direction_t direction)
    :   pins(1 << pin),
        values(0xFFFF),
        _callback((void (*)(void)) NULL),
        ready(false),
        owner(_owner)
{
    directions = 0xFFFF & (direction << pin);
}

PCAL64::PinActionAdder::PinActionAdder(PCAL64* _owner, pin_t pin, value_t value)
    :   pins(1 << pin),
        directions(0xFFFF),
        _callback((void (*)(void)) NULL),
        ready(false),
        owner(_owner)
{
    values = 0xFFFF & (value << pin);
}

const PCAL64::PinActionAdder& PCAL64::PinActionAdder::operator=(const PCAL64::PinActionAdder& adder)
{
    pins       = adder.pins;
    directions = adder.directions;
    values     = adder.values;
    _callback  = adder._callback;
    ready      = adder.ready;
    owner      = adder.owner;

    return *this;
}

PCAL64::PinActionAdder::PinActionAdder(const PinActionAdder& adder)
{
    *this = adder;
}

PCAL64::PinActionAdder::~PinActionAdder()
{
    if (ready)
    {
        owner->bulkSet(pins, directions, values, _callback);
    }
}

PCAL64::PinActionAdder& PCAL64::PinActionAdder::set(pin_t pin, direction_t direction)
{
    pins |= 1 << pin;
    directions &= direction << pin;
    ready = true;

    return *this;
}

PCAL64::PinActionAdder& PCAL64::PinActionAdder::set(pin_t pin, value_t value)
{
    pins |= 1 << pin;
    values &= value << pin;
    ready = true;

    return *this;
}

PCAL64::PinActionAdder& PCAL64::PinActionAdder::callback(FunctionPointer0<void> __callback)
{
    _callback = __callback;
    ready = true;

    return *this;
}
