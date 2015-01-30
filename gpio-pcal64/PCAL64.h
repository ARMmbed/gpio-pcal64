#ifndef __PCAL64_H__
#define __PCAL64_H__

#import "gpio/GPIO.h"
#import "yterror/YTError.h"


enum PCAL64PinName{
    Pin_P0_0,
    Pin_P0_1,
    Pin_P0_2,
    Pin_P0_3,
    Pin_P0_4,
    Pin_P0_5,
    Pin_P0_6,
    Pin_P0_7,

    Pin_P1_0,
    Pin_P1_1,
    Pin_P1_2,
    Pin_P1_3,
    Pin_P1_4,
    Pin_P1_5,
    Pin_P1_6,
    Pin_P1_7
};


void PCAL64Init(PinNameIntType interruptPin);

void PCAL64PinSetMode(enum PCAL64PinName pin, enum PinMode mode, enum PinOutput value);
void PCAL64PinSetOutput(enum PCAL64PinName pin, enum PinOutput value);

enum PinOutput PCAL64PinGetValue(enum PCAL64PinName pin);

YTError PCAL64PinSetCallback(enum PCAL64PinName pin, enum PinTransition transition, yt_callback_t callback);




#endif // __PCAL64_H__