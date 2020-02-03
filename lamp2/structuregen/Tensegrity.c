#include <stdio.h>
#include <math.h>

#define ABS(x) (((x)<0)?-(x):(x))

#define TAU (6.2831853)
#define ETCH "stroke=\"#ff0000\" fill=\"none\" stroke-width=\".05mm\""
#define CUT  "stroke=\"#0000ff\" fill=\"none\" stroke-width=\".05mm\""
#define MATERIAL_THICKNESS 5.5
//1.6
#define SWIRVINESS 10.0
#define LOUPES 5
#define SUPPORTSY 1.2
#define SUPPORTSX 1.0
#define SWEEPSQUASH  .88
#define TIGHTEN -.1 //For offsetting sockets to make them tighter.
#define HOLESIZE 1.3
#define SWIZZLEQTY 80
#define SWIZZLEPOWER .07
#define SWIZZLEANGDELTA .005 //In radians

float rs[] = { 54, 74, 77, 97 };

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
	float width = 103;
	float height = 102;
	float centerx = width/2+2;
	float centery = height/2-3;
	StartSVG( width, height );
	
	int i, k;

	float circleposes[6];
	
	{
		//The two arms
		float rotates[2] = { -1.1, 2.35 };
		for(k = 0; k < 2; k++ )
		{
			float cx = centerx+(k?-17:-24);
			float cy = centery+(k?-4:-3.5);
			float kpx = cx*cos(rotates[k]) + cy*sin(rotates[k]);
			float kpy =-cx*sin(rotates[k]) + cy*cos(rotates[k]);
			float keyposx = 0;
			float keyposy = 0;
			float ofs = 0;//k?-1.5707:0;
			//First make the toothed part.
			float am0 = rs[k*2+0]/2.0;
			float bm0 = rs[k*2+1]/2.0;
			float am = keyposx + am0;
			float bm = keyposx + bm0;
			float ha = keyposy - MATERIAL_THICKNESS/2.0;
			float hb = keyposy + MATERIAL_THICKNESS/2.0;

			printf( "<g transform=\"rotate(%f)\">\n", rotates[k]*180.0/3.14159 );
			PathStart( CUT );
			PathM( lerp( am, bm, 0.0 )+kpx, ha+kpy );
			PathL( lerp( am, bm, 0.2 )+kpx, ha+kpy );
			PathL( lerp( am, bm, 0.2 )+kpx, hb+kpy );
			PathL( lerp( am, bm, 0.4 )+kpx, hb+kpy );
			PathL( lerp( am, bm, 0.4 )+kpx, ha+kpy );
			PathL( lerp( am, bm, 0.6 )+kpx, ha+kpy );
			PathL( lerp( am, bm, 0.6 )+kpx, hb+kpy );
			PathL( lerp( am, bm, 0.8 )+kpx, hb+kpy );
			PathL( lerp( am, bm, 0.8 )+kpx, ha+kpy );
			PathL( lerp( am, bm, 1.0 )+kpx, ha+kpy );

			if( k == 0 )
			{
				circleposes[k*2+0] = lerp( am, bm, 0.5 );
				circleposes[k*2+1] = ha - 4;
			}

			//Then make the smooth part.
			for( i = 0; i < 90; i++ )
			{
				float ai = (i)/90.0;
				float ang = (sin( ai * 3.14159 ) ) * 1.2 /*Amount of arc */; //Goes 0...1.5707...0
				ang *= SWEEPSQUASH; //Don't make it a full sweep.
				float smoothai = smooth( (ai*2.0-1.0)*5.0 );
				//smoothai needs to have bit of a bump in the middle, near zero
				float r = lerp( bm0, am0, smoothai * 0.5 + 0.5 );
				float xv = cos(ang)*r*(ABS(sin(ai*3.14159*2))*.2/*Amount of bow*/+1);
				//fprintf( stderr, "%f %d %f\n", ai, i, r );
				xv /= (1.0+ang*.1);
				PathL( keyposx+xv*SUPPORTSX + kpx, ha-sin(ang)*r*SUPPORTSY+kpy );
				if( i == 36 )
				{
					float lr = (bm0+am0)/2.0;
					float xv = cos(ang)*lr;
					xv /= (1.0+ang*.1);
					circleposes[k*2+2] = keyposx+xv*SUPPORTSX+HOLESIZE+kpx;
					circleposes[k*2+3] = ha-sin(ang)*lr*SUPPORTSY+kpy;
				}
			}

			PathClose();
			printf( "</g>\n" );
		}
		
		printf( "<g transform=\"rotate(%f)\">\n", rotates[0]*180.0/3.14159 );
		//Circle( CUT, circleposes[0], circleposes[1], 2 );
		Circle( CUT, circleposes[2], circleposes[3], HOLESIZE );
		printf( "</g>\n" );
		printf( "<g transform=\"rotate(%f)\">\n", rotates[1]*180.0/3.14159 );
		Circle( CUT, circleposes[4], circleposes[5], HOLESIZE );
		printf( "</g>\n" );
	}
	
	//The insertion points in the main planes.
	for( i = 0; i < 2; i++ )
	{
		// 
		float am = centerx+(i?(-rs[i*2+0]/2.0+SWIRVINESS*2):(-rs[i*2+0]/2.0+SWIRVINESS*2+20));
		float bm = centerx+(i?(-rs[i*2+1]/2.0+SWIRVINESS*2):(-rs[i*2+1]/2.0+SWIRVINESS*2+20));
		float ha = centery - MATERIAL_THICKNESS/2.0 + TIGHTEN;
		float hb = centery + MATERIAL_THICKNESS/2.0 - TIGHTEN;
		
		float ltighten = -TIGHTEN;
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
	
	float circleangles[2] = { 180-36, 180+36 };
	for( k = 0; k < 2; k++ )
	{
		float r0 = rs[k*2+0];
		float r1 = rs[k*2+1];
		float tr = (r0+r1)/2;
		float r = tr;

		for( i = 0; i < 2; i++ )
		{
		//	float td0 = (i+0.75)/3.0*TAU;
		//	float d0 = (i+0)/3.*TAU;
		//	if( i == 0 && k == 0 ) continue;
		//float r0 = r + cos( d0*LOUPES ) * SWIRVINESS - SWIRVINESS;
		//	Circle( CUT, sin(td0)*r0 + centerx, cos(td0)*r0 + centery, 2 );
			float d0 = (circleangles[i])/360.0*TAU;
			float r0 = r / 2.0 + cos( d0*LOUPES ) * SWIRVINESS - SWIRVINESS;
			//if( i == 0 ) PathM( r0 * cos( d0 ) + centerx, r0 * sin( d0 ) + centery );
			//PathL( r1 * cos( d1 ) + centerx, r1 * sin( d1 ) + centery );
			Circle( CUT, r0 * cos( d0 ) + centerx, r0 * sin( d0 ) + centery, HOLESIZE );		
		}
	}

	//The main body
	for( k = 1; k < 4; k+=2 )
	{
		int i;
		float r = rs[k];
		PathStart( CUT );
		for( i = 0; i < 180; i++ )
		{
			float d0 = (i+0)/180.0*TAU;
			float d1 = (i+1)/180.0*TAU;
			float r0 = r / 2.0 + cos( d0*LOUPES ) * SWIRVINESS - SWIRVINESS;
			float r1 = r / 2.0 + cos( d1*LOUPES ) * SWIRVINESS - SWIRVINESS;
			if( i == 0 ) PathM( r0 * cos( d0 ) + centerx, r0 * sin( d0 ) + centery );
			PathL( r1 * cos( d1 ) + centerx, r1 * sin( d1 ) + centery );
		}
		PathClose();
	}
	
	//Draw the swizzles./
		//The circles
	for( k = 0; k < 2; k++ )
	{
		int i;
		for( i = 0; i < 1; i++ )
		{
			float r = (rs[k*2+1]+rs[k*2+0])/2;
			PathStart( ETCH );
			float ang = 0;
			for( ; ang < TAU; ang+=SWIZZLEANGDELTA )
			{
				float d0 = ang;
				float d1 = ang+SWIZZLEANGDELTA;
				float r0 = r / 2.0 + cos( d0*LOUPES ) * SWIRVINESS - SWIRVINESS;
				float r1 = r / 2.0 + cos( d1*LOUPES ) * SWIRVINESS - SWIRVINESS;
				int iofs = i?0:3.14159;
				float swizzx = 0;
				float swizzy = 0;
				{
					float cx0 = r0 * cos( d0 );
					float cy0 = r0 * sin( d0 );
					float cx1 = r1 * cos( d1 );
					float cy1 = r1 * sin( d1 );
					float tx = cx1 - cx0;
					float ty = cy1 - cy0;
					//normalize t's.
					float ta = sqrt( tx*tx+ty*ty );
					tx /= ta;
					ty /= ta;
					float nx = -ty;
					float ny = tx;
					float dr1 = 1.0-ABS(r1 - r0)*2;
					float swizzdiff = cos(SWIZZLEQTY*ang+iofs)*SWIZZLEPOWER*r1*dr1;
					swizzx = nx * swizzdiff;// + tx * swizzdiff;
					swizzy = ny * swizzdiff;// + ty * swizzdiff;
				}
				if( ang == 0 ) PathM( r0 * cos( d0 ) + centerx + swizzx, r0 * sin( d0 ) + centery + swizzy );
				PathL( r1 * cos( d1 ) + centerx + swizzx, r1 * sin( d1 ) + centery + swizzy );
			}
			PathClose();
		}
	}
		
	EndSVG();

}
