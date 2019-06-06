

#ifndef __ARDUINO_H__
#define __ARDUINO_H__

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "stdutils.h"
#include "gpio.h"
#include "adc.h"
#include "systick.h"
#include "timer.h"
#include "delay.h"
#include "reset.h"


#include "interrupt_lpc.h"


#include "ecv.h"        // included to compile rrf
#undef yield            // eCv definition clashes with function 'yield' in wiring.c (can use _ecv_yield instead within annotations)


extern void loop();
extern void setup();
extern void init( void );

#ifdef __cplusplus
extern "C"
{
#endif
    void CoreSysTick(void);
#ifdef __cplusplus
}// end exten "C"
#endif

typedef gpioPins_et Pin;
static const Pin NoPin = P_NC; //which =0xff

#include "exti.h"
#include "board.h"
#include "wiring_digital.h"
#include "wirish_time.h"
#include "wdt.h"

#ifdef __cplusplus

// Pin Attributes to be OR-ed
constexpr uint8_t PIN_ATTR_NONE = 0;
constexpr uint8_t PIN_ATTR_COMBO = 1 << 0;
constexpr uint8_t PIN_ATTR_ANALOG = 1 << 1;
constexpr uint8_t PIN_ATTR_DIGITAL = 1 << 2;
constexpr uint8_t PIN_ATTR_PWM = 1 << 3;
constexpr uint8_t PIN_ATTR_TIMER = 1 << 4;
constexpr uint8_t PIN_ATTR_DAC = 1 << 5;


// Usage:
// 1. Enable the channels you need by making calls to AnalogEnableChannel.
// 2. If desired, call AnalogSetCallback to set a callback for when conversion is complete.
// 3. Call AnalogStartConversion. This may be done e.g. in a tick interrupt if regular conversions are wanted.
// 4. Either use the callback to determine when conversion is complete, or call AnalogCheckReady to poll the status.
// 5. Call AnalogReadChannel to read the most recent converted result for each channel of interest.

enum AnalogChannelNumber : int8_t
{
    NO_ADC=-1,
    ADC0=0,
    ADC1,
    ADC2,
    ADC3,
    ADC4,
    ADC5,
    ADC6,
    ADC7
};

// Definitions for PWM channels
enum EPWMChannel : int8_t
{
    NOT_ON_PWM=-1,
    PWM1_1=0,
    PWM1_2,
    PWM1_3,
    PWM1_4,
    PWM1_5,
    PWM1_6,
};


struct PinDescription{
    uint8_t pPort;
    uint8_t ulPinAttribute;
    AnalogChannelNumber ulADCChannelNumber; // ADC or DAC channel number
    EPWMChannel ulPWMChannel;
};

/* Pins table to be instantiated into variant.cpp */
constexpr uint32_t MaxPinNumber = 160;// 5*32
extern const PinDescription g_APinDescription[MaxPinNumber];


enum SSPChannel {
    SSP0 = 0,
    SSP1
};


#include "WCharacter.h"
#include "HardwareSerial.h"
#include "Stream.h"
#include "usb_serial.h"
#include "WMath.h"
#include "AnalogIn.h"
#include "AnalogOut.h"

#endif // __cplusplus


#include "variant.h"


#ifndef __STATIC_INLINE
 #define __STATIC_INLINE  static inline
#endif


#endif /* __ARDUINO_H__ */

/*lint -restore */
