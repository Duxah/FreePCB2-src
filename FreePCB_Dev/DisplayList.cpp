// DisplayList.cpp : implementation of class CDisplayList
//
// this is a linked-list of display elements
// each element is a primitive graphics object such as a line-segment,
// circle, annulus, etc.
//
#include "stdafx.h" 
#include <math.h>

// dimensions passed to DisplayList from the application are in PCBU (i.e. nm)
// since the Win95/98 GDI uses 16-bit arithmetic, PCBU must be scaled to DU (i.e. mils) 
//
//#define PCBU_PER_DU		PCBU_PER_WU
//#define MIL_PER_DU		NM_PER_MIL/PCBU_PER_DU	// conversion from mils to display units
//#define DU_PER_MIL		PCBU_PER_DU/NM_PER_MIL	// conversion from display units to mils

#define DL_MAX_LAYERS	32
#define HILITE_POINT_W	10	// size/2 of selection box for points (mils)  

extern CFreePcbApp theApp;
// constructor
//
CDisplayList::CDisplayList()
{
	// create lists for all layers
	for( int layer=0; layer<MAX_LAYERS; layer++ )
	{
		// linked list for layer
		// default colors, these should be overwritten with actual colors
		m_rgb[layer][0] = layer*63;			// R 
		m_rgb[layer][1] = (layer/4)*127;	// G
		m_rgb[layer][2] = (layer/8)*63;		// B
		// default order
		m_layer_in_order[layer] = layer;	// will be drawn from highest to lowest
		m_order_for_layer[layer] = layer;
	}
	m_start.prev = 0;		// first element
	m_start.next = &m_end;
	m_start.magic = 0;
	m_end.next = 0;			// last element
	m_end.prev = &m_start;
	m_end.magic = 0;
	// miscellaneous
	m_drag_flag = 0;
	m_drag_num_lines = 0;
	m_drag_line_pt = 0;
	m_drag_num_ratlines = 0;
	m_drag_ratline_start_pt = 0;
	m_drag_ratline_end_pt = 0;
	m_drag_ratline_width = 0;
	m_cross_hairs = 0;
	m_visual_grid_on = 0;
	m_visual_grid_spacing = 0;
	m_inflection_mode = IM_NONE;
	m_last_inflection_mode = IM_NONE;
	m_highlight_mode = EL_NON_STATIC;
	m_grid_style = 0;
	m_drag_layer = 0;
}

// destructor
//
CDisplayList::~CDisplayList()
{
	RemoveAll();
}

void CDisplayList::RemoveAll()
{
	// traverse lists for all layers, removing all elements
	while( m_end.prev != &m_start )
	{
		dl_element *  el = m_end.prev;
		Remove( el );
	}
	if( m_drag_line_pt )
	{
		free( m_drag_line_pt );
		m_drag_line_pt = 0;
	}
	if( m_drag_ratline_start_pt )
	{
		free( m_drag_ratline_start_pt );
		m_drag_ratline_start_pt = 0;
	}
	if( m_drag_ratline_end_pt )
	{
		free( m_drag_ratline_end_pt );
		m_drag_ratline_end_pt = 0;
	}
}

// Set conversion parameters between world coords (used by elements in
// display list) and window coords (pixels)
//
// enter with:
//	client_r = window client rectangle (pixels)
//	screen_r = window client rectangle in screen coords (pixels)
//	pane_org_x = starting x-coord of PCB drawing area in client rect (pixels)
//  pane_bottom_h = height of bottom pane (pixels)
//	pcb units per pixel = nm per pixel
//	org_x = x-coord of left edge of drawing area (pcb units)
//	org_y = y-coord of bottom edge of drawing area (pcb units)
//
// note that the actual scale factor is set by the arguments to 
// SetWindowExt and SetViewportExt, and may be slightly different for x and y
//
void CDisplayList::SetMapping( CRect *client_r, CRect *screen_r, int pane_org_x, int pane_bottom_h, 
							double pcbu_per_pixel, int org_x, int org_y )
{
	m_client_r = client_r;				// pixels
	m_screen_r = screen_r;				// pixels
	m_pane_org_x = pane_org_x;			// pixels
	m_bottom_pane_h = pane_bottom_h;	// pixels
	m_pane_org_y = client_r->bottom - pane_bottom_h;	// pixels
	m_scale = pcbu_per_pixel/m_pcbu_per_wu;	// world units per pixel
	m_org_x = org_x/m_pcbu_per_wu;		// world units
	m_org_y = org_y/m_pcbu_per_wu;		// world units

	//now set extents
	double rw = m_client_r.Width();	// width of client area (pixels)
	double rh = m_client_r.Height();	// height of client area (pixels)
	double ext_x = rw*pcbu_per_pixel/m_pcbu_per_wu;	// width in WU
	double ext_y = rh*pcbu_per_pixel/m_pcbu_per_wu;	// height in WU
	int div = 1, 
		mult = m_pcbu_per_wu;
	
	//if( ext_x*mult/div > INT_MAX )
	//	ASSERT(0);
	//if( ext_y*mult/div > INT_MAX )
	//	ASSERT(0);
	w_ext_x = ext_x*mult/div;
	v_ext_x = rw*mult/div;
	w_ext_y = ext_y*mult/div;
	v_ext_y = -rh*mult/div;
	m_wu_per_pixel_x = (double)w_ext_x/v_ext_x;		// actual ratios
	m_wu_per_pixel_y = (double)w_ext_y/v_ext_y;
	m_pcbu_per_pixel_x = m_wu_per_pixel_x * m_pcbu_per_wu;		
	m_pcbu_per_pixel_y = m_wu_per_pixel_y * m_pcbu_per_wu;
	m_max_x = m_org_x + m_wu_per_pixel_x*(client_r->right-pane_org_x) + 2;	// world units
	m_max_y = m_org_y - m_wu_per_pixel_y*client_r->bottom + 2;				// world units
}

// add graphics element used for selection
//
dl_element * CDisplayList::AddSelector( id _id, void * ptr, int gtype, int visible, 
										RECT * el_rect, int el_w, CPoint * p, int np, int map_orig_layer )
{
	int lay = 0;
	setbit( lay, LAY_SELECTION );
	dl_element * test = Add( _id, ptr, lay, gtype, visible, el_rect, el_w, p, np );
	test->map_orig_layer = map_orig_layer;
	return test;
}

void CDisplayList::AddSelector( dl_element * el )
{
	el->map_orig_layer |= el->layers_bitmap;
	setbit( el->layers_bitmap, LAY_SELECTION );
}

// Add entry to end of list, returns pointer to element created.
//
// Dimensional units for input parameters are PCBU
//
dl_element * CDisplayList::Add( id _id, void * ptr, int layers_bitmap, int gtype, int visible,
								RECT * el_rect, int el_w, CPoint * p, int np, BOOL convert )
{

	// create new element and link into list
	dl_element * new_element = new dl_element;// add
	new_element->prev = m_end.prev;
	new_element->next = &m_end;
	new_element->prev->next = new_element;
	new_element->next->prev = new_element;
	new_element->magic = NULL;
	// now copy data from entry into element	
	new_element->id = _id;
	new_element->ptr = ptr;
	new_element->gtype = gtype;
	new_element->visible = visible;

	int per = m_pcbu_per_wu;
	if( !convert )
		per = 1;
	if(p&&np)
	{
		new_element->pts.SetSize( np );
		for( int i=np-1; i>=0; i-- )
		{
			new_element->pts[i].x = p[i].x/per;
			new_element->pts[i].y = p[i].y/per;
		}
	}
	else
		new_element->pts.SetSize( 0 );

	new_element->el_w =	el_w/per;
	if( el_rect )
	{
		new_element->rect.left =	el_rect->left	/per;
		new_element->rect.right =	el_rect->right	/per;
		new_element->rect.bottom =	el_rect->bottom	/per;
		new_element->rect.top =		el_rect->top	/per;
	}
	else if( p&&np >= 2 )
		new_element->rect = rect(new_element->pts[0].x,new_element->pts[0].y,new_element->pts[1].x,new_element->pts[1].y);
	else
		new_element->rect = rect(0,0,0,0);
	new_element->rectGroup.left =	INT_MAX     /per;
	new_element->layers_bitmap = layers_bitmap;
	new_element->map_orig_layer = 0;
	new_element->el_static = EL_NON_STATIC;
	new_element->transparent = 0;
	new_element->dlist = this;
	return new_element;
}

void CDisplayList::HighLight( dl_element * el )
{
	el->map_orig_layer |= el->layers_bitmap;
	setbit( el->layers_bitmap, LAY_HILITE );
}

int CDisplayList::HighlightAll()
{
	for( dl_element * el=m_start.next; el->next; el=el->next )
		HighLight( el );
	return 0;
}

RECT CDisplayList::GetHighlightedBounds( int * map_orig_layer )
{
	RECT r;
	r.left = r.bottom = INT_MAX;
	r.right = r.top = INT_MIN;
	if( map_orig_layer == NULL )
		return r;
	for( dl_element * el=m_start.next; el->next; el=el->next )
	{
		if( getbit( el->layers_bitmap, LAY_HILITE ) && el->el_static == 0 )
		{
			SwellRect(&r, el->rect );
			*map_orig_layer |= el->map_orig_layer;
			if(el->pts.GetSize() == 4)
			{
				SwellRect(&r, el->pts[0] );
				SwellRect(&r, el->pts[1] );
				SwellRect(&r, el->pts[2] );
				SwellRect(&r, el->pts[3] );
			}
		}
	}
	if( r.right > r.left )
	{
		r.top *= m_pcbu_per_wu;
		r.left *= m_pcbu_per_wu;
		r.right *= m_pcbu_per_wu;		
		r.bottom *= m_pcbu_per_wu;
	}
	return r;
}

dl_element * CDisplayList::Cpy( dl_element * el )
{
	dl_element * cel=0;
	if( el->pts.GetSize() )
		cel = Add( el->id, el->ptr, el->layers_bitmap, 
					el->gtype, el->visible, &el->rect, el->el_w, 
					&el->pts[0], el->pts.GetSize(), FALSE );
	else
		cel = Add( el->id, el->ptr, el->layers_bitmap, 
					el->gtype, el->visible, &el->rect, el->el_w, 
					NULL, 0, FALSE );
	cel->map_orig_layer = el->map_orig_layer;
	cel->transparent = el->transparent;
	cel->el_static = el->el_static;
	return cel;
}


// set element parameters in PCBU
//

void CDisplayList::Move( dl_element * el, int dx, int dy )
{
	for( int i=el->pts.GetSize()-1; i>=0; i-- )
	{
		el->pts[i].x += dx;
		el->pts[i].y += dy;
	}
}


// get element parameters in PCBU
//
dl_element * CDisplayList::Get_Start()
{
	return &m_start;
}
dl_element * CDisplayList::Get_End()
{
	return &m_end;
}
RECT * CDisplayList::Get_Rect( dl_element * el, RECT * Get )
{
	if( Get )
	{
		(*Get).left = el->rect.left		*m_pcbu_per_wu;
		(*Get).bottom = el->rect.bottom	*m_pcbu_per_wu;
		(*Get).right = el->rect.right	*m_pcbu_per_wu;
		(*Get).top = el->rect.top		*m_pcbu_per_wu;
	}
	return &el->rect;
}

CArray<CPoint> * CDisplayList::Get_Points( dl_element * el, CPoint * Get, int * npoints, int dx, int dy )
{
	if( el )
	{
		if( npoints )
		{
			*npoints = min( *npoints, el->pts.GetSize() );
			if( Get ) for( int ii=(*npoints)-1; ii>=0; ii-- )
			{
				Get[ii].x = el->pts[ii].x * m_pcbu_per_wu + dx;
				Get[ii].y = el->pts[ii].y * m_pcbu_per_wu + dy;
			}
		}
		return &el->pts;
	}
	if( npoints )
		*npoints = 0;
	return NULL;
}

int CDisplayList::Get_el_w( dl_element * el )
{
	return el->el_w * m_pcbu_per_wu;
}

int CDisplayList::Get_Selected ()
{ 
	dl_element * el = &m_end;
	while( el->prev != &m_start )
	{
		el = el->prev;
		if( getbit( el->layers_bitmap, LAY_HILITE ) )
			return 1;
	}
	return 0;
}

void CDisplayList::Set_RectGroup( dl_element * el, int l, int r, int b, int t )
{
	el->rectGroup.left =		l/m_pcbu_per_wu;
	el->rectGroup.right =	r/m_pcbu_per_wu;
	el->rectGroup.bottom =	b/m_pcbu_per_wu;
	el->rectGroup.top =		t/m_pcbu_per_wu;
}

BOOL CDisplayList::Get_RectGroup( dl_element * el, RECT * rgr )
{
	if( el->rectGroup.left < (INT_MAX/m_pcbu_per_wu)-1 )
	{
		rgr->left =		el->rectGroup.left	*m_pcbu_per_wu;
		rgr->right =	el->rectGroup.right	*m_pcbu_per_wu;
		rgr->bottom =	el->rectGroup.bottom*m_pcbu_per_wu;
		rgr->top =		el->rectGroup.top	*m_pcbu_per_wu;
		return 1;
	}
	else
		return 0;
}

