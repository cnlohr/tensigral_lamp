// Copyright 2017, 2020 <>< Charles Lohr. See README.md for copyright details.

#include <stm32f0xx_rcc.h>
#include <systems.h>
#include <usb.h>
#include <usbconfig.h>
#include <adc.h>
#include "touch.h"
#include <string.h>
#include <ws2812.h>

RCC_ClocksTypeDef RCC_Clocks;

volatile int r;

void SysTick_Handler(void)
{
	r++;
}

void SystemBootFault( uint32_t HSEStatus, uint32_t Timeout )
{
	send_text( "No external oscillator.\n" );
//	send_text_value( "HSEStatus:", HSEStatus );
//	send_text_value( "Timeout:", Timeout );
//	send_text_value( "RCC->CFGR:", RCC->CFGR );
//	send_text_value( "RCC->CR:", RCC->CR );
 
//	while(1);
}

void assert_param( int x )
{
	if( !x )
	{
		send_text( "ASSERT\n" );	
		while(1);
	}
}

//All data is assumed 64-byte chunks.

void CBHIDSetup( uint16_t length, uint8_t code )
{
	//Don't care about initial packet showing start of control transfer.

	//send_text_value( "L: ", length );
	//send_text_value( "C: ", code );
}

volatile int do_trigger_leds = 0;

void CBHIDData( uint8_t paklen, uint8_t * data )
{
#if 0
	//Byte 1 = MSB = Launch when done. --- LSBs = length of this packet in LEDs
	//Byte 2 = Offset, LEDs

	UpdateLEDs( data[1], &data[2], data[0] & 0x7f );
//	send_text_value( "TV:", data[1] );
	if( data[0] & 0x80 )
	{
		TriggerDMA();
	}
#endif
//Seems unreliable.
}

void CBHIDInterruptIn( uint8_t paklen, uint8_t * data )
{
	//Byte 1 = MSB = Launch when done. --- LSBs = length of this packet in LEDs
	//Byte 2 = Offset, LEDs

//	send_text_value( "SVD: ", data[0] );

	UpdateLEDs( data[1], &data[2], data[0] & 0x7f );
//	send_text_value( "TV:", data[1] );
	if( data[0] & 0x80 )
	{
		TriggerDMA();
	}
}


uint8_t senddata[64];

int main()
{
	int i;
	SystemCoreClockUpdate();
	RCC_GetClocksFreq( &RCC_Clocks );
	SysTick_Config( RCC_Clocks.SYSCLK_Frequency/100 );  //100 Hz.
#if 0
	send_text_value( "SYSCLK_Frequency: ", RCC_Clocks.SYSCLK_Frequency );
	send_text_value( "HCLK_Frequency: ", RCC_Clocks.HCLK_Frequency );
	send_text_value( "PCLK_Frequency: ", RCC_Clocks.PCLK_Frequency );
	send_text_value( "ADCCLK_Frequency: ", RCC_Clocks.ADCCLK_Frequency );
	send_text_value( "CECCLK_Frequency: ", RCC_Clocks.CECCLK_Frequency );
	send_text_value( "I2C1CLK_Frequency: ", RCC_Clocks.I2C1CLK_Frequency );
	send_text_value( "USART1CLK_Frequency: ", RCC_Clocks.USART1CLK_Frequency );
	send_text_value( "USART2CLK_Frequency: ", RCC_Clocks.USART2CLK_Frequency );
	send_text_value( "USBCLK_Frequency: ", RCC_Clocks.USBCLK_Frequency );
	send_text_value( "SYSCLK_Frequency: ", RCC_Clocks.SYSCLK_Frequency );
	send_text_value( "RCC->CFGR:", RCC->CFGR );
	send_text_value( "RCC->CR:", RCC->CR );
	send_text_value( "SWS:",RCC->CFGR & RCC_CFGR_SWS);
#endif

	ConfigureGPIO( 0x05, INOUT_OUT );	//Also do this first to enable port A.
	init_touch();

	setup_adcs();

	initialize_adc_start();
	InitWSDMA();

	init_usb();

	for( i = 0; i < 20; i++ )
	{
		UpdateLEDs( i, (uint8_t*)"\x60\x00\x80\xff", 1 );
	}

	TriggerDMA();

	uint8_t counts[3];

	while(1)
	{
	//	GPIOLatch( 0x00 ) &= ~(1<<6);
		//GPIOOn( 0x06 );

		//GPIOOff( 0x06 );

		if( usbDataOkToRead )
		{
			CBHIDInterruptIn( 64, usb_get_out_ep_buffer() );
			usb_release_out_ep_buffer();
			usbDataOkToRead = 0;
		}
		if( usbDataOkToSend && adc_done )
		{
			for( i = 0; i < 3; i++ )
			{
				senddata[4+i*2+0] = ADCs[i] & 0xff;
				senddata[4+i*2+1] = (ADCs[i] >> 8) & 0xff;
			}

			senddata[0] = adcreads>>8;
			senddata[1] = adcreads;
			senddata[2]++;
			senddata[20] = counts[0];
			senddata[21] = counts[1];
			senddata[22] = counts[2];
			usb_data( senddata, ENDPOINT1_SIZE );
			initialize_adc_start();

			run_touch( counts );
		}
		//GPIOOn( 0x05 );
		//GPIOOff( 0x05 );
		//GPIOOn( 0x05 );
		//GPIOOff( 0x05 );
	}
}

