// utility routines
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
#define _USE_MATH_DEFINES
#include <math.h>
#include <time.h>
#include "DisplayList.h" 
 
// globals for timer functions
LARGE_INTEGER PerfFreq, tStart, tStop; 
int PerfFreqAdjust;
int OverheadTicks;

// function to find inflection-pont to create a "dogleg" of two straight-line segments
// where one segment is vertical or horizontal and the other is at 45 degrees or 90 degrees
// enter with:
//	pi = start point
//	pf = end point
//	mode = IM_90_45 or IM_45_90 or IM_90
//
CPoint GetInflectionPoint( CPoint pi, CPoint pf, int mode )
{
	CPoint p = pi;
	if( mode == IM_NONE )
		return p;

	int dx = pf.x - pi.x;
	int dy = pf.y - pi.y;
	if( dx == 0 || dy == 0 || abs(dx) == abs(dy) )
	{
		// only one segment needed
	}
	else
	{
		if( abs(dy) > abs(dx) )
		{
			// vertical > horizontal
			if( mode == IM_90 )
			{
				p.x = pi.x;
				p.y = pf.y;
			}
			else if( mode == IM_45_90 || mode == IM_90_45 )
			{
				int vert;	// length of vertical line needed
				if( dy > 0 )
					vert = dy - abs(dx);	// positive	
				else
					vert = dy + abs(dx);	// negative
				if( mode == IM_90_45 )
					p.y = pi.y + vert;
				else if( mode == IM_45_90 )
				{
					p.y = pf.y - vert;
					p.x = pf.x;
				}
			}
			else
				ASSERT(0);
		}
		else
		{
			// horizontal > vertical
			if( mode == IM_90 )
			{
				p.x = pf.x;
				p.y = pi.y;
			}
			else if( mode == IM_45_90 || mode == IM_90_45 )
			{
				int hor;	// length of horizontal line needed
				if( dx > 0 )
					hor = dx - abs(dy);	// positive	
				else
					hor = dx + abs(dy);	// negative
				if( mode == IM_90_45 )
					p.x = pi.x + hor;
				else if( mode == IM_45_90 )
				{
					p.x = pf.x - hor;
					p.y = pf.y;
				}
			}
			else
				ASSERT(0);
		}
	}
	return p;
}

BYTE getbit( int rgstr, int bit )
{
	BYTE R=1;
	rgstr >>= bit;
	_asm{ 
		BT rgstr,0
		jb goodbye
		mov R,0 
		goodbye: nop
		}	
	return R;
}
// 
// function to rotate a point clockwise about another point
// currently
//
void RotatePoint( CPoint *p, float angle, CPoint org ) 
{
	if (angle < BY_ZERO && angle > -BY_ZERO)
		return;
	float 
		x=(*p).x-org.x,
		y=(*p).y-org.y;
	Rotate_Vertex(&x,&y,angle);
	(*p).x = x + org.x;
	(*p).y = y + org.y;
}

void RotatePOINTS( CPoint * p, int np, float angle, CPoint org ) 
{
	if (angle < BY_ZERO && angle > -BY_ZERO)
		return;
	for( int i=0; i<np; i++ )
	{
		float x=p[i].x-org.x,
		      y=p[i].y-org.y;
		Rotate_Vertex(&x,&y,angle);
		p[i].x = x + org.x;
		p[i].y = y + org.y;
	}
}

float Angle ( CPoint pv, CPoint v, CPoint nv )
{
	float an1 = Angle(v.x,v.y,pv.x,pv.y);
	float an2 = Angle(nv.x,nv.y,v.x,v.y);
	float d_an = an2 - an1;
	if( d_an < -180.0 )
		d_an += 360.0;
	else if( d_an > 180.0 )
		d_an -= 360.0;
	return d_an;
}

int FindArcElements ( cconnect * c, int * i )
{
	if( *i >= c->nsegs )
		return 0;
	if( *i < 0 )
		return 0;
	if( *i == 0 )
		(*i)++;
	CPoint pv, v, nv;
	int i_start=-1, i_end=-1, ang_start=0, layer_start=0;
	for( int ii=*i; ii<c->nsegs; ii++ )
	{
		pv.x = c->vtx[ii-1].x;
		pv.y = c->vtx[ii-1].y;
		v.x = c->vtx[ii].x;
		v.y = c->vtx[ii].y;
		nv.x = c->vtx[ii+1].x;
		nv.y = c->vtx[ii+1].y;
		float dang = Angle(pv,v,nv);
		if( i_start == -1 )
			if( abs(dang) < 44.0 )
				if( Check_45(pv,v) || Check_90(pv,v) )
					if( !Check_45(v,nv) && !Check_90(v,nv) )
					{
						i_start = ii;
						ang_start = dang;
						layer_start = c->seg[ii].layer;
					}

		// first seg...
		if( i_start == ii )
		{
		}
		else if( i_start >= 0 ) // arc segs...
		{
			if( dang > 0 && ang_start < 0 )
				i_start = -1; // fail
			if( dang < 0 && ang_start > 0 ) 
				i_start = -1; // fail
			if( abs(dang) > 44.0 )
				i_start = -1; // fail
			if( i_start >= 0 )
				if( !Check_45(pv,v) && !Check_90(pv,v) )
					if( Check_45(v,nv) || Check_90(v,nv) )
					{
						i_end = ii-1;
						break;
					}
			int d1 = Distance( pv.x,pv.y,v.x,v.y );
			int d2 = Distance( v.x,v.y,nv.x,nv.y );
			if( abs( d1-d2 ) > _2540 )
				i_start = -1; // fail
			if( layer_start != c->seg[ii].layer )
				i_start = -1; // fail
		}
	}
	if( i_start >= 0 && i_end >= 0 )
	{
		*i = i_start;
		return i_end-i_start+1;
	}
	return 0;
}


int FindArcElements ( CPolyLine * p, int * i, BOOL * EndFlag )
{
	if( *i >= p->GetNumCorners() )
		return 0;
	if( *i < 0 )
		return 0;
	*EndFlag = FALSE;
	int nc = p->GetNumContour(*i);
	int cont_st = p->GetContourStart(nc);
	int cont_end = p->GetContourEnd(nc);
	CPoint pv, v, nv;
	int i_start=-1, i_end=-1, ang_start;
	while(1)
	{
		for( int ii=*i; ii<=cont_end; ii++ )
		{
			pv.x = p->GetX( p->GetIndexCornerBack(ii) );
			pv.y = p->GetY( p->GetIndexCornerBack(ii) );
			v.x = p->GetX( ii );
			v.y = p->GetY( ii );
			nv.x = p->GetX( p->GetIndexCornerNext(ii) );
			nv.y = p->GetY( p->GetIndexCornerNext(ii) );
			float dang = Angle(pv,v,nv);
			if( i_start == -1 )
				if( abs(dang) < 44.0 )
					if( Check_45(pv,v) || Check_90(pv,v) )
						if( !Check_45(v,nv) && !Check_90(v,nv) )
						{
							i_start = ii;
							ang_start = dang;
						}
	
			// first seg...
			if( i_start == ii )
			{
			}
			else if( i_start >= 0 ) // arc segs...
			{
				if( (dang > 0 && ang_start < 0) || (dang < 0 && ang_start > 0) || (abs(dang) > 44.0) )
				{
					i_start = -1; // fail
					if( *EndFlag )
						break;
				}
				if( i_start >= 0 )
					if( !Check_45(pv,v) && !Check_90(pv,v) )
						if( Check_45(v,nv) || Check_90(v,nv) )
						{
							i_end = p->GetIndexCornerBack(ii);
							break;
						}
				int d1 = Distance( pv.x,pv.y,v.x,v.y );
				int d2 = Distance( v.x,v.y,nv.x,nv.y );
				if( abs( d1-d2 ) > _2540*4 )
				{
					i_start = -1; // fail
					if( *EndFlag )
						break;
				}
			}
		}
		if( i_start >= 0 && i_end == -1 && !*EndFlag )
		{
			*EndFlag = TRUE;
			*i = cont_st;
		}
		else
			break;
	}
	if( i_start >= 0 && i_end >= 0 )
	{
		*i = i_start;
		return i_end-i_start+1;
	}
	return 0;
}

BOOL Check_45 ( CPoint pv, CPoint v )
{
	float an = Angle(v.x,v.y,pv.x,pv.y,FALSE);
	an = abs(an);
	if( abs(an-45.0) < 1.5 )
		return TRUE;
	if( abs(an-135.0) < 1.5 )
		return TRUE;
	return FALSE;
}

BOOL Check_90 ( CPoint pv, CPoint v )
{
	float an = Angle(v.x,v.y,pv.x,pv.y,FALSE);
	an = abs(an);
	if( an < 1.5 || an > 178.5 )
		return TRUE;
	if( abs(an-90.0) < 1.5 )
		return TRUE;
	return FALSE;
}

BOOL Check_45 ( CPoint pv, CPoint v, CPoint nv )
{
	return FALSE;
}

BOOL Check_90 ( CPoint pv, CPoint v, CPoint nv )
{
	return FALSE;
}

// from 'Copper Areas Splitter'
float Angle (float dx, float dy, float x0, float y0, BOOL b_0_360 )
    {
    float ANG_s=0;
    dx = dx - x0;
    dy = dy - y0;
    if (abs(dx) < BY_ZERO)
            {
            if (dy > BY_ZERO)       
				ANG_s = 90.0;
            else if (dy < -BY_ZERO) 
				ANG_s = 270.0;
            }
    else
            {
            double Dxy = dy/dx;
            ANG_s = (atan(Dxy))*(double)180.0/(double)M_PI;
            if ( dx > 0 && dy < 0 ) 
				ANG_s = 360.0 + ANG_s;
            if (dx < 0) 
				ANG_s = 180.0 + ANG_s;
            }
	if( b_0_360 )
		return ANG_s;
	else if( ANG_s > 180.0 )
		ANG_s -= 360.0;
    return ANG_s;
    }


void Rotate_i_Vertex (int *X, int *Y, int Ang, int orgx, int orgy )
{
	float fx = *X - orgx; 
	float fy = *Y - orgy; 
	Rotate_Vertex(&fx, &fy, Ang);
	*X = fx + orgx;
	*Y = fy + orgy;
}


// from 'Copper Areas Splitter'
void Rotate_Vertex (float *X, float *Y, float Ang)
{
float start_Ang=0, end_Ang;
double radius = 0;
if (abs(*X) < BY_ZERO)
        {
        if (*Y > 0) 
			start_Ang = 90;
        else if (*Y < 0) 
			start_Ang = 270;
        }
else
        {
        start_Ang = (atan(*Y/(*X)))*180.0/M_PI;
        if ((*X > 0)&&(*Y < 0)) 
			start_Ang = 360.0 + start_Ang;
        if (*X < 0) 
			start_Ang = 180.0 + start_Ang;
        }
end_Ang = start_Ang + Ang;
double powx = (*X)*(*X);
double powy = (*Y)*(*Y);
if (powx > BY_ZERO || powy > BY_ZERO)   
	radius = sqrt(powx + powy);
else                                    
	radius = powx + powy;
*Y = radius*sin(end_Ang*M_PI/180.0);
*X = radius*cos(end_Ang*M_PI/180.0);
}


// Colinear sides
BOOL Colinear( int x1, int y1, int x2, int y2, int x3, int y3 )
{
	long long dx1 = x2 - x1;
	long long dy1 = y2 - y1;
	long long dx2 = x3 - x2;
	long long dy2 = y3 - y2;
	if( abs(dx1) < _2540 && abs(dx2) < _2540 && dy1*dy2 > 0 )
		return TRUE;
	if( abs(dy1) < _2540 && abs(dy2) < _2540 && dx1*dx2 > 0 )
		return TRUE;
	if( dx1*dx2 > 0 && dy1*dy2 > 0 )
	{
		double d1 = (float)dx1/(float)dy1;
		double d2 = (float)dx2/(float)dy2;
		if( abs(d1/d2-1) < 0.01 )
			return TRUE;
	}
	return FALSE;
}


