// Copyright (C) 2013 ARM Limited. All rights reserved.
#import "yottos/yottos.h"

#ifdef TARGET_LIKE_OBJECTADOR
#import "emlib/em_cmu.h"
#endif


#import "gpio-pcal64/PCAL64.h"


yt_callback_t startTask = ^
{
    PCAL64Init(Pin_PA1);
    PCAL64PinSetMode(Pin_P0_2, Pin_Input, Pin_High);
};


int main(){
#ifdef TARGET_LIKE_OBJECTADOR
    
  // Use crystal oscillator for HFXO
  CMU->CTRL |= CMU_CTRL_HFXOMODE_XTAL;
  // HFXO setup
  CMU->CTRL  = (CMU->CTRL & ~_CMU_CTRL_HFXOBOOST_MASK) 
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

  [Yottos postCallback:startTask withTolerance:ytMilliseconds(100)];

  [Yottos start];

  return 1;
}
