#include "ws2812.h"
#include <systems.h>
#include <stm32f0xx_spi.h>

//based off of https://github.com/fduignan/NucleoF042_DMAandSPItoWS2812Bs/blob/master/spi.c

uint32_t rawwsdata[WSLEDS*4+8];

void InitWSDMA()
{
	int i;

	//PB5 = SPI1_MOSI  -> LED2
	//PB4 = SPI1_MISO
	//PB3 = SPI1_SCK

    // Turn on the clock for the SPI interface
    RCC->APB2ENR |= _BV(12);

    // Turn on PORT B
//	RCC->AHBENR |= _BV(18);

	ConfigureGPIO( GPIO_FROM_NUMS( 0, 7 ), 0b11 );  //Just for the sake of testing/setting the port up.

	for( i = 0; i < WSLEDS*4+8; i++ )
	{
		rawwsdata[i] = 0;
	}

    // Configure the pins
//    GPIOB->MODER |= _BV(11);  //GPIO B5 AF
//    GPIOB->MODER &= ~_BV(10);
//	GPIOB->AFR[0] &= ~(_BV(20) | _BV(21) | _BV(22) | _BV(23)); //Set AFMode 0.

    GPIOA->MODER |= _BV(15);  //GPIO A7 AF
    GPIOA->MODER &= ~_BV(14);
	GPIOA->AFR[0] &= ~(_BV(28) | _BV(29) | _BV(30) | _BV(31)); //Set AFMode 0.

	SPI1->CR1 = SPI1->CR2 = 0;

	SPI1->CR1 |= SPI_CR1_CPHA; // set CPHA to ensure MOSI is low when idle  
	SPI1->CR1 |= SPI_CR1_MSTR; // select master mode
	SPI1->CR1 |= (3 << 3); // select divider of 16.  48MHz/16 = 3MHz.  3 bits per WS2812 bit so: 1 Million WSbits/second : Within range of  1.53MHz to 540kHz    
	SPI1->CR1 |= SPI_CR1_SSI | SPI_CR1_SSM; // select software slave management and drive SSI high (not relevant in TI mode and not output to pins)

	SPI1->CR2 |= (7 << 8); // select 8 bit data transfer size  
	SPI1->CR2 |= SPI_CR2_TXDMAEN; // enable transmit DMA

	SPI1->CR1 |= SPI_CR1_SPE; // enable SPI1


	RCC->AHBENR |= RCC_AHBENR_DMAEN; // enable clocks for DMA controller

	DMA1_Channel3->CPAR = (uint32_t)(&(SPI1->DR));
	// Select high priority, 8 bits, Memory increment mode (only), 	Read from memory, enable
	DMA1_Channel3->CCR = 
		_BV(13) | _BV(12) |  //Very high priority
		 //MSIZE = PSIZE = 0 = 8 bit operations.  TODO Consider trying to do it with 16-bit-wide registers.  WARNING: This did not work.  Don't know why.
		_BV(7) | //Memory increment mode
		//No circular mode.
		_BV(4) |
		0; 
}


void TriggerDMA()
{
//	if( DMA1_Channel3->CCR != 12433 )
//		send_text_value( "CDMA", DMA1_Channel3->CCR );
	//send_text_value( "DMA: ", DMA1_Channel3->CCR );
	DMA1_Channel3->CCR &= ~_BV(0); // disable DMA
	DMA1->IFCR = 0x0f00; // clear any pending interrupts  XXX Fix this later.
	DMA1_Channel3->CPAR = (uint32_t)(&(SPI1->DR));
	DMA1_Channel3->CMAR = (uint32_t)&rawwsdata;  // Don't know what to send yet
	DMA1_Channel3->CNDTR = sizeof(rawwsdata)+1; // No bytes yet
	DMA1_Channel3->CCR |= _BV(0); // re-enable DMA
}

void UpdateLEDs( int ledstart, uint8_t * leddata, int lengthleds )
{

	static const uint16_t CodeData[4*4] = { 
		0x8888, 0x8e88, 0xe888, 0xee88,
		0x888e, 0x8e8e, 0xe88e, 0xee8e,
		0x88e8, 0x8ee8, 0xe8e8, 0xeee8,
		0x88ee, 0x8eee, 0xe8ee, 0xeeee
	};

	if( ledstart + lengthleds > WSLEDS )
	{
		lengthleds = WSLEDS - ledstart;
	}

	int inmark = ledstart * 4;
	int bytes = lengthleds * 4;
	int i;
	for( i = 0; i < bytes; i++ )
	{
		int8_t color = leddata[i];
		//Word-on-wire order:  (Determined experiomentally)
		//LSByte first. MSBit first.
		if( i + inmark >= WSLEDS*4 ) break;
		rawwsdata[i+inmark] = CodeData[(color>>4)&0xf] | (CodeData[(color)&0xf]<<16);
	}
}