CPoint AlignPoints (CPoint p, CPoint pback, CPoint pnext, BOOL CH, float SwitchAngle)
{
	CPoint BestPoint;
	float	
		min_len = DEFAULT,
		px, py, 
		bx, by,
		nx, ny,
		dx, dy,
		BestAngle = 0,
		pnewx, 
		pnewy,
		len;
	// test on colinear
	px = p.x; 
	py = p.y; 
	bx = pback.x;
	by = pback.y;
	nx = pnext.x;
	ny = pnext.y;
	float b_seg_angle = Angle( bx, by, px, py );
	float n_seg_angle = Angle( nx, ny, px, py );
	float ABS = abs(n_seg_angle-b_seg_angle);
	if (( ABS > 353.0 /*degree*/ )
	|| ( ABS > 173.0 && ABS < 187.0 )
	|| ( ABS < 7.0 ))
	{
		float seg_angle = Angle( nx, ny, bx, by );
		Rotate_Vertex(&bx,&by,-seg_angle);
		Rotate_Vertex(&nx,&ny,-seg_angle);
		Rotate_Vertex(&px,&py,-seg_angle);
		pnewx = px;
		pnewy = by;
		Rotate_Vertex(&pnewx,&pnewy,seg_angle);
		BestPoint.x = pnewx;
		BestPoint.y = pnewy;
		return BestPoint; // STRAIGHT
	}
	// end test
	if (SwitchAngle < 7.5)
		SwitchAngle = 7.5;
	if (SwitchAngle > 45.0)
		SwitchAngle = 45.0;
	for (double rot=0.0; rot<89.9; rot += SwitchAngle)
	{
		px = p.x; 
		py = p.y; 
		bx = pback.x;
		by = pback.y;
		nx = pnext.x;
		ny = pnext.y;
		if (rot > BY_ZERO)
		{
			Rotate_Vertex(&px,&py,rot);
			Rotate_Vertex(&bx,&by,rot);
			Rotate_Vertex(&nx,&ny,rot); 
		}
		dx = (nx - bx);
		dy = (ny - by);
		for (double an=SwitchAngle; an<179.9; an += SwitchAngle)
		{
			for (int step=0; step<4; step++)
			{
				switch(step)
				{
				case 0:
					pnewx = nx;
					pnewy = by + dx/sin(an*M_PI/180.0)*cos(an*M_PI/180.0);
					break;	
				case 1:
					pnewx = bx;
					pnewy = ny - dx/tan(an*M_PI/180.0);
					break;	
				case 2:
					pnewy = ny;
					pnewx = bx + dy/tan(an*M_PI/180.0);
					break;
				case 3:
					pnewy = by;
					pnewx = nx - dy/tan(an*M_PI/180.0);
					break;	 
				}
				len = (pnewx - px)*(pnewx - px) + (pnewy - py)*(pnewy - py);
				len = sqrt(len);
				if ( len < min_len )
				{
					min_len = len;
					BestPoint.x = (int)pnewx;
					BestPoint.y = (int)pnewy;
					BestAngle = rot;
				}
			}
		}
	}
	if (BestAngle > BY_ZERO)
	{
		float
			x = BestPoint.x,
			y = BestPoint.y;
		Rotate_Vertex(&x,&y,-BestAngle);
		BestPoint.x = (int)x;
		BestPoint.y = (int)y;
	}
	if (min_len < _2540 && CH)
	{	// mirror vertex
		bx = pback.x;
		by = pback.y;
		nx = pnext.x;
		ny = pnext.y;
		double ang = Angle(nx, ny, bx, by);
		float x = BestPoint.x;
		float y = BestPoint.y;
		Rotate_Vertex(&bx, &by, -ang);
		Rotate_Vertex(&x, &y, -ang);
		Rotate_Vertex(&nx, &ny, -ang);
		int cx = (bx+nx)/2;
		int cy = (by+ny)/2;
		x -= cx;
		y -= cy;
		x = -x;
		y = -y;
		x += cx;
		y += cy;
		Rotate_Vertex(&x, &y, ang);
		BestPoint.x = (int)x;
		BestPoint.y = (int)y;
	}
	return BestPoint;
}

int TripplePointArc (int xi, int yi, int xf, int yf, int xc, int yc, CPoint * OutPut, int npoints )
{
	double s_an, e_an, inc_an, rad;
	s_an = Angle(xi,yi,xc,yc);
	e_an = Angle(xf,yf,xc,yc);
	if( e_an < s_an )
		e_an += 360.0;
	rad = double(xi-xc)*double(xi-xc) + double(yi-yc)*double(yi-yc);
	rad = sqrt(rad);
	inc_an = (e_an-s_an)/((double)npoints-1.0);
	for(int np=0; np<npoints; np++)
	{
		OutPut[np].x = xc + rad*cos(s_an*M_PI/180.0);
		OutPut[np].y = yc + rad*sin(s_an*M_PI/180.0);
		s_an += inc_an;
	}
	return npoints;
}

int Gen_HollowLinePoly (int xi, int yi, int xf, int yf, int w, CPoint * OutPut, int npoints )
{
	if( npoints%2 )
		return 0;
	const double pi = 3.14159265359;
	float angle = (Angle(xf,yf,xi,yi)-90.0)*pi/180.0;
	float inc = ((float)npoints-4.0)/2.0;
	for( int iv=0; iv<npoints-2; iv++ )
	{
		if ( iv < npoints/2-1 )
		{
			OutPut[iv].x = xf + 0.5 * w * cos(angle);
			OutPut[iv].y = yf + 0.5 * w * sin(angle);
		}
		else if ( iv == npoints/2-1 )
		{
			angle -= pi/inc;
			OutPut[iv].x = xi + 0.5 * w * cos(angle);
			OutPut[iv].y = yi + 0.5 * w * sin(angle);
			//continue;
		}
		else 
		{
			OutPut[iv].x = xi + 0.5 * w * cos(angle);
			OutPut[iv].y = yi + 0.5 * w * sin(angle);
		}
		angle += pi/inc;
	}
	OutPut[npoints-2].x = OutPut[0].x;
	OutPut[npoints-2].y = OutPut[0].y;
	return npoints-1;
}

int Gen_RndRectPoly (int x, int y, int dx, int dy, float rad, float ang, CPoint * OutPut, int npoints )
{
	if( npoints%4 )
		return 0;
	double angle = 0.0;
	double angle_step = M_PI/((npoints-4)/2);
	int npo = 0;
	if( rad > 2 )
		for( int i=0; i<(npoints/4-1); i++ )
		{
			OutPut[npo].x = + (float)dx/2.0 - rad + rad*cos(angle);
			OutPut[npo].y = + (float)dy/2.0 - rad + rad*sin(angle);
			npo++;
			angle += angle_step;
		}
	OutPut[npo].x = + (float)dx/2.0 - rad;
	OutPut[npo].y = + (float)dy/2.0;
	npo++;
	if( rad > 2 )
		for( int i=0; i<(npoints/4-1); i++ )
		{
			OutPut[npo].x = - (float)dx/2.0 + rad + rad*cos(angle);
			OutPut[npo].y = + (float)dy/2.0 - rad + rad*sin(angle);
			npo++;
			angle += angle_step;
		}
	OutPut[npo].x = - (float)dx/2.0;
	OutPut[npo].y = + (float)dy/2.0 - rad;
	npo++;
	if( rad > 2 )
		for( int i=0; i<(npoints/4-1); i++ )
		{
			OutPut[npo].x = - (float)dx/2.0 + rad + rad*cos(angle);
			OutPut[npo].y = - (float)dy/2.0 + rad + rad*sin(angle);
			npo++;
			angle += angle_step;
		}
	OutPut[npo].x = - (float)dx/2.0 + rad;
	OutPut[npo].y = - (float)dy/2.0;
	npo++;
	// 4
	if( rad > 2 )
		for( int i=0; i<(npoints/4-1); i++ )
		{
			OutPut[npo].x = + (float)dx/2.0 - rad + rad*cos(angle);
			OutPut[npo].y = - (float)dy/2.0 + rad + rad*sin(angle);
			npo++;
			angle += angle_step;
		}
	OutPut[npo].x = + (float)dx/2.0;
	OutPut[npo].y = - (float)dy/2.0 + rad;
	npo++;
	if( ang )
		RotatePOINTS( OutPut, npo, ang, zero );
	for( int i=0; i<npo; i++ )
	{
		OutPut[i].x += x;
		OutPut[i].y += y;
	}
	return npo;
}

// from 'ExportDXF'
int Generate_Arc (int xi, int yi, int xf, int yf,  int type_L, CPoint * OutPut, int n_sides )
{
long  lines = 0;
if (type_L > 2) type_L = 0;
if (type_L == 0 || ( abs(xi - xf) < BY_ZERO || abs(yi - yf) < BY_ZERO ))
        { //прямая линия
			OutPut[lines].x = xi;
			OutPut[lines].y = yi;
			lines++;
			OutPut[lines].x = xf;
			OutPut[lines].y = yf;
			lines++;
        }
else    { // дуга
        float xxi, xxf, yyi, yyf;
		if( type_L == CPolyLine::ARC_CCW )
		{
			xxi = xf;
			xxf = xi;
			yyi = yf;
			yyf = yi;
		}
		else
		{
			xxi = xi;
			xxf = xf;
			yyi = yi;
			yyf = yf;
	    }
			// find center and radii of ellipse
		float xo, yo, rx, ry;
		if( xxf > xxi && yyf > yyi )
		{
			xo = xxf;
			yo = yyi;
		}
		else if( xxf < xxi && yyf > yyi )
		{
			xo = xxi;
			yo = yyf;
		}
		else if( xxf < xxi && yyf < yyi )
		{
			xo = xxf;
			yo = yyi;
		}
		else if( xxf > xxi && yyf < yyi )
		{
			xo = xxi;
			yo = yyf;
		}
		else
		{
			OutPut[lines].x = xi;
			OutPut[lines].y = yi;
			lines++;
			OutPut[lines].x = xf;
			OutPut[lines].y = yf;
			lines++;
			return 2;
		}
		rx = abs( (float)(xxi-xxf) );
		ry = abs( (float)(yyi-yyf) );
		float k = rx/ry;
        OutPut[lines].x = xi;
        OutPut[lines].y = yi;
        lines++;
        xi = xi - xo;
        yi = yi - yo;
        xi = xi/k;
		if( n_sides < 2 )
			ASSERT(0);
		double cur_an = 0.0;
        for (int step=0; step<n_sides; step++)
                {
				xxi = xi;
				yyi = yi;
                if (type_L == 1)
					cur_an -= (double)90.0/(double)n_sides;
                if (type_L == 2)
					cur_an += (double)90.0/(double)n_sides;
				Rotate_Vertex (&xxi, &yyi, cur_an);
                xxi = xxi*k + xo;
                yyi = yyi + yo;
                OutPut[lines].x = xxi;
                OutPut[lines].y = yyi;
                lines++;
                } // while (ang < ang_end)
        }
return lines;
}

// from 'RoundingRect'
int Rnd_Func(   float xn,
                float yn,
                float x2,
                float y2,
                float x1,
                float y1,
				float xb,
                float yb,
                float * PTS,
				int PtsMaxValue)
{
 float ab = Angle (x1, y1, xb, yb);
 float an = Angle (x2, y2, xn, yn);
 float a12 = Angle (x2, y2, x1, y1);
 float an_bt = an - ab;
 if( an_bt > 180.0 )
	 an_bt -= 360.0;
 else if( an_bt < -180.0 )
	 an_bt += 360.0;
 if( abs(an_bt) > 165.0 )
	 return 0;
 float dist = Distance(x1,y1,x2,y2);
 float Rad = dist/2.0/cos(an_bt*M_PI/360.0);
 float H = Rad*sin(an_bt*M_PI/360.0);
 float xc,yc;
 float xc_ = ((float)x1 + (float)x2)/2.0;
 float yc_ = ((float)y1 + (float)y2)/2.0;

 xc = xc_ + H*cos((a12-90.0)*M_PI/180.0);
 yc = yc_ + H*sin((a12-90.0)*M_PI/180.0);
 float start = Angle(x2,y2,xc,yc);
 float step;
 int CountNew = PtsMaxValue;
 float rxb = xb;
 float ryb = yb;
 float rx1 = x1;
 float ry1 = y1;
 float rx2 = x2;
 float ry2 = y2;
 float rxc = xc;
 float ryc = yc;
 Rotate_Vertex( &rxb, &ryb, -a12 );
 Rotate_Vertex( &rx1, &ry1, -a12 );
 Rotate_Vertex( &rx2, &ry2, -a12 );
 Rotate_Vertex( &rxc, &ryc, -a12 );
 BOOL external = (( ryb<ry1 && ryc>ry1 ) || ( ryb>ry1 && ryc<ry1 ));
//
	if( external )
		step = (180.0 + abs(an_bt))/((float)CountNew/2.0-1.0);
	else
	{
		if (abs(an_bt) > 130.0 )
			CountNew = PtsMaxValue/3;
		else if (abs(an_bt) > 85.0)
			CountNew = PtsMaxValue/2;
		step = (180.0 - abs(an_bt))/((float)CountNew/2.0-1.0);
	}
 float x2_p = abs(Rad)*cos((start+step)*M_PI/180.0) + xc;
 float y2_p = abs(Rad)*sin((start+step)*M_PI/180.0) + yc;
 float x2_m = abs(Rad)*cos((start-step)*M_PI/180.0) + xc;
 float y2_m = abs(Rad)*sin((start-step)*M_PI/180.0) + yc;
 float dm, dp;
 dm = Distance(x2_m,y2_m,xn,yn);
 dp = Distance(x2_p,y2_p,xn,yn);
 for (int i=0; i<CountNew; i=i+2)
    {
	 *(PTS+i) = abs(Rad)*cos(start*M_PI/180.0) + xc;
     *(PTS+i+1) = abs(Rad)*sin(start*M_PI/180.0) + yc;
	 if( dp > dm )
		start += step;
	 else
		start -= step;
    }
 return CountNew - (CountNew%2);
}



// function to mirror a rectangle
// on exit, r->top > r.bottom, r.right > r.left
void MirrorRect( RECT * rect, CPoint org )
{
	int l,r,t,b;
	l = -(*rect).right;
	r = -(*rect).left;
	t = -(*rect).top;
	b = -(*rect).bottom;
	(*rect).right = r;
	(*rect).left = l;
	(*rect).top = t;
	(*rect).bottom = b;
}


