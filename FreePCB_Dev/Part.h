// Part.h : interface for the CShape class
//

#ifndef PART_H
#define PART_H

#include "stdafx.h"

// pad shapes
enum
{
	PAD_NONE = 0,
	PAD_ROUND,
	PAD_SQUARE,
	PAD_RECT,
	PAD_RRECT
};

// error returns
enum
{
	PART_NOERR = 0,
	PART_ERR_TOO_MANY_PINS
};

// structure describing stroke (ie. line segment)
struct stroke
{
	int w, xi, yi, xf, yf;
};

// structure describing pad
struct pad
{
	int shape;	// see enum above
	int size_l, size_r, size_h;	
};

// padstack is pads and hole associated with a pin
struct padstack
{
	int hole_flag;
	int hole_size;
	int x_rel, y_rel;	// position relative to part origin
	int x, y;			// position on board
	int angle;		// orientation: 0=along left side of part, 1=bottom, 2=right, 3=top
	pad top;
	pad bottom;
	pad inner;
};

class CShape
{
	// if variables are added, remember to modify Copy!
public:
	char m_name[255];
	int m_sel_x, m_sel_y, m_sel_w, m_sel_h;				// selection rectangle
	int m_ref_size, m_ref_xi, m_ref_yi, m_ref_angle;	// ref text
	int m_num_pins;				// number of pins
	padstack * m_padstack;		// pointer to array of padstacks
	int m_num_outline_strokes;	// number of strokes in outline
	stroke * m_outline_stroke;	// pointer to array of strokes for outline

public:
	CShape( char *name, int num_pins );
	~CShape();
	SetSMTPadstack( int pin, int shape, int size_h, int size_l, int size_r );
	SetThruholePadstack( int pin, int shape, int size_h, int size_l, int size_r, int hole_size );
	SetThruholeTopPad( int pin, int shape, int size_h, int size_l, int size_r );
	SetThruholeInnerPad( int pin, int shape, int size_h, int size_l, int size_r );
	GetNumPins();
	Copy( CShape * part );	// copy all data including padstacks
};

#endif // PART_H