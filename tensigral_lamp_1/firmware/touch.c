#include "touch.h"
#include "systems.h"

uint8_t touch_calib[3];
uint8_t done_startup;
#define DO_PD 1

void init_touch()
{
#if DO_PD
	ConfigureGPIO( 0x00, INOUT_IN | PUPD_DOWN | DEFAULT_ON );	//T0
	ConfigureGPIO( 0x01, INOUT_IN | PUPD_DOWN | DEFAULT_ON );	//T1
	ConfigureGPIO( 0x03, INOUT_IN | PUPD_DOWN | DEFAULT_ON );	//T2
#else
	ConfigureGPIO( 0x00, INOUT_IN | PUPD_UP | DEFAULT_OFF );	//T0
	ConfigureGPIO( 0x01, INOUT_IN | PUPD_UP | DEFAULT_OFF );	//T1
	ConfigureGPIO( 0x03, INOUT_IN | PUPD_UP | DEFAULT_OFF );	//T2
#endif

	run_touch( touch_calib );
	run_touch( touch_calib );
	done_startup = 1;
}


void run_touch( uint8_t * counts )
{
	counts[0] = counts[1] = counts[2] = 0;

	//Do touch buttons.
	__disable_irq();
	GPIOOf(0)->MODER = (GPIOOf(0)->MODER & ~(0x000cf));
	uint32_t idrcheck[32];
	uint32_t *idrcheckmark = idrcheck;
	int i;
	*(idrcheckmark++) = GPIOOf(0)->IDR;
	*(idrcheckmark++) = GPIOOf(0)->IDR;
	*(idrcheckmark++) = GPIOOf(0)->IDR;
	*(idrcheckmark++) = GPIOOf(0)->IDR;
	*(idrcheckmark++) = GPIOOf(0)->IDR;
	*(idrcheckmark++) = GPIOOf(0)->IDR;
	*(idrcheckmark++) = GPIOOf(0)->IDR;
	*(idrcheckmark++) = GPIOOf(0)->IDR;
	*(idrcheckmark++) = GPIOOf(0)->IDR;
	*(idrcheckmark++) = GPIOOf(0)->IDR;
	*(idrcheckmark++) = GPIOOf(0)->IDR;
	*(idrcheckmark++) = GPIOOf(0)->IDR;
	*(idrcheckmark++) = GPIOOf(0)->IDR;
	*(idrcheckmark++) = GPIOOf(0)->IDR;
	*(idrcheckmark++) = GPIOOf(0)->IDR;
	*(idrcheckmark++) = GPIOOf(0)->IDR;
	*(idrcheckmark++) = GPIOOf(0)->IDR;
	*(idrcheckmark++) = GPIOOf(0)->IDR;
	*(idrcheckmark++) = GPIOOf(0)->IDR;
	*(idrcheckmark++) = GPIOOf(0)->IDR;
	*(idrcheckmark++) = GPIOOf(0)->IDR;
	*(idrcheckmark++) = GPIOOf(0)->IDR;
	*(idrcheckmark++) = GPIOOf(0)->IDR;
	*(idrcheckmark++) = GPIOOf(0)->IDR;
	*(idrcheckmark++) = GPIOOf(0)->IDR;
	*(idrcheckmark++) = GPIOOf(0)->IDR;
	*(idrcheckmark++) = GPIOOf(0)->IDR;
	*(idrcheckmark++) = GPIOOf(0)->IDR;
	*(idrcheckmark++) = GPIOOf(0)->IDR;
	*(idrcheckmark++) = GPIOOf(0)->IDR;
	*(idrcheckmark++) = GPIOOf(0)->IDR;
	*(idrcheckmark++) = GPIOOf(0)->IDR;

	GPIOOf(0)->MODER = (GPIOOf(0)->MODER | (0x00045));
	__enable_irq();
	
	counts[0] = counts[1] = counts[2] = 0;
	for( i = 0; i < 32; i++ )
	{
#if DO_PD
		if( ( idrcheck[i] & 1 ) ) counts[0]++;
		if( ( idrcheck[i] & 2 ) ) counts[1]++;
		if( ( idrcheck[i] & 8 ) ) counts[2]++;
#else
		if( !( idrcheck[i] & 1 ) ) counts[0]++;
		if( !( idrcheck[i] & 2 ) ) counts[1]++;
		if( !( idrcheck[i] & 8 ) ) counts[2]++;
#endif
	}
	for( i = 0; i < 3; i++ )
	{
		if( done_startup )
		{
			if( counts[i] < touch_calib[i] )
				touch_calib[i] = counts[i];
			counts[i] = counts[i] - touch_calib[i];
		}
		else
		{
			touch_calib[i] = counts[i];
		}
	}
}