void CDisplayList::Get_Endpoints(CPoint *cpi, CPoint *cpf) 
{ 
	cpi->x = m_drag_xi*m_pcbu_per_wu; cpi->y = m_drag_yi*m_pcbu_per_wu;
	cpf->x = m_drag_xf*m_pcbu_per_wu; cpf->y = m_drag_yf*m_pcbu_per_wu;
}

// Remove element from list, return id
//
id CDisplayList::Remove( dl_element * element )
{
	if( !element )
	{
		id no_id;
		return no_id;
	}
	// remove links to this element
	element->next->prev = element->prev;
	element->prev->next = element->next;

	// destroy element
	id el_id = element->id;
	element->pts.SetSize(0);
	delete( element );
	return el_id;
}

// Draw the display list using device DC or memory DC
//
void CDisplayList::Draw( CDC * dDC, int draw_layer )
{
	static int mem_org_x = 0;
	static int mem_max_x = 0;	
	if( draw_layer )
	{
		if( mem_org_x != m_org_x || mem_max_x != m_max_x )
			draw_layer = 0;
		else if( draw_layer < 0 )
			draw_layer = 0;
	}
	CDC * pDC = dDC;
	if( botDC )
		pDC = botDC;
	//
	COLORREF CLR_BACKGND = RGB( m_rgb[LAY_BACKGND][0], m_rgb[LAY_BACKGND][1], m_rgb[LAY_BACKGND][2] );	
	pDC->SetBkColor( CLR_BACKGND );
	//
	// create pens and brushes
	CBrush backgnd_brush( CLR_BACKGND );
	CPen backgnd_pen( PS_SOLID, 1, CLR_BACKGND );
	CPen * old_pen = pDC->SelectObject( &backgnd_pen );
	CBrush * old_brush = pDC->SelectObject( &backgnd_brush );
	RECT client_rect;
	client_rect.left = m_org_x;
	client_rect.right = m_max_x;
	client_rect.bottom = m_org_y;
	client_rect.top = m_max_y;
	int old_BkMode = pDC->GetBkMode();
	if( !draw_layer )
	{	
		pDC->Rectangle( &client_rect );
		// visual grid
		if( m_visual_grid_on && (m_visual_grid_spacing/m_scale)>5 && m_vis[LAY_VISIBLE_GRID] )
		{
			CPen grid_pen( PS_SOLID, 1, RGB( m_rgb[LAY_VISIBLE_GRID][0], m_rgb[LAY_VISIBLE_GRID][1], m_rgb[LAY_VISIBLE_GRID][2] ) );
			pDC->SelectObject( &grid_pen );
			int startix = m_org_x/m_visual_grid_spacing;
			int startiy = m_org_y/m_visual_grid_spacing;
			double start_grid_x = startix*m_visual_grid_spacing;
			double start_grid_y = startiy*m_visual_grid_spacing;
			if( m_grid_style )
			{
				for( double ix=start_grid_x; ix<m_max_x; ix+=m_visual_grid_spacing )
				{
					CPoint gp(ix,m_org_y);
					pDC->MoveTo(gp);
					gp.x = ix;
					gp.y = m_max_y;
					pDC->LineTo(gp);
				}
				for( double iy=start_grid_y; iy<m_max_y; iy+=m_visual_grid_spacing )
				{
					CPoint gp(m_org_x,iy);
					pDC->MoveTo(gp);
					gp.x = m_max_x;
					gp.y = iy;
					pDC->LineTo(gp);
				}
			}
		}
	}

	int m_vis_mask = 0;
	for( int lay=max(LAY_HILITE,LAY_SELECTION)+1; lay<MAX_LAYERS; lay++ )
	{
		if( m_vis[lay] )
			setbit( m_vis_mask, lay );
	}
	// now traverse the lists, starting with the layer in the last element 
	// of the m_order[] array
	int nlines = 0;
	int size_of_2_pixels = 4*m_scale;
	//
	int draw_order = m_order_for_layer[draw_layer];
	for( int order=(MAX_LAYERS-1); order>=0; order-- )
	{
		//if( ??? )
		{
		//time out...
		//	break;
		}
		//
		int layer = m_layer_in_order[order];
		if( !m_vis[layer] || layer == LAY_SELECTION )
			continue;
		if( draw_layer )
		{
			if( order > m_order_for_layer[draw_layer] )
				continue;
		}
		if( order <= m_order_for_layer[LAY_HILITE] && pDC == botDC ) // switch on other DC
		{
			if( topDC )
			{
				topDC->BitBlt(	m_org_x, m_org_y, m_max_x-m_org_x, m_max_y-m_org_y,
								botDC, m_org_x, m_org_y, SRCCOPY ) ;
				pDC = topDC;
			}
		}
		int dsp = (INT_MAX/m_pcbu_per_wu)-1;
		dl_element * el = &m_start;
		while( el->next->next )
		{
			el = el->next;

			// if el magical
			if( el->magic )
			{
				if( el->rectGroup.left < dsp )
				{
					if( el->rectGroup.left > m_max_x || 
						el->rectGroup.right < m_org_x ||
						el->rectGroup.bottom > m_max_y ||
						el->rectGroup.top < m_org_y )
					{
						el = el->magic;
						continue;
					}
				}
				else if( !getbit( el->layers_bitmap, layer ) )
				{
					if( layer == LAY_HILITE )
					{
						// empty
					}
					else 
					{
						el = el->magic;
						continue;
					}
				}
			}
			if( !getbit( el->layers_bitmap, layer ) )
				continue;

			// if el hilite
			if( layer == LAY_HILITE ) // (if orig layer not vis)	
			{			
				if( !(m_vis_mask & el->map_orig_layer))
					continue;
			}
			else if( !el->visible )
				continue;
			
			// el rect
			if( el->rect.left > m_max_x )
				continue;
			if( el->rect.right < m_org_x )
				continue;
			if( el->rect.bottom > m_max_y )
				continue;
			if( el->rect.top < m_org_y )
				continue;

			// COLOR FOR TRANSPARENT MODE
			int CL1,CL2,CL3,min_t=60;
			CL3 = min_t + m_rgb[el->transparent][0]/2;
			CL1 = min_t + m_rgb[el->transparent][1]/2;
			CL2 = min_t + m_rgb[el->transparent][2]/2;
			CL1 = min(150, CL1);
			CL2 = min(150, CL2);
			CL3 = min(150, CL3);
			COLORREF Transparent_color = RGB( CL1,CL2,CL3 );
			COLORREF Layer_color = RGB( m_rgb[layer][0], m_rgb[layer][1], m_rgb[layer][2] );
			COLORREF * lay_clr; 
			if( el->transparent && layer == LAY_HILITE )
				lay_clr = &Transparent_color;
			else
				lay_clr = &Layer_color;
			CPen T__pen( PS_SOLID, el->el_w, Transparent_color );
			CPen L__pen( PS_SOLID, el->el_w, Layer_color );
			CPen pen( PS_SOLID, el->el_w, *lay_clr );
			CPen line_pen( PS_SOLID, 0, *lay_clr );
			CBrush fill_brush( *lay_clr );
			int old_ROP2;
			if( el->transparent && layer == LAY_HILITE )
			{
				old_ROP2 = pDC->SetROP2(R2_MERGEPEN);
				pDC->SelectObject( T__pen );
			}
			else
				pDC->SelectObject( pen );
			pDC->SelectObject( fill_brush );
			
//---------> first type
			if( el->gtype == DL_CIRC || el->gtype == DL_HOLLOW_CIRC )
			{
				if( el->gtype == DL_HOLLOW_CIRC )
					pDC->SelectObject( GetStockObject( HOLLOW_BRUSH ) );
				pDC->SelectObject( line_pen );
				pDC->Ellipse(	el->rect.left, 
								el->rect.bottom, 
								el->rect.right, 
								el->rect.top );		
			}
//---------> next type
			else if( el->gtype == DL_LINE || el->gtype == DL_HOLLOW_LINE )
			{
				if( el->pts.GetSize() >= 2 )
				{
					// only draw line segments which are in viewport
					// line segment from (xi,yi) to (xf,yf)
					int draw_flag = 0;
					int xi = el->pts[0].x;
					int xf = el->pts[1].x;
					int yi = el->pts[0].y;
					int yf = el->pts[1].y;

					if( el->rect.right < m_max_x && 
						el->rect.left > m_org_x )
						draw_flag = 1;
					else if( el->rect.top < m_max_y && 
							 el->rect.bottom > m_org_y )
						draw_flag = 1;
					else 
					{
						double angle = Angle( xi,yi,xf,yf );
						CPoint P[5];
						P[0].x = m_org_x, P[0].y = m_org_y ;			//bot_left( m_org_x, m_org_y );
						P[1].x = m_org_x, P[1].y = m_max_y ;			//top_left( m_org_x, m_max_y );
						P[2].x = m_max_x, P[2].y = m_org_y ;			//bot_right( m_max_x, m_org_y );
						P[3].x = m_max_x, P[3].y = m_max_y ;			//top_right( m_max_x, m_max_y );
						P[4].x = xi, P[4].y = yi ;						//p1( xi,yi );
						RotatePOINTS( P, 5, -angle, zero );
						if( ( P[0].y < P[4].y && P[1].y < P[4].y && P[2].y < P[4].y && P[3].y < P[4].y ) ||
							( P[0].y > P[4].y && P[1].y > P[4].y && P[2].y > P[4].y && P[3].y > P[4].y ) )
						{
							int test=0;
							// протестировано: работает на ура
							// tested: works with a bang
						}
						else
							draw_flag = 1;
					}
					// now draw the line segment if not clipped
					if( draw_flag )
					{
						if ( el->gtype == DL_LINE )
						{	
							pDC->MoveTo( xi, yi );
							pDC->LineTo( xf, yf );
							nlines++;
						}
						else 
						{
							pDC->SelectObject( line_pen );
							int sz = el->pts.GetSize();
							if( sz >= 4 )
								pDC->Polyline( &el->pts[2], sz-2 );
						}
					}
				}
			}
			//---------> next type
			else if( el->gtype == DL_LINES_ARRAY )
			{
				int sz = el->pts.GetSize();
				if( el->transparent == TRANSPARENT_BLACK_GROUND )
				{
					pDC->SetROP2(old_ROP2);
					CPen TBlack__pen( PS_SOLID, el->el_w*7, RGB( m_rgb[LAY_SELECTION][0], m_rgb[LAY_SELECTION][1], m_rgb[LAY_SELECTION][2] ) );
					pDC->SelectObject( TBlack__pen );
					for( int ii=0; ii<sz; ii+=2 )
					{
						pDC->MoveTo( el->pts[ii].x, el->pts[ii].y );
						pDC->LineTo( el->pts[ii+1].x, el->pts[ii+1].y );
					}
					pDC->SelectObject( L__pen );		
				}			
				for( int ii=0; ii<sz; ii+=2 )
				{
					pDC->MoveTo( el->pts[ii].x, el->pts[ii].y );
					pDC->LineTo( el->pts[ii+1].x, el->pts[ii+1].y );
				}
				if( el->transparent == TRANSPARENT_BLACK_GROUND )
					pDC->SetROP2(R2_MERGEPEN);
				nlines+=(sz/2);
			}
//---------> next type
			else if( el->gtype == DL_HOLE )
			{
				pDC->SelectObject( line_pen );
				pDC->Ellipse(	el->rect.left, 
								el->rect.bottom, 
								el->rect.right, 
								el->rect.top );
			}
//---------> next type
			else if( el->gtype == DL_RECT )
			{
				pDC->SelectObject( line_pen );
				pDC->Rectangle( el->rect.left, 
								el->rect.bottom, 
								el->rect.right, 
								el->rect.top );
			}
//---------> next type
			else if( el->gtype == DL_HOLLOW_RECT )
			{
				pDC->SelectObject( GetStockObject( HOLLOW_BRUSH ) );
				pDC->SelectObject( line_pen );
				pDC->MoveTo( el->rect.left, el->rect.bottom );
				pDC->LineTo( el->rect.left, el->rect.top );
				pDC->LineTo( el->rect.right, el->rect.top );
				pDC->LineTo( el->rect.right, el->rect.bottom );
				pDC->LineTo( el->rect.left, el->rect.bottom );		
				nlines += 4;	
			}
//---------> next type
			else if( el->gtype == DL_RRECT || el->gtype == DL_HOLLOW_RRECT )
			{
				if( el->gtype == DL_HOLLOW_RRECT )
					pDC->SelectObject( GetStockObject( HOLLOW_BRUSH ) );
				pDC->SelectObject( line_pen );
				pDC->RoundRect( el->rect.left, 
								el->rect.bottom, 
								el->rect.right, 
								el->rect.top, 
								2*el->el_w, 
								2*el->el_w );	
			}
//---------> next type
			else if( el->gtype == DL_POLYGON || el->gtype == DL_POLYLINE )
			{
				int np = el->pts.GetSize();
				if( el->el_w || el->gtype == DL_POLYLINE )
				{
					pDC->MoveTo( el->pts[0].x, el->pts[0].y );
					for( int n=1; n<np; n++ )
						pDC->LineTo( el->pts[n].x, el->pts[n].y );
					if( el->gtype == DL_POLYGON )
						pDC->LineTo( el->pts[0].x, el->pts[0].y );
					nlines += np-1;
				}
				if( el->gtype == DL_POLYGON )
				{
					pDC->SelectObject( fill_brush );
					pDC->Polygon( &el->pts[0], np );
				}
			}
//---------> next type
			else if( el->gtype == DL_RECT_X )
			{
				if( el->pts.GetSize() == 4 )
				{
					pDC->MoveTo( el->pts[0].x,  el->pts[0].y );
					pDC->LineTo( el->pts[1].x,  el->pts[1].y );
					pDC->LineTo( el->pts[2].x,  el->pts[2].y );
					pDC->LineTo( el->pts[3].x,  el->pts[3].y );
					pDC->LineTo( el->pts[0].x,  el->pts[0].y );
					pDC->LineTo( el->pts[2].x,  el->pts[2].y );
					pDC->MoveTo( el->pts[1].x,  el->pts[1].y );
					pDC->LineTo( el->pts[3].x,  el->pts[3].y );
				}
				else
				{
					pDC->MoveTo( el->rect.left,  el->rect.bottom );
					pDC->LineTo( el->rect.right, el->rect.bottom );
					pDC->LineTo( el->rect.right, el->rect.top );
					pDC->LineTo( el->rect.left,  el->rect.top );
					pDC->LineTo( el->rect.left,  el->rect.bottom );
					pDC->LineTo( el->rect.right, el->rect.top );
					pDC->MoveTo( el->rect.left,  el->rect.top );
					pDC->LineTo( el->rect.right, el->rect.bottom );
				}
				nlines += 6;
			}
//---------> next type
			else if( el->gtype == DL_ARC_CW )
			{
				DrawArc( pDC, DL_ARC_CW, el->pts[0].x, el->pts[0].y, el->pts[1].x, el->pts[1].y );
			}
//---------> next type
			else if( el->gtype == DL_ARC_CCW )
			{
				DrawArc( pDC, DL_ARC_CCW, el->pts[0].x, el->pts[0].y, el->pts[1].x, el->pts[1].y );
			}
//---------> next type
			else if( el->gtype == DL_CURVE_CW )
			{
				DrawCurve( pDC, DL_CURVE_CW, el->pts[0].x, el->pts[0].y, el->pts[1].x, el->pts[1].y );
			}
//---------> next type
			else if( el->gtype == DL_CURVE_CCW )
			{
				DrawCurve( pDC, DL_CURVE_CCW, el->pts[0].x, el->pts[0].y, el->pts[1].x, el->pts[1].y );
			}
//---------> next type
			else if( el->gtype == DL_X )
			{
				if( el->pts.GetSize() == 4 )
				{
					pDC->MoveTo( el->pts[0].x,  el->pts[0].y );
					pDC->LineTo( el->pts[2].x,  el->pts[2].y );
					pDC->MoveTo( el->pts[1].x,  el->pts[1].y );
					pDC->LineTo( el->pts[3].x,  el->pts[3].y );
				}
				else
				{
					pDC->MoveTo( el->rect.left,  el->rect.bottom );
					pDC->LineTo( el->rect.right, el->rect.top );
					pDC->MoveTo( el->rect.right, el->rect.bottom );
					pDC->LineTo( el->rect.left,  el->rect.top );
				}
				nlines += 2;
			}
//---------> next type
			else if( el->gtype == DL_CENTROID )
			{
				if( el->pts.GetSize() == 1 )
				{
					// x,y are center coords; w = width; 
					// xf,yf define arrow end-point for P&P orientation
					int w = el->rect.right - el->rect.left;
					int x = (el->rect.right + el->rect.left)/2;
					int y = (el->rect.bottom + el->rect.top)/2;
					pDC->Ellipse( x - w/4, y - w/4, x + w/4, y + w/4 );
					pDC->SelectObject( fill_brush );
					pDC->MoveTo( el->rect.left,  el->rect.bottom );
					pDC->LineTo( el->rect.right, el->rect.top );
					pDC->MoveTo( el->rect.right, el->rect.bottom );
					pDC->LineTo( el->rect.left,  el->rect.top );
					pDC->MoveTo( x, y );	// p&p arrow
					pDC->LineTo( el->pts[0].x, el->pts[0].y );	
					if( el->pts[0].x > el->rect.right || el->pts[0].x < el->rect.left )   
					{
						// horizontal arrow
						pDC->LineTo( el->pts[0].x - (el->pts[0].x - x)/4, el->pts[0].y + w/4 );
						pDC->LineTo( el->pts[0].x - (el->pts[0].x - x)/4, el->pts[0].y - w/4 );
					}
					else if( el->pts[0].y > el->rect.top || el->pts[0].y < el->rect.bottom )  
					{
						// vertical arrow
						pDC->LineTo( el->pts[0].x + w/4, el->pts[0].y - (el->pts[0].y - y)/4 );
						pDC->LineTo( el->pts[0].x - w/4, el->pts[0].y - (el->pts[0].y - y)/4 );
					}
					else
						ASSERT(0);
					pDC->LineTo( el->pts[0].x, el->pts[0].y );
					nlines += 2;
				}
			}
			if( el->transparent && layer == LAY_HILITE )
				pDC->SetROP2(old_ROP2);
		}
		// restore original objects
		pDC->SelectObject( old_pen );
		pDC->SelectObject( old_brush );
	}
	// visual grid
	if( m_visual_grid_on && (m_visual_grid_spacing/m_scale)>5 && m_vis[LAY_VISIBLE_GRID] )
	{
		if( !m_grid_style )
		{
			int startix = m_org_x/m_visual_grid_spacing;
			int startiy = m_org_y/m_visual_grid_spacing;
			double start_grid_x = startix*m_visual_grid_spacing;
			double start_grid_y = startiy*m_visual_grid_spacing;
			for( double ix=start_grid_x; ix<m_max_x; ix+=m_visual_grid_spacing )
			{
				for( double iy=start_grid_y; iy<m_max_y; iy+=m_visual_grid_spacing )
			 	{
					pDC->SetPixel(ix,iy, RGB(	m_rgb[LAY_VISIBLE_GRID][0], 
												m_rgb[LAY_VISIBLE_GRID][1], 
												m_rgb[LAY_VISIBLE_GRID][2] ));
				}
			}
		}
	}
	
	// draw origin
	CRect r;
	r.top = m_scale*15;
	r.left = -m_scale*15;
	r.right = m_scale*15;
	r.bottom = -m_scale*15;
	CPen pen;
	if( m_rgb[LAY_BACKGND][0]+m_rgb[LAY_BACKGND][1]+m_rgb[LAY_BACKGND][2] < 384 )
		pen.CreatePen(PS_SOLID,1,RGB(255,255,255));
	else
		pen.CreatePen(PS_SOLID,1,RGB(0,0,0));
	pDC->SelectObject( &pen );
	pDC->SelectObject( GetStockObject( GRAY_BRUSH ) );
	pDC->Ellipse( r );
	pDC->SelectObject( GetStockObject( LTGRAY_BRUSH ) );
	r.top = m_scale*10;
	r.left = -m_scale*10;
	r.right = m_scale*10;
	r.bottom = -m_scale*10;
	pDC->Ellipse( r );
	pDC->MoveTo( -m_scale*30, 0 );
	pDC->LineTo( -m_scale*15, 0 );
	pDC->MoveTo( m_scale*30, 0 );
	pDC->LineTo( m_scale*15, 0 );
	pDC->MoveTo( 0, -m_scale*30 );
	pDC->LineTo( 0, -m_scale*15 );
	pDC->MoveTo( 0, m_scale*30 );
	pDC->LineTo( 0, m_scale*15 );
	pDC->SelectObject( old_pen );
	pDC->SelectObject( old_brush );
	
	// if dragging, draw drag lines or shape 
	int old_ROP2 = pDC->SetROP2( R2_XORPEN );
	COLORREF drag_color = RGB( m_rgb[m_drag_layer][0], m_rgb[m_drag_layer][1], m_rgb[m_drag_layer][2] );
	if( m_drag_num_lines )
	{
		// draw line array
		CPen drag_pen( PS_SOLID, 1, drag_color );
		CPen * old_pen = pDC->SelectObject( &drag_pen );
		for( int il=0; il<m_drag_num_lines; il++ )
		{
			pDC->MoveTo( m_drag_x+m_drag_line_pt[2*il].x, m_drag_y+m_drag_line_pt[2*il].y );
			pDC->LineTo( m_drag_x+m_drag_line_pt[2*il+1].x, m_drag_y+m_drag_line_pt[2*il+1].y );
		}
		pDC->SelectObject( old_pen );
	}
	
	if( m_drag_num_ratlines )
	{
		// draw ratline array
		CPen drag_pen( PS_SOLID, 1, drag_color );
		CPen * old_pen = pDC->SelectObject( &drag_pen );
		for( int il=0; il<m_drag_num_ratlines; il++ )
		{
			pDC->MoveTo( m_drag_ratline_start_pt[il].x, m_drag_ratline_start_pt[il].y );
			pDC->LineTo( m_drag_x+m_drag_ratline_end_pt[il].x, m_drag_y+m_drag_ratline_end_pt[il].y );
		}
		pDC->SelectObject( old_pen );
	}
	
	if( m_drag_flag )
	{
		// 4. Redraw the three segments:
		if(m_drag_shape == DS_SEGMENT)
		{
			
			pDC->MoveTo( m_drag_xb, m_drag_yb );
	
			// draw first segment
			CPen pen0( PS_SOLID, m_drag_w0, drag_color );
			CPen * old_pen = pDC->SelectObject( &pen0 );
			pDC->LineTo( m_drag_xi, m_drag_yi );
	
			// draw second segment
			CPen pen1( PS_SOLID, m_drag_w1, drag_color );
			pDC->SelectObject( &pen1 );
			pDC->LineTo( m_drag_xf, m_drag_yf );
	
			// draw third segment
			if(m_drag_style2 != DSS_NONE)
			{
				CPen pen2( PS_SOLID, m_drag_w2, drag_color );
				pDC->SelectObject( &pen2 );
				pDC->LineTo( m_drag_xe, m_drag_ye );
			}
			pDC->SelectObject( old_pen );
		}
		// draw drag shape, if used
		if( m_drag_shape == DS_LINE_VERTEX || m_drag_shape == DS_LINE )
		{
			CPen pen_w( PS_SOLID, m_drag_w1, drag_color );
			// draw dragged shape
			pDC->SelectObject( &pen_w );
			if( m_drag_style1 == DSS_STRAIGHT )
			{
				if( m_inflection_mode == IM_NONE )
				{
					pDC->MoveTo( m_drag_xi, m_drag_yi );
					pDC->LineTo( m_drag_x, m_drag_y );
				}
				else
				{
					CPoint pi( m_drag_xi, m_drag_yi );
					CPoint pf( m_drag_x, m_drag_y );
					CPoint p = GetInflectionPoint( pi, pf, m_inflection_mode );
					pDC->MoveTo( m_drag_xi, m_drag_yi );
					if( p != pi )
						pDC->LineTo( p.x, p.y );
					pDC->LineTo( m_drag_x, m_drag_y );
				}
				m_last_inflection_mode = m_inflection_mode;
			}
			else if( m_drag_style1 == DSS_ARC_CW )
				DrawArc( pDC, DL_ARC_CW, m_drag_xi, m_drag_yi, m_drag_x, m_drag_y );
			else if( m_drag_style1 == DSS_ARC_CCW )
				DrawArc( pDC, DL_ARC_CCW, m_drag_xi, m_drag_yi, m_drag_x, m_drag_y );
			else if( m_drag_style1 == DSS_CURVE_CW )
				DrawCurve( pDC, DL_CURVE_CW, m_drag_xi, m_drag_yi, m_drag_x, m_drag_y );
			else if( m_drag_style1 == DSS_CURVE_CCW )
				DrawCurve( pDC, DL_CURVE_CCW, m_drag_xi, m_drag_yi, m_drag_x, m_drag_y );
			else
				ASSERT(0);
			if( m_drag_shape == DS_LINE_VERTEX )
			{
				CPen pen( PS_SOLID, m_drag_w2, drag_color );
				pDC->SelectObject( &pen );
				if( m_drag_style2 == DSS_STRAIGHT )
					pDC->LineTo( m_drag_xf, m_drag_yf );
				else if( m_drag_style2 == DSS_ARC_CW )
					DrawArc( pDC, DL_ARC_CW, m_drag_x, m_drag_y, m_drag_xf, m_drag_yf );
				else if( m_drag_style2 == DSS_ARC_CCW )
					DrawArc( pDC, DL_ARC_CCW, m_drag_x, m_drag_y, m_drag_xf, m_drag_yf );
				else if( m_drag_style2 == DSS_CURVE_CW )
					DrawCurve( pDC, DL_CURVE_CW, m_drag_x, m_drag_y, m_drag_xf, m_drag_yf );
				else if( m_drag_style2 == DSS_CURVE_CCW )
					DrawCurve( pDC, DL_CURVE_CCW, m_drag_x, m_drag_y, m_drag_xf, m_drag_yf );
				else
					ASSERT(0);
				pDC->SelectObject( old_pen );
			}
			pDC->SelectObject( old_pen );
	
			// see if leading via needs to be drawn
			if( m_drag_via_drawn )
			{
				// draw or undraw via
				int thick = (m_drag_via_w - m_drag_via_holew)/2;
				int w = m_drag_via_w - thick;
				int holew = m_drag_via_holew;
				CPen pen( PS_SOLID, thick, drag_color );
				CPen * old_pen = pDC->SelectObject( &pen );
				CBrush black_brush( RGB( 0, 0, 0 ) );
				CBrush * old_brush = pDC->SelectObject( &black_brush );
				pDC->Ellipse( m_drag_xi - w/2, m_drag_yi - w/2, m_drag_xi + w/2, m_drag_yi + w/2 );
				pDC->SelectObject( old_brush );
				pDC->SelectObject( old_pen );
			}
	
		}
		else if( m_drag_shape == DS_ARC_STRAIGHT 
			|| m_drag_shape == DS_ARC_CW || m_drag_shape == DS_ARC_CCW 
			|| m_drag_shape == DS_CURVE_CW || m_drag_shape == DS_CURVE_CCW 
			)
		{
			CPen pen_w( PS_SOLID, m_drag_w1, drag_color );
			// redraw dragged shape
			pDC->SelectObject( &pen_w );
			if( m_drag_shape == DS_ARC_STRAIGHT )
				DrawArc( pDC, DL_LINE, m_drag_x, m_drag_y, m_drag_xi, m_drag_yi );
			else if( m_drag_shape == DS_ARC_CW )
				DrawArc( pDC, DL_ARC_CW, m_drag_xi, m_drag_yi, m_drag_x, m_drag_y );
			else if( m_drag_shape == DS_ARC_CCW )
				DrawArc( pDC, DL_ARC_CCW, m_drag_xi, m_drag_yi, m_drag_x, m_drag_y );
			else if( m_drag_shape == DS_CURVE_CW )
				DrawCurve( pDC, DL_CURVE_CW, m_drag_xi, m_drag_yi, m_drag_x, m_drag_y );
			else if( m_drag_shape == DS_CURVE_CCW )
				DrawCurve( pDC, DL_CURVE_CCW, m_drag_xi, m_drag_yi, m_drag_x, m_drag_y );
			pDC->SelectObject( old_pen );	
			m_last_drag_shape = m_drag_shape;
		}
	}
	
	// if cross hairs, draw them
	if( m_cross_hairs )
	{
		// draw cross-hairs
		CPen pen( PS_SOLID, 0, drag_color );
		old_pen = pDC->SelectObject( &pen );
		int x = m_cross_bottom.x;
		int y = m_cross_left.y;
		m_cross_left.x = m_org_x;
		m_cross_right.x = m_max_x;
		m_cross_bottom.y = m_org_y;
		m_cross_top.y = m_max_y;
		if( x-m_org_x > y-m_org_y )
		{
			// bottom-left cursor line intersects m_org_y
			m_cross_botleft.x = x - (y - m_org_y);
			m_cross_botleft.y = m_org_y;
		}
		else
		{
			// bottom-left cursor line intersects m_org_x
			m_cross_botleft.x = m_org_x;
			m_cross_botleft.y = y - (x - m_org_x);
		}
		if( m_max_x-x > y-m_org_y )
		{
			// bottom-right cursor line intersects m_org_y
			m_cross_botright.x = x + (y - m_org_y);
			m_cross_botright.y = m_org_y;
		}
		else
		{
			// bottom-right cursor line intersects m_max_x
			m_cross_botright.x = m_max_x;
			m_cross_botright.y = y - (m_max_x - x);
		}
	
		if( x-m_org_x > m_max_y-y )
		{
			// top-left cursor line intersects m_max_y
			m_cross_topleft.x = x - (m_max_y - y);
			m_cross_topleft.y = m_max_y;
		}
		else
		{
			// top-left cursor line intersects m_org_x
			m_cross_topleft.x = m_org_x;
			m_cross_topleft.y = y + (x - m_org_x);
		}
		if( m_max_x-x > m_max_y-y )
		{
			// top-right cursor line intersects m_max_y
			m_cross_topright.x = x + (m_max_y - y);
			m_cross_topright.y = m_max_y;
		}
		else
		{
			// top-right cursor line intersects m_max_x
			m_cross_topright.x = m_max_x;
			m_cross_topright.y = y + (m_max_x - x);
		}
		pDC->MoveTo( m_cross_left );
		pDC->LineTo( m_cross_right );
		pDC->MoveTo( m_cross_bottom );
		pDC->LineTo( m_cross_top );
		if( m_cross_hairs == 2 )
		{
			pDC->MoveTo( m_cross_topleft );
			pDC->LineTo( m_cross_botright );
			pDC->MoveTo( m_cross_botleft );
			pDC->LineTo( m_cross_topright );
		}
		pDC->SelectObject( old_pen );
		COLORREF bk = pDC->SetBkColor( RGB(0,0,0) );
		pDC->SetBkColor( bk );
	}
	pDC->SetROP2( old_ROP2 );

	// restore original objects
	pDC->SelectObject( old_pen );
	pDC->SelectObject( old_brush );
	
	// double-buffer to screen
	if( topDC )
		dDC->BitBlt(	m_org_x, m_org_y, m_max_x-m_org_x, m_max_y-m_org_y,
					topDC, m_org_x, m_org_y, SRCCOPY ) ;
	else if( botDC )
		dDC->BitBlt(	m_org_x, m_org_y, m_max_x-m_org_x, m_max_y-m_org_y,
								botDC, m_org_x, m_org_y, SRCCOPY ) ;
	if( !draw_layer )
	{
		mem_org_x = m_org_x;
		mem_max_x = m_max_x;
	}
}

