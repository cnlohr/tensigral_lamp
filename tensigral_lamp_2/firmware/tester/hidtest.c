#include <stdio.h>
#include "hidapi.h"
#include <math.h>
#include <stdint.h>
#include "os_generic.h"
#include <stdlib.h>
#include "CNFGFunctions.h"
#include <string.h>

#ifdef WINDOWS
#include <windows.h>
#define usleep(x) Sleep(x/1000)
#else
#include <unistd.h>
#endif

hid_device *handle = NULL;
short screenx, screeny;
int RXHz;
int TXHz;
short voltage;
short temperature;
short adc;
//short ADCs[38];
uint8_t Colorbuf[2048];
int colormode;
double colormodechangetime;

unsigned char RXbuf[65];
int RXdeltaReads;
int RXSkips;
int RXFaults;
int TXFaults;
int RXTotal;
int TXTotal;

void HandleKey( int keycode, int bDown ) {
	if( keycode == 27 || keycode == 'q' || keycode == 'Q' ) exit(-1); 
	if( keycode >= '1' && keycode <= '9' )
	{
		int code = keycode - '1';
		colormode = !colormode;
		colormodechangetime = OGGetAbsoluteTime();
	}
}

int lastx, lasty;

void HandleButton( int x, int y, int button, int bDown ) { }
void HandleMotion( int x, int y, int mask ) { lastx = x; lasty = y; }
void HandleDestroy() { }


unsigned long HSVtoHEX( float hue, float sat, float value );


void * rxthread( void * v )
{
	int i;
	int res;
	res = 100;
	int lastreads;
	int countr = 0;
	int first = 1;
	int lastkey = 0;
	double LastFPSTime = OGGetAbsoluteTime();
	while(1)
	{
		res = hid_read(handle, RXbuf, 64);
		if( res != 64 ) RXFaults++;
		int reads = ( RXbuf[0]<<8 ) | RXbuf[1];
		RXdeltaReads = (uint16_t)(reads - lastreads);
		lastreads = reads;
		countr ++;
		if( !first )
		{
			if( (uint8_t)(lastkey + 1) != RXbuf[2] )
			{
				RXSkips ++;
			}
		}
		first = 0;
		lastkey = RXbuf[2];

		if( OGGetAbsoluteTime() > LastFPSTime + 1 )
		{
			LastFPSTime++;
			RXHz = countr;
			countr = 0;
		}
		
#ifdef PROFILE
		printf( "." ); fflush( stdout );
#endif

		for( i = 0; i < 64; i++ ) printf( "%02x", RXbuf[i] ); printf( "\n" );
			printf( "%03d:%03d [", reads-lastreads, RXbuf[2] );

		temperature = RXbuf[4] | (RXbuf[5]<<8);
		adc = RXbuf[6] | (RXbuf[7]<<8);
		voltage = RXbuf[8] | (RXbuf[9]<<8);
/*		for( i = 0; i < 38/2; i++ )
		{
			uint8_t marker = i * 3 + 4;
			uint16_t val1 = (((uint16_t)RXbuf[marker])<<4) | (RXbuf[marker+1]>>4);
			uint16_t val2 = (((uint16_t)RXbuf[marker+1]&0x0f)<<8) | (RXbuf[marker+2]);
			ADCs[i*2+0] = val1;
			ADCs[i*2+1] = val2;
		}
*/
//		printf( "]\n" );

		RXTotal++;
	}
}

void * txthread( void * v )
{
	int i, res;
	int frame = 0;

	double LastFPSTime = OGGetAbsoluteTime();
	int countr = 0;
	while(1)
	{
		//This whole section does cool stuff with LEDs
		int allledbytes = 20*4;
		for( i = 0; i < allledbytes; i+=4 )
		{
			uint32_t rk;

			//switch( colormode )
		//	{
		//	default: rk = HSVtoHEX( i * 0.01+ frame* .01, 1.0, 1.0 ); break;
		//	case 0:  rk = HSVtoHEX( 0, 0.0, 1.0 + colormodechangetime - OGGetAbsoluteTime() ); break;
		//	}

			float sat = OGGetAbsoluteTime() - colormodechangetime;

			rk = HSVtoHEX( i * 0.012+ frame* .01, (sat<1)?sat:1, 1.0 );

			int white = (int)((1.-sat) * 255);
			if( white > 255 ) white = 255;
			if( white < 0 ) white = 0;

			Colorbuf[i+0] = rk>>8;
			Colorbuf[i+1] = rk;
			Colorbuf[i+2] = rk>>16;
			Colorbuf[i+3] = white;
		}
			//96..111 = brighter.

		//This section does the crazy wacky stuff to actually split the LEDs into HID Packets and get them out the door... Carefully.
		int byrem = allledbytes;
		int offset = 0;
		for( i = 0; i < 2; i++ )
		{
			uint8_t sendbuf[65];
			sendbuf[0] = 0x00;
			sendbuf[1] = (byrem > 60)?15:(byrem/4);
			sendbuf[2] = offset;

			memcpy( sendbuf + 3, Colorbuf + offset*4, sendbuf[1]*4 );

			offset += sendbuf[1];
			byrem -= sendbuf[1]*4;


			if( byrem == 0 ) sendbuf[1] |= 0x80;
			int tsend = 65; //Size of payload (must be 64+1 always)


		//	res = hid_send_feature_report( handle, sendbuf, tsend);	//Method 1 control transfer (janky on STM32F0)
			res = hid_write(handle, sendbuf+1, tsend = 64 );
			//printf( "RES: %d\n", res );

			usleep(1000);
			if( res != tsend ) TXFaults++;
		}


#ifdef PROFILE
		printf( "x" ); fflush( stdout );
#endif
		usleep(10000);
		frame++;

		countr++;
		if( OGGetAbsoluteTime() > LastFPSTime + 1 )
		{
			LastFPSTime ++;
			TXHz = countr;
			countr = 0;
		}
		TXTotal++;
	}
}

