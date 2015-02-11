// Copyright (C) 2013 ARM Limited. All rights reserved.
#import "yottos/yottos.h"

#ifdef TARGET_LIKE_OBJECTADOR
#import "emlib/em_cmu.h"
#endif


#import "gpio-pcal64/PCAL64Pin.h"


yt_callback_t ioIrqHandler =
^{
//    GPIO_PinOutToggle(gpioPortA, 1);
};



yt_callback_t startTask = 
^{
    id <GPIOPinProtocol> led = [[PCAL64Pin alloc] initWithPin:Pin_P0_3 andIrqPin:Pin_PC3];
    id <GPIOPinProtocol> irq = [[PCAL64Pin alloc] initWithPin:Pin_P0_2 andIrqPin:Pin_PC3];

    [led setLow];
    [led setOutput];

    [irq attachCallback:ioIrqHandler onEdge:GPIOEdgeAny];

//    PCAL64SetStrength(0);
//    PCAL64PinSetMode(0xffff, Mode_Pin_Output, 0x0000);

    [Yottos postCallback:^{
                [led toggle];
            }
            withPeriod:ytMilliseconds(1000)
            andTolerance:ytMilliseconds(10)];

};


int main(){
#ifdef TARGET_LIKE_OBJECTADOR
    
    // Use crystal oscillator for HFXO
    CMU->CTRL |= CMU_CTRL_HFXOMODE_XTAL;
    // HFXO setup
    CMU->CTRL = (CMU->CTRL & ~_CMU_CTRL_HFXOBOOST_MASK) 
              | CMU_CTRL_HFXOBOOST_100PCENT;
            
    // Enable HFXO as high frequency clock, HFCLK
    CMU_ClockSelectSet(cmuClock_HF,cmuSelect_HFXO);

    // Use crystal oscillator for LFXO
    CMU->CTRL |= CMU_CTRL_LFXOMODE_XTAL;

    // LFXO setup
    CMU->CTRL    = (CMU->CTRL & ~_CMU_CTRL_LFXOBOOST_MASK) 
                | CMU_CTRL_LFXOBOOST_70PCENT;
    EMU->AUXCTRL = (EMU->AUXCTRL & ~_EMU_AUXCTRL_REDLFXOBOOST_MASK) 
                | EMU_AUXCTRL_REDLFXOBOOST;
            
    // Enable LFXO and wait for it to stabilize 
    CMU_OscillatorEnable(cmuOsc_LFXO, true, true);

    // Select LFXO as clock source for LFACLK
    CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFXO);

#endif

    // debug
//    GPIO_PinModeSet(gpioPortA, 1, gpioModeWiredAndPullUp, 1);
    GPIO_PinModeSet(gpioPortA, 2, gpioModeWiredAndPullUp, 1);


    [Yottos postCallback:startTask withTolerance:ytMilliseconds(100)];

    [Yottos start];

    return 1;
}
