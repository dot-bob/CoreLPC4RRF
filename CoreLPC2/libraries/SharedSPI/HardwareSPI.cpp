//Hardware SPI

#include "HardwareSPI.h"


#define SPI0_FUNCTION  PINSEL_FUNC_2 //SSP
#define SPI1_FUNCTION  PINSEL_FUNC_2


//SSP Status Register Bits
constexpr uint8_t SR_TFE = (1<<0); //Transmit FIFO Empty. This bit is 1 is the Transmit FIFO is empty, 0 if not.
constexpr uint8_t SR_TNF = (1<<1); //Transmit FIFO Not Full. This bit is 0 if the Tx FIFO is full, 1 if not.
constexpr uint8_t SR_RNE = (1<<2); //Receive FIFO Not Empty. This bit is 0 if the Receive FIFO is empty, 1 if not
constexpr uint8_t SR_RFF = (1<<3); //Receive FIFO Full. This bit is 1 if the Receive FIFO is full, 0 if not.
constexpr uint8_t SR_BSY = (1<<4); //Busy. This bit is 0 if the SSPn controller is idle, or 1 if it is currently sending/receiving a frame and/or the Tx FIFO is not empty.


//#define SSPI_DEBUG
extern "C" void debugPrintf(const char* fmt, ...) __attribute__ ((format (printf, 1, 2)));

// Wait for transmitter ready returning true if timed out
static inline bool waitForTxReady(LPC_SSP_TypeDef* sspDevice)
{
    uint32_t timeout = SPI_TIMEOUT;
    while (sspDevice->SR & SR_BSY) //check if SSP BSY flag (1=busy)
    {
        if (--timeout == 0)
        {
            return true;
        }
    }
    return false;
}



//Make sure all the SSP FIFOs are empty, if they aren't clear them
//returns true if ready, false if timeout
static inline bool clearSSP(LPC_SSP_TypeDef* sspDevice)
{
#ifdef SSPI_DEBUG
    if( (sspDevice->SR & SR_BSY) ) debugPrintf("SPI Busy\n");
    if( !(sspDevice->SR & SR_TFE) ) debugPrintf("SPI Tx Not Empty\n");
    if( (sspDevice->SR & SR_RNE) ) debugPrintf("SPI Rx Not Empty\n");
#endif

    //wait for SSP to be idle
    if (waitForTxReady(sspDevice)) return true; //timed out

    //check the Receive FIFO
    if( (sspDevice->SR & SR_RNE) ){
        //clear out the FIFO
        while( (sspDevice->SR & SR_RNE) ){
            sspDevice->DR; // read
        }
    }

    return false; //did not timeout (success)

}

//wait for SSP receive FIFO to be not empty
static inline bool waitForReceiveNotEmpty(LPC_SSP_TypeDef* sspDevice)
{
    uint32_t timeout = SPI_TIMEOUT;
    
    //Bit 2 - RNE (Receive Not Empty) - This bit is 0 if the Receive FIFO is empty
    while( !(sspDevice->SR & SR_RNE) )
    {
        if (--timeout == 0)
        {
            return true;
        }
    }
    return false;
}



// Wait for transmitter empty returning true if timed out
//static inline bool waitForTxEmpty(LPC_SSP_TypeDef* sspDevice)
bool HardwareSPI::waitForTxEmpty()
{
    return waitForTxEmpty_timeout(selectedSSPDevice, SPI_TIMEOUT);
}

// Wait for receive data available returning true if timed out
static inline bool waitForRxReady(LPC_SSP_TypeDef* sspDevice)
{
    return ssp_readable_timeout(sspDevice, SPI_TIMEOUT);
}



//setup the master device.
void HardwareSPI::setup_device(const struct sspi_device *device)
{
    if(needInit)
    {
        if(device->sspChannel == SSP0)
        {
            LPC_SC->PCONP |= 1 << 21;//enable Power and clocking
            
            GPIO_PinFunction(SPI0_SCK,  SPI0_FUNCTION);   /* Configure the Pinfunctions for SPI */
            GPIO_PinFunction(SPI0_MOSI, SPI0_FUNCTION);
            GPIO_PinFunction(SPI0_MISO, SPI0_FUNCTION);
            
            GPIO_PinDirection(SPI0_SCK,OUTPUT);        /* Configure SCK,MOSI,SSEl as Output and MISO as Input */
            GPIO_PinDirection(SPI0_MOSI,OUTPUT);
            GPIO_PinDirection(SPI0_MISO,INPUT);
            
        }
        else if (device->sspChannel == SSP1)
        {
            LPC_SC->PCONP |= 1 << 10;//enable Power and clocking
            
            GPIO_PinFunction(SPI1_SCK,SPI1_FUNCTION);   /* Configure the Pinfunctions for SPI */
            GPIO_PinFunction(SPI1_MOSI,SPI1_FUNCTION);
            GPIO_PinFunction(SPI1_MISO,SPI1_FUNCTION);
            
            GPIO_PinDirection(SPI1_SCK,OUTPUT);        /* Configure SCK,MOSI,SSEl as Output and MISO as Input */
            GPIO_PinDirection(SPI1_MOSI,OUTPUT);
            GPIO_PinDirection(SPI1_MISO,INPUT);
        }
        
        needInit = false;
    }

    spi_format(selectedSSPDevice, device->bitsPerTransferControl, device->spiMode, 0); //Set the bits, set Mode 0, and set as Master
    spi_frequency(selectedSSPDevice, device->clockFrequency);
    
    clearSSP(selectedSSPDevice);
}


