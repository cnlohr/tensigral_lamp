#include "adc.h"
#include <stm32f0xx_adc.h>
#include <stm32f0xx_gpio.h>
#include <stm32f0xx_rcc.h>
#include <stm32f0xx.h>
#include <systems.h>
#include <stm32f0xx_misc.h>


/*
#define ADC_IT_ADRDY                               ADC_IER_ADRDYIE  // Doesn't trigger?
#define ADC_IT_EOSMP                               ADC_IER_EOSMPIE  //Triggers/
#define ADC_IT_EOC                                 ADC_IER_EOCIE //Triggers.
#define ADC_IT_EOSEQ                               ADC_IER_EOSEQIE //BINGO
#define ADC_IT_OVR                                 ADC_IER_OVRIE
#define ADC_IT_AWD                                 ADC_IER_AWDIE
*/ 
//#define ADCINT ADC_IT_EOSEQ || ADC_IT_EOSMP

volatile uint16_t adcreads = 0;
volatile static int adcstate = 0;
volatile uint8_t adc_done = 0;
volatile uint8_t ADCs[4];


#define ADCCHANS 3 //Base, 3.6v and Temp.

void setup_adcs()
{
	int i;

	//First set up ADMUXing for the external muxers.
	ConfigureGPIO( GPIO_FROM_NUMS( 0, 6 ), INOUT_OUT );
	GPIOOf(0)->BRR = 0x3c0; //Reset all mux's..


	//From http://hung2492.blogspot.com/2015/02/lesson-5-adcdac-stm32f0.html

	GPIO_InitTypeDef GPIO_InitStructure;
	ADC_InitTypeDef  ADC_InitStructure;

	//(#) Enable the ADC interface clock using
	//    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

	//(#) ADC pins configuration
	//   (++) Enable the clock for the ADC GPIOs using the following function:
	//        RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOx, ENABLE);  
	//   (++) Configure these ADC pins in analog mode using GPIO_Init();  
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);

	GPIO_InitStructure.GPIO_Pin = 0x3f; //PA0..5
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = 1; //PB0 (5V rail)
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	//(#) Configure the ADC conversion resolution, data alignment, external
	//    trigger and edge, scan direction and Enable/Disable the continuous mode
	//    using the ADC_Init() function.
	ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;   
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_TRGO;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_ScanDirection = ADC_ScanDirection_Upward;
	ADC_Init(ADC1, &ADC_InitStructure);

	ADC_TempSensorCmd( ENABLE );
	ADC_ChannelConfig(ADC1, 5, ADC_SampleTime_13_5Cycles);

	// Calibrate ADC before enabling
	ADC_GetCalibrationFactor(ADC1);
	//(#) Activate the ADC peripheral using ADC_Cmd() function.
	ADC_Cmd(ADC1, ENABLE);

	// Wait until ADC enabled
	while(ADC_GetFlagStatus(ADC1, ADC_FLAG_ADEN) == RESET);  

	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = ADC1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
 
	// Enable Watchdog interrupt
	ADC_ITConfig(ADC1, ADC_IT_EOSEQ | ADC_IT_EOSMP, ENABLE); //Enable for end-of-conversion.

	adc_done = 1;

	send_text( "ADC Setup\n" );
}

int initialize_adc_start()
{
	if( !adc_done )
	{
		return -1;
	}

	adc_done = 0;

	adcstate = 0;
	ADC1->CHSELR = 5;
	ADC_StartOfConversion(ADC1);

	adcstate = 1;
	return 0;
}

void __attribute__ ((interrupt("IRQ"))) ADC1_IRQHandler(void)
{
	if(ADC1->ISR & ADC_IT_EOSMP)
	{
		//End of sample?  Could switch to another mux.
		ADC_ClearFlag( ADC1, ADC_IT_EOSMP );
	}
	if( ADC1->ISR & ADC_IT_EOSEQ )
	{

		//Careful: Take as much time as possible between changing the mux values and the below part.
		//This gives time for the mux to switch over.

		ADC_ClearFlag( ADC1, ADC_IT_EOSEQ );
		adcreads++;

		uint8_t gotval = adcstate-1;
		uint16_t val = ADC1->DR; //ADC_GetConversionValue(ADC1);
		uint8_t triple = (gotval>>1) * 3;

		//XXX Tricky: Get temperature state
		if( adcstate == ADCCHANS )
		{
			#define TEMP110_CAL_ADDR ((uint16_t*) ((uint32_t) 0x1FFFF7C2))
			#define TEMP30_CAL_ADDR ((uint16_t*) ((uint32_t) 0x1FFFF7B8))
			#define VDD_CALIB ((uint16_t) (330))
			#define VDD_APPLI ((uint16_t) (360))
			int32_t temperature; /* will contain the temperature in degrees Celsius */
			temperature = (((int32_t) val * VDD_APPLI / VDD_CALIB) - (int32_t) *TEMP30_CAL_ADDR );
			temperature = temperature * (int32_t)(110 - 30) * 10;
			temperature = temperature / (int32_t)(*TEMP110_CAL_ADDR - *TEMP30_CAL_ADDR);
			val = temperature + 300;
		}

		//XXX Your value is in "val"
		ADCs[adcstate] = val;

		if( adcstate >= ADCCHANS )
		{
			adc_done = 1;
			return;		
		}

		adc_done = 1;
		ADC1->CHSELR = 5;
		ADC_StartOfConversion(ADC1);

		adcstate++;
	}
}



