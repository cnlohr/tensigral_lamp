// Copyright 2017 <>< Charles Lohr. See README.md for copyright details.

#include <stm32f0xx_rcc.h>
#include <systems.h>
#include <usb.h>
#include <usbconfig.h>
#include <adc.h>
#include <string.h>
#include <ws2812.h>

RCC_ClocksTypeDef RCC_Clocks;

volatile int r;

void SysTick_Handler(void)
{
	r++;
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

//	send_text_value( "L: ", length );
//	send_text_value( "C: ", code );
}

volatile int do_trigger_leds = 0;

void CBHIDData( uint8_t paklen, uint8_t * data )
{
	//Byte 1 = MSB = Launch when done. --- LSBs = length of this packet in LEDs
	//Byte 2 = Offset, LEDs

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
	RCC_GetClocksFreq( &RCC_Clocks );
	SysTick_Config( 480000 );  //100 Hz.

	send_text( "\n" );
	send_text( "Online\n" );
	setup_adcs();
	initialize_adc_start();
	InitWSDMA();

	send_text( "WS2812B\n" );

	init_usb();
	send_text( "USB\n" );

	while(1)
	{
		if( usbDataOkToSend && adc_done )
		{
			//for( i = 0; i < 15; i++ )
			//{
			//	((uint32_t*)senddata)[i+1] = ((uint32_t*)ADCs)[i];
			//}
			*(uint16_t*)(&senddata[4]) = ADCs[0];
			*(uint16_t*)(&senddata[6]) = ADCs[1];
			*(uint16_t*)(&senddata[8]) = ADCs[2];

			senddata[0] = adcreads>>8;
			senddata[1] = adcreads;
			senddata[2]++;
			usb_data( senddata, ENDPOINT1_SIZE );
			initialize_adc_start();

			//send_text_value( "   ", usbirqlast );
		}
	}
}