// function to rotate a rectangle clockwise about a point
// on exit, r->top > r.bottom, r.right > r.left
//
void RotateRect( int * left, int * right, int * bottom, int * top, int angle, int orgX, int orgY )
{
	RECT r;
	r.left = *left;
	r.right = *right;
	r.bottom = *bottom;
	r.top = *top;
	CPoint org( orgX, orgY);
	RotateRect( &r, angle, org );
	*left = r.left;
	*right = r.right;
	*bottom = r.bottom;
	*top = r.top;
}
void RotateRect( RECT *r, int angle, CPoint org )
{
	RECT tr;
	float x,y;
	tr.left =	INT_MAX;
	tr.bottom = INT_MAX;
	tr.right =	INT_MIN;
	tr.top =	INT_MIN;
	(*r).left =   (*r).left - org.x;
	(*r).right =  (*r).right - org.x;
	(*r).top =    (*r).top - org.y;
	(*r).bottom = (*r).bottom - org.y;
	for (int step=0;step<4; step++)
	{
		if( step == 0 )
		{
			x = (*r).left;
			y = (*r).bottom;
		}
		else if( step == 1 )
		{
			x = (*r).right;
			y = (*r).top;
		}
		else if( step == 2 )
		{
			x = (*r).left;
			y = (*r).top;
		}
		else if( step == 3 )
		{
			x = (*r).right;
			y = (*r).bottom;
		}
		Rotate_Vertex(&x,&y,-angle);
		tr.left =	min (tr.left,x);
		tr.bottom = min (tr.bottom,y);
		tr.right =	max (tr.right,x);
		tr.top =	max (tr.top,y);
	}
	*r = tr;
	(*r).left =   (*r).left + org.x;
	(*r).right =  (*r).right + org.x;
	(*r).top =    (*r).top + org.y;
	(*r).bottom = (*r).bottom + org.y;
}

// тест прямоугольников
// возвращает -1 если прямоугольники не пересекаются
// возвращает +1 если один внутри другого
// возвращает 0 если стороны пересекаются
BOOL RectsIntersection ( RECT r1, RECT r2 )
{
	if (r1.left > r2.right ||
		r2.left > r1.right ||
		r1.bottom > r2.top ||
		r2.bottom > r1.top )
		return -1;
	else if (r1.left > r2.left &&
			r1.right < r2.right &&
			r1.bottom > r2.bottom &&
			r1.top < r2.top )
		return 1;
	else if (r2.left > r1.left &&
			r2.right < r1.right &&
			r2.bottom > r1.bottom &&
			r2.top < r1.top )
		return 1;
	else
		return 0;
}


// раздувает прямоугольник на величину swValue
void SwellRect ( RECT *r, int swValue )
{
	r->left	  =	r->left		- swValue;
	r->right  =	r->right	+ swValue;
	r->top	  =	r->top		+ swValue;
	r->bottom = r->bottom	- swValue;
}

void SwellRect ( RECT *r, RECT swell )
{
	r->left		= min(swell.left,	r->left);
	r->right	= max(swell.right,	r->right);
	r->top		= max(swell.top,	r->top);
	r->bottom	= min(swell.bottom,	r->bottom);
}

void SwellRect ( RECT *r, CPoint Po )
{
	r->left		= min(Po.x,	r->left);
	r->right	= max(Po.x,	r->right);
	r->top		= max(Po.y,	r->top);
	r->bottom	= min(Po.y,	r->bottom);
}

void SwellRect ( RECT *r, int x, int y )
{
	r->left		= min(x,r->left);
	r->right	= max(x,r->right);
	r->top		= max(y,r->top);
	r->bottom	= min(y,r->bottom);
}

void MoveRect( RECT *r, int dx, int dy )
{
	r->left		= r->left  +dx;
	r->right	= r->right +dx;
	r->top		= r->top	   +dy;
	r->bottom	= r->bottom+dy;
}

RECT rect(int xi, int yi, int xf, int yf)
{
	RECT r;
	r.left		= min(xi,xf);
	r.right		= max(xi,xf);
	r.top		= max(yi,yf);
	r.bottom	= min(yi,yf);
	return r;
}


int SwellPolygon( CPoint * P, int np, CPoint * newP, int swValue )
{
	if( np < 3 )
		ASSERT(0);
	enum{
		dCW = 1,
		dCCW
	};
	int pDir, iout=0;
	for( int i=0; i<np; i++ )
	{
		if( pDir = TestPolygon( P[i].x+99, P[i].y+99, P, np )) 
			break;
		if( pDir = TestPolygon( P[i].x+99, P[i].y, P, np )) 
			break;
	}
	const double pi = 3.14159265359;
	int iprev, ipost;
	for( int i=0; i<np; i++ )
	{
		if( i != np-1 )
			ipost = i+1;
		else
			ipost = 0;
		if( Distance( P[i].x, P[i].y, P[ipost].x, P[ipost].y ) < m_pcbu_per_wu )
			continue;
		iprev = i;
		for( int ii=i-1; ii!=i; ii-- )
		{
			if( ii < 0 )
				ii = np-1;
			if( Distance( P[ii].x, P[ii].y, P[i].x, P[i].y ) > m_pcbu_per_wu )
			{
				iprev = ii;
				break;
			}
		}
		if( iprev == i )
			break;
		double ang_btwn = 180.0 - abs(Angle( P[iprev], P[i], P[ipost] ));
		double ang_1 = Angle( P[iprev].x, P[iprev].y, P[i].x, P[i].y );
		if( ang_btwn > 100.0 )
		{
			double ang_new;
			if( pDir == dCW )
				ang_new = ang_1 - (360.0-ang_btwn)/2;
			else
				ang_new = ang_1 + (360.0-ang_btwn)/2;
			double len = swValue/sin(ang_btwn*pi/360.0);
			double xnew = (double)P[i].x + len*cos(ang_new*pi/180.0);
			double ynew = (double)P[i].y + len*sin(ang_new*pi/180.0);
			if( newP )
			{
				newP[iout].x = xnew;
				newP[iout].y = ynew;		
			}
			iout++;
		}
		else
		{
			double ang_2 = Angle( P[ipost].x, P[ipost].y, P[i].x, P[i].y );
			double angST_new, angEND_new;
			if ( pDir == dCW )
				angST_new = ang_1 - 90.0;
			else
				angST_new = ang_1 + 90.0;
			if ( pDir == dCW )
				angEND_new = ang_2 + 90.0;
			else
				angEND_new = ang_2 - 90.0;
			CPoint pSTART, pEND;
			pSTART.x = (double)P[i].x + (double)swValue*cos(angST_new*pi/180.0);
			pSTART.y = (double)P[i].y + (double)swValue*sin(angST_new*pi/180.0);
			pEND.x = (double)P[i].x + (double)swValue*cos(angEND_new*pi/180.0);
			pEND.y = (double)P[i].y + (double)swValue*sin(angEND_new*pi/180.0);
			int n_sides;
			if( ang_btwn < 45.0 )
				n_sides = 20;
			else if( ang_btwn < 90.0 )
				n_sides = 15;
			else
				n_sides = N_SIDES_APPROX_ARC;
			CPoint * pARC = new CPoint[n_sides];//ok
			if( pDir == dCW )
			{
				int arc = TripplePointArc( pEND.x, pEND.y, pSTART.x, pSTART.y, P[i].x, P[i].y, pARC, n_sides-1 );	
				for( int n=arc-1; n>=0; n-- )
				{
					if( newP )
					{
						newP[iout].x = pARC[n].x;
						newP[iout].y = pARC[n].y;
					}
					iout++;
				}
			}
			else
			{
				int arc = TripplePointArc( pSTART.x, pSTART.y, pEND.x, pEND.y, P[i].x, P[i].y, pARC, n_sides-1 );	
				for( int n=0; n<arc; n++ )
				{
					if( newP )
					{
						newP[iout].x = pARC[n].x;
						newP[iout].y = pARC[n].y;
					}
					iout++;
				}
			}
			delete pARC;
		}	
	}
	return iout;
}

int TestPolygon( int X, int Y, CPoint * P, int np )
{
	enum{
		dir___CW=1,
		dir___CCW
	};
	double st_angle =   Angle(P[0].x, P[0].y, X, Y);
	double back_angle = st_angle;
	double diff_angle;
	double angle,d;
	double SUM = 0;
	for (int q=0; q<np; q++)
    {
        d = Distance( P[q].x, P[q].y, X, Y );
        if (d < BY_ZERO) return 0;
        angle = Angle(P[q].x, P[q].y, X, Y);
        diff_angle = angle - back_angle;
        if      (diff_angle < -180.0) diff_angle += 360.0;
        else if (diff_angle > 180.0)  diff_angle -= 360.0;
        SUM += diff_angle;
        back_angle = angle;
    }
	diff_angle = st_angle - back_angle;
	if      (diff_angle < -180.0) diff_angle += 360.0;
	else if (diff_angle > 180.0)  diff_angle -= 360.0;
	SUM += diff_angle;
	if      (SUM < -359)    return dir___CW;
	else if (SUM > 359)     return dir___CCW;
	return 0;
}


// test for hit on line segment
// i.e. cursor within a given distance from segment
// enter with:	x,y = cursor coords
//				(xi,yi) and (xf,yf) are the end-points of the line segment
//				dist = maximum distance for hit
//
int TestLineHit( int xi, int yi, int xf, int yf, int x, int y, double dist )
{
	double dd;

	// test for vertical or horizontal segment
	if( xf==xi )
	{
		// vertical segment
		dd = fabs( (double)(x-xi) );
		if( dd<dist && ( (yf>yi && y<yf && y>yi) || (yf<yi && y>yf && y<yi) ) )
			return 1;
	}
	else if( yf==yi )
	{
		// horizontal segment
		dd = fabs( (double)(y-yi) );
		if( dd<dist && ( (xf>xi && x<xf && x>xi) || (xf<xi && x>xf && x<xi) ) )
			return 1;
	}
	else
	{
		// oblique segment
		// find a,b such that (xi,yi) and (xf,yf) lie on y = a + bx
		double b = (double)(yf-yi)/(xf-xi);
		double a = (double)yi-b*xi;
		// find c,d such that (x,y) lies on y = c + dx where d=(-1/b)
		double d = -1.0/b;
		double c = (double)y-d*x;
		// find nearest point to (x,y) on line segment (xi,yi) to (xf,yf)
		double xp = (a-c)/(d-b);
		double yp = a + b*xp;
		// find distance
		dd = sqrt((x-xp)*(x-xp)+(y-yp)*(y-yp));
		if( fabs(b)>0.7 )
		{
			// line segment more vertical than horizontal
			if( dd<dist && ( (yf>yi && yp<yf && yp>yi) || (yf<yi && yp>yf && yp<yi) ) )
				return 1;
		}
		else
		{
			// line segment more horizontal than vertical
			if( dd<dist && ( (xf>xi && xp<xf && xp>xi) || (xf<xi && xp>xf && xp<xi) ) )
				return 1;
		}
	}	
	return 0;	// no hit
}

// function to read font file
// format is form "default.fnt" file from UnixPCB
//
// enter with:	fn = filename
//							
// return pointer to struct font, or 0 if error
//
int ReadFontFile( char * fn )
{
	return 0;
}

// safer version of strtok which buffers input string
// limited to strings < 256 characters
// returns pointer to substring if delimiter found
// returns 0 if delimiter not found
//
char * mystrtok( LPCTSTR str, LPCTSTR delim )
{
	static char s[256] = "";
	static int pos = 0;
	static int len = 0;
	int delim_len = strlen( delim ); 

	if( str )
	{
		len = strlen(str);
		if( len > 255 || len == 0 || delim_len == 0 )
		{
			len = 0;
			return 0;
		}
		strcpy( s, str );
		pos = 0;
	}
	else if( len == 0 )
		return 0;

	// now find delimiter, starting from pos
	int i = pos;
	while( i<=len )
	{
		for( int id=0; id<delim_len; id++ )
		{
			if( s[i] == delim[id] || s[i] == 0 )
			{
				// found delimiter, update pos and return
				int old_pos = pos;
				pos = i+1;
				s[i] = 0;
				return &s[old_pos]; 
			}
		}
		i++;
	}
	return 0;
}

// function to get dimension in PCB units from string
// string format:	nnn.nnMIL for mils
//					nnn.nnMM for mm.
//					nnnnnnNM for nm.
//					nnnn for default units
// if bRound10 = TRUE, round return value to nearest 10 nm.
// returns the dimension in nanometers, or 0.0 if error
//
double GetDimensionFromString( CString * str, int def_units, BOOL bRound10 )
{
	double dim;
	int mult;

	if( def_units == MM )
		mult = NM_PER_MM;
	else if( def_units == MIL )
		mult = NM_PER_MIL;
	else if( def_units == NM )
		mult = 1;

	int len = str->GetLength();
	if( len > 2 )
	{
		if( str->Right(2) == "MM" )
			mult = NM_PER_MM;
		else if( str->Right(3) == "MIL" )
			mult = NM_PER_MIL;
		else if( str->Right(2) == "NM" )
			mult = 1;
	}
	dim = mult*atof( (LPCSTR)str->GetBuffer() );
	if( bRound10 )
	{
		long ldim;
		if( dim >= 0.0 )
			ldim = dim + 5.0;
		else
			ldim = dim - 5.0;
		ldim = ldim/10 * 10;
		dim = ldim;
	}
	return dim;
}

