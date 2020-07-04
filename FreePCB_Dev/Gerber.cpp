// make Gerber file from partlist, netlist, textlist, etc.
//
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *		Author:    Allan Wright (ver. 1.0 - 1.359)		   *
 *		URL: www.freepcb.com							   *
 *		Maintained:    Duxah (ver. 2.0 - 2.033)			   *
 *		email: duxah@yahoo.com							   *
 *		URL: www.freepcb.dev							   *
 *		Copyright: (C) Duxah 2014 - 2020.				   *
 *		This software is free for non-commercial use.	   *
 *		It may be copied, modified, and redistributed	   *
 *		provided that this copyright notice is 			   *
 *		preserved on all copies. You may not use this	   *
 *		software, in whole or in part, in support of	   *
 *		any commercial product without the express 		   *
 *		consent of the authors.							   *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#include "stdafx.h"
#include "Gerber.h"
#include <math.h>
#include <afxcoll.h>
#include <afxtempl.h>

#define pi  3.14159265359
int THERMAL_W;
#define CLEARANCE_POLY_STROKE_MILS 1

class c_cutout {
public:
	cnet * net;
	int ia;
//	int ic;
};

class c_area {
public:
	cnet * net;
	int ia;
	CArray< c_cutout > containers;
};

// constructor
CAperture::CAperture()
{
	m_type = 0;
	m_size1 = 0;
	m_size2 = 0;
}

// constructor
CAperture::CAperture( int type, int size1, int size2, int size3, int size4 ) 
{
	m_type = type;
	m_size1 = size1;
	m_size2 = size2;
	m_size3 = size3;
	m_size4 = size4;
}

// destructor
CAperture::~CAperture()
{
}

// test for equality
//
BOOL CAperture::Equals( CAperture * ap )
{
	if( m_type == ap->m_type )
	{
		if( m_type == AP_CIRCLE && m_size1 == ap->m_size1 )
			return TRUE;
		else if( m_type == AP_SQUARE && m_size1 == ap->m_size1 )
			return TRUE;
		else if( m_type == AP_OCTAGON && m_size1 == ap->m_size1 )
			return TRUE;
		else if( m_type == AP_RECT && m_size1 == ap->m_size1 && m_size2 == ap->m_size2 )
			return TRUE;
		else if( m_type == AP_RRECT && m_size1 == ap->m_size1 && m_size2 == ap->m_size2 
				&& m_size3 == ap->m_size3 )
			return TRUE;
		else if( m_type == AP_RECT_THERMAL && m_size1 == ap->m_size1 && m_size2 == ap->m_size2 
				&& m_size3 == ap->m_size3 && m_size4 == ap->m_size4 )
			return TRUE;
		else if( m_type == AP_RECT_THERMAL_45 && m_size1 == ap->m_size1 && m_size2 == ap->m_size2 
				&& m_size3 == ap->m_size3 && m_size4 == ap->m_size4 )
			return TRUE;
		else if( m_type == AP_THERMAL && m_size1 == ap->m_size1 && m_size2 == ap->m_size2 )
			return TRUE;
		else if( m_type == AP_MOIRE && m_size1 == ap->m_size1 && m_size2 == ap->m_size2 )
			return TRUE;
		else if( m_type == AP_OVAL && m_size1 == ap->m_size1 && m_size2 == ap->m_size2 )
			return TRUE;
	}
	return FALSE;
}

// find aperture in array 
// if not found and ok_to_add == TRUE, add to array
// return posiiton in array, or -1 if not found and not added
//
int CAperture::FindInArray( aperture_array * ap_array, BOOL ok_to_add )
{
	int na = ap_array->GetSize();
	int ifound = -1;
	for( int ia=0; ia<na; ia++ )
	{
		if( this->Equals( &(ap_array->GetAt(ia)) ) )
		{
			ifound = ia;
			break;
		}
	}
	if( ifound != -1 )
		return ifound;
	if( ok_to_add )
	{
		ap_array->Add( *this );
		return ap_array->GetSize()-1;
	}
	ASSERT(0);
	return -1;
}

// things that you can do with a light
enum {
	LIGHT_NONE,
	LIGHT_ON,
	LIGHT_OFF,
	LIGHT_FLASH
};

// generate a "moveto" string for the Gerber file
// enter with:
//	x, y = coords in NM
//
void WriteMoveTo( CStdioFile * f, int x, int y, int light_state )
{
	_int64 x_10 = (_int64)10*x/NM_PER_MIL;	// 2.4
	_int64 y_10 = (_int64)10*y/NM_PER_MIL;	// 2.4
	CString str;
	if( light_state == LIGHT_NONE )
		ASSERT(0);
	else
		str.Format( "G01X%I64dY%I64dD0%d*\n", x_10, y_10, light_state );
	f->WriteString( str );
}

// draw one side of a CPolyLine by writing commands to Gerber file
// the side may be straight or an arc
// arc is aproximated by straight-line segments
// assumes that plotter already at x1, y1
// does not turn the light on or off
// dimensions are in NM
//
void WritePolygonSide( CStdioFile * f, int x1, int y1, int x2, int y2, int style, int nsteps, int light_state )
{
	int n;	// number of steps for arcs
	n = (abs(x2-x1)+abs(y2-y1))/(5*NM_PER_MIL);	// step size approx. 3 to 5 mil
	n = max( n, nsteps );	// or at most 5 degrees of arc
	CString line;
	double xo, yo, a, b, theta1, theta2;
	a = fabs( (double)(x1 - x2) );
	b = fabs( (double)(y1 - y2) );

	if( style == CPolyLine::STRAIGHT )
	{
		// just draw a straight line with linear interpolation
		WriteMoveTo( f, x2, y2, light_state );
		return;
	}
	else if( style == CPolyLine::ARC_CW )
	{
		// clockwise arc (ie.quadrant of ellipse)
		int i=0, j=0;
		if( x2 > x1 && y2 > y1 )
		{
			// first quadrant, draw second quadrant of ellipse
			xo = x2;	
			yo = y1;
			theta1 = pi;
			theta2 = pi/2.0;
		}
		else if( x2 < x1 && y2 > y1 )
		{
			// second quadrant, draw third quadrant of ellipse
			xo = x1;	
			yo = y2;
			theta1 = 3.0*pi/2.0;
			theta2 = pi;
		}
		else if( x2 < x1 && y2 < y1 )	
		{
			// third quadrant, draw fourth quadrant of ellipse
			xo = x2;	
			yo = y1;
			theta1 = 2.0*pi;
			theta2 = 3.0*pi/2.0;
		}
		else
		{
			xo = x1;	// fourth quadrant, draw first quadrant of ellipse
			yo = y2;
			theta1 = pi/2.0;
			theta2 = 0.0;
		}
	}
	else if( style == CPolyLine::ARC_CCW )
	{
		// counter-clockwise arc
		int i=0, j=0;
		if( x2 > x1 && y2 > y1 )
		{
			xo = x1;	// first quadrant, draw fourth quadrant of ellipse
			yo = y2;
			theta1 = 3.0*pi/2.0;
			theta2 = 2.0*pi;
		}
		else if( x2 < x1 && y2 > y1 )
		{
			xo = x2;	// second quadrant
			yo = y1;
			theta1 = 0.0;
			theta2 = pi/2.0;
		}
		else if( x2 < x1 && y2 < y1 )	
		{
			xo = x1;	// third quadrant
			yo = y2;
			theta1 = pi/2.0;
			theta2 = pi;
		}
		else
		{
			xo = x2;	// fourth quadrant
			yo = y1;
			theta1 = pi;
			theta2 = 3.0*pi/2.0;
		}
	}
	else
		ASSERT(0);
	// now write steps
	for( int is=1; is<=n; is++ )
	{
		double theta = theta1 + ((theta2-theta1)*(double)is)/n;
		double x = xo + a*cos(theta);
		double y = yo + b*sin(theta);
		if( is == n )
		{
			x = x2;
			y = y2;
		}
		WriteMoveTo( f, x, y, light_state );
	}
}



// Change aperture if necessary
// If PASS0, just make sure that the aperture is in ap_array
// Otherwise, write aperture change to file if necessary
//
void ChangeAperture( CAperture * new_ap,			// new aperture
					CAperture * current_ap,			// current aperture
					aperture_array * ap_array,		// array of apertures
					BOOL PASS0,						// flag for PASS0
					CStdioFile * f )				// file to write to
{
	int current_iap;
	CString line;
	new_ap->m_size1 = max(new_ap->m_size1, _2540);
	if( !(*current_ap).Equals( new_ap ) )
	{
		// change aperture
		current_iap = new_ap->FindInArray( ap_array, PASS0 );
		if( !PASS0 )
		{
			*current_ap = *new_ap;
			line.Format( "G54D%2d*\n", current_iap+10 );
			f->WriteString( line );	 // select new aperture
		}
	}
}

