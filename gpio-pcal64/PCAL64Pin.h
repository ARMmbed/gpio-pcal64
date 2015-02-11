#import "Foundation/NSObject.h"
#import "gpio/GPIO.h"
#import "yterror/YTError.h"

#import "gpio/GPIOPinProtocol.h"

enum PCAL64PinName {
    Pin_P0_0 = 0,
    Pin_P0_1 = 1,
    Pin_P0_2 = 2,
    Pin_P0_3 = 3,
    Pin_P0_4 = 4,
    Pin_P0_5 = 5,
    Pin_P0_6 = 6,
    Pin_P0_7 = 7,

    Pin_P1_0 = 8,
    Pin_P1_1 = 9,
    Pin_P1_2 = 10,
    Pin_P1_3 = 11,
    Pin_P1_4 = 12,
    Pin_P1_5 = 13,
    Pin_P1_6 = 14,
    Pin_P1_7 = 15,
    Pin_End
};

enum PCAL64PinMode {
    Mode_Pin_Input,
    Mode_Pin_Output
};

void PCAL64Init(PinNameIntType interruptPin);

// Get/Set all pins
void PCAL64SetMode(uint16_t modes, uint16_t values);
void PCAL64SetOutput(uint16_t values);
uint16_t PCAL64GetValue();
void PCAL64SetStrength(uint32_t values);

// Get/Set individual pins
YTError PCAL64PinSetMode(enum PCAL64PinName pin, enum PCAL64PinMode mode, enum PinOutput value);
YTError PCAL64PinSetOutput(enum PCAL64PinName pin, enum PinOutput value);
YTError PCAL64PinToggle(enum PCAL64PinName pin);
enum PinOutput PCAL64PinGetValue(enum PCAL64PinName pin);

YTError PCAL64PinSetCallback(enum PCAL64PinName pinName, enum GPIOEdge transition, yt_callback_t callback);
YTError PCAL64PinRemoveCallback(enum PCAL64PinName pinName);


@interface PCAL64Pin : NSObject <GPIOPinProtocol>

- (id)initWithPin:(enum PCAL64PinName) _pin
        andIrqPin:(PinNameIntType) interruptPin;

@end