// function to make string from dimension in NM, using requested units
// if append_units == TRUE, add unit string, like "10MIL"
// if lower_case == TRUE, use lower case for units, like "10mil"
// if space == TRUE, insert space, like "10 mil"
// max_dp is the maximum number of decimal places to include in string
// if strip = TRUE, strip trailing zeros if decimal point
//
void MakeCStringFromDimension( CString * str, int dim, int units, BOOL append_units, 
							  BOOL lower_case, BOOL space, int max_dp, BOOL strip )
{
	CString f_str;
	f_str.Format( "11.%df", max_dp );
	f_str = "%" + f_str;
	if( units == MM )
		str->Format( f_str, (double)dim/1000000.0 );
	else if( units == MIL )
		str->Format( f_str, (double)dim/NM_PER_MIL );
	else if( units == NM )
		str->Format( "%d", dim );
	else
		ASSERT(0);
	str->Trim();

	// look for decimal point
	str->Trim();
	int dp_pos = str->Find( "." );

	// if decimal point, strip trailing zeros from MIL and MM strings
	if( dp_pos != -1 && strip )
	{
		while(1 )
		{
			if( str->Right(1) == "0" )
				*str = str->Left( str->GetLength() - 1 );
			else if( str->Right(1) == "." )
			{
				*str = str->Left( str->GetLength() - 1 );
				break;
			}
			else
				break;
		}
	}

	// append units if requested
	if( append_units )
	{
		if( units == MM && space == FALSE )
			*str = *str + "MM";
		else if( units == MM && space == TRUE )
			*str = *str + " MM";
		else if( units == MIL && space == FALSE )
			*str = *str + "MIL";
		else if( units == MIL && space == TRUE )
			*str = *str + " MIL";
		else if( units == NM && space == FALSE )
			*str = *str + "NM";
		else if( units == MIL && space == TRUE )
			*str = *str + " NM";
		if( lower_case )
			str->MakeLower();
	}
}

// function to make a CString from a double, stripping trailing zeros and "."
// allows maximum of 4 decimal places
//
void MakeCStringFromDouble( CString * str, double d )
{
	str->Format( "%12.5f", d );
	while(1 )
	{
		if( str->Right(1) == "0" )
			*str = str->Left( str->GetLength() - 1 );
		else if( str->Right(1) == "." )
		{
			*str = str->Left( str->GetLength() - 1 );
			break;
		}
		else
			break;
	}
	str->Trim();
}

// parse reference designator, such as "R1"
// return numeric suffix, or 0 if none
// set prefix to alphanumeric prefix
//
int ParseRef( CString * ref, CString * prefix )
{
	// get length of numeric suffix
	int ref_length = ref->GetLength();
	int num_length = 0;
	for( int i=ref_length-1; i>=0; i-- )
	{
		if( (*ref)[i] < '0' || (*ref)[i] > '9' )
		{
			break;
		}
		num_length++;
	}
	*prefix = ref->Left( ref_length - num_length );
	if( num_length ==  0 )
		return 0;
	CString num_str = ref->Right( num_length );
	int num = atoi( num_str );
	return num;
}

// test for legal pin name, such as "1", "A4", "SOURCE", but not "1A"
// if astr != NULL, set to alphabetic part
// if nstr != NULL, set to numeric part
// if n != NULL, set to value of numeric part
//
BOOL CheckLegalPinName( CString * pinstr, CString * astr, CString * nstr, int * n )
{
	CString aastr;
	CString nnstr;
	int nn = -1;

	if( (*pinstr).GetLength() == 0 )
		return FALSE;
	if( -1 != pinstr->FindOneOf( " .,;:/!@#$%^&*(){}[]|<>?\\~\'\"" ) )
		return FALSE;
	int asize = pinstr->FindOneOf( "0123456789" );
	if( asize == -1 )
	{
		// starts with a non-number
		aastr = *pinstr;
	}
	else if( asize == 0 )
	{
		// starts with a number, illegal if any non-numbers
		nnstr = *pinstr;
		for( int ic=0; ic<nnstr.GetLength(); ic++ )
		{
			if( nnstr[ic] < '0' || nnstr[ic] > '9' )
				return FALSE;
		}
		nn = atoi( nnstr );
	}
	else
	{
		// both alpha and numeric parts
		// get alpha substring
		aastr = pinstr->Left( asize );
		int test = aastr.FindOneOf( "0123456789" );
		if( test != -1 )
			return FALSE;	// alpha substring contains a number
		// get numeric substring
		nnstr = pinstr->Right( pinstr->GetLength() - asize );
		CString teststr = nnstr.SpanIncluding( "0123456789" );
		if( teststr != nnstr )
			return FALSE;	// numeric substring contains non-number
		nn = atoi( nnstr );
	}
	if( astr )
		*astr = aastr;
	if( nstr )
		*nstr = nnstr;
	if( n )
		*n = nn;
	return TRUE;
}



// find intersection between y = a + bx and y = c + dx;
//
int FindLineIntersection( double a, double b, double c, double d, double * x, double * y )
{
	*x = (c-a)/(b-d);
	*y = a + b*(*x);
	return 0;
}

// Return the coordinates of the extrapolated intersection between the two lines described
//  by the passed endpoints: [(x1a, y1a),(x2a, y2a)] and [(x1b, y1b), (x2b, y2b)]. 
//	Note that it also works for vertical lines (i.e. infinite slope).
//	Returns: -1 if the lines don't intersect, 
//			  1 if the intersection is on both of the line segments, 
//			  0 if the intersection is beyond the ends of both two segments.
//
int FindLineIntersection(double x0, double y0, double x1, double y1,
						 double x2, double y2, double x3, double y3,
						 double *linx, double *liny)
{
	double d=(x1-x0)*(y3-y2)-(y1-y0)*(x3-x2);
	if (fabs(d)<0.001) return -1;

	double AB=((y0-y2)*(x3-x2)-(x0-x2)*(y3-y2))/d;
	*linx=x0+AB*(x1-x0);
	*liny=y0+AB*(y1-y0);

	if (AB>0.0 && AB<1.0)
	{
		double CD=((y0-y2)*(x1-x0)-(x0-x2)*(y1-y0))/d;
		if (CD>0.0 && CD<1.0) return 1;
    }
	return 0;
}


// set EllipseKH struct to describe the ellipse for an arc
//
int MakeEllipseFromArc( int xi, int yi, int xf, int yf, int style, EllipseKH * el )
{
	// arc (quadrant of ellipse)
	// convert to clockwise arc
	int xxi, xxf, yyi, yyf;
	if( style == CPolyLine::ARC_CCW )
	{
		xxi = xf;
		xxf = xi;
		yyi = yf;
		yyf = yi;
	}
	else
	{
		xxi = xi;
		xxf = xf;
		yyi = yi;
		yyf = yf;
	}
	// find center and radii of ellipse
	double xo=xi, yo=yi, rx, ry;
	if( xxf > xxi && yyf > yyi )
	{
		xo = xxf;
		yo = yyi;
		el->theta1 = M_PI;
		el->theta2 = M_PI/2.0;
	}
	else if( xxf < xxi && yyf > yyi )
	{
		xo = xxi;
		yo = yyf;
		el->theta1 = -M_PI/2.0;
		el->theta2 = -M_PI;
	}
	else if( xxf < xxi && yyf < yyi )
	{
		xo = xxf;
		yo = yyi;
		el->theta1 = 0.0;
		el->theta2 = -M_PI/2.0;
	}
	else if( xxf > xxi && yyf < yyi )
	{
		xo = xxi;
		yo = yyf;
		el->theta1 = M_PI/2.0;
		el->theta2 = 0.0;
	}
	el->Center.X = xo;
	el->Center.Y = yo;
	el->xrad = abs(xf-xi);
	el->yrad = abs(yf-yi);
#if 0
	el->Phi = 0.0;
	el->MaxRad = el->xrad;
	el->MinRad = el->yrad;
	if( el->MaxRad < el->MinRad )
	{
		el->MaxRad = el->yrad;
		el->MinRad = el->xrad;
		el->Phi = M_PI/2.0;
	}
#endif
	return 0;
}

// find intersections between line segment (xi,yi) to (xf,yf)
// and line segment (xi2,yi2) to (xf2,yf2)
// the line segments may be arcs (i.e. quadrant of an ellipse) or straight
// returns number of intersections found (max of 2)
// returns coords of intersections in arrays x[2], y[2]
//
int FindSegmentIntersections( int xi, int yi, int xf, int yf, int style, 
								 int xi2, int yi2, int xf2, int yf2, int style2,
								 double x[], double y[] )
{
	double xr[12], yr[12];
	int iret = 0;

	if( max(xi,xf) < min(xi2,xf2) 
		|| min(xi,xf) > max(xi2,xf2) 
		|| max(yi,yf) < min(yi2,yf2) 
		|| min(yi,yf) > max(yi2,yf2) )
		return 0;

	if( style != CPolyLine::STRAIGHT && style2 != CPolyLine::STRAIGHT )
	{
		// two identical arcs intersect
		if( style == style2 && xi == xi2 && yi == yi2 && xf == xf2 && yf == yf2 )
		{
			if( x && y )
			{
				x[0] = xi;
				y[0] = yi;
			}
			return 1;
		}
		else if( style != style2 && xi == xf2 && yi == yf2 && xf == xi2 && yf == yi2 )
		{
			if( x && y )
			{
				x[0] = xi;
				y[0] = yi;
			}
			return 1;
		}
	}

	if( style == CPolyLine::STRAIGHT && style2 == CPolyLine::STRAIGHT )
	{
		// both straight-line segments
		int x, y;
		BOOL bYes = TestForIntersectionOfStraightLineSegments( xi, yi, xf, yf, xi2, yi2, xf2, yf2, &x, &y );
		if( !bYes )
			return 0;
		xr[0] = x;
		yr[0] = y;
		iret = 1;
	}
	else if( style == CPolyLine::STRAIGHT )
	{
		// first segment is straight, second segment is an arc
		int ret;
		double x1r, y1r, x2r, y2r;
		if( xf == xi )
		{
			// vertical first segment
			double a = xi;
			double b = DBL_MAX/2.0;
			ret = FindLineSegmentIntersection( a, b, xi2, yi2, xf2, yf2, style2,
				&x1r, &y1r, &x2r, &y2r );
		}
		else
		{
			double b = (double)(yf-yi)/(double)(xf-xi);
			double a = yf - b*xf;
			ret = FindLineSegmentIntersection( a, b, xi2, yi2, xf2, yf2, style2,
				&x1r, &y1r, &x2r, &y2r );
		}
		if( ret == 0 )
			return 0;
		if( InRange( x1r, xi, xf ) && InRange( y1r, yi, yf ) )
		{
			xr[iret] = x1r;
			yr[iret] = y1r;
			iret++;
		}
		if( ret == 2 )
		{
			if( InRange( x2r, xi, xf ) && InRange( y2r, yi, yf ) )
			{
				xr[iret] = x2r;
				yr[iret] = y2r;
				iret++;
			}
		}
	}
	else if( style2 == CPolyLine::STRAIGHT )
	{
		// first segment is an arc, second segment is straight
		int ret;
		double x1r, y1r, x2r, y2r;
		if( xf2 == xi2 )
		{
			// vertical second segment
			double a = xi2;
			double b = DBL_MAX/2.0;
			ret = FindLineSegmentIntersection( a, b, xi, yi, xf, yf, style,
				&x1r, &y1r, &x2r, &y2r );
		}
		else
		{
			double b = (double)(yf2-yi2)/(double)(xf2-xi2);
			double a = yf2 - b*xf2;
			ret = FindLineSegmentIntersection( a, b, xi, yi, xf, yf, style,
				&x1r, &y1r, &x2r, &y2r );
		}
		if( ret == 0 )
			return 0;
		if( InRange( x1r, xi2, xf2 ) && InRange( y1r, yi2, yf2 ) )
		{
			xr[iret] = x1r;
			yr[iret] = y1r;
			iret++;
		}
		if( ret == 2 )
		{
			if( InRange( x2r, xi2, xf2 ) && InRange( y2r, yi2, yf2 ) )
			{
				xr[iret] = x2r;
				yr[iret] = y2r;
				iret++;
			}
		}
	}
	else
	{
		// both segments are arcs
		EllipseKH el1;
		EllipseKH el2;
		Point IntPts[12];
		MakeEllipseFromArc( xi, yi, xf, yf, style, &el1 );
		MakeEllipseFromArc( xi2, yi2, xf2, yf2, style2, &el2 );
		int n;
		if( el1.xrad+el1.yrad > el2.xrad+el2.yrad )
			n = GetArcIntersections( &el1, &el2 );
		else
			n = GetArcIntersections( &el2, &el1 );
		iret = n;
	}
	if( x && y )
	{
		for( int i=0; i<iret; i++ )
		{
			x[i] = xr[i];
			y[i] = yr[i];
		}
	}
	return iret;
}