void WriteTiltPolygon( int x, int y, int t, int s1, int s2, int s3, int s4, int r, int ang, CStdioFile * f )
{
#define cnt_pts 20
static CPoint rnd[cnt_pts];
	if( t == CAperture::AP_RECT_THERMAL_45 )
	{
		if( s1/2 < (s3+s4)*0.707 || s2/2 < (s3+s4)*0.707 )
			t = CAperture::AP_RECT_THERMAL;
	}
	else if( t == CAperture::AP_THERMAL )
	{
		t = CAperture::AP_RECT_THERMAL;
		s4 = s1-s2;
		s2 = s1;
		s3 = THERMAL_W;
	}
	// correct radius
	if( t == CAperture::AP_SQUARE )
	{
		r = 0;
		s2 = s1;
	}
	else if( t == CAperture::AP_CIRCLE )
		ASSERT(0);
	else if( t == CAperture::AP_RECT )
		r = 0;
	else if( t == CAperture::AP_OVAL )
		r = min(s1,s2)/2;
	else if( t == CAperture::AP_RECT_THERMAL )
	{
		s3 = min(s3, s1-2*s4);
		s3 = min(s3, s2-2*s4);
	}
	else if( t == CAperture::AP_RECT_THERMAL_45 )
	{
		r = s4;
	}
	else if( t == CAperture::AP_OCTAGON )
	{
		double alpha = 22.5;
		s1 = s1*cos(alpha*pi/180);
		r = s1/2 - s1/2*sin(alpha*pi/180);
		s2 = s1;
	}
	CPoint porg;
		porg.x = x;
		porg.y = y;
	if( t == CAperture::AP_RECT_THERMAL_45 )
	{
		double k = ((double)s2-(double)s3)/((double)s1-(double)s3);
		f->WriteString( "G36*\n" );
		CPoint p[6];
		if( s1 > (s2+((float)s3*1.5)) )
		{
			p[0].x = x + s1/2;
			p[0].y = y + s2/2 - (float)s3*0.707;
			p[1].x = x + s1/2;
			p[1].y = y - s2/2 + (float)s3*0.707;
			p[2].x = x + s1/2 - s2/2 + (float)s3*0.707;
			p[2].y = y;
	
			RotatePOINTS(p,3,-ang,porg);
			WriteMoveTo( f, p[2].x, p[2].y, LIGHT_OFF );
			WriteMoveTo( f, p[0].x, p[0].y, LIGHT_ON );
			WriteMoveTo( f, p[1].x, p[1].y, LIGHT_ON );
			WriteMoveTo( f, p[2].x, p[2].y, LIGHT_ON );

			RotatePOINTS(p,3,180,porg);
			WriteMoveTo( f, p[2].x, p[2].y, LIGHT_OFF );
			WriteMoveTo( f, p[0].x, p[0].y, LIGHT_ON );
			WriteMoveTo( f, p[1].x, p[1].y, LIGHT_ON );
			WriteMoveTo( f, p[2].x, p[2].y, LIGHT_ON );

			p[0].x = x + s1/2 - (float)s3*0.707;
			p[0].y = y + s2/2;
			p[1].x = x - s1/2 + (float)s3*0.707;
			p[1].y = y + s2/2;
			p[2].x = x - s1/2 + s2/2 + (float)s3*0.707;
			p[2].y = y;
			p[3].x = x - s1/2 + (float)s3*0.707;
			p[3].y = y - s2/2;
			p[4].x = x + s1/2 - (float)s3*0.707;
			p[4].y = y - s2/2;
			p[5].x = x + s1/2 - s2/2 - (float)s3*0.707;
			p[5].y = y;

			RotatePOINTS(p,6,-ang,porg);
			WriteMoveTo( f, p[5].x, p[5].y, LIGHT_OFF );
			for(int g=0;g<6;g++)
				WriteMoveTo( f, p[g].x, p[g].y, LIGHT_ON );
		}
		else if( s2 > (s1+(float)s3*1.5) )
		{
			p[0].y = y + s2/2;
			p[0].x = x + s1/2 - (float)s3*0.707;
			p[1].y = y + s2/2;
			p[1].x = x - s1/2 + (float)s3*0.707;
			p[2].y = y + s2/2 - s1/2 + (float)s3*0.707;
			p[2].x = x;
	
			RotatePOINTS(p,3,-ang,porg);
			WriteMoveTo( f, p[2].x, p[2].y, LIGHT_OFF );
			WriteMoveTo( f, p[0].x, p[0].y, LIGHT_ON );
			WriteMoveTo( f, p[1].x, p[1].y, LIGHT_ON );
			WriteMoveTo( f, p[2].x, p[2].y, LIGHT_ON );

			RotatePOINTS(p,3,180,porg);
			WriteMoveTo( f, p[2].x, p[2].y, LIGHT_OFF );
			WriteMoveTo( f, p[0].x, p[0].y, LIGHT_ON );
			WriteMoveTo( f, p[1].x, p[1].y, LIGHT_ON );
			WriteMoveTo( f, p[2].x, p[2].y, LIGHT_ON );

			p[0].y = y + s2/2 - (float)s3*0.707;
			p[0].x = x + s1/2;
			p[1].y = y - s2/2 + (float)s3*0.707;
			p[1].x = x + s1/2;
			p[2].y = y - s2/2 + s1/2 + (float)s3*0.707;
			p[2].x = x;
			p[3].y = y - s2/2 + (float)s3*0.707;
			p[3].x = x - s1/2;
			p[4].y = y + s2/2 - (float)s3*0.707;
			p[4].x = x - s1/2;
			p[5].y = y + s2/2 - s1/2 - (float)s3*0.707;
			p[5].x = x;

			RotatePOINTS(p,6,-ang,porg);
			WriteMoveTo( f, p[5].x, p[5].y, LIGHT_OFF );
			for(int g=0;g<6;g++)
				WriteMoveTo( f, p[g].x, p[g].y, LIGHT_ON );
		}
		else
		{
			p[0].x = x + s1/2;
			p[0].y = y + s2/2 - (float)s3*0.707;
			p[1].x = x + s1/2;
			p[1].y = y - s2/2 + (float)s3*0.707;
			p[2].x = x + s1/2 - s2/2 + (float)s3*0.707;
			p[2].y = y;
		
			RotatePOINTS(p,3,-ang,porg);
			WriteMoveTo( f, p[2].x, p[2].y, LIGHT_OFF );
			WriteMoveTo( f, p[0].x, p[0].y, LIGHT_ON );
			WriteMoveTo( f, p[1].x, p[1].y, LIGHT_ON );
			WriteMoveTo( f, p[2].x, p[2].y, LIGHT_ON );

			RotatePOINTS(p,3,180,porg);
			WriteMoveTo( f, p[2].x, p[2].y, LIGHT_OFF );
			WriteMoveTo( f, p[0].x, p[0].y, LIGHT_ON );
			WriteMoveTo( f, p[1].x, p[1].y, LIGHT_ON );
			WriteMoveTo( f, p[2].x, p[2].y, LIGHT_ON );

			p[0].y = y + s2/2;
			p[0].x = x + s1/2 - (float)s3*0.707;
			p[1].y = y + s2/2;
			p[1].x = x - s1/2 + (float)s3*0.707;
			p[2].y = y + s2/2 - s1/2 + (float)s3*0.707;
			p[2].x = x;

			RotatePOINTS(p,3,-ang,porg);
			WriteMoveTo( f, p[2].x, p[2].y, LIGHT_OFF );
			WriteMoveTo( f, p[0].x, p[0].y, LIGHT_ON );
			WriteMoveTo( f, p[1].x, p[1].y, LIGHT_ON );
			WriteMoveTo( f, p[2].x, p[2].y, LIGHT_ON );

			RotatePOINTS(p,3,180,porg);
			WriteMoveTo( f, p[2].x, p[2].y, LIGHT_OFF );
			WriteMoveTo( f, p[0].x, p[0].y, LIGHT_ON );
			WriteMoveTo( f, p[1].x, p[1].y, LIGHT_ON );
			WriteMoveTo( f, p[2].x, p[2].y, LIGHT_ON );
		}
		f->WriteString( "G37*\n" );
	}
	else
	{
		int npo;
		if( t == CAperture::AP_OCTAGON || r < NM_PER_MIL )
			npo = Gen_RndRectPoly( x,y,s1,s2,r,-ang, rnd, 8 );
		else
			npo = Gen_RndRectPoly( x,y,s1,s2,r,-ang, rnd, cnt_pts );
		if( npo )
		{
			if( t == CAperture::AP_SQUARE ||
				t == CAperture::AP_RECT ||
				t == CAperture::AP_RRECT ||
				t == CAperture::AP_OVAL ||
				t == CAperture::AP_OCTAGON )
			{
				f->WriteString( "G36*\n" );
				WriteMoveTo( f, rnd[0].x, rnd[0].y, LIGHT_OFF );
				for( int i=1; i<npo; i++ )
					WriteMoveTo( f, rnd[i].x, rnd[i].y, LIGHT_ON );
				f->WriteString( "G37*\n" );
			}
			else if( t == CAperture::AP_RECT_THERMAL )
			{
				if( npo%4 )
					ASSERT(0);
				//
				f->WriteString( "G36*\n" );
				WriteMoveTo( f, rnd[0].x, rnd[0].y, LIGHT_OFF );
				for( int i=1; i<npo/4; i++ )
					WriteMoveTo( f, rnd[i].x, rnd[i].y, LIGHT_ON );
	
				CPoint p[3];
				p[0].x = x+s3/2;
				p[0].y = y+s2/2;
				p[1].x = x+s3/2;
				p[1].y = y+s3/2;
				p[2].x = x+s1/2;
				p[2].y = y+s3/2;
				
				RotatePOINTS(p,3,-ang,porg);
				WriteMoveTo( f, p[0].x, p[0].y, LIGHT_ON );
				WriteMoveTo( f, p[1].x, p[1].y, LIGHT_ON );
				WriteMoveTo( f, p[2].x, p[2].y, LIGHT_ON );
				
				f->WriteString( "G37*\n" );
				//
				f->WriteString( "G36*\n" );
				WriteMoveTo( f, rnd[npo/4].x, rnd[npo/4].y, LIGHT_OFF );
				for( int i=npo/4+1; i<npo/2; i++ )
					WriteMoveTo( f, rnd[i].x, rnd[i].y, LIGHT_ON );
	
				p[0].x = x-s1/2;
				p[0].y = y+s3/2;
				p[1].x = x-s3/2;
				p[1].y = y+s3/2;
				p[2].x = x-s3/2;
				p[2].y = y+s2/2;
				RotatePOINTS(p,3,-ang,porg);
				WriteMoveTo( f, p[0].x, p[0].y, LIGHT_ON );
				WriteMoveTo( f, p[1].x, p[1].y, LIGHT_ON );
				WriteMoveTo( f, p[2].x, p[2].y, LIGHT_ON );
				
				f->WriteString( "G37*\n" );
				//
				f->WriteString( "G36*\n" );
				WriteMoveTo( f, rnd[npo/2].x, rnd[npo/2].y, LIGHT_OFF );
				for( int i=npo/2+1; i<3*npo/4; i++ )
					WriteMoveTo( f, rnd[i].x, rnd[i].y, LIGHT_ON );
				
				p[0].x = x-s3/2;
				p[0].y = y-s2/2;
				p[1].x = x-s3/2;
				p[1].y = y-s3/2;
				p[2].x = x-s1/2;
				p[2].y = y-s3/2;
				RotatePOINTS(p,3,-ang,porg);
				WriteMoveTo( f, p[0].x, p[0].y, LIGHT_ON );
				WriteMoveTo( f, p[1].x, p[1].y, LIGHT_ON );
				WriteMoveTo( f, p[2].x, p[2].y, LIGHT_ON );
				
				f->WriteString( "G37*\n" );
				//
				f->WriteString( "G36*\n" );
				WriteMoveTo( f, rnd[3*npo/4].x, rnd[3*npo/4].y, LIGHT_OFF );
				for( int i=3*npo/4+1; i<npo; i++ )
					WriteMoveTo( f, rnd[i].x, rnd[i].y, LIGHT_ON );
	
				p[0].x = x+s1/2;
				p[0].y = y-s3/2;
				p[1].x = x+s3/2;
				p[1].y = y-s3/2;
				p[2].x = x+s3/2;
				p[2].y = y-s2/2;
				RotatePOINTS(p,3,-ang,porg);
				WriteMoveTo( f, p[0].x, p[0].y, LIGHT_ON );
				WriteMoveTo( f, p[1].x, p[1].y, LIGHT_ON );
				WriteMoveTo( f, p[2].x, p[2].y, LIGHT_ON );
				
				f->WriteString( "G37*\n" );
			}
		}
	}
}


// draw clearance for a pad or trace segment in intersecting areas in foreign nets
// enter with shape = pad shape or -1 for trace segment
//
void DrawClearanceInForeignAreas(   cnet * net, int shape,
									int w, int xi, int yi, int xf, int yf,	// for line segment
									int wid, int len, int radius, int angle, // for pad
								    CStdioFile * f, int flags, int layer,
									int fill_clearance,
								    CArray<cnet*> * area_net_list,
									CArray<carea*> * area_list,
									CDlgLog * log=NULL
									)
{
	//
	double seg_angle, cw;
	BOOL bClearanceMade = FALSE;
	BOOL WRNNG = FALSE;
	gpc_vertex_list gpc_seg_contour;
	gpc_polygon gpc_seg_poly;
	gpc_vertex_list gpc_side_contour;
	gpc_polygon gpc_side_poly;
	int * i_Intersect;
	for( int ia=0; ia<area_list->GetSize(); ia++ ) 
	{
		// foreign net, see if possible intersection
		cnet * area_net = (*area_net_list)[ia];
		if( area_net == net )
			continue;
		carea * area = (*area_list)[ia];
		CPolyLine * p = area->poly;
		if( p->GetHatch() == CPolyLine::DIAGONAL_FULL )
			continue;
		if( p->GetHatch() == CPolyLine::NO_HATCH && p->GetW() == 0 )
			continue;//  area Ghost
		int a_w = p->GetW();
		int nc = p->GetNumCorners();
		i_Intersect = new int[nc];//ok
		nc = 0;
		BOOL bIntersection = FALSE;
		if( shape == -1 )
		{
			// trace segment
			if( p->TestPointInside(xi,yi) || p->TestPointInside(xf,yf) )
				bIntersection = TRUE;
			for( int is=0; is<p->GetNumCorners(); is++ )
			{
				int d = GetClearanceBetweenSegments( xi, yi, xf, yf, CPolyLine::STRAIGHT, w,
						p->GetX(is), 
						p->GetY(is), 
						p->GetX(p->GetIndexCornerNext(is)), 
						p->GetY(p->GetIndexCornerNext(is)),
						p->GetSideStyle(is), a_w, 
						fill_clearance, NULL, NULL );
				if( d < fill_clearance )
				{
					bIntersection = TRUE;
					i_Intersect[nc] = is;
					nc++;
				}
			}
		}
		else if( shape == PAD_ROUND || shape == PAD_OCTAGON  )
		{
			if( p->TestPointInside(xi,yi) )
				bIntersection = TRUE;
			for( int is=0; is<p->GetNumCorners(); is++ )
			{
				int d = GetClearanceBetweenSegments( xi, yi, xi+10, yi+10, CPolyLine::STRAIGHT, wid,
						p->GetX(is), 
						p->GetY(is), 
						p->GetX(p->GetIndexCornerNext(is)), 
						p->GetY(p->GetIndexCornerNext(is)), 
						p->GetSideStyle(is), a_w, 
						fill_clearance, NULL, NULL );
				if( d < fill_clearance )
				{
					bIntersection = TRUE;
					i_Intersect[nc] = is;
					nc++;
				}
			}
		}
		else
		{
			if( p->TestPointInside(xi,yi) )
				bIntersection = TRUE;
			int min_d;
			if( shape == PAD_SQUARE )
				min_d = wid*sqrt((double)2.0);
			else
				min_d = sqrt((double)wid*wid +(double)len*len);
			for( int is=0; is<p->GetNumCorners(); is++ )
			{
				int d = GetClearanceBetweenSegments( xi, yi, xi+10, yi+10, CPolyLine::STRAIGHT, min_d,
						p->GetX(is), 
						p->GetY(is), 
						p->GetX(p->GetIndexCornerNext(is)), 
						p->GetY(p->GetIndexCornerNext(is)), 
						p->GetSideStyle(is), a_w, 
						fill_clearance, NULL, NULL );
				if( d < fill_clearance )
				{
					bIntersection = TRUE;
					i_Intersect[nc] = is;
					nc++;
				}
			}
		}
		if( bIntersection )
		{
			if( !bClearanceMade ) 
			{
				// construct a gpc_poly for the clearance
				if( shape == -1 )
				{
					int npoints = 18;	// number of points in poly
					gpc_seg_contour.num_vertices = npoints;
					gpc_seg_contour.vertex = (gpc_vertex*)calloc( npoints, 2*sizeof(double) );
					if( !gpc_seg_contour.vertex )
						ASSERT(0);
					double x,y;
					// create points around beginning of segment
					seg_angle = atan2( (double)yf - yi, (double)xf - xi );
					cw = (double)fill_clearance + (double)w/2.0;
					double angle = seg_angle + pi/2.0;		// rotate 90 degrees ccw
					double angle_step = pi/(npoints/2-1);
					for( int i=0; i<npoints/2; i++ )
					{
						x = xi + cw*cos(angle);
						y = yi + cw*sin(angle);
						gpc_seg_contour.vertex[i].x = x;
						gpc_seg_contour.vertex[i].y = y;
						angle += angle_step;
					}
					// create points around end of segment
					angle = seg_angle - pi/2.0;
					for( int i=npoints/2; i<npoints; i++ )
					{
						x = xf + cw*cos(angle);
						y = yf + cw*sin(angle);
						gpc_seg_contour.vertex[i].x = x;
						gpc_seg_contour.vertex[i].y = y;
						angle += angle_step;
					}
					gpc_seg_poly.num_contours = 1;
					gpc_seg_poly.hole = new int;//???????
					gpc_seg_poly.hole[0] = 0;
					gpc_seg_poly.contour = &gpc_seg_contour;
					bClearanceMade = TRUE;
				}
				else if( shape == PAD_ROUND || shape == PAD_OCTAGON )
				{
					// round clearance
					int npoints = 32;	// number of points in poly
					gpc_seg_contour.num_vertices = npoints;
					gpc_seg_contour.vertex = (gpc_vertex*)calloc( npoints, 2*sizeof(double) );
					if( !gpc_seg_contour.vertex )
						ASSERT(0);
					double x,y;
					// create points around pad
					double angle = 0.0;
					double angle_step = pi/(npoints/2);
					cw = wid/2 + fill_clearance;
					for( int i=0; i<npoints; i++ )
					{
						x = xi + cw*cos(angle);
						y = yi + cw*sin(angle);
						gpc_seg_contour.vertex[i].x = x;
						gpc_seg_contour.vertex[i].y = y;
						angle += angle_step;
					}
					gpc_seg_poly.num_contours = 1;
					gpc_seg_poly.hole = new int;//???????
					gpc_seg_poly.hole[0] = 0;
					gpc_seg_poly.contour = &gpc_seg_contour;
					bClearanceMade = TRUE;
				}
				else
				{
					if( shape == PAD_SQUARE )
						len = wid;
					// create points around pad
					cw = fill_clearance + radius;
					// number of points in poly
					int npoints = 20;	 
					CPoint rnd[20];
					int nv = Gen_RndRectPoly(xi,yi,wid+2*fill_clearance,len+2*fill_clearance,cw,-angle,rnd,npoints);
					gpc_seg_contour.num_vertices = nv;
					gpc_seg_contour.vertex = (gpc_vertex*)calloc( nv, 2*sizeof(double) );
					if( !gpc_seg_contour.vertex ) 
						ASSERT(0);
					for(int i=0; i<nv; i++)
					{
						gpc_seg_contour.vertex[i].x = rnd[i].x;
						gpc_seg_contour.vertex[i].y = rnd[i].y;
					}
					
					gpc_seg_poly.num_contours = 1;
					gpc_seg_poly.hole = new int;//???????
					gpc_seg_poly.hole[0] = 0;
					gpc_seg_poly.contour = &gpc_seg_contour;
					bClearanceMade = TRUE;
				}
			}
			// intersect area and clearance polys
			gpc_polygon gpc_intersection;
			gpc_intersection.num_contours = 0;
			gpc_intersection.hole = NULL;
			gpc_intersection.contour = NULL;
			// make area GpcPoly if not already made, including cutouts
			if( area->poly->GetGpcPoly()->num_contours == 0 )
				area->poly->MakeGpcPoly( -1 );
			gpc_polygon_clip( GPC_INT, &gpc_seg_poly, area->poly->GetGpcPoly(), &gpc_intersection );

			// if area inside area
			for( int i_area=0; i_area<net->nareas; i_area++ )
			{
				if( net->area[i_area].poly->GetLayer() != layer )
					continue;
				RECT R1 = area->poly->GetBounds();
				RECT R2 = net->area[i_area].poly->GetBounds();
				long long S1 = long long(R1.right-R1.left)*long long(R1.top-R1.bottom);
				long long S2 = long long(R2.right-R2.left)*long long(R2.top-R2.bottom);
				if( abs(S1) > abs(S2) && RectsIntersection(R1,R2) >= 0 )
				{ 
					net->area[i_area].poly->MakeGpcPoly();
					gpc_polygon sbj = gpc_intersection;
					gpc_polygon_clip( GPC_DIFF, &sbj, net->area[i_area].poly->GetGpcPoly(), &gpc_intersection );
					if( sbj.num_contours )
						gpc_free_polygon( &sbj );
				}
				if( log && abs(S2) > abs(S1) )
				{
					if( net->area[i_area].poly->TestPointInside( xi, yi ) && area->poly->TestPointInside( xi, yi ) )
					{
						CString mess;// xs, ys;
						//::MakeCStringFromDimension( &xs, xi, m_units, FALSE, FALSE, FALSE );
						mess.Format( "    isWarning: no connect? x=%dnm, y=%dnm\r\n", xi , yi );
						log->AddLine( mess );
					}
				}
			}
			int ncontours = gpc_intersection.num_contours;
			for( int ic=0; ic<ncontours; ic++ )
			{
				// draw clearance
				gpc_vertex * gpv = gpc_intersection.contour[ic].vertex;
				int nv = gpc_intersection.contour[ic].num_vertices;
				if( nv )
				{
					f->WriteString( "G36*\n" );
					WriteMoveTo( f, gpv[0].x, gpv[0].y, LIGHT_OFF );
					for( int iv=1; iv<nv; iv++ )
						WriteMoveTo( f, gpv[iv].x, gpv[iv].y, LIGHT_ON );
					WriteMoveTo( f, gpv[0].x, gpv[0].y, LIGHT_ON );
					f->WriteString( "G37*\n" );
				}
			}
			// free 
			if( gpc_intersection.num_contours )
				gpc_free_polygon( &gpc_intersection );
			//
			double side_angle, ccw;
			if( (*area_net_list)[ia] != net )
			{
				for( int iisec=0; iisec<nc; iisec++ )
				{
					int s_xi = p->GetX(i_Intersect[iisec]);
					int s_yi = p->GetY(i_Intersect[iisec]);
					int s_xf = p->GetX(p->GetIndexCornerNext(i_Intersect[iisec]));
					int s_yf = p->GetY(p->GetIndexCornerNext(i_Intersect[iisec]));
					// side
					side_angle = atan2( (double)s_yf - s_yi, (double)s_xf - s_xi );
					ccw =  (double)a_w/2.0 + (double)CLEARANCE_POLY_STROKE_MILS*NM_PER_MIL;
					int npoints = 18;	// number of points in poly
					gpc_side_contour.num_vertices = npoints;
					gpc_side_contour.vertex = (gpc_vertex*)calloc( npoints, 2*sizeof(double) );
					if( !gpc_seg_contour.vertex )
						ASSERT(0);
					double x,y;
					// create points around beginning of segment
					double angle = side_angle + pi/2.0;		// rotate 90 degrees ccw
					double angle_step = pi/(npoints/2-1);
					for( int i=0; i<npoints/2; i++ )
					{
						x = s_xi + ccw*cos(angle);
						y = s_yi + ccw*sin(angle);
						gpc_side_contour.vertex[i].x = x;
						gpc_side_contour.vertex[i].y = y;
						angle += angle_step;
					}
					// create points around end of segment
					angle = side_angle - pi/2.0;
					for( int i=npoints/2; i<npoints; i++ )
					{
						x = s_xf + ccw*cos(angle);
						y = s_yf + ccw*sin(angle);
						gpc_side_contour.vertex[i].x = x;
						gpc_side_contour.vertex[i].y = y;
						angle += angle_step;
					}
					gpc_side_poly.num_contours = 1;
					gpc_side_poly.hole = new int;//???????
					gpc_side_poly.hole[0] = 0;
					gpc_side_poly.contour = &gpc_side_contour;
					gpc_intersection.num_contours = 0;
					gpc_intersection.hole = NULL;
					gpc_intersection.contour = NULL;
					gpc_polygon_clip( GPC_INT, &gpc_seg_poly, &gpc_side_poly, &gpc_intersection );
					// draw clearance
					int n_c = gpc_intersection.num_contours;
					if (n_c)
					{
						gpc_vertex * gpv = gpc_intersection.contour[0].vertex;
						int nv = gpc_intersection.contour[0].num_vertices;
						if( nv )
						{
							f->WriteString( "G36*\n" );
							WriteMoveTo( f, gpv[0].x, gpv[0].y, LIGHT_OFF );
							for( int iv=1; iv<nv; iv++ )
								WriteMoveTo( f, gpv[iv].x, gpv[iv].y, LIGHT_ON );
							WriteMoveTo( f, gpv[0].x, gpv[0].y, LIGHT_ON );
							f->WriteString( "G37*\n" );
						}
					}
					// free 
					if( gpc_intersection.num_contours )
						gpc_free_polygon( &gpc_intersection );
					delete gpc_side_poly.hole;
					delete gpc_side_contour.vertex;		
				}
			}
		}
		delete[] i_Intersect;
		
	}
	if( bClearanceMade )
	{
		delete gpc_seg_contour.vertex ;
		delete gpc_seg_poly.hole;
	}
}



