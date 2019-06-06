/***************************************************************************************************
                                   ExploreEmbedded    
****************************************************************************************************
 * File:   systickHandler.c
 * Version: 16.0
 * Author: ExploreEmbedded
 * Website: http://www.exploreembedded.com/wiki
 * Description: File contains the functions for configuring and using the LPC1768 systick Timer.
                 

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
**************************************************************************************************/
#include <lpc17xx.h>
#include "systick.h"
#include "stdutils.h"
#include "Core.h"
#include "wdt.h"
#include "wirish_time.h"


static volatile uint32_t V_SysTickMiliSecCount_U32 = 0;
static volatile uint64_t V_SysTickMiliSecCount_U64 = 0;
//sysTickCallBackFunPtr sysTickCallBack = NULL;


/***************************************************************************************************
                             void SysTick_Init(void)
****************************************************************************************************
 * I/P Arguments: none


 * Return value: none

 * description :This function initializes the systick timer with 1ms tick time.
                1ms timer value for 100Mhz clock.
                COUNT_PER_MS = 100Mhz / 1000(ticks/sec) - 1 = 100000000/1000 - 1 = 99999; 
                
 
 ##Note: This function will only initializes the systick timer with 1ms time.
         SysTick_Start() fuctions needs to be called to start the timer.                                  
****************************************************************************************************/


void SysTick_Init(void)
{   
    SYSTICK_RELOAD = COUNT_PER_MS;
    
}




/***************************************************************************************************
                 void SysTick_Start(void)
****************************************************************************************************
 * I/P Arguments: none

 * Return value: none

 * description :This function turns on the systick timer and enables the interrupt.
  
 ##Note: Before calling the SysTick_Start function the timers should to be initialized by calling SysTick_Init()                  
****************************************************************************************************/
void SysTick_Start(void)
{
    SYSTICK_CTRL = (1<<SBIT_ENABLE) | (1<<SBIT_TICKINT) | (1<<SBIT_CLKSOURCE);
}






/***************************************************************************************************
                           void SysTick_Stop(void)
****************************************************************************************************
 * I/P Arguments: none

 * Return value: none

 * description :This function turns OFF the SysTick timer.                 
****************************************************************************************************/
void SysTick_Stop(void)
{
  SYSTICK_CTRL = 0x00;
}





/***************************************************************************************************
                void SysTick_AttachInterrupt(sysTickCallBackFunPtr funPtr)
****************************************************************************************************
 * I/P Arguments:
               sysTickCallBackFunPtr: Function name thats needs to be called by the SysTickHandler ISR.
                             The function parameter and return type should be void as shown below.
                             void MyFunToBeCalledByISR(void);

 * Return value: none                              

 * description :This functions attaches/hookes the user callback function to ISR.
                Ever time the interrupt occurs the ISR will call the user CallBack function.                 
****************************************************************************************************/
/*void SysTick_AttachInterrupt(sysTickCallBackFunPtr funPtr)
{
   sysTickCallBack = funPtr;
}*/












/***************************************************************************************************
               uint32_t SysTick_GetMsTime(void)
****************************************************************************************************
 * I/P Arguments: none

 * Return value: None 

 * description :This functions returns the time in ms since the power on.
                Max time=0xffffffff ms after that it rolls back to 0. 
****************************************************************************************************/
uint32_t SysTick_GetMsTime(void)
{
   return V_SysTickMiliSecCount_U32;
}

uint32_t millis(void)
{
   return (uint32_t) V_SysTickMiliSecCount_U64;
}

uint64_t millis64( void ){
    return V_SysTickMiliSecCount_U64;
    
}








//Micros commented out for RTOS as Systick is not being used.

//uint32_t micros( void )
//{
//    uint32_t ticks, ticks2;
//    uint32_t pend, pend2;
//    uint32_t count, count2;
//
//    ticks2  = SysTick->VAL;
//    pend2   = !!((SCB->ICSR & SCB_ICSR_PENDSTSET_Msk)||((SCB->SHCSR & SCB_SHCSR_SYSTICKACT_Msk)))  ;
//    count2  = (uint32_t)V_SysTickMiliSecCount_U64;
//
//    do {
//        ticks=ticks2;
//        pend=pend2;
//        count=count2;
//        ticks2  = SysTick->VAL;
//        pend2   = !!((SCB->ICSR & SCB_ICSR_PENDSTSET_Msk)||((SCB->SHCSR & SCB_SHCSR_SYSTICKACT_Msk)))  ;
//        count2  = (uint32_t)V_SysTickMiliSecCount_U64;
//    } while ((pend != pend2) || (count != count2) || (ticks < ticks2));
//
//    return ((count+pend) * 1000) + (((SysTick->LOAD  - ticks)*(1048576/(VARIANT_MCK/1000000)))>>20) ;
//    // this is an optimization to turn a runtime division into two compile-time divisions and
//    // a runtime multiplication and shift, saving a few cycles
//}





/***************************************************************************************************
                            SysTick ISR's
****************************************************************************************************
 desc: SysTick_Handler will be called every ms. A counter will be incremented to keep track of Ms.                 
       If the user CallBack Function is configured then it will be called. 
                                 
****************************************************************************************************/


//extern void TimeTick_Increment(void);                // in wiring.c




//#ifdef RTOS
//void vApplicationTickHook(void)
//#else
//void SysTick_Handler(void)
//#endif
//{
//    V_SysTickMiliSecCount_U32++;
//    V_SysTickMiliSecCount_U64++;
//
//    sysTickHook();
//
//    wdt_restart(WDT);                            // kick the watchdog
//
//}

void CoreSysTick(void)
{
    const irqflags_t flags = cpu_irq_save();    // save and disable interrupts, because under RTOS the systick interrupt is low priority
    V_SysTickMiliSecCount_U32++;
    V_SysTickMiliSecCount_U64++;
    cpu_irq_restore(flags);

}


/*************************************************************************************************
                                    END of  ISR's 
*************************************************************************************************/
