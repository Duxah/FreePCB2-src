// PartList.cpp : implementation of class CPartList
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
// this is a linked-list of parts on a PCB board
//
#include "stdafx.h"
#include <math.h>
#include "DisplayList.h"
#include "DlgMyMessageBox.h"
#include "mainfrm.h"

#define PL_MAX_SIZE		10000		// default max. size 

// globals
BOOL g_bShow_header_28mil_hole_warning = TRUE;	
BOOL g_bShow_SIP_28mil_hole_warning = TRUE;	

//******** constructors and destructors *********
 
cpart::cpart()
{
	// zero out pointers
	dl_sel = 0;
	dl_ref_sel = 0;
	shape = 0;
	drawn = FALSE;
}

cpart::~cpart()
{
}

CPartList::CPartList( CDisplayList * dlist, SMFontUtil * fontutil ) 
{
	m_start.prev = 0;		// dummy first element in list
	m_start.next = &m_end;
	m_end.next = 0;			// dummy last element in list
	m_end.prev = &m_start;
	m_max_size = PL_MAX_SIZE-1;	// size limit
	m_size = 0;					// current size
	m_dlist = dlist;
	m_fontutil = fontutil;
	m_footprint_cache_map = NULL;
	m_swell_pad_for_solder_mask = 0;
	m_swell_pad_for_paste_mask = 0;
	m_layers = 2;
	m_annular_ring = 0;
	begin_dragging_x = 0;
	begin_dragging_y = 0;
	begin_dragging_ang = 0;
	begin_dragging_side = 0;
}

CPartList::~CPartList()
{
	// traverse list, removing all parts
	while( m_end.prev != &m_start )
		Remove( m_end.prev );
}

// Create new empty part and add to end of list
// return pointer to element created.
//
cpart * CPartList::Add()
{
	if(m_size >= (m_max_size-1) )
	{
		AfxMessageBox( "Maximum number of parts exceeded" );
		return 0;
	}

	// create new instance and link into list
	cpart * part = new cpart;//ok
	part->prev = m_end.prev;
	part->next = &m_end;
	part->prev->next = part;
	part->next->prev = part;
	part->m_merge = -1;
	part->selected = 0;
	return part;
}

// Create new part, add to end of list, set part data 
// return pointer to element created.
//
cpart * CPartList::Add( CShape * shape, CString * ref_des, 
							int x, int y, int side, int angle, int visible, int glued )
{
	if(m_size >= m_max_size )
	{
		AfxMessageBox( "Maximum number of parts exceeded" );
		return 0;
	}

	// create new instance and link into list
	cpart * part = Add();
	// set data
	SetPartData( part, shape, ref_des, x, y, side, angle, visible, glued, -1);
	return part;
}

// Set part data, draw part if m_dlist != NULL
//
int CPartList::SetPartData( cpart * part, CShape * shape, CString * ref_des, 
							int x, int y, int side, int angle, int visible, int glued, int merge )
{
	UndrawPart( part );
	CDisplayList * old_dlist = m_dlist;
	m_dlist = NULL;		// cancel further drawing
	// now copy data into part
	id id( ID_PART );
	part->visible = visible;
	part->ref_des = *ref_des;
	part->m_id = id;
	part->x = x;
	part->y = y;
	part->side = side;
	part->angle = angle;
	part->glued = glued;
	part->m_merge = merge;
	if( !shape )
	{
		part->shape = NULL;
		part->pin.SetSize(0);
		part->m_ref_xi = 0;
		part->m_ref_yi = 0;
		part->m_ref_angle = 0;
		part->m_ref_size = 0;
		part->m_ref_w = 0;
		part->m_value_xi = 0;
		part->m_value_yi = 0;
		part->m_value_angle = 0;
		part->m_value_size = 0;
		part->m_value_w = 0;		
	}
	else
	{
		part->shape = shape;
		part->pin.SetSize( shape->m_padstack.GetSize() );
		Move( part, x, y, angle, side );	// force setting pin positions
		part->m_ref_xi = shape->m_ref_xi;
		part->m_ref_yi = shape->m_ref_yi;
		part->m_ref_angle = shape->m_ref_angle;
		part->m_ref_size = shape->m_ref_size;
		part->m_ref_w = shape->m_ref_w;
		part->m_value_xi = shape->m_value_xi;
		part->m_value_yi = shape->m_value_yi;
		part->m_value_angle = shape->m_value_angle;
		part->m_value_size = shape->m_value_size;
		part->m_value_w = shape->m_value_w;
	}
	part->m_outline_stroke.SetSize(0);
	part->dl_ref_el = NULL;
	part->dl_value_el = NULL;
	m_size++;

	// now draw part into display list
	m_dlist = old_dlist;
	if( part->shape )
		DrawPart( part );

	return 0;
}

// Highlight part
//
int CPartList::HighlightPart( cpart * part, BOOL bX )
{
	// highlight it by making its selection rectangle visible
	int EL_W = 50;
	if (part->shape)
	{
		for( int i=part->m_outline_stroke.GetSize()-1; i>=0; i-- )
		{
			if( part->m_outline_stroke[i] )
				m_dlist->HighLight( part->m_outline_stroke[i] );
		}
		if( bX )
		{
			dl_element * n_el = m_dlist->Cpy( part->dl_sel );
			n_el->gtype = DL_RECT_X;
			n_el->layers_bitmap = 0;
			n_el->el_w = EL_W;
			m_dlist->HighLight( n_el );
		}
		else
		{
			m_dlist->HighLight( part->dl_sel );
			part->dl_sel->el_w = EL_W;
			part->dl_sel->transparent = LAY_BACKGND;
		}
	}
	return 0;
}

void CPartList::SetLinesVis( cpart * p, int vis )
{
	if( p->shape )
	{
		for( int lns=p->shape->m_outline_poly.GetSize()-1; lns>=0; lns-- )
		{
			if( p->shape->m_outline_poly[lns].GetLayer() == LAY_FP_SILK_TOP )
				p->shape->m_outline_poly[lns].SetVisible(vis);
		}
	}
}



// Highlight part ref text
//
int CPartList::SelectRefText( cpart * part )
{
	// highlight it by making its selection rectangle visible
	if( part->dl_ref_el )
		m_dlist->HighLight( part->dl_ref_el );
	return 0;
}

// Highlight part value
//
int CPartList::SelectValueText( cpart * part )
{
	// highlight it by making its selection rectangle visible
	if( part->dl_value_el )
		m_dlist->HighLight( part->dl_value_el );
	return 0;
}

void CPartList:: HighlightAllPadsOnNet( cnet * net, int swell, int layer, int bTRANSPARENT, int excl, cpart * ex_p )
{
	for( int ip=0; ip<net->npins; ip++ )
		if( net->pin[ip].part )
			if( net->pin[ip].part->shape )
			{
				CString pname = net->pin[ip].pin_name;
				for( int ii=net->pin[ip].part->shape->GetPinIndexByName(pname,-1); ii>=0; ii=net->pin[ip].part->shape->GetPinIndexByName(pname,ii))
					if( ii>=0 && (ii != excl || net->pin[ip].part != ex_p ))
					{
						
						if(!SelectPad( net->pin[ip].part, ii, swell, layer, bTRANSPARENT ))
						{
							int ll = GetPinLayer( net->pin[ip].part, ii );
							if( ll == LAY_PAD_THRU )
								ll = layer;
							if(!SelectPad( net->pin[ip].part, ii, swell, ll, bTRANSPARENT ))
								SelectPad( net->pin[ip].part, ii, swell, 0, bTRANSPARENT );
						}
					}
			}
}

// Select all pads
int CPartList::SelectPads( cpart * part, int drc, int layer, int bTRANSPARENT )
{
	// select it by making its selection rectangle visible
	if ( !part->shape )
		return 0;
	for ( int np=part->shape->GetNumPins()-1; np>=0; np-- )
		SelectPad( part, np, drc, layer, bTRANSPARENT );
	return 1;
}


// Select part pad
//
dl_element * CPartList::SelectPad( cpart * part, int i, int swell, int layer, int bTRANSPARENT )
{
	// select it by making its selection rectangle visible
	if ( !part->shape )
		return NULL;
	if( i < 0 || i >= part->shape->GetNumPins() )
		return NULL;
	
	dl_element * pad_el = NULL;
	if( !layer && part->pin[i].dl_sel )
	{
		pad_el = part->pin[i].dl_sel;
	}
	int pad_layers = 0;
	for( int nl=part->pin[i].dl_els.GetSize()-1; nl>=0; nl-- )
	{
		if( part->pin[i].dl_els[nl] )
		{
			pad_layers |= part->pin[i].dl_els[nl]->layers_bitmap;
			if( getbit( part->pin[i].dl_els[nl]->layers_bitmap, layer ) )
			{
				if( !pad_el )
				{
					pad_el = part->pin[i].dl_els[nl];
					break;
				}
			}
		}
	}
	if( !pad_layers )
	{
		int gl = m_dlist->GetTopLayer();
		if( gl > 0 )
			setbit( pad_layers, gl );
		if( part->pin[i].dl_hole )
		{
			if( !pad_el )
			{
				pad_el = part->pin[i].dl_hole;
				swell = 0;
			}
		}
	}
	if( !pad_el )
		return NULL;
	//
	// Draw Highlighted Pad..
	//
	dl_element * el=NULL;
	int EL_W = 10*m_pcbu_per_wu;
	//
	//-----------> DL_RECT_X , FILL TYPE
	if( swell < 2 )
	{
		m_dlist->HighLight( pad_el );
		el = pad_el;
	}
	//
	//-----------> HOLLOW TYPE
	else 
	{	
		CArray<CPoint> * pts = m_dlist->Get_Points( pad_el, NULL, 0 );
		int np = pts->GetSize();
		CPoint * P=NULL;
		if( np )
		{		
			P = new CPoint[np];//ok
			m_dlist->Get_Points( pad_el, P, &np );
		}
		if( pad_el->gtype == DL_CIRC )
		{
			RECT rct;
			m_dlist->Get_Rect( pad_el, &rct );
			SwellRect( &rct, swell);
			el = m_dlist->Add( pad_el->id, pad_el->ptr, 0, DL_HOLLOW_CIRC, 1, &rct, EL_W, P, np );
			m_dlist->HighLight( el );
			el->map_orig_layer = pad_layers;
		}
		else if( pad_el->gtype == DL_LINE )
		{
			CArray<CPoint> * pnts = m_dlist->Get_Points( pad_el, NULL, NULL );
			if( pnts->GetSize() > 2 )
			{
				CPoint hlP[32];
				hlP[0] = (*pnts)[0];
				hlP[1] = (*pnts)[1];
				RECT * rct = m_dlist->Get_Rect( pad_el, NULL );
				int npt = Gen_HollowLinePoly( (*pnts)[0].x, (*pnts)[0].y, (*pnts)[1].x, (*pnts)[1].y, pad_el->el_w, &hlP[2], 30 );
				dl_element * el = m_dlist->Add( pad_el->id, pad_el->ptr, 0, DL_HOLLOW_LINE, 1, 
												rct, 0, hlP, npt+2, FALSE );
				m_dlist->HighLight( el );
				el->map_orig_layer = pad_layers;
			}
		}
		else if( pad_el->gtype == DL_RECT || pad_el->gtype == DL_RRECT )
		{
			RECT rct;
			m_dlist->Get_Rect( pad_el, &rct );
			SwellRect( &rct, swell);
			int w = pad_el->el_w*m_pcbu_per_wu;
			el = m_dlist->Add( pad_el->id, pad_el->ptr, 0, DL_HOLLOW_RRECT, 1, &rct, w+swell, P, np );
			m_dlist->HighLight( el );
			el->map_orig_layer = pad_layers;
		}
		else if( pad_el->gtype == DL_POLYGON || pad_el->gtype == DL_RECT_X )
		{
			RECT rct;
			m_dlist->Get_Rect( pad_el, &rct );
			if( !np )
			{
				P[0].x = rct.left;
				P[0].y = rct.bottom;
				P[1].x = rct.left;
				P[1].y = rct.top;
				P[2].x = rct.right;
				P[2].y = rct.top;
				P[3].x = rct.right;
				P[3].y = rct.bottom;
				np = 4;
			}
			int r = m_dlist->Get_el_w( pad_el )/2.0;
			int new_np = SwellPolygon( P, np, NULL, swell+r );
			if( new_np )
			{
				CPoint * Pnew = new CPoint[new_np+1];//ok						
				new_np = SwellPolygon( P, np, Pnew, swell+r );
				Pnew[new_np] = Pnew[0];
				el = m_dlist->Add( pad_el->id, pad_el->ptr, 0, DL_POLYLINE, 1, &rct, 1, Pnew, new_np+1 );
				m_dlist->HighLight( el );
				el->map_orig_layer = pad_layers;
				delete Pnew;
			}
		}
		if(P)
			delete P;
	}
	if( el )
		el->transparent = bTRANSPARENT;
	return el;	
}

// Test for hit on pad
//
int CPartList::TestHitOnPad( cpart * part, CString * pin_name, int x, int y, int layer )
{
	if( !part )
		return -1;
	if( !part->shape )
		return -1;
	for( int pin_index=part->shape->GetPinIndexByName(*pin_name,-1); 
			 pin_index>=0; 
			 pin_index=part->shape->GetPinIndexByName(*pin_name,pin_index))
	{

		CPoint pts[4];
		int np = 4;
		dl_element * el=0;
		for(int ip=part->pin[pin_index].dl_els.GetSize()-1; ip>=0; ip-- )
		{
			if( part->pin[pin_index].dl_els[ip] )
				if( getbit( part->pin[pin_index].dl_els[ip]->layers_bitmap, layer ) )
				{
					el = part->pin[pin_index].dl_els[ip];
					break;
				}
		}
		if( el )
		{
			m_dlist->Get_Points( el, pts, &np );
			if( np )
			{
				if( TestPolygon( x, y, pts, np ))
					return pin_index;
			}
			else
			{
				RECT r = rect(0,0,0,0);
				m_dlist->Get_Rect( el, &r );
				if( InRange( x, r.left, r.right ) )
					if( InRange( y, r.top, r.bottom ) )
						return pin_index;
			}
		}
	}
	return -1;
}


// Move part to new position, angle and side
// x and y are in world coords
// Does not adjust connections to pins
//
int CPartList::Move( cpart * part, int x, int y, int angle, int side, BOOL bDraw )
{
	// remove all display list elements
	if( bDraw )
		UndrawPart( part );
	// move part
	part->x = x;
	part->y = y;
	part->angle = angle % 360;
	part->side = side;
	// calculate new pin positions
	if( part->shape )
	{
		for( int ip=0; ip<part->pin.GetSize(); ip++ )
		{
			CPoint pt = GetPinPoint( part, ip, part->side, part->angle );
			part->pin[ip].x = pt.x;
			part->pin[ip].y = pt.y;
		}
	}
	// now redraw it
	if( bDraw )
		DrawPart( part );
	return PL_NOERR;
}

// Move ref text with given id to new position and angle
// x and y are in absolute world coords
// angle is relative to part angle
//
int CPartList::MoveRefText( cpart * part, int x, int y, int angle, int size, int w )
{
	// remove all display list elements
	UndrawPart( part );
	
	// get position of new text box origin relative to part
	CPoint part_org, tb_org;
	tb_org.x = x - part->x;
	tb_org.y = y - part->y;
	
	// correct for rotation of part
	if( part->angle )
		RotatePoint( &tb_org, part->angle, zero );
	
	// correct for part on bottom of board (reverse relative x-axis)
	if( part->side == 1 )
		tb_org.x = -tb_org.x;
	
	// reset ref text position
	part->m_ref_xi = tb_org.x;
	part->m_ref_yi = tb_org.y;
	part->m_ref_angle = angle % 360;
	if (part->m_ref_angle < 0)
		part->m_ref_angle += 360;
	part->m_ref_size = size;
	part->m_ref_w = w;
	
	// now redraw part
	DrawPart( part );
	return PL_NOERR;
}

// Resize ref text for part
//
void CPartList::ResizeRefText( cpart * part, int size, int width, BOOL vis, BOOL bDraw )
{
	if( part->shape )
	{
		// remove all display list elements
		if( bDraw )
			UndrawPart( part );
		// change ref text size
		part->m_ref_size = size;
		part->m_ref_w = width;	
		part->m_ref_vis = vis;
		// now redraw part
		if( bDraw )
			DrawPart( part );
	}
}

// Move value text to new position and angle
// x and y are in absolute world coords
// angle is relative to part angle
//
int CPartList::MoveValueText( cpart * part, int x, int y, int angle, int size, int w )
{
	// remove all display list elements
	UndrawPart( part );
	
	// get position of new text box origin relative to part
	CPoint part_org, tb_org;
	tb_org.x = x - part->x;
	tb_org.y = y - part->y;
	
	// correct for rotation of part
	if( part->angle )
		RotatePoint( &tb_org, part->angle, zero );
	
	// correct for part on bottom of board (reverse relative x-axis)
	if( part->side == 1 )
		tb_org.x = -tb_org.x;
	
	// reset value text position
	part->m_value_xi = tb_org.x;
	part->m_value_yi = tb_org.y;
	part->m_value_angle = angle % 360;
	if (part->m_value_angle < 0)
		part->m_value_angle += 360;
	part->m_value_size = size;
	part->m_value_w = w;
	
	// now redraw part
	DrawPart( part );
	return PL_NOERR;
}

// Resize value text for part
//
void CPartList::ResizeValueText( cpart * part, int size, int width, BOOL vis, BOOL bDraw )
{
	if( part->shape )
	{
		// remove all display list elements
		if( bDraw )
			UndrawPart( part );
		// change ref text size
		part->m_value_size = size;
		part->m_value_w = width;
		part->m_value_vis = vis;
		// now redraw part
		if( bDraw )
			DrawPart( part );
	}
}

// Set part value
//
void CPartList::SetValue( cpart * part, CString * value, 
						 int x, int y, int angle, int size, int w, BOOL vis )
{
	part->value = *value;
	part->m_value_xi = x;
	part->m_value_yi = y;
	part->m_value_angle = angle;
	part->m_value_size = size;
	part->m_value_w = w;
	part->m_value_vis = vis;
	if( part->shape && m_dlist )
	{
		UndrawPart( part );
		DrawPart( part );
	}
}

// Set part value
//
void CPartList::SetValue( cpart * part, CString * value )
{
	part->value = *value;
	if( part->shape && m_dlist )
	{
		UndrawPart( part );
		DrawPart( part );
	}
}


// Get side of part
//
int CPartList::GetSide( cpart * part )
{
	return part->side;
}

// Get angle of part
//
int CPartList::GetAngle( cpart * part )
{
	return part->angle;
}

// Get angle of ref text for part
//
int CPartList::GetRefAngle( cpart * part )
{
	return part->m_ref_angle;
}

// Get angle of value for part
//
int CPartList::GetValueAngle( cpart * part )
{
	return part->m_value_angle;
}

// get actual position of ref text
//
CPoint CPartList::GetRefPoint( cpart * part )
{
	CPoint ref_pt;

	// move origin of text box to position relative to part
	ref_pt.x = part->m_ref_xi;
	ref_pt.y = part->m_ref_yi;
	// flip if part on bottom
	if( part->side )
		ref_pt.x = -ref_pt.x;
	// rotate with part about part origin
	if( part->angle )	
		RotatePoint( &ref_pt, -part->angle, zero );
	ref_pt.x += part->x;
	ref_pt.y += part->y;
	return ref_pt;
}

// get actual position of value text
//
CPoint CPartList::GetValuePoint( cpart * part )
{
	CPoint value_pt;

	// move origin of text box to position relative to part
	value_pt.x = part->m_value_xi;
	value_pt.y = part->m_value_yi;
	// flip if part on bottom
	if( part->side )
		value_pt.x = -value_pt.x;
	// rotate with part about part origin
	if( part->angle )
		RotatePoint( &value_pt, -part->angle, zero );
	value_pt.x += part->x;
	value_pt.y += part->y;
	return value_pt;
}

// Get pin info from part
//
CPoint CPartList::GetPinPoint( cpart * part, int pin_index, int side, int angle )
{
	if( !part->shape )
		ASSERT(0);

	// get pin coords relative to part origin
	CPoint pp;
	if( pin_index == -1 )
		ASSERT(0);
	pp.x = part->shape->m_padstack[pin_index].x_rel;
	pp.y = part->shape->m_padstack[pin_index].y_rel;
	// flip if part on bottom
	if( side )
		pp.x = -pp.x;
	// rotate if necess.
	if( angle )
		RotatePoint( &pp, -angle, zero );

	// add coords of part origin
	pp.x = part->x + pp.x;
	pp.y = part->y + pp.y;
	return pp;
}

