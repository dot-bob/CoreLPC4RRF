
#include "lpc17xx.h"

#include "ring_buffer.h"
#include "uart_17xx_40xx.h"

#include "usart.h"
#include "gpio.h"
#include "stdutils.h"
#include <inttypes.h>
#include "Core.h"


static const usart_channel_map USART_BASE[4]=
{  /* TxPin RxPin UART_PinFun   PCON Bit Associated UART Structure    */
    { P0_2,  P0_3,    PINSEL_FUNC_1,  3     ,(LPC_USART_T *)LPC_UART0_BASE}, /* Configure P0_2,P0_3 for UART0 function */
    { P0_15, P0_16,   PINSEL_FUNC_1,  4     ,(LPC_USART_T *)LPC_UART1_BASE}, /* Configure P2_0,P2_1 for UART1 function */
    { P0_10, P0_11,   PINSEL_FUNC_1,  24    ,(LPC_USART_T *)LPC_UART2_BASE}, /* Configure P0_10,P0_11 for UART2 function */
    { P0_0,  P0_1,    PINSEL_FUNC_2,  25    ,(LPC_USART_T *)LPC_UART3_BASE}  /* Configure P4_28,P4_29 for UART3 function */
};


static const usart_dev usart0 = {
    .channel     = &USART_BASE[0],
    .max_baud = 4500000UL,
    .irq_NUM = UART0_IRQn,
};
const usart_dev *USART0 = &usart0;


//UART Interrupt Handler
extern "C" void UART0_IRQHandler(void)
{
    Serial0.IRQHandler();
}




/*
static usart_dev usart1 = {
    .channel     = &USART_BASE[1],
    .baud_rate = 0,
    .max_baud = 4500001UL,
    .irq_NUM = UART1_IRQn,
    .userFunction = usart1_IRQHandler,
};
usart_dev *USART1 = &usart1;




static usart_dev usart2 = {
    .channel     = &USART_BASE[2],
    .baud_rate = 0,
    .max_baud = 2250001UL,
    .irq_NUM = UART2_IRQn,
    .userFunction = usart2_IRQHandler,
};
usart_dev *USART2 = &usart2;




static usart_dev usart3 = {
    .channel     = &USART_BASE[3],
    .baud_rate = 0,
    .max_baud = 2250002UL,
    .irq_NUM = UART3_IRQn,
    .userFunction = usart3_IRQHandler,
};
usart_dev *USART3 = &usart3;

*/


 

//Serial_baud from MBED
void serial_baud(LPC_USART_T *obj, int baudrate) {

    // set pclk to /1
    if( obj == LPCOPEN_LPC_UART0) { LPC_SC->PCLKSEL0 &= ~(0x3 <<  6); LPC_SC->PCLKSEL0 |= (0x1 <<  6);}
    else if( obj == LPCOPEN_LPC_UART1){ LPC_SC->PCLKSEL0 &= ~(0x3 <<  8); LPC_SC->PCLKSEL0 |= (0x1 <<  8); }
    else if( obj == LPCOPEN_LPC_UART2){ LPC_SC->PCLKSEL1 &= ~(0x3 << 16); LPC_SC->PCLKSEL1 |= (0x1 << 16); }
    else if( obj == LPCOPEN_LPC_UART0){ LPC_SC->PCLKSEL1 &= ~(0x3 << 18); LPC_SC->PCLKSEL1 |= (0x1 << 18); }
    else {
        //error("serial_baud");
        return;
        
    }
    
    if(baudrate == 0) return;
    
    
    uint32_t PCLK = SystemCoreClock;
    
    // First we check to see if the basic divide with no DivAddVal/MulVal
    // ratio gives us an integer result. If it does, we set DivAddVal = 0,
    // MulVal = 1. Otherwise, we search the valid ratio value range to find
    // the closest match. This could be more elegant, using search methods
    // and/or lookup tables, but the brute force method is not that much
    // slower, and is more maintainable.
    uint16_t DL = PCLK / (16 * baudrate);
    
    uint8_t DivAddVal = 0;
    uint8_t MulVal = 1;
    int hit = 0;
    uint16_t dlv;
    uint8_t mv, dav;
    if ((PCLK % (16 * baudrate)) != 0) {     // Checking for zero remainder
        int err_best = baudrate, b;
        for (mv = 1; mv < 16 && !hit; mv++)
        {
            for (dav = 0; dav < mv; dav++)
            {
                // baudrate = PCLK / (16 * dlv * (1 + (DivAdd / Mul))
                // solving for dlv, we get dlv = mul * PCLK / (16 * baudrate * (divadd + mul))
                // mul has 4 bits, PCLK has 27 so we have 1 bit headroom which can be used for rounding
                // for many values of mul and PCLK we have 2 or more bits of headroom which can be used to improve precision
                // note: X / 32 doesn't round correctly. Instead, we use ((X / 16) + 1) / 2 for correct rounding
                
                if ((mv * PCLK * 2) & 0x80000000) // 1 bit headroom
                    dlv = ((((2 * mv * PCLK) / (baudrate * (dav + mv))) / 16) + 1) / 2;
                else // 2 bits headroom, use more precision
                    dlv = ((((4 * mv * PCLK) / (baudrate * (dav + mv))) / 32) + 1) / 2;
                
                // datasheet says if DLL==DLM==0, then 1 is used instead since divide by zero is ungood
                if (dlv == 0)
                    dlv = 1;
                
                // datasheet says if dav > 0 then DL must be >= 2
                if ((dav > 0) && (dlv < 2))
                    dlv = 2;
                
                // integer rearrangement of the baudrate equation (with rounding)
                b = ((PCLK * mv / (dlv * (dav + mv) * 8)) + 1) / 2;
                
                // check to see how we went
                b = abs(b - baudrate);
                if (b < err_best)
                {
                    err_best  = b;
                    
                    DL        = dlv;
                    MulVal    = mv;
                    DivAddVal = dav;
                    
                    if (b == baudrate)
                    {
                        hit = 1;
                        break;
                    }
                }
            }
        }
    }
    
    // set LCR[DLAB] to enable writing to divider registers
    obj->LCR |= (1 << 7);
    
    // set divider values
    obj->DLM = (DL >> 8) & 0xFF;
    obj->DLL = (DL >> 0) & 0xFF;
    obj->FDR = (uint32_t) DivAddVal << 0
    | (uint32_t) MulVal    << 4;
    
    // clear LCR[DLAB]
    obj->LCR &= ~(1 << 7);
}