// write the Gerber file for a layer
// assumes that the file is already open for writing
// returns 0 if successful
//
int WriteGerberFile( CStdioFile * f, int flags, int layer,
					CDlgLog * log, int paste_mask_shrink,
					int fill_clearance, int mask_clearance, int pilot_diameter,
					int min_silkscreen_stroke_wid, int highlight_width, int thermal_wid,
					int hole_clearance, int thermal_clearance,
					int n_x, int n_y, int step_x, int step_y,
					CArray<CPolyLine> * op, CPartList * pl, 
					CNetList * nl, CTextList * tl, CDisplayList * dl, Merge * ml )
{
THERMAL_W = thermal_wid;
int LPD = 0;
#define SET_LPD	if(LPD != 1) {f->WriteString( "%LPD*%\n" ); LPD = 1;} else {;}
#define SET_LPC	if(LPD != 2) {f->WriteString( "%LPC*%\n" ); LPD = 2;} else {;}
#define LAYER_TEXT_HEIGHT			100*NM_PER_MIL	// for layer ID sring
#define	LAYER_TEXT_STROKE_WIDTH		10*NM_PER_MIL
#define PASS0 (ipass==0)	
#define PASS1 (ipass==1)

	BOOL bUsePinThermals = !(flags & GERBER_NO_PIN_THERMALS);

	aperture_array ap_array;
	int current_iap = -1;
	CAperture current_ap;
	RECT op_rect;
	op_rect.left = op_rect.bottom = INT_MAX;
	op_rect.right = op_rect.top = INT_MIN;
	const double cos_oct = cos( pi/8.0 ); 
	CString str;

	// get boundaries of board outline (in nm)
	if( int s = op->GetSize() )
		for( int ib=0; ib<s; ib++ ) 
		{
			id gid = (*op)[ib].GetId();
			if( gid.st != ID_BOARD )
				continue;
			RECT gr = (*op)[ib].GetBounds();
			SwellRect( &op_rect, gr );
		}
	if( op_rect.left == INT_MAX )
	{
		op_rect = rect( 0,0,0,0 );
	}

	// perform two passes through data, first just get apertures, then write file
	for( int ipass=0; ipass<2; ipass++ )
	{
		CString line;
		if( PASS1 )
		{	//=============================================================================
			// ******************** apertures created, now write them *********************
			//=============================================================================
			CString thermal_angle_str = "45.0";
			if( flags & GERBER_90_THERMALS )
				thermal_angle_str = "0.0";
			CString layer_name_str;
			if( layer == LAY_PASTE_TOP )
				layer_name_str = "top paste mask";
			else if( layer == LAY_PASTE_BOTTOM )
				layer_name_str = "bottom paste mask";
			else
				layer_name_str.Format( "%s", &layer_str[layer][0] );
			line.Format( "G04 %s layer *\n", layer_name_str );
			f->WriteString( line );
			f->WriteString( "G04 Scale: 100 percent, Rotated: No, Reflected: No *\n" );
			f->WriteString( "%FSLAX24Y24*%\n" );	// 2.4
			f->WriteString( "%MOIN*%\n" );
			f->WriteString( "%LN " + layer_name_str + " *%\n" );
			// macros
			f->WriteString( "G04 Rounded Rectangle Macro, params: W/2, H/2, R *\n" );
			f->WriteString( "%AMRNDREC*\n" );
			f->WriteString( "21,1,$1+$1,$2+$2-$3-$3,0,0,0*\n" );
			f->WriteString( "21,1,$1+$1-$3-$3,$2+$2,0,0,0*\n" );
			f->WriteString( "1,1,$3+$3,$1-$3,$2-$3*\n" );
			f->WriteString( "1,1,$3+$3,$3-$1,$2-$3*\n" );
			f->WriteString( "1,1,$3+$3,$1-$3,$3-$2*\n" );
			f->WriteString( "1,1,$3+$3,$3-$1,$3-$2*%\n" );
			f->WriteString( "G04 Rectangular Thermal Macro, params: W/4, H/4, TW/4, TC/4 *\n" );
			f->WriteString( "%AMRECTHERM*\n" );  
			f->WriteString( "21,1,$1+$1-$3-$3-$4-$4,$2+$2-$3-$3-$4-$4-$4-$4,0-$1-$3-$4,0-$2-$3,0*\n" ); 
			f->WriteString( "21,1,$1+$1-$3-$3-$4-$4-$4-$4,$2+$2-$3-$3-$4-$4,0-$1-$3,0-$2-$3-$4,0*\n" );
			f->WriteString( "1,1,$4+$4+$4+$4,0-$1-$1+$4+$4,0-$2-$2+$4+$4*\n" );
			f->WriteString( "1,1,$4+$4+$4+$4,0-$1-$1+$4+$4,0-$3-$3-$4-$4*\n" );
			f->WriteString( "1,1,$4+$4+$4+$4,0-$3-$3-$4-$4,0-$2-$2+$4+$4*\n" );
			//
			f->WriteString( "21,1,$1+$1-$3-$3-$4-$4,$2+$2-$3-$3-$4-$4-$4-$4,0-$1-$3-$4,$2+$3,0*\n" ); 
			f->WriteString( "21,1,$1+$1-$3-$3-$4-$4-$4-$4,$2+$2-$3-$3-$4-$4,0-$1-$3,$2+$3+$4,0*\n" );
			f->WriteString( "1,1,$4+$4+$4+$4,0-$1-$1+$4+$4,$2+$2-$4-$4*\n" );
			f->WriteString( "1,1,$4+$4+$4+$4,0-$1-$1+$4+$4,$3+$3+$4+$4*\n" );
			f->WriteString( "1,1,$4+$4+$4+$4,0-$3-$3-$4-$4,$2+$2-$4-$4*\n" );
			//
			f->WriteString( "21,1,$1+$1-$3-$3-$4-$4,$2+$2-$3-$3-$4-$4-$4-$4,$1+$3+$4,0-$2-$3,0*\n" ); 
			f->WriteString( "21,1,$1+$1-$3-$3-$4-$4-$4-$4,$2+$2-$3-$3-$4-$4,$1+$3,0-$2-$3-$4,0*\n" );
			f->WriteString( "1,1,$4+$4+$4+$4,$1+$1-$4-$4,0-$2-$2+$4+$4*\n" );
			f->WriteString( "1,1,$4+$4+$4+$4,$1+$1-$4-$4,0-$3-$3-$4-$4*\n" );
			f->WriteString( "1,1,$4+$4+$4+$4,$3+$3+$4+$4,0-$2-$2+$4+$4*\n" );
			//
			f->WriteString( "21,1,$1+$1-$3-$3-$4-$4,$2+$2-$3-$3-$4-$4-$4-$4,$1+$3+$4,$2+$3,0*\n" ); 
			f->WriteString( "21,1,$1+$1-$3-$3-$4-$4-$4-$4,$2+$2-$3-$3-$4-$4,$1+$3,$2+$3+$4,0*\n" );
			f->WriteString( "1,1,$4+$4+$4+$4,$1+$1-$4-$4,$2+$2-$4-$4*\n" );
			f->WriteString( "1,1,$4+$4+$4+$4,$1+$1-$4-$4,$3+$3+$4+$4*\n" );
			f->WriteString( "1,1,$4+$4+$4+$4,$3+$3+$4+$4,$2+$2-$4-$4*%\n" );
			//
			f->WriteString( "%AMRECTHERM_45*\n" );  
			/*f->WriteString( "21,1,$4+$4+$4+$4,$2+$2+$2+$2-$4-$4-$4-$4-$4-$4-$4-$4-$4-$4-$3-$3,$1+$1-$4-$4,0,0*\n" ); 
			f->WriteString( "21,1,$4+$4+$4+$4,$2+$2+$2+$2-$4-$4-$4-$4-$4-$4-$4-$4-$4-$4-$3-$3,0-$1-$1+$4+$4,0,0*\n" );
			f->WriteString( "21,1,$1+$1+$1+$1-$4-$4-$4-$4-$4-$4-$4-$4-$4-$4-$3-$3,$4+$4+$4+$4,0,$2+$2-$4-$4,0*\n" ); 
			f->WriteString( "21,1,$1+$1+$1+$1-$4-$4-$4-$4-$4-$4-$4-$4-$4-$4-$3-$3,$4+$4+$4+$4,0,0-$2-$2+$4+$4,0*\n" );*/
			f->WriteString( "21,1,$1+$1+$1+$1,$2+$2+$2+$2-$4-$4-$4-$4-$4-$4-$4-$4-$4-$4-$3-$3,0,0,0*\n" );
			f->WriteString( "21,1,$1+$1+$1+$1-$4-$4-$4-$4-$4-$4-$4-$4-$4-$4-$3-$3,$2+$2+$2+$2,0,0,0*\n" );
			f->WriteString( "1,1,$4+$4+$4+$4,$1+$1-$4-$4,$2+$2-$4-$4-$4-$4-$4-$3*\n" );
			f->WriteString( "1,1,$4+$4+$4+$4,$1+$1-$4-$4,0-$2-$2+$4+$4+$4+$4+$4+$3*\n" );
			f->WriteString( "1,1,$4+$4+$4+$4,$1+$1-$4-$4-$4-$4-$4-$3,$2+$2-$4-$4*\n" );
			f->WriteString( "1,1,$4+$4+$4+$4,0-$1-$1+$4+$4+$4+$4+$4+$3,$2+$2-$4-$4*\n" );
			f->WriteString( "1,1,$4+$4+$4+$4,0-$1-$1+$4+$4,$2+$2-$4-$4-$4-$4-$4-$3*\n" );
			f->WriteString( "1,1,$4+$4+$4+$4,0-$1-$1+$4+$4,0-$2-$2+$4+$4+$4+$4+$4+$3*\n" );
			f->WriteString( "1,1,$4+$4+$4+$4,$1+$1-$4-$4-$4-$4-$4-$3,0-$2-$2+$4+$4*\n" );
			f->WriteString( "1,1,$4+$4+$4+$4,0-$1-$1+$4+$4+$4+$4+$4+$3,0-$2-$2+$4+$4*%\n" );
			//
			// define all of the apertures
			for( int ia=0; ia<ap_array.GetSize(); ia++ )
			{
				if( ap_array[ia].m_type == CAperture::AP_CIRCLE )
				{
					line.Format( "ADD%dC,%.6f*", ia+10, (double)ap_array[ia].m_size1/25400000.0 );
					f->WriteString( "%" + line + "%\n" );
				}
				else if( ap_array[ia].m_type == CAperture::AP_SQUARE )
				{
					line.Format( "ADD%dR,%.6fX%.6f*", ia+10, 
						(double)ap_array[ia].m_size1/25400000.0, (double)ap_array[ia].m_size1/25400000.0 );
					f->WriteString( "%" + line + "%\n" );
				}
				else if( ap_array[ia].m_type == CAperture::AP_THERMAL )
				{
					line.Format( "AMTHERM%d*7,0,0,%.6f,%.6f,%.6f,%s*", ia+10,
						(double)ap_array[ia].m_size1/25400000.0,	// outer diam
						(double)ap_array[ia].m_size2/25400000.0,	// inner diam
						(double)thermal_wid/25400000.0,				// cross-hair width
						thermal_angle_str );						// thermal spoke angle
					f->WriteString( "%" + line + "%\n" );
					line.Format( "ADD%dTHERM%d*", ia+10, ia+10 );
					f->WriteString( "%" + line + "%\n" );
				}
				else if( ap_array[ia].m_type == CAperture::AP_RECT_THERMAL )
				{
					line.Format( "ADD%dRECTHERM,%.6fX%.6fX%.6fX%.6f*", ia+10, 
						(double)ap_array[ia].m_size1/(4*25400000.0), (double)ap_array[ia].m_size2/(4*25400000.0),
						(double)ap_array[ia].m_size3/(4*25400000.0), (double)ap_array[ia].m_size4/(4*25400000.0) );
					f->WriteString( "%" + line + "%\n" ); 
				}
				else if( ap_array[ia].m_type == CAperture::AP_RECT_THERMAL_45 )
				{
					line.Format( "ADD%dRECTHERM_45,%.6fX%.6fX%.6fX%.6f*", ia+10, 
						(double)ap_array[ia].m_size1/(4*25400000.0), (double)ap_array[ia].m_size2/(4*25400000.0),
						(double)ap_array[ia].m_size3/(1.41*25400000.0), (double)ap_array[ia].m_size4/(4*25400000.0) );
					f->WriteString( "%" + line + "%\n" ); 
				}
				else if( ap_array[ia].m_type == CAperture::AP_MOIRE )
				{
					line.Format( "AMMOIRE%d*6,0,0,%.6f,0.005,0.050,3,0.005,%.6f,0.0*", ia+10,
						(double)ap_array[ia].m_size1/25400000.0,
						(double)ap_array[ia].m_size2/25400000.0 );
					f->WriteString( "%" + line + "%\n" );
					line.Format( "ADD%dMOIRE%d*", ia+10, ia+10 );
					f->WriteString( "%" + line + "%\n" );
				}
				else if( ap_array[ia].m_type == CAperture::AP_OCTAGON )
				{
					line.Format( "ADD%dP,%.6fX8X22.5*", ia+10, 
						((double)ap_array[ia].m_size1/25400000.0 )/cos_oct );
					f->WriteString( "%" + line + "%\n" );
				}
				else if( ap_array[ia].m_type == CAperture::AP_OVAL ) 
				{
					line.Format( "ADD%dO,%.6fX%.6f*", ia+10, 
						(double)ap_array[ia].m_size1/25400000.0, (double)ap_array[ia].m_size2/25400000.0 );
					f->WriteString( "%" + line + "%\n" );
				}
				else if( ap_array[ia].m_type == CAperture::AP_RECT ) 
				{
					line.Format( "ADD%dR,%.6fX%.6f*", ia+10, 
						(double)ap_array[ia].m_size1/25400000.0, (double)ap_array[ia].m_size2/25400000.0 );
					f->WriteString( "%" + line + "%\n" );
				}
				else if( ap_array[ia].m_type == CAperture::AP_RRECT ) 
				{
					line.Format( "ADD%dRNDREC,%.6fX%.6fX%.6f*", ia+10, 
						(double)ap_array[ia].m_size1/(2*25400000.0), (double)ap_array[ia].m_size2/(2*25400000.0),
						(double)ap_array[ia].m_size3/25400000.0 );
					f->WriteString( "%" + line + "%\n" ); 
				}
				else
					ASSERT(0);
			}
			f->WriteString( "G90*\n" );			// absolute format
			f->WriteString( "G70D02*\n" );		// use inches
			f->WriteString( "G04 begin color: clBlack*\n" );
			SET_LPD								// start color clBlack
		}

		// draw moires
		double f_step_x = (op_rect.right - op_rect.left + (double)step_x)/NM_PER_MIL;	// mils
		double f_step_y = (op_rect.top - op_rect.bottom + (double)step_y)/NM_PER_MIL;	// mils
		if( op && (flags & GERBER_AUTO_MOIRES) )
		{
			if( PASS1 )
			{	
				f->WriteString( "\nG04 ----------------------- Draw moires (positive)*\n" );
				SET_LPD
				current_ap.m_type = CAperture::AP_NONE;	// force selection of aperture
			}
			CAperture moire_ap( CAperture::AP_MOIRE, 400*NM_PER_MIL, 350*NM_PER_MIL );
			ChangeAperture( &moire_ap, &current_ap, &ap_array, PASS0, f );
			if( PASS1 )
			{
				// flash 3 moires
				int x = op_rect.left - 500*NM_PER_MIL;
				int y = op_rect.bottom;
				::WriteMoveTo( f, x, y, LIGHT_FLASH );	// lower left
				x = op_rect.right + ((n_x-1)*f_step_x + 500)*NM_PER_MIL;
				::WriteMoveTo( f, x, y, LIGHT_FLASH );	// lower right
				x = op_rect.left - 500*NM_PER_MIL;
				y = op_rect.top + ((n_y-1)*f_step_y*NM_PER_MIL);
				::WriteMoveTo( f, x, y, LIGHT_FLASH );	// upper left
			}
		}

		//draw layer identification string if requested
		if( tl && (flags & GERBER_LAYER_TEXT) )
		{
			if( PASS1 )
			{
				f->WriteString( "\nG04 Draw Layer Name*\n" );
			}			
			CString str = "";
			switch( layer )
			{
			case LAY_BOARD_OUTLINE: str = "Board Outline"; break;
			case LAY_SM_TOP: str = "Top Solder Mask"; break;
			case LAY_SM_BOTTOM: str = "Bottom Solder Mask"; break;
			case LAY_PASTE_TOP: str = "Top Solder Paste Mask"; break;
			case LAY_PASTE_BOTTOM: str = "Bottom Solder Paste Mask"; break;
			case LAY_SILK_TOP: str = "Top Silkscreen"; break;
			case LAY_SILK_BOTTOM: str = "Bottom Silkscreen"; break;
			case LAY_TOP_COPPER: str = "Top Copper Layer"; break;
			case LAY_BOTTOM_COPPER: str = "Bottom Copper Layer"; break;
			case LAY_HILITE: str = "Highlight Layer"; break;
			case LAY_REFINE_TOP: str = "Top notes Layer"; break;
			case LAY_REFINE_BOT: str = "Bottom notes Layer"; break;
			case LAY_SCRIBING: str = "Scribing Layer"; break;
			}
			if( layer > LAY_BOTTOM_COPPER )
				str.Format( "Inner %d Copper Layer", layer - LAY_BOTTOM_COPPER );
			CText * t = tl->AddText( op_rect.left, op_rect.bottom-LAYER_TEXT_HEIGHT*2, 0, 0, 0, 
				LAY_SILK_TOP, LAYER_TEXT_HEIGHT, LAYER_TEXT_STROKE_WIDTH, &str );
			// draw text
			int w = t->m_stroke_width;
			CAperture text_ap( CAperture::AP_CIRCLE, w, 0 );
			ChangeAperture( &text_ap, &current_ap, &ap_array, PASS0, f );
			if( PASS1 )
			{
				if( t->dl_el )
				{
					CArray<CPoint> * tpt = dl->Get_Points( t->dl_el, NULL, 0 );
					int np = tpt->GetSize();
					CPoint * m_stroke = new CPoint[np];//ok
					dl->Get_Points( t->dl_el, m_stroke, &np );
					for( int istroke=0; istroke+1<np; istroke+=2 )
					{
						::WriteMoveTo( f, m_stroke[istroke].x, m_stroke[istroke].y, LIGHT_OFF );
						::WriteMoveTo( f, m_stroke[istroke+1].x, m_stroke[istroke+1].y, LIGHT_ON );
					}
					delete m_stroke;
				}
			}
			tl->RemoveText( t );
		}

		// step and repeat for panelization
		if( PASS1 )
		{
			f->WriteString( "\nG04 Step and Repeat for panelization *\n" );
			if( n_x > 1 || n_y > 1 )
			{
				CString str;
				str.Format( "SRX%dY%dI%fJ%f*", n_x, n_y, f_step_x/1000.0, f_step_y/1000.0 );
				f->WriteString( "%" + str + "%\n" );
			}
		}
		// draw outline poly
		if( PASS1 )
		{
			f->WriteString( "\nG04 ----------------------- Draw board outline (positive)*\n" );
			SET_LPD
			current_ap.m_type = CAperture::AP_NONE;	// force selection of aperture
		}
		// if hilite layer
		// scan all elements and continues
		if  ( layer == LAY_HILITE )
		{
			dl_element * el = dl->Get_Start();
			while( el->next->next )
			{
				el = el->next;
				if ( !el->visible )
					continue;
				BOOL fl = FALSE;
				int n_l = nl->GetNumCopperLayers();
				for( int sc=LAY_BOARD_OUTLINE; sc<=LAY_TOP_COPPER+n_l; sc++ )
					if( getbit(el->map_orig_layer,sc) && dl->m_vis[sc] )
					{
						fl = TRUE;
						break;
					}
				if( !fl )
					continue;
				if( getbit(el->layers_bitmap,LAY_HILITE) == 0 )
					continue;
				RECT Get;
				dl->Get_Rect( el, &Get );
				int xi = (Get.right + Get.left)/2;
				int yi = (Get.top + Get.bottom)/2;
				int w = max(highlight_width,abs(el->el_w)*m_pcbu_per_wu);
				if( el->gtype == DL_CIRC || el->gtype == DL_HOLLOW_CIRC )
				{
					if( el->gtype == DL_CIRC )
						w = (Get.right - Get.left);
					CAperture ap( CAperture::AP_CIRCLE, w, 0 );
					ChangeAperture( &ap, &current_ap, &ap_array, PASS0, f );
					if PASS1
						::WriteMoveTo( f, xi, yi, LIGHT_FLASH );
					if( el->gtype == DL_HOLLOW_CIRC && w > highlight_width*2 )
					{
						CAperture ap( CAperture::AP_CIRCLE, w-(highlight_width*2), 0 );
						ChangeAperture( &ap, &current_ap, &ap_array, PASS0, f );
						if PASS1
						{
							SET_LPC
							::WriteMoveTo( f, xi, yi, LIGHT_FLASH );
							SET_LPD
						}
					}
				}
				else if( el->gtype == DL_RECT )
				{
					CAperture ap( CAperture::AP_RECT, Get.right-Get.left, Get.top-Get.bottom );
					ChangeAperture( &ap, &current_ap, &ap_array, PASS0, f );
					if PASS1
						::WriteMoveTo( f, xi, yi, LIGHT_FLASH );
				}
				else if( el->gtype == DL_RRECT )
				{
					CAperture ap( CAperture::AP_RRECT, Get.right-Get.left, Get.top-Get.bottom );
					ChangeAperture( &ap, &current_ap, &ap_array, PASS0, f );
					if PASS1
						::WriteMoveTo( f, xi, yi, LIGHT_FLASH );
				}
				else if( el->gtype == DL_HOLLOW_RECT  )
				{
					CAperture ap( CAperture::AP_CIRCLE, w, 0 );
					ChangeAperture( &ap, &current_ap, &ap_array, PASS0, f );
					if PASS1
					{
						::WriteMoveTo( f, Get.left,  Get.bottom, LIGHT_OFF );
						::WriteMoveTo( f, Get.left,  Get.top, LIGHT_ON );
						::WriteMoveTo( f, Get.right, Get.top, LIGHT_ON );
						::WriteMoveTo( f, Get.right, Get.bottom, LIGHT_ON );
						::WriteMoveTo( f, Get.left,  Get.bottom, LIGHT_ON );
					}
				}
				if( el->gtype == DL_RECT_X )
				{
					CAperture ap( CAperture::AP_CIRCLE, w, 0 );
					ChangeAperture( &ap, &current_ap, &ap_array, PASS0, f );
					if PASS1
					{
						CPoint P[4];
						int np = 4;
						dl->Get_Points( el, P, &np ); 
						if( np == 4 )
						{
							::WriteMoveTo( f, P[0].x, P[0].y, LIGHT_OFF );
							for( int ii=1; ii<np; ii++ )
								::WriteMoveTo( f, P[ii].x, P[ii].y, LIGHT_ON );
							::WriteMoveTo( f, P[0].x, P[0].y, LIGHT_ON );
							::WriteMoveTo( f, P[2].x, P[2].y, LIGHT_ON );
							::WriteMoveTo( f, P[1].x, P[1].y, LIGHT_OFF );
							::WriteMoveTo( f, P[3].x, P[3].y, LIGHT_ON );
						}
					}
				}
				if( el->gtype == DL_LINES_ARRAY )
				{
					CAperture ap( CAperture::AP_CIRCLE, w, 0 );
					ChangeAperture( &ap, &current_ap, &ap_array, PASS0, f );
					if PASS1
					{
						CArray<CPoint> * PPTS;
						PPTS = dl->Get_Points( el, NULL, 0 ); 
						int np = PPTS->GetSize();
						CPoint * PTS = new CPoint[np];//ok
						dl->Get_Points( el, PTS, &np ); 
						if( np )
						{
							for( int ii=0; ii<np-1; ii+=2 )
							{
								::WriteMoveTo( f, PTS[ii].x, PTS[ii].y, LIGHT_OFF );
								::WriteMoveTo( f, PTS[ii+1].x, PTS[ii+1].y, LIGHT_ON );
							}
						}
						delete PTS;
					}
				}
				if( el->gtype == DL_POLYGON || el->gtype == DL_POLYLINE )
				{
					CAperture ap( CAperture::AP_CIRCLE, w, 0 );
					ChangeAperture( &ap, &current_ap, &ap_array, PASS0, f );
					if PASS1
					{
						CArray<CPoint> * PPTS;
						PPTS = dl->Get_Points( el, NULL, 0 ); 
						int np = PPTS->GetSize();
						CPoint * PTS = new CPoint[np];//ok
						dl->Get_Points( el, PTS, &np ); 
						if( np )
						{
							::WriteMoveTo( f, PTS[0].x, PTS[0].y, LIGHT_OFF );
							for( int ii=1; ii<np; ii++ )
								::WriteMoveTo( f, PTS[ii].x, PTS[ii].y, LIGHT_ON );
							::WriteMoveTo( f, PTS[0].x, PTS[0].y, LIGHT_ON );
							if( el->gtype == DL_POLYGON )
							{
								f->WriteString( "G36*\n" );
								::WriteMoveTo( f, PTS[0].x, PTS[0].y, LIGHT_OFF );
								for( int ii=1; ii<np; ii++ )
									::WriteMoveTo( f, PTS[ii].x, PTS[ii].y, LIGHT_ON );
								::WriteMoveTo( f, PTS[0].x, PTS[0].y, LIGHT_ON );
								f->WriteString( "G37*\n" );
							}
						}
						delete PTS;
					}
				}
				else if( el->gtype == DL_LINE )
				{
					CAperture ap( CAperture::AP_CIRCLE, w, 0 );
					ChangeAperture( &ap, &current_ap, &ap_array, PASS0, f );
					if PASS1
					{
						CPoint P[2];
						int np = 2;
						dl->Get_Points( el, P, &np ); 
						if( np == 2 )
						{
							::WriteMoveTo( f, P[0].x, P[0].y, LIGHT_OFF );
							::WriteMoveTo( f, P[1].x, P[1].y, LIGHT_ON );
						}
					}
				}
				else if( el->gtype == DL_HOLLOW_LINE )
				{
					CAperture ap( CAperture::AP_CIRCLE, w, 0 );
					ChangeAperture( &ap, &current_ap, &ap_array, PASS0, f );
					if PASS1
					{
						CArray<CPoint> * PTS;
						PTS = dl->Get_Points( el, NULL, 0 ); 
						int np = PTS->GetSize();
						if( np > 4 )
						{
							::WriteMoveTo( f, (*PTS)[2].x, (*PTS)[2].y, LIGHT_OFF );
							for( int ii=3; ii<np; ii++ )
								::WriteMoveTo( f, (*PTS)[ii].x, (*PTS)[ii].y, LIGHT_ON );
							::WriteMoveTo( f, (*PTS)[2].x, (*PTS)[2].y, LIGHT_ON );
						}
					}
				}
				else if( el->gtype == DL_ARC_CW || el->gtype == DL_ARC_CCW )
				{
					int tp;
					if (el->gtype == DL_ARC_CW)
						tp = CPolyLine::ARC_CW;
					else
						tp = CPolyLine::ARC_CCW;
					CAperture ap( CAperture::AP_CIRCLE, w, 0 );
					ChangeAperture( &ap, &current_ap, &ap_array, PASS0, f );
					if PASS1
					{
						CArray<CPoint> * PTS;
						PTS = dl->Get_Points( el, NULL, 0 ); 
						int np = PTS->GetSize();
						if( np >= 2 )
						{
							::WriteMoveTo( f, (*PTS)[0].x, (*PTS)[0].y, LIGHT_OFF );
							::WritePolygonSide( f, (*PTS)[0].x, (*PTS)[0].y, (*PTS)[1].x, (*PTS)[1].y, tp, N_SIDES_APPROX_ARC, LIGHT_ON );
						}
					}
				}
			}
			// end of file
			if( PASS0 )
				continue;
			f->WriteString( "M00*\n" );
			return 0;
		}
		// establish nesting order of copper areas and cutouts:
		//	- for each area, tabulate all cutouts that contain it
		//	LOOP:
		//	- draw all areas that are only contained by cutouts that are already drawn
		//	- draw cutouts for those areas
		//  END_LOOP:
		//
		// loop through all nets and add areas to lists
		BOOL areas_present = FALSE;
		int num_area_nets = 0;		// 0, 1 or 2 (2 if more than 1) 
		int num_areas = 0;
		cnet * first_area_net = NULL;
		CArray<cnet*> area_net_list;
		CArray<carea*> area_list;
		cnet * net = nl->GetFirstNet();
		RECT areas_rect;
		areas_rect.left = areas_rect.bottom = INT_MAX;
		areas_rect.top = areas_rect.right = INT_MIN;
		while( net )
		{
			// loop through all areas
			for( int ia=0; ia<net->nareas; ia++ )
			{
				CPolyLine * p = net->area[ia].poly;
				if( p->GetHatch() == CPolyLine::DIAGONAL_FULL )
				{
				}
				else if( p->GetHatch() == CPolyLine::NO_HATCH && p->GetW() == 0 )
				{//  area Ghost
				}
				else if( p->GetLayer() == layer )
				{
					areas_present = TRUE;
					RECT a_r = net->area[ia].poly->GetBounds();
					SwellRect( &areas_rect, a_r );
					//
					// area on this layer, add to lists
					area_net_list.Add( net );
					area_list.Add( &net->area[ia] );
					num_areas++;
				}
			}
			// keep track of whether we have areas on separate nets
			if( num_area_nets == 0 )
			{
				num_area_nets = 1;
				first_area_net = net;
			}
			else if( num_area_nets == 1 && net != first_area_net )
				num_area_nets = 2;
			net = nl->GetNextNet(/*LABEL*/);
		}
		if( areas_present )
			SwellRect( &areas_rect, fill_clearance );
		//=============================================================================
		// ********************** draw areas hsNone and hsEdge ************************
		//=============================================================================
		nl->MarkAllNets(0);
		BOOL YES = TRUE;
		long long maxS;
		while (YES)
		{
			YES = FALSE;
			cnet * best_net = 0;
			int best_ia;
			maxS = 0;
			net = nl->GetFirstNet();
			// find next area
			while( net )
			{
				for( int ia=0; ia<net->nareas; ia++ )
				{
					carea * a = &net->area[ia];
					if (a->utility || a->poly->GetLayer() != layer ) 
						continue;
					int hs = a->poly->GetHatch();
					if( hs == CPolyLine::NO_HATCH && a->poly->GetW() == 0 )
						continue;//  area Ghost
					if ( hs == CPolyLine::DIAGONAL_FULL )
						continue;
					areas_present = TRUE;
					RECT a_r;
					a_r = a->poly->GetBounds();
					long long S = (long long)(a_r.top-a_r.bottom)*(long long)(a_r.right-a_r.left);
					if ( S > maxS )
					{
						maxS = S;
						best_net = net;
						best_ia = ia;
						YES = TRUE;
					}
				}
				net = nl->GetNextNet(/*LABEL*/);
			}
			if ( YES )
			{
				carea * a = &best_net->area[best_ia];
				int a_w = a->poly->GetW();
				a->utility = 1;
				// draw outline polygon
				if ( PASS1 )
				{
					f->WriteString( "\nG04 ----------------------- Draw copper area *\n" );
					SET_LPD
					for( int icont=0; icont<a->poly->GetNumContours(); icont++ )
					{
						if ( icont == 1 )
							SET_LPC
						f->WriteString( "G36*\n" );
						int start = a->poly->GetContourStart(icont);
						::WriteMoveTo( f, a->poly->GetX(start), a->poly->GetY(start), LIGHT_OFF );
						for (int ic=start; ic<=a->poly->GetContourEnd(icont); ic++)
						{
							int last_x = a->poly->GetX(ic);
							int last_y = a->poly->GetY(ic);
							int x = a->poly->GetX(a->poly->GetIndexCornerNext(ic));
							int y = a->poly->GetY(a->poly->GetIndexCornerNext(ic));
							int style = a->poly->GetSideStyle(ic);
							::WritePolygonSide( f, last_x, last_y, x, y, style, N_SIDES_APPROX_ARC, LIGHT_ON );
						}
						f->WriteString( "G37*\n" );
					}
				}
				// driwing edge
				if ( abs(a_w) > _2540 )
				{
					if ( PASS1 )
					{
						if (a_w > _2540)
							SET_LPD
						else
							SET_LPC
					}
					CAperture ap( CAperture::AP_CIRCLE, abs(a_w), 0 );
					ChangeAperture( &ap, &current_ap, &ap_array, PASS0, f );
					// turn on linear interpolation, move to first corner
					if ( PASS1 )
					{
						for( int icont=0; icont<a->poly->GetNumContours(); icont++ )
						{
							int start = a->poly->GetContourStart(icont);
							::WriteMoveTo( f, a->poly->GetX(start), a->poly->GetY(start), LIGHT_OFF );
							for (int ic=start; ic<=a->poly->GetContourEnd(icont); ic++)
							{
								int last_x = a->poly->GetX(ic);
								int last_y = a->poly->GetY(ic);
								int x = a->poly->GetX(a->poly->GetIndexCornerNext(ic));
								int y = a->poly->GetY(a->poly->GetIndexCornerNext(ic));
								int style = a->poly->GetSideStyle(ic);
								::WritePolygonSide( f, last_x, last_y, x, y, style, N_SIDES_APPROX_ARC, LIGHT_ON ); 
							}
						}
					}
				}
			}
		}
		//
		if( areas_present )
		{	//=============================================================================
			// ********** draw pad, trace, and via clearances and thermals ****************
			//=============================================================================
			//
			// first, remove all GpcPolys
			net = nl->GetFirstNet();
			while( net )
			{
				for( int ia=0; ia<net->nareas; ia++ )
				{
					carea * a = &net->area[ia];
					CPolyLine * p = a->poly;
					p->FreeGpcPoly();
				}
				net = nl->GetNextNet(/*LABEL*/);
			}
			if( PASS1 ) 
			{			
				f->WriteString( "\nG04 -------------------- Draw copper area clearances (negative)*\n" );
				SET_LPC
				current_ap.m_type = CAperture::AP_NONE;	// force selection of aperture
			}
			if( pl ) 
			{
				// iterate through all parts for pad clearances and thermals
				if( PASS1 )
				{
					f->WriteString( "\nG04 Draw clearances for pads*\n" );
				}	
				for( cpart * part = pl->m_start.next; part->next != 0; part = part->next ) 
				{			
					CShape * s = part->shape;
					if( s )
					{
						RECT pr;
						pl->GetPartBoundingRect( part, &pr );
						if( RectsIntersection( pr, areas_rect ) == -1 )
							continue;

						// clearance
						int _clearance = fill_clearance;
						if( part->m_merge >= 0 )
							_clearance = ml->GetClearance( part->m_merge );
						if( _clearance == 0 )
							_clearance = fill_clearance;

						// iterate through all pins
						for( int ip=0; ip<s->GetNumPins(); ip++ )
						{
							// get pad info
							int pad_type, pad_x, pad_y, pad_w, pad_l, pad_r, pad_hole, pad_angle;
							cnet * pad_net;
							int pad_connect_status, pad_connect_flag, clearance_type ;
							BOOL bPad = pl->GetPadDrawInfo( part, ip, layer,
								!(flags & GERBER_NO_PIN_THERMALS), 
								!(flags & GERBER_NO_SMT_THERMALS),
								mask_clearance, paste_mask_shrink,
								&pad_type, &pad_x, &pad_y, &pad_w, &pad_l, &pad_r, &pad_hole, &pad_angle,
								&pad_net, &pad_connect_status, &pad_connect_flag, &clearance_type );

							if( bPad ) 
							{
								// pad or hole exists on this layer
								// parameters for clearance on adjacent areas (if needed)
								int area_pad_type = PAD_NONE;
								int area_pad_wid = 0;
								int area_pad_len = 0;
								int area_pad_radius = 0;
								int area_pad_angle = 0;
								int area_pad_clearance = 0;
								// parameters for clearance aperture
								int type = CAperture::AP_NONE;
								int size1=0, size2=0, size3=0, size4=0; 
								if( pad_type == PAD_NONE && pad_hole > 0 )
								{
									// through-hole pin, no pad, just annular ring and hole
									if( pad_connect_status & CPartList::AREA_CONNECT )
									{
										// hole connects to copper area
										if( flags & GERBER_NO_PIN_THERMALS || pad_connect_flag == PAD_CONNECT_NOTHERMAL )
										{
											// no thermal, no clearance needed except on adjacent areas for hole
											area_pad_type = PAD_ROUND;
											area_pad_wid = pad_hole;
											area_pad_clearance = hole_clearance;
											// testing
											if( clearance_type != CLEAR_NONE )
												ASSERT(0);
										}
										else
										{
											// make thermal for annular ring and hole
											type = CAperture::AP_THERMAL;
											size1 = pad_w + 2*thermal_clearance;
											size2 = pad_w;	// inner diameter
											area_pad_type = PAD_ROUND;
											area_pad_wid = pad_w;
											area_pad_clearance = _clearance;
											// testing
											if( clearance_type != CLEAR_THERMAL )
												ASSERT(0);
										}
									}
									else
									{
										// no area connection, just make clearance for annular ring and hole
										type = CAperture::AP_CIRCLE;
										size1 = pad_hole + 2*hole_clearance;
										// testing
										if( clearance_type != CLEAR_NORMAL )
											ASSERT(0);
									}
								}
								else if( pad_type != PAD_NONE )
								{
									if( pad_connect_status & CPartList::AREA_CONNECT ) 
									{
										// pad connects to copper area, make clearance or thermal
										// see if we need to make a thermal
										size1 = 0;
										size3 = thermal_wid;
										size4 = thermal_clearance;
										BOOL bMakeThermal = (clearance_type == CLEAR_THERMAL);
										if( pad_type == PAD_ROUND || pad_type == PAD_OCTAGON )
										{
											if( bMakeThermal )
											{
												size1 = ( pad_w + 2*thermal_clearance );
												// make thermal for pad
												pad_r = (size1- 2*size4 - size3)/2;
												if( flags & GERBER_90_THERMALS )
													type = CAperture::AP_RECT_THERMAL; 
												else
													type = CAperture::AP_RECT_THERMAL_45;									
											}
											size2 = size1;
											// make clearance for adjacent areas
											area_pad_type = pad_type;
											area_pad_wid = pad_w;
											area_pad_clearance = _clearance;
										}
										else if( pad_type == PAD_RECT || pad_type == PAD_RRECT || pad_type == PAD_OVAL 
											|| pad_type == PAD_SQUARE )
										{		
											// make thermal for pad
											// can't use an aperture for this pad, need to draw a polygon
											// if hole, check hole clearance
											int x1, x2, y1, y2;
											if( pad_type == PAD_RECT || pad_type == PAD_RRECT || pad_type == PAD_OVAL )
											{
												if( pad_angle%90 == 0 && pad_angle%180 )
												{
													x1 = pad_x - pad_w/2;
													x2 = pad_x + pad_w/2;
													y1 = pad_y - pad_l/2;
													y2 = pad_y + pad_l/2;
													pad_angle = 0;
												}
												else 
												{
													x1 = pad_x - pad_l/2;
													x2 = pad_x + pad_l/2;
													y1 = pad_y - pad_w/2;
													y2 = pad_y + pad_w/2;
												}
											}
											else
											{
												x1 = pad_x - pad_w/2;
												x2 = pad_x + pad_w/2;
												y1 = pad_y - pad_w/2;
												y2 = pad_y + pad_w/2;
											}
											area_pad_wid = x2-x1;
											area_pad_len = y2-y1;
											if( bMakeThermal )
											{
												// add clearance
												x1 -= thermal_clearance;
												x2 += thermal_clearance;
												y1 -= thermal_clearance;
												y2 += thermal_clearance;
												// make thermal for pad
												if( flags & GERBER_90_THERMALS )
													type = CAperture::AP_RECT_THERMAL; 
												else
													type = CAperture::AP_RECT_THERMAL_45;
												size1 = (x2 - x1);	// width
												size2 = (y2 - y1);	// height
												int max_pad_r = (min(size1,size2) - 2*size4 - size3)/2;
												max_pad_r = max( 0, max_pad_r );
												pad_r = min( pad_r, max_pad_r );
											}
											area_pad_clearance = _clearance;
											area_pad_type = pad_type;
											area_pad_angle = pad_angle;
											if (pad_type == PAD_SQUARE || pad_type == PAD_RECT)
												area_pad_radius = 0;
											else if (pad_type == PAD_RRECT)
												area_pad_radius = pad_r;
											else if (pad_type == PAD_OVAL)
												area_pad_radius = min((float)pad_w/2.05,(float)pad_l/2.05);
										}
										else 
											ASSERT(0);
									}
									else
									{
										// pad doesn't connect to area, make clearance for pad and hole
										size1 = pad_w + 2*_clearance;
										size2 = 0;
										if( pad_type == PAD_ROUND )
										{
											type = CAperture::AP_CIRCLE;
										}
										else if( pad_type == PAD_SQUARE )
										{
											type = CAperture::AP_SQUARE;
										}
										else if( pad_type == PAD_OCTAGON )
										{
											type = CAperture::AP_OCTAGON;
										}
										else if( pad_type == PAD_OVAL || pad_type == PAD_RECT || pad_type == PAD_RRECT ) 
										{
											if( pad_type == PAD_OVAL )
												type = CAperture::AP_OVAL;
											if( pad_type == PAD_RECT )
												type = CAperture::AP_RECT;
											if( pad_type == PAD_RRECT )
												type = CAperture::AP_RRECT;
											size1 = pad_l + 2*_clearance;
											size2 = pad_w + 2*_clearance;
											if( pad_type == PAD_RRECT )
												size3 = pad_r + _clearance;
											if( pad_angle%90 == 0 && pad_angle%180 )
											{
												int temp = size1;
												size1 = size2;
												size2 = temp;
												pad_angle = 0;
											}
										}
									}
								}
								if( pad_connect_status & CPartList::INSIDE_AREA_FULL )
								{
									type = CAperture::AP_NONE;
								}
								// now flash the aperture
								if( type != CAperture::AP_NONE )  
								{
									CAperture pad_ap( type, size1, size2, size3, size4 );
									ChangeAperture( &pad_ap, &current_ap, &ap_array, PASS0, f );
									if( PASS1 )
									{
										if(type == CAperture::AP_RECT_THERMAL && 
												min(size1-2*thermal_clearance,size2-2*thermal_clearance) <= thermal_wid )
											WriteTiltPolygon( pad_x, pad_y, type, size1, size2, size3, size4, 
															  0, pad_angle, f );
										else if(type == CAperture::AP_RECT_THERMAL || type == CAperture::AP_RECT_THERMAL_45 )
											WriteTiltPolygon( pad_x, pad_y, type, size1, size2, size3, size4, 
															  pad_r+size4, pad_angle, f );
										else if( pad_angle%90 && type != CAperture::AP_CIRCLE )
											WriteTiltPolygon( pad_x, pad_y, type, size1, size2, size3, size4, 
															  pad_r+_clearance, pad_angle, f );
										else
												// now flash the pad
												::WriteMoveTo( f, pad_x, pad_y, LIGHT_FLASH );
									}
								}
								// now create clearances on adjacent areas for pins connected
								// to copper area
								if( area_pad_type != PAD_NONE ) 
								{
									if ( PASS1 )
										DrawClearanceInForeignAreas( pad_net, area_pad_type, 0, pad_x, pad_y,
											0, 0, area_pad_wid, area_pad_len, area_pad_radius, pad_angle,
											f, flags, layer, area_pad_clearance,
											&area_net_list, &area_list, log );
								}
							}
						}
					}
					if( layer >= LAY_TOP_COPPER )
					{
						int nstrokes = part->m_outline_stroke.GetSize();
						if( nstrokes )
						{
							for( int ips=0; ips<nstrokes; ips++ )
							{
								if( !part->m_outline_stroke[ips] )
									continue;
								if( !part->m_outline_stroke[ips]->visible )
									continue;
								dl_element * h_el = part->m_outline_stroke[ips];
								int lm = h_el->layers_bitmap;
								if( getbit(lm,layer) )
								{
									int s_w = part->m_outline_stroke[ips]->dlist->Get_el_w(part->m_outline_stroke[ips]);
									s_w = max( s_w, min_silkscreen_stroke_wid );
									CAperture outline_ap( CAperture::AP_CIRCLE, (s_w+2*fill_clearance), 0 );
									ChangeAperture( &outline_ap, &current_ap, &ap_array, PASS0, f );
									// move to start of stroke
									if( PASS1 )
									{
										CArray<CPoint> * PA = dl->Get_Points( h_el, NULL, 0 );
										int np = PA->GetSize();
										if( h_el->gtype == DL_LINES_ARRAY )
										{	
											CPoint * PT = new CPoint[np];//ok
											dl->Get_Points( h_el, PT, &np );
											for( int ih=0; ih<np-1; ih+=2 )
											{
												::WriteMoveTo( f, PT[ih].x, PT[ih].y, LIGHT_OFF );
												::WritePolygonSide( f, PT[ih].x, PT[ih].y, PT[ih+1].x,PT[ih+1].y,
																	CPolyLine::STRAIGHT, N_SIDES_APPROX_ARC, LIGHT_ON );
											}
											delete PT;
										}
										if( h_el->gtype == DL_POLYGON || h_el->gtype == DL_POLYLINE )
										{	
											CPoint * PT = new CPoint[np];//ok
											dl->Get_Points( h_el, PT, &np );
											if( np >= 2 )
											{
												::WriteMoveTo( f, PT[0].x, PT[0].y, LIGHT_OFF );
												for( int ih=1; ih<np; ih++ )
													::WriteMoveTo( f, PT[ih].x, PT[ih].y, LIGHT_ON );
												if( h_el->gtype == DL_POLYGON )
												{
													::WriteMoveTo( f, PT[0].x, PT[0].y, LIGHT_ON );
													f->WriteString( "G36*\n" );
													::WriteMoveTo( f, PT[0].x, PT[0].y, LIGHT_OFF );
													for( int ih=1; ih<np; ih++ )
														::WriteMoveTo( f, PT[ih].x, PT[ih].y, LIGHT_ON );
													::WriteMoveTo( f, PT[0].x, PT[0].y, LIGHT_ON );
													f->WriteString( "G37*\n" );
												}
											}
											delete PT;
										}
									}
								}
							}
						}
					}
				}
			} // end if pl
			if( nl ) 
			{
				// iterate through all nets and draw trace and via clearances
				if( PASS1 )
				{
					f->WriteString( "\nG04 Draw clearances for traces*\n" );
				}
				net = nl->GetFirstNet();
				while( net )
				{
					for( int ic=0; ic<net->nconnects; ic++ )
					{
						int nsegs = net->connect[ic].nsegs;
						cconnect * c = &net->connect[ic]; 

						// clearance
						int _clearance = fill_clearance;
						if( c->m_merge >= 0 )
							_clearance = ml->GetClearance(c->m_merge);
						if( _clearance == 0 )
							_clearance = fill_clearance;
						//
						BOOL PrevIntOwnNet = FALSE;
						for( int is=nsegs-1; is>=0; is-- )
						{
							// get segment and vertices
							cseg * s = &c->seg[is];
							cvertex * pre_vtx = &c->vtx[is];
							cvertex * post_vtx = &c->vtx[is+1];
							double xi = pre_vtx->x;
							double yi = pre_vtx->y;
							double xf = post_vtx->x;
							double yf = post_vtx->y;
							int test;
							int pad_w;
							int hole_w;
							nl->GetViaPadInfo( net, ic, is+1, layer, 
								&pad_w, &hole_w, &test );
							if( hole_w == 0 )
							{
								RECT sr = rect(xi,yi,xf,yf);
								SwellRect( &sr, s->width/2 );
								if( RectsIntersection( sr, areas_rect ) == -1 )
									continue;
							}
							BOOL bIntOwnNet = FALSE;
							if( s->layer == layer && num_area_nets == 1 && net != first_area_net ) 
							{
								// segment is on this layer and there is a single copper area
								// on this layer not on the same net, draw clearance
								int type = CAperture::AP_CIRCLE;
								int size1 = s->width + 2*_clearance;
								CAperture seg_ap( type, size1, 0 );
								ChangeAperture( &seg_ap, &current_ap, &ap_array, PASS0, f );
								if( PASS1 )
								{
									WriteMoveTo( f, xi, yi, LIGHT_OFF );
									WriteMoveTo( f, xf, yf, LIGHT_ON );
								}
							}
							else if( s->layer == layer && num_area_nets > 1 )
							{
								// test for segment intersection with area on own net
								//
								for( int ia=0; ia<area_list.GetSize(); ia++ )
								{
									CPolyLine * poly = area_list[ia]->poly; 
									int a_w  = poly->GetW();
									if( net == area_net_list[ia] )
									{
										if( poly->TestPointInside( xi, yi ) )
										{
											bIntOwnNet = TRUE;
											break;
										}
										if( poly->TestPointInside( xf, yf ) )
										{
											bIntOwnNet = TRUE;
											break;
										}
									}
									for( int icont=0; icont<poly->GetNumContours(); icont++ )
									{
										int cont_start = poly->GetContourStart(icont);
										int cont_end = poly->GetContourEnd(icont);
										for( int is=cont_start; is<=cont_end; is++ )
										{
											// test for clearance from area sides < fill_clearance
											int x2i = poly->GetX(is);
											int y2i = poly->GetY(is);
											int ic2 = is+1;
											if( ic2 > cont_end )
												ic2 = cont_start;
											int x2f = poly->GetX(ic2);
											int y2f = poly->GetY(ic2);
											int style2 = poly->GetSideStyle( is );
											int d = ::GetClearanceBetweenSegments( xi, yi, xf, yf, CPolyLine::STRAIGHT, s->width,
												x2i, y2i, x2f, y2f, style2, a_w, _clearance, 0, 0 );
											if( d < _clearance )
											{
												bIntOwnNet = TRUE;
												break;
											}
										}
										if( bIntOwnNet )
											break;
									}
								}
								if( bIntOwnNet )
								{
									// handle segment that crosses from an area on its own net to
									// an area on a foreign net
									if ( PASS1 )
										DrawClearanceInForeignAreas( net, -1, s->width, xi, yi, xf, yf,
												0, 0, 0, 0, f, flags, layer, _clearance, 
												&area_net_list, &area_list, log);
								}
								else
								{
									// segment does not intersect area on own net, just make clearance
									int w = s->width + 2*_clearance;
									CAperture seg_ap( CAperture::AP_CIRCLE, w, 0 );
									ChangeAperture( &seg_ap, &current_ap, &ap_array, PASS0, f );
									if( PASS1 )
									{
										WriteMoveTo( f, pre_vtx->x, pre_vtx->y, LIGHT_OFF );
										WriteMoveTo( f, post_vtx->x, post_vtx->y, LIGHT_ON );
									}
								}
							}
							// flash the via clearance if necessary
							if( hole_w > 0 && layer >= LAY_TOP_COPPER )
							{
								RECT vr = rect(xf,yf,xf,yf);
								SwellRect( &vr, pad_w/2 );
								if( RectsIntersection( vr, areas_rect ) == -1 )
								{
									RECT sr = rect(xi,yi,xf,yf);
									SwellRect( &sr, s->width/2 );
									if( RectsIntersection( sr, areas_rect ) == -1 )
										continue;
								}
								// this is a copper layer
								// set aperture to draw normal via clearance
								int type = CAperture::AP_CIRCLE;
								int size1 = pad_w + 2*_clearance;
								int size2 = 0;
								// set parameters for no foreign area clearance
								int area_pad_type = PAD_ROUND;
								int area_pad_wid = pad_w;
								int area_pad_len = 0;
								int area_pad_radius = 0;
								int area_pad_angle = 0;
								int area_pad_clearance = _clearance;
								if( pad_w == 0 )
								{
									// no pad, just make hole clearance 
									type = CAperture::AP_CIRCLE;
									size1 = hole_w + 2*hole_clearance;
									// except on adjacent foreign nets
									area_pad_type = PAD_ROUND;
									area_pad_wid = pad_w;
									area_pad_clearance = hole_clearance;
								}
								else if( test & CNetList::VIA_AREA )
								{
									// inner layer and connected to copper area
									if( flags & GERBER_NO_VIA_THERMALS )
									{
										// no thermal, therefore no clearance
										type = CAperture::AP_NONE;
										size1 = 0;
										// except on adjacent foreign nets
										area_pad_type = PAD_ROUND;
										area_pad_wid = pad_w;
										area_pad_clearance = _clearance;
									}
									else
									{
										// thermal
										type = CAperture::AP_THERMAL;
										size1 = pad_w + 2*thermal_clearance;
										size2 = pad_w;
										area_pad_type = PAD_ROUND;
										area_pad_wid = pad_w;
										area_pad_clearance = _clearance;
									}
								}
								if( type != CAperture::AP_NONE && bIntOwnNet == 0 && PrevIntOwnNet == 0 )
								{
									CAperture via_ap( type, size1, size2 );
									ChangeAperture( &via_ap, &current_ap, &ap_array, PASS0, f );
									if( PASS1 )
									{
										// flash the via clearance
										WriteMoveTo( f, post_vtx->x, post_vtx->y, LIGHT_FLASH );
									}
								}
								else if( area_pad_type != PAD_NONE && num_area_nets > 1 )
								{
									if ( PASS1 )
										DrawClearanceInForeignAreas( net, area_pad_type, 0,
											xf, yf, 0, 0, area_pad_wid, 0, 0, 0,
											f, flags, layer, area_pad_clearance, &area_net_list, &area_list, log );
								}
							}
							PrevIntOwnNet = bIntOwnNet;
						}
					}
					net = nl->GetNextNet(/*LABEL*/);
				}
			}
			//
			if( tl )
			{
				// draw clearances for text
				if( PASS1 )
				{
					f->WriteString( "\nG04 Draw clearances for text*\n" );
				}
				for( int it=0; it<tl->text_ptr.GetSize(); it++ )
				{
					CText * t = tl->text_ptr[it];
					RECT tr;
					tl->GetTextRectOnPCB(t,&tr);
					if( RectsIntersection( areas_rect, tr ) == -1 )
						continue;
					if( t->m_layer == layer )
					{
						// clearance
						int _clearance = fill_clearance;
						if( t->m_merge >= 0 )
							_clearance = ml->GetClearance( t->m_merge );
						if( _clearance == 0 )
							_clearance = fill_clearance;

						// draw text
						int w = t->m_stroke_width + 2*_clearance;

						if( t->m_bNegative )
							w = t->m_stroke_width;	// if negative text, just draw the text
						CAperture text_ap( CAperture::AP_CIRCLE, w, 0 );
						ChangeAperture( &text_ap, &current_ap, &ap_array, PASS0, f );
						if( PASS1 )
						{
							if( t->dl_el )
							{
								CArray<CPoint> * tpt = dl->Get_Points( t->dl_el, NULL, 0 );
								int np = tpt->GetSize();
								CPoint * m_stroke = new CPoint[np];//ok
								dl->Get_Points( t->dl_el, m_stroke, &np );
								for( int istroke=0; istroke+1<np; istroke+=2 )
								{
									::WriteMoveTo( f, m_stroke[istroke].x, m_stroke[istroke].y, LIGHT_OFF );
									::WriteMoveTo( f, m_stroke[istroke+1].x, m_stroke[istroke+1].y, LIGHT_ON );
								}
								delete m_stroke;
							}
						}
					}
				}
			} // end loop through nets
		}
		//=============================================================================
		// ********************** draw areas hs Full **********************************
		//=============================================================================
		nl->MarkAllNets(0);
		YES = TRUE;
		//long long maxS;
		while (YES)
		{
			YES = FALSE;
			cnet * best_net = 0;
			int best_ia;
			maxS = 0;
			net = nl->GetFirstNet();
			// find next area
			while( net )
			{
				for( int ia=0; ia<net->nareas; ia++ )
				{
					carea * a = &net->area[ia];
					if (a->utility || a->poly->GetLayer() != layer ) 
						continue;
					if (a->poly->GetHatch() != CPolyLine::DIAGONAL_FULL)
						continue;
					RECT a_r;
					a_r = a->poly->GetBounds();
					long long S = (long long)(a_r.top-a_r.bottom)*(long long)(a_r.right-a_r.left);
					if ( S >= maxS )
					{
						maxS = S;
						best_net = net;
						best_ia = ia;
						YES = TRUE;
					}
				}
				net = nl->GetNextNet(/*LABEL*/);
			}
			if ( YES )
			{
				carea * a = &best_net->area[best_ia];
				int a_w = a->poly->GetW();
				a->utility = 1;
				// draw outline polygon
				areas_present = TRUE;
				if ( abs(a_w) > _2540 )
				{	//=============================================================================
					// ********************** draw areas hs Full with width!=0 ********************
					//=============================================================================	
					if (PASS0)
					{
						CAperture ap( CAperture::AP_CIRCLE, abs(a_w), 0 );
						ChangeAperture( &ap, &current_ap, &ap_array, PASS0, f );
					}
					CAperture ap( CAperture::AP_CIRCLE, abs(a_w)*9/10, 0 );
					ChangeAperture( &ap, &current_ap, &ap_array, PASS0, f );
					// turn on linear interpolation, move to first corner
					if ( PASS1 )
					{
						// driwing hatch
						f->WriteString( "G04 driwing hatch*\n" );
						SET_LPD
						int nh = a->poly->GetHatchSize();
						for (int h=0; h<nh; h++)
						{
							RECT hatch = a->poly->GetHatchLoc(h);
							::WriteMoveTo( f, hatch.left, hatch.bottom, LIGHT_OFF );
							::WriteMoveTo( f, hatch.right, hatch.top, LIGHT_ON );
						}
						// driwing edge
						CAperture ap( CAperture::AP_CIRCLE, abs(a_w), 0 );
						ChangeAperture( &ap, &current_ap, &ap_array, PASS0, f );
						if ( a_w > _2540 )
							SET_LPD
						else
							SET_LPC
						for( int icont=0; icont<a->poly->GetNumContours(); icont++ )
						{
							int start = a->poly->GetContourStart(icont);
							::WriteMoveTo( f, a->poly->GetX(start), a->poly->GetY(start), LIGHT_OFF );
							for (int ic=start; ic<=a->poly->GetContourEnd(icont); ic++)
							{
								int last_x = a->poly->GetX(ic);
								int last_y = a->poly->GetY(ic);
								int x = a->poly->GetX(a->poly->GetIndexCornerNext(ic));
								int y = a->poly->GetY(a->poly->GetIndexCornerNext(ic));
								int style = a->poly->GetSideStyle(ic);
								::WritePolygonSide( f, last_x, last_y, x, y, style, N_SIDES_APPROX_ARC, LIGHT_ON ); 
							}
						}
					}
				}
				else
				{
					if ( PASS1 )
					{	//=============================================================================
						// ********************** draw areas hs Full with width==0 ********************
						//=============================================================================
						f->WriteString( "\nG04 ----------------------- Draw copper area *\n" );
						SET_LPD
						for( int icont=0; icont<a->poly->GetNumContours(); icont++ )
						{
							if ( icont == 1 )
								SET_LPC
							f->WriteString( "G36*\n" );
							int start = a->poly->GetContourStart(icont);
							::WriteMoveTo( f, a->poly->GetX(start), a->poly->GetY(start), LIGHT_OFF );
							for (int ic=start; ic<=a->poly->GetContourEnd(icont); ic++)
							{
								int last_x = a->poly->GetX(ic);
								int last_y = a->poly->GetY(ic);
								int x = a->poly->GetX(a->poly->GetIndexCornerNext(ic));
								int y = a->poly->GetY(a->poly->GetIndexCornerNext(ic));
								int style = a->poly->GetSideStyle(ic);
								::WritePolygonSide( f, last_x, last_y, x, y, style, N_SIDES_APPROX_ARC, LIGHT_ON );
							}
							f->WriteString( "G37*\n" );
						}
					}
				}
			}
		}
		//=============================================================================
		// ********************** draw pads, vias and traces **************************
		//=============================================================================
		if( PASS1 )
		{
			f->WriteString( "\nG04 -------------- Draw Parts, Pads, Traces, Vias and Text (positive)*\n" );
			SET_LPD
			current_ap.m_type = CAperture::AP_NONE;	// force selection of aperture
		}
		// draw pads and reference designators
		if( pl )
		{
			// iterate through all parts and draw pads
			cpart * part = pl->m_start.next;
			while( part->next != 0 )
			{
				CShape * s = part->shape;
				if( s )
				{
					if( PASS1 )
					{
						//ERROR IF '*' INCLUDED
						//line.Format( "G04 Draw part %s *\n", part->ref_des ); 
						//f->WriteString( line );
					}
					for( int ip=0; ip<s->GetNumPins(); ip++ )
					{
						// get pad info
						int pad_x;
						int pad_y;
						int pad_w;
						int pad_l;
						int pad_r;
						int pad_type;
						int pad_hole;
						int pad_connect;
						int pad_angle;
						cnet * pad_net;
						BOOL bPad = pl->GetPadDrawInfo( part, ip, layer,
							0, 0,
							mask_clearance, paste_mask_shrink,
							&pad_type, &pad_x, &pad_y, &pad_w, &pad_l, &pad_r, &pad_hole, &pad_angle,
							&pad_net, &pad_connect );

						// draw pad
						if( bPad && pad_type != PAD_NONE && pad_w > 0 )
						{
							int type, size1, size2, size3;
							if( pad_type == PAD_ROUND || pad_type == PAD_SQUARE 
								|| pad_type == PAD_OCTAGON || pad_type == PAD_OVAL
								|| pad_type == PAD_RECT || pad_type == PAD_RRECT )
							{
								type = CAperture::AP_CIRCLE;
								size1 = pad_w;
								size2 = 0;
								size3 = 0;
								if( pad_type == PAD_SQUARE )
									type = CAperture::AP_SQUARE;
								else if( pad_type == PAD_OCTAGON )
									type = CAperture::AP_OCTAGON;
								else if( pad_type == PAD_OVAL || pad_type == PAD_RECT || pad_type == PAD_RRECT )
								{
									if( pad_type == PAD_OVAL )
										type = CAperture::AP_OVAL;
									if( pad_type == PAD_RECT )
										type = CAperture::AP_RECT;
									if( pad_type == PAD_RRECT )
										type = CAperture::AP_RRECT;
									size1 = pad_l; 
									size2 = pad_w; 
									size3 = pad_r;
									if( pad_angle%90 == 0 && pad_angle%180 )
									{
										int temp = size1;
										size1 = size2;
										size2 = temp;
										pad_angle = 0;
									}
								}
								CAperture pad_ap( type, size1, size2, size3 ); 
								ChangeAperture( &pad_ap, &current_ap, &ap_array, PASS0, f );
								if( PASS1 )
								{
									if( pad_angle%90 && type != CAperture::AP_CIRCLE )
										WriteTiltPolygon( pad_x, pad_y, type, size1, size2, size3, 0, pad_r, pad_angle, f );
									else
										// now flash the pad
										::WriteMoveTo( f, pad_x, pad_y, LIGHT_FLASH );
								}
							}
						}
					}
				}
				// now draw silkscreen items
				if( layer == LAY_SILK_TOP || layer == LAY_SILK_BOTTOM ||
					layer == LAY_REFINE_TOP || layer == LAY_REFINE_BOT ||
					layer == LAY_TOP_COPPER || layer == LAY_BOTTOM_COPPER )
				{
					// draw part outline
					if( PASS1 )
					{
						//ERROR IF '*' INCLUDED
						//line.Format( "G04 draw part outline for part %s*\n", part->ref_des ); 
						//f->WriteString( line );
					}
					int nstrokes = part->m_outline_stroke.GetSize();
					if( nstrokes )
					{
						for( int ips=0; ips<nstrokes; ips++ )
						{
							if( !part->m_outline_stroke[ips] )
								continue;
							if( !part->m_outline_stroke[ips]->visible )
								continue;
							dl_element * h_el = part->m_outline_stroke[ips];
							int lm = h_el->layers_bitmap;
							if( getbit(lm,layer) )
							{
								int s_w = part->m_outline_stroke[ips]->dlist->Get_el_w(part->m_outline_stroke[ips]);
								s_w = max( s_w, min_silkscreen_stroke_wid );
								CAperture outline_ap( CAperture::AP_CIRCLE, s_w, 0 );
								ChangeAperture( &outline_ap, &current_ap, &ap_array, PASS0, f );
								// move to start of stroke
								if( PASS1 )
								{
									CArray<CPoint> * PA = dl->Get_Points( h_el, NULL, 0 );
									int np = PA->GetSize();
									if( h_el->gtype == DL_LINES_ARRAY )
									{	
										CPoint * PT = new CPoint[np];//ok
										dl->Get_Points( h_el, PT, &np );
										for( int ih=0; ih<np-1; ih+=2 )
										{
											::WriteMoveTo( f, PT[ih].x, PT[ih].y, LIGHT_OFF );
											::WritePolygonSide( f, PT[ih].x, PT[ih].y, PT[ih+1].x,PT[ih+1].y,
																CPolyLine::STRAIGHT, N_SIDES_APPROX_ARC, LIGHT_ON );
										}
										delete PT;
									}
									if( h_el->gtype == DL_POLYGON || h_el->gtype == DL_POLYLINE )
									{	
										CPoint * PT = new CPoint[np];//ok
										dl->Get_Points( h_el, PT, &np );
										if( np >= 2 )
										{
											::WriteMoveTo( f, PT[0].x, PT[0].y, LIGHT_OFF );
											for( int ih=1; ih<np; ih++ )
												::WriteMoveTo( f, PT[ih].x, PT[ih].y, LIGHT_ON );
											if( h_el->gtype == DL_POLYGON )
											{
												::WriteMoveTo( f, PT[0].x, PT[0].y, LIGHT_ON );
												f->WriteString( "G36*\n" );
												::WriteMoveTo( f, PT[0].x, PT[0].y, LIGHT_OFF );
												for( int ih=1; ih<np; ih++ )
													::WriteMoveTo( f, PT[ih].x, PT[ih].y, LIGHT_ON );
												::WriteMoveTo( f, PT[0].x, PT[0].y, LIGHT_ON );
												f->WriteString( "G37*\n" );
											}
										}
										delete PT;
									}
								}
							}
						}
					}
					if( (layer == LAY_SILK_TOP && part->side == 0) || (layer == LAY_SILK_BOTTOM && part->side == 1) )
					{
						// draw reference designator text
						if( part->m_ref_size && part->m_ref_vis )
						{
							if( PASS1 )
							{
								//ERROR IF '*' INCLUDED
								//line.Format( "G04 draw reference designator for part %s*\n", part->ref_des ); 
								//f->WriteString( line );
							}
							int s_w = max( part->m_ref_w, min_silkscreen_stroke_wid );
							CAperture ref_ap( CAperture::AP_CIRCLE, s_w, 0 );
							ChangeAperture( &ref_ap, &current_ap, &ap_array, PASS0, f );
							if( PASS1 )
							{
								CArray<CPoint> * pts = dl->Get_Points( part->dl_ref_el, NULL, 0 );
								for( int istroke=0; istroke<pts->GetSize(); istroke+=2 )
								{
									::WriteMoveTo( f,	(*pts)[istroke].x*m_pcbu_per_wu, 
														(*pts)[istroke].y*m_pcbu_per_wu, LIGHT_OFF );
									::WriteMoveTo( f,	(*pts)[istroke+1].x*m_pcbu_per_wu, 
														(*pts)[istroke+1].y*m_pcbu_per_wu, LIGHT_ON );
								}
							}
						}
						// draw value text
						if( part->m_value_size && part->m_value_vis && part->value.GetLength() )
						{
							if( PASS1 )
							{
								//ERROR IF '*' INCLUDED
								//line.Format( "G04 draw value for part %s*\n", part->ref_des ); 
								//f->WriteString( line );
							}
							int s_w = max( part->m_value_w, min_silkscreen_stroke_wid );
							CAperture value_ap( CAperture::AP_CIRCLE, s_w, 0 );
							ChangeAperture( &value_ap, &current_ap, &ap_array, PASS0, f );
							if( PASS1 )
							{
								CArray<CPoint> * pts = dl->Get_Points( part->dl_value_el, NULL, 0 );
								for( int istroke=0; istroke<pts->GetSize(); istroke+=2 )
								{
									::WriteMoveTo( f,	(*pts)[istroke].x*m_pcbu_per_wu, 
														(*pts)[istroke].y*m_pcbu_per_wu, LIGHT_OFF );
									::WriteMoveTo( f,	(*pts)[istroke+1].x*m_pcbu_per_wu, 
														(*pts)[istroke+1].y*m_pcbu_per_wu, LIGHT_ON );
								}
							}
						}
					}
				}
				// go to next part
				part = part->next;
			}
		}
		// draw vias and traces
		if( nl )
		{
			// iterate through all nets
			if( PASS1 )
			{
				f->WriteString( "\nG04 Draw traces*\n" );
			}
			POSITION pos;
			CString name;
			void * ptr;
			for( pos = nl->m_map.GetStartPosition(); pos != NULL; )
			{
				nl->m_map.GetNextAssoc( pos, name, ptr );
				cnet * net = (cnet*)ptr;
				for( int ic=0; ic<net->nconnects; ic++ )
				{
					int nsegs = net->connect[ic].nsegs;
					for( int is=0; is<nsegs; is++ )
					{
						// get segment info
						cseg * s = &(net->connect[ic].seg[is]);
						cvertex * pre_vtx = &(net->connect[ic].vtx[is]);
						cvertex * post_vtx = &(net->connect[ic].vtx[is+1]);
						// get following via info
						int test, pad_w, hole_w;
						nl->GetViaPadInfo( net, ic, is+1, 
							layer==LAY_SM_TOP?LAY_TOP_COPPER:(layer==LAY_SM_BOTTOM?LAY_BOTTOM_COPPER:layer),
							&pad_w, &hole_w, &test );
						if( s->layer == layer )
						{
							// segment is on this layer, draw it
							int w = s->width;
							CAperture seg_ap( CAperture::AP_CIRCLE, w, 0 );
							ChangeAperture( &seg_ap, &current_ap, &ap_array, PASS0, f );
							if( PASS1 )
							{
								WriteMoveTo( f, pre_vtx->x, pre_vtx->y, LIGHT_OFF );
								WriteMoveTo( f, post_vtx->x, post_vtx->y, LIGHT_ON );
							}
						}
						if( pad_w )
						{
							// via exists
							CAperture via_ap( CAperture::AP_CIRCLE, 0, 0 );
							int w = 0;
							if( layer == LAY_SM_TOP || layer == LAY_SM_BOTTOM )
							{
								if( !(flags & GERBER_MASK_VIAS) )
								{
									// solder mask layer, add mask clearance
									w = pad_w + 2*mask_clearance;
								}
							}
							else if( layer >= LAY_TOP_COPPER )
							{
								// copper layer, set aperture
								w = pad_w;							
							}
							if( w )
							{
								via_ap.m_size1 = w;
								ChangeAperture( &via_ap, &current_ap, &ap_array, PASS0, f );
								// flash the via
								if( PASS1 )
								{
									WriteMoveTo( f, post_vtx->x, post_vtx->y, LIGHT_FLASH );
								}
							}
						}
					}
				}
			}
		}
		// draw text
		if( tl )
		{
			if( PASS1 )
			{
				f->WriteString( "\nG04 Draw Text*\n" );
			}
			for( int it=0; it<tl->text_ptr.GetSize(); it++ )
			{
				CText * t = tl->text_ptr[it];
				if( !t->m_bNegative && t->m_font_size )
				{
					if( t->m_layer == layer )
					{
						// draw text
						int w = t->m_stroke_width;
						if( layer == LAY_SILK_TOP || layer == LAY_SILK_BOTTOM )
							w = max( t->m_stroke_width, min_silkscreen_stroke_wid );
						CAperture text_ap( CAperture::AP_CIRCLE, w, 0 );
						ChangeAperture( &text_ap, &current_ap, &ap_array, PASS0, f );
						if( PASS1 )
						{
							if( t->dl_el )
							{
								CArray<CPoint> * tpt = dl->Get_Points( t->dl_el, NULL, 0 );
								int np = tpt->GetSize();
								CPoint * m_stroke = new CPoint[np];//ok
								dl->Get_Points( t->dl_el, m_stroke, &np );
								for( int istroke=0; istroke+1<np; istroke+=2 )
								{
									::WriteMoveTo( f, m_stroke[istroke].x, m_stroke[istroke].y, LIGHT_OFF );
									::WriteMoveTo( f, m_stroke[istroke+1].x, m_stroke[istroke+1].y, LIGHT_ON );
								}
								delete m_stroke;
							}
						}
					}
				}
			}
		}

		// draw solder mask cutouts, board outlines, outlines graphic
		if( op )
		{
			for( int i=0; i<op->GetSize(); i++ )
			{
				CPolyLine * poly = &(*op)[i];
				int pL = poly->GetLayer();
				if ( layer == poly->GetLayer() ||
					 ((flags & GERBER_BOARD_OUTLINE) && pL == LAY_BOARD_OUTLINE)) 
				{
					int pW =  poly->GetW();
					if( pL == LAY_SILK_TOP || pL == LAY_SILK_BOTTOM )
						pW = max( pW, min_silkscreen_stroke_wid );
					else if( pW >= -1 )
						pW = max( pW, _2540 );
					else
						pW = min( pW, -_2540 );
					int pCl = poly->GetClosed();
					if( pCl )
					{
						if( abs(pW) > _2540 )
						{
							CAperture _ap( CAperture::AP_CIRCLE,  abs(pW)*9/10, 0 );
							ChangeAperture( &_ap, &current_ap, &ap_array, PASS0, f );
							if( PASS1 )
							{	
								if( int nh = poly->GetHatchSize() )
								{
									SET_LPD
									f->WriteString( "\nG04 Draw polyline*\n" );
									for (int h=0; h<nh; h++)
									{
										RECT hatch = poly->GetHatchLoc(h);
										::WriteMoveTo( f, hatch.left, hatch.bottom, LIGHT_OFF );
										::WriteMoveTo( f, hatch.right, hatch.top, LIGHT_ON );
									}
								}
							}
						}
					}
					// stroke outline with aperture to create clearance
					CAperture sm_ap( CAperture::AP_CIRCLE, abs(pW), 0 );
					ChangeAperture( &sm_ap, &current_ap, &ap_array, PASS0, f );
					if( PASS1 )
					{
						if( pW < _2540 )
							SET_LPC
						// stroke outline with aperture to create clearance
						int nc = poly->GetNumCorners();
						if( pCl == 0 )
							nc--;
						for( int ico=0; ico<poly->GetNumContours(); ico++ )
						{
							int cst = poly->GetContourStart(ico);
							int cend = poly->GetContourEnd(ico);
							cend = min(cend,nc-1);
							int last_x = poly->GetX(cst);
							int last_y = poly->GetY(cst);
							::WriteMoveTo( f, last_x, last_y, LIGHT_OFF );
							for( int ic=cst; ic<=cend; ic++ )
							{
								last_x = poly->GetX(ic);
								last_y = poly->GetY(ic);
								int in = poly->GetIndexCornerNext(ic);
								int x = poly->GetX(in);
								int y = poly->GetY(in);
								int style = poly->GetSideStyle(ic);
								::WritePolygonSide( f, last_x, last_y, x, y, style, N_SIDES_APPROX_ARC, LIGHT_ON );	
							}
						}
						SET_LPD
					}
				}
			}
		}

		// draw pilot holes for pads and vias
		if( (flags & GERBER_PILOT_HOLES) && pilot_diameter && (layer == LAY_TOP_COPPER || layer == LAY_BOTTOM_COPPER ) )
		{
			if( PASS1 )
			{			
				f->WriteString( "\nG04 ----------------------- Draw Pilot Holes (scratch)*\n" );
				SET_LPC
				current_ap.m_type = CAperture::AP_NONE;	// force selection of aperture
			}
			if( pl )
			{
				// iterate through all parts
				cpart * part = pl->m_start.next;
				while( part->next != 0 )
				{
					CShape * s = part->shape;
					if( s )
					{
						if( PASS1 )
						{
							//ERROR IF '*' INCLUDED
							//line.Format( "G04 draw pilot holes for part %s*\n", part->ref_des ); 
							//f->WriteString( line );
						}
						for( int ip=0; ip<s->GetNumPins(); ip++ )
						{
							pad * p = 0;
							padstack * ps = &s->m_padstack[ip];
							if( ps->hole_size )
							{
								p = &ps->top;
								// check current aperture and change if needed
								CAperture pad_ap( CAperture::AP_CIRCLE, pilot_diameter, 0 );
								ChangeAperture( &pad_ap, &current_ap, &ap_array, PASS0, f );
								// now flash the pad
								if( PASS1 )
								{
									::WriteMoveTo( f, part->pin[ip].x, part->pin[ip].y, LIGHT_FLASH );
								}
							}
						}
					}
					// go to next part
					part = part->next;
				}
			}
			// draw pilot holes for vias
			if( nl )
			{
				// iterate through all nets
				if( PASS1 )
				{
					f->WriteString( "\nG04 Draw pilot holes for vias*\n" );
				}
				POSITION pos;
				CString name;
				void * ptr;
				for( pos = nl->m_map.GetStartPosition(); pos != NULL; )
				{
					nl->m_map.GetNextAssoc( pos, name, ptr );
					cnet * net = (cnet*)ptr;
					for( int ic=0; ic<net->nconnects; ic++ )
					{
						int nsegs = net->connect[ic].nsegs;
						for( int is=0; is<nsegs; is++ )
						{
							// get segment
							cseg * s = &(net->connect[ic].seg[is]);
							cvertex * post_vtx = &(net->connect[ic].vtx[is+1]);
							if( post_vtx->via_w )
							{
								// via exists
								CAperture via_ap( CAperture::AP_CIRCLE, pilot_diameter, 0 );
								ChangeAperture( &via_ap, &current_ap, &ap_array, PASS0, f );
								// flash the via
								if( PASS1 )
									::WriteMoveTo( f, post_vtx->x, post_vtx->y, LIGHT_FLASH );
							}
						}
					}
				}
			}
		}

		// end of file
		if( PASS1 )
			f->WriteString( "M00*\n" );

	}	// end of pass
	return 0;
}

