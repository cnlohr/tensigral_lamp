#include <stdio.h>
#include <math.h>

#define TAU (6.2831853)
#define ETCH "stroke=\"#ff0000\" fill=\"none\" stroke-width=\".05mm\""
#define CUT  "stroke=\"#0000ff\" fill=\"none\" stroke-width=\".05mm\""
#define MATERIAL_THICKNESS 1.6
#define SWIRVINESS 14.0
#define SUPPORTSY 1.1
#define SUPPORTSX 1.0
#define SWEEPSQUASH  .88
#define TIGHTEN -.1 //For offsetting sockets to make them tighter.

float rs[] = { 58, 82, 86, 110 };

//CNLohr's Super Basic SVG Toolkit.
int inpath = 0;
int started = 0;
const char * lastcolor = "black";
float centerx, centery;
void StartSVG( float width, float height ) { printf( "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n" ); printf( "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%fmm\" height=\"%fmm\" x=\"0\" y=\"0\" viewBox=\"0 0 %f %f\">\n", width, height, width, height ); }
void PathClose() { if( !inpath ) return; printf( "Z\" />\n" ); inpath = 0; }
void PathStart( const char * props ) { if( inpath ) PathClose(); lastcolor = props; printf( "<path %s d=\"", props ); inpath = 1; started = 0; }
void PathM( float x, float y ) { if( !inpath ) PathStart(lastcolor); printf( "M%f %f ", x+centerx, y+centery ); started = 1; }
void PathL( float x, float y ) { if( !inpath ) PathStart(lastcolor); printf( "%c%f %f ", started?'L':'M', x+centerx, y+centery ); started = 1; }
void PathQ( float xc, float yc, float x, float y ) { if( !inpath ) PathStart(lastcolor); printf( "Q%f %f, %f %f ", xc+centerx, yc+centery, x+centerx, y+centery ); started = 1; }
void PathAS( float r, float x, float y, int laf, int sf ) { printf( "A%f %f 0 %d %d %f %f ", r, r, laf, sf, x+centerx, y+centery ); }
void Circle( const char * props, float x, float y, float r ) { if( inpath ) PathClose(); printf( "<circle  cx=\"%f\" cy=\"%f\" r=\"%f\" %s />\n", x+centerx, y+centery, r, props ); }
void EndSVG() { PathClose(); printf( "</svg>" ); }

float smooth( float t ) { return t / sqrt(t*t+1.); }
float lerp( float a, float b, float t ) { return a * (1.-t) + b * t; }
void Scale2d( float * out, float * a, float scale ) { out[0] = a[0] * scale; out[1] = a[1] * scale; }
void Sub2d( float * out, float * a, float * b ) { out[0] = a[0] - b[0]; out[1] = a[1] - b[1]; }
void Add2d( float * out, float * a, float * b ) { out[0] = a[0] + b[0]; out[1] = a[1] + b[1]; }
void Normalize2d( float * out, float * in ) { float mag = 1./sqrt( in[0] * in[0] + in[1] * in[1] ); Scale2d( out, in, mag ); }
void Normal2d( float * out, float * in ) { out[0] = -in[1]; out[1] = in[0]; }

