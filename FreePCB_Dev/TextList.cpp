// TextList.cpp ... implementation of class CTextList
//
// class representing text elements for PCB (except reference designators for parts)
//
#include "stdafx.h"
#include "utility.h"
#include "textlist.h"
#include "PcbFont.h"
#include "shape.h"
#include "ids.h"
#include "smfontutil.h"
#include "file_io.h"

// CText constructor
// draws strokes into display list if dlist != 0
//
CText::CText( CDisplayList * dlist, int x, int y, int angle, int mirror,
			BOOL bNegative, int layer, int font_size, int stroke_width, 
			SMFontUtil * smfontutil, CString * str_ptr )
{
	m_guid = GUID_NULL;
	HRESULT hr = ::UuidCreate(&m_guid);
	m_x = x;
	m_y = y;
	m_angle = angle;
	m_mirror = mirror;
	m_bNegative = bNegative;
	m_layer = layer;
	m_stroke_width = stroke_width;
	m_font_size = font_size;
	m_str = *str_ptr;
	m_nchars = str_ptr->GetLength();
	m_dlist = dlist;
	m_selected = 0;
	net_Ptr = NULL;
	if( smfontutil )
		Draw( dlist, smfontutil );
	else
		Undraw();
}

// CText destructor
// removes all dlist elements
//
CText::~CText()
{
	Undraw();
}

// Draw text as a series of strokes
// If dlist == NULL, generate strokes but don't draw into display list
void CText::Draw( CDisplayList * dlist, SMFontUtil * smfontutil, CArray<CPoint> * m_stroke )
{
	// draw text
	m_dlist = dlist;
	id id( ID_TEXT, ID_STROKE );
	CArray<CPoint> strk;
	if( m_stroke == NULL )
		m_stroke = &strk;
	m_stroke->SetSize( 0 );

	if( smfontutil )
	{
		// now draw strokes
		CPoint si, sf;
		double x_scale = (double)m_font_size/22.0;
		double y_scale = (double)m_font_size/22.0;
		double y_offset = 9.0*y_scale;
		double xc = 0.0;
		int l_map=0;
		setbit( l_map, m_layer );
		RECT sel;
		sel.left = sel.bottom = INT_MAX;
		sel.right = sel.top = INT_MIN;
		for( int ic=0; ic<m_nchars; ic++ )
		{
			// get stroke info for character
			int xi, yi, xf, yf;
			double coord[64][4];
			double min_x, min_y, max_x, max_y;
			int nstrokes;
			nstrokes = smfontutil->GetCharStrokes( m_str[ic], SIMPLEX, &min_x, &min_y, &max_x, &max_y,
					coord, 64 );
			if( nstrokes > 0 )
			{
				int sz = m_stroke->GetSize();
				m_stroke->SetSize( sz + 2*nstrokes );
				for( int is=0; is<nstrokes; is++ )
				{
					si.x = (coord[is][0] - min_x)*x_scale + xc;
					si.y = coord[is][1]*y_scale + y_offset;
					sf.x = (coord[is][2] - min_x)*x_scale + xc;
					sf.y = coord[is][3]*y_scale + y_offset;
					// mirror
					if( m_mirror )
					{
						si.x = -si.x;
						sf.x = -sf.x;
					}
					SwellRect( &sel, si.x, si.y );
					SwellRect( &sel, sf.x, sf.y  );
					// rotate
					if( m_angle )
					{
						RotatePoint( &si, -m_angle, zero );
						RotatePoint( &sf, -m_angle, zero );
					}			
					// add x, y and draw
					CPoint pp1( m_x + si.x, m_y + si.y );
					m_stroke->SetAt(sz+(2*is),pp1);
					CPoint pp2( m_x + sf.x, m_y + sf.y );
					m_stroke->SetAt(sz+(2*is)+1,pp2);
				}
				xc += (max_x - min_x + 8.0)*x_scale;
			}
			else
				xc += 16.0*x_scale;
		}
		m_smfontutil = smfontutil;
	
		if( dlist )
		{
			// create selection rectangle
			MoveRect( &sel, m_x, m_y );
			SwellRect( &sel, m_stroke_width/2 );
			int width = xc - 8.0*x_scale;
			CPoint pts[4];
			CPoint c;
			pts[0].x = sel.right;
			pts[0].y = sel.bottom;
			pts[1].x = sel.right;
			pts[1].y = sel.top;
			pts[2].x = sel.left;
			pts[2].y = sel.top;
			pts[3].x = sel.left;
			pts[3].y = sel.bottom; 
			c.x = m_x;
			c.y = m_y;
			// rotate to angle
			if( m_angle )
			{
				RotateRect( &sel, m_angle, c );
				RotatePOINTS( &pts[0], 4, -m_angle, c );
			}
			// draw it
			id.st = ID_TXT;
			id.i = 0;
			if( getbit(l_map,LAY_PAD_THRU))
				dl_sel = NULL;
			else
				dl_sel = dlist->AddSelector( id, this, DL_POLYGON, 1,
										&sel, 0, pts, 4, l_map );
			dl_el = dlist->Add( id, this, l_map, DL_LINES_ARRAY, 1,
										&sel, m_stroke_width, &(*m_stroke)[0], m_stroke->GetSize() );
			m_dlist = dlist;
		}
		else 
		{
			dl_sel = NULL;
			dl_el = NULL;
		}
	}
	else
		ASSERT(0);
}




