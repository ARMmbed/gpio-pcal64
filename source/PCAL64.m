#import "gpio-pcal64/PCAL64.h"

#import "emlib/em_i2c.h"
#import "emlib/em_cmu.h"
#import "emlib/em_gpio.h"

static YTError readRegister(uint8_t address, uint16_t* value, uint8_t rxLength);
static YTError writeRegister(uint8_t address, uint16_t value, uint8_t txLength);

/* Transfer structure */
static I2C_TransferSeq_TypeDef i2cTransfer;

static uint8_t txBuffer[3] = {0};
static uint8_t rxBuffer[2] = {0};

yt_callback_t handleInterrupt;

yt_callback_t callbacks[16] = {0};
enum PinTransition transitions[16] = {0};

#define PCAL64_ADDRESS 0x40

enum {
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
};


void PCAL64Init(PinNameIntType interruptPin)
{
    /***************************************************************************/
    /***************************************************************************/
    /***************************************************************************/
    /* Enabling clock to the I2C, GPIO, LE */
    CMU_ClockEnable(cmuClock_I2C0, true);  
    CMU_ClockEnable(cmuClock_GPIO, true);
    CMU_ClockEnable(cmuClock_CORELE, true);
    
    // Enabling USART0 (see errata)
    CMU_ClockEnable(cmuClock_USART0, true);
    
    /* Starting LFXO and waiting until it is stable */
    CMU_OscillatorEnable(cmuOsc_LFXO, true, true);

    /* Routing the LFXO clock to the RTC */
    CMU_ClockSelectSet(cmuClock_LFA,cmuSelect_LFXO);
    CMU_ClockEnable(cmuClock_RTC, true);


    /***************************************************************************/
    /***************************************************************************/
    /***************************************************************************/
    // Using default settings
    I2C_Init_TypeDef i2cInit = I2C_INIT_DEFAULT;

    /* Using PC0 (SDA) and PC1 (SCL) */
    GPIO_PinModeSet(gpioPortC, 0, gpioModeWiredAndPullUpFilter, 1);
    GPIO_PinModeSet(gpioPortC, 1, gpioModeWiredAndPullUpFilter, 1);
    
    /* Enable pins at location 4 */
    I2C0->ROUTE = I2C_ROUTE_SDAPEN |
                  I2C_ROUTE_SCLPEN |
                  (0x04 << _I2C_ROUTE_LOCATION_SHIFT);

    /* Initializing the I2C */
    i2cTransfer.addr = PCAL64_ADDRESS;
    i2cTransfer.buf[0].data = txBuffer;
    i2cTransfer.buf[1].data = rxBuffer;

    I2C_Init(I2C0, &i2cInit);

    /* Listen for interrupts */
    [GPIOControl requireActiveThenCall:^{
        pinSetMode(interruptPin, Pin_Input_Pull, Pin_On);
        pinSetCallback(interruptPin, Pin_Any_Edge, handleInterrupt);          
    }];

}

yt_callback_t handleInterrupt =
^{  
    /*  Read interrupt flags and the current pin value
    */
    uint16_t flags = 0;
    uint16_t values = 0;

    readRegister(INPUT_PORT_0, &values, 2);
    readRegister(INTERRUPT_STATUS_0, &flags, 2);

    // step through each pin
    for (uint8_t pin = 0; pin < 16; pin++)
    {
        uint16_t pinBit = 1 << pin;

        // check against interrupt flags
        if (flags & pinBit)
        {
            enum PinTransition transition = transitions[pin];
            bool pinSet = (values & pinBit);

            // if the transition matches, post callback task
            if ((transition == Pin_Any_Edge) ||
               ((transition == Pin_Rising_Edge) && pinSet) ||
               ((transition == Pin_Falling_Edge) && !pinSet))
            {
                ytPost(callbacks[pin], ytMilliseconds(0));
            }
        }
    }

    GPIO_PinOutToggle(gpioPortA, 2);
};

/*
    Get / Set all pins at once
*/
void PCAL64SetMode(uint16_t modes, uint16_t values)
{
    writeRegister(OUTPUT_PORT_0, values, 2);
    writeRegister(CONFIGURATION_PORT_0, modes, 2);    
}

void PCAL64SetOutput(uint16_t values)
{
    writeRegister(OUTPUT_PORT_0, values, 2);
}