// Get centroid info from part
//
CPoint CPartList::GetCentroidPoint( cpart * part )
{
	if( part->shape == NULL )
		ASSERT(0);
	// get coords relative to part origin
	CPoint pp;
	pp.x = part->shape->m_centroid_x;
	pp.y = part->shape->m_centroid_y;
	// flip if part on bottom
	if( part->side )
		pp.x = -pp.x;
	// rotate if necess.
	if( part->angle )
		RotatePoint( &pp, -part->angle, zero );
	// add coords of part origin
	pp.x = part->x + pp.x;
	pp.y = part->y + pp.y;
	return pp;
}

// Get glue spot info from part
//
CPoint CPartList::GetGluePoint( cpart * part, int iglue )
{
	if( part->shape == NULL )
		ASSERT(0);
	if( iglue >= part->shape->m_glue.GetSize() )
		ASSERT(0);
	// get coords relative to part origin
	CPoint pp;
	pp.x = part->shape->m_glue[iglue].x_rel;
	pp.y = part->shape->m_glue[iglue].x_rel;
	// flip if part on bottom
	if( part->side )
		pp.x = -pp.x;
	// rotate if necess.
	if( part->angle )
		RotatePoint( &pp, -part->angle, zero );
	// add coords of part origin
	pp.x = part->x + pp.x;
	pp.y = part->y + pp.y;
	return pp;
}

// Get pin layer
// returns LAY_TOP_COPPER, LAY_BOTTOM_COPPER or LAY_PAD_THRU
//

// Get pin layer
// returns LAY_TOP_COPPER, LAY_BOTTOM_COPPER or LAY_PAD_THRU
//
int CPartList::GetPinLayer( cpart * part, int pin_index )
{
	if( part->shape->m_padstack[pin_index].hole_size )
		return LAY_PAD_THRU;
	else if( part->shape->m_padstack[pin_index].top.shape != PAD_NONE && part->shape->m_padstack[pin_index].bottom.shape != PAD_NONE )
		return LAY_PAD_THRU;
	else if( part->side == 0 && part->shape->m_padstack[pin_index].top.shape != PAD_NONE 
		|| part->side == 1 && part->shape->m_padstack[pin_index].bottom.shape != PAD_NONE )
		return LAY_TOP_COPPER;
	else
		return LAY_BOTTOM_COPPER;
}

// Get pin net
//
cnet * CPartList::GetPinNet( cpart * part, CString * pin_name )
{
	int pin_index = part->shape->GetPinIndexByName( *pin_name, -1 );
	if( pin_index == -1 )
		return NULL;
	return part->pin[pin_index].net;
}

// Get pin net
//
cnet * CPartList::GetPinNet( cpart * part, int pin_index )
{
	return part->pin[pin_index].net;
}

// Get max pin width, for drawing thermal symbol
// enter with pin_num = pin # (1-based)


// Get bounding rect for all pins
// Currently, just uses selection rect
// returns 1 if successful
//
int CPartList::GetPartBoundingRect( cpart * part, RECT * part_r )
{
	if( !part )
		return 0;
	if( !part->shape )
		return 0;
	if( part->dl_sel )
	{
		m_dlist->Get_Rect( part->dl_sel, part_r );
		return 1;
	}  
	return 0;
}

int CPartList::GetPinsBoundingRect	( cpart * part, RECT * pins_r )
{
	if( !part )
		return 0;
	if( !part->shape )
		return 0;
	int np = part->shape->GetNumPins();
	pins_r->left = pins_r->bottom = INT_MAX;
	pins_r->right = pins_r->top = INT_MIN;
	RECT r;
	for( int ip=0; ip<np; ip++ )
	{
		m_dlist->Get_Rect( part->pin[ip].dl_sel, &r );
		SwellRect( pins_r, r );
	}  
	return 1;
}

int CPartList::GetPartThruPadsRect	( cpart * part, RECT * ThruPads )
{
	ThruPads->left = ThruPads->bottom = INT_MAX;
	ThruPads->right = ThruPads->top = INT_MIN;
	int np = 0;
	if( part->shape )
		for( int ip=part->shape->GetNumPins()-1; ip>=0; ip-- )
		{
			if( part->pin[ip].dl_hole )
			{
				RECT Get;
				m_dlist->Get_Rect( part->pin[ip].dl_hole, &Get );
				SwellRect( ThruPads, Get );
				np++;
			}
		}
	return np;
}

// GetRefBoundingRect
// return 1 if ok
//
int CPartList::GetRefBoundingRect	( cpart * part, RECT * ref_r )
{
	if( !part )
		return 0;
	if( !part->shape )
		return 0;
	if( part->dl_ref_sel )
	{
		m_dlist->Get_Rect( part->dl_ref_sel, ref_r );
		return 1;
	}
	return 0;
}

// GetValueBoundingRect
// return 1 if ok
//
int CPartList::GetValueBoundingRect( cpart * part, RECT * value_r )
{
	if( !part )
		return 0;
	if( !part->shape )
		return 0;
	if( part->dl_value_sel )
	{
		m_dlist->Get_Rect( part->dl_value_sel, value_r );
		return 1;
	}
	return 0;
}

// GetPadBounds
// return 1 if ok
//
int CPartList::GetPadBounds( cpart * part, int pad, RECT * pr )
{
	if( !part )
		return 0;
	if( !part->shape )
		return 0;
	if( part->drawn )
		*pr = part->pin[pad].bounds;
	else
	{
		pr->left =		0;
		pr->right =		10*NM_PER_MIL;
		pr->bottom =	0;
		pr->top =		10*NM_PER_MIL;
		return 0;
	}
	return 1;
}

// get bounding rectangle of parts
// return parts found
//
int CPartList::GetPartBoundaries( RECT * part_r )
{
	int parts_found = 0;
	// iterate
	cpart * part = m_start.next;
	while( part->next != 0 )
	{
		RECT gr;
		if( GetPartBoundingRect(part,&gr) )
		{
			if( parts_found == 0 )
				*part_r = gr;
			else
				SwellRect(part_r, gr);
			parts_found++;
		}
		part = part->next;
	}
	return parts_found;
}

// Get pointer to part in part_list with given ref
//
cpart * CPartList::GetPart( LPCTSTR ref_des )
{
	// find element with given ref_des, return pointer to element
	cpart * part = m_start.next;
	if( part ) 
		while( part->next != 0 )
		{
			if(  part->ref_des.Compare( ref_des ) == 0 )
				return part;
			part = part->next;
		}
	return NULL;	// if unable to find part
}

// Iterate through parts
//
cpart * CPartList::GetFirstPart()
{
	cpart * p = m_start.next;
	if( p->next )
		return p;
	else
		return NULL;
}

cpart * CPartList::GetEndPart()
{
	cpart * p = m_end.prev;
	if( p->prev )
		return p;
	else
		return NULL;
}

cpart * CPartList::GetNextPart( cpart * part )
{
	cpart * p = part->next;
	if( !p )
		return NULL;
	if( !p->next )
		return NULL;
	else
		return p;
}

cpart * CPartList::GetPrevPart( cpart * part )
{
	cpart * p = part->prev;
	if( !p )
		return NULL;
	if( !p->prev )
		return NULL;
	else
		return p;
}

// Get selected count
int CPartList::GetSelCount()
{
	int cnt=0;
	cpart * part = m_start.next;
	while( part->next != 0 )
	{
		if( part->shape )
			if( part->selected )
				cnt++;
		part = part->next;
	}
	return cnt;
}

int CPartList::GetSelParts(CString * AllSelected)
{
	if( AllSelected )
	{
		*AllSelected = "";
		for(cpart* gp=GetFirstPart(); gp; gp=GetNextPart(gp))
			if( gp->selected )
				*AllSelected += (gp->ref_des + " ");
		return 1;
	}
	return 0;
}

// get number of times a particular shape is used
//
int CPartList::GetNumFootprintInstances( CShape * shape )
{
	int n = 0;

	cpart * part = m_start.next;
	while( part->next != 0 )
	{
		if(  part->shape == shape  )
			n++;
		part = part->next;
	}
	return n;
}

// Purge unused footprints from cache
//
void CPartList::PurgeFootprintCache()
{
	POSITION pos;
	CString key;
	void * ptr;

	if( !m_footprint_cache_map )
		ASSERT(0);

	for( pos = m_footprint_cache_map->GetStartPosition(); pos != NULL; )
	{
		m_footprint_cache_map->GetNextAssoc( pos, key, ptr );
		CShape * shape = (CShape*)ptr;
		if( GetNumFootprintInstances( shape ) == 0 )
		{
			// purge this footprint
			delete shape;
			m_footprint_cache_map->RemoveKey( key );
		}
	}
}

// Remove part from list and delete it
//
int CPartList::Remove( cpart * part )
{
	// delete all entries in display list
	UndrawPart( part );

	// remove links to this element
	part->next->prev = part->prev;
	part->prev->next = part->next;
	// destroy part
	m_size--;
	delete( part );

	return 0;
}

// Remove all parts from list
//
void CPartList::RemoveAllParts()
{
	// traverse list, removing all parts
	while( m_end.prev != &m_start )
		Remove( m_end.prev );
}

// Set utility flag for all parts
//
void CPartList::MarkAllParts( int mark )
{
	cpart * part = GetFirstPart();
	while( part )
	{
		part->utility = mark;
		part = GetNextPart( part );
	}
}

// generate an array of strokes for a string that is attached to a part
// enter with:
//  str = pointer to text string
//	size = height of characters
//	w = stroke width
//	rel_angle = angle of string relative to part
//	rel_x, rel_y = position of string relative to part
//	angle = angle of part
//	x, y = postion of part
//	side = side of PCB that part is on
//	strokes = pointer to CArray of strokes to receive data
//	br = pointer to RECT to receive bounding rectangle
//	dlist = pointer to display list to use for drawing (not used)
//	sm = pointer to SMFontUtil for character data	
// returns number of strokes generated
//
int GenerateStrokesForPartString( CString * str, 
								  int size, int mirror, int rel_angle, int rel_xi, int rel_yi, int w, 
								  int x, int y, int angle, int side,
								  CArray<CPoint> * strokes, RECT * br, CPoint * pts,
								  CDisplayList * dlist, SMFontUtil * sm )
{
	strokes->SetSize( 0 );
	double x_scale = (double)size/22.0;
	double y_scale = (double)size/22.0;
	double y_offset = 9.0*y_scale;
	double xc = 0.0;
	CPoint si, sf;
	br->left = br->bottom = pts[0].x = pts[0].y = INT_MAX;
	br->right = br->top = pts[2].x = pts[2].y =   INT_MIN;
	for( int ic=0; ic<str->GetLength(); ic++ )
	{
		// get stroke info for character
		//int xi, yi, xf, yf;
		double coord[64][4];
		double min_x, min_y, max_x, max_y;
		int nstrokes = sm->GetCharStrokes( str->GetAt(ic), SIMPLEX, 
			&min_x, &min_y, &max_x, &max_y, coord, 64 );
		br->left = min( br->left, xc );
		br->bottom = min( br->bottom, (min_y*y_scale + y_offset ) );
		br->right = max( br->right, (max_x - min_x)*x_scale + xc );
		br->top = max( br->top, (max_y*y_scale + y_offset) );
		int sz = strokes->GetSize();
		if( nstrokes > 0 )
			strokes->SetSize( sz + nstrokes*2 );
		for( int is=0; is<nstrokes; is++ )
		{
			si.x = (coord[is][0] - min_x)*x_scale + xc;
			si.y = coord[is][1]*y_scale + y_offset;
			sf.x = (coord[is][2] - min_x)*x_scale + xc;
			sf.y = coord[is][3]*y_scale + y_offset;

			// mirror
			if( mirror )
			{
				si.x = -si.x;
				sf.x = -sf.x;
			}
			// get stroke relative to text box
			// rotate about text box origin
			if( rel_angle )
			{
				RotatePoint( &si, -rel_angle, zero );
				RotatePoint( &sf, -rel_angle, zero );
			}
			// move origin of text box to position relative to part
			si.x += rel_xi;
			sf.x += rel_xi;
			si.y += rel_yi;
			sf.y += rel_yi;
			// flip if part on bottom
			if( side )
			{
				si.x = -si.x;
				sf.x = -sf.x;
			}

			// rotate with part about part origin
			if( angle )
			{
				RotatePoint( &si, -angle, zero );
				RotatePoint( &sf, -angle, zero );
			}

			// add x, y to part origin and draw
			(*strokes)[sz+is*2].x = x + si.x;
			(*strokes)[sz+is*2].y = y + si.y;
			(*strokes)[sz+is*2+1].x = x + sf.x;
			(*strokes)[sz+is*2+1].y = y + sf.y;
		}
		xc += (max_x - min_x + 8.0)*x_scale;
	}
	SwellRect( br, w/2 );
	pts[0].x = pts[1].x = br->left;
	pts[0].y = pts[3].y = br->bottom;
	pts[2].x = pts[3].x = br->right;
	pts[2].y = pts[1].y = br->top;
	if( rel_angle )
		RotatePOINTS( pts, 4, -rel_angle, zero );
	// move origin of text box to position relative to part
	pts[0].x += rel_xi;
	pts[0].y += rel_yi;
	pts[1].x += rel_xi;
	pts[1].y += rel_yi;
	pts[2].x += rel_xi;
	pts[2].y += rel_yi;
	pts[3].x += rel_xi;
	pts[3].y += rel_yi;
	// flip if part on bottom
	if( side )
	{
		pts[0].x = -pts[0].x;
		pts[1].x = -pts[1].x;
		pts[2].x = -pts[2].x;
		pts[3].x = -pts[3].x;
	}
	if(angle)
		RotatePOINTS( pts, 4, -angle, zero );
	pts[0].x += x;
	pts[0].y += y;
	pts[1].x += x;
	pts[1].y += y;
	pts[2].x += x;
	pts[2].y += y;
	pts[3].x += x;
	pts[3].y += y;
	*br = rect( pts[0].x, pts[0].y, pts[1].x, pts[1].y );
	SwellRect(br,pts[2]);
	SwellRect(br,pts[3]);
	return strokes->GetSize();
}


