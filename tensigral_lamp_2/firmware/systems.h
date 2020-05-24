// Copyright 2017 <>< Charles Lohr. This file may be licensed under the MIT/x11 license or the NewBSD license.

//Partially from colorchord.

#ifndef _SYSTEMS_H
#define _SYSTEMS_H


#ifdef STM32F30X
#include <stm32f3xx_gpio.h>
#elif defined( STM32F40_41xxx )
#include <stm32f4xx_gpio.h>
#elif defined( STM32F042 )
#include <stm32f0xx_gpio.h>
#endif


void send_openocd_command(int command, void *message);

#ifdef DEBUG
	void send_text( const char * text );
	void send_text_value( const char * text, uint32_t val );
#else
	//No debug = no printfing.
	#define send_text( x )
	#define send_text_value( x, y )
#endif

void _delay_us( uint32_t us);
char hexfrom1( uint8_t val );


typedef uint8_t gpio;

gpio GetGPIOFromString( const char * str );  //i.e. "A4" or "PA5" or "B.7"
#define GPIO_FROM_NUMS( port, pin ) (((port)<<4 ) | (pin))

#define DEFAULT_VALUE_FLAG	0x00000001
#define DEFAULT_ON 			0x00000001
#define DEFAULT_OFF			0x00000000

#define INOUT_FLAG			0x00000002
#define INOUT_OUT 			0x00000002
#define INOUT_IN			0x00000000

#define PUPD_FLAG			0x0000000C
#define PUPD_NONE 			0x00000000
#define PUPD_UP 			0x00000004
#define PUPD_DOWN			0x00000008

#define GPALTFUNCTION_FLAGS		0x0000ff00

void ConfigureGPIO( gpio gpio, int parameters );



#ifdef STM32F30X
#define GPIOOf(x)  ((GPIO_TypeDef *) ((((x)>>4)<=6)?(AHB2PERIPH_BASE+0x400*((x)>>4)):0x60000000) )
#elif defined( STM32F40_41xxx )
#define GPIOOf(x)  ((GPIO_TypeDef *) ((((x)>>4)<=6)?(AHB1PERIPH_BASE+0x400*((x)>>4)):0x60000000) )
#elif defined( STM32F042 )
#define GPIOOf(x)  ((GPIO_TypeDef *) ((AHB2PERIPH_BASE + 0x00000400 * ((x)>>4) )) )
#endif

#define GPIOPin(x)   ((1<<((x)&0x0f)))
#define GPIOLatch(x) GPIOOf(x)->ODR

#ifdef STM32F30X
#define GPIOOn(x)   GPIOOf(x)->BSRR  = (1<<((x)&0x0f));
#define GPIOOff(x)  GPIOOf(x)->BRR   = (1<<((x)&0x0f));
#elif defined( STM32F40_41xxx )
#define GPIOOn(x)   GPIOOf(x)->BSRRH = (1<<((x)&0x0f));
#define GPIOOff(x)  GPIOOf(x)->BSRRL = (1<<((x)&0x0f));
#elif defined( STM32F042 )
#define GPIOOn(x)   GPIOOf(x)->BSRR  = (1<<((x)&0x0f));
#define GPIOOff(x)  GPIOOf(x)->BRR   = (1<<((x)&0x0f));
#else
#error Undefined architecture
#endif

#ifndef STM32F042

#ifdef STM32F30X
#define LEDPIN 0x18
#elif defined( STM32F40_41xxx )
#define LEDPIN 0x3f
#endif

void ConfigureLED();
#define LED_TOGGLE {GPIOOf(LEDPIN)->ODR^=(1<<((LEDPIN)&0x0f));}
#define LED_ON     GPIOOn(LEDPIN)
#define LED_OFF    GPIOOff(LEDPIN)

#endif

#ifndef _BV
#define _BV(x)  (1<<(x))
#endif

#endif

