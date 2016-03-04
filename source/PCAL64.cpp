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

PCAL64::PCAL64(PinName sda, PinName scl, uint16_t _address, PinName _irq)
    :   i2c(sda, scl),
        address(_address),
        irq(_irq),
        state(STATE_IDLE)
{
    i2c.frequency(400000);

    if (_irq != NC)
    {
        irq.fall(this, &PCAL64::internalIRQHandler);
    }
}

PCAL64::~PCAL64()
{
    irq.fall(NULL);
}

bool PCAL64::bulkRead(FunctionPointer1<void, uint32_t> callback)
{
    bool result = false;

    if (state == STATE_IDLE)
    {
        state = STATE_READ_GET;
        externalReadHandler = callback;

        FunctionPointer0<void> fp(this, &PCAL64::eventHandler);
        result = i2c.read(address, INPUT_PORT_0, readBuffer, 2, fp);
    }

    return result;
}

bool PCAL64::bulkWrite(uint32_t _pins, uint32_t directions, uint32_t values, FunctionPointer0<void> callback)
{
    bool result = false;

    if (state == STATE_IDLE)
    {
        state = STATE_WRITE_GET_DIRECTIONS;
        externalDoneHandler = callback;

        pins = _pins;

        /* NOTE: the PCAL64 defines 0 to be output and 1 to be input.
           This is opposite from the gpio-expander API, hence the invesion.
        */
        param1 = ~directions;
        param2 = values;

        FunctionPointer0<void> fp(this, &PCAL64::eventHandler);
        result = i2c.read(address, CONFIGURATION_PORT_0, readBuffer, 2, fp);
    }

    return result;
}

bool PCAL64::bulkToggle(uint32_t _pins, FunctionPointer0<void> callback)
{
    bool result = false;

    if (state == STATE_IDLE)
    {
        state = STATE_TOGGLE_GET_VALUES;
        externalDoneHandler = callback;

        pins = _pins;

        FunctionPointer0<void> fp(this, &PCAL64::eventHandler);
        result = i2c.read(address, OUTPUT_PORT_0, readBuffer, 2, fp);
    }

    return result;
}

bool PCAL64::bulkSetInterrupt(uint32_t _pins, uint32_t values, FunctionPointer0<void> callback)
{
    bool result = false;

    if (state == STATE_IDLE)
    {
        state = STATE_INTERRUPT_GET_DIRECTIONS;
        externalDoneHandler = callback;

        pins = _pins;
        param1 = values;

        FunctionPointer0<void> fp(this, &PCAL64::eventHandler);
        result = i2c.read(address, CONFIGURATION_PORT_0, readBuffer, 2, fp);
    }

    return result;
}

void PCAL64::setInterruptHandler(FunctionPointer3<void, uint16_t, uint32_t, uint32_t> callback)
{
    externalIRQHandler = callback;
}

void PCAL64::clearInterruptHandler(void)
{
    externalIRQHandler.clear();
}

void PCAL64::internalIRQHandler(void)
{
    if (state == STATE_IDLE)
    {
        state = STATE_INTERRUPT_GET_VALUES;

        FunctionPointer0<void> fp(this, &PCAL64::eventHandler);
        i2c.read(address, INPUT_PORT_0, readBuffer, 2, fp);
    }
    else
    {
        // transfer in progress, repost IRQ handler with 1 ms delay
        minar::Scheduler::postCallback(this, &PCAL64::internalIRQHandler)
            .delay(minar::milliseconds(1))
            .tolerance(1);
    }
}