// set the display color for a layer
//
void CDisplayList::SetLayerRGB( int layer, int r, int g, int b )
{
	m_rgb[layer][0] = r;
	m_rgb[layer][1] = g;
	m_rgb[layer][2] = b;
}

void CDisplayList::SetLayerVisible( int layer, BOOL vis )
{
	m_vis[layer] = vis;
}

// test x,y for a hit on an item in the selection layer
// creates arrays with layer and id of each hit item
// assigns priority based on layer and id
// then returns pointer to item with highest priority
// If exclude_id != NULL, excludes item with 
// id == exclude_id and ptr == exclude_ptr 
// If include_id != NULL, only include items that match include_id[]
// where n_include_ids is size of array, and
// where 0's in include_id[] fields are treated as wildcards
//
void * CDisplayList::TestSelect( int x, int y, id * sel_id, int * sel_layer, 
								id * exclude_id, void * exclude_ptr, 
								id * include_id, int n_include_ids )
{
	enum {
		MAX_HITS = 2000
	};
	int  nhits = 0;
	int hit_order[MAX_HITS];
	int hit_layer_map[MAX_HITS];
	id hit_id[MAX_HITS];
	void * hit_ptr[MAX_HITS];
	long long S[MAX_HITS];

	int xx = x/m_pcbu_per_wu;
	int yy = y/m_pcbu_per_wu;

	// traverse the list, looking for selection shapes
	dl_element * el = &m_start;
	RECT rgr;
	while( el->next != &m_end )
	{
		el = el->next;	
		if( el->magic )
		{
			if( Get_RectGroup( el, &rgr ) )
			{
				if( x < rgr.left || 
					x > rgr.right ||
					y < rgr.bottom ||
					y > rgr.top )
				{
					el = el->magic;
					continue;
				}
			}
			else if( theApp.m_view_mode == theApp.FOOTPRINT )
			{
			}
			else if( el->id.st != ID_AREA &&  el->id.type != ID_POLYLINE )
			{
				el = el->magic;
				continue;
			}	
		}
		if( !el->map_orig_layer )
			continue;
		if( !getbit( el->layers_bitmap, LAY_SELECTION ) )
			continue;
		// don't select anything invisible
		if( xx<el->rect.left )
			continue;
		if( xx>el->rect.right )
			continue;
		if( yy<el->rect.bottom )
			continue;
		if( yy>el->rect.top )
			continue;
		// don't select anything on an invisible layer
		BOOL Ok = FALSE;
		for( int lay=max(LAY_SELECTION,LAY_HILITE)+1; lay<MAX_LAYERS; lay++ )
		{
			if( getbit( el->map_orig_layer, lay ) && m_vis[lay] )
			{
				Ok = TRUE;
				break;
			}
		}
		if( !Ok )
			continue;
		
		// Ok 
		Ok = FALSE;
		if( el->gtype == DL_RECT || el->gtype == DL_HOLLOW_RECT )
		{
			// OK, hit
			Ok = TRUE;
		}
		else if(	el->gtype == DL_ARC_CW || el->gtype == DL_ARC_CCW || 
					el->gtype == DL_CURVE_CW || el->gtype == DL_CURVE_CCW )
		{
			int tp;
			if( el->gtype == DL_CURVE_CW || el->gtype == DL_ARC_CW )
				tp = CPolyLine::ARC_CW;
			else 
				tp = CPolyLine::ARC_CCW;
			CArray<CPoint> * pts = Get_Points( el, NULL, 0 );
			if( pts->GetSize() >= 2 )
			{
				int w = max( abs(el->el_w)/2, 4.0*m_scale );
				w = max( w, 2 );
				int px=0,py=0;
				int dd = GetClearanceBetweenSegments(	xx-10, yy-10, xx+10, yy+10, CPolyLine::STRAIGHT, 1, 
														(*pts)[0].x, (*pts)[0].y, (*pts)[1].x, (*pts)[1].y, tp, w, (abs(el->el_w)+1)*10, NULL, NULL );
				if( dd == 0 )
					Ok = TRUE;
			}
		}
		else if( el->gtype == DL_RECT_X )
		{
			if( el->pts.GetSize() )
			{
				if( TestPolygon( xx, yy, &el->pts[0], el->pts.GetSize() ) )
					Ok = TRUE;
			}
			else
				Ok = TRUE;
		}
		else if( el->gtype == DL_LINE || el->gtype == DL_HOLLOW_LINE )
		{
			int w = max( abs(el->el_w)/2, 4.0*m_scale );
			w = max( w, 2 );
			int test = TestLineHit( el->pts[0].x, el->pts[0].y, el->pts[1].x, el->pts[1].y, xx, yy, w );
			if( test )
			{
				// OK, hit
				Ok = TRUE;
			}
		}
		else if( el->gtype == DL_HOLLOW_CIRC || el->gtype == DL_CIRC || el->gtype == DL_HOLE )
		{
			int w = (el->rect.right - el->rect.left)/2;
			int x = (el->rect.right + el->rect.left)/2;
			int y = (el->rect.top + el->rect.bottom)/2;
			int d = Distance( x, y, xx, yy );
			if( d < w )
			{
				// OK, hit
				Ok = TRUE;
			}
		}
		else if( el->gtype == DL_POLYGON || el->gtype == DL_POLYLINE )
		{
			if( TestPolygon( xx, yy, &el->pts[0], el->pts.GetSize() ) )
				Ok = TRUE;
		}

		if( !Ok )
			continue;
		
		hit_order[nhits] = MAX_LAYERS-1;
		for( int order=0; order<MAX_LAYERS; order++ )
		{ // set priority
			int layer = m_layer_in_order[order];
			BOOL yes = FALSE;
			if( theApp.m_view_mode == theApp.PCB )
			{
				if( layer == LAY_SM_TOP )
					layer = LAY_TOP_COPPER;
				else if( layer == LAY_SM_BOTTOM )
					layer = LAY_BOTTOM_COPPER;
				//
				if( layer > LAY_DRC_ERROR )
					if( getbit( el->map_orig_layer, layer ) )
						yes = TRUE;
				if( layer > LAY_HILITE )
					if( getbit( el->layers_bitmap, layer ) )
						yes = TRUE;
			}
			else if( theApp.m_view_mode == theApp.FOOTPRINT )
			{
				if( layer > LAY_FP_HILITE )
					if( getbit( el->layers_bitmap, layer ) || getbit( el->map_orig_layer, layer ) )
						yes = TRUE;
			}
			if(yes)
				{
					hit_order[nhits] = order;
					if((layer == LAY_PAD_THRU && theApp.m_view_mode == theApp.PCB ) ||
					   (layer == LAY_FP_PAD_THRU && theApp.m_view_mode == theApp.FOOTPRINT ))
						hit_order[nhits] = m_order_for_layer[m_top_layer];
					break;
				}
		}
		hit_layer_map[nhits] = el->map_orig_layer;
		hit_id[nhits] = el->id;
		hit_ptr[nhits] = el->ptr;
		if( el->gtype == DL_LINE || el->gtype == DL_HOLLOW_LINE )
		{
			long long d = Distance( el->pts[0].x, el->pts[0].y, el->pts[1].x, el->pts[1].y );
			long long w = el->el_w;
			S[nhits] = w*(d+w);
		}
		else
			S[nhits] = (long long)(el->rect.right - el->rect.left)*(long long)(el->rect.top - el->rect.bottom);
		if( nhits < MAX_HITS-1 )
			nhits++;
	}
	// now return highest priority hit
	if( nhits == 0 )
	{
		// no hit
		id no_id;
		*sel_id = no_id;
		*sel_layer = 0;
		return NULL;
	}
	else
	{
		// assign priority to each hit, track maximum, exclude exclude_id item
		int best_hit = nhits;	
		S[nhits] = INT_MAX;
		hit_order[nhits] = MAX_LAYERS-1;
		for( int i=0; i<nhits; i++ )
		{
			BOOL excluded_hit = FALSE;
			BOOL included_hit = TRUE;
			if( exclude_id )
			{
				if( hit_id[i] == *exclude_id && hit_ptr[i] == exclude_ptr )
					excluded_hit = TRUE;
			}
			if( include_id )
			{
				included_hit = FALSE;
				for( int inc=0; inc<n_include_ids; inc++ )
				{
					id * inc_id = &include_id[inc];
					if( inc_id->type == hit_id[i].type
						&& ( inc_id->st == 0 || inc_id->st == hit_id[i].st )
						&& ( inc_id->i == 0 || inc_id->i == hit_id[i].i )
						&& ( inc_id->sst == 0 || inc_id->sst == hit_id[i].sst )
						&& ( inc_id->ii == 0 || inc_id->ii == hit_id[i].ii ) )
					{
						included_hit = TRUE;
						break;
					}
				}
			}
			if( !excluded_hit && included_hit )
			{
				if( hit_order[i] < hit_order[best_hit] )
					best_hit = i;
				else if( hit_order[i] == hit_order[best_hit] && S[i] < S[best_hit] )
					best_hit = i;
			}
		}
		if( best_hit == nhits )
		{
			id no_id;
			*sel_id = no_id;
			*sel_layer = 0;
			return NULL;
		}
		*sel_id = hit_id[best_hit];

		// sel layer
		*sel_layer = m_layer_in_order[hit_order[best_hit]];
		return hit_ptr[best_hit];
	}
}