// Draw part into display list
//
int CPartList::DrawPart( cpart * part )
{
	if( !m_dlist )
		return PL_NO_DLIST;
	if( !part->shape )
		return PL_NO_FOOTPRINT;
	if( part->drawn )
		UndrawPart( part );		// ideally, should be undrawn when changes made, not now

	// this part
	CShape * shape = part->shape;
	CPoint pp(part->x,part->y);
	int angle = part->angle;
	id pid = part->m_id;

	CArray<CPoint> LinesArray;	// used for text
	RECT br;
	CPoint si, sf;

	int silk_lay = LAY_SILK_TOP;
	if( part->side )
		silk_lay = LAY_SILK_BOTTOM;
	int lay_map = 0;
	setbit( lay_map, silk_lay );
	
	RECT selection;
	selection.left = selection.bottom = INT_MAX;
	selection.right = selection.top = INT_MIN;

	// draw ref designator text
	part->dl_ref_el = NULL;
	part->dl_ref_sel = NULL;
	CPoint pts[4];
	int npts = 0;
	//
	// remove the tail after the symbol |
	CString ref = part->ref_des;
	int pos = ref.Find("|");
	if( pos > 0 )
		ref = ref.Left(pos);
	//
	if( part->m_ref_vis && part->m_ref_size )	
		npts = ::GenerateStrokesForPartString( &ref, part->m_ref_size, 0,
			part->m_ref_angle, part->m_ref_xi, part->m_ref_yi, part->m_ref_w,
			part->x, part->y, part->angle, part->side,
			&LinesArray, &br, pts, m_dlist, m_fontutil );
	if( npts )
	{
		pid.type = ID_PART_LINES;
		pid.st = ID_REF_TXT;
		pid.sst = ID_STROKE;
		part->dl_ref_el = m_dlist->Add( pid, this, lay_map, DL_LINES_ARRAY, 1, &br, part->m_ref_w, &LinesArray[0], npts );
		pid.st = ID_SEL_REF_TXT;
		part->dl_ref_sel = m_dlist->AddSelector( pid, part, DL_POLYGON, 1, &br, 0, pts, 4, lay_map );
	}

	// draw value text
	npts = 0;
	part->dl_value_el = NULL;
	part->dl_value_sel = NULL;
	if( part->m_value_vis && part->m_value_size )
		npts = ::GenerateStrokesForPartString( &part->value, part->m_value_size, 0,
			part->m_value_angle, part->m_value_xi, part->m_value_yi, part->m_value_w,
			part->x, part->y, part->angle, part->side,
			&LinesArray, &br, pts, m_dlist, m_fontutil );	
	if( npts )
	{
		pid.type = ID_PART_LINES;
		pid.st = ID_VALUE_TXT;
		pid.sst = ID_STROKE;
		part->dl_value_el = m_dlist->Add( pid, this, lay_map, DL_LINES_ARRAY, 1, &br, part->m_value_w, &LinesArray[0], npts );
		pid.st = ID_SEL_VALUE_TXT;
		part->dl_value_sel = m_dlist->AddSelector( pid, part, DL_POLYGON, 1,	&br, 0, pts, 4, lay_map );
	}

	// draw part outline
	int p_sz = shape->m_outline_poly.GetSize();
	part->m_outline_stroke.SetSize( p_sz );
	pid.type = ID_PART_LINES;
	pid.st = ID_OUTLINE;
	pid.sst = ID_STROKE;
	dl_element * el_magical = NULL;
	dl_element * el_RctGr = NULL;
	pos = 0;
	for( int step=0; step<6; step++ ) // First we write all the lines on top then on notes then on copper
	{
		for( int ip=0; ip<p_sz; ip++ )
		{
			CPolyLine * poly = &shape->m_outline_poly[ip]; 

			// Get Visible
			if( poly->GetVisible() == 0 )
				continue;

			// Get Layer
			lay_map = 0;
			int ls = poly->GetLayer();
			if( (ls == LAY_FP_SILK_TOP && !part->side) ||
				(ls == LAY_FP_SILK_BOTTOM && part->side) )
			{
				if(step != 0)
					continue;
				setbit(lay_map, LAY_SILK_TOP);
			}
			else if( (ls == LAY_FP_SILK_TOP && part->side) ||
				(ls == LAY_FP_SILK_BOTTOM && !part->side) )
			{
				if(step != 1)
					continue;
				setbit(lay_map, LAY_SILK_BOTTOM);
			}
			else if( (ls == LAY_REFINE_TOP && !part->side) ||
				(ls == LAY_REFINE_BOT && part->side) )
			{
				if(step != 2)
					continue;
				setbit(lay_map, LAY_REFINE_TOP);
			}
			else if( (ls == LAY_REFINE_TOP && part->side) ||
				(ls == LAY_REFINE_BOT && !part->side) )
			{
				if(step != 3)
					continue;
				setbit(lay_map, LAY_REFINE_BOT);
			}
			else if( (ls == LAY_FP_TOP_COPPER && !part->side) ||
				(ls == LAY_FP_BOTTOM_COPPER && part->side) )
			{
				if(step != 4)
					continue;
				setbit(lay_map, LAY_TOP_COPPER);
			}
			else if( (ls == LAY_FP_TOP_COPPER && part->side) ||
				(ls == LAY_FP_BOTTOM_COPPER && !part->side) )
			{
				if(step != 5)
					continue;
				setbit(lay_map, LAY_BOTTOM_COPPER); 
			}
			else
				ASSERT(0);
			int nsides = shape->m_outline_poly[ip].GetNumSides();
			int n_arcs = shape->m_outline_poly[ip].GetNumArcs();
			int m_np = nsides-n_arcs+1+(n_arcs*N_SIDES_APPROX_ARC);
			CPoint * PTS = new CPoint[m_np];//ok
			int i_pts = 0;
			int w = shape->m_outline_poly[ip].GetW();
			RECT el_rect;
			el_rect.left = el_rect.bottom = INT_MAX;
			el_rect.right = el_rect.top  = INT_MIN;	
			for( int i=0; i<nsides; i++ )
			{
				int s_st = poly->GetSideStyle( i );

				// get locations
				si.x = poly->GetX( i );
				si.y = poly->GetY( i );
				int inext = poly->GetIndexCornerNext(i);
				sf.x = poly->GetX( inext );
				sf.y = poly->GetY( inext );
	
				// flip if part on bottom
				if( part->side )
				{
					si.x = -si.x;
					sf.x = -sf.x;
				}

				// shift position
				si.x += pp.x;
				si.y += pp.y;
				sf.x += pp.x;
				sf.y += pp.y;

				if( part->side )
				{
					if( s_st == CPolyLine::ARC_CW )
						s_st = CPolyLine::ARC_CCW;
					else if( s_st == CPolyLine::ARC_CCW )
						s_st = CPolyLine::ARC_CW;
				}
				int n_arcs;
				if( !s_st )
				{
					n_arcs = 2;
					PTS[i_pts].x = si.x;
					PTS[i_pts].y = si.y;
					PTS[i_pts+1].x = sf.x;
					PTS[i_pts+1].y = sf.y;
				}
				else
				{
					n_arcs = Generate_Arc(si.x,si.y,sf.x,sf.y,s_st,&PTS[i_pts],N_SIDES_APPROX_ARC-1);	
				}

				// rotate with part and draw
				if( angle )
				{
					RotatePOINTS( &PTS[i_pts], n_arcs, -angle, pp );
				}

				// Swell rectangle 1
				SwellRect( &el_rect, PTS[i_pts] );

				// increment i_pts 
				i_pts += n_arcs - 1;

				// Swell rectangle 2
				SwellRect( &el_rect, PTS[i_pts] );
			}
			SwellRect( &el_rect, w/2 );
			SwellRect(&selection, el_rect );
			i_pts++;
			if( poly->GetClosed() && poly->GetHatch() )
				part->m_outline_stroke[pos] = m_dlist->Add( pid, poly, lay_map, DL_POLYGON, poly->GetVisible(), &el_rect, w, PTS, i_pts );
			else
				part->m_outline_stroke[pos] = m_dlist->Add( pid, poly, lay_map, DL_POLYLINE, poly->GetVisible(), &el_rect, w, PTS, i_pts );
			if( !el_RctGr )
				el_RctGr = part->m_outline_stroke[pos];
			else if( !el_magical )
				el_magical = part->m_outline_stroke[pos];
			delete PTS;
			pos++;
		}
		if( el_magical )
			if( el_magical->next != m_dlist->Get_End() )
				el_magical->magic = m_dlist->Get_End()->prev;
		el_magical = NULL;
	}
	part->m_outline_stroke.SetSize(pos);
	// draw text
	pid.type = ID_PART_LINES;
	pid.st = ID_TXT;
	pid.sst = ID_STROKE;
	int tsz = part->shape->m_tl->text_ptr.GetSize();
	if( tsz )
	{
		part->shape->m_tl->m_smfontutil = ((CFreePcbApp*)AfxGetApp())->m_Doc->m_smfontutil;
		part->m_outline_stroke.SetSize(pos+tsz);
	}
	for( int step=0; step<2; step++ )
	{
		for( int it=0; it<tsz; it++ )
		{
			CText * t = part->shape->m_tl->text_ptr[it];

			// First we draw in the layer bottom, then in the top
			lay_map = 0;
			if( ( t->m_mirror == 0 && part->side) ||
				( t->m_mirror && !part->side) )
			{
				if(step == 0)
					setbit(lay_map, LAY_SILK_BOTTOM);
				else
					continue;
			}
			else
			{
				if(step == 1)
					setbit(lay_map, LAY_SILK_TOP);
				else
					continue;
			}
			//
			double x_scale = (double)t->m_font_size/22.0;
			double y_scale = (double)t->m_font_size/22.0;
			double y_offset = 9.0*y_scale;
			int i = 0;
			double xc = 0.0;
			CPoint si, sf;
			int w = t->m_stroke_width;
			npts = ::GenerateStrokesForPartString( &t->m_str, t->m_font_size, t->m_mirror,
				t->m_angle, t->m_x, t->m_y, t->m_stroke_width,
				part->x, part->y, part->angle, part->side,
				&LinesArray, &br, pts, m_dlist, m_fontutil );
			RECT el_rect;
			el_rect.left = el_rect.bottom = INT_MAX;
			el_rect.right = el_rect.top  = INT_MIN;	
			if(npts)
			{
				CPoint * PTS = new CPoint[npts];//ok
				SwellRect(&selection,br);
				pid.i = it;		
				for( int is=0; is<npts; is++ )
				{
					PTS[is].x = LinesArray[is].x;
					PTS[is].y = LinesArray[is].y;
				}
				part->m_outline_stroke[pos] = m_dlist->Add(	pid, this, lay_map, DL_LINES_ARRAY, 1, &br, w, PTS, npts );
				if( !el_RctGr )
					el_RctGr = part->m_outline_stroke[pos];
				else if( !el_magical )
						el_magical = part->m_outline_stroke[pos];
				delete PTS;
				pos++;
			}
		}
		if( el_magical )
			if( el_magical->next != m_dlist->Get_End() )
				el_magical->magic = m_dlist->Get_End()->prev;
		el_magical = NULL;
	}
	part->m_outline_stroke.SetSize(pos);

	// draw padstacks and save absolute position of pins
	pid.type = ID_PART;
	pid = part->m_id;
	pid.st = ID_PAD;	
	const int mask_lrs = min(2,m_layers);	//( LAY_SM_TOP, LAY_SM_BOTTOM )
	const int copper_lrs = min(3,m_layers);	//( TOP, BOTTOM, INNER )
	for( pid.i=0; pid.i<shape->GetNumPins(); pid.i++ ) 
	{
		// set layer for pads
		padstack * ps = &shape->m_padstack[pid.i];
		part_pin * pin = &part->pin[pid.i];
		pin->dl_els.SetSize(copper_lrs+mask_lrs);
		pad * p;
		CPoint pin_pt = GetPinPoint(part,pid.i,part->side,part->angle);
		pin->x = pin_pt.x;
		pin->y = pin_pt.y;
		int ps_ang = -angle - ps->angle;
		if( part->side )
			ps_ang = -angle + ps->angle;

		// iterate through all copper layers 
		// pad rect
		RECT * pad_bounds = &pin->bounds;
		pad_bounds->left = pad_bounds->bottom = INT_MAX;
		pad_bounds->right = pad_bounds->top = INT_MIN;

		int i_el = 0;
		int pin_sel_lay_map = 0;
		for( int il=0; il<3; il++ )
		{
			if( il >= m_layers )
				break;
			int pad_layer = il + LAY_TOP_COPPER;		
			
			// get appropriate pad
			padstack * ps = &shape->m_padstack[pid.i];
			pad * p = NULL;
			if( pad_layer == LAY_TOP_COPPER )
			{
				if( part->side == 0 )
					p = &ps->top;
				else 
					p = &ps->bottom;
			}
			else if( pad_layer == LAY_BOTTOM_COPPER )
			{
				if( part->side == 0 )
					p = &ps->bottom;
				else 
					p = &ps->top;
			}
			else if( ps->hole_size )
				p = &ps->inner;
			//
			if( ps->hole_size )
				setbit( pin_sel_lay_map, pad_layer );
			if( p )
			{
				int SwellPad = 0;
				for( int step=0; step<3; step++ )
				{
					// draw pad
					dl_element * pad_el = NULL;
					if( p->shape != PAD_NONE )
					{
						lay_map = 0;
						if( il == 2 )
						{
							for( int sl=2; sl<m_layers; sl++ )
							{
								setbit( lay_map, (sl+LAY_TOP_COPPER) );
								setbit( pin_sel_lay_map, (sl+LAY_TOP_COPPER) );
							}
						}
						else
						{
							setbit( lay_map, pad_layer );
							setbit( pin_sel_lay_map, pad_layer );
						}
					}
					if( p->shape == PAD_ROUND )
					{	
						// add to display list
						int swp = p->size_Y/2 + SwellPad;
						RECT r;
						r.left =	(pin->x - swp);
						r.bottom =	(pin->y - swp);
						r.right =	(pin->x + swp);
						r.top =		(pin->y + swp);
						if( step < 2 )
							pad_el = m_dlist->Add(	pid, part, lay_map, DL_CIRC, 1, 
													&r, 0, NULL, 0 );

						// rectangels do rotation
						SwellRect(pad_bounds,r);
					}
					else if( p->shape != PAD_NONE )
					{
						int size_X = p->size_X;
						int size_Y = p->size_Y;
						float radius = p->radius + SwellPad;
						if( p->shape == PAD_SQUARE || p->shape == PAD_OCTAGON )
						{
							size_X = size_Y;
							radius = 0;
						}
						else if( p->shape == PAD_RECT )
						{
							radius = 0;
						}
						else if( p->shape == PAD_OVAL )
							radius = min( ( size_X/2+SwellPad ), ( size_Y/2+SwellPad ) );
						RECT r;
						r.left =   pin->x - size_X/2 - SwellPad;
						r.right =  pin->x + size_X/2 + SwellPad;
						r.top =    pin->y + size_Y/2 + SwellPad;
						r.bottom = pin->y - size_Y/2 - SwellPad;

						// rectangels do rotation
						SwellRect( pad_bounds, r );

						if( step < 2 )
						{
							if( p->shape == PAD_OCTAGON )
							{
								size_X = size_Y;
								float fX = size_X + 2*SwellPad;
								radius = fX/3.48;
								int dx = fX;
								int dy = size_Y + 2*SwellPad;		
								CPoint pad_p[8];
								int npts = Gen_RndRectPoly( pin->x, pin->y, dx, dy, radius, ps_ang, pad_p, 8 );
								if( ps_ang )
									RotateRect(&r, ps_ang, pin_pt);
								pad_el = m_dlist->Add(	pid, part, lay_map, DL_POLYGON, 1,
														&r, 0, pad_p, npts );
							}
							else if( ps_ang%90 )
							{
								int dx = size_X - 2*radius + 2*SwellPad;
								int dy = size_Y - 2*radius + 2*SwellPad;					
								CPoint pad_p[4];
								int npts = Gen_RndRectPoly( pin->x, pin->y, dx, dy, 0, ps_ang, pad_p, 4 );
								RotateRect(&r, ps_ang, pin_pt);
								pad_el = m_dlist->Add(	pid, part, lay_map, DL_POLYGON, 1,
														&r, 2*radius, pad_p, npts );
							}
							else
							{
								if( ps_ang )
									RotateRect(&r, ps_ang, pin_pt);
								int gtype;
								if( radius == 0 )
								{
									gtype = DL_RECT;
								}
								else
								{
									gtype = DL_RRECT;
								}
								pad_el = m_dlist->Add(	pid, part, lay_map, gtype, 1,
														&r, radius, NULL, 0 );
							}
						}
					}
					if( pad_el )
					{
						pin->dl_els[i_el] = pad_el;
						i_el++;
					}
					// TOP
					if( part->side )
						pad_layer = LAY_SM_BOTTOM;
					else
						pad_layer = LAY_SM_TOP;
					if( p == &ps->top )
					{
						p = &ps->top_mask;
						if(p)
						{
							if( p->shape == PAD_DEFAULT )
							{
								p = &ps->top;
								SwellPad = m_swell_pad_for_solder_mask;
							}
						}
						if(p)
							continue;
					}
					if( p == &ps->top_mask )
					{
						p = &ps->top_paste;
						if(p)
						{
							if( p->shape == PAD_DEFAULT )
							{
								p = &ps->top;
								SwellPad = m_swell_pad_for_paste_mask;
							}
						}
						if(p)
							continue;
					}
					// BOT
					if( part->side )
						pad_layer = LAY_SM_TOP;
					else
						pad_layer = LAY_SM_BOTTOM;
					if( p == &ps->bottom )
					{
						p = &ps->bottom_mask;		
						if(p)
						{
							if( p->shape == PAD_DEFAULT )
							{
								p = &ps->bottom;
								SwellPad = m_swell_pad_for_solder_mask;
							}
						}
						if(p)
							continue;
					}
					if( p == &ps->bottom_mask )
					{
						p = &ps->bottom_paste;
						if(p)
						{
							if( p->shape == PAD_DEFAULT )
							{
								p = &ps->bottom;
								SwellPad = m_swell_pad_for_paste_mask;
							}
						}	
						if(p)
							continue;
					}
					break;
				}
			}
		}
		// if through-hole pad, just draw hole and set pin_dl_el;
		if( ps->hole_size )
		{
			// add to display list
			RECT r;
			r.left =   pin->x - ps->hole_size/2;
			r.right =  pin->x + ps->hole_size/2;
			r.top =    pin->y + ps->hole_size/2;
			r.bottom = pin->y - ps->hole_size/2;
			setbit( pin_sel_lay_map, LAY_PAD_THRU );
			int hole = 0;
			setbit( hole, LAY_PAD_THRU );
			pin->dl_hole = m_dlist->Add( pid, part, hole, 
										 DL_HOLE, 1, 
										 &r, 0, NULL, 0 );  
			SwellRect( pad_bounds, r );
		}
		else
			pin->dl_hole = NULL;
		if( pad_bounds->left != INT_MAX )
		{
			pts[0].x = pad_bounds->left;
			pts[0].y = pad_bounds->bottom;
			pts[1].x = pad_bounds->left;
			pts[1].y = pad_bounds->top;
			pts[2].x = pad_bounds->right;
			pts[2].y = pad_bounds->top;
			pts[3].x = pad_bounds->right;
			pts[3].y = pad_bounds->bottom;
			if( ps_ang )
			{
				CPoint org_p;
				org_p.x = pin->x;
				org_p.y = pin->y;
				RotatePOINTS( pts, 4, ps_ang, org_p );
				RotateRect( pad_bounds, ps_ang, org_p );
			}
			SwellRect(&selection,*pad_bounds);
			int EL_W = 10*m_pcbu_per_wu;
			if( getbit( pin_sel_lay_map, LAY_PAD_THRU ) || (pin_sel_lay_map>>LAY_TOP_COPPER) )
				pin->dl_sel = m_dlist->AddSelector( pid, part, DL_RECT_X, 1,
												pad_bounds, EL_W,
												pts, 4, pin_sel_lay_map );
			else
			{
				id Mid = pid;
				Mid.st = ID_SM_CUTOUT;
				pin->dl_sel = m_dlist->AddSelector( Mid, part, DL_RECT_X, 1,
												pad_bounds, EL_W,
												pts, 4, pin_sel_lay_map );
			}
			if( !el_RctGr )
				el_RctGr = pin->dl_sel;
		}
	}

	pid = part->m_id;
	pid.st = ID_SEL_RECT;
	RECT sel = part->shape->GetBounds(1);
	pts[0].x = sel.left;
	pts[0].y = sel.bottom;
	pts[1].x = sel.left;
	pts[1].y = sel.top;
	pts[2].x = sel.right;
	pts[2].y = sel.top;
	pts[3].x = sel.right;
	pts[3].y = sel.bottom;
	if( part->side )
	{
		pts[0].x = -pts[0].x;
		pts[1].x = -pts[1].x;
		pts[2].x = -pts[2].x;
		pts[3].x = -pts[3].x;
	}
	if( part->angle )
	{
		RotatePOINTS( pts, 4, -part->angle, zero );
	}

	pts[0].x += part->x;
	pts[0].y += part->y;
	pts[1].x += part->x;
	pts[1].y += part->y;
	pts[2].x += part->x;
	pts[2].y += part->y;
	pts[3].x += part->x;
	pts[3].y += part->y;
	int len1 = Distance(pts[0].x,pts[0].y,pts[1].x,pts[1].y);
	int len2 = Distance(pts[1].x,pts[1].y,pts[2].x,pts[2].y);
	int len3 = selection.right - selection.left;
	int len4 = selection.top - selection.bottom;
	if( selection.left >= INT_MAX )
	{
		if( part->m_ref_vis == 0 || part->m_ref_size != NM_PER_MIL*50 )
		{
			CString strp;
			strp.Format("  The Part %s has no visibility, so visibility will be set for reference designator", part->ref_des );
			part->m_ref_vis = 1;
			part->m_ref_size = NM_PER_MIL*50;
			part->m_ref_w = NM_PER_MIL*5;
			part->m_ref_angle = 0;
			part->m_ref_xi = 0;
			part->m_ref_yi = 0;
			AfxMessageBox(strp);
			part->drawn = TRUE;
			UndrawPart(part);
			DrawPart(part);
			return PL_NOERR;
		}
		pts[0].x = selection.left =		part->x;
		pts[0].y = selection.bottom =	part->y;
		pts[1].x = selection.left =		part->x;
		pts[1].y = selection.top =		part->y+NM_PER_MIL*10;
		pts[2].x = selection.right =	part->x+NM_PER_MIL*10;
		pts[2].y = selection.top =		part->y+NM_PER_MIL*10;
		pts[3].x = selection.right =	part->x+NM_PER_MIL*10;
		pts[3].y = selection.bottom =	part->y;
	}
	else if( len3+len4 < len1+len2 )
	{
		pts[0].x = selection.left;
		pts[0].y = selection.bottom;
		pts[1].x = selection.left;
		pts[1].y = selection.top;
		pts[2].x = selection.right;
		pts[2].y = selection.top;
		pts[3].x = selection.right;
		pts[3].y = selection.bottom;
	}
	int sel_layer_map = 0;
	if( !part->side )
		setbit( sel_layer_map, LAY_TOP_COPPER );
	else
		setbit( sel_layer_map, LAY_BOTTOM_COPPER );
	int gt = DL_RECT;
	if( part->angle%90 )
		gt = DL_POLYGON;
	SwellRect(&selection,_2540);
	part->dl_sel = m_dlist->AddSelector(	pid, part, gt, 1,
											&selection, 0, pts, 4, sel_layer_map );
	if( el_RctGr )
		if( el_RctGr->next != m_dlist->Get_End() )
		{
			m_dlist->Set_RectGroup(el_RctGr, selection.left, selection.right, selection.bottom, selection.top );
			el_RctGr->magic = m_dlist->Get_End()->prev;
		}
	part->drawn = TRUE;
	return PL_NOERR;
}

// Undraw part from display list
//
int CPartList::UndrawPart( cpart * part )
{
	int i;

	if( !m_dlist )
		return 0;
	if( !part )
		return 0;
	if( part->drawn == FALSE )
		return 0;

	CShape * shape = part->shape;
	if( shape )
	{
		// undraw selection rectangle
		m_dlist->Remove( part->dl_sel );
		part->dl_sel = 0;
		m_dlist->Remove( part->dl_ref_el );
		part->dl_ref_el = 0;
		// undraw selection rectangle for ref text
		m_dlist->Remove( part->dl_ref_sel );
		part->dl_ref_sel = 0;
		m_dlist->Remove( part->dl_value_el );
		part->dl_value_el = 0;
		// undraw selection rectangle for value
		m_dlist->Remove( part->dl_value_sel );
		part->dl_value_sel = 0;

		// undraw part outline (this also includes footprint free text)
		for( i=part->m_outline_stroke.GetSize()-1; i>=0; i-- )
		{
			m_dlist->Remove( (dl_element*)part->m_outline_stroke[i] );
			part->m_outline_stroke[i] = 0;
		}

		// undraw padstacks
		for( i=part->pin.GetSize()-1; i>=0; i-- )
		{
			part_pin * pin = &part->pin[i];
			for( int il=pin->dl_els.GetSize()-1; il>=0; il-- )
				m_dlist->Remove( pin->dl_els[il] );
			pin->dl_els.RemoveAll();
			m_dlist->Remove( pin->dl_hole );
			m_dlist->Remove( pin->dl_sel );
			pin->dl_hole = NULL;
			pin->dl_sel = NULL;
		}
	}
	part->drawn = FALSE;
	return PL_NOERR;
}

// the footprint was changed for a particular part
// note that this function also updates the netlist
//
void CPartList::PartFootprintChanged( cpart * part, CShape * new_shape )
{
	UndrawPart( part );
	// new footprint
	part->shape = new_shape;
	part->pin.SetSize( new_shape->GetNumPins() );
	// calculate new pin positions
	if( part->shape && new_shape )
	{
		int dx=0, dy=0;
		if( part->shape->origin_moved_X || part->shape->origin_moved_Y )
		{
			dx = part->shape->origin_moved_X;
			dy = part->shape->origin_moved_Y;
		}
		else if( new_shape->origin_moved_X || new_shape->origin_moved_Y )
		{
			dx = new_shape->origin_moved_X;
			dy = new_shape->origin_moved_Y;
		}
		if( dx || dy )
		{
			part->m_ref_xi += dx;
			part->m_ref_yi += dy;
			part->m_value_xi += dx;
			part->m_value_yi += dy;
			if( part->side )
				dx = -dx;
			if( 180-part->angle )
				Rotate_i_Vertex( &dx, &dy, 180-part->angle, 0, 0 );
			Move( part, part->x+dx, part->y+dy,
				part->angle, part->side, 1 );

		}
		for( int ip=0; ip<part->pin.GetSize(); ip++ )
		{
			CPoint pt = GetPinPoint( part, ip, part->side, part->angle );
			part->pin[ip].x = pt.x;
			part->pin[ip].y = pt.y;
		}
	}
	DrawPart( part );
	m_nlist->PartFootprintChanged( part );
}

// the footprint was modified, apply to all parts using it
//
void CPartList::FootprintChanged( CShape * shape )
{
	// find all parts with given footprint and update them
	cpart * part = m_start.next;
	while( part->next != 0 )
	{
		if( part->shape )
		{
			if(  part->shape->m_name == shape->m_name  )
			{
				PartFootprintChanged( part, shape );
			}
		}
		part = part->next;
	}
}

