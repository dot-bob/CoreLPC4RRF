/***************************************************************************************************
                                   ExploreEmbedded    
*****************************************************************************************************
 * File:   extintr.c
 * Version: 16.0
 * Author: ExploreEmbedded
 * Website: http://www.exploreembedded.com/wiki
 * Description: File contains the functions for configuring and using the LPC1768 External Interrupts.


The libraries have been tested on ExploreEmbedded development boards. We strongly believe that the 
library works on any of development boards for respective controllers. However, ExploreEmbedded 
disclaims any kind of hardware failure resulting out of usage of libraries, directly or indirectly.
Files may be subject to change without prior notice. The revision history contains the information 
related to updates. 


GNU GENERAL PUBLIC LICENSE: 
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>. 

Errors and omissions should be reported to codelibraries@exploreembedded.com
 
 SD: Modified to handle GPIO interrupts and support RRF
 
***************************************************************************************************/
#include "lpc17xx.h"
#include "exti.h"
#include "stdutils.h"
#include "gpio.h"
#include "board.h"
#include "core.h"


typedef void (*interruptCB)(CallbackParameter);

struct InterruptCallback
{
    interruptCB func;
    void *param;
    InterruptCallback() : func(nullptr) { }

};


struct CallbackEntry{
    InterruptCallback intCallback;
};


//support only 3 callbacks to save memory
static CallbackEntry callbacks[MaxExtIntEntries] __attribute__((section ("AHBSRAM0")));
Pin ExternalInterruptPins[MaxExtIntEntries] = {NoPin, NoPin, NoPin};


//Function from WInterrupts from RRF.

// Get the number of the highest bit that is set in a 32-bit word. Returns 0 if no bit set (same as if lowest bit is set).
// This needs to be fast. Hopefully the ARM conditional instructions will be used to advantage here.
static unsigned int GetHighestBit(uint32_t bits)
{
    unsigned int bitNum = (bits >= 0x00010000) ? 16 : 0;
    if ((bits >> bitNum) >= 0x0100u)
    {
        bitNum += 8;
    }
    if ((bits >> bitNum) >= 0x0010u)
    {
        bitNum += 4;
    }
    if ((bits >> bitNum) >= 0x0004u)
    {
        bitNum += 2;
    }
    if ((bits >> bitNum) >= 0x0002u)
    {
        bitNum += 1;
    }
    return bitNum;
}


bool attachInterrupt(Pin pin, void (*callback)(CallbackParameter), enum InterruptMode mode, void *param)
{
    
    
    //Port 0 and Port 2 can provide a single interrupt for any combination of port pins.
    //GPIO INTS call EINT3 handler!!

    uint8_t portNumber;
    uint8_t var_pinNumber_u8;
    
    portNumber =  (pin>>5);  //Divide the pin number by 32 go get the PORT number
    var_pinNumber_u8  =   pin & 0x1f;  //lower 5-bits contains the bit number of a 32bit port


    // Set callback function and parameter

    size_t slot = MaxExtIntEntries;
    for(size_t i=0; i<MaxExtIntEntries; i++)
    {
        if(ExternalInterruptPins[i] == pin){
            slot = i; //found slot
            break;
        }
    }

    //ensure pin is on Port 0 or Port 2
    //Each port pin can be programmed to generate an interrupt on a rising edge, a falling edge, or both.
    if(slot < MaxExtIntEntries && (portNumber == 0 || portNumber == 2) )
    {
        const irqflags_t flags = cpu_irq_save();

        callbacks[slot].intCallback.func = callback;
        callbacks[slot].intCallback.param = param;
        
        NVIC_EnableIRQ(EINT3_IRQn); // GPIO interrupts on P0 and P2 trigger EINT3 handler

        switch(mode)
        {
            case INTERRUPT_MODE_LOW:
                //LOW level int not implemented in GPIO
                break;
                
            case INTERRUPT_MODE_HIGH:
                //HIGH level int not implemented in GPIO
                break;
                
            case INTERRUPT_MODE_FALLING:
                if(portNumber == 0){
                    util_BitSet(LPC_GPIOINT->IO0IntEnF, var_pinNumber_u8); // set Falling Interrupt bit for pin
                    util_BitClear(LPC_GPIOINT->IO0IntEnR, var_pinNumber_u8); //ensure Rising disabled
                }
                if(portNumber == 2){
                    util_BitSet(LPC_GPIOINT->IO2IntEnF, var_pinNumber_u8);
                    util_BitClear(LPC_GPIOINT->IO2IntEnR, var_pinNumber_u8); //ensure Rising disabled
                }

                break;
                
            case INTERRUPT_MODE_RISING:
                if(portNumber == 0) {
                    util_BitSet(LPC_GPIOINT->IO0IntEnR, var_pinNumber_u8);
                    util_BitClear(LPC_GPIOINT->IO0IntEnF, var_pinNumber_u8); // ensure Falling disabled
                }
                if(portNumber == 2) {
                    util_BitSet(LPC_GPIOINT->IO2IntEnR, var_pinNumber_u8);
                    util_BitClear(LPC_GPIOINT->IO2IntEnF, var_pinNumber_u8); // ensure Falling disabled
                }

                break;
            case INTERRUPT_MODE_CHANGE:
                //Rising and Falling
                if(portNumber == 0){
                    util_BitSet(LPC_GPIOINT->IO0IntEnF, var_pinNumber_u8); //Falling
                    util_BitSet(LPC_GPIOINT->IO0IntEnR, var_pinNumber_u8); //Rising
                }
                if(portNumber == 2){
                    util_BitSet(LPC_GPIOINT->IO2IntEnF, var_pinNumber_u8); //Falling
                    util_BitSet(LPC_GPIOINT->IO2IntEnR, var_pinNumber_u8); //Rising
                }


                break;
            default:
                break;
                
        }

        cpu_irq_restore(flags);
        
    } else {
        
        return false; // no interrupts avail on this pin
    }

    
    return true;
}


