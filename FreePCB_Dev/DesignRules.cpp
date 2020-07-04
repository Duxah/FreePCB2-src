// Implementation of class DRErrorList
//
#include "stdafx.h"

//******************************************************
//
// class DRError
//
// constructor
//
DRError::DRError()
{
	dl_el = NULL;
	dl_sel = NULL;
}

DRError::~DRError()
{
	if( dl_el )
	{
		dl_el->dlist->Remove( dl_el );
		dl_sel->dlist->Remove( dl_sel );
	}
}



//******************************************************
//
// class DRErrorList
//
// constructor
//
DRErrorList::DRErrorList()
{
	m_dlist = NULL;
}

DRErrorList::~DRErrorList()
{
	Clear();
}

// Set the DisplayList for error symbols
//
void DRErrorList::SetLists( CPartList * pl, CNetList * nl, CDisplayList * dl, CTextList * tl, CArray<CPolyLine>* poly )
{
	m_plist = pl;
	m_nlist = nl;
	m_dlist = dl;
	m_tlist = tl;
	m_poly = poly;
}

// clear all entries from DRE list
//
void DRErrorList::Clear()
{
	while( !list.IsEmpty() )
	{
		void * ptr = list.GetHead();
		DRError * dre = (DRError*)ptr;
		delete dre;
		list.RemoveHead();
	}
}

// Remove entry from list
//
void DRErrorList::Remove( DRError * dre )
{
	POSITION pos = list.Find( dre );
	if( pos )
	{
		list.RemoveAt( pos );
		delete dre;
	}
}

// Add error to list
//
DRError * DRErrorList::Add( long index, int type, CString * str, 
						   CString * name1, CString * name2, id id1, id id2,
						   int x1, int y1, int x2, int y2, int w, int layer )
{ 
	POSITION pos;
	void * ptr;

	// find center of coords
	int x = x1;
	int y = y1;
	if( name2 )
	{
		x = (x1 + x2)/2;
		y = (y1 + y2)/2;
	}
	
#define DRE_MIN_SIZE 50*NM_PER_MIL 
	DRError * dre = new DRError;// add
	// make id
	dre->m_id.type = ID_DRC;
	dre->m_id.st = ID_DRE;
	dre->m_id.i = (unsigned int)dre;
	dre->m_id.sst = type;
	dre->m_id.ii = index;
	// set rest of params
	if( name1 )
		dre->name1 = *name1;
	if( name2 )
		dre->name2 = *name2;
	dre->str = *str;
	dre->id1 = id1;
	dre->id2 = id2;
	dre->x = x;
	dre->y = y;
	dre->layer = layer;
	if( m_dlist )
	{
		int d = DRE_MIN_SIZE/2;
		if( name2 )
			d = max( DRE_MIN_SIZE, Distance( x1, y1, x2, y2 ) )/2;
		if( w )
			d = max( d, w/2 );
		int l_map = 0;
		setbit( l_map, LAY_DRC_ERROR );
		RECT rdre;
		rdre.left =   x-d;
		rdre.right =  x+d;
		rdre.bottom = y-d;
		rdre.top =    y+d;
		dre->dl_el = m_dlist->Add(	dre->m_id, (void*)dre, l_map, DL_HOLLOW_CIRC, 1, 
									&rdre, d,  NULL, 0 ); 
		dre->m_id.st = ID_DRE;
		dre->dl_sel = m_dlist->AddSelector( dre->m_id, (void*)dre, DL_HOLLOW_CIRC, 1, 
											&rdre, d, NULL, 0, l_map ); 
	}
	list.AddTail( dre );
#undef DRE_MIN_SIZE
	return dre;
}

// returns the graphic element for pad
// if none, returns the element for the pad's hole
//
dl_element * GetPadElement( part_pin * pin, int layer )
{
	for( int il=pin->dl_els.GetSize()-1; il>=0; il-- )
	{
		if( pin->dl_els[il] )
			if( getbit( pin->dl_els[il]->layers_bitmap, layer ) )
				return pin->dl_els[il];
	}
	return pin->dl_hole;
}

