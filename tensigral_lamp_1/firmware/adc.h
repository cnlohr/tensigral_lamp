#ifndef _ADC_H
#define _ADC_H

#include <stdint.h>

void setup_adcs();

int initialize_adc_start();

extern volatile uint16_t adcreads;
extern volatile uint8_t adc_done;
extern volatile uint16_t ADCs[4];

#endif


