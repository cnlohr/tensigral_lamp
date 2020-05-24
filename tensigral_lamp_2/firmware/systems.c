// Copyright 2017 <>< Charles Lohr. This file may be licensed under the MIT/x11 license or the NewBSD license.
//Partially from colorchord.

#include <stdint.h>
#include "systems.h"
#include <string.h>
#ifdef STM32F042
#include <stm32f0xx.h>
#include <stm32f0xx_rcc.h>
#include <stm32f0xx_gpio.h>
#elif defined( STM32F30X )
#include <stm32f30x.h>
#include <stm32f30x_rcc.h>
#include <stm32f30x_gpio.h>
#elif defined( STM32F40_41xxx )
#include <stm32f4xx.h>
#include <stm32f4xx_rcc.h>
#include <stm32f4xx_gpio.h>
#endif

#ifdef STM32F042
#include <core_cm0.h>
#else
#include <core_cm4.h>
#include <core_cmFunc.h>
#endif

extern RCC_ClocksTypeDef RCC_Clocks;


char hexfrom1( uint8_t val )
{
	if( val < 10 ) return '0' + val;
	if( val < 16 ) return 'a' - 10 + val;
	return '-';
}


void send_openocd_command(int command, void *message)
{
#ifdef DEBUG
   asm("mov r0, %[cmd];"
       "mov r1, %[msg];"
       "bkpt #0xAB"
         :
         : [cmd] "r" (command), [msg] "r" (message)
         : "r0", "r1", "memory");
#endif
}


#ifdef DEBUG
void send_text( const char * text )
{
	uint32_t m[] = { 2, (uint32_t)text, strlen(text) };
	send_openocd_command(0x05, m);
}

void send_text_value( const char * text, uint32_t val )
{
	int len = strlen( text );
	uint8_t markdata[len+14];
	memcpy( markdata, text, len );
	uint32_t place = 1000000000;
	int hit = 0;
	int mdp = len;
	while( place )
	{
		int r = val / place;
		if( r || hit )
		{
			hit = 1;
			markdata[mdp++] = '0' + r;
			val -= r * place;
		}
		place /= 10;
	}
	if( !hit )
		markdata[mdp++] = '0';
	markdata[mdp++] = '\n';
	markdata[mdp] = 0;
	uint32_t m[] = { 2, (uint32_t)markdata, mdp };
	send_openocd_command(0x05, m);
}
#endif

int __attribute__((used)) _write (int fd, const void *buf, size_t count)
{
	uint32_t m[] = { 2, (uint32_t)buf, count };
	send_openocd_command(0x05, m);
	return count;
}

void __attribute__((used)) * _sbrk(int incr) {
    extern char _ebss; // Defined by the linker
    static char *heap_end;
    char *prev_heap_end;


    if (heap_end == 0) {
        heap_end = &_ebss;
    }
    prev_heap_end = heap_end;

	char * stack = (char*) __get_MSP();
     if (heap_end + incr >  stack)
     {
		return  (void*)(-1);
     }

    heap_end += incr;

    return (void*) prev_heap_end;
}

#ifdef STM32F042
void _delay_us(uint32_t us) {
	asm(	"	mov r0, %[num];"
			"1: sub r0, r0, #1;"
			"	bne   1b"
         :
         : [num] "r" (us*(RCC_Clocks.HCLK_Frequency/4000000)) );
}

#else 
//Not available on Cortex-M0
//For precise timing.
volatile unsigned int *DWT_CYCCNT   = (volatile unsigned int *)0xE0001004; //address of the register
volatile unsigned int *SCB_DEMCR        = (volatile unsigned int *)0xE000EDFC; //address of the register
volatile unsigned int *DWT_CONTROL  = (volatile unsigned int *)0xE0001000; //address of the register

void _delay_us(uint32_t us) {
	if( us ) us--; //Approximate extra overhead time.
	us *= RCC_Clocks.HCLK_Frequency/1000000;
    *SCB_DEMCR = *SCB_DEMCR | 0x01000000;
    *DWT_CYCCNT = 0; // reset the counter
    *DWT_CONTROL = *DWT_CONTROL | 1 ; // enable the counter
	while( *DWT_CYCCNT < us );
}
#endif