// find intersection between line y = a + bx and line segment (xi,yi) to (xf,yf)
// if b > DBL_MAX/10, assume vertical line at x = a
// the line segment may be an arc (i.e. quadrant of an ellipse)
// return 0 if no intersection
// returns 1 or 2 if intersections found
// sets coords of intersections in *x1, *y1, *x2, *y2
// if no intersection, returns min distance in dist
//
int FindLineSegmentIntersection( double a, double b, int xi, int yi, int xf, int yf, int style, 
								double * x1, double * y1, double * x2, double * y2,
								double * dist )
{
	double xx, yy;
	BOOL bVert = FALSE;
	if( b > DBL_MAX/10.0 )
		bVert = TRUE;

	if( xf != xi )
	{
		// non-vertical segment, get intersection
		if( style == CPolyLine::STRAIGHT || yf == yi )
		{
			// horizontal or oblique straight segment
			// put into form y = c + dx;
			double d = (double)(yf-yi)/(double)(xf-xi);
			double c = yf - d*xf;
			if( bVert )
			{
				// if vertical line, easy
				if( InRange( a, xi, xf ) )
				{
					*x1 = a;
					*y1 = c + d*a;
					return 1;
				}
				else
				{
					if( dist )
						*dist = min( abs(a-xi), abs(a-xf) );
					return 0;
				}
			}
			if( fabs(b-d) < 1E-12 )
			{
				// parallel lines
				if( dist )
				{
					*dist = GetPointToLineDistance( a, b, xi, xf );
				}
				return 0;	// lines parallel
			}
			// calculate intersection
			xx = (c-a)/(b-d);
			yy = a + b*(xx);
			// see if intersection is within the line segment
			if( yf == yi )
			{
				// horizontal line
				if( (xx>=xi && xx>xf) || (xx<=xi && xx<xf) )
					return 0;
			}
			else
			{
				// oblique line
				if( (xx>=xi && xx>xf) || (xx<=xi && xx<xf) 
					|| (yy>yi && yy>yf) || (yy<yi && yy<yf) )
					return 0;
			}
		}
		else if( style == CPolyLine::ARC_CW || style == CPolyLine::ARC_CCW )
		{
			// arc (quadrant of ellipse)
			// convert to clockwise arc
			int xxi, xxf, yyi, yyf;
			if( style == CPolyLine::ARC_CCW )
			{
				xxi = xf;
				xxf = xi;
				yyi = yf;
				yyf = yi;
			}
			else
			{
				xxi = xi;
				xxf = xf;
				yyi = yi;
				yyf = yf;
			}
			// find center and radii of ellipse
			double xo, yo, rx, ry;
			if( xxf > xxi && yyf > yyi )
			{
				xo = xxf;
				yo = yyi;
			}
			else if( xxf < xxi && yyf > yyi )
			{
				xo = xxi;
				yo = yyf;
			}
			else if( xxf < xxi && yyf < yyi )
			{
				xo = xxf;
				yo = yyi;
			}
			else if( xxf > xxi && yyf < yyi )
			{
				xo = xxi;
				yo = yyf;
			}
			rx = fabs( (double)(xxi-xxf) );
			ry = fabs( (double)(yyi-yyf) );
			BOOL test;
			double xx1, xx2, yy1, yy2, aa;
			if( bVert )
			{
				// shift vertical line to coordinate system of ellipse
				aa = a - xo;
				test = FindVerticalLineEllipseIntersections( rx, ry, aa, &yy1, &yy2 );
				if( !test )
					return 0;
				// shift back to PCB coordinates
				yy1 += yo;
				yy2 += yo;
				xx1 = a;
				xx2 = a;
			}
			else
			{
				// shift line to coordinate system of ellipse
				aa = a + b*xo - yo;
				test = FindLineEllipseIntersections( rx, ry, aa, b, &xx1, &xx2 );
				if( !test )
					return 0;
				// shift back to PCB coordinates
				yy1 = aa + b*xx1;
				xx1 += xo;
				yy1 += yo;
				yy2 = aa + b*xx2;
				xx2 += xo;
				yy2 += yo;
			}
			int npts = 0;
			if( (xxf>xxi && xx1<xxf && xx1>xxi) || (xxf<xxi && xx1<xxi && xx1>xxf) )
			{
				if( (yyf>yyi && yy1<yyf && yy1>yyi) || (yyf<yyi && yy1<yyi && yy1>yyf) )
				{
					*x1 = xx1;
					*y1 = yy1;
					npts = 1;
				}
			}
			if( (xxf>xxi && xx2<xxf && xx2>xxi) || (xxf<xxi && xx2<xxi && xx2>xxf) )
			{
				if( (yyf>yyi && yy2<yyf && yy2>yyi) || (yyf<yyi && yy2<yyi && yy2>yyf) )
				{
					if( npts == 0 )
					{
						*x1 = xx2;
						*y1 = yy2;
						npts = 1;
					}
					else
					{
						*x2 = xx2;
						*y2 = yy2;
						npts = 2;
					}
				}
			}
			return npts;
		}
		else
			ASSERT(0);
	}
	else
	{
		// vertical line segment
		if( bVert )
			return 0;
		xx = xi;
		yy = a + b*xx;
		if( (yy>=yi && yy>yf) || (yy<=yi && yy<yf) )
			return 0;
	}
	*x1 = xx;
	*y1 = yy;
	return 1;
}

// Test for intersection of line segments
// If lines are parallel, returns FALSE
// If TRUE, returns intersection coords in x, y
// if FALSE, returns min. distance in dist (may be 0.0 if parallel)
// and coords on nearest point in one of the segments in (x,y)
//
BOOL TestForIntersectionOfStraightLineSegments( int x1i, int y1i, int x1f, int y1f, 
									   int x2i, int y2i, int x2f, int y2f,
									   int * x, int * y, double * d )
{
	double a, b, dist;

	// first, test for intersection
	if( x1i == x1f && x2i == x2f )
	{
		// both segments are vertical, can't intersect
	}
	else if( y1i == y1f && y2i == y2f )
	{
		// both segments are horizontal, can't intersect
	}
	else if( x1i == x1f && y2i == y2f )
	{
		// first seg. vertical, second horizontal, see if they cross
		if( InRange( x1i, x2i, x2f )
			&& InRange( y2i, y1i, y1f ) )
		{
			if( x )
				*x = x1i;
			if( y )
				*y = y2i;
			if( d )
				*d = 0.0;
			return TRUE;
		}
	}
	else if( y1i == y1f && x2i == x2f )
	{
		// first seg. horizontal, second vertical, see if they cross
		if( InRange( y1i, y2i, y2f )
			&& InRange( x2i, x1i, x1f ) )
		{
			if( x )
				*x = x2i;
			if( y )
				*y = y1i;
			if( d )
				*d = 0.0;
			return TRUE;
		}
	}
	else if( x1i == x1f )
	{
		// first segment vertical, second oblique
		// get a and b for second line segment, so that y = a + bx;
		b = (double)(y2f-y2i)/(x2f-x2i);
		a = (double)y2i - b*x2i;
		double x1, y1, x2, y2;
		int test = FindLineSegmentIntersection( a, b, x1i, y1i, x1f, y1f, CPolyLine::STRAIGHT,
			&x1, &y1, &x2, &y2 );
		if( test )
		{
			if( InRange( y1, y1i, y1f ) && InRange( x1, x2i, x2f ) && InRange( y1, y2i, y2f ) )
			{
				if( x )
					*x = x1;
				if( y )
					*y = y1;
				if( d )
					*d = 0.0;
				return TRUE;
			}
		}
	}
	else if( y1i == y1f )
	{
		// first segment horizontal, second oblique
		// get a and b for second line segment, so that y = a + bx;
		b = (double)(y2f-y2i)/(x2f-x2i);
		a = (double)y2i - b*x2i;
		double x1, y1, x2, y2;
		int test = FindLineSegmentIntersection( a, b, x1i, y1i, x1f, y1f, CPolyLine::STRAIGHT,
			&x1, &y1, &x2, &y2 );
		if( test )
		{
			if( InRange( x1, x1i, x1f ) && InRange( x1, x2i, x2f ) && InRange( y1, y2i, y2f ) )
			{
				if( x )
					*x = x1;
				if( y )
					*y = y1;
				if( d )
					*d = 0.0;
				return TRUE;
			}
		}
	}
	else if( x2i == x2f )
	{
		// second segment vertical, first oblique
		// get a and b for first line segment, so that y = a + bx;
		b = (double)(y1f-y1i)/(x1f-x1i);
		a = (double)y1i - b*x1i;
		double x1, y1, x2, y2;
		int test = FindLineSegmentIntersection( a, b, x2i, y2i, x2f, y2f, CPolyLine::STRAIGHT,
			&x1, &y1, &x2, &y2 );
		if( test )
		{
			if( InRange( x1, x1i, x1f ) &&  InRange( y1, y1i, y1f ) && InRange( y1, y2i, y2f ) )
			{
				if( x )
					*x = x1;
				if( y )
					*y = y1;
				if( d )
					*d = 0.0;
				return TRUE;
			}
		}
	}
	else if( y2i == y2f )
	{
		// second segment horizontal, first oblique
		// get a and b for second line segment, so that y = a + bx;
		b = (double)(y1f-y1i)/(x1f-x1i);
		a = (double)y1i - b*x1i;
		double x1, y1, x2, y2;
		int test = FindLineSegmentIntersection( a, b, x2i, y2i, x2f, y2f, CPolyLine::STRAIGHT,
			&x1, &y1, &x2, &y2 );
		if( test )
		{
			if( InRange( x1, x1i, x1f ) && InRange( y1, y1i, y1f ) )
			{
				if( x )
					*x = x1;
				if( y )
					*y = y1;
				if( d )
					*d = 0.0;
				return TRUE;
			}
		}
	}
	else
	{
		// both segments oblique
		if( (long)(y1f-y1i)*(x2f-x2i) != (long)(y2f-y2i)*(x1f-x1i) )
		{
			// not parallel, get a and b for first line segment, so that y = a + bx;
			b = (double)(y1f-y1i)/(x1f-x1i);
			a = (double)y1i - b*x1i;
			double x1, y1, x2, y2;
			int test = FindLineSegmentIntersection( a, b, x2i, y2i, x2f, y2f, CPolyLine::STRAIGHT,
				&x1, &y1, &x2, &y2 );
			// both segments oblique
			if( test )
			{
				if( InRange( x1, x1i, x1f ) && InRange( y1, y1i, y1f ) )
				{
					if( x )
						*x = x1;
					if( y )
						*y = y1;
					if( d )
						*d = 0.0;
					return TRUE;
				}
			}
		}
	}
	// don't intersect, get shortest distance between each endpoint and the other line segment
	dist = GetPointToLineSegmentDistance( x1i, y1i, x2i, y2i, x2f, y2f );
	double xx = x1i;
	double yy = y1i;
	double dd = GetPointToLineSegmentDistance( x1f, y1f, x2i, y2i, x2f, y2f );
	if( dd < dist )
	{
		dist = dd;
		xx = x1f;
		yy = y1f;
	}
	dd = GetPointToLineSegmentDistance( x2i, y2i, x1i, y1i, x1f, y1f );
	if( dd < dist )
	{
		dist = dd;
		xx = x2i;
		yy = y2i;
	}
	dd = GetPointToLineSegmentDistance( x2f, y2f, x1i, y1i, x1f, y1f );
	if( dd < dist )
	{
		dist = dd;
		xx = x2f;
		yy = y2f;
	}
	if( x )
		*x = xx;
	if( y )
		*y = yy;
	if( d )
		*d = dist;
	return FALSE;
}

// these functions are for profiling
//
void DunselFunction() { return; }

void CalibrateTimer()
{
	void (*pFunc)() = DunselFunction;

	if( QueryPerformanceFrequency( &PerfFreq ) )
	{
		// use hires timer
		PerfFreqAdjust = 0;
		int High32 = PerfFreq.HighPart;
		int Low32 = PerfFreq.LowPart;
		while( High32 )
		{
			High32 >>= 1;
			PerfFreqAdjust++;
		}
		return;
	}
	else
		ASSERT(0);
}

void StartTimer()
{
	SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL );
	QueryPerformanceCounter( &tStart );
}

double GetElapsedTime()
{
	QueryPerformanceCounter( &tStop );
	SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_NORMAL );

	double time;
	int ReduceMag = 0;
	LARGE_INTEGER Freq = PerfFreq;
	unsigned int High32 = tStop.HighPart - tStart.HighPart;
	while( High32 )
	{
		High32 >>= 1;
		ReduceMag++;
	}
	if( PerfFreqAdjust || ReduceMag )
	{
		if( PerfFreqAdjust > ReduceMag )
			ReduceMag = PerfFreqAdjust;
		tStart.QuadPart = Int64ShrlMod32( tStart.QuadPart, ReduceMag );
		tStop.QuadPart = Int64ShrlMod32( tStop.QuadPart, ReduceMag );
		Freq.QuadPart = Int64ShrlMod32( Freq.QuadPart, ReduceMag );
	}
	if( Freq.LowPart == 0 )
		time = 0.0;
	else
		time = ((double)(tStop.LowPart - tStart.LowPart))/Freq.LowPart;
	return time;
}

// quicksort algorithm
// sorts array numbers[], also moves elements of another array index[]
//
#define Q3WAY
void quickSort(int numbers[], int index[], int array_size)
{
#ifdef Q3WAY
  q_sort_3way(numbers, index, 0, array_size - 1);
#else
  q_sort(numbers, index, 0, array_size - 1);
#endif
}

// standard quicksort
//
void q_sort(int numbers[], int index[], int left, int right)
{
	int pivot, pivot_index, l_hold, r_hold;

	l_hold = left;
	r_hold = right;
	pivot = numbers[left];
	pivot_index = index[left];
	while (left < right)
	{
		while ((numbers[right] >= pivot) && (left < right))
			right--;
		if (left != right)
		{
			numbers[left] = numbers[right];
			index[left] = index[right];
			left++;
		}
		while ((numbers[left] <= pivot) && (left < right))
			left++;
		if (left != right)
		{
			numbers[right] = numbers[left];
			index[right] = index[left];
			right--;
		}
	}
	numbers[left] = pivot;
	index[left] = pivot_index;

	pivot = left;
	left = l_hold;
	right = r_hold;
	if (left < pivot)
		q_sort(numbers, index, left, pivot-1);
	if (right > pivot)
		q_sort(numbers, index, pivot+1, right);
}

// 3-way quicksort...useful where there are duplicate values
//
void q_sort_3way( int a[], int b[], int l, int r )
{
	#define EXCH(i,j) {int temp=a[i]; a[i]=a[j]; a[j]=temp; temp=b[i]; b[i]=b[j]; b[j]=temp;}

	int i = l - 1;
	int j = r;
	int p = l - 1;
	int q = r;
	int v = a[r];

	if( r <= l )
		return;

	for(;;)
	{
		while( a[++i] < v );
		while( v < a[--j] )
			if( j == 1 )
				break;
		if( i >= j )
			break;
		EXCH( i, j );
		if( a[i] == v )
		{
			p++;
			EXCH( p, i );
		}
		if( v == a[j] )
		{
			q--;
			EXCH( j, q );
		}
	}
	EXCH( i, r );
	j = i - 1;
	i = i + 1;
	for( int k=l; k<p; k++, j-- )
		EXCH( k, j );
	for( int k=r-1; k>q; k--, i++ )
		EXCH( i, k );
	q_sort_3way( a, b, l, j );
	q_sort_3way( a, b, i, r );
}