uint16_t PCAL64GetValue()
{
    uint16_t values;

    readRegister(INPUT_PORT_0, &values, 2);

    return values;
}

void PCAL64SetStrength(uint32_t values)
{    
    writeRegister(OUTPUT_DRIVE_STRENGTH_0_LOW, values, 2);
    writeRegister(OUTPUT_DRIVE_STRENGTH_1_LOW, (values >> 16), 2);
}

/* 
    get/set individual pins
*/
YTError PCAL64PinSetMode(enum PCAL64PinName pinName, enum PCAL64PinMode mode, enum PinOutput value)
{
    if (pinName < Pin_End)
    {
        uint16_t pinBit = 1 << pinName;

        /*  Set pin value
        */
        uint16_t values = 0;
        readRegister(OUTPUT_PORT_0, &values, 2);

        // only write register if value has changed
        if ((value == Pin_High) && !(values & pinBit))
        {
            values |= pinBit;
            writeRegister(OUTPUT_PORT_0, values, 2);
        }
        else if ((value == Pin_Low) && (values & pinBit))
        {
            values &= ~pinBit;
            writeRegister(OUTPUT_PORT_0, values, 2);
        }

        /*  Set pin direction
        */
        uint16_t modes = 0;
        readRegister(CONFIGURATION_PORT_0, &modes, 2);

        // only write register if direction has changed
        if ((mode == Mode_Pin_Input) && !(modes & pinBit))
        {
            modes |= pinBit;
            writeRegister(CONFIGURATION_PORT_0, modes, 2);    
        }
        else if ((mode == Mode_Pin_Output) && (modes & pinBit))
        {
            modes &= ~pinBit;
            writeRegister(CONFIGURATION_PORT_0, modes, 2);    
        }

        return ytError(YTNoError);
    }
    else
    {
        return ytError(YTBadRequest);        
    }
}

YTError PCAL64PinSetOutput(enum PCAL64PinName pinName, enum PinOutput value)
{
    if (pinName < Pin_End)
    {
        uint16_t pinBit = 1 << pinName;

        /*  Set pin output value
        */
        uint16_t values = 0;
        readRegister(OUTPUT_PORT_0, &values, 2);

        // only write register if output has changed
        if ((value == Pin_High) && !(values & pinBit))
        {
            values |= pinBit;
            writeRegister(CONFIGURATION_PORT_0, values, 2);    
        }
        else if ((value == Pin_Low) && (values & pinBit))
        {
            values &= ~pinBit;
            writeRegister(CONFIGURATION_PORT_0, values, 2);    
        }

        return ytError(YTNoError);
    }
    else
    {
        return ytError(YTBadRequest);        
    }
}

YTError PCAL64PinToggle(enum PCAL64PinName pinName)
{
    if (pinName < Pin_End)
    {
        uint16_t pinBit = 1 << pinName;
        uint16_t values = 0;

        readRegister(OUTPUT_PORT_0, &values, 2);
        values = (values & ~pinBit) | ((values ^ pinBit) & pinBit);
        writeRegister(OUTPUT_PORT_0, values, 2);

        return ytError(YTNoError);
    }
    else
    {
        return ytError(YTBadRequest);        
    }
}

enum PinOutput PCAL64PinGetValue(enum PCAL64PinName pinName)
{
    uint16_t pinBit = 1 << pinName;
    uint16_t values;

    readRegister(INPUT_PORT_0, &values, 2);

    return (values & pinBit);
}


/*
    Setup interrupt handling
*/

YTError PCAL64PinSetCallback(enum PCAL64PinName pinName, enum PinTransition transition, yt_callback_t callback)
{
    if ((pinName < Pin_End) && (callback))
    {
        callbacks[pinName] = callback;
        transitions[pinName] = transition;

        uint16_t pinBit = 1 << pinName;
        uint16_t interruptMask = 0;
        uint16_t direction = 0;

        readRegister(CONFIGURATION_PORT_0, &direction, 2);
        // only write register if direction is not already set to out
        if (!(direction & pinBit))
        {
            direction |= pinBit;
            writeRegister(CONFIGURATION_PORT_0, direction, 2);
        }

        readRegister(INTERRUPT_MASK_0, &interruptMask, 2);
        // only clear interrupt mask if not already cleared
        if (interruptMask & pinBit)
        {
            interruptMask &= ~pinBit;
            writeRegister(INTERRUPT_MASK_0, interruptMask, 2);            
        }

        return ytError(YTNoError);
    }
    else
    {
        return ytError(YTBadRequest);        
    }
}