// the ref text height and width were modified, apply to all parts using it
//
void CPartList::RefTextSizeChanged( CShape * shape )
{
	// find all parts with given shape and update them
	cpart * part = m_start.next;
	while( part->next != 0 )
	{
		if(  part->shape->m_name == shape->m_name  )
		{
			ResizeRefText( part, shape->m_ref_size, shape->m_ref_w );
		}
		part = part->next;
	}
}

// Make part visible or invisible, including thermal reliefs
//
void CPartList::MakePartVisible( cpart * part, BOOL bVisible )
{
	// make part elements invisible, including copper area connections
	// outline strokes
	for( int i=0; i<part->m_outline_stroke.GetSize(); i++ )
	{
		dl_element * el = part->m_outline_stroke[i];
		if( el )
			el->visible = bVisible;
	}
	// pins
	if( part->shape )
	{
		for( int ip=0; ip<part->shape->m_padstack.GetSize(); ip++ )
		{
			// pin pads
			dl_element * el = part->pin[ip].dl_hole;
			if( el )
				el->visible = bVisible;
			for( int i=part->pin[ip].dl_els.GetSize()-1; i>=0; i-- )
			{
				if( part->pin[ip].dl_els[i] )
					part->pin[ip].dl_els[i]->visible = bVisible;
			}
			// pin copper area connections
			cnet * net = (cnet*)part->pin[ip].net;
			if( net )
			{
				for( int ia=0; ia<net->nareas; ia++ )
				{
					for( int i=0; i<net->area[ia].npins; i++ )
					{
						if( net->pin[net->area[ia].pin[i]].part == part )
						{
							net->area[ia].dl_thermal[i]->visible = bVisible;
						}
					}
				}
			}
		}
	}
	// ref text strokes
	if( part->dl_ref_el )
		part->dl_ref_el->visible = bVisible;
	// value strokes
	if( part->dl_value_el )
		part->dl_value_el->visible = bVisible;
}

// Start dragging part by setting up display list
// if bRatlines == FALSE, no rubber-band ratlines
// else if bBelowPinCount == TRUE, only use ratlines for nets with less than pin_count pins
//
int CPartList::StartDraggingPart( CDC * pDC, cpart * part, BOOL bRatlines, 
								 BOOL bBelowPinCount, int pin_count, BOOL bSaveStartPos )
{
	if( !part->shape )
		return 0;
	if( !part->drawn )
		DrawPart( part );

	// save position
	if( bSaveStartPos )
	{
		begin_dragging_x = part->x;
		begin_dragging_y = part->y;
		begin_dragging_ang = part->angle;
		begin_dragging_side = part->side;
	}

	// make part invisible
	MakePartVisible( part, FALSE );
	m_dlist->CancelHighLight();

	// create drag lines
	int psz = part->shape->m_padstack.GetSize();
	CArray<CPoint> pin_points;
	pin_points.SetSize( part->shape->m_padstack.GetSize() );
	if( psz == 0 )
	{
		m_dlist->MakeDragLineArray( 4 );
		CPoint P[4];
		int np = 4;
		m_dlist->Get_Points( part->dl_sel, P, &np, -part->x, -part->y );
		if( np == 4 )
		{
			for( int ii=0; ii<np-1; ii++ )
				m_dlist->AddDragLine( P[ii], P[ii+1] );
			m_dlist->AddDragLine( P[3], P[0] );
		} 
		else // rectangle
		{
			RECT Get;
			m_dlist->Get_Rect( part->dl_sel, &Get );
			CPoint pt[4];
			pt[0].x = Get.left		- part->x;
			pt[0].y = Get.bottom	- part->y;
			pt[1].x = Get.left		- part->x;
			pt[1].y = Get.top		- part->y;
			pt[2].x = Get.right		- part->x;
			pt[2].y = Get.top		- part->y;
			pt[3].x = Get.right		- part->x;
			pt[3].y = Get.bottom	- part->y;
			m_dlist->AddDragLine( P[0], P[3] );
			m_dlist->AddDragLine( P[0], P[1] );
			m_dlist->AddDragLine( P[1], P[2] );
			m_dlist->AddDragLine( P[2], P[3] );
		}
	}
	else
	{
		m_dlist->MakeDragLineArray( 4*part->shape->m_padstack.GetSize() );
		for( int ip=0; ip<part->shape->m_padstack.GetSize(); ip++ )
		{
			if( part->pin[ip].dl_sel == 0 )
				continue;
			CPoint P[4];
			int np = 4;
			m_dlist->Get_Points( part->pin[ip].dl_sel, P, &np, -part->x, -part->y );
			if( np == 4 )
			{
				for( int ii=0; ii<np-1; ii++ )
					m_dlist->AddDragLine( P[ii], P[ii+1] );
				m_dlist->AddDragLine( P[3], P[0] );
			} 
			else // rectangle
			{
				RECT Get;
				m_dlist->Get_Rect( part->pin[ip].dl_sel, &Get );
				CPoint pt[4];
				pt[0].x = Get.left    - part->x;
				pt[0].y = Get.bottom  - part->y;
				pt[1].x = Get.left	  - part->x;
				pt[1].y = Get.top	  - part->y;
				pt[2].x = Get.right   - part->x;
				pt[2].y = Get.top	  - part->y;
				pt[3].x = Get.right   - part->x;
				pt[3].y = Get.bottom  - part->y;
				m_dlist->AddDragLine( P[0], P[3] );
				m_dlist->AddDragLine( P[0], P[1] );
				m_dlist->AddDragLine( P[1], P[2] );
				m_dlist->AddDragLine( P[2], P[3] );
			}
			// save pin position
			pin_points[ip].x = part->pin[ip].x - part->x;
			pin_points[ip].y = part->pin[ip].y - part->y;
		}
	}
	// create drag lines for ratlines connected to pins
	if( bRatlines ) 
	{
		m_dlist->MakeDragRatlineArray( 2*part->shape->m_padstack.GetSize(), 1 );
		// zero utility flags for all nets
		cnet * n = m_nlist->GetFirstNet();
		while( n )
		{
			n->utility = 0;
			n = m_nlist->GetNextNet(/*LABEL*/);
		}

		// now loop through all pins in part to find nets that connect
		for( int ipp=0; ipp<part->shape->m_padstack.GetSize(); ipp++ )
		{
			n = (cnet*)part->pin[ipp].net;
			if( n )
			{
				// only look at visible nets, only look at each net once
				if( /*n->visible &&*/ n->utility == 0 )
				{
					// zero utility flags for all connections
					for( int ic=0; ic<n->nconnects; ic++ )
					{
						n->connect[ic].utility = 0;
					}
					for( int ic=0; ic<n->nconnects; ic++ )
					{
						cconnect * c = &n->connect[ic];
						if( c->utility )
							continue;	// skip this connection

						// check for connection to part
						int pin1 = n->connect[ic].start_pin;
						int pin2 = n->connect[ic].end_pin;
						cpart * pin1_part = n->pin[pin1].part;
						cpart * pin2_part = NULL;
						if( pin2 != cconnect::NO_END )
							pin2_part = n->pin[pin2].part;
						if( pin1_part != part && pin2_part != part )
							continue;	// no

						// OK, this connection is attached to our part 
						if( pin1_part == part )
						{
							int ip = n->connect[ic].start_pin_shape;
							if( ip != -1 )
							{
								// ip is the start pin for the connection
								// hide segment
								c->seg[0].dl_el->visible = 0;
								// add ratline
//**								if( !bBelowPinCount || n->npins <= pin_count )
								{
									BOOL bDraw = FALSE;
									if( pin2_part == part )
									{
										// connection starts and ends on this part,
										// only drag if 3 or more segments
										if( c->nsegs > 2 )
											bDraw = TRUE;
									}
									else 
										bDraw = TRUE;
									if( bDraw )
									{
										// add dragline from pin to second vertex
										CPoint vx( c->vtx[1].x, c->vtx[1].y );
										m_dlist->AddDragRatline( vx, pin_points[ip] );
									}
								}
							}
						}
						if( pin2_part == part )
						{
							int ip = -1;
							if( pin2 != cconnect::NO_END )
								ip = n->connect[ic].end_pin_shape;
							if( ip != -1 )
							{
								// ip is the end pin for the connection
								c->seg[c->nsegs-1].dl_el->visible = 0;
								// OK, get prev vertex, add ratline and hide segment
//**								if( !bBelowPinCount || n->npins <= pin_count )
								{
									BOOL bDraw = FALSE;
									if( pin1_part == part )
									{
										// starts and ends on part
										if( c->nsegs > 2 )
											bDraw = TRUE;
									}
									else
										bDraw = TRUE;
									if( bDraw )
									{
										CPoint vx( n->connect[ic].vtx[c->nsegs-1].x, n->connect[ic].vtx[c->nsegs-1].y );
										m_dlist->AddDragRatline( vx, pin_points[ip] );
									}
								}
							}
						}
						c->utility = 1;	// this connection has been checked
					}
				}
				n->utility = 1;	// all connections for this net have been checked
			}
		}
	}
	m_dlist->StartDraggingArray( pDC, part->x, part->y );
	return 0;
}

// start dragging ref text
//
int CPartList::StartDraggingRefText( CDC * pDC, cpart * part )
{
	// make ref text elements invisible
	if( part->dl_ref_el )
		part->dl_ref_el->visible = 0;

	// cancel selection 
	m_dlist->CancelHighLight();
	CPoint P[4];
	int np = 4;
	// create drag lines
	m_dlist->MakeDragLineArray( np );
	CPoint ref_p = GetRefPoint( part );
	m_dlist->Get_Points( part->dl_ref_sel, P, &np, -ref_p.x, -ref_p.y );
	if( np == 4 )
	{
		for( int ii=0; ii<np-1; ii++ )
			m_dlist->AddDragLine( P[ii], P[ii+1] );
		m_dlist->AddDragLine( P[3], P[0] );
	} 
	else //rectangle
	{
		RECT Get;
		m_dlist->Get_Rect( part->dl_ref_sel, &Get );
		CPoint pt[4];
		pt[0].x = Get.left;
		pt[0].y = Get.bottom;
		pt[1].x = Get.left;
		pt[1].y = Get.top;
		pt[2].x = Get.right;
		pt[2].y = Get.top;
		pt[3].x = Get.right;
		pt[3].y = Get.bottom;
		m_dlist->AddDragLine( pt[0], pt[3] );
		m_dlist->AddDragLine( pt[0], pt[1] );
		m_dlist->AddDragLine( pt[1], pt[2] );
		m_dlist->AddDragLine( pt[2], pt[3] );
	}
	m_dlist->StartDraggingArray( pDC, ref_p.x, ref_p.y );
	return 0;
}

// start dragging value
//
int CPartList::StartDraggingValue( CDC * pDC, cpart * part )
{
	// make value text elements invisible
	if( part->dl_value_el )
		part->dl_value_el->visible = 0;
	int rx,ry;
	// cancel selection 
	m_dlist->CancelHighLight();
	CPoint P[4];
	int np = 4;
	// create drag lines
	m_dlist->MakeDragLineArray( np );
	CPoint value_p = GetValuePoint( part );
	m_dlist->Get_Points( part->dl_value_sel, P, &np, -value_p.x, -value_p.y );
	if( np == 4 )
	{
		for( int ii=0; ii<np-1; ii++ )
			m_dlist->AddDragLine( P[ii], P[ii+1] );
		m_dlist->AddDragLine( P[3], P[0] );
	} 
	else //rectangle
	{
		RECT Get;
		m_dlist->Get_Rect( part->dl_value_sel, &Get );
		CPoint pt[4];
		pt[0].x = Get.left;
		pt[0].y = Get.bottom;
		pt[1].x = Get.left;
		pt[1].y = Get.top;
		pt[2].x = Get.right;
		pt[2].y = Get.top;
		pt[3].x = Get.right;
		pt[3].y = Get.bottom;
		m_dlist->AddDragLine( pt[0], pt[3] );
		m_dlist->AddDragLine( pt[0], pt[1] );
		m_dlist->AddDragLine( pt[1], pt[2] );
		m_dlist->AddDragLine( pt[2], pt[3] );
	}
	m_dlist->StartDraggingArray( pDC, value_p.x, value_p.y );
	return 0;
}

// cancel dragging, return to pre-dragging state
//
int CPartList::CancelDraggingPart( cpart * part )
{
	Move(part,begin_dragging_x,begin_dragging_y,begin_dragging_ang,begin_dragging_side);
	m_nlist->PartMoved(part,0);
	// make part visible again
	MakePartVisible( part, TRUE );

	// get any connecting segments and make visible
	for( int ip=0; ip<part->shape->m_padstack.GetSize(); ip++ )
	{
		cnet * net = (cnet*)part->pin[ip].net;
		if( net )
		{
			if( net->visible )
			{
				for( int ic=0; ic<net->nconnects; ic++ )
				{
					cconnect * c = &net->connect[ic];
					int pin1 = c->start_pin;
					int pin2 = c->end_pin;
					int nsegs = c->nsegs;
					if( net->pin[pin1].part == part )
					{
						// start pin
						c->seg[0].dl_el->visible = 1;
					}
					if( pin2 != cconnect::NO_END )
					{
						if( net->pin[pin2].part == part )
						{
							// end pin
							c->seg[nsegs-1].dl_el->visible = 1;
						}
					}
				}
			}
		}
	}
	m_dlist->StopDragging();
	return 0;
}

// cancel dragging of ref text, return to pre-dragging state
int CPartList::CancelDraggingRefText( cpart * part )
{
	// make ref text elements invisible
	if( part->dl_ref_el )
		part->dl_ref_el->visible = 1;
	m_dlist->StopDragging();
	return 0;
}

// cancel dragging value, return to pre-dragging state
int CPartList::CancelDraggingValue( cpart * part )
{
	// make ref text elements invisible
	if( part->dl_value_el )
		part->dl_value_el->visible = 0;
	m_dlist->StopDragging();
	return 0;
}

// normal completion of any dragging operation
//
int CPartList::StopDragging()
{
	m_dlist->StopDragging();
	return 0;
}

// create part from string
//
cpart * CPartList::AddFromString( CString * str )
{
	CShape * s = NULL;
	CString in_str, key_str;
	CArray<CString> p;
	int pos = 0;
	int len = str->GetLength();
	int np;
	CString ref_des = "";
	BOOL ref_vis = FALSE;
	int ref_size = 0;
	int ref_width = 0;
	int ref_angle = 0;
	int ref_xi = 0;
	int ref_yi = 0;
	CString value = "";
	BOOL value_vis = FALSE;
	int value_size = 0;
	int value_width = 0;
	int value_angle = 0;
	int value_xi = 0;
	int value_yi = 0;
	CString package = "";
	int x = 0;
	int y = 0;
	int side = 0;
	int angle = 0;
	int glued = 0;
	int merge = -1;
	//
	in_str = str->Tokenize( "\n", pos );
	while( in_str.GetLength() )
	{
		np = ParseKeyString( &in_str, &key_str, &p );
		if( key_str == "ref" )
		{
			ref_des = in_str.Right( in_str.GetLength()-4 );
			ref_des.Trim();
			ref_des = ref_des.Left(MAX_REF_DES_SIZE);
		}
		else if( key_str == "part" )
		{
			ref_des = in_str.Right( in_str.GetLength()-5 );
			ref_des.Trim();
			ref_des = ref_des.Left(MAX_REF_DES_SIZE);
		}
		else if( np >= 6 && key_str == "ref_text" )
		{
			ref_size = my_atoi( &p[0] );
			ref_width = my_atoi( &p[1] );
			ref_angle = my_atoi( &p[2] );
			ref_xi = my_atoi( &p[3] );
			ref_yi = my_atoi( &p[4] );
			if( np >= 7 )
				ref_vis = my_atoi( &p[5] );
			else
			{
				if( ref_size )
					ref_vis = TRUE;
				else
					ref_vis = FALSE;
			}
		}
		else if( np >= 7 && key_str == "value" )
		{
			value = p[0];
			value_size = my_atoi( &p[1] );
			value_width = my_atoi( &p[2] );
			value_angle = my_atoi( &p[3] );
			value_xi = my_atoi( &p[4] );
			value_yi = my_atoi( &p[5] );
			if( np >= 8 )
				value_vis = my_atoi( &p[6] );
			else
			{
				if( value_size )
					value_vis = TRUE;
				else
					value_vis = FALSE;
			}
		}
		else if( key_str == "package" )
		{
			if( np >= 2 )
				package = p[0];
			else
				package = "";
			package = package.Left(CShape::MAX_NAME_SIZE);
		}
		else if( key_str == "merge" )
		{
			if( np >= 2 )
				merge = my_atoi( &p[0] );
		}
		else if( np >= 2 && key_str == "shape" )
		{
			// lookup shape in cache
			s = NULL;
			void * ptr;
			CString name = p[0];
			name = name.Left(CShape::MAX_NAME_SIZE);
			int err = m_footprint_cache_map->Lookup( name, ptr );
			if( err )
			{
				// found in cache
				s = (CShape*)ptr; 
			}
		}
		else if( key_str == "pos" )
		{
			if( np >= 6 )
			{
				x = my_atoi( &p[0] );
				y = my_atoi( &p[1] );
				side = my_atoi( &p[2] );
				angle = my_atoi( &p[3] );
				glued = my_atoi( &p[4] );
			}
			else
			{
				x = 0;
				y = 0;
				side = 0;
				angle = 0;
				glued = 0;
			}
		}
		in_str = str->Tokenize( "\n", pos );
	}
	if( ref_des.GetLength() == 0 )
		return 0;
	cpart * part = GetPart( ref_des );
	if( part )
		return part;
	// so we only draw once
	CDisplayList * old_dlist = m_dlist;
	m_dlist = NULL;
	part = Add();
	SetPartData( part, s, &ref_des, x, y, side, angle, 1, glued, merge );
	SetValue( part, &value, value_xi, value_yi, value_angle, value_size, value_width, value_vis );
	if( part->shape ) 
	{
		part->m_ref_xi = ref_xi;
		part->m_ref_yi = ref_yi;
		part->m_ref_angle = ref_angle;
		ResizeRefText( part, ref_size, ref_width, ref_vis, 0 );
	}
	m_dlist = old_dlist;
	DrawPart( part );
	return part;
}

// read partlist
//
int CPartList::ReadParts( CStdioFile * pcb_file )
{
	int pos, err;
	CString in_str, key_str;

	// find beginning of [parts] section
	do
	{
		err = pcb_file->ReadString( in_str );
		if( !err )
		{
			// error reading pcb file
			CString mess;
			mess.Format( "Unable to find [parts] section in file" );
			AfxMessageBox( mess );
			return 0;
		}
		in_str.Trim();
	}
	while( in_str != "[parts]" );

	// get each part in [parts] section
	while( 1 )
	{
		pos = pcb_file->GetPosition();
		err = pcb_file->ReadString( in_str );
		if( !err )
		{
			CString * err_str = new CString( "unexpected EOF in project file" );// throw
			throw err_str;
		}
		in_str.Trim();
		if( in_str.Left(1) == "[" && in_str != "[parts]" )
		{
			pcb_file->Seek( pos, CFile::begin );
			break;		// next section, exit
		}
		else if( in_str.Left(4) == "ref:" || in_str.Left(5) == "part:" )
		{
			CString str;
			do
			{
				str.Append( in_str );
				str.Append( "\n" );
				pos = pcb_file->GetPosition();
				err = pcb_file->ReadString( in_str );
				if( !err )
				{
					CString * err_str = new CString( "unexpected EOF in project file" );// throw
					throw err_str;
				}
				in_str.Trim();
			} while( (in_str.Left(4) != "ref:" && in_str.Left(5) != "part:" )
						&& in_str[0] != '[' );
			pcb_file->Seek( pos, CFile::begin );

			// now add part to partlist
			AddFromString( &str );
		}
	}
	return 0;
}

// set CString to description of part
//
int CPartList::SetPartString( cpart * part, CString * str )
{
	CString line;
	if( part->ref_des.GetLength() == 0 )
	{
		*str = "";
		return 0;
	}
	line.Format( "part: %s\n", part->ref_des );  
	*str = line;
	if( part->shape )
	{
		line.Format( "  ref_text: %d %d %d %d %d %d\n", 
						part->m_ref_size, part->m_ref_w, 
						part->m_ref_angle%360,
						part->m_ref_xi, part->m_ref_yi, part->m_ref_vis );
		str->Append( line );
		line.Format( "  shape: \"%s\"\n", part->shape->m_name );
		str->Append( line );
		if ( part->m_merge >= 0 )
		{
			line.Format( "  merge: %d\n", part->m_merge );
			str->Append( line );
		}
		if ( part->value.GetLength() )
		{
			line.Format( "  value: \"%s\" %d %d %d %d %d %d\n", 
							part->value, part->m_value_size, 
							part->m_value_w, part->m_value_angle%360,
							part->m_value_xi, part->m_value_yi,
							part->m_value_vis );
			str->Append( line );
		}
		line.Format( "  pos: %d %d %d %d %d\n", part->x, part->y, part->side, part->angle%360, part->glued );
		str->Append( line );
	}
	else if ( part->value.GetLength() )
	{
		line.Format( "  value: \"%s\" %d %d %d %d %d %d\n", 
						part->value, part->m_value_size, 
						part->m_value_w, part->m_value_angle%360,
						part->m_value_xi, part->m_value_yi,
						part->m_value_vis );
		str->Append( line );
	}
	line.Format( "\n" );
	str->Append( line );
	return 0;
}