// convert angle in degrees from CW to CCW
//
int ccw( int angle )
{
	return (720-angle)%360;
}

// rotate point representing vector between ends of 45-degree curve
// so that it's long axis points along y
// use -octant to reverse the transform
//
CPoint t_octant( int octant, CPoint& pt )
{
	CPoint p;
	switch( octant )
	{
	case 0: p.x =  pt.x;  p.y =  pt.y;  break; 

	case 1: p.x =  pt.y;  p.y = -pt.x;  break; 
	case 2: p.x =  pt.y;  p.y = -pt.x;  break;
	case 3: p.x = -pt.x;  p.y = -pt.y;  break;
	case 4: p.x = -pt.x;  p.y = -pt.y;  break;
	case 5: p.x = -pt.y;  p.y =  pt.x;  break;
	case 6: p.x = -pt.y;  p.y =  pt.x;  break;
	case 7: p.x =  pt.x;  p.y =  pt.y;  break;

	case -1: p.x = -pt.y;  p.y =  pt.x;  break; 
	case -2: p.x = -pt.y;  p.y =  pt.x;  break;
	case -3: p.x = -pt.x;  p.y = -pt.y;  break;
	case -4: p.x = -pt.x;  p.y = -pt.y;  break;
	case -5: p.x =  pt.y;  p.y = -pt.x;  break;
	case -6: p.x =  pt.y;  p.y = -pt.x;  break;
	case -7: p.x =  pt.x;  p.y =  pt.y;  break;
	}
	return p;
}


// solves quadratic equation
// i.e.   ax**2 + bx + c = 0
// returns TRUE if solution exist, with solutions in x1 and x2
// else returns FALSE
//
BOOL Quadratic( double a, double b, double c, double *x1, double *x2 )
{
	double root = b*b - 4.0*a*c;
	if( root < 0.0 )
		return FALSE;
	root = sqrt( root );
	*x1 = (-b+root)/(2.0*a);
	*x2 = (-b-root)/(2.0*a);
	return TRUE;
}

// finds intersections of vertical line at x
// with ellipse defined by (x^2)/(a^2) + (y^2)/(b^2) = 1;
// returns TRUE if solution exist, with solutions in y1 and y2
// else returns FALSE
//
BOOL FindVerticalLineEllipseIntersections( double a, double b, double x, double *y1, double *y2 )
{
	double y_sqr = (1.0-(x*x)/(a*a))*b*b;
	if( y_sqr < 0.0 )
		return FALSE;
	*y1 = sqrt(y_sqr);
	*y2 = -*y1;
	return TRUE;
}

// finds intersections of straight line y = c + dx
// with ellipse defined by (x^2)/(a^2) + (y^2)/(b^2) = 1;
// returns TRUE if solution exist, with solutions in x1 and x2
// else returns FALSE
//
BOOL FindLineEllipseIntersections( double a, double b, double c, double d, double *x1, double *x2 )
{
	// quadratic terms
	double A = d*d+b*b/(a*a);
	double B = 2.0*c*d;
	double C = c*c-b*b;
	return Quadratic( A, B, C, x1, x2 );
}

// draw a straight line or an arc between xi,yi and xf,yf
//
void DrawArc( CDC * pDC, int shape, int xxi, int yyi, int xxf, int yyf, BOOL bMeta )
{
	int xi, yi, xf, yf;
	if( shape == DL_LINE || xxi == xxf || yyi == yyf )
	{
		// draw straight line
		pDC->MoveTo( xxi, yyi );
		pDC->LineTo( xxf, yyf );
	}
	else if( shape == DL_ARC_CCW || shape == DL_ARC_CW ) 
	{
		// set endpoints so we can always draw counter-clockwise arc
		if( shape == DL_ARC_CW )
		{
			xi = xxf;
			yi = yyf;
			xf = xxi; 
			yf = yyi;
		}
		else
		{
			xi = xxi;
			yi = yyi;
			xf = xxf;
			yf = yyf;
		}
		pDC->MoveTo( xi, yi );
		if( xf > xi && yf > yi )
		{
			// quadrant 1
			int w = (xf-xi)*2;
			int h = (yf-yi)*2;
			if( !bMeta )
				pDC->Arc( xf-w, yi+h, xf, yi,
					xi, yi, xf, yf );
			else
				pDC->Arc( xf-w, yi, xf, yi+h,
					xf, yf, xi, yi );
		}
		else if( xf < xi && yf > yi )
		{
			// quadrant 2
			int w = -(xf-xi)*2;
			int h = (yf-yi)*2;
			if( !bMeta )
				pDC->Arc( xi-w, yf, xi, yf-h,
					xi, yi, xf, yf );
			else
				pDC->Arc( xi-w, yf-h, xi, yf,
					xf, yf, xi, yi );
		}
		else if( xf < xi && yf < yi )
		{
			// quadrant 3
			int w = -(xf-xi)*2;
			int h = -(yf-yi)*2;
			if( !bMeta )
				pDC->Arc( xf, yi, xf+w, yi-h,
					xi, yi, xf, yf ); 
			else
				pDC->Arc( xf, yi-h, xf+w, yi,
					xf, yf, xi, yi );
		}
		else if( xf > xi && yf < yi )
		{
			// quadrant 4
			int w = (xf-xi)*2;
			int h = -(yf-yi)*2;
			if( !bMeta )
				pDC->Arc( xi, yf+h, xi+w, yf,
					xi, yi, xf, yf );
			else
				pDC->Arc( xi, yf, xi+w, yf+h,
					xf, yf, xi, yi );
		}
		pDC->MoveTo( xxf, yyf );
	}
	else
		ASSERT(0);	// oops
}

// Draw a straight line or a curve between xi,yi and xf,yf
// The type of curve depends on the angle between the endpoints
// If a multiple of 90 degrees, draw a straight line,
// else if a multiple of 45 degrees, draw a quadrant of a circle,
// else draw a compound curve consisting of an octant of a circle 
// extended by a straight line at one end or the other
// shape can be DL_LINE, DL_CURVE_CW or DL_CURVE_CCW
//
void DrawCurve( CDC * pDC, int shape, int xxi, int yyi, int xxf, int yyf, BOOL bMeta )
{
	CPoint pt_start(xxi,yyi), pt_end(xxf,yyf);
	if( shape == DL_LINE || xxi == xxf || yyi == yyf || abs(xxf-xxi) == abs(yyf-yyi) )
	{
		// for straight line or 90-degree arc, use DrawArc()
		if( shape == DL_LINE )
			DrawArc( pDC, DL_LINE, xxi, yyi, xxf, yyf, bMeta );
		else if( shape == DL_CURVE_CW )
			DrawArc( pDC, DL_ARC_CW, xxi, yyi, xxf, yyf, bMeta );
		else if( shape == DL_CURVE_CCW )
			DrawArc( pDC, DL_ARC_CCW, xxi, yyi, xxf, yyf, bMeta );
	}
	else if( shape == DL_CURVE_CW || shape == DL_CURVE_CCW ) 
	{
		// for the compound curve, there are 8 possible octants,
		// the straight line can be either vert/horizontal or at 45 degrees 
		// (depending on dy/dx ratio)
		// and the curve can be CW or CCW for a total of 32 different cases
		// to simplify, set endpoints so we can always draw a counter-clockwise arc
		if( shape == DL_CURVE_CW )
		{
			CPoint temp = pt_start;
			pt_start = pt_end;
			pt_end = temp;
		}
		pDC->MoveTo( pt_start );
		// now get the vector from the start to end of curve
		// to simplify further, rotate the vector so dy > 0 and dy > |dx|
		// now there are only 4 cases
		// we'll reverse the rotation at the end
		CPoint d = pt_end - pt_start;	// vector	
		double angle = atan2( (double)d.y, (double)d.x );
		angle -= M_PI_2;
		if( angle < 0.0 )
			angle = 2.0 * M_PI + angle;
		int octant = angle/M_PI_4;
		if ( octant < 0 )
			octant = 0;
		else if( octant > 7 ) 
			octant = 7;
		// rotate vector
		d = t_octant( octant, d );
		double tdx = d.x;	// width of bounding rect for compound curve 
		double tdy = d.y;	// height of bounding rect for compound curve
		double sin_45 = sin(M_PI_4);
		double arc_ratio = abs(sin_45/(1.0 - sin_45)); // dy/dx for octant arc in quadrant 0
		CPoint pt_arc_start, pt_arc_end, pt_line_start, pt_line_end, pt_circle_center;
		int circle_radius;
		if( abs(tdy/tdx) > arc_ratio )
		{
			// height/width ratio of rect is greater than arc, add vertical line to arc
			if( tdx > 0 )
			{
				pt_arc_start.x = 0;
				pt_arc_start.y = 0;
				pt_arc_end.x = tdx;
				pt_arc_end.y = tdx*arc_ratio;
				pt_line_start = pt_arc_end;
				pt_line_end.x = tdx;
				pt_line_end.y = tdy;
				pt_circle_center.y = pt_arc_end.y;
				pt_circle_center.x = - tdx*arc_ratio;
				circle_radius = tdx - pt_circle_center.x;
			}
			else
			{
				pt_arc_start.x = 0;
				pt_arc_start.y = tdy + tdx*arc_ratio;
				pt_arc_end.x = tdx;
				pt_arc_end.y = tdy;
				pt_line_start.x = 0;
				pt_line_start.y = 0;
				pt_line_end = pt_arc_start;
				pt_circle_center.y = pt_arc_start.y;
				pt_circle_center.x = tdx - (tdy - pt_arc_start.y);
				circle_radius = -pt_circle_center.x;
			}
		}
		else
		{
			// height/width ratio of rect is less than arc, add 45-degree line to arc
			if( tdx > 0 )
			{
				double ax = (-tdx+tdy)/(1.0-arc_ratio);
				double ay = -arc_ratio*ax;	
				pt_line_start.x = 0;
				pt_line_start.y = 0;
				pt_line_end.x = tdx + ax;
				pt_line_end.y = tdy - ay;
				pt_arc_start = pt_line_end;
				pt_arc_end.x = tdx;
				pt_arc_end.y = tdy;
				pt_circle_center.x = tdx + ax - ay;
				pt_circle_center.y = tdy;
				circle_radius = ay - ax;
			}
			else
			{
				double ax = (tdx+tdy)/(1.0-arc_ratio);
				double ay = -arc_ratio*ax;	
				pt_arc_start.x = 0;
				pt_arc_start.y = 0;
				pt_arc_end.x = ax;
				pt_arc_end.y = ay;
				pt_line_start = pt_arc_end;
				pt_line_end.x = tdx;
				pt_line_end.y = tdy;
				pt_circle_center.x = pt_arc_end.x - pt_arc_end.y;
				pt_circle_center.y = 0;
				circle_radius = -pt_circle_center.x;
			}

		}
		// now transform points back to original octant
		pt_arc_start = t_octant( -octant, pt_arc_start );
		pt_arc_end = t_octant( -octant, pt_arc_end );
		pt_line_start = t_octant( -octant, pt_line_start );
		pt_line_end = t_octant( -octant, pt_line_end );
		pt_circle_center = t_octant( -octant, pt_circle_center );
		// and offset back to starting point
		CPoint pt_offset = pt_start;
		pt_arc_start += pt_offset;
		pt_arc_end += pt_offset;
		pt_line_start += pt_offset;
		pt_line_end += pt_offset;
		pt_circle_center += pt_offset;
		CRect br;
		br.left = pt_circle_center.x - circle_radius;
		br.right = pt_circle_center.x + circle_radius;
		br.top = pt_circle_center.y - circle_radius;
		br.bottom = pt_circle_center.y + circle_radius;
		// draw arc and line
		pDC->Arc( br, pt_arc_start, pt_arc_end );
		pDC->MoveTo( pt_line_start );
		pDC->LineTo( pt_line_end );
	}
	else
		ASSERT(0);	// illegal shape
	pDC->MoveTo( xxf, yyf );
}