void CText::Undraw()
{
	if( m_dlist )
	{
		m_dlist->Remove( dl_sel );
		dl_sel = NULL;
		m_dlist->Remove( dl_el );
		dl_el = NULL;
		m_dlist = NULL;		// indicated that it is not drawn
	}
	m_smfontutil = NULL;	// indicate that strokes have been removed
}

// CTextList constructor/destructors
//

// default constructor
//
CTextList::CTextList()
{
	m_dlist = NULL;
	m_smfontutil = NULL;
}

// normal constructor
//
CTextList::CTextList( CDisplayList * dlist, SMFontUtil * smfontutil )
{
	m_dlist = dlist;
	m_smfontutil = smfontutil;
}

// destructor
//
CTextList::~CTextList()
{
	// destroy all CTexts
	for( int i=0; i<text_ptr.GetSize(); i++ )
	{
		delete text_ptr[i];
	}
}

// AddText ... adds a new entry to TextList, returns pointer to entry
//
CText * CTextList::AddText( int x, int y, int angle, int mirror, BOOL bNegative, int layer, 
						   int font_size, int stroke_width, CString * str_ptr, BOOL draw_flag )
{
	// create new CText and put pointer into text_ptr[]
	CText * text;
	if( draw_flag )
	{
		text = new CText( m_dlist, x, y, angle, mirror, bNegative, layer,//ok
			font_size, stroke_width, m_smfontutil, str_ptr );	
	}
	else
	{
		text = new CText( NULL, x, y, angle, mirror, bNegative, //ok
			layer, font_size, stroke_width, NULL, str_ptr );
		text->m_dlist = m_dlist;
		text->m_smfontutil = m_smfontutil;
	}
	text_ptr.Add( text );
	text->m_merge = -1;
	return text;
}

int CTextList::GetSelCount()
{
	int cnt=0;
	for( int i=text_ptr.GetSize()-1; i>=0; i-- )
	{
		if( text_ptr[i]->m_selected )
		cnt++;
	}
	return cnt;
}


// RemoveText ... removes an entry and destroys it
//	returns 0 if successful, 1 if unable to find text
//
int CTextList::RemoveText( CText * text )
{
	for( int i=0; i<text_ptr.GetSize(); i++ )
	{
		if( text_ptr[i] == text )
		{
			text_ptr[i]->Undraw();
			delete text_ptr[i];
			text_ptr.RemoveAt( i );
			return 0;
		}
	}
	return 1;
}

// remove all text entries
//	returns 0 if successful, 1 if unable to find text
//
void CTextList::RemoveAllTexts()
{
	int n = text_ptr.GetSize();
	for( int i=0; i<n; i++ )
	{
		RemoveText( text_ptr[0] );
	}
	return;
}

// Select text for editing
//
void CTextList::HighlightText( CText * text, int transparent_static )
{
	m_dlist->HighLight( text->dl_el );
	if( transparent_static )
	{
		text->dl_el->transparent = transparent_static;
		text->dl_el->el_static = transparent_static;
		dl_element * el = m_dlist->Cpy( text->dl_el );
		el->layers_bitmap = 0;
		m_dlist->HighLight(el);
		setbit( el->map_orig_layer, text->m_layer );
		el->el_static = transparent_static;
	}
}

