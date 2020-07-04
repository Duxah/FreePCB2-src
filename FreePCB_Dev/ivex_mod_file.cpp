// functions to convert Ivex .mod file to FreePCB .lib file
//
#include "stdafx.h"
#include "file_io.h"
#include "Shape.h"
#include "ivex_mod_file.h"
#include "math.h"

// define layer masks
enum {
	LAY_T = 0x1,
	LAY_I = 0x2,
	LAY_B = 0x4,
	LAY_C = 0x8,
	LAY_P = 0x10
};

// structures to hold padstack data from mod file
struct pad_stack_entry {
	CString shape;
	int pad_shape;
	int orientation;
	int x_wid, y_wid;
	int x_off, y_off;
	int layer_mask;
};
struct mod_padstack {
	CString name;
	int hole_diam;		// units = NM
	CArray<pad_stack_entry> entry;
};
typedef mod_padstack ps;

ps * current_ps;

// FreePCB shape

// this is the main conversion function
//
int ConvertIvex( CStdioFile * mod_file, CStdioFile * lib_file, CEdit * edit_ctrl )
{
	// state-machine
	enum { IDLE,	// idle (i.e. no state)
		PAD_STACK,	// parsing pad stack
		MODULE		// parsing module
	};

	CArray<mtg_hole> m_mtg_hole;	
	CShape s;
	s.m_padstack.SetSize(0);
	s.m_outline_poly.SetSize(0);
	m_mtg_hole.SetSize(0);
	s.m_author = "Ivex";

	int min_x;
	int max_x;
	int min_y;
	int max_y;

	#define pi  3.14159265359	
	double sin45 = sin( pi/4.0 );
	double cos45 = sin45;
	double sin22 = sin( pi/8.0 );
	double cos22 = cos( pi/8.0 );
	double sin67 = sin( 3.0*pi/8.0 );
	double cos67 = cos( 3.0*pi/8.0 );

	// state
	int state = IDLE;
	int units = MIL;
	int mult = NM_PER_MIL;
	CString instr, keystr, subkey;
	CArray<CString> p;
	mod_padstack * new_ps;
	int current_x, current_y;
	int current_angle;

	// map of pad identifiers
	CMapStringToPtr mod_padstack_map;

	// start reading mod file
	int ret = mod_file->ReadString( instr );
	instr.Trim();
	while( ret )
	{
		instr.MakeUpper();
		if( instr.Left(10).MakeUpper() == "// PACKAGE" )
		{
			// get source of footprint
			CString pck = instr.Right( instr.GetLength() - 9);
			pck.Remove( '\"' );
			pck.Trim();
			s.m_package = pck;
		}
		int np = ParseKeyString( &instr, &keystr, &p ); 
		if( np )
		{
			// now handle incoming line
			if( state == IDLE && keystr == "UNITS" )
			{
				subkey = p[0];
				if( subkey == "MIL" )
				{
					units = MIL;
					mult = NM_PER_MIL;
				}
				else if( subkey == "MM" )
				{
					units = MM;
					mult = 1000000;
				}
				else if( subkey == "NM" )
				{
					units = NM;
					mult = 1;
				}
				else if( subkey == "MICRON" )
				{
					units = MM;
					mult = 1000;
				}
				else
					ASSERT(0);
			}
			else if( state == IDLE && keystr == "PAD" && p[0] == "STACK" )
			{
				// parse PAD STACK
				state = PAD_STACK;
				CString name = p[1];
				double hole_diam = my_atof(&p[2]);
				if( p[3] != "{" )
					ASSERT(0);
				// store it in map
				new_ps = new mod_padstack;//???
				new_ps->name = name;
				new_ps->hole_diam = hole_diam * mult;
				mod_padstack_map.SetAt( name, new_ps );
			}
			else if( state == PAD_STACK && ( keystr == "OVAL" || keystr == "RECT" ) )
			{
				// parse PAD STACK entry
				int i = new_ps->entry.GetSize();
				new_ps->entry.SetSize( i+1 );
				new_ps->entry[i].shape = keystr;
				new_ps->entry[i].pad_shape = PAD_NONE;
				if( keystr == "OVAL" )
					new_ps->entry[i].pad_shape = PAD_ROUND;
				else if( keystr == "RECT" )
					new_ps->entry[i].pad_shape = PAD_RECT;
				else
					ASSERT(0);
				new_ps->entry[i].orientation = my_atof(&p[0]);
				new_ps->entry[i].x_wid = my_atof(&p[1])*mult;
				new_ps->entry[i].y_wid = my_atof(&p[2])*mult;
				if( new_ps->entry[i].x_wid == new_ps->entry[i].y_wid && new_ps->entry[i].pad_shape == PAD_RECT )
					new_ps->entry[i].pad_shape = PAD_SQUARE;	
				new_ps->entry[i].x_off = my_atof(&p[3])*mult;
				new_ps->entry[i].y_off = my_atof(&p[4])*mult;
				CString layers = p[5];
				if( np >= 8 )
					layers += p[6];
				if( np >= 9 )
					layers += p[7];
				new_ps->entry[i].layer_mask = 0;
				if( layers.Find('T') != -1 )
					new_ps->entry[i].layer_mask |= LAY_T;
				if( layers.Find('I') != -1 )
					new_ps->entry[i].layer_mask |= LAY_I;
				if( layers.Find('B') != -1 )
					new_ps->entry[i].layer_mask |= LAY_B;
				if( layers.Find('C') != -1 )
					new_ps->entry[i].layer_mask |= LAY_C;
				if( layers.Find('P') != -1 )
					new_ps->entry[i].layer_mask |= LAY_P;
			}
			else if( state == PAD_STACK && keystr == "}" )
			{
				state = IDLE;
			}
			else if( state == IDLE && keystr == "BEGIN" )
			{
				// start new module (i.e. footprint) definition
				state = MODULE;
				s.m_units = units;
				s.m_padstack.SetSize(0);
				s.m_outline_poly.SetSize(0);
				m_mtg_hole.SetSize(0);
				// default ref text params
				s.m_ref_size = 50 * NM_PER_MIL;
				s.m_ref_xi = 50 * NM_PER_MIL;
				s.m_ref_yi = 50 * NM_PER_MIL;
				s.m_ref_angle = 0;
				s.m_ref_w = 7 * NM_PER_MIL;
				// track min and max dimensions to create selection rectangle
				min_x = INT_MAX;
				max_x = INT_MIN;
				min_y = INT_MAX;
				max_y = INT_MIN;
			}
			else if( state == MODULE && keystr == "END" )
			{
				// end module def.
				state = IDLE;
				// check for all pins defined
				BOOL error = FALSE;
				for( int ip=0; ip<s.m_padstack.GetSize(); ip++ )
				{
					if( !s.m_padstack[ip].exists )
					{
						error = TRUE;
						break;
					}
				}
				if( !error )
				{
					// convert mounting holes to pads
					int ip = s.m_padstack.GetSize();
					s.m_padstack.SetSize( ip + m_mtg_hole.GetSize() );
					for( int ih=0; ih<m_mtg_hole.GetSize(); ih++ )
					{
						s.m_padstack[ip].exists = 1;
						s.m_padstack[ip].name.Format( "MH%d", ih+1 );
						s.m_padstack[ip].hole_size = m_mtg_hole[ih].diam;
						s.m_padstack[ip].x_rel = m_mtg_hole[ih].x;
						s.m_padstack[ip].y_rel = m_mtg_hole[ih].y;
						s.m_padstack[ip].angle = 0;
						s.m_padstack[ip].top.shape = m_mtg_hole[ih].pad_shape;
						s.m_padstack[ip].top.size_Y = m_mtg_hole[ih].pad_diam;
						s.m_padstack[ip].top.size_X = 0;
						int inner_pad_diam;
						if( m_mtg_hole[ih].pad_shape == PAD_NONE )
							s.m_padstack[ip].inner.shape = PAD_NONE;
						else
						{
							s.m_padstack[ip].inner.shape = PAD_ROUND;
							inner_pad_diam = m_mtg_hole[ih].diam + 20*NM_PER_MIL;
							if( inner_pad_diam < m_mtg_hole[ih].pad_diam )
								s.m_padstack[ip].inner.size_Y = inner_pad_diam;
							else
								s.m_padstack[ip].inner.size_Y = m_mtg_hole[ih].pad_diam;
							s.m_padstack[ip].inner.size_X = 0;
						}
						s.m_padstack[ip].bottom.shape = m_mtg_hole[ih].pad_shape;
						s.m_padstack[ip].bottom.size_Y = m_mtg_hole[ih].pad_diam;
						s.m_padstack[ip].bottom.size_X = 0;
						ip++;
					}
					m_mtg_hole.RemoveAll();
					// make sure selection rectangle surrounds pads
					for( int ip=0; ip<s.m_padstack.GetSize(); ip++ )
					{
						int x = s.m_padstack[ip].x_rel;
						int y = s.m_padstack[ip].y_rel;
						int dx = s.m_padstack[ip].top.size_Y/2;
						int dy = s.m_padstack[ip].top.size_Y/2;
						if( (x+dx) > max_x )
							max_x = x + dx;
						if( (x-dx) < min_x )
							min_x = x - dx;
						if( (y+dy) > max_y )
							max_y = y + dy;
						if( (y-dy) < min_y )
							min_y = y - dy;
					}
					s.selection.left =	min_x - 8*NM_PER_MIL;
					s.selection.bottom =min_y - 8*NM_PER_MIL;
					s.selection.right = max_x + 8*NM_PER_MIL;
					s.selection.top =	max_y + 8*NM_PER_MIL;
					s.WriteFootprint( lib_file );
				}
				else
				{
					CString str;
					str.Format( "    missing pins, aborting conversion\r\n" );
					edit_ctrl->ReplaceSel( str );
				}
			}
			else if( state == MODULE && keystr == "NAME" )
			{
				CString name = p[0];
				s.m_name = name;
				CString str;
				str.Format( "  converting part \"%s\"\r\n", name );
				edit_ctrl->ReplaceSel( str );
			}
			else if( state == MODULE && keystr == "OUTLINE" )
			{
				if( p[0] == "BOX" )
				{
					int xi = my_atof(&p[1]) * mult;
					int yi = -(my_atof(&p[2]) * mult);
					int xf = my_atof(&p[3]) * mult;
					int yf = -(my_atof(&p[4]) * mult);
					int np = s.m_outline_poly.GetSize();
					s.m_outline_poly.SetSize(np+1);
					s.m_outline_poly[np].Start( 0, 7*NM_PER_MIL, 0, xi, yi, 0, NULL, NULL );
					s.m_outline_poly[np].AppendCorner( xf, yi );
					s.m_outline_poly[np].AppendCorner( xf, yf );
					s.m_outline_poly[np].AppendCorner( xi, yf );
					s.m_outline_poly[np].Close();
					if( xi > max_x )
						max_x = xi;
					if( xi < min_x )
						min_x = xi;
					if( yi > max_y )
						max_y = yi;
					if( yi < min_y )
						min_y = yi;
					if( xf > max_x )
						max_x = xf;
					if( xf < min_x )
						min_x = xf;
					if( yf > max_y )
						max_y = yf;
					if( yf < min_y )
						min_y = yf;
				}
				else if( p[0] == "LINE" )
				{
					int xi = my_atof(&p[1]) * mult;
					int yi = -(my_atof(&p[2]) * mult);
					int xf = my_atof(&p[3]) * mult;
					int yf = -(my_atof(&p[4]) * mult);
					int np = s.m_outline_poly.GetSize();
					s.m_outline_poly.SetSize(np+1);
					s.m_outline_poly[np].Start( 0, 7*NM_PER_MIL, 0, xi, yi, 0, NULL, NULL );
					s.m_outline_poly[np].AppendCorner( xf, yf );
					if( xi > max_x )
						max_x = xi;
					if( xi < min_x )
						min_x = xi;
					if( yi > max_y )
						max_y = yi;
					if( yi < min_y )
						min_y = yi;
					if( xf > max_x )
						max_x = xf;
					if( xf < min_x )
						min_x = xf;
					if( yf > max_y )
						max_y = yf;
					if( yf < min_y )
						min_y = yf;
				}
				else if( p[0] == "ARC" )
				{
					// make an arc with a polyline
					int xc = my_atof(&p[1]) * mult;
					int yc = -(my_atof(&p[2]) * mult);
					int r = my_atof(&p[3]) * mult;
					int quadrant = my_atoi(&p[4]);
					int np = s.m_outline_poly.GetSize();
					s.m_outline_poly.SetSize(np+1);
					if( quadrant == 1 )
					{
						s.m_outline_poly[np].Start( 0, 7*NM_PER_MIL, 0, xc+r, yc, 0, NULL, NULL );
						s.m_outline_poly[np].AppendCorner( xc, yc+r, CPolyLine::ARC_CCW );
					}
					else if( quadrant == 2 )
					{
						s.m_outline_poly[np].Start( 0, 7*NM_PER_MIL, 0, xc, yc+r, 0, NULL, NULL );
						s.m_outline_poly[np].AppendCorner( xc-r, yc, CPolyLine::ARC_CCW);
					}
					else if( quadrant == 3 )
					{
						s.m_outline_poly[np].Start( 0, 7*NM_PER_MIL, 0, xc-r, yc, 0, NULL, NULL );
						s.m_outline_poly[np].AppendCorner( xc, yc-r, CPolyLine::ARC_CCW );
					}
					else
					{
						s.m_outline_poly[np].Start( 0, 7*NM_PER_MIL, 0, xc, yc-r, 0, NULL, NULL );
						s.m_outline_poly[np].AppendCorner( xc+r, yc, CPolyLine::ARC_CCW );
					}
					if( xc+r > max_x )
						max_x = xc+r;
					if( xc-r < min_x )
						min_x = xc-r;
					if( yc+r > max_y )
						max_y = yc+r;
					if( yc-r < min_y )
						min_y = yc-r;
				}
				else if( p[0] == "CIRCLE" )
				{
					// make a circle with a polyline
					int xc = my_atof(&p[1]) * mult;
					int yc = -(my_atof(&p[2]) * mult);
					int r = my_atof(&p[3]) * mult;
					int np = s.m_outline_poly.GetSize();
					s.m_outline_poly.SetSize(np+1);
					s.m_outline_poly[np].Start( 0, 7*NM_PER_MIL, 0, xc+r, yc, 0, NULL, NULL );
					s.m_outline_poly[np].AppendCorner( xc, yc+r, CPolyLine::ARC_CCW );
					s.m_outline_poly[np].AppendCorner( xc-r, yc, CPolyLine::ARC_CCW );
					s.m_outline_poly[np].AppendCorner( xc, yc-r, CPolyLine::ARC_CCW );
					s.m_outline_poly[np].Close( CPolyLine::ARC_CCW );
					if( xc+r > max_x )
						max_x = xc+r;
					if( xc-r < min_x )
						min_x = xc-r;
					if( yc+r > max_y )
						max_y = yc+r;
					if( yc-r < min_y )
						min_y = yc-r;
				}
			}
			else if( state == MODULE && keystr == "MOUNTING" )
			{
				// mounting hole
				int ih = m_mtg_hole.GetSize();
				m_mtg_hole.SetSize( ih+1 );
				if( p[0] == "EMPTY" )
					m_mtg_hole[ih].pad_shape = PAD_NONE;
				else if( p[0] == "SQUARE" )
					m_mtg_hole[ih].pad_shape = PAD_SQUARE;
				else if( p[0] == "ROUND" )
					m_mtg_hole[ih].pad_shape = PAD_ROUND;
				else
					ASSERT(0);
				int x = my_atof( &p[1])*mult; 
				int y = my_atof( &p[2])*mult; 
				int diam = my_atof( &p[3])*mult; 
				int pad_diam = my_atof( &p[4])*mult;	// only used for ROUND or SQUARE
				m_mtg_hole[ih].x = x;
				m_mtg_hole[ih].y = y;
				m_mtg_hole[ih].diam = diam;
				m_mtg_hole[ih].pad_diam = pad_diam;
				int d = diam;
				if( pad_diam > diam )
					d = pad_diam;
				int r = d/2;
				if( x+r > max_x )
					max_x = x+r;
				if( x-r < min_x )
					min_x = x-r;
				if( y+r > max_y )
					max_y = y+r;
				if( y-r < min_y )
					min_y = y-r;
			}
			else if( state == MODULE && keystr == "TEXT" )
			{
				if( p[7] == "REF" )
				{
					int x = my_atof(&p[0]) * mult;
					int y = -(my_atof(&p[1]) * mult);
					int angle = my_atof(&p[2]);
					s.m_ref_size = 50 * NM_PER_MIL;
					s.m_ref_xi = x;
					s.m_ref_yi = y;
					s.m_ref_angle = angle;
					s.m_ref_w = 7 * NM_PER_MIL;
				}
			}
			else if( state == MODULE && keystr == "PAD" 
				&& ( p[0] == "DEF" || p[0] == "STEP" ) )
			{
				// add pad to shape
//				int pin_num;
				CString pin_str;
				if( p[0] == "DEF" )
				{
					current_x = my_atof(&p[1]) * mult;
					current_y = my_atof(&p[2]) * mult;
					current_angle = my_atof(&p[3]);
					CString pad_name = p[4];
					void * ptr;
					BOOL found = mod_padstack_map.Lookup( pad_name, ptr );
					if( !found )
						ASSERT(0);
					current_ps = (mod_padstack *)ptr;
					pin_str = p[5];
				}
				else if( p[0] == "STEP" )
				{
					CString step_str = p[1];
					CString step = step_str.Right( step_str.GetLength()-2 );
					int is = my_atof( &step ) * mult;
					if( step_str.Left(2) == "X+" )
						current_x = current_x + is;
					if( step_str.Left(2) == "X-" )
						current_x = current_x - is;
					if( step_str.Left(2) == "Y+" )
						current_y = current_y + is;
					if( step_str.Left(2) == "Y-" )
						current_y = current_y - is;
					pin_str = p[2];
				}
				if( pin_str.GetLength() )
				{
					int pin_num = s.m_padstack.GetSize() + 1;
					s.m_padstack.SetSize(pin_num);
					s.m_padstack[pin_num-1].exists = TRUE;
					s.m_padstack[pin_num-1].name = pin_str;
					s.m_padstack[pin_num-1].hole_size = current_ps->hole_diam;
					s.m_padstack[pin_num-1].x_rel = current_x;
					s.m_padstack[pin_num-1].y_rel = -(current_y);	// since Ivex reverses y-axis
					s.m_padstack[pin_num-1].angle = current_angle;
					s.m_padstack[pin_num-1].top.shape = PAD_NONE;
					s.m_padstack[pin_num-1].inner.shape = PAD_NONE;
					s.m_padstack[pin_num-1].bottom.shape = PAD_NONE;
					if( current_ps->entry[0].layer_mask & (LAY_T | LAY_C) )
					{
						s.m_padstack[pin_num-1].top.shape = current_ps->entry[0].pad_shape;
						s.m_padstack[pin_num-1].top.size_X = current_ps->entry[0].x_wid;
						s.m_padstack[pin_num-1].top.size_Y = current_ps->entry[0].y_wid;
					}
					if( current_ps->entry[0].layer_mask & LAY_I || current_ps->hole_diam > 0 )
					{
						// inner pads are forced round
						s.m_padstack[pin_num-1].inner.shape = PAD_ROUND;
						s.m_padstack[pin_num-1].inner.size_X = current_ps->entry[0].x_wid;
						s.m_padstack[pin_num-1].inner.size_Y = current_ps->entry[0].y_wid;
					}
					if( current_ps->entry[0].layer_mask & LAY_B )
					{
						s.m_padstack[pin_num-1].bottom.shape = current_ps->entry[0].pad_shape;
						s.m_padstack[pin_num-1].bottom.size_X = current_ps->entry[0].x_wid;
						s.m_padstack[pin_num-1].bottom.size_Y = current_ps->entry[0].y_wid;
					}
				}
				else
				{
					// illegal pin number
					CString str;
					str.Format( "    illegal pin \"%s\", abort conversion\r\n", pin_str );
					edit_ctrl->ReplaceSel( str );
					state = IDLE;
				}
			}
		}
		ret = mod_file->ReadString( instr );
	}
	// now delete map of padstacks
	POSITION pos;
	CString key;
	void * ptr;
	for( pos = mod_padstack_map.GetStartPosition(); pos != NULL; )
	{
		mod_padstack_map.GetNextAssoc( pos, key, ptr );
		delete ptr;
		return 0;
	}
	return 0;
}