// Get arrays of circles, rects and line segments to represent pad
// for purposes of drawing pad or calculating clearances
// margins of circles and line segments represent pad outline
// circles and rects are used to find points inside pad
//
void GetPadElements( int type, int x, int y, int wid, int len, int radius, int angle,
					int * nr, my_rect r[], int * nc, my_circle c[], int * ns, my_seg s[] )
{
	*nc = 0;
	*nr = 0; 
	*ns = 0;
	if( type == PAD_ROUND )
	{
		*nc = 1;
		c[0] = my_circle(x,y,wid/2);
		return;
	}
	if( type == PAD_SQUARE )
	{
		*ns = 4;
		s[0] = my_seg(x-wid/2, y+wid/2,x+wid/2, y+wid/2);	// top
		s[1] = my_seg(x-wid/2, y-wid/2,x+wid/2, y-wid/2);	// bottom
		s[2] = my_seg(x-wid/2, y-wid/2,x-wid/2, y+wid/2);	// left
		s[3] = my_seg(x+wid/2, y-wid/2,x+wid/2, y+wid/2);	// right
		if( angle%90 )
		{
			for( int st=0; st<4; st++)
			{
				Rotate_i_Vertex( &s[st].xi, &s[st].yi, angle, x, y ); 
				Rotate_i_Vertex( &s[st].xf, &s[st].yf, angle, x, y );
			}
		}
		else
		{
			*nr = 1;
			r[0] = my_rect(x-wid/2, y-wid/2,x+wid/2, y+wid/2);
		}
		return;
	}
	if( type == PAD_OCTAGON )
	{
		*ns = 8;
		CPoint P[8];
		int np = Gen_RndRectPoly( x, y, wid, wid, (float)wid/3.48, -angle, P, 8 );
		for( int is=0; is<np-1; is++ )
			s[is] = my_seg(P[is].x, P[is].y, P[is+1].x, P[is+1].y );
		s[np-1] = my_seg(P[np-1].x, P[np-1].y, P[0].x, P[0].y );
		return;
	}
	
	// 
	int h;
	int v;
	if( angle%90 == 0 && angle%180 )
	{
		h = wid;
		v = len;
	}
	else
	{	
		v = wid;
		h = len;			
	}
	if( type == PAD_RECT )
	{
		*ns = 4;
		s[0] = my_seg(x-h/2, y+v/2,x+h/2, y+v/2);	// top
		s[1] = my_seg(x-h/2, y-v/2,x+h/2, y-v/2);	// bottom
		s[2] = my_seg(x-h/2, y-v/2,x-h/2, y+v/2);	// left
		s[3] = my_seg(x+h/2, y-v/2,x+h/2, y+v/2);	// right
		if( angle%90 )
		{
			for( int st=0; st<4; st++)
			{
				Rotate_i_Vertex( &s[st].xi, &s[st].yi, -angle, x, y ); 
				Rotate_i_Vertex( &s[st].xf, &s[st].yf, -angle, x, y );
			}
		}
		else
		{
			*nr = 1;
			r[0] = my_rect(x-h/2, y-v/2, x+h/2, y+v/2);
		}
		return;
	}
	if( type == PAD_RRECT )
	{
		*nc = 4;
		c[0] = my_circle(x-h/2+radius, y-v/2+radius, radius);	// bottom left circle
		c[1] = my_circle(x+h/2-radius, y-v/2+radius, radius);	// bottom right circle
		c[2] = my_circle(x-h/2+radius, y+v/2-radius, radius);	// top left circle
		c[3] = my_circle(x+h/2-radius, y+v/2-radius, radius);	// top right circle
		*ns = 4;
		s[0] = my_seg(x-h/2+radius, y+v/2, x+h/2-radius, y+v/2);	// top
		s[1] = my_seg(x-h/2+radius, y-v/2, x+h/2-radius, y-v/2);	// bottom
		s[2] = my_seg(x-h/2, y-v/2+radius, x-h/2, y+v/2-radius);	// left
		s[3] = my_seg(x+h/2, y-v/2+radius, x+h/2, y+v/2-radius);	// right
		if( angle%90 )
		{
			for( int st=0; st<4; st++)
			{
				Rotate_i_Vertex( &c[st].x, &c[st].y, -angle, x, y ); 
				Rotate_i_Vertex( &s[st].xi, &s[st].yi, -angle, x, y ); 
				Rotate_i_Vertex( &s[st].xf, &s[st].yf, -angle, x, y );
			}
		}
		else
		{
			*nr = 2;
			r[0] = my_rect(x-h/2+radius, y-v/2, x+h/2-radius, y+v/2);
			r[1] = my_rect(x-h/2, y-v/2+radius, x+h/2, y+v/2-radius);
		}
		return;
	}
	if( type == PAD_OVAL )
	{
		if( h > v )
		{
			// horizontal
			*nc = 2;
			c[0] = my_circle(x-h/2+v/2, y, v/2);	// left circle
			c[1] = my_circle(x+h/2-v/2, y, v/2);	// right circle
			*nr = 1;
			r[0] = my_rect(x-h/2+v/2, y-v/2, x+h/2-v/2, y+v/2);
			*ns = 2;
			s[0] = my_seg(x-h/2+v/2, y+v/2, x+h/2-v/2, y+v/2);	// top
			s[1] = my_seg(x-h/2+v/2, y-v/2, x+h/2-v/2, y-v/2);	// bottom
		}
		else
		{
			// vertical
			*nc = 2;
			c[0] = my_circle(x, y+v/2-h/2, h/2);	// top circle
			c[1] = my_circle(x, y-v/2+h/2, h/2);	// bottom circle
			*nr = 1;
			r[0] = my_rect(x-h/2, y-v/2+h/2, x+h/2, y+v/2-h/2);
			*ns = 2;
			s[0] = my_seg(x-h/2, y-v/2+h/2, x-h/2, y+v/2-h/2);	// left
			s[1] = my_seg(x+h/2, y-v/2+h/2, x+h/2, y+v/2-h/2);	// left
		}
		if( angle%90 )
		{
			*nr = 0;
			for( int st=0; st<2; st++)
			{
				Rotate_i_Vertex( &c[st].x, &c[st].y, -angle, x, y ); 
				Rotate_i_Vertex( &s[st].xi, &s[st].yi, -angle, x, y ); 
				Rotate_i_Vertex( &s[st].xf, &s[st].yf, -angle, x, y );
			}
		}
		return;
	}
	ASSERT(0);
}

// Find distance from a staright line segment to a pad
//
int GetClearanceBetweenSegmentAndPad( int x1, int y1, int x2, int y2, int w,
								  int type, int x, int y, int wid, int len, int radius, int angle )
{
	if( type == PAD_NONE )
		return INT_MAX;
	else
	{
		int nc, nr, ns;
		my_circle c[4];
		my_rect r[2];
		my_seg s[8];
		GetPadElements( type, x, y, wid, len, radius, angle,
						&nr, r, &nc, c, &ns, s );
		// first test for endpoints of line segment in rectangle
		for( int ir=0; ir<nr; ir++ )
		{
			if( x1 >= r[ir].xlo && x1 <= r[ir].xhi && y1 >= r[ir].ylo && y1 <= r[ir].yhi )
				return 0;
			if( x2 >= r[ir].xlo && x2 <= r[ir].xhi && y2 >= r[ir].ylo && y2 <= r[ir].yhi )
				return 0;
		}
		// now get distance from elements of pad outline
		int dist = INT_MAX;
		for( int ic=0; ic<nc; ic++ )
		{
			int d = GetPointToLineSegmentDistance( c[ic].x, c[ic].y, x1, y1, x2, y2 ) - c[ic].r - w/2;
			dist = min(dist,d);
		}
		for( int is=0; is<ns; is++ )
		{
			double d;
			TestForIntersectionOfStraightLineSegments( s[is].xi, s[is].yi, s[is].xf, s[is].yf,
					x1, y1, x2, y2, NULL, NULL, &d );
			d -= w/2;
			dist = min(dist,d);
		}
		return max(0,dist);
	}
}

// Get clearance between 2 segments
// Returns point in segment closest to other segment in x, y
// in clearance > max_cl, just returns max_cl and doesn't return x,y
//
int GetClearanceBetweenSegments( int x1i, int y1i, int x1f, int y1f, int style1, int w1,
								   int x2i, int y2i, int x2f, int y2f, int style2, int w2,
								   int max_cl, int * x, int * y )
{
	// check clearance between bounding rectangles
	int test = max_cl*2 + w1/2 + w2/2;
	if( min(x1i,x1f)-max(x2i,x2f) > test )
		return max_cl*2;
	if( min(x2i,x2f)-max(x1i,x1f) > test )
		return max_cl*2;
	if( min(y1i,y1f)-max(y2i,y2f) > test )
		return max_cl*2;
	if( min(y2i,y2f)-max(y1i,y1f) > test )
		return max_cl*2;

	if( style1 == CPolyLine::STRAIGHT && style2 == CPolyLine::STRAIGHT )
	{
		// both segments are straight lines
		int xx, yy;
		double dd;
		TestForIntersectionOfStraightLineSegments( x1i, y1i, x1f, y1f, 
			x2i, y2i, x2f, y2f, &xx, &yy, &dd );
		int d = max( 0, dd - w1/2 - w2/2 );
		if( x )
			*x = xx;
		if( y )
			*y = yy;
		return d;
	}

	// not both straight-line segments
	// see if segments intersect
	double xr[2];
	double yr[2];
	test = FindSegmentIntersections( x1i, y1i, x1f, y1f, style1, x2i, y2i, x2f, y2f, style2, xr, yr );
	if( test ) 
	{
		if( x )
			*x = xr[0];
		if( y )
			*y = yr[0];
		return max( 0, 0 - w1/2 - w2/2 );
	}

	// at least one segment is an arc
	EllipseKH el1;
	EllipseKH el2;
	BOOL bArcs;
	int xi, yi, xf, yf;
	if( style2 == CPolyLine::STRAIGHT )
	{
		// style1 = arc, style2 = straight
		MakeEllipseFromArc( x1i, y1i, x1f, y1f, style1, &el1 );
		xi = x2i;
		yi = y2i;
		xf = x2f;
		yf = y2f;
		bArcs = FALSE;
	}
	else if( style1 == CPolyLine::STRAIGHT )
	{
		// style2 = arc, style1 = straight
		xi = x1i;
		yi = y1i;
		xf = x1f;
		yf = y1f;
		MakeEllipseFromArc( x2i, y2i, x2f, y2f, style2, &el1 );
		bArcs = FALSE;
	}
	else
	{
		// style1 = arc, style2 = arc
		MakeEllipseFromArc( x1i, y1i, x1f, y1f, style1, &el1 );
		MakeEllipseFromArc( x2i, y2i, x2f, y2f, style2, &el2 );
		bArcs = TRUE;
	}
	const int NSTEPS = 32;

	if( el1.theta2 > el1.theta1 )
		ASSERT(0);
	if( bArcs && el2.theta2 > el2.theta1 )
		ASSERT(0);

	// test multiple points in both segments
	double th1;
	double th2;
	double len2;
	if( bArcs )
	{
		th1 = el2.theta1;
		th2 = el2.theta2;
		len2 = max(el2.xrad, el2.yrad);
	}
	else
	{
		th1 = 1.0;
		th2 = 0.0;
		len2 = abs(xf-xi)+abs(yf-yi);
	}
	double s_start = el1.theta1;
	double s_end = el1.theta2;
	double s_start2 = th1;
	double s_end2 = th2;
	double dmin = Distance( x1i, y1i, x2i, y2i );;
	double xmin = x1i;
	double ymin = y1i;
	double smin = DBL_MAX, smin2 = DBL_MAX;
	
	int nsteps = NSTEPS;
	int nsteps2 = NSTEPS;
	double step = (s_start-s_end)/(nsteps-1);
	double step2 = (s_start2-s_end2)/(nsteps2-1);
	double c1 = (step * max(el1.xrad, el1.yrad))/100.0;
	double c2 = (step2 * len2)/100.0;
	while( (step * max(el1.xrad, el1.yrad)) > c1 && (step2 * len2) > c2 )
	{
		step = (s_start-s_end)/(nsteps-1);
		for( int i=0; i<nsteps; i++ )
		{
			double s;
			if( i < nsteps-1 )
				s = s_start - i*step;
			else
				s = s_end;
			double x = el1.Center.X + el1.xrad*cos(s);
			double y = el1.Center.Y + el1.yrad*sin(s);
			// if not an arc, use s2 as fractional distance along line
			step2 = (s_start2-s_end2)/(nsteps2-1);
			for( int i2=0; i2<nsteps2; i2++ )
			{
				double s2;
				if( i2 < nsteps2-1 )
					s2 = s_start2 - i2*step2;
				else
					s2 = s_end2;
				double x2, y2;
				if( !bArcs )
				{
					x2 = xi + (xf-xi)*s2;
					y2 = yi + (yf-yi)*s2;
				}
				else
				{
					x2 = el2.Center.X + el2.xrad*cos(s2);
					y2 = el2.Center.Y + el2.yrad*sin(s2);
				}
				double d = Distance( x, y, x2, y2 );
				if( d < dmin )
				{
					dmin = d;
					xmin = x;
					ymin = y;
					smin = s;
					smin2 = s2;
				}
			}
		}
		if( step > step2 )
		{
			s_start = min(el1.theta1, smin + step);
			s_end = max(el1.theta2, smin - step);
			step = (s_start - s_end)/nsteps;
		}
		else
		{
			s_start2 = min(th1, smin2 + step2);
			s_end2 = max(th2, smin2 - step2);
			step2 = (s_start2 - s_end2)/nsteps2;
		}
	}
	if( x )
		*x = xmin;
	if( y )
		*y = ymin;
	return max(0,dmin-w1/2-w2/2);	// allow for widths
}



