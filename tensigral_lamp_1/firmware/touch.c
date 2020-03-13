#include "touch.h"
#include "systems.h"

uint8_t touch_calib[3];
uint8_t done_startup;

//DO_PD means we use the internal pull-down resistors to measure the capacitance
//if we define this to be 0, then it will use the internal pull-up resistors. 
//I am still experimenting with both options to see which is better overall and
//in a lot of situations.
#define DO_PD 1


//The methodology is we are trying to measure the capacitance of a pad.  This is
//done by driving the pad high, then, releasing the pad.  If the capacitance is
//low, then, the pad's voltage will very quickly drop.  If the capacitance is high
//then the pad's voltage will drop much more slowly.  If your finger is not present
//then the voltage will cross the 0 threshhold very quicky.  If your finger is present
//it will take much longer, sometimes as long as a microsecond!
//
//          v Stop driving pin high
// V  ------
// O        |
// L         \
// T          \
// A            \
// G               \
// E  -------------------------------  Threshhold for measuring zero
//                         \
//                                  \
//           ^ ------- ^ The time we're measuring

void init_touch()
{
  //This demo shows the use of 3 touch buttons, but really, an unlimited number
  //of touch buttons can be used using this method as long as they're all on the
  //same IO section.  I.e. This demo uses Port A.
  
  //I use "systems.c" of which an example can be found in the ColorChord Embedded
  //codebase.  The rest of this specific project will be made public soon.  But
  //you can use the HAL if you absolutely need to.  This is just to turn on the 
  //port's AHB, and enable the IO as an IO push-pull port will pull-down or pull
  //up resistors.
#if DO_PD
	ConfigureGPIO( 0x00, INOUT_IN | PUPD_DOWN | DEFAULT_ON );	//T0
	ConfigureGPIO( 0x01, INOUT_IN | PUPD_DOWN | DEFAULT_ON );	//T1
	ConfigureGPIO( 0x03, INOUT_IN | PUPD_DOWN | DEFAULT_ON );	//T2
#else
	ConfigureGPIO( 0x00, INOUT_IN | PUPD_UP | DEFAULT_OFF );	//T0
	ConfigureGPIO( 0x01, INOUT_IN | PUPD_UP | DEFAULT_OFF );	//T1
	ConfigureGPIO( 0x03, INOUT_IN | PUPD_UP | DEFAULT_OFF );	//T2
#endif

  //We run the algorithm twice.  The very first time, something is always
  //askew with the pads, not sure why, but the second run is 100%
	run_touch( touch_calib );
	run_touch( touch_calib );
	done_startup = 1;
}


void run_touch( uint8_t * counts )
{
	counts[0] = counts[1] = counts[2] = 0;

	//For this tiny period of time, we disable interrupts because timing matters.
	__disable_irq();
  
  //We stop driving the pin here, and let it float down.
	GPIOOf(0)->MODER = (GPIOOf(0)->MODER & ~(0x000cf));
	uint32_t idrcheck[32];
	uint32_t *idrcheckmark = idrcheck;
	int i;
  
  //We read the value of IDR for the port.  This will make it so
  //we can read every IO on the port at exactly the same time. This
  //makes it very convenient to synchronously read many touch sensors.
  
  //You can think of this a little bit like running a logic analyzer
  //for a very short period of time over every pin on a given port.
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

  //Once we've spent about 2 microseconds or so reading the ports, we
  //then go back to driving the ports high.
	GPIOOf(0)->MODER = (GPIOOf(0)->MODER | (0x00045));
  
  //Because we're messing with MODER here, we probably don't want to let
  //an interrupt fire before we set it back, otherwise we might corrupt
  //it's value.
	__enable_irq();
	
  
  //Now, that we have our data of what the 
	counts[0] = counts[1] = counts[2] = 0;
	for( i = 0; i < 32; i++ )
	{
#if DO_PD
    //The &1 selects PinA.0, &2 selects PinA.1 and &8 selects PinA.3
		if( ( idrcheck[i] & 1 ) ) counts[0]++;
		if( ( idrcheck[i] & 2 ) ) counts[1]++;
		if( ( idrcheck[i] & 8 ) ) counts[2]++;
#else
		if( !( idrcheck[i] & 1 ) ) counts[0]++;
		if( !( idrcheck[i] & 2 ) ) counts[1]++;
		if( !( idrcheck[i] & 8 ) ) counts[2]++;
#endif
	}
  
  //Potentially write calibration data, and/or subtract out
  //our calibration data as being our reference "zero" pressure.
  
  //If the read value every drops very percipitiously, then, it
  //may be because the user had pressed the button at boot.  This
  //will allow it to reset back down.
  
  //XXX TODO: Don't allow the calibration to reset down super fast
  //because there is theoretically (thouh I could not make it happen)
  //a possibility that ESD could make there be some sort of transient.
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