// Start dragging arrays of drag lines and ratlines
// Assumes that arrays have already been set up using MakeLineArray, etc.
// If no arrays, just drags point
//
int CDisplayList::StartDraggingArray( CDC * pDC, int xx, int yy, int crosshair )
{
	// convert to display units
	int x = xx/m_pcbu_per_wu;
	int y = yy/m_pcbu_per_wu;

	// cancel dragging non-array shape
	m_drag_flag = 0;
	m_drag_shape = 0;

	// set up for dragging array
	m_drag_x = x;			// "grab point"
	m_drag_y = y;
	m_drag_layer = LAY_SELECTION;
	m_drag_angle = 0;
	m_drag_side = 0;
	SetUpCrosshairs( crosshair, x, y );
	
	// done
	return 0;
}

// Start dragging rectangle 
//
int CDisplayList::StartDraggingRectangle( CDC * pDC, int x, int y, int xi, int yi,
										 int xf, int yf, int layer )
{
	// create drag lines
	CPoint p1(xi,yi);
	CPoint p2(xf,yi);
	CPoint p3(xf,yf);
	CPoint p4(xi,yf);
	MakeDragLineArray( 4 );
	AddDragLine( p1, p2 ); 
	AddDragLine( p2, p3 ); 
	AddDragLine( p3, p4 ); 
	AddDragLine( p4, p1 ); 

	StartDraggingArray( pDC, x, y );
	
	// done
	return 0;
}


