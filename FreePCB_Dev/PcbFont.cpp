// file PcbFont.cpp : implements class CPcbFont
//
#include "stdio"
#include "stdafx.h"
#include "PcbFont.h"

CPcbFont::CPcbFont( char * fn )
{
	FILE *f;
	char str[255], symbol=0;
	int xi, xf, yi, yf, ic;

	f = fopen( fn, "r" );
	ASSERT( f );
	for( int i=0; i<MAX_CHAR; i++ )
		m_ch[i].n_strokes = 0;
	while( !feof(f) )
	{
		fgets( str, 255, f );
		if( !strncmp( str, "Symbol('", 8 ) )
		{
			symbol = str[8];
			ic = 0;
		}
		else if( !strncmp( &str[1], "SymbolLine(", 11 ) )
		{
			sscanf( &str[12], "%d %d %d %d", &xi, &yi, &xf, &yf );
			m_ch[symbol].stroke[m_ch[symbol].n_strokes].xi = xi;
			m_ch[symbol].stroke[m_ch[symbol].n_strokes].xf = xf;
			m_ch[symbol].stroke[m_ch[symbol].n_strokes].yi = yi;
			m_ch[symbol].stroke[m_ch[symbol].n_strokes].yf = yf;
			m_ch[symbol].n_strokes++;
			ASSERT( m_ch[symbol].n_strokes != MAX_CHAR_STROKES );
		}
	}
	fclose( f );
	return;
}

CPcbFont::~CPcbFont()
{
	return;
}

int CPcbFont::GetNumStrokes( char symbol )
{
	return m_ch[symbol].n_strokes;
}

int CPcbFont::GetStroke( char symbol, int istroke, int size, int *xi, int *xf, int *yi, int *yf )
{
	*xi = m_ch[symbol].stroke[istroke].xi * size/55;
	*xf = m_ch[symbol].stroke[istroke].xf * size/55;
	*yi = (55 - m_ch[symbol].stroke[istroke].yi) * size/55;
	*yf = (55 - m_ch[symbol].stroke[istroke].yf) * size/55;
	return 0;
}
