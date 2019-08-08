//SoftwareSPI


#include "SoftwareSPI.h"

//#define SWSPI_DEBUG
extern "C" void debugPrintf(const char* fmt, ...) __attribute__ ((format (printf, 1, 2)));


bool SoftwareSPI::waitForTxEmpty()
{
    return false;
}


void SoftwareSPI::InitPins(Pin sck_pin, Pin miso_pin, Pin mosi_pin)
{
    sck = sck_pin;
    miso = miso_pin;
    mosi = mosi_pin;
}

//setup the master device.
void SoftwareSPI::setup_device(const struct sspi_device *device)
{
    if(needInit)
    {
        pinMode(miso, INPUT_PULLUP);
        pinMode(mosi, OUTPUT_HIGH);
        pinMode(sck, OUTPUT_LOW);

        needInit = false;
    }

    //TODO::
    //Only supports mode 0, and no frequency can be set yet
}


SoftwareSPI::SoftwareSPI():needInit(true),sck(NoPin),mosi(NoPin),miso(NoPin)
{

}


spi_status_t SoftwareSPI::sspi_transceive_packet(const uint8_t *tx_data, uint8_t *rx_data, size_t len)
{
    for (uint32_t i = 0; i < len; ++i)
    {
        uint32_t dOut = (tx_data == nullptr) ? 0x000000FF : (uint32_t)*tx_data++;
        uint8_t dIn = transfer_byte(dOut);
        if(rx_data != nullptr) *rx_data++ = dIn;
    }
	return SPI_OK;
}

/*
 * Simultaneously transmit and receive a byte on the SPI.
 *
 * Polarity and phase are assumed to be both 0, i.e.:
 *   - input data is captured on rising edge of SCLK.
 *   - output data is propagated on falling edge of SCLK.
 *
 * Returns the received byte.
 
 //WikiPedia: https://en.wikipedia.org/wiki/Serial_Peripheral_Interface#Example_of_bit-banging_the_master_protocol
 
 */
uint8_t SoftwareSPI::transfer_byte(uint8_t byte_out)
{
    uint8_t byte_in = 0;
    uint8_t bit;
    
    for (bit = 0x80; bit; bit >>= 1) {
        /* Shift-out a bit to the MOSI line */
        digitalWrite(mosi, (byte_out & bit) ? HIGH : LOW);
        
        /* Delay for at least the peer's setup time */
        //delay(SPI_SCLK_LOW_TIME);
        
        /* Pull the clock line high */
        digitalWrite(sck, HIGH);
        
        /* Shift-in a bit from the MISO line */
        if (digitalRead(miso) == HIGH)
            byte_in |= bit;
        
        /* Delay for at least the peer's hold time */
        //delay(SPI_SCLK_HIGH_TIME);
        
        /* Pull the clock line low */
        digitalWrite(sck, LOW);
    }
    
    return byte_in;
}

//unused for SoftwareSPI
spi_status_t SoftwareSPI::sspi_transceive_packet_16(const uint8_t *tx_data, uint8_t *rx_data, size_t len)
{
    return sspi_transceive_packet(tx_data,rx_data,len);
}
