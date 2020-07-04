// CShape.cpp : implementation of CShape class
//
#include "stdafx.h"
#include "Shape.h"
#include "FreePcb.h"

// constructors
//
// this constructor creates part with desired name and number of pins
// assigns default parameters for a DIP package
//
CShape::CShape( char *name, int num_pins )
{
	// set name
	strcpy( m_name, name );
	// create selection box
	m_sel_x = -50*PCBU_PER_MIL;
	m_sel_y = -50*PCBU_PER_MIL;
	m_sel_w = ((num_pins/2-1)*100 + 100)*PCBU_PER_MIL;
	m_sel_h = (300 + 100)*PCBU_PER_MIL;
	// create ref text box
	m_ref_size = 50*PCBU_PER_MIL;
	m_ref_xi = 100*PCBU_PER_MIL;
	m_ref_yi = 100*PCBU_PER_MIL;
	m_ref_angle = 0;
	m_ref_w = 10*PCBU_PER_MIL;
	// create part outline
	m_num_outline_strokes = 8;
	m_outline_stroke = (stroke*)calloc( m_num_outline_strokes, sizeof(stroke) );
	stroke s;
	s.xi = 0*PCBU_PER_MIL; s.yi = 50*PCBU_PER_MIL; 
	s.xf = (num_pins/2-1)*100*PCBU_PER_MIL; s.yf = 50*PCBU_PER_MIL;
	m_outline_stroke[0] = s;
	s.xi = (num_pins/2-1)*100*PCBU_PER_MIL; s.yi = 50*PCBU_PER_MIL; 
	s.xf = (num_pins/2-1)*100*PCBU_PER_MIL; s.yf = 250*PCBU_PER_MIL;
	m_outline_stroke[1] = s;
	s.xi = (num_pins/2-1)*100*PCBU_PER_MIL; s.yi = 250*PCBU_PER_MIL; 
	s.xf = 0*PCBU_PER_MIL; s.yf = 250*PCBU_PER_MIL;
	m_outline_stroke[2] = s;
	s.xi = 0*PCBU_PER_MIL; s.yi = 250*PCBU_PER_MIL; 
	s.xf = 0*PCBU_PER_MIL; s.yf = 175*PCBU_PER_MIL;
	m_outline_stroke[3] = s;
	s.xi = 0*PCBU_PER_MIL; s.yi = 175*PCBU_PER_MIL; 
	s.xf = 30*PCBU_PER_MIL; s.yf = 175*PCBU_PER_MIL;
	m_outline_stroke[4] = s;
	s.xi = 30*PCBU_PER_MIL; s.yi = 175*PCBU_PER_MIL; 
	s.xf = 30*PCBU_PER_MIL; s.yf = 125*PCBU_PER_MIL;
	m_outline_stroke[5] = s;
	s.xi = 30*PCBU_PER_MIL; s.yi = 125*PCBU_PER_MIL; 
	s.xf = 0*PCBU_PER_MIL; s.yf = 125*PCBU_PER_MIL;
	m_outline_stroke[6] = s;
	s.xi = 0*PCBU_PER_MIL; s.yi = 125*PCBU_PER_MIL; 
	s.xf = 0*PCBU_PER_MIL; s.yf = 50*PCBU_PER_MIL;
	m_outline_stroke[7] = s;
	// create array of padstacks
	m_num_pins = num_pins;
	m_padstack = (padstack*)calloc( num_pins, sizeof(padstack) );
	for( int i=0; i<num_pins; i++ )
	{
		m_padstack[i].hole_flag = 1;
		m_padstack[i].hole_size = 30*PCBU_PER_MIL;
		m_padstack[i].angle = 1;
		m_padstack[i].y_rel = 0*PCBU_PER_MIL;
		m_padstack[i].x_rel = i*100*PCBU_PER_MIL;
		if( i>=(num_pins/2) )
		{
			m_padstack[i].angle = 3;
			m_padstack[i].y_rel = 300*PCBU_PER_MIL;
			m_padstack[i].x_rel = (num_pins-i-1)*100*PCBU_PER_MIL;
		}
		if( i==0 )
		{
			m_padstack[i].top.shape = PAD_SQUARE;
			m_padstack[i].top.size_h = 50*PCBU_PER_MIL;
			m_padstack[i].bottom.shape = PAD_SQUARE;
			m_padstack[i].bottom.size_h = 50*PCBU_PER_MIL;
			m_padstack[i].inner.shape = PAD_NONE;
			m_padstack[i].inner.size_h = 0*PCBU_PER_MIL;
		}
		else
		{
			m_padstack[i].top.shape = PAD_ROUND;
			m_padstack[i].top.size_h = 50*PCBU_PER_MIL;
			m_padstack[i].bottom.shape = PAD_ROUND;
			m_padstack[i].bottom.size_h = 50*PCBU_PER_MIL;
			m_padstack[i].inner.shape = PAD_NONE;
			m_padstack[i].inner.size_h = 0*PCBU_PER_MIL;
		}
	}
}