#ifndef STM32F042
void ConfigureLED()
{
	ConfigureGPIO( LEDPIN, INOUT_OUT );
}
#endif

uint8_t GetGPIOFromString( const char * str )
{
	int mode = 0;
	int port = -1;
	int pin = -1;
	const char * st = str;
	for( ; *st; st++ )
	{
		char c = *st;
		if( mode == 0 )
		{
			if( c >= 'A' && c <= 'F' )
			{
				port = c - 'A';
				mode = 2;
			}
			else if( c >= 'a' && c <= 'f' )
			{
				port = c - 'a';
				mode = 2;
			}
		}
		else if( mode == 2 )
		{
			if( c >= '0' && c <= '9' )
			{
				pin = 0;
				mode = 3;
			}
		}

		if( mode == 3 )
		{
			if( c >= '0' && c <= '9' )
			{
				pin = pin * 10;
				pin+= c - '0';
			}
			else
			{
				break;
			}
		}
	}

	if( port > 0 && pin > 0 && port <= 6 && pin <= 15)
	{
		return (port<<4)|pin;
	}
	else
	{
		return 0xff;
	}
}


void ConfigureGPIO( uint8_t gpio, int parameters )
{
	GPIO_InitTypeDef  GPIO_InitStructure;

#ifdef STM32F30X

	/* Enable the GPIO_LED Clock */
	RCC_AHBPeriphClockCmd( 1<<(17+(gpio>>4)), ENABLE);

	if( parameters & DEFAULT_VALUE_FLAG )
	{
		GPIOOn( gpio );
	}
	else
	{
		GPIOOff( gpio );
	}

	/* Configure the GPIO_LED pin */
	GPIO_InitStructure.GPIO_Pin = 1<<(gpio&0xf);
	GPIO_InitStructure.GPIO_Mode = (parameters&INOUT_FLAG)?GPIO_Mode_OUT:GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = (parameters&PUPD_FLAG)?( (parameters&PUPD_UP)?GPIO_PuPd_UP:GPIO_PuPd_DOWN ):GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOOf(gpio), &GPIO_InitStructure);

#elif defined( STM32F40_41xxx )


	/* Enable the GPIO_LED Clock */
	RCC_AHB1PeriphClockCmd( 1<<((gpio>>4)), ENABLE);

	if( parameters & DEFAULT_VALUE_FLAG )
	{
		GPIOOn( gpio );
	}
	else
	{
		GPIOOff( gpio );
	}

	/* Configure the GPIO_LED pin */
	GPIO_InitStructure.GPIO_Pin = 1<<(gpio&0xf);
	GPIO_InitStructure.GPIO_Mode = (parameters&INOUT_FLAG)?GPIO_Mode_OUT:GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = (parameters&PUPD_FLAG)?( (parameters&PUPD_UP)?GPIO_PuPd_UP:GPIO_PuPd_DOWN ):GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOOf(gpio), &GPIO_InitStructure);

#elif defined( STM32F042 )

	RCC_AHBPeriphClockCmd( (gpio&0x10)?RCC_AHBPeriph_GPIOB:RCC_AHBPeriph_GPIOA, ENABLE );
	if( parameters & DEFAULT_VALUE_FLAG )
	{
		GPIOOn( gpio );
	}
	else
	{
		GPIOOff( gpio );
	}

	/* Configure the GPIO_LED pin */
	GPIO_InitStructure.GPIO_Pin = 1<<(gpio&0xf);
	GPIO_InitStructure.GPIO_Mode = (parameters&INOUT_FLAG)?GPIO_Mode_OUT:GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = (parameters&PUPD_FLAG)?( (parameters&PUPD_UP)?GPIO_PuPd_UP:GPIO_PuPd_DOWN ):GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOOf(gpio), &GPIO_InitStructure);

#else
#error Undefined processor.
#endif

}