YTError PCAL64PinRemoveCallback(enum PCAL64PinName pinName)
{
    if (pinName < Pin_End)
    {
        uint16_t pinBit = 1 << pinName;
        uint16_t interruptMask = 0;

        readRegister(INTERRUPT_MASK_0, &interruptMask, 2);

        // only set interrupt mask if not already set
        if (!(interruptMask & pinBit))
        {
            interruptMask |= pinBit;
            writeRegister(INTERRUPT_MASK_0, interruptMask, 2);            
        }

        return ytError(YTNoError);
    }
    else
    {
        return ytError(YTBadRequest);        
    }
}

/* 
    Read / Write register over I2C
*/
static YTError readRegister(uint8_t address, uint16_t* value, uint8_t rxLength)
{
    if (rxLength < 3)
    {
        /* setup I2C transfer */
        i2cTransfer.flags = I2C_FLAG_WRITE_READ;

        i2cTransfer.buf[0].data[0] = address;
        i2cTransfer.buf[0].len = 1;

        i2cTransfer.buf[1].data[0] = 0;
        i2cTransfer.buf[1].data[1] = 0;
        i2cTransfer.buf[1].len = rxLength;

        I2C_TransferInit(I2C0, &i2cTransfer);
        
        /* Sending data */ 
        while (I2C_Transfer(I2C0) == i2cTransferInProgress)
        {
            ;
        }  

        *value = i2cTransfer.buf[1].data[1];
        *value = (*value << 8) | i2cTransfer.buf[1].data[0];

        return ytError(YTNoError);
    }
    else
    {
        return ytError(YTBadRequest);        
    }
}

static YTError writeRegister(uint8_t address, uint16_t value, uint8_t txLength)
{
    if (txLength < 3)
    {
        /* setup I2C transfer */
        i2cTransfer.flags = I2C_FLAG_WRITE;

        i2cTransfer.buf[0].data[0] = address;
        i2cTransfer.buf[0].data[1] = value;
        i2cTransfer.buf[0].data[2] = value >> 8;
        i2cTransfer.buf[0].len = txLength + 1;

        i2cTransfer.buf[1].len = 0;

        I2C_TransferInit(I2C0, &i2cTransfer);
        
        /* Sending data */ 
        while (I2C_Transfer(I2C0) == i2cTransferInProgress)
        {
            ;
        }  

        return ytError(YTNoError);
    }
    else
    {
        return ytError(YTBadRequest);        
    }
}


void I2C0_IRQHandler()
{
//    GPIO_PinOutToggle(gpioPortA, 1);
}

void I2C1_IRQHandler()
{
    
}


#if 0
/**
 * @brief
 *   Indicate plain write sequence: S+ADDR(W)+DATA0+P.
 * @details
 *   @li S - Start
 *   @li ADDR(W) - address with W/R bit cleared
 *   @li DATA0 - Data taken from buffer with index 0
 *   @li P - Stop
 */
#define I2C_FLAG_WRITE          0x0001

/**
 * @brief
 *   Indicate plain read sequence: S+ADDR(R)+DATA0+P.
 * @details
 *   @li S - Start
 *   @li ADDR(R) - address with W/R bit set
 *   @li DATA0 - Data read into buffer with index 0
 *   @li P - Stop
 */
#define I2C_FLAG_READ           0x0002

/**
 * @brief
 *   Indicate combined write/read sequence: S+ADDR(W)+DATA0+Sr+ADDR(R)+DATA1+P.
 * @details
 *   @li S - Start
 *   @li Sr - Repeated start
 *   @li ADDR(W) - address with W/R bit cleared
 *   @li ADDR(R) - address with W/R bit set
 *   @li DATAn - Data written from/read into buffer with index n
 *   @li P - Stop
 */
#define I2C_FLAG_WRITE_READ     0x0004

/**
 * @brief
 *   Indicate write sequence using two buffers: S+ADDR(W)+DATA0+DATA1+P.
 * @details
 *   @li S - Start
 *   @li ADDR(W) - address with W/R bit cleared
 *   @li DATAn - Data written from buffer with index n
 *   @li P - Stop
 */
#define I2C_FLAG_WRITE_WRITE    0x0008

/** Use 10 bit address. */
#define I2C_FLAG_10BIT_ADDR     0x0010



#endif