// start dragging a rectangle representing the boundaries of text string
//
void CTextList::StartDraggingText( CDC * pDC, CText * text )
{
	// make text strokes invisible
	text->dl_el->visible = 0;

	// cancel selection 
	m_dlist->CancelHighLight();
	RECT tr;
	GetTextRectOnPCB(text,&tr);
	int tx = text->m_x;
	int ty = text->m_y;
	Rotate_i_Vertex( &tx, &ty, -text->m_angle, text->m_x, text->m_y );
	m_dlist->StartDraggingRectangle(	pDC, 
										0,0,
										tr.left-tx,
										tr.bottom-ty,
										tr.right-tx,
										tr.top-ty,
										LAY_SELECTION );
}

// stop dragging text 
//
void CTextList::CancelDraggingText( CText * text )
{
	m_dlist->StopDragging();
	text->dl_el->visible = 1;
}

#if 0
// move text
//
CText * CTextList::MoveText( CText * text, int x, int y, int angle, int mirror, int layer )
{
	CText * new_text = AddText( x, y, angle, mirror, layer, text->m_font_size, text->m_stroke_width, &(text->m_str) );
	new_text->m_guid = text->m_guid;
	RemoveText( text );
	return new_text;
}
#endif

// move text
//
void CTextList::MoveText( CText * text, int x, int y, int angle, 
						 BOOL mirror, BOOL negative, int layer )
{
	CDisplayList * dl = text->m_dlist;
	SMFontUtil * smf = text->m_smfontutil;
	text->Undraw();
	text->m_x = x;
	text->m_y = y;
	text->m_angle = angle;
	text->m_layer = layer;
	text->m_mirror = mirror;
	text->m_bNegative = negative;
	text->Draw( dl, smf );
}

// write text info to file
//
int CTextList::WriteTexts( CStdioFile * file )
{
	CString line;
	CText * t;
	try
	{
		// now write all text strings
		file->WriteString( "[texts]\n\n" );
		for( int it=0; it<text_ptr.GetSize(); it++ )
		{
			t = text_ptr[it];
			line.Format( "text: \"%s\" %d %d %d %d %d %d %d %d %d\n", t->m_str,
				t->m_x, t->m_y, t->m_layer, t->m_angle, t->m_mirror,
				t->m_font_size, t->m_stroke_width, t->m_bNegative, t->m_merge );
			file->WriteString( line );
			if( m_dlist )
			{
				CArray<CPoint> * tP = m_dlist->Get_Points( t->dl_el, NULL, 0 );
				for( int ti=0; ti<tP->GetSize(); ti+=2 )
				{
					line.Format( "text_line: %d %d %d %d\n",tP->GetAt(ti).x*m_pcbu_per_wu, 
															tP->GetAt(ti).y*m_pcbu_per_wu, 
															tP->GetAt(ti+1).x*m_pcbu_per_wu, 
															tP->GetAt(ti+1).y*m_pcbu_per_wu );
					file->WriteString( line );
				}
				CPoint pts[4];
				int np = 4;
				m_dlist->Get_Points( t->dl_sel, pts, &np );
				if( np >= 4 )
				{
					line.Format( "selection: %d %d %d %d %d %d %d %d\n",pts[0].x, pts[0].y, pts[1].x, pts[1].y, 
																		pts[2].x, pts[2].y, pts[3].x, pts[3].y );
					file->WriteString( line );
				}
			}
			file->WriteString("\n");
		}
		
	}
	catch( CFileException * e )
	{
		CString str;
		if( e->m_lOsError == -1 )
			str.Format( "File error: %d\n", e->m_cause );
		else
			str.Format( "File error: %d %ld (%s)\n", e->m_cause, e->m_lOsError,
			_sys_errlist[e->m_lOsError] );
		return 1;
	}

	return 0;
}