// Start dragging line 
//
int CDisplayList::StartDraggingRatLine( CDC * pDC, int x, int y, int xi, int yi, 
									   int layer, int w, int crosshair )
{
	// create drag line
	CPoint p1(xi,yi);
	CPoint p2(x,y);
	MakeDragRatlineArray( 1, w );
	AddDragRatline( p1, p2 ); 

	StartDraggingArray( pDC, xi, yi, crosshair );
	
	// done
	return 0;
}

// set style of arc being dragged, using CPolyLine styles
//
void CDisplayList::SetDragArcStyle( int style )
{
	if( style == CPolyLine::STRAIGHT )
		m_drag_shape = DS_ARC_STRAIGHT;
	else if( style == CPolyLine::ARC_CW )
		m_drag_shape = DS_ARC_CW;
	else if( style == CPolyLine::ARC_CCW )
		m_drag_shape = DS_ARC_CCW;
}

// Start dragging arc endpoint, using style from CPolyLine
// Use the layer color and width w
//
int CDisplayList::StartDraggingArc( CDC * pDC, int style, int xx, int yy, int xi, int yi, 
								   int layer, int w, int crosshair )
{
	int x = xx/m_pcbu_per_wu;
	int y = yy/m_pcbu_per_wu;

	// set up for dragging
	m_drag_flag = 1;
	if( style == CPolyLine::STRAIGHT )
		m_drag_shape = DS_ARC_STRAIGHT;
	else if( style == CPolyLine::ARC_CW )
		m_drag_shape = DS_ARC_CW;
	else if( style == CPolyLine::ARC_CCW )
		m_drag_shape = DS_ARC_CCW;
	m_drag_x = x;	// position of endpoint (at cursor)
	m_drag_y = y;
	m_drag_xi = xi/m_pcbu_per_wu;	// start point
	m_drag_yi = yi/m_pcbu_per_wu;
	m_drag_side = 0;
	m_drag_layer_1 = layer;
	m_drag_w1 = w/m_pcbu_per_wu;

	// set up cross hairs
	SetUpCrosshairs( crosshair, x, y );

	//Redraw
//	Draw( pDC );

	// done
	return 0;
}

// Start dragging the selected line endpoint
// Use the layer1 color and width w
//
int CDisplayList::StartDraggingLine( CDC * pDC, int x, int y, int xi, int yi, int layer1, int w,
									int layer_no_via, int via_w, int via_holew,
									int crosshair, int style, int inflection_mode )
{
	// set up for dragging
	m_drag_flag = 1;
	m_drag_shape = DS_LINE;
	m_drag_x = x/m_pcbu_per_wu;	// position of endpoint (at cursor)
	m_drag_y = y/m_pcbu_per_wu;
	m_drag_xi = xi/m_pcbu_per_wu;	// initial vertex
	m_drag_yi = yi/m_pcbu_per_wu;
	m_drag_side = 0;
	m_drag_layer_1 = layer1;
	m_drag_w1 = w/m_pcbu_per_wu;
	m_drag_style1 = style;
	m_drag_layer_no_via = layer_no_via;
	m_drag_via_w = via_w/m_pcbu_per_wu;
	m_drag_via_holew = via_holew/m_pcbu_per_wu;
	m_drag_via_drawn = 0;
	m_inflection_mode = inflection_mode;
	m_last_inflection_mode = IM_NONE;

	// set up cross hairs
	SetUpCrosshairs( crosshair, x, y );

	//Redraw
//	Draw( pDC );

	// done
	return 0;
}

// Start dragging line vertex (i.e. vertex between 2 line segments)
// Use the layer1 color and width w1 for the first segment from (xi,yi) to the vertex,
// Use the layer2 color and width w2 for the second segment from the vertex to (xf,yf)
// While dragging, insert via at start point of first segment if layer1 != layer_no_via
// using via parameters via_w and via_holew
// Note that layer1 may be changed while dragging by ChangeRouting Layer()
// If dir = 1, swap start and end points
//
int CDisplayList::StartDraggingLineVertex( CDC * pDC, int x, int y, 
									int xi, int yi, int xf, int yf,
									int layer1, int layer2, int w1, int w2, int style1, int style2,
									int layer_no_via, int via_w, int via_holew, int dir,
									int crosshair )
{
	// set up for dragging
	m_drag_flag = 1;
	m_drag_shape = DS_LINE_VERTEX;
	m_drag_x = x/m_pcbu_per_wu;	// position of central vertex (at cursor)
	m_drag_y = y/m_pcbu_per_wu;
	if( dir == 0 )
	{
		// routing forward
		m_drag_xi = xi/m_pcbu_per_wu;	// initial vertex
		m_drag_yi = yi/m_pcbu_per_wu;
		m_drag_xf = xf/m_pcbu_per_wu;	// final vertex
		m_drag_yf = yf/m_pcbu_per_wu;
	}
	else
	{
		// routing backward
		m_drag_xi = xf/m_pcbu_per_wu;	// initial vertex
		m_drag_yi = yf/m_pcbu_per_wu;
		m_drag_xf = xi/m_pcbu_per_wu;	// final vertex
		m_drag_yf = yi/m_pcbu_per_wu;
	}
	m_drag_side = 0;
	m_drag_layer_1 = layer1;
	m_drag_layer_2 = layer2;
	m_drag_w1 = w1/m_pcbu_per_wu;
	m_drag_w2 = w2/m_pcbu_per_wu;
	m_drag_style1 = style1;
	m_drag_style2 = style2;
	m_drag_layer_no_via = layer_no_via;
	m_drag_via_w = via_w/m_pcbu_per_wu;
	m_drag_via_holew = via_holew/m_pcbu_per_wu;
	m_drag_via_drawn = 0;

	// set up cross hairs
	SetUpCrosshairs( crosshair, x, y );

	//Redraw
	//Draw( pDC );

	// done
	return 0;
}


// Start dragging line segment (i.e. line segment between 2 vertices)
// Use the layer0 color and width w0 for the "before" segment from (xb,yb) to (xi, yi),
// Use the layer1 color and width w1 for the moving segment from (xi,yi) to (xf, yf),
// Use the layer2 color and width w2 for the ending segment from (xf, yf) to (xe,ye)
// While dragging, insert via at (xi, yi) if layer1 != layer_no_via1, insert via
// at (xf, yf) if layer2 != layer_no_via.
// using via parameters via_w and via_holew
// Note that layer1 may be changed while dragging by ChangeRouting Layer()
// If dir = 1, swap start and end points
//
int CDisplayList::StartDraggingLineSegment( CDC * pDC, int x, int y,
									int xb, int yb,
									int xi, int yi, 
									int xf, int yf,
									int xe, int ye,
									int layer0, int layer1, int layer2,
									int w0,		int w1,		int w2,
									int style0, int style1, int style2,
									
									int layer_no_via, int via_w, int via_holew, 
									int crosshair )
{
	// set up for dragging
	m_drag_flag = 1;
	m_drag_shape = DS_SEGMENT;

	m_drag_x = x/m_pcbu_per_wu;	// position of reference point (at cursor)
	m_drag_y = y/m_pcbu_per_wu;

	m_drag_xb = xb/m_pcbu_per_wu;	// vertex before
	m_drag_yb = yb/m_pcbu_per_wu;
	m_drag_xi = xi/m_pcbu_per_wu;	// initial vertex
	m_drag_yi = yi/m_pcbu_per_wu;
	m_drag_xf = xf/m_pcbu_per_wu;	// final vertex
	m_drag_yf = yf/m_pcbu_per_wu;
	m_drag_xe = xe/m_pcbu_per_wu;	// End vertex
	m_drag_ye = ye/m_pcbu_per_wu;

	m_drag_side = 0;
	m_drag_layer_0 = layer0;
	m_drag_layer_1 = layer1;
	m_drag_layer_2 = layer2;

	m_drag_w0 = w0/m_pcbu_per_wu;
	m_drag_w1 = w1/m_pcbu_per_wu;
	m_drag_w2 = w2/m_pcbu_per_wu;

	m_drag_style0 = style0;
	m_drag_style1 = style1;
	m_drag_style2 = style2;

	m_drag_layer_no_via = layer_no_via;
	m_drag_via_w = via_w/m_pcbu_per_wu;
	m_drag_via_holew = via_holew/m_pcbu_per_wu;
	m_drag_via_drawn = 0;

	// set up cross hairs
	SetUpCrosshairs( crosshair, x, y );

	//Redraw
	Draw( pDC, NULL );

	// done
	return 0;
}

