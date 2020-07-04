// file PcbFont.h : header file for CPcbFont class
//
#pragma once

#define MAX_CHAR_STROKES 20
#define MAX_CHAR 128

struct font_stroke  
{
	double xi;
	double yi;
	double xf;
	double yf;
};    

struct font_char
{
	int n_strokes;
	font_stroke stroke[MAX_CHAR_STROKES];
};

class CPcbFont
{
public:
	CPcbFont( char * fn );
	~CPcbFont();
	int GetNumStrokes( char symbol );
	int GetStroke( char symbol, int istroke, int size, int *xi, int *xf, int *yi, int *yf );

private:
	font_char m_ch[MAX_CHAR];
};
