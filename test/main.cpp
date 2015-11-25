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

#include "mbed-drivers/mbed.h"
#include "swo/swo.h"

#include "gpio-pcal64/PCAL64.h"

// vibra off, DigitalOut is 0 by default
static DigitalOut vibra(VIBRA);

/*****************************************************************************/
/* PCAL64                                                                    */
/*****************************************************************************/

PCAL64 ioexpander1(YOTTA_CFG_HARDWARE_TEST_PINS_I2C_SDA, YOTTA_CFG_HARDWARE_TEST_PINS_I2C_SCL, PCAL64::PRIMARY_ADDRESS, PF3);
PCAL64 ioexpander2(YOTTA_CFG_HARDWARE_TEST_PINS_I2C_SDA, YOTTA_CFG_HARDWARE_TEST_PINS_I2C_SCL, PCAL64::SECONDARY_ADDRESS, PB6);

void readDone2(int value)
{
    swoprintf("readDone: %02X\r\n", value);
}

void readDone1(int value)
{
    swoprintf("readDone: %02X\r\n", value);

//    ioexpander2.read(PCAL64::P1_1, readDone2);
}

void writeDone(void)
{
    swoprintf("writeDone\r\n");
}

void modeDone(void)
{
    swoprintf("modeDone\r\n");
}

void toggleDone(void)
{
    swoprintf("toggleDone\r\n");
}

/*****************************************************************************/
/* Debug                                                                     */
/*****************************************************************************/

// enable buttons to initiate transfer
static InterruptIn button1(BTN1);

void button1Task()
{
    swoprintf("button:\r\n");

    ioexpander1.toggle(PCAL64::P0_3, toggleDone);
}

void button1ISR()
{
    minar::Scheduler::postCallback(button1Task);
}

/*****************************************************************************/
/* App start                                                                 */
/*****************************************************************************/

void app_start(int, char *[])
{
    // setup buttons
    button1.fall(button1ISR);

    ioexpander1.mode(PCAL64::P0_3, PCAL64::Output, modeDone);
}