// Drag graphics with cursor 
//
void CDisplayList::Drag( CDC * pDC, int x, int y )
{	
	// convert from PCB to display coords
	int xx = x/m_pcbu_per_wu;
	int yy = y/m_pcbu_per_wu;
	
	// set XOR pen mode for dragging
	int old_ROP2 = pDC->SetROP2( R2_XORPEN );

	//**** there are three dragging modes, which may be used simultaneously ****//
	COLORREF drag_color = RGB( m_rgb[m_drag_layer][0], m_rgb[m_drag_layer][1], m_rgb[m_drag_layer][2] );
	// drag array of lines, used to make complex graphics like a part
	if( m_drag_num_lines )
	{
		CPen drag_pen( PS_SOLID, 1, drag_color );
		CPen * old_pen = pDC->SelectObject( &drag_pen );
		for( int il=0; il<m_drag_num_lines; il++ )
		{
			// undraw
			pDC->MoveTo( m_drag_x+m_drag_line_pt[2*il].x, m_drag_y+m_drag_line_pt[2*il].y );
			pDC->LineTo( m_drag_x+m_drag_line_pt[2*il+1].x, m_drag_y+m_drag_line_pt[2*il+1].y );
			// redraw
			pDC->MoveTo( xx+m_drag_line_pt[2*il].x, yy+m_drag_line_pt[2*il].y );
			pDC->LineTo( xx+m_drag_line_pt[2*il+1].x, yy+m_drag_line_pt[2*il+1].y );
		}
		pDC->SelectObject( old_pen );
	}

	// drag array of rubberband lines, used for ratlines to dragged part
	if( m_drag_num_ratlines )
	{
		CPen drag_pen( PS_SOLID, 1, drag_color );
		CPen * old_pen = pDC->SelectObject( &drag_pen );
		for( int il=0; il<m_drag_num_ratlines; il++ )
		{
			// undraw
			pDC->MoveTo( m_drag_ratline_start_pt[il].x, m_drag_ratline_start_pt[il].y );
			pDC->LineTo( m_drag_x+m_drag_ratline_end_pt[il].x, m_drag_y+m_drag_ratline_end_pt[il].y );
			// draw
			pDC->MoveTo( m_drag_ratline_start_pt[il].x, m_drag_ratline_start_pt[il].y );
			pDC->LineTo( xx+m_drag_ratline_end_pt[il].x, yy+m_drag_ratline_end_pt[il].y );
		}
		pDC->SelectObject( old_pen );
	}

	// draw special shapes, used for polyline sides and trace segments
	if( m_drag_flag )
	{
		if( m_drag_shape == DS_LINE_VERTEX || m_drag_shape == DS_LINE )
		{
			// drag rubberband trace segment, or vertex between two rubberband segments
			// used for routing traces
			CPen pen_w( PS_SOLID, m_drag_w1, drag_color );

			// undraw first segment
			CPen * old_pen = pDC->SelectObject( &pen_w );
			{
				if( m_drag_style1 == DSS_STRAIGHT )
				{
					if( m_last_inflection_mode == IM_NONE )
					{
						pDC->MoveTo( m_drag_xi, m_drag_yi );
						pDC->LineTo( m_drag_x, m_drag_y );
					}
					else
					{
						CPoint pi( m_drag_xi, m_drag_yi );
						CPoint pf( m_drag_x, m_drag_y );
						CPoint p = GetInflectionPoint( pi, pf, m_last_inflection_mode );
						pDC->MoveTo( m_drag_xi, m_drag_yi );
						if( p != pi )
							pDC->LineTo( p.x, p.y );
						pDC->LineTo( m_drag_x, m_drag_y );
					}
				}
				else if( m_drag_style1 == DSS_ARC_CW )
					DrawArc( pDC, DL_ARC_CW, m_drag_xi, m_drag_yi, m_drag_x, m_drag_y );
				else if( m_drag_style1 == DSS_ARC_CCW )
					DrawArc( pDC, DL_ARC_CCW, m_drag_xi, m_drag_yi, m_drag_x, m_drag_y );
				else if( m_drag_style1 == DSS_CURVE_CW )
					DrawCurve( pDC, DL_CURVE_CW, m_drag_xi, m_drag_yi, m_drag_x, m_drag_y );
				else if( m_drag_style1 == DSS_CURVE_CCW )
					DrawCurve( pDC, DL_CURVE_CCW, m_drag_xi, m_drag_yi, m_drag_x, m_drag_y );
				else 
					ASSERT(0);
				if( m_drag_shape == DS_LINE_VERTEX )
				{
					// undraw second segment
					CPen pen( PS_SOLID, m_drag_w2, drag_color );
					CPen * old_pen = pDC->SelectObject( &pen );
					if( m_drag_style2 == DSS_STRAIGHT )
						pDC->LineTo( m_drag_xf, m_drag_yf );
					else if( m_drag_style2 == DSS_ARC_CW )
						DrawArc( pDC, DL_ARC_CW, m_drag_x, m_drag_y, m_drag_xf, m_drag_yf );
					else if( m_drag_style2 == DSS_ARC_CCW )
						DrawArc( pDC, DL_ARC_CCW, m_drag_x, m_drag_y, m_drag_xf, m_drag_yf );
					else if( m_drag_style2 == DSS_CURVE_CW )
						DrawCurve( pDC, DL_CURVE_CW, m_drag_x, m_drag_y, m_drag_xf, m_drag_yf );
					else if( m_drag_style2 == DSS_CURVE_CCW )
						DrawCurve( pDC, DL_CURVE_CCW, m_drag_x, m_drag_y, m_drag_xf, m_drag_yf );
					else
						ASSERT(0);
					pDC->SelectObject( old_pen );
				}

				// draw first segment
				if( m_drag_style1 == DSS_STRAIGHT ) 
				{
					if( m_inflection_mode == IM_NONE )
					{
						pDC->MoveTo( m_drag_xi, m_drag_yi );
						pDC->LineTo( xx, yy );
					}
					else
					{
						CPoint pi( m_drag_xi, m_drag_yi );
						CPoint pf( xx, yy );
						CPoint p = GetInflectionPoint( pi, pf, m_inflection_mode );
						pDC->MoveTo( m_drag_xi, m_drag_yi );
						if( p != pi )
							pDC->LineTo( p.x, p.y );
						pDC->LineTo( xx, yy );
					}
					m_last_inflection_mode = m_inflection_mode;
				}
				else if( m_drag_style1 == DSS_ARC_CW )
					DrawArc( pDC, DL_ARC_CW, m_drag_xi, m_drag_yi, xx, yy );
				else if( m_drag_style1 == DSS_ARC_CCW )
					DrawArc( pDC, DL_ARC_CCW, m_drag_xi, m_drag_yi, xx, yy );
				else if( m_drag_style1 == DSS_CURVE_CW )
					DrawCurve( pDC, DL_CURVE_CW, m_drag_xi, m_drag_yi, xx, yy );
				else if( m_drag_style1 == DSS_CURVE_CCW )
					DrawCurve( pDC, DL_CURVE_CCW, m_drag_xi, m_drag_yi, xx, yy );
				else
					ASSERT(0);
				if( m_drag_shape == DS_LINE_VERTEX )
				{
					// draw second segment
					CPen pen( PS_SOLID, m_drag_w2, drag_color );
					CPen * old_pen = pDC->SelectObject( &pen );
					if( m_drag_style2 == DSS_STRAIGHT )
						pDC->LineTo( m_drag_xf, m_drag_yf );
					else if( m_drag_style2 == DSS_ARC_CW )
						DrawArc( pDC, DL_ARC_CW, xx, yy, m_drag_xf, m_drag_yf );
					else if( m_drag_style2 == DSS_ARC_CCW )
						DrawArc( pDC, DL_ARC_CCW, xx, yy, m_drag_xf, m_drag_yf );
					else if( m_drag_style2 == DSS_CURVE_CW )
						DrawArc( pDC, DL_ARC_CW, xx, yy, m_drag_xf, m_drag_yf );
					else if( m_drag_style2 == DSS_CURVE_CCW )
						DrawArc( pDC, DL_ARC_CCW, xx, yy, m_drag_xf, m_drag_yf );
					else
						ASSERT(0);
					pDC->SelectObject( old_pen );
				}

				// see if leading via needs to be changed
				/*if( ( (m_drag_layer_no_via != 0 && m_drag_layer_1 != m_drag_layer_no_via) && !m_drag_via_drawn )
					|| (!(m_drag_layer_no_via != 0 && m_drag_layer_1 != m_drag_layer_no_via) && m_drag_via_drawn ) )
				{
					// draw or undraw via
					int thick = (m_drag_via_w - m_drag_via_holew)/2;
					int w = m_drag_via_w - thick;
					int holew = m_drag_via_holew;
//					CPen pen( PS_SOLID, thick, RGB( m_rgb[LAY_PAD_THRU][0], m_rgb[LAY_PAD_THRU][1], m_rgb[LAY_PAD_THRU][2] ) );
					CPen pen( PS_SOLID, thick, RGB( m_rgb[m_drag_layer_1][0], m_rgb[m_drag_layer_1][1], m_rgb[m_drag_layer_1][2] ) );
					CPen * old_pen = pDC->SelectObject( &pen );
					{
						CBrush black_brush( RGB( 0, 0, 0 ) );
						CBrush * old_brush = pDC->SelectObject( &black_brush );
						pDC->Ellipse( m_drag_xi - w/2, m_drag_yi - w/2, m_drag_xi + w/2, m_drag_yi + w/2 );
						pDC->SelectObject( old_brush );
					}
					pDC->SelectObject( old_pen );
					m_drag_via_drawn = 1 - m_drag_via_drawn;
				}*/	
			}
			pDC->SelectObject( old_pen );
		}
		// move a trace segment
		else if( m_drag_shape == DS_SEGMENT )
		{
			ASSERT(m_drag_style0 == DSS_STRAIGHT);
			pDC->MoveTo( m_drag_xb, m_drag_yb );

			// undraw first segment
			CPen pen0( PS_SOLID, m_drag_w0, drag_color );
			CPen * old_pen = pDC->SelectObject( &pen0 );
			pDC->LineTo( m_drag_xi, m_drag_yi );

			// undraw second segment
			ASSERT(m_drag_style1 == DSS_STRAIGHT);
			CPen pen1( PS_SOLID, m_drag_w1, drag_color );
			pDC->SelectObject( &pen1 );
			pDC->LineTo( m_drag_xf, m_drag_yf );

			// undraw third segment
			if(m_drag_style2 == DSS_STRAIGHT)		// Could also be DSS_NONE (this segment only)
			{
				CPen pen2( PS_SOLID, m_drag_w2, drag_color );
				pDC->SelectObject( &pen2 );
				pDC->LineTo( m_drag_xe, m_drag_ye );
			}
			pDC->SelectObject( old_pen );

			// Adjust the two vertices, (xi, yi) and (xf, yf) based on the movement of xx and yy
			//  relative to m_drag_x and m_drag_y:

			// 1. Move the endpoints of (xi, yi), (xf, yf) of the line by the mouse movement. This
			//		is just temporary, since the final ending position is determined by the intercept
			//		points with the leading and trailing segments:
			int new_xi = m_drag_xi + xx - m_drag_x;			// Check sign here.
			int new_yi = m_drag_yi + yy - m_drag_y;

			int new_xf = m_drag_xf + xx - m_drag_x;
			int new_yf = m_drag_yf + yy - m_drag_y;

			int old_xb_dir = sign(m_drag_xi - m_drag_xb);
			int old_yb_dir = sign(m_drag_yi - m_drag_yb);

			int old_xi_dir = sign(m_drag_xf - m_drag_xi);
			int old_yi_dir = sign(m_drag_yf - m_drag_yi);

			int old_xe_dir = sign(m_drag_xe - m_drag_xf);
			int old_ye_dir = sign(m_drag_ye - m_drag_yf);

			// 2. Find the intercept between the extended segment in motion and the leading segment.
			double d_new_xi;
			double d_new_yi;
			FindLineIntersection(m_drag_xb, m_drag_yb, m_drag_xi, m_drag_yi,
									new_xi,    new_yi,	   new_xf,    new_yf,
									&d_new_xi, &d_new_yi);
			int i_drag_xi = floor(d_new_xi + .5);
			int i_drag_yi = floor(d_new_yi + .5);

			// 3. Find the intercept between the extended segment in motion and the trailing segment:
			int i_drag_xf, i_drag_yf;
			if(m_drag_style2 == DSS_STRAIGHT)
			{
				double d_new_xf;
				double d_new_yf;
				FindLineIntersection(new_xi,    new_yi,	   new_xf,    new_yf,
										m_drag_xf, m_drag_yf, m_drag_xe, m_drag_ye,
										&d_new_xf, &d_new_yf);

				i_drag_xf = floor(d_new_xf + .5);
				i_drag_yf = floor(d_new_yf + .5);
			} else {
				i_drag_xf = new_xf;
				i_drag_yf = new_yf;
			}
			
			// If we drag too far, the line segment can reverse itself causing a little triangle to form.
			//   That's a bad thing.
			if(sign(i_drag_xf - i_drag_xi) == old_xi_dir && sign(i_drag_yf - i_drag_yi) == old_yi_dir &&
			   sign(i_drag_xi - m_drag_xb) == old_xb_dir && sign(i_drag_yi - m_drag_yb) == old_yb_dir &&
			   sign(m_drag_xe - i_drag_xf) == old_xe_dir && sign(m_drag_ye - i_drag_yf) == old_ye_dir   )
			{
				m_drag_xi = i_drag_xi;
				m_drag_yi = i_drag_yi;
				m_drag_xf = i_drag_xf;
				m_drag_yf = i_drag_yf;
			}
			else
			{
				xx = m_drag_x;
				yy = m_drag_y;
			}

			// 4. Redraw the three segments:
			{
				pDC->MoveTo( m_drag_xb, m_drag_yb );

				// draw first segment
				CPen pen0( PS_SOLID, m_drag_w0, drag_color );
				CPen * old_pen = pDC->SelectObject( &pen0 );
				pDC->LineTo( m_drag_xi, m_drag_yi );

				// draw second segment
				CPen pen1( PS_SOLID, m_drag_w1, drag_color );
				pDC->SelectObject( &pen1 );
				pDC->LineTo( m_drag_xf, m_drag_yf );

				if(m_drag_style2 == DSS_STRAIGHT)
				{
					// draw third segment
					CPen pen2( PS_SOLID, m_drag_w2, drag_color );
					pDC->SelectObject( &pen2 );
					pDC->LineTo( m_drag_xe, m_drag_ye );
				}
				pDC->SelectObject( old_pen );
			}
		}
		else if( (m_drag_shape == DS_ARC_STRAIGHT 
		|| m_drag_shape == DS_ARC_CW || m_drag_shape == DS_ARC_CCW
		|| m_drag_shape == DS_CURVE_CW || m_drag_shape == DS_CURVE_CCW) )
		{
			CPen pen_w( PS_SOLID, m_drag_w1, drag_color );
	
			// undraw old arc
			CPen * old_pen = pDC->SelectObject( &pen_w );
			{
				if( m_last_drag_shape == DS_ARC_STRAIGHT )
					DrawArc( pDC, DL_LINE, m_drag_x, m_drag_y, m_drag_xi, m_drag_yi );
				else if( m_last_drag_shape == DS_ARC_CW )
					DrawArc( pDC, DL_ARC_CW, m_drag_xi, m_drag_yi, m_drag_x, m_drag_y );
				else if( m_last_drag_shape == DS_ARC_CCW )
					DrawArc( pDC, DL_ARC_CCW, m_drag_xi, m_drag_yi, m_drag_x, m_drag_y );
				else if( m_last_drag_shape == DS_CURVE_CW )
					DrawCurve( pDC, DL_CURVE_CW, m_drag_xi, m_drag_yi, m_drag_x, m_drag_y );
				else if( m_last_drag_shape == DS_CURVE_CCW )
					DrawCurve( pDC, DL_CURVE_CCW, m_drag_xi, m_drag_yi, m_drag_x, m_drag_y );
	
				// draw new arc
				if( m_drag_shape == DS_ARC_STRAIGHT )
					DrawArc( pDC, DL_LINE, xx, yy, m_drag_xi, m_drag_yi );
				else if( m_drag_shape == DS_ARC_CW )
					DrawArc( pDC, DL_ARC_CW, m_drag_xi, m_drag_yi, xx, yy );
				else if( m_drag_shape == DS_ARC_CCW )
					DrawArc( pDC, DL_ARC_CCW, m_drag_xi, m_drag_yi, xx, yy );
				else if( m_drag_shape == DS_CURVE_CW )
					DrawCurve( pDC, DL_CURVE_CW, m_drag_xi, m_drag_yi, xx, yy );
				else if( m_drag_shape == DS_CURVE_CCW )
					DrawCurve( pDC, DL_CURVE_CCW, m_drag_xi, m_drag_yi, xx, yy );
				m_last_drag_shape = m_drag_shape;	// used only for polylines
			}
			pDC->SelectObject( old_pen );
		}
	}
	// remember shape data
	m_drag_x = xx;
	m_drag_y = yy;
	
	// now undraw and redraw cross-hairs, if necessary
	if( m_cross_hairs )
	{
		int x = xx;
		int y = yy;
		CPen _pen( PS_SOLID, 1, drag_color );
		CPen * old_pen = pDC->SelectObject( &_pen );
		pDC->MoveTo( m_cross_left );
		pDC->LineTo( m_cross_right );
		pDC->MoveTo( m_cross_bottom );
		pDC->LineTo( m_cross_top );
		if( m_cross_hairs == 2 )
		{
			pDC->MoveTo( m_cross_topleft );
			pDC->LineTo( m_cross_botright );
			pDC->MoveTo( m_cross_botleft );
			pDC->LineTo( m_cross_topright );
		}
		m_cross_left.x = m_org_x;
		m_cross_left.y = y;
		m_cross_right.x = m_max_x;
		m_cross_right.y = y;
		m_cross_bottom.x = x;
		m_cross_bottom.y = m_org_y;
		m_cross_top.x = x;
		m_cross_top.y = m_max_y;
		if( x-m_org_x > y-m_org_y )
		{
			// bottom-left cursor line intersects m_org_y
			m_cross_botleft.x = x - (y - m_org_y);
			m_cross_botleft.y = m_org_y;
		}
		else
		{
			// bottom-left cursor line intersects m_org_x
			m_cross_botleft.x = m_org_x;
			m_cross_botleft.y = y - (x - m_org_x);
		}
		if( m_max_x-x > y-m_org_y )
		{
			// bottom-right cursor line intersects m_org_y
			m_cross_botright.x = x + (y - m_org_y);
			m_cross_botright.y = m_org_y;
		}
		else
		{
			// bottom-right cursor line intersects m_max_x
			m_cross_botright.x = m_max_x;
			m_cross_botright.y = y - (m_max_x - x);
		}

		if( x-m_org_x > m_max_y-y )
		{
			// top-left cursor line intersects m_max_y
			m_cross_topleft.x = x - (m_max_y - y);
			m_cross_topleft.y = m_max_y;
		}
		else
		{
			// top-left cursor line intersects m_org_x
			m_cross_topleft.x = m_org_x;
			m_cross_topleft.y = y + (x - m_org_x);
		}
		if( m_max_x-x > m_max_y-y )
		{
			// top-right cursor line intersects m_max_y
			m_cross_topright.x = x + (m_max_y - y);
			m_cross_topright.y = m_max_y;
		}
		else
		{
			// top-right cursor line intersects m_max_x
			m_cross_topright.x = m_max_x;
			m_cross_topright.y = y + (m_max_x - x);
		}
		pDC->MoveTo( m_cross_left );
		pDC->LineTo( m_cross_right );
		pDC->MoveTo( m_cross_bottom );
		pDC->LineTo( m_cross_top );
		if( m_cross_hairs == 2 )
		{
			pDC->MoveTo( m_cross_topleft );
			pDC->LineTo( m_cross_botright );
			pDC->MoveTo( m_cross_botleft );
			pDC->LineTo( m_cross_topright );
		}
		//pDC->SelectObject( old_pen );
		COLORREF bk = pDC->SetBkColor( RGB(0,0,0) );
		pDC->SetBkColor( bk );
	}

	// restore drawing mode
	pDC->SetROP2( old_ROP2 );

	return;
}