HardwareSPI::HardwareSPI(LPC_SSP_TypeDef *sspDevice):needInit(true)
{
    selectedSSPDevice = sspDevice;
}


spi_status_t HardwareSPI::sspi_transceive_packet(const uint8_t *tx_data, uint8_t *rx_data, size_t len)
{
    if(selectedSSPDevice == nullptr) return SPI_ERROR_TIMEOUT;//todo: just return timeout error if null
    if(clearSSP(selectedSSPDevice)) return SPI_ERROR_TIMEOUT;

    for (uint32_t i = 0; i < len; ++i)
	{
		uint32_t dOut = (tx_data == nullptr) ? 0x000000FF : (uint32_t)*tx_data++;

        //Wait for Transmit to be ready
        if (waitForTxReady(selectedSSPDevice)) return SPI_ERROR_TIMEOUT;
        
        selectedSSPDevice->DR = dOut; //put data into the Data Registor to transmit
        
        // Wait for receive to be ready
        if (waitForRxReady(selectedSSPDevice)) return SPI_ERROR_TIMEOUT;

        uint8_t dIn = selectedSSPDevice->DR; //get received data from Data Registor
        if(rx_data != nullptr) *rx_data++ = dIn;
    }
    
	return SPI_OK;
}


//16bit version of transceive packet for SDCard - Sends 2x8bits in 16bit mode
// len - Number of bytes to transceive
// (TODO: must be at least 16 bytes to use this method)
spi_status_t HardwareSPI::sspi_transceive_packet_16(const uint8_t *tx_data, uint8_t *rx_data, size_t len)
{
    if(selectedSSPDevice == nullptr) return SPI_ERROR_TIMEOUT;//todo: just return timeout error if null
    if(clearSSP(selectedSSPDevice)) return SPI_ERROR_TIMEOUT;
    
    const uint32_t cr0 =  selectedSSPDevice->CR0 & 0xFFFF; // save old value
    
    //change to 16 bit mode (DSS = (bits-1) dss is bits 0:3 of CR)
    selectedSSPDevice->CR0 |= 0x000F;
    
    //Fill the FIFO by loading up the first 8 frames (each 16bit frame holds 2 bytes)
    for (uint8_t i = 0; i < 8; i++)
    {
        uint32_t dOut = 0x0000FFFF;
        if(tx_data != nullptr)
        {
            dOut = *tx_data++;
            dOut = (dOut << 8) | *tx_data++;
        }
        selectedSSPDevice->DR = dOut; //put data into the Data Register to transmit

    }
    len -= 16;
    
    while (len >= 2)
    {
        //wait until there is data in the FIFO
        if (waitForReceiveNotEmpty(selectedSSPDevice))
        {
            selectedSSPDevice->CR0 = cr0; //set mode back to previous value
            //debugPrintf("SharedSPI: Timeout Error\n");
            return SPI_ERROR_TIMEOUT;
        }

        //Read in a Frame
        uint32_t dIn = selectedSSPDevice->DR; //get received data from Data Register
        if(rx_data != nullptr)
        {
            *rx_data++ = dIn >> 8;
            *rx_data++ = dIn;
        }
            
        //Send the next Frame
        uint32_t dOut = 0x0000FFFF;
        if(tx_data != nullptr)
        {
            dOut = *tx_data++;
            dOut = (dOut << 8) | *tx_data++;
        }

        selectedSSPDevice->DR = dOut; //put data into the Data Register to transmit
        
        len -= 2;
    }
    
    //get the last 8 frames from Receive FIFO
    for (uint8_t i = 0; i < 8; i++)
    {
        //wait until there is data in the FIFO
        if (waitForReceiveNotEmpty(selectedSSPDevice))
        {
            selectedSSPDevice->CR0 = cr0; //set mode back to previous value
//            debugPrintf("SharedSPI: Timeout Error\n");
            return SPI_ERROR_TIMEOUT;
        }
        
        
        //Read Frame
        uint32_t dIn = selectedSSPDevice->DR; //get received data from Data Register
        if(rx_data != nullptr)
        {
            *rx_data++ = dIn >> 8;
            *rx_data++ = dIn;
        }
    }
    
    selectedSSPDevice->CR0 = cr0; //set mode back to previous value

    return SPI_OK;
}