void PCAL64::eventHandler()
{
    switch (state)
    {
        /*********************************************************************/
        /* bulkRead                                                          */
        /*********************************************************************/
        case STATE_READ_GET:
            {
                if (externalReadHandler)
                {
                    uint32_t values = readBuffer[1];
                    values = (values << 8) | readBuffer[0];

                    minar::Scheduler::postCallback(externalReadHandler.bind(values))
                        .tolerance(1);
                }

                state = STATE_IDLE;
            }
            break;

        /*********************************************************************/
        /* bulkWrite                                                         */
        /*********************************************************************/
        case STATE_WRITE_GET_DIRECTIONS:
            {
                uint16_t directions = readBuffer[1];
                directions = (directions << 8) | readBuffer[0];

                // enable bits
                directions |= (pins & param1);

                // disable bits
                directions &= ~(pins & ~param1);

                uint8_t writeBuffer[2];
                writeBuffer[0] = directions;
                writeBuffer[1] = directions >> 8;

                FunctionPointer0<void> fp(this, &PCAL64::eventHandler);
                i2c.write(address, CONFIGURATION_PORT_0, writeBuffer, 2, fp);

                state = STATE_WRITE_SET_DIRECTIONS;
            }
            break;

        case STATE_WRITE_SET_DIRECTIONS:
            {
                FunctionPointer0<void> fp(this, &PCAL64::eventHandler);
                i2c.read(address, OUTPUT_PORT_0, readBuffer, 2, fp);

                state = STATE_WRITE_GET_VALUES;
            }
            break;

        case STATE_WRITE_GET_VALUES:
            {
                uint16_t values = readBuffer[1];
                values = (values << 8) | readBuffer[0];

                // enable bits
                values |= (pins & param2);

                // disable bits
                values &= ~(pins & ~param2);

                uint8_t writeBuffer[2];
                writeBuffer[0] = values;
                writeBuffer[1] = values >> 8;

                FunctionPointer0<void> fp(this, &PCAL64::eventHandler);
                i2c.write(address, OUTPUT_PORT_0, writeBuffer, 2, fp);

                state = STATE_SIGNAL_DONE;
            }
            break;

        /*********************************************************************/
        /* bulkToggle                                                        */
        /*********************************************************************/
        case STATE_TOGGLE_GET_VALUES:
            {
                uint16_t values = readBuffer[1];
                values = (values << 8) | readBuffer[0];

                uint16_t original = values;

                // enable bits
                values |= (pins & ~original);

                // disable bits
                values &= ~(pins & original);

                uint8_t writeBuffer[2];
                writeBuffer[0] = values;
                writeBuffer[1] = values >> 8;

                FunctionPointer0<void> fp(this, &PCAL64::eventHandler);
                i2c.write(address, OUTPUT_PORT_0, writeBuffer, 2, fp);

                state = STATE_SIGNAL_DONE;
            }
            break;

        /*********************************************************************/
        /* bulkInterrupt                                                     */
        /*********************************************************************/
        case STATE_INTERRUPT_GET_DIRECTIONS:
            {
                uint16_t directions = readBuffer[1];
                directions = (directions << 8) | readBuffer[0];

                /* set interrupt pins to be input */

                // enable bits
                directions |= (pins & param1);

                // disable bits
                directions &= ~(pins & ~param1);

                uint8_t writeBuffer[2];
                writeBuffer[0] = directions;
                writeBuffer[1] = directions >> 8;

                FunctionPointer0<void> fp(this, &PCAL64::eventHandler);
                i2c.write(address, CONFIGURATION_PORT_0, writeBuffer, 2, fp);

                state = STATE_INTERRUPT_SET_DIRECTIONS;

            }
            break;

        case STATE_INTERRUPT_SET_DIRECTIONS:
            {
                FunctionPointer0<void> fp(this, &PCAL64::eventHandler);
                i2c.read(address, INTERRUPT_MASK_0, readBuffer, 2, fp);

                state = STATE_INTERRUPT_GET_MASK;
            }
            break;

        case STATE_INTERRUPT_GET_MASK:
            {
                uint16_t values = readBuffer[1];
                values = (values << 8) | readBuffer[0];

                // enable bits
                values |= (pins & param1);

                // disable bits
                values &= ~(pins & ~param1);

                uint8_t writeBuffer[2];
                writeBuffer[0] = values;
                writeBuffer[1] = values >> 8;

                FunctionPointer0<void> fp(this, &PCAL64::eventHandler);
                i2c.write(address, INTERRUPT_MASK_0, writeBuffer, 2, fp);

                state = STATE_SIGNAL_DONE;
            }
            break;

        /*********************************************************************/
        /* IRQ handler                                                       */
        /*********************************************************************/
        case STATE_INTERRUPT_GET_VALUES:
            {
                cache = readBuffer[1];
                cache = (cache << 8) | readBuffer[0];

                FunctionPointer0<void> fp(this, &PCAL64::eventHandler);
                i2c.read(address, INTERRUPT_STATUS_0, readBuffer, 2, fp);

                state = STATE_INTERRUPT_GET_STATUS;
            }
            break;

        case STATE_INTERRUPT_GET_STATUS:
            {
                uint16_t status = readBuffer[1];
                status = (status << 8) | readBuffer[0];

                if (externalIRQHandler)
                {
                    minar::Scheduler::postCallback(externalIRQHandler.bind(address, status, cache))
                        .tolerance(1);
                }

                state = STATE_IDLE;
            }
            break;

        /*********************************************************************/
        /* signal done                                                       */
        /*********************************************************************/
        case STATE_SIGNAL_DONE:
            {
                if (externalDoneHandler)
                {
                    minar::Scheduler::postCallback(externalDoneHandler)
                        .tolerance(1);
                }

                state = STATE_IDLE;
            }
            break;

        default:
            state = STATE_IDLE;
            break;
    }
}