// Stop dragging 
//
int CDisplayList::StopDragging()
{
	m_drag_flag = 0;
	m_drag_num_lines = 0;
	m_drag_num_ratlines = 0;
	m_cross_hairs = 0;
	m_last_drag_shape = DS_NONE;
	return 0;
}

// Change the drag layer and/or width for DS_LINE_VERTEX
// if ww = 0, don't change width
//
void CDisplayList::ChangeRoutingLayer( CDC * pDC, int layer1, int layer2, int ww )
{
	int w = ww;
	if( !w )
		w = m_drag_w1*m_pcbu_per_wu;
	int old_ROP2 = pDC->GetROP2();
	pDC->SetROP2( R2_XORPEN );
	
	if( m_drag_shape == DS_LINE_VERTEX )
	{
		CPen pen_old( PS_SOLID, 1, RGB( m_rgb[m_drag_layer_2][0], 
						m_rgb[m_drag_layer_2][1], m_rgb[m_drag_layer_2][2] ) );
		CPen pen_old_w( PS_SOLID, m_drag_w1, RGB( m_rgb[m_drag_layer_1][0], 
						m_rgb[m_drag_layer_1][1], m_rgb[m_drag_layer_1][2] ) );
		CPen pen( PS_SOLID, 1, RGB( m_rgb[layer2][0], 
						m_rgb[layer2][1], m_rgb[layer2][2] ) );
		CPen pen_w( PS_SOLID, w/m_pcbu_per_wu, RGB( m_rgb[layer1][0], 
						m_rgb[layer1][1], m_rgb[layer1][2] ) );

		// undraw segments
		CPen * old_pen = pDC->SelectObject( &pen_old_w );
		pDC->MoveTo( m_drag_xi, m_drag_yi );
		pDC->LineTo( m_drag_x, m_drag_y );
		pDC->SelectObject( &pen_old );
		pDC->LineTo( m_drag_xf, m_drag_yf );
		
		// redraw segments
		pDC->SelectObject( &pen_w );
		pDC->MoveTo( m_drag_xi, m_drag_yi );
		pDC->LineTo( m_drag_x, m_drag_y );
		pDC->SelectObject( &pen );
		pDC->LineTo( m_drag_xf, m_drag_yf );								
								
		// update variables
		m_drag_layer_1 = layer1;
		m_drag_layer_2 = layer2;
		m_drag_w1 = w/m_pcbu_per_wu;

		// see if leading via needs to be changed
		/*if( ( (m_drag_layer_no_via != 0 && m_drag_layer_1 != m_drag_layer_no_via) && !m_drag_via_drawn )
			|| (!(m_drag_layer_no_via != 0 && m_drag_layer_1 != m_drag_layer_no_via) && m_drag_via_drawn ) )
		{
			// draw or undraw via
			//int thick = (m_drag_via_w - m_drag_via_holew)/2;
			//int w = m_drag_via_w - thick;
			//int holew = m_drag_via_holew;
			//CPen pen( PS_SOLID, thick, RGB( m_rgb[m_drag_layer_1][0], m_rgb[m_drag_layer_1][1], m_rgb[m_drag_layer_1][2] ) );
			//CPen * old_pen = pDC->SelectObject( &pen );
			//CBrush black_brush( RGB( 0, 0, 0 ) );
			//CBrush * old_brush = pDC->SelectObject( &black_brush );
			//pDC->Ellipse( m_drag_xi - w/2, m_drag_yi - w/2, m_drag_xi + w/2, m_drag_yi + w/2 );
			//pDC->SelectObject( old_brush );
			//pDC->SelectObject( old_pen );
			//m_drag_via_drawn = 1 - m_drag_via_drawn;
		}*/
	}
	else if( m_drag_shape == DS_LINE )
	{
		CPen pen_old_w( PS_SOLID, m_drag_w1, RGB( m_rgb[m_drag_layer_1][0], 
						m_rgb[m_drag_layer_1][1], m_rgb[m_drag_layer_1][2] ) );
		CPen pen( PS_SOLID, 1, RGB( m_rgb[layer2][0], 
						m_rgb[layer2][1], m_rgb[layer2][2] ) );
		CPen pen_w( PS_SOLID, w/m_pcbu_per_wu, RGB( m_rgb[layer1][0], 
						m_rgb[layer1][1], m_rgb[layer1][2] ) );

		// undraw segments
		CPen * old_pen = pDC->SelectObject( &pen_old_w );
		pDC->MoveTo( m_drag_xi, m_drag_yi );
		pDC->LineTo( m_drag_x, m_drag_y );
		
		// redraw segments
		pDC->SelectObject( &pen_w );
		pDC->MoveTo( m_drag_xi, m_drag_yi );
		pDC->LineTo( m_drag_x, m_drag_y );
								
		// update variables
		m_drag_layer_1 = layer1;
		m_drag_w1 = w/m_pcbu_per_wu;

		// see if leading via needs to be changed
		/*if( ( (m_drag_layer_no_via != 0 && m_drag_layer_1 != m_drag_layer_no_via) && !m_drag_via_drawn )
			|| (!(m_drag_layer_no_via != 0 && m_drag_layer_1 != m_drag_layer_no_via) && m_drag_via_drawn ) )
		{
			// draw or undraw via
			//int thick = (m_drag_via_w - m_drag_via_holew)/2;
			//int w = m_drag_via_w - thick;
			//int holew = m_drag_via_holew;
			//CPen pen( PS_SOLID, thick, RGB( m_rgb[m_drag_layer_1][0], m_rgb[m_drag_layer_1][1], m_rgb[m_drag_layer_1][2] ) );
			//CPen * old_pen = pDC->SelectObject( &pen );
			//CBrush black_brush( RGB( 0, 0, 0 ) );
			//CBrush * old_brush = pDC->SelectObject( &black_brush );
			//pDC->Ellipse( m_drag_xi - w/2, m_drag_yi - w/2, m_drag_xi + w/2, m_drag_yi + w/2 );
			//pDC->SelectObject( old_brush );
			//pDC->SelectObject( old_pen );
			//m_drag_via_drawn = 1 - m_drag_via_drawn;
		}*/

		pDC->SelectObject( old_pen );
	}
	
	// restore drawing mode
	pDC->SetROP2( old_ROP2 );

	return;
}

// change angle by adding 90 degrees clockwise
//
void CDisplayList::IncrementDragAngle( CDC * pDC )
{
	m_drag_angle = (m_drag_angle + 90) % 360;

	CPoint zero(0,0);

	CPen drag_pen( PS_SOLID, 1, RGB( m_rgb[m_drag_layer][0], 
					m_rgb[m_drag_layer][1], m_rgb[m_drag_layer][2] ) );
	CPen *old_pen = pDC->SelectObject( &drag_pen );

	int old_ROP2 = pDC->GetROP2();
	pDC->SetROP2( R2_XORPEN );
	
	// erase lines
	for( int il=0; il<m_drag_num_lines; il++ )
	{
		pDC->MoveTo( m_drag_x+m_drag_line_pt[2*il].x, m_drag_y+m_drag_line_pt[2*il].y );
		pDC->LineTo( m_drag_x+m_drag_line_pt[2*il+1].x, m_drag_y+m_drag_line_pt[2*il+1].y );
	}
	for( int il=0; il<m_drag_num_ratlines; il++ )
	{
		pDC->MoveTo( m_drag_ratline_start_pt[il].x, m_drag_ratline_start_pt[il].y );
		pDC->LineTo( m_drag_x+m_drag_ratline_end_pt[il].x, m_drag_y+m_drag_ratline_end_pt[il].y );
	}
		
	// rotate points, redraw lines
	for( int il=0; il<m_drag_num_lines; il++ )
	{
		RotatePoint( &m_drag_line_pt[2*il], -90, zero );
		RotatePoint( &m_drag_line_pt[2*il+1], -90, zero );
		pDC->MoveTo( m_drag_x+m_drag_line_pt[2*il].x, m_drag_y+m_drag_line_pt[2*il].y );
		pDC->LineTo( m_drag_x+m_drag_line_pt[2*il+1].x, m_drag_y+m_drag_line_pt[2*il+1].y );
	}
	for( int il=0; il<m_drag_num_ratlines; il++ )
	{
		RotatePoint( &m_drag_ratline_end_pt[il], -90, zero );
		pDC->MoveTo( m_drag_ratline_start_pt[il].x, m_drag_ratline_start_pt[il].y );
		pDC->LineTo( m_drag_x+m_drag_ratline_end_pt[il].x, m_drag_y+m_drag_ratline_end_pt[il].y );
	}

	pDC->SelectObject( old_pen );
	pDC->SetROP2( old_ROP2 );
}