void detachInterrupt(Pin pin){
 
    uint8_t portNumber;
    uint8_t var_pinNumber_u8;
    
    portNumber =  (pin>>5);  //Divide the pin number by 32 go get the PORT number
    var_pinNumber_u8  =   pin & 0x1f;  //lower 5-bits contains the bit number of a 32bit port

    //clear Rise and Fall
    if(portNumber == 0){
        util_BitClear(LPC_GPIOINT->IO0IntEnF, var_pinNumber_u8); //Falling
        util_BitClear(LPC_GPIOINT->IO0IntEnR, var_pinNumber_u8); //Rising
    }
    if(portNumber == 2){
        util_BitClear(LPC_GPIOINT->IO2IntEnF, var_pinNumber_u8); //Falling
        util_BitClear(LPC_GPIOINT->IO2IntEnR, var_pinNumber_u8); //Rising
    }

    
    
}

/** \brief  Get IPSR Register

 This function returns the content of the IPSR Register.

 \return               IPSR Register value
 */
__attribute__( ( always_inline ) ) static inline uint32_t __get_IPSR(void)
{
    uint32_t result;

    __ASM volatile ("MRS %0, ipsr" : "=r" (result) );
    return(result);
}


// Return true if we are in any interrupt service routine
bool inInterrupt()
{
    //bits 0:8 are the ISR_NUMBER
    //bits 9:31 reserved
    return (__get_IPSR() & 0xFF) != 0;
}

extern "C" void EINT3_IRQHandler(void)
{
    //We assume we arent also using EINT3 (external interrupt function), but only the GPIO interrupts which share the same interrupt
    
    //Look for Rising And Falling interrupt for both ports
    //Since we dont need to do anything different for rise/fall, we treat the same
    uint32_t isr0 = LPC_GPIOINT->IO0IntStatR | LPC_GPIOINT->IO0IntStatF; // get all pins rise and fall which triggered int
    uint32_t isr2 = LPC_GPIOINT->IO2IntStatR | LPC_GPIOINT->IO2IntStatF; // get all pins rise and fall which triggered int

    //port 0
    while (isr0 != 0)
    {
        const unsigned int pos0 = GetHighestBit(isr0);
        LPC_GPIOINT->IO0IntClr |= (1 << pos0); // clear the status
        
        //Find the slot for this pin
        const Pin pin= (Pin)(pos0);// port 0, this is just the pin number
        size_t slot = MaxExtIntEntries;
        for(size_t i=0; i<MaxExtIntEntries; i++)
        {
            if(ExternalInterruptPins[i] == pin){
                slot = i; //found slot
                break;
            }
        }
        
        if(slot < MaxExtIntEntries && callbacks[slot].intCallback.func != NULL)
        {
            callbacks[slot].intCallback.func(callbacks[slot].intCallback.param);

        }

        isr0 &= ~(1u << pos0);
    }
    //port 2
    while (isr2 != 0)
    {
        const unsigned int pos2 = GetHighestBit(isr2);
        LPC_GPIOINT->IO2IntClr |= (1 << pos2); // clear the status

        //Find the slot for this pin
        const Pin pin = (Pin) ((2 << 5) | (pos2));// pin on port 2
        size_t slot = MaxExtIntEntries;
        for(size_t i=0; i<MaxExtIntEntries; i++)
        {
            if(ExternalInterruptPins[i] == pin){
                slot = i; //found slot
                break;
            }
        }

        
        if(slot < MaxExtIntEntries && callbacks[slot].intCallback.func != NULL)
        {
            callbacks[slot].intCallback.func(callbacks[slot].intCallback.param);
        }


        isr0 &= ~(1u << pos2);
    }

    
    
}

/*************************************************************************************************
                                    END of  ISR's 
**************************************************************************************************/