// find value in CArray<int> and return position in array
// if not found, add to array if add_ok = TRUE, otherwise return -1
//
int AddToArray( int value, CArray<int,int> * array )
{
	for( int i=0; i<array->GetSize(); i++ )
		if( value == array->GetAt(i) )
			return i;
	array->Add( value );
	return array->GetSize()-1;
}

// write NC drill file
//
int WriteDrillFile( CStdioFile * file, CPartList * pl, CNetList * nl, CArray<CPolyLine> * bd,
				   int n_x, int n_y, int space_x, int space_y )
{
	CArray<int,int> diameter;
	diameter.SetSize(0);

	// first, find all hole diameters for parts
	if( pl )
	{
		// iterate through all parts
		cpart * part = pl->m_start.next;
		while( part->next != 0 )
		{
			CShape * s = part->shape;
			if( s )
			{
				// get all pins
				for( int ip=0; ip<s->GetNumPins(); ip++ )
				{
					padstack * ps = &s->m_padstack[ip];
					if( ps->hole_size )
						::AddToArray( ps->hole_size/NM_PER_MIL, &diameter );
				}
			}
			// go to next part
			part = part->next;
		}
	}
	// now find hole diameters for vias
	if( nl )
	{
		// iterate through all nets
		// traverse map
		POSITION pos;
		CString name;
		void * ptr;
		for( pos = nl->m_map.GetStartPosition(); pos != NULL; )
		{
			nl->m_map.GetNextAssoc( pos, name, ptr );
			cnet * net = (cnet*)ptr;
			for( int ic=0; ic<net->nconnects; ic++ )
			{
				int nsegs = net->connect[ic].nsegs;
				for( int is=0; is<nsegs; is++ )
				{
					cvertex * v = &(net->connect[ic].vtx[is+1]);
					if( v->via_w )
					{
						// via
						int w = v->via_w;
						int h_w = v->via_hole_w;
						if( w && h_w )
							::AddToArray( abs(h_w)/NM_PER_MIL, &diameter );
					}
				}
			}
		}
	}

	// now, write data to file
	CString str;
	for( int id=0; id<diameter.GetSize(); id++ )
	{
		str.Format( ";Holesize %d = %6.1f MILS\n", 
			id+1, (double)diameter[id] );
		file->WriteString( str );
	}
	file->WriteString( "M48\n" );	// start header
	file->WriteString( "INCH,00.0000\n" );	// format (inch,retain all zeros,2.4)
	for( int id=0; id<diameter.GetSize(); id++ )
	{
		// write hole sizes
		int d = diameter[id];
		str.Format( "T%02dC%5.3f\n", id+1, (double)diameter[id]/1000.0 ); 
		file->WriteString( str );
	}
	file->WriteString( "%\n" );		// start data
	file->WriteString( "G05\n" );	// drill mode
	file->WriteString( "G90\n" );	// absolute data

	// get boundaries of board outline
	int bd_min_x = INT_MAX;
	int bd_min_y = INT_MAX;
	int bd_max_x = INT_MIN;
	int bd_max_y = INT_MIN;
	for( int ib=0; ib<bd->GetSize(); ib++ ) 
	{
		for( int ic=0; ic<(*bd)[ib].GetNumCorners(); ic++ )
		{
			int x = (*bd)[ib].GetX(ic);
			if( x < bd_min_x )
				bd_min_x = x;
			if( x > bd_max_x )
				bd_max_x = x;
			int y = (*bd)[ib].GetY(ic);
			if( y < bd_min_y )
				bd_min_y = y;
			if( y > bd_max_y )
				bd_max_y = y;
		}
	}
	int x_step = bd_max_x - bd_min_x + space_x;
	int y_step = bd_max_y - bd_min_y + space_y;
	for( int id=0; id<diameter.GetSize(); id++ )
	{
		// now write hole size and all holes
		int d = diameter[id];
		str.Format( "T%02d\n", id+1 ); 
		file->WriteString( str );
		// loop for panelization
		for( int ix=0; ix<n_x; ix++ )
		{
			int x_offset = ix * x_step;
			for( int iy=0; iy<n_y; iy++ )
			{
				int y_offset = iy * y_step;
				if( pl )
				{
					// iterate through all parts
					cpart * part = pl->m_start.next;
					while( part->next != 0 )
					{
						CShape * s = part->shape;
						if( s )
						{
							// get all pins
							for( int ip=0; ip<s->GetNumPins(); ip++ )
							{
								padstack * ps = &s->m_padstack[ip];
								if( ps->hole_size )
								{
									part_pin * p = &part->pin[ip];
									if( d == ps->hole_size/NM_PER_MIL )
									{
										str.Format( "X%.6dY%.6d\n", 
											(p->x + x_offset)/(NM_PER_MIL/10), 
											(p->y + y_offset)/(NM_PER_MIL/10) );
										file->WriteString( str );
									}
								}
							}
						}
						// go to next part
						part = part->next;
					}
				}
				// now find hole diameters for vias
				if( nl )
				{
					// iterate through all nets
					// traverse map
					POSITION pos;
					CString name;
					void * ptr;
					for( pos = nl->m_map.GetStartPosition(); pos != NULL; )
					{
						nl->m_map.GetNextAssoc( pos, name, ptr );
						cnet * net = (cnet*)ptr;
						for( int ic=0; ic<net->nconnects; ic++ )
						{
							int nsegs = net->connect[ic].nsegs;
							for( int is=0; is<nsegs; is++ )
							{
								cvertex * v = &(net->connect[ic].vtx[is+1]);
								if( v->via_w )
								{
									// via
									int h_w = v->via_hole_w;
									if( h_w )
									{
										if( d == abs(h_w)/NM_PER_MIL )
										{
											str.Format( "X%.6dY%.6d\n", 
												(v->x + x_offset)/(NM_PER_MIL/10), 
												(v->y + y_offset)/(NM_PER_MIL/10) );
											file->WriteString( str );
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	file->WriteString( "M30\n" );	// program end
	return 0;
}