// Find clearance between pads
// For each pad:
//	type = PAD_ROUND, PAD_SQUARE, etc.
//	x, y = center position
//	w, l = width and length
//  r = corner radius
//	angle = 0 or 90 (if 0, pad length is along x-axis)
//
int GetClearanceBetweenPads( int type1, int x1, int y1, int w1, int l1, int r1, int angle1,
							 int type2, int x2, int y2, int w2, int l2, int r2, int angle2 )
{
	if( type1 == PAD_NONE )
		return INT_MAX;
	if( type2 == PAD_NONE )
		return INT_MAX;

	int dist = INT_MAX;
	int nr, nc, ns, nrr, ncc, nss;
	my_rect r[2], rr[2];
	my_circle c[4], cc[4];
	my_seg s[8], ss[8];
	GetPadElements( type1, x1, y1, w1, l1, r1, angle1,
					&nr, r, &nc, c, &ns, s );
	GetPadElements( type2, x2, y2, w2, l2, r2, angle2,
					&nrr, rr, &ncc, cc, &nss, ss );
	if ( nr )
	{
		if( nrr )
		{
			RECT ir,irr;
			ir.left =	r[0].xlo;
			ir.right =	r[0].xhi;
			ir.bottom = r[0].ylo;
			ir.top =	r[0].yhi;
			irr.left =	rr[0].xlo;
			irr.right =	rr[0].xhi;
			irr.bottom =rr[0].ylo;
			irr.top =	rr[0].yhi;
			if (RectsIntersection(ir,irr) >= 0)
				return 0;
			if( nrr == 2 )
			{
				irr.left =	rr[1].xlo;
				irr.right =	rr[1].xhi;
				irr.bottom =rr[1].ylo;
				irr.top =	rr[1].yhi;
				if (RectsIntersection(ir,irr) >= 0)
					return 0;
			}
		}
		else if (InRange(x2,r[0].xlo,r[0].xhi) && InRange(y2,r[0].ylo,r[0].yhi))
			return 0;
		if( nr == 2 )
		{
			if( nrr )
			{
				RECT ir,irr;
				ir.left =	r[1].xlo;
				ir.right =	r[1].xhi;
				ir.bottom = r[1].ylo;
				ir.top =	r[1].yhi;
				irr.left =	rr[0].xlo;
				irr.right =	rr[0].xhi;
				irr.bottom =rr[0].ylo;
				irr.top =	rr[0].yhi;
				if (RectsIntersection(ir,irr) >= 0)
					return 0;
				if( nrr == 2 )
				{
					irr.left =	rr[1].xlo;
					irr.right =	rr[1].xhi;
					irr.bottom =rr[1].ylo;
					irr.top =	rr[1].yhi;
					if (RectsIntersection(ir,irr) >= 0)
						return 0;
				}
			}
			else if (InRange(x2,r[1].xlo,r[1].xhi) && InRange(y2,r[1].ylo,r[1].yhi))
				return 0;
		}
	}
	else if( nrr )   
	{
		if (InRange(x1,rr[0].xlo,rr[0].xhi) && InRange(y1,rr[0].ylo,rr[0].yhi))
			return 0;
		if( nrr == 2 )
			if (InRange(x1,rr[1].xlo,rr[1].xhi) && InRange(y1,rr[1].ylo,rr[1].yhi))
				return 0;
	}

	// now find distance from every element of pad1 to every element of pad2
	for( int ic=0; ic<nc; ic++ )
	{
		for( int icc=0; icc<ncc; icc++ )
		{
			int d = Distance( c[ic].x, c[ic].y, cc[icc].x, cc[icc].y ) - c[ic].r - cc[icc].r;
			dist = min(dist,d);
		}
		for( int iss=0; iss<nss; iss++ )
		{
			int d = GetPointToLineSegmentDistance( c[ic].x, c[ic].y, 
						ss[iss].xi, ss[iss].yi, ss[iss].xf, ss[iss].yf ) - c[ic].r;
			dist = min(dist,d);
		}
	}
	for( int is=0; is<ns; is++ )
	{
		for( int icc=0; icc<ncc; icc++ )
		{
			int d = GetPointToLineSegmentDistance( cc[icc].x, cc[icc].y, 
						s[is].xi, s[is].yi, s[is].xf, s[is].yf ) - cc[icc].r;
			dist = min(dist,d);
		}
		for( int iss=0; iss<nss; iss++ )
		{
			double d;
			TestForIntersectionOfStraightLineSegments( s[is].xi, s[is].yi, s[is].xf, s[is].yf,
						ss[iss].xi, ss[iss].yi, ss[iss].xf, ss[iss].yf, NULL, NULL, &d );
			dist = min(dist,d);
		}
	}
	return max(dist,0);
}

// Get min. distance from (x,y) to line y = a + bx
// if b > DBL_MAX/10, assume vertical line at x = a
// returns closest point on line in xp, yp
//
double GetPointToLineDistance( double a, double b, int x, int y, double * xpp, double * ypp )
{
	if( b > DBL_MAX/10 )
	{
		// vertical line
		if( xpp && ypp )
		{
			*xpp = a;
			*ypp = y;
		}
		return abs(a-x);
	}
	// find c,d such that (x,y) lies on y = c + dx where d=(-1/b)
	double d = -1.0/b;
	double c = (double)y-d*x;
	// find nearest point to (x,y) on line through (xi,yi) to (xf,yf)
	double xp = (a-c)/(d-b);
	double yp = a + b*xp;
	if( xpp && ypp )
	{
		*xpp = xp;
		*ypp = yp;
	}
	// find distance
	return Distance( x, y, xp, yp );
}

// Get distance between line segment and point
// enter with:	x,y = point
//				(xi,yi) and (xf,yf) are the end-points of the line segment
//
double GetPointToLineSegmentDistance( int x, int y, int xi, int yi, int xf, int yf )
{
	// test for vertical or horizontal segment
	if( xf==xi )
	{
		// vertical line segment
		if( InRange( y, yi, yf ) )
			return abs( x - xi );
		else
			return min( Distance( x, y, xi, yi ), Distance( x, y, xf, yf ) );
	}
	else if( yf==yi )
	{
		// horizontal line segment
		if( InRange( x, xi, xf ) )
			return abs( y - yi );
		else
			return min( Distance( x, y, xi, yi ), Distance( x, y, xf, yf ) );
	}
	else
	{
		// oblique segment
		// find a,b such that (xi,yi) and (xf,yf) lie on y = a + bx
		double b = (double)(yf-yi)/(xf-xi);
		double a = (double)yi-b*xi;
		// find c,d such that (x,y) lies on y = c + dx where d=(-1/b)
		double d = -1.0/b;
		double c = (double)y-d*x;
		// find nearest point to (x,y) on line through (xi,yi) to (xf,yf)
		double xp = (a-c)/(d-b);
		double yp = a + b*xp;
		// find distance
		if( InRange( xp, xi, xf ) && InRange( yp, yi, yf ) )
			return Distance( x, y, xp, yp );
		else
			return min( Distance( x, y, xi, yi ), Distance( x, y, xf, yf ) );
	}
}

// test for value within range
//
BOOL InRange( double x, double xi, double xf )
{
	if( xf>xi )
	{
		if( x >= xi && x <= xf )
			return TRUE;
	}
	else
	{
		if( x >= xf && x <= xi )
			return TRUE;
	}
	return FALSE;
}

// Get distance between 2 points
//
double Distance( int x1, int y1, int x2, int y2 )
{
	double d;
	d = sqrt( (double)(x1-x2)*(double)(x1-x2) + (double)(y1-y2)*(double)(y1-y2) );
	if( d > INT_MAX || d < INT_MIN )
		return INT_MAX;
	return (int)d;
}

// this finds approximate solutions
// note: this works best if el2 is smaller than el1
//
int GetArcIntersections( EllipseKH * el1, EllipseKH * el2, 
						double * x1, double * y1, double * x2, double * y2 )						
{
	if( el1->theta2 > el1->theta1 )
		ASSERT(0);
	if( el2->theta2 > el2->theta1 )
		ASSERT(0);

	const int NSTEPS = 32;
	double xret[2], yret[2];

	double xscale = 1.0/el1->xrad;
	double yscale = 1.0/el1->yrad;
	// now transform params of second ellipse into reference frame
	// with origin at center if first ellipse, 
	// scaled so the first ellipse is a circle of radius = 1.0
	double xo = (el2->Center.X - el1->Center.X)*xscale;
	double yo = (el2->Center.Y - el1->Center.Y)*yscale;
	double xr = el2->xrad*xscale;
	double yr = el2->yrad*yscale;
	// now test NSTEPS positions in arc, moving clockwise (ie. decreasing theta)
	double step = M_PI/((NSTEPS-1)*2.0);
	double d_prev, th_prev;
	double th_interp;
	double th1;
	int n = 0;
	for( int i=0; i<NSTEPS; i++ )
	{
		double theta;
		if( i < NSTEPS-1 )
			theta = el2->theta1 - i*step;
		else
			theta = el2->theta2;
		double x = xo + xr*cos(theta);
		double y = yo + yr*sin(theta);
		double d = 1.0 - sqrt(x*x + y*y);
		if( i>0 )
		{
			BOOL bInt = FALSE;
			if( d >= 0.0 && d_prev <= 0.0 )
			{
				th_interp = theta + (step*(-d_prev))/(d-d_prev);
				bInt = TRUE;
			}
			else if( d <= 0.0 && d_prev >= 0.0 )
			{
				th_interp = theta + (step*d_prev)/(d_prev-d);
				bInt = TRUE;
			}
			if( bInt )
			{
				x = xo + xr*cos(th_interp);
				y = yo + yr*sin(th_interp);
				th1 = atan2( y, x );
				if( th1 <= el1->theta1 && th1 >= el1->theta2 )
				{
					if( n < 2 )
					{
						xret[n] = x*el1->xrad + el1->Center.X;
						yret[n] = y*el1->yrad + el1->Center.Y;
						n++;
					}
				}
			}
		}
		d_prev = d;
		th_prev = theta;
	}
	if( x1 )
		*x1 = xret[0];
	if( y1 )
		*y1 = yret[0];
	if( x2 )
		*x2 = xret[1];
	if( y2 )
		*y2 = yret[1];
	return n;
}

// this finds approximate solution
//
//double GetSegmentClearance( EllipseKH * el1, EllipseKH * el2, 
double GetArcClearance( EllipseKH * el1, EllipseKH * el2, 
					 double * x1, double * y1 )						
{
	const int NSTEPS = 32;

	if( el1->theta2 > el1->theta1 )
		ASSERT(0);
	if( el2->theta2 > el2->theta1 )
		ASSERT(0);

	// test multiple positions in both arcs, moving clockwise (ie. decreasing theta)
	double th_start = el1->theta1;
	double th_end = el1->theta2;
	double th_start2 = el2->theta1;
	double th_end2 = el2->theta2;
	double dmin = DBL_MAX;
	double xmin, ymin, thmin, thmin2;

	int nsteps = NSTEPS;
	int nsteps2 = NSTEPS;
	double step = (th_start-th_end)/(nsteps-1);
	double step2 = (th_start2-th_end2)/(nsteps2-1);
	while( (step * max(el1->xrad, el1->yrad)) > 1.0*NM_PER_MIL 
		&& (step2 * max(el2->xrad, el2->yrad)) > 1.0*NM_PER_MIL )
	{
		step = (th_start-th_end)/(nsteps-1);
		for( int i=0; i<nsteps; i++ )
		{
			double theta;
			if( i < nsteps-1 )
				theta = th_start - i*step;
			else
				theta = th_end;
			double x = el1->Center.X + el1->xrad*cos(theta);
			double y = el1->Center.Y + el1->yrad*sin(theta);
			step2 = (th_start2-th_end2)/(nsteps2-1);
			for( int i2=0; i2<nsteps2; i2++ )
			{
				double theta2;
				if( i2 < nsteps2-1 )
					theta2 = th_start2 - i2*step2;
				else
					theta2 = th_end2;
				double x2 = el2->Center.X + el2->xrad*cos(theta2);
				double y2 = el2->Center.Y + el2->yrad*sin(theta2);
				double d = Distance( x, y, x2, y2 );
				if( d < dmin )
				{
					dmin = d;
					xmin = x;
					ymin = y;
					thmin = theta;
					thmin2 = theta2;
				}
			}
		}
		if( step > step2 )
		{
			th_start = min(el1->theta1, thmin + step);
			th_end = max(el1->theta2, thmin - step);
			step = (th_start - th_end)/nsteps;
		}
		else
		{
			th_start2 = min(el2->theta1, thmin2 + step2);
			th_end2 = max(el2->theta2, thmin2 - step2);
			step2 = (th_start2 - th_end2)/nsteps2;
		}
	}
	if( x1 )
		*x1 = xmin;
	if( y1 )
		*y1 = ymin;
	return dmin;
}

void SetGuidFromString( CString * str, GUID * guid  )
{
	unsigned char x[80];
	strcpy( (char*)x, *str );
	::UuidFromString( x, guid );
}

void GetStringFromGuid( GUID * guid, CString * str )
{
	unsigned char x[80];
	unsigned char * y = x;
	::UuidToString( guid, &y );
	*str = y;
}

// split string at first instance of split_at
// return TRUE if split_at found, set a and b
// return FALSE if not found, return with a = str, b = ""
//
BOOL SplitString( CString * str, CString * a, CString * b, char split_at, BOOL bReverseFind )
{
	int n;
	CString in_str = *str;
	if( bReverseFind )
		n = in_str.ReverseFind( split_at );
	else
		n = in_str.Find( split_at );
	if( n == -1 )
	{
		if( a ) *a = in_str;
		if( b ) *b = "";
		return FALSE;
	}
	else
	{
		if( a ) *a = in_str.Left(n);;
		if( b ) *b = in_str.Right(in_str.GetLength()-n-1);
		return TRUE;
	}
}

// Get angle of part as reported in status line, corrected for centroid angle and side
//
int GetReportedAngleForPart( int part_angle, int cent_angle, int side )
{
	int angle = (360 + part_angle - cent_angle) % 360; 
	if( side )
		angle = (angle + 180) % 360;
	return ccw(angle);
}

// Returns angle of part as reported in status line, corrected for centroid angle and side
//
int GetPartAngleForReportedAngle( int angle, int cent_angle, int side )
{
	int a = ccw(angle);
	if( side )
		a = (a + 180)%360;
	a = (a + cent_angle) % 360;  
	return a;
}

int sign(int thing)
{
	if(thing == 0) return  0;
	if(thing <  0) return -1;
	return 1;
}