// destructor
//
CShape::~CShape()
{
	free( m_padstack );
	free( m_outline_stroke );
}

// copy part
//
CShape::Copy( CShape * part )
{
	strcpy( m_name, part->m_name );
	m_sel_x = part->m_sel_x;
	m_sel_y = part->m_sel_y;
	m_sel_w = part->m_sel_w;
	m_sel_h = part->m_sel_h;
	m_ref_size;
	m_ref_xi = part->m_ref_xi;
	m_ref_yi = part->m_ref_yi;
	m_ref_angle = part->m_ref_angle;
	m_num_pins = part->m_num_pins;
	if( m_padstack )
		free( m_padstack );
	m_padstack = (padstack*)calloc( m_num_pins, sizeof(padstack) );
	for( int i=0; i<m_num_pins; i++ )
		m_padstack[i] = part->m_padstack[i];
	m_num_outline_strokes = part->m_num_outline_strokes;
	if( m_outline_stroke )
		free( m_outline_stroke );
	m_outline_stroke = (stroke*)calloc( m_num_outline_strokes, sizeof(stroke) );
	for( i=0; i<m_num_outline_strokes; i++ )
		m_outline_stroke[i] = part->m_outline_stroke[i];
}

// set up SMT padstack
//
CShape::SetSMTPadstack( int pin, int shape, int size_h, int size_l, int size_r )
{
	if( pin < 1 || pin > m_num_pins )
		return PART_ERR_TOO_MANY_PINS;
	m_padstack[pin-1].hole_flag = 0;
	m_padstack[pin-1].top.shape = shape;
	m_padstack[pin-1].top.size_h = size_h;
	m_padstack[pin-1].top.size_l = size_l;
	m_padstack[pin-1].top.size_r = size_r;
	return PART_NOERR;
}

// set up padstack for through-hole pin (bottom pad only)
//
CShape::SetThruholePadstack( int pin, int shape, int size_h, int size_l, int size_r, int hole_size )
{
	if( pin < 1 || pin > m_num_pins )
		return PART_ERR_TOO_MANY_PINS;
	m_padstack[pin-1].hole_flag = 1;
	m_padstack[pin-1].hole_size = hole_size;
	m_padstack[pin-1].bottom.shape = shape;
	m_padstack[pin-1].bottom.size_h = size_h;
	m_padstack[pin-1].bottom.size_l = size_l;
	m_padstack[pin-1].bottom.size_r = size_r;
	m_padstack[pin-1].inner.shape = PAD_NONE;
	m_padstack[pin-1].top.shape = PAD_NONE;
	return PART_NOERR;
}

// set top pad for through-hole pin
//
CShape::SetThruholeTopPad( int pin, int shape, int size_h, int size_l, int size_r )
{
	if( pin < 1 || pin > m_num_pins )
		return PART_ERR_TOO_MANY_PINS;
	m_padstack[pin-1].top.shape = shape;
	m_padstack[pin-1].top.size_h = size_h;
	m_padstack[pin-1].top.size_l = size_l;
	m_padstack[pin-1].top.size_r = size_r;
	return PART_NOERR;
}

// set inner pad for through-hole pin
//
CShape::SetThruholeInnerPad( int pin, int shape, int size_h, int size_l, int size_r )
{
	if( pin < 1 || pin > m_num_pins )
		return PART_ERR_TOO_MANY_PINS;
	m_padstack[pin-1].inner.shape = shape;
	m_padstack[pin-1].inner.size_h = size_h;
	m_padstack[pin-1].inner.size_l = size_l;
	m_padstack[pin-1].inner.size_r = size_r;
	return PART_NOERR;
}

CShape::GetNumPins()
{
	return m_num_pins;
}