// create record describing part for use by CUndoList
// if part == NULL, just set m_plist and new_ref_des
//
undo_part * CPartList::CreatePartUndoRecord( cpart * part, CString * new_ref_des )
{
	int NumPins = 0;	
	int size = sizeof( undo_part );
	if( part )
	{
		NumPins = part->pin.GetSize();
		size = sizeof( undo_part ) + NumPins*(CShape::MAX_PIN_NAME_SIZE+1);
	}
	undo_part * upart = (undo_part*)malloc( size );
	upart->size = size;
	upart->m_plist = this;
	if( part )
	{
		char * chptr = (char*)upart;
		chptr += sizeof(undo_part);
		upart->m_id = part->m_id;
		upart->visible = part->visible;
		upart->x = part->x;
		upart->y = part->y;
		upart->side = part->side;
		upart->angle = part->angle;
		upart->glued = part->glued;
		upart->m_ref_vis = part->m_ref_vis;
		upart->m_ref_xi = part->m_ref_xi;
		upart->m_ref_yi = part->m_ref_yi;
		upart->m_ref_angle = part->m_ref_angle;
		upart->m_ref_size = part->m_ref_size;
		upart->m_ref_w = part->m_ref_w;
		upart->m_value_vis = part->m_value_vis;
		upart->m_value_xi = part->m_value_xi;
		upart->m_value_yi = part->m_value_yi;
		upart->m_value_angle = part->m_value_angle;
		upart->m_value_size = part->m_value_size;
		upart->m_value_w = part->m_value_w;
		strcpy( upart->ref_des, part->ref_des );				
		strcpy( upart->value, part->value );
		upart->merge_name = part->m_merge;
		upart->shape = part->shape;
		if( part->shape )
		{
			strcpy( upart->shape_name, part->shape->m_name );
			// save names of nets attached to each pin
			for( int ip=0; ip<NumPins; ip++ )
			{
				if( cnet * net = part->pin[ip].net )
					strcpy( chptr, net->name );
				else
					*chptr = 0;
				chptr += CShape::MAX_PIN_NAME_SIZE + 1;
			}
		}
	}
	if( new_ref_des )
		strcpy( upart->new_ref_des, *new_ref_des );
	else
		strcpy( upart->new_ref_des, part->ref_des );
	return upart;
}

#if 0
// create special record for use by CUndoList
//
void * CPartList::CreatePartUndoRecordForRename( cpart * part, CString * old_ref_des )
{
	int size = sizeof( undo_part );
	undo_part * upart = (undo_part*)malloc( size );
	upart->m_plist = this;
	strcpy( upart->ref_des, part->ref_des );
	upart->merge_name = part->m_merge;
	strcpy( upart->package, *old_ref_des );
	return (void*)upart;
}
#endif

