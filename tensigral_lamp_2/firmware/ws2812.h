#ifndef _WS2812_H
#define _WS2812_H

#define WSLEDS 20

#include <stdint.h>

//Initialize DMA, etc.
void InitWSDMA( );
void TriggerDMA( );
void UpdateLEDs( int ledstart, uint8_t * leddata, int lengthleds );

#endif