// read text strings from file
//
void CTextList::ReadTexts( CStdioFile * pcb_file, double read_version )
{
	int pos, err, np;
	CString in_str, key_str;
	CArray<CString> p;

	// find beginning of [texts] section
	do
	{
		err = pcb_file->ReadString( in_str );
		if( !err )
		{
			// error reading pcb file
			CString mess;
			mess.Format( "Unable to find [texts] section in file" );
			AfxMessageBox( mess );
			return;
		}
		in_str.Trim();
	}
	while( in_str != "[texts]" );

	// get all text strings
	CText * t = NULL;
	CPoint selection[4];
	int sz = 0;
	int layer = 0;
	int stroke_width = 0;
	CArray<CPoint> m_stroke;
	m_stroke.SetSize(0);
	while( 1 )
	{
		pos = pcb_file->GetPosition();
		err = pcb_file->ReadString( in_str );
		if( !err )
		{
			CString * err_str = new CString( "unexpected EOF in project file" );//
			throw err_str;
		}
		in_str.Trim();
		if( in_str.Left(1) == "[" && in_str != "[texts]" )
		{
			if(t && m_dlist)
			{
				if( !sz || !t->dl_sel )
				{
					t->Undraw();
					t->Draw(m_dlist,m_smfontutil);
				}
			}
			sz = 0;
			pcb_file->Seek( pos, CFile::begin );
			break;		// next section, exit
		}
		else if( in_str.Left(5) == "text:" )
		{
			if( t && m_dlist )
			{
				if( !sz || !t->dl_sel )
				{
					t->Undraw();
					t->Draw(m_dlist,m_smfontutil);
				}
			}
			t = NULL;
			sz = 0;
			np = ParseKeyString( &in_str, &key_str, &p );
			CString str = p[0];
			if( str.GetLength() )
			{
				int x = my_atoi( &p[1] );
				int y = my_atoi( &p[2] );
				layer = my_atoi( &p[3] );
				if( read_version < 2.0199 )
					layer += 3;
				int angle = my_atoi( &p[4] );
				int mirror = my_atoi( &p[5] );
				int font_size = my_atoi( &p[6] );
				stroke_width = my_atoi( &p[7] );
				BOOL m_bNegative = 0;
				if( np >= 10)
					m_bNegative = my_atoi( &p[8] );
				int merge = -1;
				if ( np >= 11 )
					merge = my_atoi( &p[9] );
				t = AddText( x, y, angle, mirror, m_bNegative, layer, font_size, stroke_width, &str );			
				t->m_merge = merge;
			}
		}
	}
}

// create new undo_text object for undoing changes
//
undo_text * CTextList::CreateUndoRecord( CText * text )
{
	undo_text * undo = new undo_text;//ok
	undo->m_guid = text->m_guid;
	undo->m_x = text->m_x;
	undo->m_y = text->m_y; 
	undo->m_layer = text->m_layer; 
	undo->m_angle = text->m_angle; 
	undo->m_mirror = text->m_mirror; 
	undo->m_merge_name = text->m_merge;
	undo->m_bNegative = text->m_bNegative; 
	undo->m_font_size = text->m_font_size; 
	undo->m_stroke_width = text->m_stroke_width;
	undo->m_str = text->m_str;
	undo->m_tlist = this;
	return undo;
}

void CTextList::TextUndoCallback( int type, void * ptr, BOOL undo )
{
	int ifound;
	undo_text * un_t = (undo_text*)ptr;
	CText * text = 0;
	if( undo )
	{
		CTextList * tlist = un_t->m_tlist;
		if( type == CTextList::UNDO_TEXT_ADD || type == CTextList::UNDO_TEXT_MODIFY )
		{
			// find existing CText object
			ifound = -1;
			for( int it=0; it<tlist->text_ptr.GetSize(); it++ )
			{
				text = tlist->text_ptr[it];
				if( text->m_guid == un_t->m_guid )
				{
					ifound = it;
					break;
				}
			}
			if( ifound != -1 )
			{
				// text string found
				if( type == CTextList::UNDO_TEXT_ADD )
				{
					// delete text
					tlist->RemoveText( text );
				}
				else if( type == CTextList::UNDO_TEXT_MODIFY )
				{
					// modify text back
					CDisplayList * dl = text->m_dlist;
					SMFontUtil * smf = text->m_smfontutil;
					text->Undraw();
					text->m_guid = un_t->m_guid;
					text->m_x = un_t->m_x;
					text->m_y = un_t->m_y;
					text->m_angle = un_t->m_angle;
					text->m_layer = un_t->m_layer;
					text->m_mirror = un_t->m_mirror;
					text->m_font_size = un_t->m_font_size;
					text->m_stroke_width = un_t->m_stroke_width;
					text->m_nchars = un_t->m_str.GetLength();
					text->m_str = un_t->m_str;
					text->m_merge = un_t->m_merge_name;
					text->Draw( dl, smf );
				}
			}
		}
		else if( type == CTextList::UNDO_TEXT_DELETE )
		{
			// add deleted text back into list
			CText * new_text = tlist->AddText( un_t->m_x, un_t->m_y, un_t->m_angle, 
				un_t->m_mirror, un_t->m_bNegative,
				un_t->m_layer, un_t->m_font_size, un_t->m_stroke_width, &un_t->m_str );
			new_text->m_guid = un_t->m_guid;
		}
	}
	delete un_t;
}