// write all parts and footprints to file
//
int CPartList::WriteParts( CStdioFile * file )
{
	CMapStringToPtr shape_map;
	cpart * el = m_start.next;
	CString line;
	CString key;
	try
	{
		// now write all parts
		line.Format( "[parts]\n\n" );
		file->WriteString( line );
		el = m_start.next;
		while( el->next != 0 )
		{
			// test
			CString test;
			SetPartString( el, &test );
			if( test.GetLength() )
				file->WriteString( test );
			el = el->next;
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




// export part list data into partlist_info structure for editing in dialog
// if test_part != NULL, returns index of test_part in partlist_info
//
int CPartList::ExportPartListInfo( partlist_info * pl, cpart * test_part )
{
	// traverse part list to find number of parts
	int ipart = -1;
	int nparts = 0;
	cpart * part = m_start.next;
	while( part->next != 0 )
	{
		nparts++;
		part = part->next;
	}
	// now make struct
	pl->SetSize( nparts );
	int i = 0;
	part = m_start.next;
	while( part->next != 0 )
	{
		if( part == test_part )
			ipart = i;
		(*pl)[i].part = part;
		(*pl)[i].shape = part->shape;
		(*pl)[i].bShapeChanged = FALSE;
		(*pl)[i].ref_des = part->ref_des;
		if( part->shape )
		{
			(*pl)[i].ref_size = part->m_ref_size;
			(*pl)[i].ref_width = part->m_ref_w;
		}
		else
		{
			(*pl)[i].ref_size = 0;
			(*pl)[i].ref_width = 0;
		}	
		(*pl)[i].value = part->value;
		(*pl)[i].value_vis = part->m_value_vis;
		(*pl)[i].x = part->x;
		(*pl)[i].y = part->y;
		(*pl)[i].angle = part->angle;
		(*pl)[i].side = part->side;
		(*pl)[i].deleted = FALSE;
		(*pl)[i].selected = part->selected;
		(*pl)[i].bOffBoard = FALSE;
		(*pl)[i].mrgs = part->m_merge;
		i++;
		part = part->next;
	}
	return ipart;
}

// import part list data from struct partlist_info
//
void CPartList::ImportPartListInfo( partlist_info * pl, int flags, CDlgLog * log )
{
	CString mess; 
	MarkAllParts(FALSE);	

	// grid for positioning parts off-board
	int pos_x = 0;
	int pos_y = 0;
	enum { GRID_X = PL_MAX_SIZE/100, GRID_Y = 100 };
	BOOL * grid = (BOOL*)calloc( GRID_X*GRID_Y, sizeof(BOOL) );
	int grid_num = 0;

	// first, look for parts in project whose ref_des has been changed
#define REPEAT_REF 1
	int Duplicate, PartRef_Changed;
	do{
		Duplicate = 0;
		PartRef_Changed = 0;
		for( int i=0; i<pl->GetSize(); i++ )
		{
			part_info * pi = &(*pl)[i];
			if( pi->part )
			{
				if( pi->ref_des.Compare(pi->part->ref_des) )
				{
					int d=0;
					// test1 (disabled)
					for( cpart * p=GetFirstPart(); p; p=GetNextPart(p) )
					{
						if( p->ref_des.Compare( pi->ref_des ) == 0 )
						{
							d = 1;
							Duplicate |= d; //fail
							break;
						}
					}
					if( d )
						continue;

					// test2
					d = m_nlist->PartRefChanged( &pi->part->ref_des, &pi->ref_des );
					Duplicate |= d;
					if( d == 0 )
					{
						pi->part->ref_des = pi->ref_des;
						pi->part->utility = 1;
						PartRef_Changed = 1;
					}
				}
				if( pi->selected )
					pi->part->utility = 1;
			}
		}
		if( PartRef_Changed == 0 && Duplicate )
		{
			AfxMessageBox( "Sorry: Parts can't be renumbered :-(" );
			return;
		}
	}while( Duplicate );
#undef REPEAT_REF

	// undraw all parts and disable further drawing
	CDisplayList * old_dlist = m_dlist;
	if( m_dlist )
	{
		cpart * part = GetFirstPart();
		while( part )
		{
			UndrawPart( part );
			part = GetNextPart( part );
		}
	}
	m_dlist = NULL;	

	// now find parts in project that are not in partlist_info
	// loop through all parts in project
	cpart * part = m_start.next;
	while( part->next != 0 )
	{
		// loop through the partlist_info array
		BOOL bFound = FALSE;
		part->bPreserve = FALSE;
		for( int i=0; i<pl->GetSize(); i++ )
		{
			part_info * pi = &(*pl)[i];
			if( pi->ref_des == part->ref_des )
			{
				// part exists in partlist_info
				bFound = TRUE;
				break;
			}
		}
		cpart * next_part = part->next;
		if( !bFound )
		{
			// part in project but not in partlist_info
			if( flags & KEEP_PARTS_AND_CON )
			{
				// set flag to preserve this part
				part->bPreserve = TRUE;
				if( log )
				{
					mess.Format( "  Keeping part %s and connections\r\n", part->ref_des );
					log->AddLine( mess );
				}
			}
			else if( flags & KEEP_PARTS_NO_CON )
			{
				// keep part but remove connections from netlist
				if( log )
				{
					mess.Format( "  Keeping part %s but removing connections\r\n", part->ref_des );
					log->AddLine( mess );
				}
				m_nlist->PartDeleted( part );
			}
			else
			{
				// remove part
				if( log )
				{
					mess.Format( "  Removing part %s\r\n", part->ref_des );
					log->AddLine( mess );
				}
				m_nlist->PartDeleted( part );
				Remove( part );
			}
		}
		part = next_part;
	}

	// loop through partlist_info array, changing partlist as necessary
	for( int i=0; i<pl->GetSize(); i++ )
	{
		part_info * pi = &(*pl)[i];
		if( pi->part == 0 && pi->deleted )
		{
			// new part was added but then deleted, ignore it
			continue;
		}
		if( pi->part != 0 && pi->deleted )
		{
			// old part was deleted, remove it
			m_nlist->PartDisconnected( pi->part );
			Remove( pi->part );
			continue;
		}

		if( pi->part == 0 )
		{
			// the partlist_info does not include a pointer to an existing part
			// the part might not exist in the project, or we are importing a netlist file
			cpart * old_part = GetPart( pi->ref_des );
			if( old_part )
			{
				// an existing part has the same ref_des as the new part
				if( old_part->shape )
				{
					BOOL compare_sh = FALSE;
					if( pi->shape )
						if(pi->shape->m_name.Compare(old_part->shape->m_name) == 0 )
							if(pi->shape->m_package.Compare(old_part->shape->m_package) == 0 )
								compare_sh = TRUE;
					// the existing part has a footprint
					// see if the incoming package name matches the old package or footprint
					if( (flags & KEEP_FP) || compare_sh )
					{
						// use footprint and parameters from existing part
						pi->part = old_part;
						pi->ref_size = old_part->m_ref_size; 
						pi->ref_width = old_part->m_ref_w;
						if( pi->value.GetLength() == 0 )
							pi->value = old_part->value;
						pi->value_vis = old_part->m_value_vis;
						pi->x = old_part->x; 
						pi->y = old_part->y;
						pi->angle = old_part->angle;
						pi->side = old_part->side;
						pi->shape = old_part->shape;
						pi->mrgs = old_part->m_merge;
					}
					else 
					{
						// use new footprint, but preserve position
						pi->part = old_part;
						pi->ref_size = old_part->m_ref_size; 
						pi->ref_width = old_part->m_ref_w;
						if( pi->value.GetLength() == 0 )
							pi->value = old_part->value;
						pi->value_vis = old_part->m_value_vis;
						pi->x = old_part->x; 
						pi->y = old_part->y;
						pi->angle = old_part->angle;
						pi->side = old_part->side;	
						pi->mrgs = old_part->m_merge;
						//
						if( pi->shape )
						{
							pi->bShapeChanged = TRUE;
							if( log && old_part->shape->m_name != pi->shape->m_name )
							{
								mess.Format( "  Changing footprint of part %s from \"%s\" to \"%s\"\r\n", 
									old_part->ref_des, old_part->shape->m_name, pi->shape->m_name );
								log->AddLine( mess );
							}
						}
					}
				}
				else
				{
					// remove old part (which did not have a footprint)
					if( log && pi->shape )
					{
						mess.Format( "  Adding footprint for part %s \"%s\"\r\n", 
							old_part->ref_des, pi->shape->m_package );
						log->AddLine( mess );
					}
					m_nlist->PartDisconnected( old_part );
					Remove( old_part );
				}
			}
		}

		if( pi->part )
		{
			if( pi->part->shape != pi->shape || pi->bShapeChanged == TRUE )
			{
				// old part exists, but footprint was changed
				if( pi->part->shape == NULL )
				{
					// old part did not have a footprint before, so remove it
					// and treat as new part
					m_nlist->PartDisconnected( pi->part );
					Remove( pi->part );
					pi->part = NULL;
				}
			}
		}

		if( pi->part == 0 )
		{
			// new part is being imported (with or without footprint)
			if( pi->shape && pi->bOffBoard )
			{
				// place new part offboard, using grid 
				int ix, iy;	// grid indices
				// find size of part in 100 mil units
				BOOL OK = FALSE;
				int w = abs( pi->shape->selection.right - pi->shape->selection.left )/(100*PCBU_PER_MIL)+2;
				int h = abs( pi->shape->selection.top - pi->shape->selection.bottom )/(100*PCBU_PER_MIL)+2;
				// now find space in grid for part
				for( ix=0; ix<GRID_X; ix++ )
				{
					iy = 0;
					while( iy < (GRID_Y - h) )
					{
						if( !grid[ix+GRID_X*iy] )
						{
							// see if enough space
							OK = TRUE;
							for( int iix=ix; iix<(ix+w); iix++ )
								for( int iiy=iy; iiy<(iy+h); iiy++ )
									if( grid[iix+GRID_X*iiy] )
										OK = FALSE;
							if( OK )
								break;
						}
						iy++;
					}
					if( OK )
						break;
				}
				if( OK )
				{
					// place part
					pi->side = 0;
					pi->angle = 0;
					if( grid_num == 0 )
					{
						// first grid, to left and above origin
						pi->x = -(ix+w)*100*PCBU_PER_MIL;
						pi->y = iy*100*PCBU_PER_MIL;
					}
					else if( grid_num == 1 )
					{
						// second grid, to left and below origin
						pi->x = -(ix+w)*100*PCBU_PER_MIL;
						pi->y = -(iy+h)*100*PCBU_PER_MIL;
					}
					else if( grid_num == 2 )
					{
						// third grid, to right and below origin
						pi->x = ix*100*PCBU_PER_MIL;
						pi->y = -(iy+h)*100*PCBU_PER_MIL;
					}
					// remove space in grid
					for( int iix=ix; iix<(ix+w); iix++ )
						for( int iiy=iy; iiy<(iy+h); iiy++ )
							grid[iix+GRID_X*iiy] = TRUE;
				}
				else
				{
					// fail, go to next grid
					if( grid_num == 2 )
						ASSERT(0);		// ran out of grids
					else
					{
						// zero grid
						for( int j=0; j<GRID_Y; j++ )
							for( int i=0; i<GRID_X; i++ )
								grid[j*GRID_X+i] = FALSE;
						grid_num++;
					}
				}
				// now offset for part origin
				pi->x -= pi->shape->selection.left;
				pi->y -= pi->shape->selection.bottom;
			}
			// now place part
			cpart * part = Add( pi->shape, &pi->ref_des, pi->x, pi->y,
				pi->side, pi->angle, TRUE, FALSE );
			if( part->shape )
			{
				ResizeRefText( part, pi->ref_size, pi->ref_width );
				SetValue( part, &pi->value, 
					part->shape->m_value_xi, 
					part->shape->m_value_yi,
					part->shape->m_value_angle, 
					part->shape->m_value_size, 
					part->shape->m_value_w,
					pi->value_vis );
			}
			else
				SetValue( part, &pi->value, 0, 0, 0, 0, 0 );
			m_nlist->PartAdded( part );
		}
		else
		{
			if( pi->part->value.Compare(pi->value) )
			{
				// value changed, keep size and position
				SetValue( pi->part, &pi->value );
			}
			if( pi->part->m_value_vis != pi->value_vis )
			{
				// value visibility changed
				pi->part->m_value_vis = pi->value_vis;
			}
			if( pi->part->shape != pi->shape || pi->bShapeChanged == TRUE )
			{
				// footprint was changed
				if( pi->part->shape == NULL )
				{
					ASSERT(0);	// should never get here
				}
				else if( pi->shape && !(flags & KEEP_FP) )
				{
					int prevx = pi->part->x;
					int prevy = pi->part->y;
					// change footprint to new one
					PartFootprintChanged( pi->part, pi->shape );
					//
					pi->x += (pi->part->x - prevx);
					pi->y += (pi->part->y - prevy);
				}
			}
			if( pi->x != pi->part->x 
				|| pi->y != pi->part->y
				|| pi->angle != pi->part->angle
				|| pi->side != pi->part->side )
			{
				// part was moved
				Move( pi->part, pi->x, pi->y, pi->angle, pi->side );
				m_nlist->PartMoved( pi->part , FALSE );
			}
		}
	}
	free( grid );

	// redraw partlist
	m_dlist = old_dlist;
	part = GetFirstPart();
	while( part )
	{
		DrawPart( part );
		part = GetNextPart( part );
	}
}

// undo an operation on a part
// note that this is a static function, for use as a callback
//
void CPartList::PartUndoCallback( int type, void * ptr, BOOL undo )
{
	undo_part * upart = (undo_part*)ptr;

	if( undo )
	{
		// perform undo
		CString new_ref_des = upart->new_ref_des;
		CString old_ref_des = upart->ref_des;
		CPartList * pl = upart->m_plist;
		CDisplayList * old_dlist = pl->m_dlist;	
		cpart * part = pl->GetPart( new_ref_des );
		if( part && type == UNDO_PART_ADD )
		{
			// part was added, just delete it		
			pl->m_nlist->PartDeleted( part );
			pl->Remove( part );
		}
		else if( type == UNDO_PART_DELETE )
		{
			// part was deleted, lookup shape in cache and add part
			pl->m_dlist = NULL;		// prevent drawing
			CShape * s;
			void * s_ptr;
			int err = pl->m_footprint_cache_map->Lookup( upart->shape_name, s_ptr );
			if( err )
			{
				// found in cache
				s = (CShape*)s_ptr; 
			}
			else
				ASSERT(0);	// shape not found
			CString ref_des = upart->ref_des;
			part = pl->Add( s, &ref_des, upart->x, upart->y,
				upart->side, upart->angle, upart->visible, upart->glued );
			part->m_merge = upart->merge_name;
			part->m_ref_vis = upart->m_ref_vis;
			part->m_ref_xi = upart->m_ref_xi;
			part->m_ref_yi = upart->m_ref_yi;
			part->m_ref_angle = upart->m_ref_angle;
			part->m_ref_size = upart->m_ref_size;
			part->m_ref_w = upart->m_ref_w;
			part->value = upart->value;
			part->m_value_vis = upart->m_value_vis;
			part->m_value_xi = upart->m_value_xi;
			part->m_value_yi = upart->m_value_yi;
			part->m_value_angle = upart->m_value_angle;
			part->m_value_size = upart->m_value_size;
			part->m_value_w = upart->m_value_w;
			char * chptr = (char*)ptr + sizeof( undo_part );
			if( part->shape )
			{
				for( int ip=0; ip<part->shape->GetNumPins(); ip++ )
				{
					if( *chptr != 0 )
					{
						CString net_name = chptr;
						cnet * net = pl->m_nlist->GetNetPtrByName( &net_name );
						if( net == NULL )
							net = pl->m_nlist->AddNet( net_name, 0, 0, 0 );
						part->pin[ip].net = net;
						if( net )
						{
							int ni = pl->m_nlist->GetNetPinIndex( net, &ref_des, &part->shape->m_padstack[ip].name );
							if( ni == -1 )
								pl->m_nlist->AddNetPin( net, &ref_des, &part->shape->m_padstack[ip].name, FALSE );
						}
					}
					else
						part->pin[ip].net = NULL;
					chptr += MAX_NET_NAME_SIZE + 1;
				}
			}
			pl->m_dlist = old_dlist;	// turn drawing back on;
			pl->DrawPart( part );
			//
		}
		else if( part && type == UNDO_PART_MODIFY )
		{
			// part was moved or modified
			pl->UndrawPart( part );
			pl->m_dlist = NULL;		// prevent further drawing
			if( upart->shape != part->shape )
			{
				// footprint was changed
				pl->PartFootprintChanged( part, upart->shape );
				pl->m_nlist->PartFootprintChanged( part );
			}
			if( upart->x != part->x
				|| upart->y != part->y 
				|| upart->angle != part->angle 
				|| upart->side != part->side )
			{
				pl->Move( part, upart->x, upart->y, upart->angle, upart->side );
				pl->m_nlist->PartMoved( part , TRUE );
			}
			part->m_merge = upart->merge_name;
			part->glued = upart->glued; 
			part->m_ref_vis = upart->m_ref_vis;
			part->m_ref_xi = upart->m_ref_xi;
			part->m_ref_yi = upart->m_ref_yi;
			part->m_ref_angle = upart->m_ref_angle;
			part->m_ref_size = upart->m_ref_size;
			part->m_ref_w = upart->m_ref_w;
			part->value = upart->value;
			part->m_value_vis = upart->m_value_vis;
			part->m_value_xi = upart->m_value_xi;
			part->m_value_yi = upart->m_value_yi;
			part->m_value_angle = upart->m_value_angle;
			part->m_value_size = upart->m_value_size;
			part->m_value_w = upart->m_value_w;
			char * chptr = (char*)ptr + sizeof( undo_part );
			if( part->shape )
			{
				for( int ip=0; ip<part->shape->GetNumPins(); ip++ )
				{
					if( *chptr != 0 )
					{
						CString net_name = chptr;
						cnet * net = pl->m_nlist->GetNetPtrByName( &net_name );
						part->pin[ip].net = net;
					}
					else
						part->pin[ip].net = NULL;
					chptr += MAX_NET_NAME_SIZE + 1;
				}
			}
			// if part was renamed
			if( new_ref_des != old_ref_des )
			{
				int d = pl->m_nlist->PartRefChanged( &new_ref_des, &old_ref_des );
				if( d == 0 )
					part->ref_des = old_ref_des;
				else
					ASSERT(0);
			}
			pl->m_dlist = old_dlist;	// turn drawing back on
			pl->DrawPart( part );
		}
	}
	free(ptr);	// dele			}he undo record
}

// checks to see if a pin is connected to a trace or a copper area on a
// particular layer
//
// returns: ON_NET | TRACE_CONNECT | AREA_CONNECT
// where:
//		ON_NET = 1 if pin is on a net
//		TRACE_CONNECT = 2 if pin connects to a trace
//		AREA_CONNECT = 4 if pin connects to copper area
//
int CPartList::GetPinConnectionStatus( cpart * part, int pin_index, int layer )
{
	if( !part->shape )
		return NOT_CONNECTED;
	pad * Pad = &part->shape->m_padstack[pin_index].inner;
	if( part->side )
	{
		if( layer == LAY_TOP_COPPER )
			Pad = &part->shape->m_padstack[pin_index].bottom;
		else if( layer == LAY_BOTTOM_COPPER )
			Pad = &part->shape->m_padstack[pin_index].top;
	}
	else
	{
		if( layer == LAY_TOP_COPPER )
			Pad = &part->shape->m_padstack[pin_index].top;
		else if( layer == LAY_BOTTOM_COPPER )
			Pad = &part->shape->m_padstack[pin_index].bottom;
	}
	int status = ON_NET;
	cnet * net = part->pin[pin_index].net;
	int xp = part->pin[pin_index].x;
	int yp = part->pin[pin_index].y;
	if( !net )
		return NOT_CONNECTED;
	CString pin_name = part->shape->GetPinNameByIndex( pin_index );
	// now check for traces
	for( int ic=0; ic<net->nconnects; ic++ )
	{
		if( net->connect[ic].nsegs < 1 )
			continue;
		int p1 = net->connect[ic].start_pin;
		int p2 = net->connect[ic].end_pin;
		if( net->pin[p1].part == part &&
			net->pin[p1].pin_name.Compare(pin_name) == 0 &&
			net->connect[ic].seg[0].layer == layer )
		{
			if( abs( net->connect[ic].vtx[0].x - xp ) < _2540  &&
				abs( net->connect[ic].vtx[0].y - yp ) < _2540 ) 
			{
				// first segment connects to pin on this layer
				status |= TRACE_CONNECT;
				break;
			}
		}
		else if( p2 == cconnect::NO_END )
		{
			// stub trace, ignore end pin
		}
		else if( net->pin[p2].part == part &&
			net->pin[p2].pin_name.Compare(pin_name) == 0 &&
			net->connect[ic].seg[net->connect[ic].nsegs-1].layer == layer )
		{
			if( abs( net->connect[ic].vtx[net->connect[ic].nsegs].x - xp ) < _2540  &&
				abs( net->connect[ic].vtx[net->connect[ic].nsegs].y - yp ) < _2540 ) 
			{
				// last segment connects to pin on this layer
				status |= TRACE_CONNECT;
				break;
			}
		}
	}
	// now check for connection to copper area
	for( int ia=0; ia<net->nareas; ia++ )
	{
		carea * a = &net->area[ia];
		if ( a->poly->GetLayer() != layer )
			continue;
		for( int ip=0; ip<a->npins; ip++ )
		{
			cpin * pin = &net->pin[a->pin[ip]];
			if( pin->part == part && pin->pin_name.Compare(pin_name) == 0 )
			{
				if( a->poly->TestPointInside( xp, yp ))
				{
					if( Pad->connect_flag == PAD_CONNECT_NEVER )
						continue;
					if( !part->shape->m_padstack[pin_index].hole_size )
					{
						if( Pad->connect_flag == PAD_CONNECT_DEFAULT && !m_nlist->m_bSMT_connect )
							continue;	// pad uses project option not to connect SMT pads
						if( Pad->shape == PAD_NONE )
							continue;	// no SMT pad defined (this should not happen)
					}
					status |= AREA_CONNECT;
					break;
				}
			}
		}
	}
	return status;
}

// Function to provide info to draw pad in Gerber file (also used by DRC routines)
// On return: 
//	if no footprint for part, or no pad and no hole on this layer, returns 0
//	else returns 1 with:
//		*type = pad shape
//		*x = pin x, 
//		*y = pin y, 
//		*w = pad width, 
//		*l = pad length, 
//		*r = pad corner radius, 
//		*hole = pin hole diameter, 
//		*angle = pad angle, 
//		**net = pin net, 
//		*connection_status = ON_NET | TRACE_CONNECT | AREA_CONNECT, where:
//			ON_NET = 1 if pin is on a net
//			TRACE_CONNECT = 2 if pin connects to a trace on this layer
//			AREA_CONNECT = 4 if pin connects to copper area on this layer
//		*pad_connect_flag = 
//			PAD_CONNECT_DEFAULT if pad uses default from project
//			PAD_CONNECT_NEVER if pad never connects to copper area
//			PAD_CONNECT_THERMAL if pad connects to copper area with thermal
//			PAD_CONNECT_NOTHERMAL if pad connects to copper area without thermal
//		*clearance_type = 
//			CLEAR_NORMAL if clearance from copper area required
//			CLEAR_THERMAL if thermal connection to copper area
//			CLEAR_NONE if no clearance from copper area
// For copper layers:
//	if no pad, uses annular ring if connected
//	Uses GetPinConnectionStatus() to determine connections, this uses the area
//	connection info from the netlist
//
int CPartList::GetPadDrawInfo( cpart * part, int ipin, int layer, 
							  BOOL bUse_TH_thermals, BOOL bUse_SMT_thermals,
							  int solder_mask_swell, int paste_mask_shrink,
							  int * type, int * x, int * y, int * w, int * l, int * r, int * hole,
							  int * angle, cnet ** net, 
							  int * connection_status, int * pad_connect_flag, 
							  int * clearance_type )
{	
	// get footprint
	CShape * s = part->shape;
	if( !s || !part->drawn )
		return 0;
	
	// get pin and padstack info
	padstack * ps = &s->m_padstack[ipin];
	BOOL bUseDefault = FALSE; // if TRUE, use copper pad for mask
	int connect_status = GetPinConnectionStatus( part, ipin, layer );

	// set default return values for no pad and no hole
	int ret_code = 0;
	int ttype = PAD_NONE;
	int xx = part->pin[ipin].x;
	int yy = part->pin[ipin].y;
	int ww = 0;
	int ll = 0;
	int rr = 0;
	int aangle = part->angle + s->m_padstack[ipin].angle;
	if( part->side )
		aangle = part->angle - s->m_padstack[ipin].angle;
	if( aangle >= 360 )
		aangle -= 360;
	else if( aangle < 0 )
		aangle += 360;
	int hole_size = s->m_padstack[ipin].hole_size;
	cnet * nnet = part->pin[ipin].net;
	int clear_type = CLEAR_NORMAL;	
	int connect_flag = PAD_CONNECT_DEFAULT;

	// get pad info
	pad * p = NULL;
	if( (layer == LAY_TOP_COPPER && part->side == 0 )
		|| (layer == LAY_BOTTOM_COPPER && part->side == 1 ) ) 
	{
		// top copper pad is on this layer 
		p = &ps->top;
	}
	else if( ((layer == LAY_MASK_TOP || layer == LAY_SM_TOP) && part->side == 0 )
		|| ((layer == LAY_MASK_BOTTOM || layer == LAY_SM_BOTTOM) && part->side == 1 ) ) 
	{
		// top mask pad is on this layer 
		if( ps->top_mask.shape != PAD_DEFAULT )
			p = &ps->top_mask;
		else
		{
			bUseDefault = TRUE;		// get mask pad from copper pad
			p = &ps->top;
		}
	}
	else if( (layer == LAY_PASTE_TOP && part->side == 0 )
		|| (layer == LAY_PASTE_BOTTOM && part->side == 1 ) ) 
	{
		// top paste pad is on this layer 
		if( ps->top_paste.shape != PAD_DEFAULT )
			p = &ps->top_paste;
		else
		{
			bUseDefault = TRUE;
			p = &ps->top;
		}
	}
	else if( (layer == LAY_TOP_COPPER && part->side == 1 )
			|| (layer == LAY_BOTTOM_COPPER && part->side == 0 ) ) 
	{
		// bottom copper pad is on this layer
		p = &ps->bottom;
	}
	else if(( (layer == LAY_MASK_TOP || layer == LAY_SM_TOP) && part->side == 1 )
		|| ((layer == LAY_MASK_BOTTOM || layer == LAY_SM_BOTTOM) && part->side == 0 ) ) 
	{
		// bottom mask pad is on this layer 
		if( ps->bottom_mask.shape != PAD_DEFAULT )
			p = &ps->bottom_mask;
		else
		{
			bUseDefault = TRUE;
			p = &ps->bottom;
		}
	}
	else if( (layer == LAY_PASTE_TOP && part->side == 1 )
		|| (layer == LAY_PASTE_BOTTOM && part->side == 0 ) ) 
	{
		// bottom paste pad is on this layer 
		if( ps->bottom_paste.shape != PAD_DEFAULT )
			p = &ps->bottom_paste;
		else
		{
			bUseDefault = TRUE;
			p = &ps->bottom;
		}
	}
	else if( layer > LAY_BOTTOM_COPPER && ps->hole_size > 0 )
	{
		// inner pad is on this layer
		p = &ps->inner;
	}

	// now set parameters for return
	if( p )
		connect_flag = p->connect_flag;
	if( p == NULL )
	{
		// no pad definition, return defaults
	}
	else if( p->shape == PAD_NONE && ps->hole_size == 0 )
	{
		// no hole, no pad, return defaults
	}
	else if( p->shape == PAD_NONE )
	{
		// hole, no pad
		ret_code = 1;
		if( connect_status > ON_NET )
		{
			// connected to copper area or trace
			// make annular ring
			ret_code = 1;
			ttype = PAD_ROUND;
			ww = 2*m_annular_ring + hole_size;
		}
		else if( ( layer == LAY_MASK_TOP || layer == LAY_SM_TOP || layer == LAY_MASK_BOTTOM || layer == LAY_SM_BOTTOM ) && bUseDefault )
		{
			// if solder mask layer and no mask pad defined, treat hole as pad to get clearance
			ret_code = 1;
			ttype = PAD_ROUND;
			ww = hole_size;
		}
	}
	else if( p->shape != PAD_NONE )
	{
		// normal pad
		ret_code = 1;
		ttype = p->shape;
		ww = p->size_Y;
		ll = p->size_X;
		if( p->shape != PAD_SQUARE && p->shape != PAD_RECT )
			rr = p->radius;
	}
	else
		ASSERT(0);	// error

	// adjust mask and paste pads if necessary
	if( ( layer == LAY_MASK_TOP || layer == LAY_SM_TOP || layer == LAY_MASK_BOTTOM || layer == LAY_SM_BOTTOM ) && bUseDefault )
	{
		ww += 2*solder_mask_swell;
		ll += 2*solder_mask_swell;
		rr += solder_mask_swell;
	}
	else if( (layer == LAY_PASTE_TOP || layer == LAY_PASTE_BOTTOM) && bUseDefault )
	{
		if( ps->hole_size == 0 )
		{
			ww -= 2*paste_mask_shrink;
			ll -= 2*paste_mask_shrink;
			rr -= paste_mask_shrink;
			if( rr < 0 )
				rr = 0;
			if( ww <= 0 || ll <= 0 )
			{
				ww = 0;
				ll = 0;
				ret_code = 0;
			}
		}
		else
		{
			ww = ll = 0;	// no paste for through-hole pins
			ret_code = 0;
		}
	}

	// if copper layer connection, decide on thermal
	if( layer >= LAY_TOP_COPPER && (connect_status & AREA_CONNECT) )
	{
		// copper area connection, thermal or not?
		if( p->connect_flag == PAD_CONNECT_NEVER )
			ASSERT(0);	// shouldn't happen, this is an error by GetPinConnectionStatus(...)
		else if( p->connect_flag == PAD_CONNECT_NOTHERMAL )
			clear_type = CLEAR_NONE;
		else if( p->connect_flag == PAD_CONNECT_THERMAL )
			clear_type = CLEAR_THERMAL;
		else if( p->connect_flag == PAD_CONNECT_DEFAULT )
		{
			if( bUse_TH_thermals && ps->hole_size )
				clear_type = CLEAR_THERMAL;
			else if( bUse_SMT_thermals && !ps->hole_size )
				clear_type = CLEAR_THERMAL;
			else
				clear_type = CLEAR_NONE;
		}
		else
			ASSERT(0);
	}
	if( x )
		*x = xx;
	if( y )
		*y = yy;
	if( type )
		*type = ttype;
	if( w )
		*w = ww;
	if( l )
		*l = ll;
	if( r )
		*r = rr;
	if( hole )
		*hole = hole_size;
	if( angle )
		*angle = aangle;
	if( connection_status )
		*connection_status = connect_status;
	if( net )
		*net = nnet;
	if( pad_connect_flag )
		*pad_connect_flag = connect_flag;
	if( clearance_type )
		*clearance_type = clear_type;
	return ret_code;
}

// check partlist for errors
//
int CPartList::CheckPartlist( CString * logstr )
{
	int nerrors = 0;
	int nwarnings = 0;
	CString str;
	CMapStringToPtr map;
	void * ptr;

	*logstr += "***** Checking Parts *****\r\n";

	// first, check for duplicate parts
	cpart * part = m_start.next;
	while( part->next != 0 )
	{
		CString ref_des = part->ref_des;
		BOOL test = map.Lookup( ref_des, ptr );
		if( test )
		{
			str.Format( "ERROR: Part \"%s\" duplicated\r\n", ref_des );
			str += "    ###   To fix this, delete one instance of the part, then save, close and re-open project\r\n";
			*logstr += str;
			nerrors++;
		}
		else
			map.SetAt( ref_des, NULL );

		// next part
		part = part->next;
	}

	// now check all parts
	part = m_start.next;
	while( part->next != 0 )
	{
		// check this part
		str = "";
		CString * ref_des = &part->ref_des;
		if( !part->shape )
		{
			// no footprint
			str.Format( "Warning: Part \"%s\" has no footprint\r\n",
				*ref_des );
			nwarnings++;
		}
		else
		{
			for( int ip=0; ip<part->pin.GetSize(); ip++ )
			{
				// check this pin
				cnet * net = part->pin[ip].net;
				CString * pin_name = &part->shape->m_padstack[ip].name;
				if( !net )
				{
					// part->pin->net is NULL, pin unconnected
					// this is not an error
					//				str.Format( "%s.%s unconnected\r\n",
					//					*ref_des, *pin_name );
				}
				else
				{
					cnet * netlist_net = m_nlist->GetNetPtrByName( &net->name );
					if( !netlist_net )
					{
						// part->pin->net->name doesn't exist in netlist
						str.Format( "ERROR: Part \"%s\" pin \"%s\" connected to net \"%s\" which doesn't exist in netlist\r\n",
							*ref_des, *pin_name, net->name );
						nerrors++;
					}
					else
					{
						if( net != netlist_net )
						{
							// part->pin->net doesn't match netlist->net
							str.Format( "ERROR: Part \"%s\" pin \"%s\" connected to net \"%s\" which doesn't match netlist\r\n",
								*ref_des, *pin_name, net->name );
							nerrors++;
						}
						else
						{
							// try to find pin in pin list for net
							int net_pin = -1;
							for( int ip=0; ip<net->npins; ip++ )
							{
								if( net->pin[ip].part == part )
								{
									if( net->pin[ip].pin_name == *pin_name )
									{
										net_pin = ip;
										break;
									}
								}
							}
							if( net_pin == -1 )
							{
								// pin not found
								str.Format( "ERROR: Part \"%s\" pin \"%s\" connected to net \"%\" but pin not in net\r\n",
									*ref_des, *pin_name, net->name );
								nerrors++;
							}
							else
							{
								// OK
							}

						}
					}
				}
			}
		}
		*logstr += str;

		// next part
		part = part->next;
	}
	str.Format( "***** %d ERROR(S), %d WARNING(S) *****\r\n", nerrors, nwarnings );
	*logstr += str;

	return nerrors;
}

void CPartList::MoveOrigin( int x_off, int y_off )
{
	cpart * part = GetFirstPart();
	while( part )
	{
		if( part->shape )
		{
			// move this part
			UndrawPart( part );
			part->x += x_off;
			part->y += y_off;
			for( int ip=0; ip<part->pin.GetSize(); ip++ )
			{
				part->pin[ip].x += x_off;
				part->pin[ip].y += y_off;
			}
			DrawPart( part );
		}
		part = GetNextPart(part);
	}
}

BOOL CPartList::CheckForProblemFootprints()
{
	BOOL bHeaders_28mil_holes = FALSE;   
	cpart * part = GetFirstPart();
	while( part )
	{
		if( part->shape)
		{
			if( part->shape->m_name.Right(7) == "HDR-100" 
				&& part->shape->m_padstack[0].hole_size == 28*NM_PER_MIL )
			{
				bHeaders_28mil_holes = TRUE;
			}
		}
		part = GetNextPart( part );
	}
	if( g_bShow_header_28mil_hole_warning && bHeaders_28mil_holes )   
	{
		CDlgMyMessageBox dlg;
		dlg.Initialize( "WARNING: You are loading footprint(s) for through-hole headers with 100 mil pin spacing and 28 mil holes.\n\nThese may be from an obsolete version of the library \"th_header.fpl\" with holes that are too small for standard parts. Please check your design." );
		dlg.DoModal();
		g_bShow_header_28mil_hole_warning = !dlg.bDontShowBoxState;
	}
	return bHeaders_28mil_holes;
}




cpart * CPartList::RefAutoLocation( cpart * p, int step, BOOL _90, RECT idsRect, CArray<CPolyLine> * bo, CTextList * tl, BOOL INCLUDE_HOLE, int Clearance  )
{
	if( p->m_ref_size == 0 )
		return NULL;
	if( (p->m_ref_angle%90 && p->angle%90 == 0) || (p->m_ref_angle%90 == 0 && p->angle%90) )
		return p;
	cpart * pbad = 0;
	BOOL intersection;
	RECT pRect, pRef, pValue;
	CPoint pPoint, pRefPoint, pValuePoint, ppPoint, ppRefPoint, ppValuePoint;
	int xpos, ypos, xref, yref, ValueVis, a;
	BOOL dir=FALSE, REPEAT=FALSE;
	// set ref angle
	if (p->side)
		a = p->angle;
	else
		a = -p->angle;
	if (_90)
		a -= 90;
	
	// set visible
	p->m_ref_vis = 1;
	MoveRefText(p, p->m_ref_xi, p->m_ref_yi, a, p->m_ref_size, p->m_ref_w );
	// get part pos
	xpos = p->x;
	ypos = p->y;
	if (!GetPartBoundingRect(p, &pRect))
		return 0;
	if (!GetRefBoundingRect(p, &pRef))
		return 0;
	if( Clearance )
		SwellRect( &pRef, Clearance );
	ValueVis = GetValueBoundingRect(p, &pValue);
	// 
	const int MAX_ITERATORS = 999;
	BOOL rotation_was_made = FALSE;
	int RefW = pRef.right - pRef.left;
	int RefH = pRef.top - pRef.bottom;
	int PartW = (pRect.right - pRect.left);
	int PartH = (pRect.top - pRect.bottom);
	int MIDX = (pRect.left + pRect.right)/2;
	int MIDY = (pRect.top + pRect.bottom)/2;
	int XSTART=MIDX, YSTART=MIDY;
	BOOL bbr=0;
	for ( int it=0; it<MAX_ITERATORS; it++ )
	{
		switch( it ) // the best positions
		{
		case 0://top
			dir = FALSE;	
			REPEAT = FALSE; 
			yref = pRect.top + p->m_ref_w/2 + _2540 + Clearance;
			if ( (p->side && _90) || (!p->side && !_90) )
				xref = MIDX - (RefW-2*Clearance)/2;
			else
				xref = MIDX + (RefW-2*Clearance)/2;
			break;
		case 1 ://right
			dir = TRUE;
			REPEAT = TRUE;
			yref = MIDY - (RefH-2*Clearance)/2;
			if ( (p->side && _90) || (!p->side && !_90) )
				xref = pRect.right + p->m_ref_w/2 + _2540 + Clearance;
			else
				xref = pRect.right + RefW - p->m_ref_w/2 + _2540 - Clearance;
			break;
		case 2 ://left
			dir = TRUE; 
			REPEAT = FALSE;
			yref = MIDY - (RefH-2*Clearance)/2;
			if ( (p->side && _90) || (!p->side && !_90) )
				xref = pRect.left - RefW + p->m_ref_w/2 - _2540 + Clearance;
			else
				xref = pRect.left - p->m_ref_w/2 - _2540 - Clearance;
			break;
		case 3 ://bottom
			dir = FALSE;
			REPEAT = TRUE;
			yref = pRect.bottom - RefH + p->m_ref_w/2 - _2540 + Clearance;
			if ( (p->side && _90) || (!p->side && !_90) )
				xref = MIDX - (RefW-2*Clearance)/2;
			else
				xref = MIDX + (RefW-2*Clearance)/2;
			break;
		case 4:
			if( _90 == 0 )
			{
				// set ref angle
				a -= 90;
				_90 = TRUE;
				it = -1;
				int buf = RefW;
				RefW = RefH;
				RefH = buf;
				rotation_was_made = TRUE;
				continue;
			}
			else if( rotation_was_made )
			{
				_90 = FALSE;
				a += 90;
				int buf = RefW;
				RefW = RefH;
				RefH = buf;
			}
		default:  // next position
			if( it == 4 )
			{
				// step1 (top)
				dir = FALSE;
				REPEAT = FALSE;
				MoveRefText(p, xref, yref, a, p->m_ref_size, p->m_ref_w );
				if (!GetRefBoundingRect(p, &pRef))
					return 0;
				if( Clearance )
					SwellRect( &pRef, Clearance );
				yref = pRect.top + p->m_ref_w/2 + _2540 + Clearance;
				if ( (p->side && _90) || (!p->side && !_90) )
					xref = MIDX - (RefW-2*Clearance)/2;
				else
					xref = MIDX + (RefW-2*Clearance)/2;
				XSTART = xref;
				YSTART = yref;
			}
			if (!dir) // Horiz
			{
				if( xref > XSTART )
					xref += step;
				else
					xref -= step;
				xref = 2*XSTART - xref; // mirror xref
				MoveRefText(p, xref, yref, a, p->m_ref_size, p->m_ref_w );
				if (!GetRefBoundingRect(p, &pRef))
					return 0;
				if( Clearance )
					SwellRect( &pRef, Clearance );
				if ( abs(XSTART-xref) > PartW/2 ) // out of limit
				{
					if (!REPEAT)
					{
						REPEAT = TRUE; // step2 (bottom)
						// set pos bottom
						yref = pRect.bottom - RefH + p->m_ref_w/2 - _2540 + Clearance;
						if ( (p->side && _90) || (!p->side && !_90) )
							xref = MIDX - (RefW-2*Clearance)/2;
						else
							xref = MIDX + (RefW-2*Clearance)/2;
					}
					else
					{
						// set pos left
						dir = TRUE; // Vertical
						REPEAT = FALSE; // step3 (left)
						yref = MIDY - (RefH-2*Clearance)/2;
						if ( (p->side && _90) || (!p->side && !_90) )
							xref = pRect.left - RefW + p->m_ref_w/2 - _2540 + Clearance;
						else
							xref = pRect.left - p->m_ref_w/2 - _2540 - Clearance;
					}
					XSTART = xref;
					YSTART = yref;
					continue;
				}
			}
			else
			{
				if( yref > YSTART )
					yref += step;
				else
					yref -= step;
				yref = 2*YSTART - yref; // mirror yref
				MoveRefText(p, xref, yref, a, p->m_ref_size, p->m_ref_w );
				if (!GetRefBoundingRect(p, &pRef))
					return 0;
				if( Clearance )
					SwellRect( &pRef, Clearance );
				if ( abs(YSTART-yref) > PartH/2 )
				{
					if (!REPEAT)
					{
						REPEAT = TRUE; // step4 (right)
						// set pos right
						yref = MIDY - (RefH-2*Clearance)/2;
						if ( (p->side && _90) || (!p->side && !_90) )
							xref = pRect.right + p->m_ref_w/2 + _2540 + Clearance;
						else
							xref = pRect.right + RefW - p->m_ref_w/2 + _2540 - Clearance;
					}
					else
					{
						// break, no position(
						bbr = 1;
						break;
					}
					XSTART = xref;
					YSTART = yref;
					continue;
				}
			}
		}
		if (bbr)
			break;
		if( it < 4 )
		{
			MoveRefText(p, xref, yref, a, p->m_ref_size, p->m_ref_w );
			if (!GetRefBoundingRect(p, &pRef))
				return 0;
			if( Clearance )
				SwellRect( &pRef, Clearance );
		}
		int debug = 0;
		if( debug )
		{
			DrawPart(p);
			CFrameWnd * pMainWnd = (CFrameWnd*)AfxGetMainWnd();
			CView * m_view = pMainWnd->GetActiveView();
			m_view->Invalidate(FALSE);
			HWND wnd = AfxGetMainWnd()->GetSafeHwnd();
			UpdateWindow(wnd);
			Sleep(100);
		}
		if( p->angle%90 )
		{
			CPoint pts[4];
			int npts = 4;
			if( p->dl_sel )
				m_dlist->Get_Points( p->dl_sel, pts, &npts );
			if( npts == 3 )
			{
				BOOL bTest1 = TestPolygon( pRef.left, pRef.bottom, pts, npts );
				BOOL bTest2 = TestPolygon( pRef.right, pRef.bottom, pts, npts );
				BOOL bTest3 = TestPolygon( pRef.left, pRef.top, pts, npts );
				BOOL bTest4 = TestPolygon( pRef.right, pRef.top, pts, npts );
				BOOL bTest = FALSE, T2;
				if( bTest1 || bTest2 || bTest3 || bTest4 ||
					(InRange( pts[0].x, pRef.left, pRef.right ) && InRange( pts[0].y, pRef.bottom, pRef.top )) ||
					(InRange( pts[1].x, pRef.left, pRef.right ) && InRange( pts[1].y, pRef.bottom, pRef.top )) ||
					(InRange( pts[2].x, pRef.left, pRef.right ) && InRange( pts[2].y, pRef.bottom, pRef.top )) ||
					(InRange( pts[3].x, pRef.left, pRef.right ) && InRange( pts[3].y, pRef.bottom, pRef.top )))
					bTest = TRUE;
				T2 = bTest;
				while( T2 == bTest )
				{	
					if( dir == 0 )
					{
						if( ( !REPEAT && !bTest ) ||
							( REPEAT && bTest ) )
							yref -= step; // go to bottom
						else 
							yref += step; // go to top
					}
					else
					{
						if( ( !REPEAT && !bTest ) ||
							( REPEAT && bTest ) )
							xref += step; // go to right
						else 
							xref -= step; // go to left
					}
					MoveRefText(p, xref, yref, a, p->m_ref_size, p->m_ref_w );
					if (!GetRefBoundingRect(p, &pRef))
						return 0;
					if( Clearance )
						SwellRect( &pRef, Clearance );
					//
					if( TestPolygon( pRef.left, pRef.bottom, pts, npts ) ||
						TestPolygon( pRef.right, pRef.bottom, pts, npts ) ||
						TestPolygon( pRef.left, pRef.top, pts, npts ) ||
						TestPolygon( pRef.right, pRef.top, pts, npts ) ||
						(InRange( pts[0].x, pRef.left, pRef.right ) && InRange( pts[0].y, pRef.bottom, pRef.top )) ||
						(InRange( pts[1].x, pRef.left, pRef.right ) && InRange( pts[1].y, pRef.bottom, pRef.top )) ||
						(InRange( pts[2].x, pRef.left, pRef.right ) && InRange( pts[2].y, pRef.bottom, pRef.top )) ||
						(InRange( pts[3].x, pRef.left, pRef.right ) && InRange( pts[3].y, pRef.bottom, pRef.top )) )
						T2 = TRUE;
					else
						T2 = FALSE;
					if( RectsIntersection( pRect, pRef ) == -1 )
						break;
				}
				if( T2 )
				{
					if( dir == 0 )
					{
						if( ( !REPEAT && !bTest ) ||
							( REPEAT && bTest ) )
							yref += step; // back
						else 
							yref -= step; // back
					}
					else
					{
						if( ( !REPEAT && !bTest ) ||
							( REPEAT && bTest ) )
							xref -= step; // back
						else 
							xref += step; // back
					}
				}
				MoveRefText(p, xref, yref, a, p->m_ref_size, p->m_ref_w );
				if (!GetRefBoundingRect(p, &pRef))
					return 0;
				if( Clearance )
					SwellRect( &pRef, Clearance );
			}
		}
		// VALUE INTERSECT
		if (ValueVis)
			if (RectsIntersection(pRef, pValue) >= 0)
			{
				if( (p->m_value_angle%90 && p->angle%90) || (p->m_value_angle%90 == 0 && p->angle%90 == 0) )
				{
					pbad = p;
					continue;
				}
				CPoint pts[4];
				int npts = 4;
				if( p->dl_value_sel )
					m_dlist->Get_Points( p->dl_value_sel, pts, &npts );
				if( npts )
					if( TestPolygon( pRef.left, pRef.top, pts, npts ) ||
						TestPolygon( pRef.left, pRef.bottom, pts, npts ) ||
						TestPolygon( pRef.right, pRef.top, pts, npts ) ||
						TestPolygon( pRef.right, pRef.bottom, pts, npts ) ||
						TestPolygon( (pRef.left+pRef.right)/2, pRef.top, pts, npts ) ||
						TestPolygon( (pRef.left+pRef.right)/2, pRef.bottom, pts, npts ) ||
						TestPolygon( pRef.left, (pRef.top+pRef.bottom)/2, pts, npts ) ||
						TestPolygon( pRef.right, (pRef.top+pRef.bottom)/2, pts, npts ) ||
						TestPolygon( (pRef.left+pRef.right)/2, (pRef.top+pRef.bottom)/2, pts, npts ) )
					{
						pbad = p;
						continue;
					}
			}
		// PREV BAD PART...
		intersection = -1;
		if (pbad && p != pbad)
		{
			if (p->side != pbad->side )
			{
				for (int h=0; h<pbad->pin.GetSize(); h++)
				{
					if( pbad->pin[h].dl_els[0] )
					{
						if( ( getbit( pbad->pin[h].dl_els[0]->layers_bitmap, LAY_TOP_COPPER ) && !p->side ) ||
							( getbit( pbad->pin[h].dl_els[0]->layers_bitmap, LAY_BOTTOM_COPPER ) && p->side ) )
						{
							RECT Get;
							m_dlist->Get_Rect( pbad->pin[h].dl_els[0], &Get );
							intersection = max(RectsIntersection(pRef, Get),intersection);
						}
					}
					if( pbad->pin[h].dl_els[2] )
					{
						if( ( getbit( pbad->pin[h].dl_els[2]->layers_bitmap, LAY_TOP_COPPER ) && !p->side ) ||
							( getbit( pbad->pin[h].dl_els[2]->layers_bitmap, LAY_BOTTOM_COPPER ) && p->side ) )
						{
							RECT Get;
							m_dlist->Get_Rect( pbad->pin[h].dl_els[2], &Get );
							intersection = max(RectsIntersection(pRef, Get),intersection);
						}
					}
					if ( pbad->pin[h].dl_hole )
					{
						RECT Get;
						m_dlist->Get_Rect( pbad->pin[h].dl_hole, &Get );
						intersection = max(RectsIntersection(pRef, Get),intersection);
					}	
				}
				// intersection
				if (intersection >= 0)
					continue;
			}
			else
			{
				RECT badRect, badRef, badValue;
				CPoint pts[4];
				int npts = 4;
				if( pbad->dl_sel )
					m_dlist->Get_Points( pbad->dl_sel, pts, &npts );
				if( npts == 4 )
				{
					if( TestPolygon( pRef.left, pRef.top, pts, npts ) ||
						TestPolygon( pRef.left, pRef.bottom, pts, npts ) ||
						TestPolygon( pRef.right, pRef.top, pts, npts ) ||
						TestPolygon( pRef.right, pRef.bottom, pts, npts ) ||
						TestPolygon( (pRef.left+pRef.right)/2, pRef.top, pts, npts ) ||
						TestPolygon( (pRef.left+pRef.right)/2, pRef.bottom, pts, npts ) ||
						TestPolygon( pRef.left, (pRef.top+pRef.bottom)/2, pts, npts ) ||
						TestPolygon( pRef.right, (pRef.top+pRef.bottom)/2, pts, npts ) ||
						TestPolygon( (pRef.left+pRef.right)/2, (pRef.top+pRef.bottom)/2, pts, npts ) )
						continue;
					if( (InRange( pts[0].x, pRef.left, pRef.right ) && InRange( pts[0].y, pRef.bottom, pRef.top )) ||
						(InRange( pts[1].x, pRef.left, pRef.right ) && InRange( pts[1].y, pRef.bottom, pRef.top )) ||
						(InRange( pts[2].x, pRef.left, pRef.right ) && InRange( pts[2].y, pRef.bottom, pRef.top )) ||
						(InRange( pts[3].x, pRef.left, pRef.right ) && InRange( pts[3].y, pRef.bottom, pRef.top )) )
						continue;
					if (_90)
					{
						int my1 = ((pRef.top + pRef.bottom)/2 + pRef.bottom)/2;
						int my2 = ((pRef.top + pRef.bottom)/2 + pRef.top)/2;
						if( TestPolygon( pRef.left, my1, pts, npts ) ||
							TestPolygon( pRef.left, my2, pts, npts ) ||
							TestPolygon( pRef.right, my1, pts, npts ) ||
							TestPolygon( pRef.right, my2, pts, npts ))
							continue;
					}
					else
					{
						int mx1 = ((pRef.left + pRef.right)/2 + pRef.left)/2;
						int mx2 = ((pRef.left + pRef.right)/2 + pRef.right)/2;
						if( TestPolygon( mx1, pRef.top, pts, npts ) ||
							TestPolygon( mx2, pRef.top, pts, npts ) ||
							TestPolygon( mx1, pRef.bottom, pts, npts ) ||
							TestPolygon( mx2, pRef.bottom, pts, npts ))
							continue;
					}
				}
				else
				{
					if (GetPartBoundingRect(pbad, &badRect))
						if (RectsIntersection(pRef, badRect) >= 0)
							continue;
				}
				if (GetRefBoundingRect(pbad, &badRef))
					if (RectsIntersection(pRef, badRef) >= 0)
						continue;
				if (GetValueBoundingRect(pbad, &badValue))
					if (RectsIntersection(pRef, badValue) >= 0)
						continue;
			}
		}
		// SCAN PARTS...
		for (cpart * pp=GetFirstPart(); pp; pp=GetNextPart(pp))
		{
			if( pp->utility == 2 || pp == pbad )
			{
				continue;
			}
			//
			if( pp != p )
			{
				RECT ppTotal, ppRect;
				if (p->side != pp->side )
				{
					for (int h=0; h<pp->pin.GetSize(); h++)
					{
						if( pp->pin[h].dl_els[0] )
						{
							if( ( getbit( pp->pin[h].dl_els[0]->layers_bitmap, LAY_TOP_COPPER ) && !p->side ) ||
								( getbit( pp->pin[h].dl_els[0]->layers_bitmap, LAY_BOTTOM_COPPER ) && p->side ) )
							{
								RECT Get;
								m_dlist->Get_Rect( pp->pin[h].dl_els[0], &Get );
								intersection = max(RectsIntersection(pRef, Get),intersection);
							}
						}
						if( pp->pin[h].dl_els[2] )
						{
							if( ( getbit( pp->pin[h].dl_els[2]->layers_bitmap, LAY_TOP_COPPER ) && !p->side ) ||
								( getbit( pp->pin[h].dl_els[2]->layers_bitmap, LAY_BOTTOM_COPPER ) && p->side ) )
							{
								RECT Get;
								m_dlist->Get_Rect( pp->pin[h].dl_els[2], &Get );
								intersection = max(RectsIntersection(pRef, Get),intersection);
							}
						}
						if ( pp->pin[h].dl_hole )
						{
							RECT Get;
							m_dlist->Get_Rect( pp->pin[h].dl_hole, &Get );
							intersection = max(RectsIntersection(pRef, Get),intersection);
						}
						// intersection
						if (intersection >= 0)
						{
							pbad = pp;
							break;
						}
					}
					if (intersection >= 0)
						break;
					else 
						continue;
				}
				CPoint pts[4];
				int npts = 4;
				if( pp->dl_sel )
					m_dlist->Get_Points( pp->dl_sel, pts, &npts );
				if( npts == 4 )
				{
					if( TestPolygon( pRef.left, pRef.top, pts, npts ) ||
						TestPolygon( pRef.left, pRef.bottom, pts, npts ) ||
						TestPolygon( pRef.right, pRef.top, pts, npts ) ||
						TestPolygon( pRef.right, pRef.bottom, pts, npts ) ||
						TestPolygon( (pRef.left+pRef.right)/2, pRef.top, pts, npts ) ||
						TestPolygon( (pRef.left+pRef.right)/2, pRef.bottom, pts, npts ) ||
						TestPolygon( pRef.left, (pRef.top+pRef.bottom)/2, pts, npts ) ||
						TestPolygon( pRef.right, (pRef.top+pRef.bottom)/2, pts, npts ) ||
						TestPolygon( (pRef.left+pRef.right)/2, (pRef.top+pRef.bottom)/2, pts, npts ) )
						intersection = max(0,intersection);
					if( (InRange( pts[0].x, pRef.left, pRef.right ) && InRange( pts[0].y, pRef.bottom, pRef.top )) ||
						(InRange( pts[1].x, pRef.left, pRef.right ) && InRange( pts[1].y, pRef.bottom, pRef.top )) ||
						(InRange( pts[2].x, pRef.left, pRef.right ) && InRange( pts[2].y, pRef.bottom, pRef.top )) ||
						(InRange( pts[3].x, pRef.left, pRef.right ) && InRange( pts[3].y, pRef.bottom, pRef.top )) )
						intersection = max(0,intersection);
					if (_90)
					{
						int my1 = ((pRef.top + pRef.bottom)/2 + pRef.bottom)/2;
						int my2 = ((pRef.top + pRef.bottom)/2 + pRef.top)/2;
						if( TestPolygon( pRef.left, my1, pts, npts ) ||
							TestPolygon( pRef.left, my2, pts, npts ) ||
							TestPolygon( pRef.right, my1, pts, npts ) ||
							TestPolygon( pRef.right, my2, pts, npts ))
							intersection = max(0,intersection);
					}
					else
					{
						int mx1 = ((pRef.left + pRef.right)/2 + pRef.left)/2;
						int mx2 = ((pRef.left + pRef.right)/2 + pRef.right)/2;
						if( TestPolygon( mx1, pRef.top, pts, npts ) ||
							TestPolygon( mx2, pRef.top, pts, npts ) ||
							TestPolygon( mx1, pRef.bottom, pts, npts ) ||
							TestPolygon( mx2, pRef.bottom, pts, npts ))
							intersection = max(0,intersection);
					}
				}
				if (GetPartBoundingRect(pp, &ppRect))
				{
					RECT ppRef, ppValue;
					if (pp->utility == 0)
					{
						ppTotal.left = ppRect.left;
						ppTotal.right = ppRect.right;
						ppTotal.top = ppRect.top;
						ppTotal.bottom = ppRect.bottom;
					}
					if (GetRefBoundingRect(pp, &ppRef))
					{
						intersection = max(RectsIntersection(pRef, ppRef),intersection);
						if (pp->utility == 0)
							SwellRect (&ppTotal,ppRef);
					}
					if (GetValueBoundingRect(pp, &ppValue))
					{
						intersection = max(RectsIntersection(pRef, ppValue),intersection);
						if (pp->utility == 0)
							SwellRect (&ppTotal,ppValue);
					}
				}
				else 
					continue;
				if (intersection >= 0)
				{
					pbad = pp;
					break;
				}
				else if (pp->utility == 0)
				{
					if (RectsIntersection(idsRect, ppTotal) == -1)
						pp->utility = 2;
					else
						pp->utility = 1;
				}
			}
		}
		if( intersection >= 0 )
			continue;
		//
		pbad = p; // no parts intersect
		// VIA HOLES...
		if (intersection == -1 && INCLUDE_HOLE)
		{
			for( cnet * net=m_nlist->GetFirstNet(); net; net=m_nlist->GetNextNet() )
			{
				for( int gic=0; gic<net->nconnects; gic++ )
				{
					for( int giv=0; giv<=net->connect[gic].nsegs; giv++ )
					{
						int whole = net->connect[gic].vtx[giv].via_hole_w;
						if( whole )
						{
							int hx = net->connect[gic].vtx[giv].x;
							int hy = net->connect[gic].vtx[giv].y;
							RECT hr = rect( hx-whole/2, hy-whole/2, hx+whole/2, hy+whole/2 );
							intersection = max(RectsIntersection(pRef, hr),intersection);
							if( intersection >= 0 )
								break; // no, via hole rect intersection
						}
					}
					if( intersection >= 0 )
						break; // no, via hole rect intersection
				}
				if( intersection >= 0 )
				{
					m_nlist->CancelNextNet();
					break; // no, via hole rect intersection
				}
			}
		}
		// SILK LINES...
		if (intersection == -1)
		{
			for( int i=0; i<bo->GetSize(); i++ )
			{
				CPolyLine * pb = &(*bo)[i];
				if( pb )
				{
					RECT por = pb->GetBounds();
					if( RectsIntersection( por, pRef ) < 0 )
						continue;
					if( pb->GetLayer() != LAY_SILK_TOP+p->side )
						continue;
					int cl = pb->GetClosed();
					int MAXC = pb->GetNumCorners();
					if( cl == 0 )
						MAXC--;
					for( int ii=0; ii<MAXC; ii++ )
					{
						int gx = pb->GetX(ii);
						int gy = pb->GetY(ii);
						int nx = pb->GetX(pb->GetIndexCornerNext(ii));
						int ny = pb->GetY(pb->GetIndexCornerNext(ii));
						int ss = pb->GetSideStyle(ii);
						if( FindSegmentIntersections(	gx, gy, nx, ny, ss,
														pRef.left, pRef.top, pRef.right, pRef.top, 0 ) == 0 )
							if( FindSegmentIntersections(	gx, gy, nx, ny, ss,
															pRef.left, pRef.top, pRef.left, pRef.bottom, 0 ) == 0 )
								if( FindSegmentIntersections(	gx, gy, nx, ny, ss,
																pRef.left, pRef.bottom, pRef.right, pRef.bottom, 0 ) == 0 )
									if( FindSegmentIntersections(	gx, gy, nx, ny, ss,
																	pRef.right, pRef.bottom, pRef.right, pRef.top, 0 ) == 0 )
										continue;
						//
						intersection = max(0,intersection);
						break;
					}
				}
				if (intersection != -1)
					break;
			}
		}
		// TEXTS...
		if (intersection == -1)
		{
			int it = 0;
			for( CText * t=tl->GetFirstText(); t; t=tl->GetNextText(&it) )
			{
				if( t->m_layer != LAY_SILK_TOP+p->side )
					continue;
				CPoint pts[4];
				int npts = 4;
				BOOL bf = 0;
				if( t->dl_sel )
				{
					if( t->dl_sel->dlist )
					{
						RECT GetR;
						m_dlist->Get_Rect( t->dl_sel, &GetR );
						if( RectsIntersection(pRef, GetR) >= 0 )
						{
							if( t->m_angle%90 == 0 )
							{
								intersection = max(0,intersection);
								break;
							}
							bf = 1;
						}
					}
				}	
				if( t->m_angle%90 && bf )
					if( t->dl_sel )
					{
						if( t->dl_sel->dlist )
						{
							CPoint Pt[4];
							int np = 4;
							m_dlist->Get_Points( t->dl_sel, Pt, &np );
							//BOOL bf=0;
							for( int iv=0; iv<4; iv++ )
							{
								int ivn = iv+1;
								if( ivn > 3 )
									ivn = 0;
								if( FindSegmentIntersections(	pRef.left, pRef.bottom, pRef.right, pRef.bottom, 0,
																Pt[iv].x, Pt[iv].y, Pt[ivn].x, Pt[ivn].y, 0 ) == 0 )
									if( FindSegmentIntersections(	pRef.left, pRef.bottom, pRef.left, pRef.top, 0,
																	Pt[iv].x, Pt[iv].y, Pt[ivn].x, Pt[ivn].y, 0 ) == 0 )
										if( FindSegmentIntersections(	pRef.right, pRef.top, pRef.right, pRef.bottom, 0,
																		Pt[iv].x, Pt[iv].y, Pt[ivn].x, Pt[ivn].y, 0 ) == 0 )
											if( FindSegmentIntersections(	pRef.right, pRef.top, pRef.left, pRef.top, 0,
																			Pt[iv].x, Pt[iv].y, Pt[ivn].x, Pt[ivn].y, 0 ) == 0 )
												continue;
								intersection = max(0,intersection);
								break;
							}
							if (intersection != -1)
								break;
						}
					}
			}
		}
		// BOARD...
		if (intersection == -1)
		{
			for ( int b=bo->GetSize()-1; b>=0; b-- )
			{
				CPolyLine * pb = &(*bo)[b];
				if( pb )
				{
					if( pb->GetLayer() != LAY_BOARD_OUTLINE )
						continue;
					RECT br;
					br = pb->GetBounds();
					if( RectsIntersection(pRef,br) < 0 )
						continue;
					if( pb->TestPointInside(pRef.left,pRef.bottom) )
						if( pb->TestPointInside(pRef.left,pRef.top) )
							if( pb->TestPointInside(pRef.right,pRef.bottom) )
								if( pb->TestPointInside(pRef.right,pRef.top) )
									return 0;		// ok, ref position found
				}
			}
		}
	}
	int x,y;
	if ( (pRect.right - pRect.left) > (pRect.top - pRect.bottom) )
	{
		if ( p->side )
		{
			x = (pRect.right + pRect.left)/2 + max((pRef.right - pRef.left),(pRef.top - pRef.bottom))/2;
			a = p->angle;
		}
		else
		{
			x = (pRect.right + pRect.left)/2 - max((pRef.right - pRef.left),(pRef.top - pRef.bottom))/2;
			a = -p->angle;
		}
		y = (pRect.top + pRect.bottom)/2 - p->m_ref_size/2;
	}
	else
	{
		x = (pRect.right + pRect.left)/2 + p->m_ref_size/2;
		if ( p->side )
		{
			y = (pRect.top + pRect.bottom)/2 + max((pRef.right - pRef.left),(pRef.top - pRef.bottom))/2;
			a = (90+p->angle);
		}
		else
		{
			y = (pRect.top + pRect.bottom)/2 - max((pRef.right - pRef.left),(pRef.top - pRef.bottom))/2;
			a = (-90-p->angle);
		}
	}
	MoveRefText(p,x,y,a,p->m_ref_size,p->m_ref_w);
	return pbad;
}





// 
void CPartList::FindNetPointForPart( cpart * p, CPoint * pp, int * np, cpart * found_p, CPoint * found_pp, int * n_pins_group, int iMODE )
{
	(*found_pp).x = 0;
	(*found_pp).y = 0;
	*n_pins_group = 0;
	int MARK = 1;
	int m_n_pins = 0;
	int n_fp_pins = 0;
	double x_sum=0, y_sum=0, x_sum_fp=0, y_sum_fp=0, Smax=0;
	for(cpart* gp=GetFirstPart(); gp; gp=GetNextPart(gp))
		if( gp->selected && gp->utility && p != gp )
		{
			// reset flags
			for( int ip=gp->shape->GetNumPins()-1; ip>=0; ip-- )
				gp->pin[ip].utility = 0;
			for( int ip=p->shape->GetNumPins()-1; ip>=0; ip-- )
				p->pin[ip].utility = 0;
			RECT gr;
			GetPartBoundingRect(gp,&gr);
			double Sgp = (double)((double)gr.right-(double)gr.left)*(double)((double)gr.top-(double)gr.bottom);
			int sum_pins = 0;
			double sum_x=0, sum_y=0;
			for( int pins=p->shape->GetNumPins()-1; pins>=0; pins-- )
			{
				if( !p->pin[pins].net )
					continue;	
				if( !p->pin[pins].net->visible )
					continue;	
				for( int gpins=gp->shape->GetNumPins()-1; gpins>=0; gpins-- )
				{
					if( !gp->pin[gpins].net )
						continue;
					if( !gp->pin[gpins].net->visible )
						continue;
					if( p->pin[pins].net == gp->pin[gpins].net )
					{	
						if( !gp->pin[gpins].utility )
						{
							sum_x += gp->pin[gpins].x;
							sum_y += gp->pin[gpins].y;
							sum_pins++;
							gp->pin[gpins].utility = TRUE;
						}
						if( !p->pin[pins].utility )
						{			
							x_sum_fp += p->pin[pins].x;
							y_sum_fp += p->pin[pins].y;
							n_fp_pins++;
							p->pin[pins].utility = TRUE;
							
						}
					}
				}
			}
			(*n_pins_group) += sum_pins;

			if( iMODE == 0 )
			{
				m_n_pins += sum_pins;
				x_sum += sum_x;
				y_sum += sum_y;
			}
			if( sum_pins > m_n_pins || ( sum_pins == m_n_pins && Sgp > Smax ) )
			{
				found_p = gp;
				Smax = Sgp;			
				if( iMODE )
				{
					m_n_pins = sum_pins;
					x_sum = sum_x;
					y_sum = sum_y;
				}
			}
		}

	if( m_n_pins )
	{
		(*found_pp).x = x_sum/(double)m_n_pins;
		(*found_pp).y = y_sum/(double)m_n_pins;
		(*pp).x = x_sum_fp/(double)n_fp_pins;
		(*pp).y = y_sum_fp/(double)n_fp_pins;
		(*np) = n_fp_pins;
	}
	else
	{
		(*found_pp).x = 0;
		(*found_pp).y = 0;
		(*pp).x = 0;
		(*pp).y = 0;
		(*np) = 0;
	}
}






//
CPoint CPartList::PartAutoLocation ( cpart * p, int step, int iMODE, int pad_clearance )
{
	CPoint pp, around;
	int npp, n_pins_g;
	cpart * fp=0;
	FindNetPointForPart( p, &pp, &npp, fp, &around, &n_pins_g, iMODE );
	int left =	around.x;
	int right = around.x;
	int bottom =around.y;
	int top =	around.y;
	CPoint pCurrent;
	pCurrent.x = around.x;
	pCurrent.y = around.y;
	CPoint pPosition;
	pPosition.x = around.x;
	pPosition.y = around.y;
	RECT rCurrent;
	GetPartBoundingRect(p,&rCurrent);
	RECT rPinsOnly;
	GetPinsBoundingRect(p,&rPinsOnly);
	SwellRect( &rPinsOnly, pad_clearance );
	SwellRect( &rCurrent, rPinsOnly );
	rCurrent.top = rCurrent.top - p->y + around.y;
	rCurrent.left = rCurrent.left - p->x + around.x;
	rCurrent.right = rCurrent.right - p->x + around.x;
	rCurrent.bottom = rCurrent.bottom - p->y + around.y;
	pp.x -= p->x;
	pp.y -= p->y;
	rCurrent.top -= pp.y;
	rCurrent.left -= pp.x;
	rCurrent.right -= pp.x;
	rCurrent.bottom -= pp.y;
	BOOL bSTART = FALSE;
	int nCirquits = 0;
	double nets_len = DEFAULT;
	while(1)
	{
		if( bSTART )
			nCirquits++;
		if( nCirquits > 2 )
			break;
		for( ; pCurrent.x<=right; pCurrent.x+=step, //pNetPoint.x+=step, 
									rCurrent.left+=step, rCurrent.right+=step )
		{
			int rIntersect = -1;
			for(cpart* gp=GetFirstPart(); gp; gp=GetNextPart(gp))
				if(( gp->selected && gp->utility && p != gp ) || ( !gp->selected && !gp->utility ))
				{
					RECT gr;
					int ok = 0;
					if( p->side == gp->side )
						ok = GetPartBoundingRect(gp,&gr);
					else
					{
						ok = GetPartThruPadsRect(gp,&gr);
						if( ok == 0 )
						{
							if( GetPartThruPadsRect(p,&gr) )
								ok = GetPartBoundingRect(gp,&gr);
						}
					}
					if( ok )
						rIntersect = RectsIntersection(rCurrent,gr);
					if( rIntersect >= 0)
						break;
				}
			if( rIntersect >= 0 )
				continue;
			bSTART = TRUE;
			int len = Distance( pCurrent.x+pp.x, pCurrent.y+pp.y, around.x, around.y );
			if( len < nets_len )
			{
				nets_len = len;
				pPosition.x = pCurrent.x-pp.x;
				pPosition.y = pCurrent.y-pp.y;
			}
		}
		right = pCurrent.x;
		for( ; pCurrent.y>=bottom; pCurrent.y-=step, //pNetPoint.y-=step, 
									rCurrent.bottom-=step, rCurrent.top-=step )
		{
			int rIntersect = -1;
			for(cpart* gp=GetFirstPart(); gp; gp=GetNextPart(gp))
				if(( gp->selected && gp->utility && p != gp ) || ( !gp->selected && !gp->utility ))
				{
					RECT gr;
					int ok = 0;
					if( p->side == gp->side )
						ok = GetPartBoundingRect(gp,&gr);
					else
					{
						ok = GetPartThruPadsRect(gp,&gr);
						if( ok == 0 )
						{
							if( GetPartThruPadsRect(p,&gr) )
								ok = GetPartBoundingRect(gp,&gr);
						}
					}
					if( ok )
						rIntersect = RectsIntersection(rCurrent,gr);
					if( rIntersect >= 0)
						break;
				}
			if( rIntersect >= 0 )
				continue;
			bSTART = TRUE;
			int len = Distance( pCurrent.x+pp.x, pCurrent.y+pp.y, around.x, around.y );
			if( len < nets_len )
			{
				nets_len = len;
				pPosition.x = pCurrent.x-pp.x;
				pPosition.y = pCurrent.y-pp.y;
			}
		}
		bottom = pCurrent.y;
		for( ; pCurrent.x>=left; pCurrent.x-=step, //pNetPoint.x-=step, 
									rCurrent.left-=step, rCurrent.right-=step )
		{
			int rIntersect = -1;
			for(cpart* gp=GetFirstPart(); gp; gp=GetNextPart(gp))
				if(( gp->selected && gp->utility && p != gp ) || ( !gp->selected && !gp->utility ))
				{
					RECT gr;
					int ok = 0;
					if( p->side == gp->side )
						ok = GetPartBoundingRect(gp,&gr);
					else
					{
						ok = GetPartThruPadsRect(gp,&gr);
						if( ok == 0 )
						{
							if( GetPartThruPadsRect(p,&gr) )
								ok = GetPartBoundingRect(gp,&gr);
						}
					}
					if( ok )
						rIntersect = RectsIntersection(rCurrent,gr);
					if( rIntersect >= 0)
						break;
				}
			if( rIntersect >= 0 )
				continue;
			bSTART = TRUE;
			int len = Distance( pCurrent.x+pp.x, pCurrent.y+pp.y, around.x, around.y );
			if( len < nets_len )
			{
				nets_len = len;
				pPosition.x = pCurrent.x-pp.x;
				pPosition.y = pCurrent.y-pp.y;
			}
		}
		left = pCurrent.x;
		for( ; pCurrent.y<=top; pCurrent.y+=step, //pNetPoint.y+=step, 
									rCurrent.bottom+=step, rCurrent.top+=step )
		{
			int rIntersect = -1;
			for(cpart* gp=GetFirstPart(); gp; gp=GetNextPart(gp))
				if(( gp->selected && gp->utility && p != gp ) || ( !gp->selected && !gp->utility ))
				{
					RECT gr;
					int ok = 0;
					if( p->side == gp->side )
						ok = GetPartBoundingRect(gp,&gr);
					else
					{
						ok = GetPartThruPadsRect(gp,&gr);
						if( ok == 0 )
						{
							if( GetPartThruPadsRect(p,&gr) )
								ok = GetPartBoundingRect(gp,&gr);
						}
					}
					if( ok )
						rIntersect = RectsIntersection(rCurrent,gr);
					if( rIntersect >= 0)
						break;
				}
			if( rIntersect >= 0 )
				continue;
			bSTART = TRUE;
			int len = Distance( pCurrent.x+pp.x, pCurrent.y+pp.y, around.x, around.y );
			if( len < nets_len )
			{
				nets_len = len;
				pPosition.x = pCurrent.x-pp.x;
				pPosition.y = pCurrent.y-pp.y;
			}
		}
		top = pCurrent.y;
	}
	return pPosition;
}







void CPartList::OptimizeRatlinesOnPin( cnet * pinnet, int innet )
{
	if( !pinnet )
		return;
	cpart * prt = pinnet->pin[innet].part;
	if( !prt->shape )
		return;
	// 1) removing old ratlines
	for( int ic=0; ic<pinnet->nconnects; ic++ )
	{
		if( pinnet->connect[ic].end_pin == innet )
		{
			if( pinnet->pin[pinnet->connect[ic].start_pin].utility )
				return;
			else if( pinnet->connect[ic].locked )
			{	
			}
			else if( pinnet->connect[ic].nsegs == 1 )
			{
				if( pinnet->connect[ic].seg[0].layer == LAY_RAT_LINE ) 
					m_nlist->RemoveNetConnect( pinnet, ic, false );
			}
			else if( pinnet->connect[ic].nsegs > 1 )
			{	
				if( pinnet->connect[ic].seg[pinnet->connect[ic].nsegs-1].layer == LAY_RAT_LINE ) 
				{
					pinnet->connect[ic].seg.SetSize( pinnet->connect[ic].nsegs-1 );
					pinnet->connect[ic].vtx.SetSize( pinnet->connect[ic].nsegs );
					pinnet->connect[ic].nsegs--;
					pinnet->connect[ic].end_pin = cconnect::NO_END;
					pinnet->connect[ic].end_pin_shape = cconnect::NO_END;
				}
			}
		}
		else if( pinnet->connect[ic].start_pin == innet )
		{
			if( pinnet->connect[ic].end_pin == cconnect::NO_END )
			{
				if( pinnet->connect[ic].locked )
				{
				}
				if( pinnet->connect[ic].nsegs == 1 )
				{
					if( pinnet->connect[ic].vtx[1].tee_ID && pinnet->connect[ic].seg[0].layer == LAY_RAT_LINE )
					{
						int tee = pinnet->connect[ic].vtx[1].tee_ID;
						pinnet->connect[ic].vtx[1].tee_ID = 0;
						// find branches
						int tc=-1, tv=-1;
						if( m_nlist->FindTeeVertexInNet( pinnet, tee, &tc, &tv ) == 0 )
						{
							tc = 0;
							tv = 0;
							// find header
							m_nlist->FindTeeVertexInNet( pinnet, tee, &tc, &tv );
							if( Colinear(	pinnet->connect[tc].vtx[tv-1].x,
											pinnet->connect[tc].vtx[tv-1].y,
											pinnet->connect[tc].vtx[tv].x,
											pinnet->connect[tc].vtx[tv].y,
											pinnet->connect[tc].vtx[tv+1].x,
											pinnet->connect[tc].vtx[tv+1].y ))
							{
								pinnet->connect[tc].seg.RemoveAt( tv-1 );
								pinnet->connect[tc].vtx.SetSize( tv );
								pinnet->connect[tc].nsegs--;
							}
						}
						m_nlist->RemoveNetConnect( pinnet, ic, false );		
					}
				}
			}
			else if( pinnet->pin[pinnet->connect[ic].end_pin].utility )
				return;
			else if( pinnet->connect[ic].locked )
			{		
			}
			else if( pinnet->connect[ic].nsegs == 1 )
			{	
				if( pinnet->connect[ic].seg[0].layer == LAY_RAT_LINE ) 
					m_nlist->RemoveNetConnect( pinnet, ic, false );
			}
		}
	}
	// 2) test on contact to pin with utility
#define MAX_BRANCHES 5000
	int con[MAX_BRANCHES];
	int vtx[MAX_BRANCHES];
	int caret=0;
	con[caret] = -1;
	vtx[caret] = -1;
	do
	{
		for(int ic=con[caret]+1; ic<pinnet->nconnects; ic++ )
		{
			if( pinnet->connect[ic].start_pin == innet || pinnet->connect[ic].end_pin == innet )
			{
				for( int iv=vtx[caret]+1; iv<=pinnet->connect[ic].nsegs; iv++ )
				{
					if( pinnet->connect[ic].vtx[iv].tee_ID )
					{
						if( iv == pinnet->connect[ic].nsegs )
						{
							// find header
							int tc=0, tv=0;
							int find = m_nlist->FindTeeVertexInNet( pinnet, pinnet->connect[ic].vtx[iv].tee_ID, &tc, &tv );
							if( find )
							{

							}
						}
						else
						{

						}
					}
				}
			}
			if( caret )
			{
				caret--;
				break;
			}
		}
	}while( con[0] >= 0 );
	// 3) test on contact to areas
	// 4) find pin with utility
}