// Highlight the error in the layout window
//
void DRErrorList::HighLight( DRError * dre )
{
	setbit( dre->dl_el->layers_bitmap, LAY_HILITE );
	setbit( dre->dl_el->map_orig_layer, LAY_DRC_ERROR );
	dl_element * dl1 = NULL; 
	dl_element * dl2 = NULL;
	if( dre->id1.type == ID_PART )
	{
		cpart * part = m_plist->GetPart( dre->name1 );
		if( part )
		{
			if( dre->id1.st == ID_PAD )
				if( dre->id1.i < part->pin.GetSize() )
					dl1 = GetPadElement( &part->pin[dre->id1.i], dre->layer );
			if( dre->id1.st == ID_OUTLINE )
				if ( dre->id1.i < part->m_outline_stroke.GetSize() )
					dl1 = part->m_outline_stroke[dre->id1.i];
		}
	}
	if( dre->id2.type == ID_PART )
	{
		cpart * part = m_plist->GetPart( dre->name2 );
		if( part )
		{
			if( dre->id2.st == ID_PAD )
				if( dre->id2.i < part->pin.GetSize() )
					dl2 = GetPadElement( &part->pin[dre->id2.i], dre->layer );
			if( dre->id2.st == ID_OUTLINE )
				if ( dre->id2.i < part->m_outline_stroke.GetSize() )
					dl2 = part->m_outline_stroke[dre->id2.i];
		}
	}
	if( dre->id1.type == ID_TEXT )
	{
		CText * text = m_tlist->GetText( dre->id1.i, dre->id1.ii );
		if( text )
			dl1 = text->dl_el;
	}
	if( dre->id2.type == ID_TEXT )
	{
		CText * text = m_tlist->GetText( dre->id2.i, dre->id2.ii );
		if( text )
			dl2 = text->dl_el;
	}
	if( dre->id1.type == ID_NET )
	{
		cnet * net1 = m_nlist->GetNetPtrByName( &dre->name1 );
		if( net1 )
		{
			if( dre->id1.st == ID_AREA )
				if ( dre->id1.i < net1->nareas )
					if ( dre->id1.ii < net1->area[dre->id1.i].poly->GetNumCorners() )
						dl1 = net1->area[dre->id1.i].poly->dl_side[dre->id1.ii];
			if( dre->id1.st == ID_CONNECT )
				if ( dre->id1.i < net1->nconnects )
					if ( dre->id1.ii < net1->connect[dre->id1.i].nsegs )
						dl1 = net1->connect[dre->id1.i].seg[dre->id1.ii].dl_el; 
		}
	}
	if( dre->id2.type == ID_NET )
	{
		cnet * net2 = m_nlist->GetNetPtrByName( &dre->name2 );
		if( net2 )
		{
			if( dre->id2.st == ID_AREA )
				if ( dre->id2.i < net2->nareas )
					if ( dre->id2.ii < net2->area[dre->id2.i].poly->GetNumCorners() )
						dl2 = net2->area[dre->id2.i].poly->dl_side[dre->id2.ii];
			if( dre->id2.st == ID_CONNECT )
				if ( dre->id2.i < net2->nconnects )
					if ( dre->id2.ii < net2->connect[dre->id2.i].nsegs )
						dl2 = net2->connect[dre->id2.i].seg[dre->id2.ii].dl_el; 
		}
	}
	if( dre->id1.type == ID_POLYLINE )
	{
		if( dre->id1.i < m_poly->GetSize() )
		{
			CPolyLine * p = &(m_poly->GetAt(dre->id1.i));
			if( dre->id1.ii < p->GetNumSides() )
				dl1 = p->dl_side[dre->id1.ii];
		}
	}
	if( dre->id2.type == ID_POLYLINE )
	{
		if( dre->id2.i < m_poly->GetSize() )
		{
			CPolyLine * p = &(m_poly->GetAt(dre->id2.i));
			if( dre->id2.ii < p->GetNumSides() )
				dl2 = p->dl_side[dre->id2.ii];
		}
	}
	if( m_dlist )
	{
		if( dl1 )
			m_dlist->HighLight( dl1 );
		if( dl2 )
			m_dlist->HighLight( dl2 );
	}
}

// Make symbol for the error a solid circle with diameter = 250 mils
//
void DRErrorList::MakeSolidCircles()
{
	POSITION pos;
	void * ptr;

	if( !list.IsEmpty() )
	{
		for( pos = list.GetHeadPosition(); pos != NULL; )
		{
			ptr = list.GetNext( pos );
			DRError * dre = (DRError*)ptr;
			if( dre->dl_el )
			{
				if( dre->dl_el->gtype == DL_HOLLOW_CIRC )
				{
					dre->dl_el->gtype = DL_CIRC;
					RECT Get;
					RECT * r = m_dlist->Get_Rect( dre->dl_el, &Get );
					dre->dl_el->el_w = r->right - r->left;
					SwellRect( r, 999 );
				}
			}
		}
	}
}

// Make symbol for error a ring 
//
void DRErrorList::MakeHollowCircles()
{
	POSITION pos;
	void * ptr;

	if( !list.IsEmpty() )
	{
		for( pos = list.GetHeadPosition(); pos != NULL; )
		{
			ptr = list.GetNext( pos );
			DRError * dre = (DRError*)ptr;
			if( dre->dl_el )
			{
				if( dre->dl_el->gtype == DL_CIRC )
				{
					dre->dl_el->gtype = DL_HOLLOW_CIRC;
					RECT Get;
					RECT * r = m_dlist->Get_Rect( dre->dl_el, &Get );
					int x = (r->right + r->left)/2;
					int y = (r->top + r->bottom)/2;
					r->left =	x - dre->dl_el->el_w/2;
					r->right =  x + dre->dl_el->el_w/2;
					r->bottom =	y - dre->dl_el->el_w/2;
					r->top =		y + dre->dl_el->el_w/2;
				}
			}
		}
	}
}