int main()
{
	float width = 100;
	float height = 100;
	float centerx = width/2-13;
	float centery = height/2;
	StartSVG( width, height );
	
	int i, k;

	float circleposes[6];
	
	float rotates[2] = { -38, 94.5 };
	//The two arms
	for(k = 0; k < 2; k++ )
	{
		float keyposx = (k?4.5:-18.5) + centerx;
		float keyposy = (k?-96.8:36) + centery;
		float ofs = k?-1.5707:0;
		//First make the toothed part.
		float am0 = rs[k*2+0]/2.0;
		float bm0 = rs[k*2+1]/2.0;
		float am = keyposx + am0;
		float bm = keyposx + bm0;
		float ha = keyposy - MATERIAL_THICKNESS/2.0;
		float hb = keyposy + MATERIAL_THICKNESS/2.0;

		printf( "<g transform=\"rotate(%f)\">\n", rotates[k] );
		PathStart( CUT );
		PathM( lerp( am, bm, 0.0 ), ha );
		PathL( lerp( am, bm, 0.2 ), ha );
		PathL( lerp( am, bm, 0.2 ), hb );
		PathL( lerp( am, bm, 0.4 ), hb );
		PathL( lerp( am, bm, 0.4 ), ha );
		PathL( lerp( am, bm, 0.6 ), ha );
		PathL( lerp( am, bm, 0.6 ), hb );
		PathL( lerp( am, bm, 0.8 ), hb );
		PathL( lerp( am, bm, 0.8 ), ha );
		PathL( lerp( am, bm, 1.0 ), ha );

		if( k == 0 )
		{
			circleposes[k*2+0] = lerp( am, bm, 0.5 );
			circleposes[k*2+1] = ha - 4;
		}

		//Then make the smooth part.
		for( i = 0; i < 90; i++ )
		{
			float ai = (i)/90.0;
			float ang = (sin( ai * 3.14159 ) ) * 1.5707; //Goes 0...1.5707...0
			ang *= SWEEPSQUASH; //Don't make it a full sweep.
			float smoothai = smooth( (ai*2.0-1.0)*5.0 );
			//smoothai needs to have bit of a bump in the middle, near zero
			float r = lerp( bm0, am0, smoothai * 0.5 + 0.5 );
			float xv = cos(ang)*r;
			//fprintf( stderr, "%f %d %f\n", ai, i, r );
			xv /= (1.0+ang*.1);
			PathL( keyposx+xv*SUPPORTSX, ha-sin(ang)*r*SUPPORTSY );
			if( i == 33 )
			{
				float lr = (bm0+am0)/2.0;
				float xv = cos(ang)*lr;
				xv /= (1.0+ang*.1);
				circleposes[k*2+2] = keyposx+xv*SUPPORTSX;
				circleposes[k*2+3] = ha-sin(ang)*lr*SUPPORTSY;
			}
		}
		PathClose();
		printf( "</g>\n" );
	}
	
	printf( "<g transform=\"rotate(%f)\">\n", rotates[0] );
	Circle( CUT, circleposes[0], circleposes[1], 2 );
	Circle( CUT, circleposes[2], circleposes[3], 2 );
	printf( "</g>\n" );
	printf( "<g transform=\"rotate(%f)\">\n", rotates[1] );
	Circle( CUT, circleposes[4], circleposes[5], 2 );
	printf( "</g>\n" );

	for( k = 0; k < 2; k++ )
	{
		float r0 = rs[k*2+0];
		float r1 = rs[k*2+1];
		float tr = (r0+r1)/2;
			tr = tr/2.0;

		for( i = 0; i < 3; i++ )
		{
			float d0 = (i+0.75)/3.0*TAU;
			if( i == 0 && k == 0 ) continue;
			Circle( CUT, sin(d0)*tr + centerx, cos(d0)*tr + centery, 2 );
		}
	}
	
	//The insertion points in the main planes.
	for( i = 0; i < 2; i++ )
	{
		float am = centerx+(i?(-rs[i*2+0]/2.0+SWIRVINESS*2):(rs[i*2+0]/2.0));
		float bm = centerx+(i?(-rs[i*2+1]/2.0+SWIRVINESS*2):(rs[i*2+1]/2.0));
		float ha = centery - MATERIAL_THICKNESS/2.0 + TIGHTEN;
		float hb = centery + MATERIAL_THICKNESS/2.0 - TIGHTEN;
		
		float ltighten = (i?1:-1) * TIGHTEN;
		PathStart( CUT );
		PathM( lerp( am, bm, 0.2 ) - ltighten, ha );
		PathL( lerp( am, bm, 0.2 ) - ltighten, hb );
		PathL( lerp( am, bm, 0.4 ) + ltighten, hb );
		PathL( lerp( am, bm, 0.4 ) + ltighten, ha );
		PathClose();
		PathStart( CUT );
		PathM( lerp( am, bm, 0.6 ) - ltighten, ha );
		PathL( lerp( am, bm, 0.6 ) - ltighten, hb );
		PathL( lerp( am, bm, 0.8 ) + ltighten, hb );
		PathL( lerp( am, bm, 0.8 ) + ltighten, ha );
		PathClose();
	}

	//The circles
	for( k = 1; k < 4; k++ )
	{
		int i;
		float r = rs[k];
		PathStart( CUT );
		for( i = 0; i < 180; i++ )
		{
			float d0 = (i+0)/180.0*TAU;
			float d1 = (i+1)/180.0*TAU;
			float r0 = r / 2.0 + cos( d0*3 ) * SWIRVINESS - SWIRVINESS;
			float r1 = r / 2.0 + cos( d1*3 ) * SWIRVINESS - SWIRVINESS;
			if( i == 0 ) PathM( r0 * cos( d0 ) + centerx, r0 * sin( d0 ) + centery );
			PathL( r1 * cos( d1 ) + centerx, r1 * sin( d1 ) + centery );
		}
		PathClose();
	}
	
		
	EndSVG();

}
