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


void io2Done(void)
{
    swoprintf("io2Done\r\n");
}

void io1Done(void)
{
    swoprintf("io1Done\r\n");

    ioexpander2.set(PCAL64::P1_2, PCAL64::Output)
               .set(PCAL64::P1_2, PCAL64::High)
               .callback(io2Done);
}

/*****************************************************************************/
/* Debug                                                                     */
/*****************************************************************************/

// enable buttons to initiate transfer
static InterruptIn button1(BTN1);

void button1Task()
{
    swoprintf("button:\r\n");

    ioexpander1.toggle(PCAL64::P0_3, NULL);
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

    ioexpander1.set(PCAL64::P0_5, PCAL64::Output)
               .set(PCAL64::P0_5, PCAL64::High)
               .callback(io1Done);
}