typedef enum {
    ParityNone = 0,
    ParityOdd = 1,
    ParityEven = 2,
    ParityForced1 = 3,
    ParityForced0 = 4
} SerialParity;

void serial_format(LPC_USART_T *obj, int data_bits, SerialParity parity, int stop_bits) {
    // 5 data bits = 0 ... 8 data bits = 3
    if (data_bits < 5 || data_bits > 8) {
        //error("Invalid number of bits (%d) in serial format, should be 5..8", data_bits);
        return;
    }

    data_bits -= 5;

    int parity_enable, parity_select;
    switch (parity) {
        case ParityNone: parity_enable = 0; parity_select = 0; break;
        case ParityOdd : parity_enable = 1; parity_select = 0; break;
        case ParityEven: parity_enable = 1; parity_select = 1; break;
        case ParityForced1: parity_enable = 1; parity_select = 2; break;
        case ParityForced0: parity_enable = 1; parity_select = 3; break;
        default:
            //error("Invalid serial parity setting");
            return;
    }

    // 1 stop bits = 0, 2 stop bits = 1
    if (stop_bits != 1 && stop_bits != 2) {
        //error("Invalid stop bits specified");
        return;
    }
    stop_bits -= 1;

    int break_transmission   = 0; // 0 = Disable, 1 = Enable
    int divisor_latch_access = 0; // 0 = Disable, 1 = Enable
    obj->LCR = data_bits << 0
    | stop_bits << 2
    | parity_enable << 3
    | parity_select << 4
    | break_transmission << 6
    | divisor_latch_access << 7;
}



void usart_init(const usart_dev *dev, uint32_t baud_rate) {

    GPIO_PinFunction(dev->channel->TxPin,dev->channel->PinFunSel);
    GPIO_PinFunction(dev->channel->RxPin,dev->channel->PinFunSel);
    util_BitSet(LPC_SC->PCONP,dev->channel->pconBit);
    
    /* Enable FIFOs by default, reset them */
    Chip_UART_SetupFIFOS(dev->channel->UARTx, (UART_FCR_FIFO_EN | UART_FCR_RX_RS | UART_FCR_TX_RS));

    /* Disable Tx */
    Chip_UART_TXDisable(dev->channel->UARTx);
    
    /* Disable interrupts */
    dev->channel->UARTx->IER = 0;
    /* Set LCR to default state */
    dev->channel->UARTx->LCR = 0;
    /* Set ACR to default state */
    dev->channel->UARTx->ACR = 0;
    /* Set RS485 control to default state */
    dev->channel->UARTx->RS485CTRL = 0;
    /* Set RS485 delay timer to default state */
    dev->channel->UARTx->RS485DLY = 0;
    /* Set RS485 addr match to default state */
    dev->channel->UARTx->RS485ADRMATCH = 0;
    
    /* Clear MCR */
    if (dev->channel->UARTx == LPCOPEN_LPC_UART1) {
        /* Set Modem Control to default state */
        dev->channel->UARTx->MCR = 0;
        /*Dummy Reading to Clear Status */
        uint32_t tmp = dev->channel->UARTx->MSR;
        (void) tmp;

    }

    //SD: updated use mbed function to set baud + format
    serial_baud(dev->channel->UARTx, baud_rate);
    serial_format(dev->channel->UARTx,8, ParityNone, 1);
    
    
    /* Reset and enable FIFOs, FIFO trigger level 1 (1 chars) */
    Chip_UART_SetupFIFOS(dev->channel->UARTx, (UART_FCR_FIFO_EN | UART_FCR_RX_RS | UART_FCR_TX_RS | UART_FCR_TRG_LEV1));
    
    Chip_UART_TXEnable(dev->channel->UARTx);

    /* Enable receive data and line status interrupt */
    Chip_UART_IntEnable(dev->channel->UARTx, (UART_IER_RBRINT | UART_IER_RLSINT));
    

    NVIC_EnableIRQ(dev->irq_NUM);
}