// flip to opposite side of board
//
void CDisplayList::FlipDragSide( CDC * pDC )
{
	m_drag_side = 1 - m_drag_side;

	CPen drag_pen( PS_SOLID, 1, RGB( m_rgb[m_drag_layer][0], 
					m_rgb[m_drag_layer][1], m_rgb[m_drag_layer][2] ) );
	CPen *old_pen = pDC->SelectObject( &drag_pen );

	int old_ROP2 = pDC->GetROP2();
	pDC->SetROP2( R2_XORPEN );

	// erase lines
	for( int il=0; il<m_drag_num_lines; il++ )
	{
		pDC->MoveTo( m_drag_x+m_drag_line_pt[2*il].x, m_drag_y+m_drag_line_pt[2*il].y );
		pDC->LineTo( m_drag_x+m_drag_line_pt[2*il+1].x, m_drag_y+m_drag_line_pt[2*il+1].y );
	}
	for( int il=0; il<m_drag_num_ratlines; il++ )
	{
		pDC->MoveTo( m_drag_ratline_start_pt[il].x, m_drag_ratline_start_pt[il].y );
		pDC->LineTo( m_drag_x+m_drag_ratline_end_pt[il].x, m_drag_y+m_drag_ratline_end_pt[il].y );
	}

	// modify drag lines
	for( int il=0; il<m_drag_num_lines; il++ )
	{
		m_drag_line_pt[2*il].x = -m_drag_line_pt[2*il].x;
		m_drag_line_pt[2*il+1].x = -m_drag_line_pt[2*il+1].x;
	}
	for( int il=0; il<m_drag_num_ratlines; il++ )
	{
		m_drag_ratline_end_pt[il].x = -m_drag_ratline_end_pt[il].x;
	}

	// redraw lines
	for( int il=0; il<m_drag_num_lines; il++ )
	{
		pDC->MoveTo( m_drag_x+m_drag_line_pt[2*il].x, m_drag_y+m_drag_line_pt[2*il].y );
		pDC->LineTo( m_drag_x+m_drag_line_pt[2*il+1].x, m_drag_y+m_drag_line_pt[2*il+1].y );
	}
	for( int il=0; il<m_drag_num_ratlines; il++ )
	{
		pDC->MoveTo( m_drag_ratline_start_pt[il].x, m_drag_ratline_start_pt[il].y );
		pDC->LineTo( m_drag_x+m_drag_ratline_end_pt[il].x, m_drag_y+m_drag_ratline_end_pt[il].y );
	}

	pDC->SelectObject( old_pen );
	pDC->SetROP2( old_ROP2 );
}

// get any changes in side which occurred while dragging
//
int CDisplayList::GetDragSide()
{
	return m_drag_side;
}

// get any changes in angle which occurred while dragging
//
int CDisplayList::GetDragAngle()
{
	return m_drag_angle;
}

int CDisplayList::MakeDragLineArray( int num_lines )
{
	if( m_drag_line_pt )
		free( m_drag_line_pt );
	m_drag_line_pt = (CPoint*)calloc( 2*num_lines, sizeof(CPoint) );
	if( !m_drag_line_pt )
		return 1;

	m_drag_max_lines = num_lines;
	m_drag_num_lines = 0;
	return 0;
}

int CDisplayList::MakeDragRatlineArray( int num_ratlines, int width )
{
	if( m_drag_ratline_start_pt )
		free(m_drag_ratline_start_pt );
	m_drag_ratline_start_pt = (CPoint*)calloc( num_ratlines, sizeof(CPoint) );
	if( !m_drag_ratline_start_pt )
		return 1;

	if( m_drag_ratline_end_pt )
		free(m_drag_ratline_end_pt );
	m_drag_ratline_end_pt = (CPoint*)calloc( num_ratlines, sizeof(CPoint) );
	if( !m_drag_ratline_end_pt )
		return 1;

	m_drag_ratline_width = width;
	m_drag_max_ratlines = num_ratlines;
	m_drag_num_ratlines = 0;
	return 0;
}

int CDisplayList::AddDragLine( CPoint pi, CPoint pf )
{
	if( m_drag_num_lines >= m_drag_max_lines )
		return  1;

	m_drag_line_pt[2*m_drag_num_lines].x = pi.x/m_pcbu_per_wu;
	m_drag_line_pt[2*m_drag_num_lines].y = pi.y/m_pcbu_per_wu;
	m_drag_line_pt[2*m_drag_num_lines+1].x = pf.x/m_pcbu_per_wu;
	m_drag_line_pt[2*m_drag_num_lines+1].y = pf.y/m_pcbu_per_wu;
	m_drag_num_lines++;
	return 0;
}

int CDisplayList::AddDragRatline( CPoint pi, CPoint pf )
{
	if( m_drag_num_ratlines == m_drag_max_ratlines )
		return  1;

	m_drag_ratline_start_pt[m_drag_num_ratlines].x = pi.x/m_pcbu_per_wu;
	m_drag_ratline_start_pt[m_drag_num_ratlines].y = pi.y/m_pcbu_per_wu;
	m_drag_ratline_end_pt[m_drag_num_ratlines].x = pf.x/m_pcbu_per_wu;
	m_drag_ratline_end_pt[m_drag_num_ratlines].y = pf.y/m_pcbu_per_wu;
	m_drag_num_ratlines++;
	return 0;
}

int CDisplayList::CancelHighLight( BOOL IncludeStatic )
{
	dl_element * el = m_end.prev;
	while ( el != &m_start )
	{
		el = el->prev;
		if (el->next->el_static == 0)
		{
			clrbit( el->next->layers_bitmap, LAY_HILITE );
			el->next->transparent = 0;
			if( el->next->layers_bitmap == 0 )
				Remove(el->next);
		}
		else if( IncludeStatic )
		{
			if( el->next->transparent == TRANSPARENT_HILITE )
			{
				clrbit( el->next->layers_bitmap, LAY_HILITE );
				el->next->transparent = 0;
				Remove(el->next);
			}
			else
				el->next->transparent = TRANSPARENT_HILITE;
		}
	}		
	return 0;
}

// Set the device context and memory context to world coords
//
void CDisplayList::SetDCToWorldCoords( CDC * pDC, CDC * bDC, CDC * tDC, int pcbu_org_x, int pcbu_org_y )
{
	botDC = NULL;
	topDC = NULL;
	if( pDC )
	{
		// set window scale (WU per pixel) and origin (WU)
		pDC->SetMapMode( MM_ANISOTROPIC );
		pDC->SetWindowExt( w_ext_x, w_ext_y );
		pDC->SetWindowOrg( pcbu_org_x/m_pcbu_per_wu, pcbu_org_y/m_pcbu_per_wu );

		// set viewport to client rect with origin in lower left
		// leave room for m_left_pane to the left of the PCB drawing area
		pDC->SetViewportExt( v_ext_x, v_ext_y );
		pDC->SetViewportOrg( m_pane_org_x, m_pane_org_y );
	}
	if( bDC->m_hDC )
	{
		// set window scale (units per pixel) and origin (units)
		bDC->SetMapMode( MM_ANISOTROPIC );
		bDC->SetWindowExt( w_ext_x, w_ext_y );
		bDC->SetWindowOrg( pcbu_org_x/m_pcbu_per_wu, pcbu_org_y/m_pcbu_per_wu );

		// set viewport to client rect with origin in lower left
		// leave room for m_left_pane to the left of the PCB drawing area
		bDC->SetViewportExt( v_ext_x, v_ext_y );
		bDC->SetViewportOrg( m_pane_org_x, m_pane_org_y );
		
		// update pointer
		botDC = bDC;
	}
	if( tDC )
		if( tDC->m_hDC )
		{
			// set window scale (units per pixel) and origin (units)
			tDC->SetMapMode( MM_ANISOTROPIC );
			tDC->SetWindowExt( w_ext_x, w_ext_y );
			tDC->SetWindowOrg( pcbu_org_x/m_pcbu_per_wu, pcbu_org_y/m_pcbu_per_wu );
	
			// set viewport to client rect with origin in lower left
			// leave room for m_left_pane to the left of the PCB drawing area
			tDC->SetViewportExt( v_ext_x, v_ext_y );
			tDC->SetViewportOrg( m_pane_org_x, m_pane_org_y );
			
			// update pointer
			topDC = tDC;
		}
}


void CDisplayList::SetVisibleGrid( BOOL on, double grid )
{
	m_visual_grid_on = on;
	m_visual_grid_spacing = grid/m_pcbu_per_wu;
}


void CDisplayList::SetStatic( int m )
{
	dl_element * el = m_end.prev;
	while ( el != &m_start )
	{
		el = el->prev;
		if( getbit( el->next->layers_bitmap, LAY_HILITE ) && el->next->el_static == 0 )
		{
			dl_element * cp = Cpy( el->next );
			cp->layers_bitmap = 0;
			cp->el_static = m;
			if( cp->id.type != ID_TEXT )
				cp->transparent = TRANSPARENT_BACKGND;
			HighLight( cp );
		}
	}
}

void CDisplayList::SetUpCrosshairs( int type, int x, int y )
{
	// set up cross hairs
	m_cross_hairs = type;
	m_cross_left.x = m_org_x;
	m_cross_left.y = y;
	m_cross_right.x = m_max_x;
	m_cross_right.y = y;
	m_cross_bottom.x = x;
	m_cross_bottom.y = m_org_y;
	m_cross_top.x = x;
	m_cross_top.y = m_max_y;
	if( x-m_org_x > y-m_org_y )
	{
		// bottom-left cursor line intersects m_org_y
		m_cross_botleft.x = x - (y - m_org_y);
		m_cross_botleft.y = m_org_y;
	}
	else
	{
		// bottom-left cursor line intersects m_org_x
		m_cross_botleft.x = m_org_x;
		m_cross_botleft.y = y - (x - m_org_x);
	}
	if( m_max_x-x > y-m_org_y )
	{
		// bottom-right cursor line intersects m_org_y
		m_cross_botright.x = x + (y - m_org_y);
		m_cross_botright.y = m_org_y;
	}
	else
	{
		// bottom-right cursor line intersects m_max_x
		m_cross_botright.x = m_max_x;
		m_cross_botright.y = y - (m_max_x - x);
	}

	if( x-m_org_x > m_max_y-y )
	{
		// top-left cursor line intersects m_max_y
		m_cross_topleft.x = x - (m_max_y - y);
		m_cross_topleft.y = m_max_y;
	}
	else
	{
		// top-left cursor line intersects m_org_x
		m_cross_topleft.x = m_org_x;
		m_cross_topleft.y = y - (x - m_org_x);
	}
	if( m_max_x-x > m_max_y-y )
	{
		// top-right cursor line intersects m_max_y
		m_cross_topright.x = x + (m_max_y - y);
		m_cross_topright.y = m_max_y;
	}
	else
	{
		// top-right cursor line intersects m_max_x
		m_cross_topright.x = m_max_x;
		m_cross_topright.y = y + (m_max_x - x);
	}
}

// Convert point in window coords to PCB units (i.e. nanometers)
//
CPoint CDisplayList::WindowToPCB( CPoint point )
{
	CPoint p;     
	double test = ((point.x-m_pane_org_x)*m_wu_per_pixel_x + m_org_x)*m_pcbu_per_wu;
	p.x = test;    
	test = ((point.y-m_pane_org_y)*m_wu_per_pixel_y + m_org_y)*m_pcbu_per_wu;
	p.y = test;
	return p;
}

// Convert point in screen coords to PCB units
//
CPoint CDisplayList::ScreenToPCB( CPoint point )
{
	CPoint p;
	p.x = point.x - m_screen_r.left;
	p.y = point.y - m_screen_r.top;
	p = WindowToPCB( p );
	return p;
}

// Convert point in PCB units to screen coords
//
CPoint CDisplayList::PCBToScreen( CPoint point )
{
	CPoint p;
	p.x = (point.x - m_org_x*m_pcbu_per_wu)/m_pcbu_per_pixel_x+m_pane_org_x+m_screen_r.left;
	p.y = (point.y - m_org_y*m_pcbu_per_wu)/m_pcbu_per_pixel_y-m_bottom_pane_h+m_screen_r.bottom; 
	return p;
}



// 
//
RECT CDisplayList::GetWindowRect()
{
	RECT wr;
	wr.left =	m_org_x;
	wr.right =	m_max_x; 
	wr.top =	m_max_y;
	wr.bottom = m_org_y; 
	return wr;
}