void CTextList::MoveOrigin( int x_off, int y_off )
{
	for( int it=0; it<text_ptr.GetSize(); it++ )
	{
		CText * t = text_ptr[it];
		t->Undraw();
		t->m_x += x_off;
		t->m_y += y_off;
		t->Draw( m_dlist, m_smfontutil );
	}
}

// return text that matches guid
//
CText * CTextList::GetText( GUID * guid )
{
	CText * text = GetFirstText();
	int it = 0;
	while( text )
	{
		if( text->m_guid == *guid )
			return text;
		text = GetNextText(&it);
	}
	return NULL;
}

CText * CTextList::GetText( int x, int y )
{
	CText * text = GetFirstText();
	int it = 0;
	while( text )
	{
		if( text->m_x == x && text->m_y == y )
			return text;
		text = GetNextText(&it);
	}
	return NULL;
}

// return first text in CTextList (or NULL if none)
//
CText * CTextList::GetFirstText()
{
	if( text_ptr.GetSize() > 0 )
		return text_ptr[0];
	else
		return NULL;
}


// return next text in CTextList, or NULL if at end of list
//
CText * CTextList::GetNextText( int * it )
{
	(*it)++;
	if( text_ptr.GetSize() > (*it) && (*it) > -1 )
		return text_ptr[(*it)];
	else
		return NULL;
}


// get bounding rectangle for all text strings
// return FALSE if no text strings
//
BOOL CTextList::GetTextBoundaries( RECT * r )
{
	BOOL bValid = FALSE;
	RECT br;
	(&br)->bottom = INT_MAX;
	(&br)->left = INT_MAX;
	(&br)->top = INT_MIN;
	(&br)->right = INT_MIN;
	if( m_smfontutil )
	{
		CText * t = GetFirstText();
		int it = 0;
		while( t )
		{
			int w = t->m_stroke_width;
			CArray<CPoint> gP;
			if( t->dl_el == NULL )
				t->Draw( 0, m_smfontutil, &gP );
			for( int is=0; is<gP.GetSize(); is+=2 )
			{
				br.bottom = min( br.bottom, gP[is].y - w );
				br.bottom = min( br.bottom, gP[is+1].y - w );
				br.top = max( br.top, gP[is].y + w );
				br.top = max( br.top, gP[is+1].y + w );
				br.left = min( br.left, gP[is].x - w );
				br.left = min( br.left, gP[is+1].x - w );
				br.right = max( br.right, gP[is].x + w );
				br.right = max( br.right, gP[is+1].x + w );	
			}
			if( t->dl_el == NULL )
				t->Undraw();
			bValid = TRUE;
			t = GetNextText(&it);
		}
	}
	*r = br;
	return bValid;
}

// get bounding rectangle for text string
// return FALSE if no text strings
//
BOOL CTextList::GetTextRectOnPCB( CText * t, RECT * r )
{
	BOOL bValid = FALSE;
	if( t->dl_sel )
	{		
		RECT Get;
		m_dlist->Get_Rect( t->dl_sel, &Get );
		(*r).left =		Get.left;
		(*r).right =	Get.right;
		(*r).bottom =	Get.bottom;
		(*r).top =		Get.top;
		bValid = 1;
	}
	return bValid;
}

// reassign copper text to new layers
// enter with layer[] = table of new copper layers for each old copper layer
//
void CTextList::ReassignCopperLayers( int n_new_layers, int * layer )
{
	CText * t = GetFirstText();
	int it = 0;
	while( t )
	{
		int old_layer = t->m_layer;
		if( old_layer >= LAY_TOP_COPPER )
		{
					int index = old_layer - LAY_TOP_COPPER;
					int new_layer = layer[index];
					if( new_layer == old_layer )
					{
						// do nothing
					}
					else if( new_layer == -1 )
					{
						// delete this text
						t->Undraw();
						RemoveText( t );
					}
					else
					{
						// move to new layer
						t->Undraw();
						t->m_layer = new_layer + LAY_TOP_COPPER;
						t->Draw( m_dlist, m_smfontutil ); 
					}
		}
		t = GetNextText(&it);
	}
}