int main()
{
	char stt[1024];
	int i, x, y;
	int frames = 0;

	//Initialize USB HID stuff.
	hid_init();
	handle = hid_open(0xabcd, 0xf410, NULL);
	if( handle == NULL )
	{
		fprintf( stderr, "No device\n" );
		return -1;
	}


	CNFGSetup( "STM32F042 CNL Debugger", 600, 400 );
	CNFGBGColor = 0x800000;
	CNFGDialogColor = 0x444444;
	OGCreateThread( txthread, 0 );
	OGCreateThread( rxthread, 0 );

	while(1)
	{

		CNFGHandleInput();

		CNFGClearFrame();
		CNFGColor( 0xFFFFFF );
		CNFGGetDimensions( &screenx, &screeny );

		int t;
		for( t = 0; t < 3; t++ )
		{
			CNFGTackSegment( t * 20, RXbuf[20+t] * 10 + 30, (t+1)*20, RXbuf[20+t] * 10 + 30 );
		}

		for( y = 0; y <= 1; y++ )
		for( x = 0; x <= 1; x++ )
		{
			CNFGPenX = 3+x;
			CNFGPenY = 3+y;
			CNFGDrawText( "STM32F042 TEST PLATFORM DEBUG CONSOLE", 4 );
		}
		CNFGPenX = screenx - 160;
		CNFGPenY = 3;
		if( TXHz < 40 || RXHz < 950 || RXdeltaReads != 38 || RXSkips > 0 || TXFaults != 0 || RXFaults != 0 )
			CNFGColor( 0x8080FF );
		else
			CNFGColor( 0xffffff );
		sprintf( stt, "TX Hz:    %d\nRX Hz:    %d\nDReads:   %d\nRXSkips:  %d\nTXFaults: %d\nRXFaults: %d\nTXTotal:  %d\nRXTotal:  %d\n5VUSB:    %4.2f\nTemp(C):  %4.1f\nADC: %4.1f", TXHz, RXHz, RXdeltaReads, RXSkips, TXFaults, RXFaults, TXTotal, RXTotal, voltage / 4096.0 * 3.60 *2.0, temperature/ 10.0, adc / 4096.*3.3 ); 
		CNFGDrawText( stt, 3 );

		frames++;
		CNFGSwapBuffers();
		usleep(10000);
	}

#if 0
	int frame = 0;
	//memcpy( buf, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!", 64 );

	for( i = 1; i < sizeof(buf); i++ )
		buf[i] = i;

	while(1)
	{
		buf[0] = 0;
		res = hid_send_feature_report( handle, buf, 512);
		//printf( "TRES: %d = %d\n", res, frame );
		printf( "." ); fflush( stdout );
		//if( res == -1 ) break;

		usleep(1000000);
	}
#endif

	hid_close( handle );
}












unsigned long HSVtoHEX( float hue, float sat, float value )
{
	float pr = 0;
	float pg = 0;
	float pb = 0;

	short ora = 0;
	short og = 0;
	short ob = 0;

	float ro = fmod( hue * 6, 6. );

	float avg = 0;

	ro = fmod( ro + 6 + 1, 6 ); //Hue was 60* off...

	if( ro < 1 ) //yellow->red
	{
		pr = 1;
		pg = 1. - ro;
	} else if( ro < 2 )
	{
		pr = 1;
		pb = ro - 1.;
	} else if( ro < 3 )
	{
		pr = 3. - ro;
		pb = 1;
	} else if( ro < 4 )
	{
		pb = 1;
		pg = ro - 3;
	} else if( ro < 5 )
	{
		pb = 5 - ro;
		pg = 1;
	} else
	{
		pg = 1;
		pr = ro - 5;
	}

	//Actually, above math is backwards, oops!
	pr *= value;
	pg *= value;
	pb *= value;

	avg += pr;
	avg += pg;
	avg += pb;

	pr = pr * sat + avg * (1.-sat);
	pg = pg * sat + avg * (1.-sat);
	pb = pb * sat + avg * (1.-sat);

	ora = pr * 255;
	og = pb * 255;
	ob = pg * 255;

	if( ora < 0 ) ora = 0;
	if( ora > 255 ) ora = 255;
	if( og < 0 ) og = 0;
	if( og > 255 ) og = 255;
	if( ob < 0 ) ob = 0;
	if( ob > 255 ) ob = 255;

	return (ob<<16) | (og<<8) | ora;
}



