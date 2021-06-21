// NetList.cpp: implementation of the CNetList class.
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
#include <math.h>
#include <stdlib.h>
#include "DlgMyMessageBox.h"
#include "gerber.h"
#include "utility.h"
#include "php_polygon.h"
#include "Cuid.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//#define PROFILE		// profiles calls to OptimizeConnections() for "GND"  
BOOL bDontShowSelfIntersectionWarning = FALSE;
BOOL bDontShowSelfIntersectionArcsWarning = FALSE;
//BOOL bDontShowIntersectionWarning = FALSE;
BOOL bDontShowIntersectionArcsWarning = FALSE;


// carea constructor
carea::carea()
{
	m_dlist = 0;
	npins = 0;
	nvias = 0;
	poly = 0;
	utility = 0;
	utility2 = 0;
	selected = 0;
}

void carea::Initialize( CDisplayList * dlist )
{
	m_dlist = dlist;
	poly = new CPolyLine( m_dlist );//ok
}

// carea destructor
carea::~carea()
{
	if( m_dlist )
	{
		for( int ip=0; ip<npins; ip++ )
			m_dlist->Remove( dl_thermal[ip] );
		for( int is=0; is<nvias; is++ )
			m_dlist->Remove( dl_via_thermal[is] );
	}
	delete poly;
}

// carea copy constructor 
// doesn't actually copy anything but required for CArray<carea,carea>.InsertAt()
carea::carea( const carea& s )
{
	npins = 0;
	nvias = 0;
	poly = new CPolyLine( m_dlist );//ok
}

// carea assignment operator
// doesn't actually assign but required for CArray<carea,carea>.InsertAt to work
carea &carea::operator=( carea &a )
{
	return *this;
}

CNetList::CNetList( CDisplayList * dlist, CPartList * plist )
{
	m_dlist = dlist;			// attach display list
	m_plist = plist;			// attach part list
	m_pos_i = -1;				// intialize index to iterators
	m_bSMT_connect = FALSE;
}

void CNetList::SetPartList( CPartList * plist )
{
	m_plist = plist;
}

CNetList::~CNetList()
{
	RemoveAllNets();
}

// Add new net to netlist
//
cnet * CNetList::AddNet( CString name, int def_w, int def_via_w, int def_via_hole_w )
{
	// create new net
	cnet * new_net = new cnet( m_dlist );//ok

	// set array sizes
	new_net->pin.SetSize( 0 );
	new_net->area.SetSize( 0 );
	new_net->connect.SetSize( 0 );
	new_net->selected = 0;
	// zero 
	new_net->nconnects = 0;
	new_net->nareas = 0;
	new_net->npins = 0;
	// set default trace width
	new_net->def_w = def_w;
	new_net->def_via_w = def_via_w;
	new_net->def_via_hole_w = def_via_hole_w;

	// create id and set name
	id id( ID_NET, 0 );
	new_net->id = id;
	new_net->name = name;

	// visible by default
	new_net->visible = 1;

	// add name and pointer to map
	m_map.SetAt( name, (void*)new_net );

	return new_net;
} 


// Remove net from list
//
void CNetList::RemoveNet( cnet * net )
{
	if( m_plist )
	{
		// remove pointers to net from pins on part
		for( int ip=0; ip<net->npins; ip++ )
		{
			cpart * pin_part = net->pin[ip].part;
			if( pin_part )
			{
				CShape * s = pin_part->shape; 
				if( s )
				{
					for( int pin_index=s->GetPinIndexByName(net->pin[ip].pin_name,-1); pin_index>=0; pin_index=s->GetPinIndexByName(net->pin[ip].pin_name,pin_index) )
						if( pin_index < pin_part->pin.GetSize() )
							pin_part->pin[pin_index].net = NULL;
				}
			}
		}
	}
	// destroy arrays
	for( int ii=0; ii<net->connect.GetSize(); ii++ )
		UndrawConnection(net,ii);
	net->connect.RemoveAll();
	net->pin.RemoveAll();
	for( int ii=0; ii<net->area.GetSize(); ii++ )
		net->area[ii].poly->Undraw();
	net->area.RemoveAll();
	m_map.RemoveKey( net->name );
	delete( net );
}

// remove all nets from netlist
//
void CNetList::RemoveAllNets()
{
	// remove all nets
   POSITION pos;
   CString name;
   void * ptr;
   for( pos = m_map.GetStartPosition(); pos != NULL; )
   {
		m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		RemoveNet( net );
   }
   m_map.RemoveAll();
}

// Get first net in list, or NULL if no nets
//
cnet * CNetList::GetFirstNet()
{
	CString name;
	void * ptr;
	// test for no nets
	if( m_map.GetSize() == 0 )
		return NULL;
	// increment iterator and get first net
	m_pos_i++;
	if( m_pos_i >= MAX_ITERATORS )
	{
		AfxMessageBox("  Fatal overflow - CNetList::GetFirstNet().cpp( m_pos_i >= MAX_ITERATORS )"); // fatal overflow
		m_pos_i--;
		return NULL;
	}
	m_pos[m_pos_i] = m_map.GetStartPosition(); 
	if( m_pos != NULL )
	{
		m_map.GetNextAssoc( m_pos[m_pos_i], name, ptr );
		cnet * net = (cnet*)ptr;
		if( net == NULL )
			ASSERT(0);
		return net;
	}
	else
		return NULL;
}

// Get next net in list
//
cnet * CNetList::GetNextNet()
{
	CString name;
	void * ptr;

	if( m_pos[m_pos_i] == NULL )
	{
		m_pos_i--;
		return NULL;
	}
	else
	{
		m_map.GetNextAssoc( m_pos[m_pos_i], name, ptr );
		cnet * net = (cnet*)ptr;
		if( net == NULL )
			ASSERT(0);
		return net;
	}
}

// Cancel loop on next net
//
void CNetList::CancelNextNet()
{
		m_pos_i--;
}

cnet * CNetList::SplitNet( cnet * net, int iconnect, int STEP )
{
	cconnect *c = &net->connect[iconnect];
	if( c->end_pin == cconnect::NO_END )
		return NULL;
	cnet * nnew;
	static CString newname;

	// reset flags
	if( STEP == 0 )
	{
		CString parse_name;
		ParseRef( &net->name, &parse_name );
		int net_i = 2;
		newname.Format( "%s%d", parse_name, net_i );
		while( GetNetPtrByName( &newname ) )
		{
			net_i++;
			newname.Format( "%s%d", parse_name, net_i );
		}
		for( int ip=0; ip>net->npins; ip++ )
			net->pin[ip].utility = 0;
		for( int ic=0; ic>net->nconnects; ic++ )
			c->utility = 0;
		for( int ia=0; ia>net->nareas; ia++ )
			net->area[ia].utility = 0;
		int start_pin = c->start_pin;
		net->pin[start_pin].utility = 1;
		nnew = AddNet( newname, net->def_w, net->def_via_w, net->def_via_hole_w );
		AddNetPin( nnew, &net->pin[start_pin].ref_des, &net->pin[start_pin].pin_name, 0 );
	}
	else
		nnew = GetNetPtrByName( &newname );

	// test
	if( nnew == NULL )
		ASSERT(0);

	// go
	BOOL f, ftee, bREPEAT=0;
	int fic, fiv;
	cconnect * hc;
	for( int ic=0; ic<net->nconnects; ic++ )
	{
		if( ic == iconnect )
			continue;
		c = &net->connect[ic];
		if( c->utility )
			continue;
		//
		f = FALSE;
		ftee = FALSE;
		fic=0, fiv=0;
		hc = NULL;
		//
		if( net->pin[c->start_pin].utility )
			f = TRUE;
		if( c->end_pin != cconnect::NO_END )
			if( net->pin[c->end_pin].utility )
				f = TRUE;
		if( c->vtx[c->nsegs].tee_ID )
		{	
			if( FindTeeVertexInNet( net, c->vtx[c->nsegs].tee_ID, &fic, &fiv ) )
			{
				if( f )
					ftee = TRUE;
				hc = &net->connect[fic];
				if( net->pin[hc->start_pin].utility )
					ftee = TRUE;
				if( hc->end_pin != cconnect::NO_END )
					if( net->pin[hc->end_pin].utility )
						ftee = TRUE;
			}
		}
		if(f || ftee)
		{
			bREPEAT = TRUE;
			if( net->pin[c->start_pin].utility == 0 )
			{
				net->pin[c->start_pin].utility = 1;
				AddNetPin( nnew, &net->pin[c->start_pin].ref_des, &net->pin[c->start_pin].pin_name, 0 );
			}
			if( c->end_pin != cconnect::NO_END )
				if( net->pin[c->end_pin].utility == 0 )
				{
					net->pin[c->end_pin].utility = 1;
					AddNetPin( nnew, &net->pin[c->end_pin].ref_des, &net->pin[c->end_pin].pin_name, 0 );
				}
			if( ftee )
			{
				if( net->pin[hc->start_pin].utility == 0 )
				{
					net->pin[hc->start_pin].utility = 1;
					AddNetPin( nnew, &net->pin[hc->start_pin].ref_des, &net->pin[hc->start_pin].pin_name, 0 );
				}
				if( hc->end_pin != cconnect::NO_END )
					if( net->pin[hc->end_pin].utility == 0 )
					{
						net->pin[hc->end_pin].utility = 1;
						AddNetPin( nnew, &net->pin[hc->end_pin].ref_des, &net->pin[hc->end_pin].pin_name, 0 );
					}
				//
				if( hc->utility == 0 )
				{
					int new_ic;
					if( hc->end_pin != cconnect::NO_END )
					{
						int pin1 = GetNetPinIndex( nnew, &net->pin[hc->start_pin].ref_des, &net->pin[hc->start_pin].pin_name );
						int pin2 = GetNetPinIndex( nnew, &net->pin[hc->end_pin].ref_des,   &net->pin[hc->end_pin].pin_name );
						new_ic = AddNetConnect( nnew, pin1, pin2, hc->vtx[0].x, hc->vtx[0].y, hc->vtx[hc->nsegs].x, hc->vtx[hc->nsegs].y );
						ChangeSegmentLayer( nnew, new_ic, 0, hc->seg[hc->nsegs-1].layer, 0, 0 );
						SetSegmentWidth( nnew, new_ic, 0, hc->seg[hc->nsegs-1].width, 0, 0 );
						for( int ii=hc->nsegs-1; ii>0; ii-- )
						{
							InsertSegment( nnew, new_ic, 0, hc->vtx[ii].x, hc->vtx[ii].y, hc->seg[ii-1].layer, hc->seg[ii-1].width, hc->vtx[ii].via_w, hc->vtx[ii].via_hole_w, 0, 0 );
							nnew->connect[new_ic].vtx[1].via_w = hc->vtx[ii].via_w;
							nnew->connect[new_ic].vtx[1].via_hole_w = hc->vtx[ii].via_hole_w;
							nnew->connect[new_ic].vtx[1].tee_ID = hc->vtx[ii].tee_ID;
						}
						hc->utility = 1;
					}
					else
					{
						int pin1 = GetNetPinIndex( nnew, &net->pin[hc->start_pin].ref_des, &net->pin[hc->start_pin].pin_name );
						new_ic = AddNetStub( nnew, pin1, hc->vtx[0].x, hc->vtx[0].y );
						for( int ii=1; ii<=hc->nsegs; ii++ )
						{
							AppendSegment( nnew, new_ic, hc->vtx[ii].x, hc->vtx[ii].y, hc->seg[ii-1].layer, hc->seg[ii-1].width, hc->vtx[ii].via_w, hc->vtx[ii].via_hole_w, 1 );
							nnew->connect[new_ic].vtx[ii].tee_ID = hc->vtx[ii].tee_ID;
							int fvf = hc->vtx[ii].force_via_flag;
							nnew->connect[new_ic].vtx[ii].force_via_flag = fvf;
						}
						hc->utility = 1;
					}
					DrawConnection( nnew, new_ic );
				}
			}
			if( c->utility == 0 )
			{
				int new_ic;
				if( c->end_pin != cconnect::NO_END )
				{
					int pin1 = GetNetPinIndex( nnew, &net->pin[c->start_pin].ref_des, &net->pin[c->start_pin].pin_name );
					int pin2 = GetNetPinIndex( nnew, &net->pin[c->end_pin].ref_des,   &net->pin[c->end_pin].pin_name );
					new_ic = AddNetConnect( nnew, pin1, pin2, c->vtx[0].x, c->vtx[0].y, c->vtx[c->nsegs].x, c->vtx[c->nsegs].y );					
					for( int ii=c->nsegs-1; ii>0; ii-- )
					{
						InsertSegment( nnew, new_ic, 0, c->vtx[ii].x, c->vtx[ii].y, c->seg[ii-1].layer, c->seg[ii-1].width, c->vtx[ii].via_w, c->vtx[ii].via_hole_w, 0, 0 );
						nnew->connect[new_ic].vtx[1].via_w = c->vtx[ii].via_w;
						nnew->connect[new_ic].vtx[1].via_hole_w = c->vtx[ii].via_hole_w;
						nnew->connect[new_ic].vtx[1].tee_ID = c->vtx[ii].tee_ID;
					}
					ChangeSegmentLayer( nnew, new_ic, c->nsegs-1, c->seg[c->nsegs-1].layer, 0, 0 );
					SetSegmentWidth( nnew, new_ic, c->nsegs-1, c->seg[c->nsegs-1].width, 0, 0 );
					c->utility = 1;
				}
				else
				{
					int pin1 = GetNetPinIndex( nnew, &net->pin[c->start_pin].ref_des, &net->pin[c->start_pin].pin_name );
					new_ic = AddNetStub( nnew, pin1, c->vtx[0].x, c->vtx[0].y );
					for( int ii=1; ii<=c->nsegs; ii++ )
					{
						AppendSegment( nnew, new_ic, c->vtx[ii].x, c->vtx[ii].y, c->seg[ii-1].layer, c->seg[ii-1].width, c->vtx[ii].via_w, c->vtx[ii].via_hole_w, 1 );
						nnew->connect[new_ic].vtx[ii].tee_ID = c->vtx[ii].tee_ID;
						int fvf = c->vtx[ii].force_via_flag;
						nnew->connect[new_ic].vtx[ii].force_via_flag = fvf;
					}
					c->utility = 1;
				}
				DrawConnection( nnew, new_ic );
			}
		}
	}
	int cntpins, cntvias=0;
	for( int ia=0; ia<net->nareas; ia++ )
	{
		if( net->area[ia].utility )
			continue;
		cntpins = 0;
		for( int ii=0; ii<net->area[ia].npins; ii++ )
		{
			int ip = net->area[ia].pin[ii];
			if( net->pin[ip].utility )
			{
				cntpins += net->area[ia].npins;
				break;
			}
		}
		cntvias = 0;
		for( int ii=0; ii<net->area[ia].nvias; ii++ )
		{
			if( net->connect[net->area[ia].vcon[ii]].utility )
				cntvias += 1;
		}
		if( (cntpins >= net->area[ia].npins/2 && cntpins) || (cntvias >= net->area[ia].nvias/2 && cntvias) )
		{
			bREPEAT = TRUE;
			net->area[ia].utility = 1;
			int anew = AddArea( nnew, net->area[ia].poly->GetLayer(), net->area[ia].poly->GetX(0), net->area[ia].poly->GetY(0), net->area[ia].poly->GetHatch() );
			int maxc = net->area[ia].poly->GetNumContours();
			for( int nc=0; nc<maxc; nc++ )
			{
				int st = net->area[ia].poly->GetContourStart(nc);
				int end = net->area[ia].poly->GetContourEnd(nc);
				for( int ii=max(1,st); ii<=end; ii++ )
					AppendAreaCorner( nnew, anew, net->area[ia].poly->GetX(ii), net->area[ia].poly->GetY(ii), net->area[ia].poly->GetSideStyle(ii-1), 0 );
				nnew->area[anew].poly->Close( net->area[ia].poly->GetSideStyle(end));
			}
			int nc = net->area[ia].poly->GetNumCorners();
			nnew->area[anew].poly->SetW( net->area[ia].poly->GetW() );
			CompleteArea( nnew, anew, net->area[ia].poly->GetSideStyle(nc-1));	
			nnew->area[anew].poly->Draw( nnew->area[anew].poly->GetDisplayList() );
			for( int ii=0; ii<net->area[ia].npins; ii++ )
			{
				int ip = net->area[ia].pin[ii];
				if( net->pin[ip].utility == 0 )
				{
					net->pin[ip].utility = 1;
					AddNetPin( nnew, &net->pin[ip].ref_des, &net->pin[ip].pin_name, 0 );
				}
			}
			for( int ii=0; ii<net->area[ia].nvias; ii++ )
			{
				int iconn = net->area[ia].vcon[ii];
				int pin1 = net->connect[iconn].start_pin;
				if( net->pin[pin1].utility == 0 )
				{	
					net->pin[pin1].utility = 1;
					AddNetPin( nnew, &net->pin[pin1].ref_des, &net->pin[pin1].pin_name, 0 );
				}
			}
		}
	}

	// repeat while pins found
	if( bREPEAT )
		SplitNet( net, iconnect, STEP+1);

	if( STEP == 0 )
	{
		// removing pins, traces, areas
		RemoveNetConnect( net, iconnect, FALSE );
		for( int ii=net->nareas-1; ii>=0; ii-- )
			if( net->area[ii].utility )
				RemoveArea( net, ii );
		for( int ii=net->nconnects-1; ii>=0; ii-- )
			if( net->connect[ii].utility )
			{
				UndrawConnection(net,ii);
				net->connect.RemoveAt( ii );
			}
		net->nconnects = net->connect.GetSize();
		RenumberConnections( net );
		for( int ii=net->npins-1; ii>=0; ii-- )
			if( net->pin[ii].utility )
				RemoveNetPin( net, ii, 1, FALSE );
		SetAreaConnections( net );
		SetAreaConnections( nnew );
	}
	return nnew;
}
// set utility parameter of all nets
//
void CNetList::MarkAllNets( int utility )
{
	cnet * net = GetFirstNet();
	while( net != NULL )
	{
		net->utility = utility;
		net->utility2 = utility;
		for( int ip=0; ip<net->npins; ip++ )
			net->pin[ip].utility = utility;
		for( int ic=0; ic<net->nconnects; ic++ )
		{
			cconnect * c = &net->connect[ic];
			c->utility = utility;
			for( int is=0; is<c->nsegs+1; is++ )
			{
				if( is < c->nsegs )
					c->seg[is].utility = utility;
				c->vtx[is].utility = utility;
				c->vtx[is].utility2 = utility;
			}
		}
		for( int ia=0; ia<net->nareas; ia++ )
		{
			carea * a = &net->area[ia];
			a->utility = utility;
			a->poly->SetUtility( utility );
			for( int is=0; is<a->poly->GetNumSides(); is++ )
			{
				a->poly->SetUtility( is, utility );
			}
		}
		net = GetNextNet(/*LABEL*/);
	}
}

// move origin of coordinate system
//
void CNetList::MoveOrigin( int x_off, int y_off )
{
	// remove all nets
	POSITION pos;
	CString name;
	void * ptr;
	for( pos = m_map.GetStartPosition(); pos != NULL; )
	{
		m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		for( int ic=0; ic<net->nconnects; ic++ )
		{
			cconnect * c =  &net->connect[ic];
			UndrawConnection( net, ic );
			for( int iv=0; iv<=c->nsegs; iv++ )
			{
				cvertex * v = &c->vtx[iv];
				v->x += x_off;
				v->y += y_off;
			}
			DrawConnection( net, ic );
		}
		for( int ia=0; ia<net->area.GetSize(); ia++ )
		{
			carea * a = &net->area[ia];
			a->poly->MoveOrigin( x_off, y_off );
			SetAreaConnections( net, ia );
		}
	}
}

// Undraw all of the grelements of a connection
// If m_dlist == 0, do nothing
//
void CNetList::UndrawConnection( cnet * net, int ic )
{
	if( m_dlist )
	{
		cconnect * c = &net->connect[ic];
		int nsegs = c->nsegs;
		int nvtx = nsegs + 1;
		for( int is=0; is<nsegs; is++ )
		{
			cseg * s = &c->seg[is];
			if( s->dl_el )
				m_dlist->Remove( s->dl_el );
			s->dl_el = NULL;
		}
		for( int iv=0; iv<nvtx; iv++ )
		{
			UndrawVia( net, ic, iv );
		}
	}
}

// Draw all of the grelements of a connection
// If m_dlist == 0, do nothing
//
void CNetList::DrawConnection( cnet * net, int ic )
{
	if( m_dlist )
	{
		UndrawConnection( net, ic );
		cconnect * c = &net->connect[ic];
		// draw display el
		for( int is=0; is<c->nsegs; is++ )
			DrawSegment( net, ic, is );
	}
}

void CNetList::DrawConnections( cnet * n )
{
	if( m_dlist )
	{
		for( int i=0; i<n->nconnects; i++ )
			DrawConnection( n, i );
	}
}

void CNetList::DrawAreas( cnet * n )
{
	if( m_dlist )
	{
		for( int i=0; i<n->nareas; i++ )
			n->area[i].poly->Draw(m_dlist);
	}
}

//AddCutoutsForArea
int CNetList::AddCutoutsForArea( cnet * net, int ia, int gerber_cl, int hole_cl, int bSMT_copper_connect, Merge * m_ml )
{//
	RECT aR = net->area[ia].poly->GetBounds();
	int aL = net->area[ia].poly->GetLayer();
	int gnc = net->area[ia].poly->GetNumCorners();
	int aW = 0;//-aW;
	for( cnet * n = GetFirstNet(); n; n = GetNextNet(/*LABEL*/) )
	{
		if( n != net )
		{
			for( int i=0; i<n->nconnects; i++ )
			{
				int mer_cl = m_ml->GetClearance(n->connect[i].m_merge);
				for( int ii=0; ii<n->connect[i].nsegs; ii++ )
				{
					int vW = n->connect[i].vtx[ii+1].via_w;
					int hW = n->connect[i].vtx[ii+1].via_hole_w;
					RECT sR = rect( n->connect[i].vtx[ii].x, n->connect[i].vtx[ii].y,
									n->connect[i].vtx[ii+1].x, n->connect[i].vtx[ii+1].y );
					RECT vR = rect( n->connect[i].vtx[ii+1].x-vW, n->connect[i].vtx[ii+1].y-vW,
									n->connect[i].vtx[ii+1].x+vW, n->connect[i].vtx[ii+1].y+vW );
					SwellRect( &sR, gerber_cl+aW/2.0 );
					SwellRect( &vR, gerber_cl+aW/2.0 );
					if( RectsIntersection( aR, vR ) != -1 && hW )
					{
						int CS=0;
						GetViaPadInfo( n, i, ii+1, aL, &vW, &hW, &CS );
						int Clearance1 = gerber_cl*2.0 + vW + aW*2.0;
						int Clearance2 = hole_cl*2.0 + hW + aW*2.0;
						if( mer_cl )
							Clearance1 = mer_cl*2.0 + vW + aW*2.0;
						int Clearance = max(Clearance1,Clearance2);
						Clearance += NM_PER_MIL;
						CPoint CONT[20];
						Gen_RndRectPoly( n->connect[i].vtx[ii+1].x, n->connect[i].vtx[ii+1].y, 
										 Clearance, Clearance, Clearance/2.0, 0, CONT, 20 );
						for( int ico=0; ico<20; ico++ )
							AppendAreaCorner( net, ia, CONT[ico].x, CONT[ico].y, 0, FALSE );
						net->area[ia].poly->Close( 0, FALSE );
					}
					if( RectsIntersection( aR, sR ) != -1 && n->connect[i].seg[ii].layer == aL )
					{
						int CS=0;
						GetViaPadInfo( n, i, ii+1, aL, &vW, &hW, &CS );
						int Clearance;
						if( mer_cl )
							Clearance = mer_cl*2.0 + n->connect[i].seg[ii].width + aW*2.0;
						else
							Clearance = gerber_cl*2.0 + n->connect[i].seg[ii].width + aW*2.0;
						Clearance += NM_PER_MIL;
						CPoint CONT[20];
						Gen_HollowLinePoly( n->connect[i].vtx[ii].x, n->connect[i].vtx[ii].y, 
										 n->connect[i].vtx[ii+1].x, n->connect[i].vtx[ii+1].y, Clearance, CONT, 20 );
						for( int ico=0; ico<20; ico++ )
							AppendAreaCorner( net, ia, CONT[ico].x, CONT[ico].y, 0, FALSE );
						net->area[ia].poly->Close( 0, FALSE );
					}
				}
			}
		}
	}
	for( cpart * p=m_plist->GetFirstPart(); p; p=m_plist->GetNextPart(p) )
	{
		if( p->shape )
		{
			int mer_cl = m_ml->GetClearance(p->m_merge);
			for( int ip=0; ip<p->shape->GetNumPins(); ip++ )
			{
				RECT pb = p->pin[ip].bounds;
				SwellRect( &pb, gerber_cl+aW/2.0 );
				if(RectsIntersection( aR, pb ) != -1 )
				{
					int pad_type=0, pad_x=0, pad_y=0, pad_w=0, pad_l=0, pad_r=0, pad_hole=0, pad_angle=0, pad_connect=0, connect_f=0;
					cnet * pad_net=NULL;
					int check = m_plist->GetPadDrawInfo( p, ip, aL, 0, 0, 0, 0,
														&pad_type, &pad_x, &pad_y, &pad_w, &pad_l, &pad_r, 
														&pad_hole, &pad_angle, &pad_net, &pad_connect, &connect_f );
					if( check )
					{
						if( (connect_f == ::PAD_CONNECT_DEFAULT && bSMT_copper_connect == 0) ||
							connect_f == ::PAD_CONNECT_NEVER || 
							net != pad_net )
						{		
							int Clearance = gerber_cl + aW;
							if( pad_hole > pad_w )
							{
								Clearance = hole_cl + aW;
								pad_w = pad_hole;
							}
							else if( mer_cl )
								Clearance = mer_cl + aW;
							Clearance += NM_PER_MIL;
							if( pad_type == PAD_DEFAULT ||
								pad_type == PAD_OCTAGON ||
								pad_type == PAD_SQUARE ||
								pad_type == PAD_ROUND ||
								pad_type == PAD_NONE )
							{
								pad_l = pad_w;
							}
							if( pad_w )
							{
								// corr pad r
								if( pad_type != PAD_RRECT && pad_type != PAD_RECT && pad_r == 0 )
									pad_r = min(pad_w,pad_l)/2.0;
						 		CPoint CONT[20];
								int npo = Gen_RndRectPoly( pad_x, pad_y, pad_l+Clearance*2, pad_w+Clearance*2, (pad_type==PAD_OCTAGON?(pad_w+Clearance*2)/3.414:(pad_r+Clearance)), -pad_angle, CONT, (pad_type==PAD_OCTAGON?8:20) );
								for( int ico=0; ico<npo; ico++ )
									AppendAreaCorner( net, ia, CONT[ico].x, CONT[ico].y, 0, FALSE );
								net->area[ia].poly->Close( 0, FALSE );
							}
						}
					}
				}
			}
		}
	}
	aW = min(0,net->area[ia].poly->GetW());
	if( aW < 0 )
	{
		CPolyLine * po = net->area[ia].poly;
		for( int i=gnc-1; i>=0; i-- )
		{
			CPoint PO[N_SIDES_APPROX_ARC+1];
			int n_sides = Generate_Arc( po->GetX(i), po->GetY(i), po->GetX(po->GetIndexCornerNext(i)), po->GetY(po->GetIndexCornerNext(i)), po->GetSideStyle(i), PO, N_SIDES_APPROX_ARC );
			for(int ii=0; ii<n_sides-1; ii++ )
			{
				CPoint CONT[10];
				Gen_HollowLinePoly( PO[ii].x, PO[ii].y, PO[ii+1].x, PO[ii+1].y, -aW, CONT, 10 );
				for( int ico=0; ico<10; ico++ )
					AppendAreaCorner( net, ia, CONT[ico].x, CONT[ico].y, 0, FALSE );
				net->area[ia].poly->Close( 0, FALSE );
			}
		}
		po->SetW(0);
	}
	return 0;
}

// Add new pin to net
//
void CNetList::AddNetPin( cnet * net, CString * ref_des, CString * pin_name, BOOL set_areas )
{
	// set size of pin array
	net->pin.SetSize( net->npins + 1 );

	// add pin to array
	net->pin[net->npins].ref_des = *ref_des;
	net->pin[net->npins].pin_name = *pin_name;
	net->pin[net->npins].part = NULL;

	// now lookup part and hook to net if successful
	cpart * part = m_plist->GetPart( *ref_des );
	if( part )
	{
		// hook part to net
		net->pin[net->npins].part = part;
		if( part->shape )
		{
			for( int pin_index=part->shape->GetPinIndexByName(*pin_name,-1); pin_index>=0; pin_index=part->shape->GetPinIndexByName(*pin_name,pin_index) )
			{
				// hook net to part
				part->pin[pin_index].net = net;
			}
		}
	}

	net->npins++;
	// adjust connections to areas
	if( net->nareas && set_areas )
		SetAreaConnections( net );
}

// Remove pin from net (by reference designator and pin number)
// Use this if the part may not actually exist in the partlist,
// or the pin may not exist in the part 
//
void CNetList::RemoveNetPin( cnet * net, CString * ref_des, CString * pin_name, BOOL bSetAreas )
{
	// find pin in pin list for net
	int net_pin = -1;
	for( int ip=0; ip<net->npins; ip++ )
	{
		if( net->pin[ip].ref_des.Compare( *ref_des ) == 0 && net->pin[ip].pin_name.Compare( *pin_name ) == 0 )
		{
			net_pin = ip;
			break;
		}
	}
	if( net_pin == -1 )
	{
		// pin not found
		ASSERT(0);
	}
	RemoveNetPin( net, net_pin, bSetAreas );
}

// Remove pin from net (by pin index)
// Use this if the part may not actually exist in the partlist,
// or the pin may not exist in the part 
//
void CNetList::RemoveNetPin( cnet * net, int net_pin_index, BOOL bSetAreas, BOOL bSetPartData )
{
	// now remove all connections to/from this pin
	int ic = 0;
	while( ic<net->nconnects )
	{
		cconnect * c = &net->connect[ic];
		if( c->start_pin == net_pin_index || c->end_pin == net_pin_index )
			RemoveNetConnect( net, ic, FALSE );
		else
			ic++;
	}
	// now remove link to net from part pin (if it exists)
	cpart * part = net->pin[net_pin_index].part;
	if( part && bSetPartData )
	{
		if( part->shape )
		{
			for( int	part_pin_index=part->shape->GetPinIndexByName(net->pin[net_pin_index].pin_name,-1); 
						part_pin_index>=0; 
						part_pin_index=part->shape->GetPinIndexByName(net->pin[net_pin_index].pin_name,part_pin_index))
				part->pin[part_pin_index].net = NULL;
		}
	}
	// now remove pin from net
	net->pin.RemoveAt(net_pin_index);
	net->npins--;
	// now adjust pin numbers in remaining connections
	for( ic=0; ic<net->nconnects; ic++ )
	{
		cconnect * c = &net->connect[ic];
		if( c->start_pin > net_pin_index )
			c->start_pin--;
		if( c->end_pin > net_pin_index )
			c->end_pin--;
	}
	// adjust connections to areas
	if( net->nareas && bSetAreas )
		SetAreaConnections( net );
}



// Remove connections to part->pin from part->pin->net
// set part->pin->net pointer and net->pin->part pointer to NULL
//
void CNetList::DisconnectNetPin( cpart * part, CString * pin_name, BOOL bSetAreas )
{
	if( !part )
		ASSERT(0);
	if( !part->shape )
		ASSERT(0);
	int pin_index = part->shape->GetPinIndexByName( *pin_name, -1 );
	if( pin_index == -1 )
		ASSERT(0);
	cnet * net = (cnet*)part->pin[pin_index].net;
	if( net == 0 )
	{
		return;
	}
	// find pin in pin list for net
	int net_pin = GetNetPinIndex( net, &part->ref_des, pin_name );
	if( net_pin == -1 )
	{
		// pin not found
		ASSERT(0);
	}
	// now remove all connections to/from this pin
	int ic = 0;
	while( ic<net->nconnects )
	{
		cconnect * c = &net->connect[ic];
		if( c->start_pin == net_pin || c->end_pin == net_pin )
			RemoveNetConnect( net, ic, FALSE );
		else
			ic++;
	}
	// now remove link to net from part
	part->pin[pin_index].net = NULL;
	// now remove link to part from net
	net->pin[net_pin].part = NULL;
	// adjust connections to areas
	if( net->nareas && bSetAreas )
		SetAreaConnections( net );
}


// Disconnect pin from net (by reference designator and pin number)
// Use this if the part may not actually exist in the partlist,
// or the pin may not exist in the part 
//
void CNetList::DisconnectNetPin( cnet * net, CString * ref_des, CString * pin_name, BOOL bSetAreas )
{
	// find pin in pin list for net
	int net_pin = -1;
	for( int ip=0; ip<net->npins; ip++ )
	{
		if( net->pin[ip].ref_des == *ref_des && net->pin[ip].pin_name == *pin_name )
		{
			net_pin = ip;
			break;
		}
	}
	if( net_pin == -1 )
	{
		// pin not found
		ASSERT(0);
	}
	// now remove all connections to/from this pin
	int ic = 0;
	while( ic<net->nconnects )
	{
		cconnect * c = &net->connect[ic];
		if( c->start_pin == net_pin || c->end_pin == net_pin )
			RemoveNetConnect( net, ic, FALSE );
		else
			ic++;
	}
	// now remove link to net from part pin (if it exists)
	cpart * part = net->pin[net_pin].part;
	if( part )
	{
		if( part->shape )
		{
			for( int pin_index=part->shape->GetPinIndexByName(*pin_name,-1); pin_index>=0; pin_index=part->shape->GetPinIndexByName(*pin_name,pin_index))
				part->pin[pin_index].net = NULL;
		}
	}
	net->pin[net_pin].part = NULL;
	// adjust connections to areas
	if( net->nareas && bSetAreas )
		SetAreaConnections( net );
}
//===============================================================================================
int CNetList::GetPinIndexByNameForPart( cpart * part, CString pin, int x, int y )
{
	int RET_I = -1;
	if( part->shape )
	{
		double min_dst = INT_MAX;
		for( int i=part->shape->GetPinIndexByName(pin,-1); i>=0; i=part->shape->GetPinIndexByName(pin,i) )
		{
			CPoint pp = m_plist->GetPinPoint( part, i, part->side, part->angle );
			int dst = Distance(x,y,pp.x,pp.y);
			dst -= dst%8;
			int pl = m_plist->GetPinLayer( part, i );		
			if( dst < min_dst )
			{
				min_dst = dst;
				RET_I = i;
			}
			else if( dst == min_dst && pl < LAY_TOP_COPPER )
			{
				min_dst = dst;
				RET_I = i;
			}
		}
		return RET_I;
	}
	//ASSERT(0);
	return RET_I;
}
// return pin index or -1 if not found
//
int CNetList::GetNetPinIndex( cnet * net, CString * ref_des, CString * pin_name )
{
	// find pin in pin list for net
	int net_pin = -1;
	for( int ip=0; ip<net->npins; ip++ )
	{
		if( net->pin[ip].ref_des == *ref_des && net->pin[ip].pin_name == *pin_name )
		{
			net_pin = ip;
			break;
		}
	}
	return net_pin;
}

//
cnet * CNetList::FindPin( CString * ref_des, CString * pin_name )
{
	POSITION pos;
	CString name;
	void * ptr;
	for( pos = m_map.GetStartPosition(); pos != NULL; )
	{
		m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		if( GetNetPinIndex( net, ref_des, pin_name ) >= 0 )
			return net;
	}
	return NULL;
}
// Add new connection to net, consisting of one unrouted segment
// p1 and p2 are indexes into pin array for this net
// returns index to connection, or -1 if fails
//
int CNetList::AddNetConnect( cnet * net, int p1, int p2, int x1, int y1, int x2, int y2 )
{
	if( net->nconnects != net->connect.GetSize() )
		ASSERT(0);

	// check for valid pins
	cpart * part1 = net->pin[p1].part;
	cpart * part2 = net->pin[p2].part;
	if( part1 == 0 || part2 == 0 )
		return -1;
	CShape * shape1 = part1->shape;
	CShape * shape2 = part2->shape;
	if( shape1 == 0 || shape2 == 0 )
		return -1;
	int pin_index1 = GetPinIndexByNameForPart( net->pin[p1].part, net->pin[p1].pin_name, x1, y1 );
	int pin_index2 = GetPinIndexByNameForPart( net->pin[p2].part, net->pin[p2].pin_name, x2, y2 );
	if( pin_index1 == -1 || pin_index2 == -1 )
		return -1;

	net->connect.SetSize( net->nconnects + 1 );
	net->connect[net->nconnects].Initialize( this );
	net->connect[net->nconnects].seg.SetSize( 1 );
	net->connect[net->nconnects].seg[0].Initialize( m_dlist, this );
	net->connect[net->nconnects].vtx.SetSize( 2 );
	net->connect[net->nconnects].vtx[0].Initialize( m_dlist, this );
	net->connect[net->nconnects].vtx[1].Initialize( m_dlist, this );
	net->connect[net->nconnects].nsegs = 1;
	net->connect[net->nconnects].locked = 0;
	net->connect[net->nconnects].start_pin = p1;
	net->connect[net->nconnects].start_pin_shape = pin_index1;
	net->connect[net->nconnects].end_pin = p2;
	net->connect[net->nconnects].end_pin_shape = pin_index2;
	net->connect[net->nconnects].m_selected = 0;
	net->connect[net->nconnects].m_merge = -1;

	// add a single unrouted segment
	CPoint pi, pf;
	pi = m_plist->GetPinPoint( net->pin[p1].part, pin_index1, net->pin[p1].part->side, net->pin[p1].part->angle );
	pf = m_plist->GetPinPoint( net->pin[p2].part, pin_index2, net->pin[p2].part->side, net->pin[p2].part->angle );
	int xi = pi.x;
	int yi = pi.y;
	int xf = pf.x;
	int yf = pf.y;
	net->connect[net->nconnects].seg[0].layer = LAY_RAT_LINE;
	net->connect[net->nconnects].seg[0].width = 0;
	net->connect[net->nconnects].seg[0].selected = 0;

	net->connect[net->nconnects].vtx[0].x = xi;
	net->connect[net->nconnects].vtx[0].y = yi;
	net->connect[net->nconnects].vtx[0].pad_layer = m_plist->GetPinLayer( net->pin[p1].part, pin_index1 );
	net->connect[net->nconnects].vtx[0].force_via_flag = 0;
	net->connect[net->nconnects].vtx[0].tee_ID = 0;
	net->connect[net->nconnects].vtx[0].via_w = 0;
	net->connect[net->nconnects].vtx[0].via_hole_w = 0;
	net->connect[net->nconnects].vtx[0].selected = 0;

	net->connect[net->nconnects].vtx[1].x = xf;
	net->connect[net->nconnects].vtx[1].y = yf;
	net->connect[net->nconnects].vtx[1].pad_layer = m_plist->GetPinLayer( net->pin[p2].part, pin_index2 );
	net->connect[net->nconnects].vtx[1].force_via_flag = 0;
	net->connect[net->nconnects].vtx[1].tee_ID = 0;
	net->connect[net->nconnects].vtx[1].via_w = 0;
	net->connect[net->nconnects].vtx[1].via_hole_w = 0;
	net->connect[net->nconnects].vtx[1].selected = 0;
	net->connect[net->nconnects].seg[0].dl_el = NULL;
	net->connect[net->nconnects].vtx[0].dl_el = NULL;
	net->connect[net->nconnects].vtx[1].dl_el = NULL;

	if( m_dlist )
	{
		// draw graphic elements for segment
		DrawSegment( net, net->nconnects, 0 );
	}
	net->nconnects++;

	return net->nconnects-1;
}

//  repair trace
void CNetList::RepairBranch( cnet * net, int ic, BOOL bMove )
{
	cconnect * c = &net->connect[ic];
	int tee = c->vtx[c->nsegs].tee_ID;
	if( tee )
	{
		int endx = c->vtx[c->nsegs].x;
		int endy = c->vtx[c->nsegs].y;
		int ci=0, vi=0; // find branches
		// Search header trace
		if ( FindTeeVertexInNet( net, tee, &ci, &vi ))
		{
			int xtee = net->connect[ci].vtx[vi].x;
			int ytee = net->connect[ci].vtx[vi].y;
			if( endx != xtee || endy != ytee )
			{
				if( bMove || c->seg[c->nsegs-1].layer == LAY_RAT_LINE )
					MoveVertex(net,ic,c->nsegs,xtee,ytee);
				else
				{
					c->vtx[c->nsegs].tee_ID = 0;
					int new_i = AppendSegment(net,ic,xtee,ytee,LAY_RAT_LINE,0,0,0);
					c->vtx[new_i+1].tee_ID = tee; 
				}
			}
		}
	}
}
void CNetList::RepairAllBranches( BOOL bMove )
{
	POSITION pos;
	CString name;
	void * ptr;
	for( pos = m_map.GetStartPosition(); pos != NULL; )
	{
		m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		if(net) for(int ni=0; ni<net->nconnects; ni++)
		{
			RepairBranch(net,ni,bMove);
		}
	}
}

// reverse
BOOL CNetList::ReverseNetConnect( cnet * net, int ic, BOOL bRev )
{
	cconnect * c = &net->connect[ic];
	if( c->end_pin < 0 )
		return FALSE;
	int buf;
	int nsegs = c->nsegs;
	if( bRev ) 
	{
		buf = c->start_pin;
		c->start_pin = c->end_pin;
		c->end_pin = buf;
		buf = c->start_pin_shape;
		c->start_pin_shape = c->end_pin_shape;
		c->end_pin_shape = buf;
		for( int i=0; i<(nsegs+1)/2; i++ )
		{
			if( c->end_pin == cconnect::NO_END )
				ASSERT(0);
			// swap layer
			buf = c->seg[i].layer;
			c->seg[i].layer = c->seg[c->nsegs-i-1].layer;
			c->seg[c->nsegs-i-1].layer = buf;
			// swap width
			buf = c->seg[i].width;
			c->seg[i].width = c->seg[c->nsegs-i-1].width;
			c->seg[c->nsegs-i-1].width = buf;
			// swap m_uid
			buf = c->seg[i].m_uid;
			c->seg[i].m_uid = c->seg[c->nsegs-i-1].m_uid;
			c->seg[c->nsegs-i-1].m_uid = buf;
			// swap utility
			buf = c->seg[i].utility;
			c->seg[i].utility = c->seg[c->nsegs-i-1].utility;
			c->seg[c->nsegs-i-1].utility = buf;
			// swap selected
			buf = c->seg[i].selected;
			c->seg[i].selected = c->seg[c->nsegs-i-1].selected;
			c->seg[c->nsegs-i-1].selected = buf;
			// swap force_via_flag
			buf = c->vtx[i].force_via_flag;
			c->vtx[i].force_via_flag = c->vtx[c->nsegs-i].force_via_flag;
			c->vtx[c->nsegs-i].force_via_flag = buf;
			// swap m_uid
			buf = c->vtx[i].m_uid;
			c->vtx[i].m_uid = c->vtx[c->nsegs-i].m_uid;
			c->vtx[c->nsegs-i].m_uid = buf;
			// swap pad_layer
			buf = c->vtx[i].pad_layer;
			c->vtx[i].pad_layer = c->vtx[c->nsegs-i].pad_layer;
			c->vtx[c->nsegs-i].pad_layer = buf;
			// swap selected
			buf = c->vtx[i].selected;
			c->vtx[i].selected = c->vtx[c->nsegs-i].selected;
			c->vtx[c->nsegs-i].selected = buf;
			// swap tee_ID
			buf = c->vtx[i].tee_ID;
			c->vtx[i].tee_ID = c->vtx[c->nsegs-i].tee_ID;
			c->vtx[c->nsegs-i].tee_ID = buf;
			// swap utility
			buf = c->vtx[i].utility;
			c->vtx[i].utility = c->vtx[c->nsegs-i].utility;
			c->vtx[c->nsegs-i].utility = buf;
			// swap utility2
			buf = c->vtx[i].utility2;
			c->vtx[i].utility2 = c->vtx[c->nsegs-i].utility2;
			c->vtx[c->nsegs-i].utility2 = buf;
			// swap via_hole_w
			buf = c->vtx[i].via_hole_w;
			c->vtx[i].via_hole_w = c->vtx[c->nsegs-i].via_hole_w;
			c->vtx[c->nsegs-i].via_hole_w = buf;
			// swap via_w
			buf = c->vtx[i].via_w;
			c->vtx[i].via_w = c->vtx[c->nsegs-i].via_w;
			c->vtx[c->nsegs-i].via_w = buf;
			// swap x
			buf = c->vtx[i].x;
			c->vtx[i].x = c->vtx[c->nsegs-i].x;
			c->vtx[c->nsegs-i].x = buf;
			// swap y
			buf = c->vtx[i].y;
			c->vtx[i].y = c->vtx[c->nsegs-i].y;
			c->vtx[c->nsegs-i].y = buf;
			// swap layerbit
			buf = c->vtx[i].layer_bit;
			c->vtx[i].layer_bit = c->vtx[c->nsegs-i].layer_bit;
			c->vtx[c->nsegs-i].layer_bit = buf;
			// swap m_micro
			buf = c->vtx[i].m_micro;
			c->vtx[i].m_micro = c->vtx[c->nsegs-i].m_micro;
			c->vtx[c->nsegs-i].m_micro = buf;
		}
	}
	c->end_pin = cconnect::NO_END;
	c->end_pin_shape = cconnect::NO_END;
	c->vtx[c->nsegs].pad_layer = 0;
	DrawConnection( net, ic );
	return TRUE;
}

//  mirror
int CNetList::MirrorNetConnect( cnet * net, int ic, int num_copper_layers, BOOL bDraw )
{
	if (bDraw)
		UndrawConnection( net, ic );
	cconnect * c = &net->connect[ic];
	// pad layer
	if ( c->vtx[0].pad_layer == LAY_TOP_COPPER )
		c->vtx[0].pad_layer = LAY_BOTTOM_COPPER;
	else if ( c->vtx[0].pad_layer == LAY_BOTTOM_COPPER )
		c->vtx[0].pad_layer = LAY_TOP_COPPER;
	if ( c->vtx[c->nsegs].pad_layer == LAY_TOP_COPPER )
		c->vtx[c->nsegs].pad_layer = LAY_BOTTOM_COPPER;
	else if ( c->vtx[c->nsegs].pad_layer == LAY_BOTTOM_COPPER )
		c->vtx[c->nsegs].pad_layer = LAY_TOP_COPPER;
	//
	int lay;
	for (int is=0; is<c->nsegs; is++)
	{
		c->vtx[is].x = -c->vtx[is].x;
		lay = c->seg[is].layer; 
		if ( lay == LAY_TOP_COPPER )
			lay = LAY_BOTTOM_COPPER;
		else if ( lay == LAY_BOTTOM_COPPER )
			lay = LAY_TOP_COPPER;
		else if ( lay > LAY_TOP_COPPER )
		{
			lay -= LAY_BOTTOM_COPPER;
			lay = num_copper_layers - lay - 1;
			lay += LAY_BOTTOM_COPPER;
		}
		else
		{
			// is ratline
		}
		c->seg[is].layer = lay;
	}
	c->vtx[c->nsegs].x = -c->vtx[c->nsegs].x;
	if (bDraw)
		DrawConnection( net, ic );
	return 0;
}
// add connection to net consisting of starting vertex only
// i.e. this will be a stub trace with no end pin
// returns index to connection or -1 if fails
//
int CNetList::AddNetStub( cnet * net, int p1, int x1, int y1 )
{
	if( net->nconnects != net->connect.GetSize() )
		ASSERT(0);

	if( net->pin[p1].part == 0 )
		return -1;
	if( net->pin[p1].part->shape == 0 )
		return -1;
	int pin_index = GetPinIndexByNameForPart( net->pin[p1].part, net->pin[p1].pin_name, x1, y1 );
	if( pin_index == -1 )
		return -1;

	net->connect.SetSize( net->nconnects + 1 );
	net->connect[net->nconnects].Initialize( this );
	net->connect[net->nconnects].seg.SetSize( 0 );
	net->connect[net->nconnects].vtx.SetSize( 1 );
	net->connect[net->nconnects].vtx[0].Initialize( m_dlist, this );
	net->connect[net->nconnects].nsegs = 0;
	net->connect[net->nconnects].locked = 0;
	net->connect[net->nconnects].start_pin = p1;
	net->connect[net->nconnects].start_pin_shape = pin_index;
	net->connect[net->nconnects].end_pin = cconnect::NO_END;
	net->connect[net->nconnects].end_pin_shape = cconnect::NO_END;
	net->connect[net->nconnects].m_selected = 0;
	net->connect[net->nconnects].m_merge = -1;

	// add a single vertex
	CPoint pi;
	pi = m_plist->GetPinPoint( net->pin[p1].part, pin_index, net->pin[p1].part->side, net->pin[p1].part->angle );
	net->connect[net->nconnects].vtx[0].x = pi.x;
	net->connect[net->nconnects].vtx[0].y = pi.y;
	net->connect[net->nconnects].vtx[0].pad_layer = m_plist->GetPinLayer( net->pin[p1].part, pin_index );
	net->connect[net->nconnects].vtx[0].force_via_flag = 0;
	net->connect[net->nconnects].vtx[0].tee_ID = 0;
	net->connect[net->nconnects].vtx[0].via_w = 0;
	net->connect[net->nconnects].vtx[0].via_hole_w = 0;
	net->connect[net->nconnects].vtx[0].dl_el = NULL;
	net->connect[net->nconnects].vtx[0].selected = 0;
	net->nconnects++;
	return net->nconnects-1;
}

// test for hit on end-pad of connection
// if dir == 0, check end pad
// if dir == 1, check start pad
//
BOOL CNetList::TestHitOnConnectionEndPad( int x, int y, cnet * net, int ic, 
										 int layer, int dir )
{
	int ip;
	cconnect * c =&net->connect[ic];
	if( dir == 1 )
	{
		// get first pad in connection
		ip = c->start_pin;
	}
	else
	{
		// get last pad in connection
		ip = c->end_pin;
	}
	if( ip != cconnect::NO_END )
	{
		cpart * part = net->pin[ip].part;
		CString pin_name = net->pin[ip].pin_name;
		if( !part )
			ASSERT(0);
		if( !part->shape )
			ASSERT(0);
		int RET = m_plist->TestHitOnPad( part, &pin_name, x, y, layer );
		RET += 1;
		return (BOOL)RET;
	}
	else
		return FALSE;
}

// test for hit on any pad on this net
// returns -1 if not found, otherwise index into net->pin[]
//
int CNetList::TestHitOnAnyPadInNet( int x, int y, int layer, cnet * net, int * part_pin_index )
{
	for( int ip=0; ip<net->pin.GetSize(); ip++ )
	{
		cpart * part = net->pin[ip].part;
		if( part )
		{
			CString pin_name = net->pin[ip].pin_name;
			*part_pin_index = m_plist->TestHitOnPad( part, &pin_name, x, y, layer );
			if( *part_pin_index >= 0 )
				return ip;
		}
	}
	return -1;
}

// Clean up connections by removing connections with no segments,
// removing zero-length segments and combining segments
//
void CNetList::CleanUpConnections( cnet * net, CString * logstr )
{
	for( int ic=net->nconnects-1; ic>=0; ic-- )   
	{
		UndrawConnection( net, ic );
		cconnect * c = &net->connect[ic];
		// now check for non-branch stubs with a single unrouted segment and no end-via
		if( c->end_pin == cconnect::NO_END && c->nsegs == 1 )
		{
			cvertex * end_v = &c->vtx[1];
			cseg * end_s = &c->seg[0];
			if( end_v->tee_ID == 0 && end_v->via_w == 0 && end_s->layer == LAY_RAT_LINE )
			{
				if( logstr )
				{
					CString str;
					str.Format( "net %s: stub trace from %s.%s: single unrouted segment and no end via, removed\r\n",
						net->name, 
						net->pin[c->start_pin].ref_des, net->pin[c->start_pin].pin_name ); 
					*logstr += str;
				}
				net->connect.RemoveAt(ic);
				net->nconnects--;
				continue;
			}
		}
		for( int is=c->nsegs-1; is>0; is-- )
		{
			if( c->seg[is].layer != c->seg[is-1].layer &&
				c->seg[is].layer != LAY_RAT_LINE &&
				c->seg[is-1].layer != LAY_RAT_LINE &&
				(!c->vtx[is].via_w || !c->vtx[is].via_hole_w) )
			{
				if( logstr )
				{
					CString str;
					str.Format( "net %s: trace %d - no via, corrected\r\n",
						net->name, ic+1 ); 
					*logstr += str;
					int vw=0, vh=0;
					for( int fis=c->nsegs; fis>0; fis-- )
					{		
						if( !vw )
							if( c->vtx[fis].via_w )
							{
								vw = c->vtx[fis].via_w;
								vh = c->vtx[fis].via_hole_w;
								break;
							}
					}
					ReconcileVia( net, ic, is, TRUE, vw, vh );
				}
			}
		}
		for( int is=c->nsegs-2; is>=0; is-- )
		{
			
			if( c->seg[is].layer == c->seg[is+1].layer && 
				c->seg[is].width == c->seg[is+1].width && 
				c->vtx[is+1].via_w == 0 && 
				c->vtx[is+1].tee_ID == 0 )
			{ 
				// see if colinear
				if( Distance( c->vtx[is].x,c->vtx[is].y,c->vtx[is+1].x,c->vtx[is+1].y ) < _2540 ||
					Distance( c->vtx[is+1].x,c->vtx[is+1].y,c->vtx[is+2].x,c->vtx[is+2].y ) < _2540 ||
					Colinear (	c->vtx[is].x,c->vtx[is].y,
								c->vtx[is+1].x,c->vtx[is+1].y,
								c->vtx[is+2].x,c->vtx[is+2].y) )
				{
					// yes, combine these segments
					if( logstr )
					{
						CString str;
						if( c->end_pin == cconnect::NO_END )
						{
							str.Format( "net %s: stub trace from %s.%s: remove zero length or colinear segments\r\n",
								net->name, 
								net->pin[c->start_pin].ref_des, net->pin[c->start_pin].pin_name ); 
						}
						else
						{
							str.Format( "net %s: trace %s.%s to %s.%s: remove zero length or colinear segments\r\n",
								net->name, 
								net->pin[c->start_pin].ref_des, net->pin[c->start_pin].pin_name, 
								net->pin[c->end_pin].ref_des, net->pin[c->end_pin].pin_name ); 
						}
						*logstr += str;
					}
					c->vtx.RemoveAt(is+1);
					c->seg.RemoveAt(is+1);
					c->nsegs--;
				}
			}	
		}
		// see if there are any segments left
		if( c->nsegs == 0 )
		{
			// no, remove connection
			net->connect.RemoveAt(ic);
			net->nconnects--;
			continue;
		}
		if( m_dlist )
			DrawConnection( net, ic );
	}
	RenumberConnections( net );
}


void CNetList::CleanUpAllConnections( CString * logstr )
{
	CString str;

	cnet * net = GetFirstNet();
	while( net )
	{
		CleanUpConnections( net, logstr );
		net = GetNextNet(/*LABEL*/);
	}
	// check tee_IDs in array
	if( logstr )
		*logstr += "\r\nChecking tees and branches:\r\n";
	if( logstr )
	{
		str.Format( "  %d tee_IDs in array:\r\n", m_tee.GetSize() );
		*logstr += str;
	}
	for( int it=0; it<m_tee.GetSize(); it++ )
	{
		int tee_id = m_tee[it];
		cnet * net = NULL;
		int ic;
		int iv;
		BOOL bFound = FindTeeVertex( tee_id, &net, &ic, &iv );
		if( !bFound )
		{
			if( logstr )
			{
				str.Format( "    tee_id %d not found in project, removed\r\n", tee_id );
				*logstr += str;
			}
			RemoveTeeID( tee_id );
		}
	}
	// now check tee_IDs in project
	net = GetFirstNet();
	while( net )
	{
		// may have to iterate until no connections removed
		int n_removed = 1;
		while( n_removed != 0 )
		{
			n_removed = 0;
			for( int ic=net->nconnects-1; ic>=0; ic-- )
			{
				cconnect * c = &net->connect[ic];
				if( c->end_pin == cconnect::NO_END )
				{
					// branch, check for tee
					int end_id = c->vtx[c->nsegs].tee_ID;
					if( end_id )
					{
						BOOL bError = FALSE;
						CString no_tee_str = "";
						CString no_ID_str = "";
						int ci=0, vi=0;
						if( !FindTeeVertexInNet( net, end_id, &ci, &vi ) )
						{
							no_tee_str = ", not in trace";
							bError = TRUE;
						}
						if( FindTeeID( end_id ) == -1 )
						{
							no_ID_str = ", not in ID array";
							bError = TRUE;
						}
						if( bError && logstr )
						{
							str.Format( "  tee_id %d found in branch%s%s, branch removed\r\n", 
								end_id, no_tee_str, no_ID_str );
							*logstr += str;
						}
						if( bError )
						{
							RemoveNetConnect( net, ic, FALSE );
							n_removed++;
						}
					}
				}
				else
				{
					for( int iv=1; iv<c->nsegs; iv++ )
					{
						if( int id=c->vtx[iv].tee_ID )
						{
							// tee-vertex, check array
							if( FindTeeID(id) == -1 && logstr )
							{
								str.Format( "  tee_id %d found in trace, not in ID array\r\n", id );
								*logstr += str;
							}
						}
					}
				}
			}
		}
		net = GetNextNet(/*LABEL*/);
	}
}


// Remove connection from net
// Does not remove any orphaned branches that result
// Leave pins in pin list for net
//
int CNetList::RemoveNetConnect( cnet * net, int ic, BOOL set_areas )
{
	if( net->connect[ic].end_pin == cconnect::NO_END )
	{
		// stub
		if( net->connect[ic].vtx[net->connect[ic].nsegs].tee_ID )
		{
			// branch ending on tee, remove tee
			DisconnectBranch( net, ic );
		}
	}

	// see if contains tee-vertices
	for( int iv=1; iv<net->connect[ic].nsegs; iv++ )
	{
		int id = net->connect[ic].vtx[iv].tee_ID;
		if( id )
		{
			RemoveTee( net, id, ic );	// yes, remove it
			RemoveOrphanBranches( net, id );
		}
	}
	// remove connection
	UndrawConnection(net,ic);
	net->connect.RemoveAt( ic );
	net->nconnects = net->connect.GetSize();
	RenumberConnections( net );
	// adjust connections to areas
	if( net->nareas && set_areas )
		SetAreaConnections( net );
	return 0;
}

// Unroute all segments of a connection and merge if possible
// Preserves tees
//
int CNetList::UnrouteNetConnect( cnet * net, int ic )
{
	cconnect * c = &net->connect[ic];
	for( int is=0; is<c->nsegs; is++ )
	{
		net->connect[ic].seg[is].width = 0;
		net->connect[ic].seg[is].layer = LAY_RAT_LINE;
	}
	MergeUnroutedSegments( net, ic );
	return 0;
}

// Change the start or end pin of a connection and redraw it
//
int CNetList::ChangeConnectionPin( cnet * net, int ic, int end_flag, cnet * nnet, cpart * part, CString * pin_name, int x2, int y2 )
{
#define c	net->connect[ic]
#define netpin nnet->pin[pin_index]

	// find pin in pin list for net
	int pin_index = GetNetPinIndex( nnet, &part->ref_des, pin_name );
	if( pin_index == -1 || part->shape == NULL )
	{
		// pin not found
		ASSERT(0);
		return ic;
	}
	int Index = GetPinIndexByNameForPart( part, *pin_name, x2, y2 );
	CPoint p = m_plist->GetPinPoint( part, Index, part->side, part->angle );
	int layer = LAY_TOP_COPPER + m_layers;
	for( int ii=part->shape->GetPinIndexByName( *pin_name, -1); ii>=0; ii=part->shape->GetPinIndexByName( *pin_name, ii) )
	{
		CPoint p2 = m_plist->GetPinPoint( part, ii, part->side, part->angle );
		if( p.x == p2.x && p.y == p2.y )
			layer = min(m_plist->GetPinLayer( part, ii ),layer);
	}
	if( end_flag )
	{
		// change end pin
		int is = c.nsegs-1;
		if( c.vtx[is+1].tee_ID )
			DisconnectBranch( net, ic );
		if( net == nnet )
		{
			c.end_pin = pin_index;
			c.end_pin_shape = Index;
			c.vtx[is+1].x = p.x;
			c.vtx[is+1].y = p.y;
			c.vtx[is+1].pad_layer = layer;
			DrawSegment( net, ic, is );
			return ic;
		}
		else
		{
			int newc = AddNetStub( nnet, pin_index, x2, y2 );
			nnet->connect[newc].start_pin = pin_index;
			nnet->connect[newc].start_pin_shape = Index;
			nnet->connect[newc].end_pin = -1;
			nnet->connect[newc].end_pin_shape = -1;
			BOOL DELETED = false;
			for( int cc=c.nsegs-1; cc>=0; cc-- )
			{
				int x = c.vtx[cc].x;
				int y = c.vtx[cc].y;
				int l = c.seg[cc].layer;
				int w = c.seg[cc].width;
				int vw = c.vtx[cc].via_w;
				int hw = c.vtx[cc].via_hole_w;
				AppendSegment( nnet, newc, x, y, l, w, vw, hw );
				int tee = c.vtx[cc].tee_ID;
				if(tee)
				{
					c.vtx[cc].tee_ID = 0;
					int rem = RemoveOrphanBranches( net, tee, true, vw, hw );
					if( rem == ic )
						DELETED = true;
					if( rem >= 0 && rem < ic )
						ic--;
				}
			}
			if( !DELETED )
				RemoveNetConnect( net, ic );
			DrawConnection( nnet, newc );
			return newc;
		}
	}
	else
	{
		// change start pin
		if( net == nnet )
		{
			c.start_pin = pin_index;
			c.start_pin_shape = Index;
			c.vtx[0].x = p.x;
			c.vtx[0].y = p.y;
			c.vtx[0].pad_layer = layer;
			DrawSegment( net, ic, 0 );
			return ic;
		}
		else
		{
			int newc = AddNetStub( nnet, pin_index, x2, y2 );
			nnet->connect[newc].start_pin = pin_index;
			nnet->connect[newc].start_pin_shape = Index;
			nnet->connect[newc].end_pin = -1;
			nnet->connect[newc].end_pin_shape = -1;
			BOOL DELETED = false;
			for( int cc=1; cc<=c.nsegs; cc++ )
			{
				int x = c.vtx[cc].x;
				int y = c.vtx[cc].y;
				int l = c.seg[cc-1].layer;
				int w = c.seg[cc-1].width;
				int vw = c.vtx[cc].via_w;
				int hw = c.vtx[cc].via_hole_w;
				AppendSegment( nnet, newc, x, y, l, w, vw, hw );
				int tee = c.vtx[cc].tee_ID;
				if(tee)
				{
					c.vtx[cc].tee_ID = 0;
					int rem = RemoveOrphanBranches( net, tee, true, vw, hw );
					if( rem == ic )
						DELETED = true;
					else if( rem >= 0 && rem < ic )
						ic--;
				}
			}
			if( !DELETED )
				RemoveNetConnect( net, ic );
			DrawConnection( nnet, newc );
			return newc;
		}
	}
#undef c
#undef netpin
}



// Merge any adjacent unrouted segment of this connection
// unless separated by a tee-connection
// Returns id of first merged segment in connection
// Reconciles vias for any tee-connections by calling DrawConnection()
//
void CNetList::MergeUnroutedSegments( cnet * net, int ic, BOOL bDraw )
{
	cconnect * c = &net->connect[ic];
	if( m_dlist && bDraw )
		UndrawConnection( net, ic );
	for( int is=c->nsegs-2; is>=0; is-- )
	{
		cseg * post_s = &c->seg[is+1];
		cseg * s = &c->seg[is];
		if( post_s->layer == LAY_RAT_LINE && s->layer == LAY_RAT_LINE
			&& c->vtx[is+1].tee_ID == 0 && c->vtx[is+1].force_via_flag == 0 )
		{
			// this segment and next are unrouted, 
			// remove next segment and interposed vertex
			c->seg.RemoveAt(is+1);
			c->vtx.RemoveAt(is+1);
			c->nsegs = c->seg.GetSize();
		}
	}
	if( m_dlist && bDraw )
		DrawConnection( net, ic );
}


// Remove segment ... currently only used for last segment of stub trace
// or segment of normal pin-pin trace without tees
// If adjacent segments are unrouted, removes them too
// NB: May change connect[] array
// If stub trace and bHandleTee == FALSE, will only alter connections >= ic
//
int CNetList::RemoveSegment( cnet * net, int ic, int is )
{
	int id = 0;
	if( net->connect[ic].end_pin == cconnect::NO_END )
	{
		int tee = net->connect[ic].vtx[is+1].tee_ID;
		// stub trace, must be last segment
		if( is < net->connect[ic].nsegs-1 )
		{
			if( tee )
			{
				int Ptee = net->connect[ic].vtx[is].tee_ID;
				// find branches
				int tic=-1, tiv=-1;
				FindTeeVertexInNet( net, tee, &tic, &tiv );
				if( tic >= 0 )
				{
					UndrawConnection( net, ic );
					if( is > 0 )
					{
						int con = AddNetStub( net, net->connect[ic].start_pin, net->connect[ic].vtx[0].x, net->connect[ic].vtx[0].y );
						for( int ii=0; ii<is; ii++ )
						{
							AppendSegment(	net, con, net->connect[ic].vtx[ii+1].x, net->connect[ic].vtx[ii+1].y, net->connect[ic].seg[ii].layer, 
											net->connect[ic].seg[ii].width,  net->connect[ic].vtx[ii+1].via_w, net->connect[ic].vtx[ii+1].via_hole_w, TRUE );
							net->connect[con].vtx[ii+1].tee_ID = net->connect[ic].vtx[ii+1].tee_ID;
						}
						net->connect[con].vtx[net->connect[con].nsegs].tee_ID = Ptee;
					}
					for( int ii=is; ii>=0; ii-- )
					{
						net->connect[ic].seg.RemoveAt(ii);
						net->connect[ic].vtx.RemoveAt(ii+1);
						net->connect[ic].nsegs--;	
					}
					cconnect * tc = &net->connect[tic];
					net->connect[ic].start_pin = tc->start_pin;
					net->connect[ic].start_pin_shape = tc->start_pin_shape;
					net->connect[ic].vtx[0].pad_layer = tc->vtx[0].pad_layer;
					net->connect[ic].vtx[0].x = tc->vtx[0].x;
					net->connect[ic].vtx[0].y = tc->vtx[0].y;
					for( int ii=tc->nsegs-1; ii>=0; ii-- )
					{
						InsertSegment(	net, ic, 0, tc->vtx[ii+1].x, tc->vtx[ii+1].y, tc->seg[ii].layer, 
										tc->seg[ii].width,  tc->vtx[ii+1].via_w, tc->vtx[ii+1].via_hole_w, FALSE );
						net->connect[ic].vtx[1].tee_ID = tc->vtx[ii+1].tee_ID;
					}
					DrawConnection( net, ic );
					RemoveNetConnect( net, tic, 0 );
					RemoveOrphanBranches( net, tee );
					RemoveOrphanBranches( net, Ptee, TRUE );
				}
			}
			return -1;
		}
		UndrawConnection( net, ic );
		// if this is a branch, disconnect it
		if( tee )
		{
			DisconnectBranch( net, ic );
			RemoveOrphanBranches( net, net->connect[ic].vtx[net->connect[ic].nsegs].tee_ID );
		}
		cconnect * c = &net->connect[ic];
		c->seg.RemoveAt(c->nsegs-1);
		c->vtx.RemoveAt(c->nsegs);
		c->nsegs--;	
		if( c->vtx[c->nsegs].tee_ID )
		{
			// special case...the vertex preceding this segment is a tee-vertex
			int del = RemoveOrphanBranches( net, c->vtx[c->nsegs].tee_ID, TRUE, c->vtx[c->nsegs].via_w, c->vtx[c->nsegs].via_hole_w );
			if( del == ic )
				return -1;
			else if( del >= 0 && del < ic )
			{
				ic--;
				c = &net->connect[ic];
			}
		}			
		if( c->nsegs )
		{
			if( c->seg[c->nsegs-1].layer == LAY_RAT_LINE 
				&& c->vtx[c->nsegs-1].via_w == 0
				&& c->vtx[c->nsegs-1].tee_ID == 0 )
			{
				c->seg.RemoveAt(c->nsegs-1);
				c->vtx.RemoveAt(c->nsegs);
				c->nsegs--;
			}
		}
		if( c->nsegs == 0 )
		{
			net->connect.RemoveAt(ic);
			net->nconnects = net->connect.GetSize();
			RenumberConnections( net );
			return -1;
		}
		DrawConnection( net, ic );
	}
	else
	{
		UndrawConnection( net, ic );
		// pin-pin trace
		cconnect * c = &net->connect[ic];
		int inew = -1;
		if (is < c->nsegs-1)
		{
			inew = AddNetStub( net, c->end_pin );
			c = &net->connect[ic];// (new adress)
			cconnect * cnew = &net->connect[inew];
			int n = 0;
			for (int s=c->nsegs-1; s>is; s--)
			{
				cseg * seg = &c->seg[s];
				cvertex * v = &c->vtx[s];
				//cvertex * post_v = &c->vtx[s+1];
				AppendSegment(	net, inew, 
								v->x, 
								v->y, 
								seg->layer, 
								seg->width, v->via_w, v->via_hole_w, TRUE );
				cnew->vtx[c->nsegs - s].tee_ID = c->vtx[s].tee_ID;
			}
			cnew->end_pin = cconnect::NO_END;
			cnew->end_pin_shape = cconnect::NO_END;
			cnew->vtx[0].pad_layer = c->vtx[c->nsegs].pad_layer;
			cnew->vtx[0].x = c->vtx[c->nsegs].x;
			cnew->vtx[0].y = c->vtx[c->nsegs].y;
			DrawConnection( net, inew );
		}
		else if( is >= c->nsegs )
		{
			ASSERT(0);
			return ic;
		}	
		c->end_pin = cconnect::NO_END;
		c->end_pin_shape = cconnect::NO_END;
		for (int s=is+1; s<c->nsegs;)
		{
			c->seg.RemoveAt(s);
			c->vtx.RemoveAt(s+1);
			c->nsegs--;
		}
		if( c->vtx[is+1].tee_ID )
		{
			// special case...the vertex preceding this segment is a tee-vertex
			int tee = c->vtx[is+1].tee_ID;
			c->vtx[is+1].tee_ID = 0;
			int del = RemoveOrphanBranches( net, tee, TRUE, c->vtx[is+1].via_w, c->vtx[is+1].via_hole_w );
			if( del != -1 )
			{
				inew--;
				if( del == ic )
				{
					if( inew >= 0 )
						return inew;
					else
						return -1;
				}
				else if( del < ic )
				{
					ic--;
					c = &net->connect[ic];
				}
			}
		}
		if (is > 0)
		{
			c->seg.RemoveAt(is);
			c->vtx.RemoveAt(is+1);
			c->nsegs--;
			if( c->vtx[is].tee_ID )
			{
				// special case...the vertex preceding this segment is a tee-vertex
				int tee = c->vtx[is].tee_ID;
				int del = RemoveOrphanBranches( net, tee, TRUE, c->vtx[is].via_w, c->vtx[is].via_hole_w );
				if( del != -1 )
				{
					inew--;
					if( del == ic )
					{
						if( inew >= 0 )
							return inew;
						else
							return -1;
					}
					else if( del < ic )
					{
						ic--;
						c = &net->connect[ic];
					}
				}
			}
			DrawConnection( net, ic );
		}
		else	{
			// remove connection
			net->connect.RemoveAt( ic );
			net->nconnects = net->connect.GetSize();
			RenumberConnections( net );
			return -1;
		}
		
	}
	return ic;
}

// renumber all ids and dl_elements for net connections
// should be used after deleting a connection
//
void CNetList::RenumberConnections( cnet * net )
{
	for( int ic=0; ic<net->nconnects; ic++ )
	{
		RenumberConnection( net, ic );
	}
}

// renumber all ids and dl_elements for net connection
//
void CNetList::RenumberConnection( cnet * net, int ic )
{
	cconnect * c = &net->connect[ic];
	for( int is=0; is<c->nsegs; is++ )
	{
		if( c->seg[is].dl_el )
		{
			c->seg[is].dl_el->id.i = ic;
			c->seg[is].dl_el->id.ii = is;
		}
	}
	for( int iv=0; iv<=c->nsegs; iv++ )
	{	
		if( c->vtx[iv].dl_el )
		{
			c->vtx[iv].dl_el->id.i = ic;
			c->vtx[iv].dl_el->id.ii = iv;
		}			
		if( c->vtx[iv].dl_hole )
		{
			c->vtx[iv].dl_hole->id.i = ic;
			c->vtx[iv].dl_hole->id.ii = iv;
		}	
	}
}

// renumber the ids for graphical elements in areas
// should be called after deleting an area
//
void CNetList::RenumberAreas( cnet * net )
{
	id a_id;
	for( int ia=0; ia<net->nareas; ia++ )
	{
		a_id = net->area[ia].poly->GetId();
		a_id.i = ia;
		net->area[ia].poly->SetId( &a_id );
		for( int i=net->area[ia].poly->dl_side.GetSize()-1; i>=0; i-- )
		{
			net->area[ia].poly->dl_side[i]->id.i = ia;
			net->area[ia].poly->dl_side[i]->id.ii = i;
		}
		for( int i=net->area[ia].poly->dl_corner_sel.GetSize()-1; i>=0; i-- )
		{
			net->area[ia].poly->dl_corner_sel[i]->id.i = ia;
			net->area[ia].poly->dl_corner_sel[i]->id.ii = i;
		}
		for( int ip=0; ip<net->area[ia].npins; ip++ )
			net->area[ia].dl_thermal[ip]->id.i = ia;
		for( int iv=0; iv<net->area[ia].nvias; iv++ )
			net->area[ia].dl_via_thermal[iv]->id.i = ia;
	}
}

// Set segment layer (must be a copper layer, not the ratline layer)
// returns 1 if unable to comply due to SMT pad
//
int CNetList::ChangeSegmentLayer( cnet * net, int ic, int iseg, int layer, int vw, int vh )
{
	if( layer == LAY_SM_TOP || layer == LAY_SM_BOTTOM )
		layer += 2;
	cconnect * c = &net->connect[ic];
	// check layer settings of adjacent vertices to make sure this is legal
	if( iseg == 0 )
	{
		// first segment, check starting pad layer
		int pad_layer = c->vtx[0].pad_layer;
		if( pad_layer != LAY_PAD_THRU && layer != pad_layer )
			return 1;
	}
	if( iseg == (c->nsegs - 1) && c->end_pin != cconnect::NO_END )
	{
		// last segment, check destination pad layer
		int pad_layer = c->vtx[iseg+1].pad_layer;
		if( pad_layer != LAY_PAD_THRU && layer != pad_layer )
			return 1;
	}
	// get via needed
	BOOL needed1 = ReconcileVia( net, ic, iseg, FALSE );
	BOOL needed2 = ReconcileVia( net, ic, iseg+1, FALSE );

	// change segment layer
	cseg * s = &c->seg[iseg];
	//cvertex * pre_v = &c->vtx[iseg];
	//cvertex * post_v = &c->vtx[iseg+1];
	s->layer = layer;

	// now adjust vias
	if( iseg )
		if( ReconcileVia( net, ic, iseg, TRUE, vw, vh ) == 0 && needed1 == 0 )
		{
			net->connect[ic].vtx[iseg].via_w = 0;
			net->connect[ic].vtx[iseg].via_hole_w = 0;
			UndrawVia( net, ic, iseg );
			DrawSegment( net, ic, iseg-1 );
		}
	if( ReconcileVia( net, ic, iseg+1, TRUE, vw, vh ) == 0 && needed2 == 0 )
	{
		net->connect[ic].vtx[iseg+1].via_w = 0;
		net->connect[ic].vtx[iseg+1].via_hole_w = 0;
		UndrawVia( net, ic, iseg+1 );
	}
	DrawSegment( net, ic, iseg );
	return 0;
}

// Convert segment from unrouted to routed
// returns 1 if segment can't be routed on given layer due to connection to SMT pad
// Adds/removes vias as necessary
//
int CNetList::RouteSegment( cnet * net, int ic, int iseg, int layer, int width, int via_w, int via_h, int next_via, int next_hole )
{
	if( next_via == 0 )
		next_via = via_w;
	if( next_hole == 0 )
		next_hole = via_h;
	// check layer settings of adjacent vertices to make sure this is legal
	cconnect * c =&net->connect[ic];
	if( iseg == 0 )
	{
		// first segment, check starting pad layer
		int pad_layer = c->vtx[0].pad_layer;
		if( pad_layer != LAY_PAD_THRU && layer != pad_layer )
			return 1;
	}
	if( iseg == (c->nsegs - 1) )
	{
		// last segment, check destination pad layer
		if( c->end_pin == cconnect::NO_END )
		{
		}
		else
		{
			int pad_layer = c->vtx[iseg+1].pad_layer;
			if( pad_layer != LAY_PAD_THRU && layer != pad_layer )
				return 1;
		}
	}

	// modify segment parameters
	c->seg[iseg].layer = layer;
	c->seg[iseg].width = width;
	c->seg[iseg].selected = 0;

	// now adjust vias
	ReconcileVia( net, ic, iseg, TRUE, via_w, via_h );
	ReconcileVia( net, ic, iseg+1, TRUE, next_via, next_hole );

	// draw elements
	if( m_dlist )
	{
		if( iseg )
			DrawSegment( net, ic, iseg-1 );
		DrawSegment( net, ic, iseg );
	}
	return 0;
}



void CNetList::RouteTrace( cnet * net, int ic, int layer, int width, int via_w, int via_h )
{
	for( int seg=0; seg<net->connect[ic].nsegs; seg++ )
		RouteSegment( net, ic, seg, layer, width, via_w, via_h );
}


int CNetList::DrawSegment( cnet * net, int ic, int is )
{
	cseg * s = &net->connect[ic].seg[is];
	cvertex * pre_v = &net->connect[ic].vtx[is];
	cvertex * post_v = &net->connect[ic].vtx[is+1];

	// undraw
	if( m_dlist )
	{
		m_dlist->Remove( s->dl_el );
		m_dlist->Remove( post_v->dl_el );
		m_dlist->Remove( post_v->dl_hole );
	
		post_v->dl_el = NULL;
		post_v->dl_hole = NULL;
		
		id s_id( ID_NET, ID_CONNECT, ic, ID_SEG, is );
		int vis = 1;
		if( s->layer == LAY_RAT_LINE && 
			net->connect[ic].nsegs == 1 &&
			net->connect[ic].vtx[net->connect[ic].nsegs].tee_ID == 0 &&
			net->connect[ic].vtx[net->connect[ic].nsegs].via_hole_w == 0 )
			vis = net->visible;
		int l_map = 0;
		setbit( l_map, s->layer );

		// rect
		RECT r = rect( pre_v->x, pre_v->y, post_v->x, post_v->y );
		SwellRect( &r, s->width/2 + m_pcbu_per_wu*20 );
		
		CPoint pts[2];
		pts[0].x = pre_v->x;
		pts[1].x = post_v->x;
		pts[0].y = pre_v->y;
		pts[1].y = post_v->y;

	 	s->dl_el = m_dlist->Add(s_id, net, l_map, DL_LINE, vis, 
								&r, s->width, pts, 2 );
		m_dlist->AddSelector( s->dl_el );	
		DrawVia( net, ic, is+1 );
		if( !is )
			DrawVia( net, ic, 0 );
	}
	return 0;
}



// Append new segment to connection 
// this is mainly used for stub traces
// returns index to new segment
//
int CNetList::AppendSegment( cnet * net, int ic, int x, int y, int layer, int width, int vw, int vh, BOOL bAddVia )
{
	// add new vertex and segment
	cconnect * c =&net->connect[ic];
	c->seg.SetSize( c->nsegs + 1 );
	c->seg[c->nsegs].Initialize( m_dlist, this );
	c->vtx.SetSize( c->nsegs + 2 );
	c->vtx[c->nsegs+1].Initialize( m_dlist, this );
	int iseg = c->nsegs;

	// set position for new vertex, zero dl_element pointers
	c->vtx[iseg+1].x = x;
	c->vtx[iseg+1].y = y;
	c->vtx[iseg+1].selected = 0;
	if( bAddVia )
	{
		c->vtx[iseg+1].via_w = vw;
		c->vtx[iseg+1].via_hole_w = vh;
	}

	// create new segment
	c->seg[iseg].layer = layer;
	c->seg[iseg].width = width;
	c->seg[iseg].selected = 0;
	int xi = c->vtx[iseg].x;
	int yi = c->vtx[iseg].y;

	// done
	c->nsegs++;

	// take care of preceding via
	if( iseg )
		if( c->vtx[iseg].via_w && c->vtx[iseg].via_hole_w )
			ReconcileVia( net, ic, iseg, TRUE, c->vtx[iseg].via_w, c->vtx[iseg].via_hole_w );
		else
			ReconcileVia( net, ic, iseg, TRUE, vw, vh );

	// draw seg
	DrawSegment( net, ic, iseg );

	return iseg;
}

// Insert new segment into connection, unless the new segment ends at the 
// endpoint of the old segment, then replace old segment 
// if dir=0 add forward in array, if dir=1 add backwards
// return 1 if segment inserted, 0 if replaced 
// tests position within +/- 10 nm.
//

int CNetList::InsertSegment( cnet * net, int ic, int iseg, int x, int y, int layer, int width,
						 int via_width, int via_hole_width, int dir, BOOL bDrawConnection )
{
	const int TOL = 10;

	UndrawConnection( net, ic );

	// see whether we need to insert new segment or just modify old segment
	cconnect * c = &net->connect[ic];
	int insert_flag = 1;
	if( dir == 0 )
	{
		// routing forward
		if( iseg < c->nsegs )
		if( (abs(x-c->vtx[iseg+1].x) + abs(y-c->vtx[iseg+1].y )) < TOL )
		{ 
			// new vertex is the same as end of old segment 
			if( iseg < (c->nsegs-1) )
			{
				// not the last segment
				if( layer == c->seg[iseg+1].layer )
				{
					// next segment routed on same layer, don't insert new seg
					insert_flag = 0;
				}
			}
			else if( iseg == (c->nsegs-1) )
			{
				// last segment, should connect to pad
				int pad_layer = c->vtx[iseg+1].pad_layer;
				if( pad_layer == LAY_PAD_THRU || layer == LAY_RAT_LINE
					|| (pad_layer == LAY_TOP_COPPER && layer == LAY_TOP_COPPER)
					|| (pad_layer == LAY_BOTTOM_COPPER && layer == LAY_BOTTOM_COPPER) )
				{
					// layer OK to connect to pad, don't insert new seg
					insert_flag = 0;
				}
			}
		}
	}
	else
	{
		// routing backward
		if( x == c->vtx[iseg].x 
			&& y == c->vtx[iseg].y )
		{ 
			// new vertex is the same as start of old segment 
			if( iseg >0 )
			{
				// not the first segment
				if( layer == c->seg[iseg-1].layer )
				{
					// prev segment routed on same layer, don't insert new seg
					insert_flag = 0;
				}
			}
			else if( iseg == 0 )
			{
				// first segment, should connect to pad
				int pad_layer = c->vtx[iseg].pad_layer;
				if( pad_layer == LAY_PAD_THRU || layer == LAY_RAT_LINE
					|| (pad_layer == LAY_TOP_COPPER && layer == LAY_TOP_COPPER)
					|| (pad_layer == LAY_BOTTOM_COPPER && layer == LAY_BOTTOM_COPPER) )
				{
					// layer OK to connect to pad, don't insert new seg
					insert_flag = 0;
				}
			}
		}
	}

	if( insert_flag )
	{
		// insert new segment and new vertex
		c->seg.SetSize( c->nsegs + 1 );	// add new segment to array
		c->seg[c->nsegs].Initialize( m_dlist, this );
		c->vtx.SetSize( c->nsegs + 2 );	
		c->vtx[c->nsegs+1].Initialize( m_dlist, this );	// add new vertex to array

		// shift higher segments and vertices up to make room
		for( int i=c->nsegs; i>iseg; i-- )
		{
			c->seg[i] = c->seg[i-1];		
			c->vtx[i+1] = c->vtx[i];
		}
		// note that seg[iseg+1] now duplicates seg[iseg], vtx[iseg+2] duplicates vtx[iseg+1]
		c->vtx[iseg+1].selected = 0;
		c->vtx[iseg+1].force_via_flag = 0;
		c->vtx[iseg+1].tee_ID = 0;
		c->vtx[iseg+1].via_hole_w = 0;
		c->vtx[iseg+1].via_w = 0;
		c->vtx[iseg+1].m_uid = nl_cuid.GetNewUID();

		// set position for new vertex
		c->vtx[iseg+1].x = x;
		c->vtx[iseg+1].y = y;
		
		// fill in data for new seg[iseg] or seg[is+1] (depending on dir)
		if( dir == 0 )
		{
			// route forward
			c->seg[iseg].layer = layer;
			c->seg[iseg].width = width;
			c->seg[iseg].selected = 0;
			c->seg[iseg].m_uid = nl_cuid.GetNewUID();
		}
		else
		{
			// route backward
			c->seg[iseg+1].layer = layer;
			c->seg[iseg+1].width = width;
			c->seg[iseg+1].selected = 0;
			c->seg[iseg+1].m_uid = nl_cuid.GetNewUID();
		}
		// done
		c->nsegs++;
	}
	else
	{
		// don't insert, just modify old segment
		c->seg[iseg].selected = 0;
		c->seg[iseg].layer = layer;
		c->seg[iseg].width = width;
	}

	// clean up vias
	if( ReconcileVia( net, ic, iseg, FALSE, via_width, via_hole_width ) == 0 )
	{
		//net->connect[ic].vtx[iseg].via_w = 0;
		//net->connect[ic].vtx[iseg].via_hole_w = 0;
	}
	if( ReconcileVia( net, ic, iseg+1, FALSE, via_width, via_hole_width ) == 0 )
	{
		//net->connect[ic].vtx[iseg+1].via_w = 0;
		//net->connect[ic].vtx[iseg+1].via_hole_w = 0;
	}
	if( (iseg+1) < c->nsegs )
	{
		if( ReconcileVia( net, ic, iseg+2, FALSE, via_width, via_hole_width ) == 0 )
		{
		//	net->connect[ic].vtx[iseg+2].via_w = 0;
		//	net->connect[ic].vtx[iseg+2].via_hole_w = 0;
		}
	}

	// redraw connection
	if( bDrawConnection )
		DrawConnection( net, ic );
	return insert_flag;
}

// Set trace width for routed segment (ignores unrouted segs)
// If w = 0, ignore it
// If via_w = 0, ignore via_w and via_hole_w
//
int CNetList::SetSegmentWidth( cnet * net, int ic, int is, int w, int via_w, int via_hole_w )
{
//	id id;
	cconnect * c = &net->connect[ic];
	if( c->seg[is].layer != LAY_RAT_LINE && w )
	{
		c->seg[is].width = w;
	}
	if( c->vtx[is].via_w && via_w && is )
	{
		c->vtx[is].via_w = via_w;
		c->vtx[is].via_hole_w = via_hole_w;
		DrawVia( net, ic, is );
	}
	if( c->vtx[is+1].via_w && via_w )
	{
		c->vtx[is+1].via_w = via_w;
		c->vtx[is+1].via_hole_w = via_hole_w;
	}
	DrawSegment( net, ic, is );
	return 0;
}

int CNetList::SetConnectionWidth( cnet * net, int ic, int w, int via_w, int via_hole_w )
{
	cconnect * c = &net->connect[ic];
	for( int is=0; is<c->nsegs; is++ )
	{
		SetSegmentWidth( net, ic, is, w, via_w, via_hole_w );
	}
	return 0;
}

int CNetList::SetNetWidth( cnet * net, int w, int via_w, int via_hole_w )
{
	for( int ic=0; ic<net->nconnects; ic++ )
	{
		for( int is=0; is<net->connect[ic].nsegs; is++ )
		{
			SetSegmentWidth( net, ic, is, w, via_w, via_hole_w );
		}
	}
	return 0;
}

// part added, hook up to nets
//
void CNetList::PartAdded( cpart * part )
{
	CString ref_des = part->ref_des;

	// iterate through all nets, hooking up to part
	POSITION pos;
	CString name;
	void * ptr;
	for( pos = m_map.GetStartPosition(); pos != NULL; )
	{
		m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		for( int ip=0; ip<net->npins; ip++ )
		{
			if( net->pin[ip].ref_des == ref_des )
			{
				// found net->pin which attaches to part
				net->pin[ip].part = part;	// set net->pin->part
				if( part->shape )
				{
					for( int pin_index=part->shape->GetPinIndexByName(net->pin[ip].pin_name,-1); 
							 pin_index>=0; 
							 pin_index=part->shape->GetPinIndexByName(net->pin[ip].pin_name,pin_index))
					{
						// hook it up
						part->pin[pin_index].net = net;		// set part->pin->net
					}
				}
			}
		}
	}
}



// Swap 2 connects
//
void CNetList::SwapConnects (cnet * n1, cnet * n2, id id1, id id2)
{
	if (!n1 || !n2)
		return;
	if (n1->name.Compare(n2->name) == 0)
		return;
	if (id1.i >= n1->nconnects ||
		id2.i >= n2->nconnects )
		return;
	if (id1.ii >= n1->connect[id1.i].nsegs ||
		id2.ii >= n2->connect[id2.i].nsegs )
		return;
	int st_pin1 = n1->connect[id1.i].start_pin;
	int stpsh1 =  n1->connect[id1.i].start_pin_shape;
	int st_pin1X = n1->connect[id1.i].vtx[0].x;
	int st_pin1Y = n1->connect[id1.i].vtx[0].y;
	int end_pin1 = n1->connect[id1.i].end_pin;
	int epsh1 = n1->connect[id1.i].end_pin_shape;
	int end_pin1X,
		end_pin1Y;
	if (n1->connect[id1.i].end_pin >= 0)
	{
		end_pin1X = n1->connect[id1.i].vtx[n1->connect[id1.i].nsegs].x;
		end_pin1Y = n1->connect[id1.i].vtx[n1->connect[id1.i].nsegs].y;
	}
	//
	int st_pin2 = n2->connect[id2.i].start_pin;
	int stpsh2 =  n2->connect[id2.i].start_pin_shape;
	int st_pin2X = n2->connect[id2.i].vtx[0].x;
	int st_pin2Y = n2->connect[id2.i].vtx[0].y;
	int end_pin2 = n2->connect[id2.i].end_pin;
	int epsh2 = n2->connect[id2.i].end_pin_shape;
	int end_pin2X, 
		end_pin2Y;
	if (end_pin2 >= 0)
	{
		end_pin2X = n2->connect[id2.i].vtx[n2->connect[id2.i].nsegs].x;
		end_pin2Y = n2->connect[id2.i].vtx[n2->connect[id2.i].nsegs].y;
	}
	int s,e,ssh,esh;
	// for n1
	// direction...
	s = st_pin1;
	ssh = stpsh1;
	e = end_pin1;
	esh = epsh1;
	if (end_pin1 >= 0)
	{
		if (Distance(	n2->connect[id2.i].vtx[0].x,
						n2->connect[id2.i].vtx[0].y,
						st_pin1X,
						st_pin1Y) > Distance(	n2->connect[id2.i].vtx[0].x,
												n2->connect[id2.i].vtx[0].y,
												end_pin1X,
												end_pin1Y) )
		{
			e = st_pin1;
			esh = stpsh1;
			s = end_pin1;
			ssh = epsh1;
			end_pin1X = st_pin1X;
			end_pin1Y = st_pin1Y;
		}
	}
	cpart * prt1 = n1->pin[s].part;
	CPoint Pt = m_plist->GetPinPoint( prt1, ssh, prt1->side, prt1->angle );
	int ic1 = AddNetStub(n1, s);
	n1->connect[ic1].start_pin_shape = ssh;	 
	for (int i=0; i<n2->connect[id2.i].nsegs; i++)
	{
		AppendSegment(n1, ic1,	n2->connect[id2.i].vtx[i+1].x, 
								n2->connect[id2.i].vtx[i+1].y, 
								i?n2->connect[id2.i].seg[i].layer:LAY_RAT_LINE, 
								i?n2->connect[id2.i].seg[i].width:0);
	}
	if (end_pin1 >= 0)
	{
		n1->connect[ic1].vtx[n1->connect[ic1].nsegs].x = end_pin1X;
		n1->connect[ic1].vtx[n1->connect[ic1].nsegs].y = end_pin1Y;
		n1->connect[ic1].seg[n1->connect[ic1].nsegs-1].layer = LAY_RAT_LINE;
		n1->connect[ic1].seg[n1->connect[ic1].nsegs-1].width = 0;
		n1->connect[ic1].end_pin = e;
		n1->connect[ic1].end_pin_shape = esh;
		int lay = m_plist->GetPinLayer( n1->pin[end_pin1].part, esh );
		n1->connect[ic1].vtx[n1->connect[ic1].nsegs].pad_layer = lay;
		DrawConnection( n1, ic1 );
	}
	// for n2
	// direction...
	s = st_pin2;
	ssh = stpsh2;
	e = end_pin2;
	esh = epsh2;
	if (end_pin2 >= 0)
	{
		if (Distance(	n1->connect[id1.i].vtx[0].x,
						n1->connect[id1.i].vtx[0].y,
						st_pin2X,
						st_pin2Y) > Distance(	n1->connect[id1.i].vtx[0].x,
												n1->connect[id1.i].vtx[0].y,
												end_pin2X,
												end_pin2Y) )
		{
			e = st_pin2;
			esh = stpsh2;
			s = end_pin2;
			ssh = epsh2;
			end_pin2X = st_pin2X;
			end_pin2Y = st_pin2Y;
		}
	}
	cpart * prt2 = n2->pin[s].part;
	Pt = m_plist->GetPinPoint( prt2, ssh, prt1->side, prt1->angle );
	int ic2 = AddNetStub(n2, s, Pt.x, Pt.y);
	n2->connect[ic2].start_pin_shape = ssh;
	for (int i=0; i<n1->connect[id1.i].nsegs; i++)
	{
		AppendSegment(n2, ic2,	n1->connect[id1.i].vtx[i+1].x, 
								n1->connect[id1.i].vtx[i+1].y, 
								i?n1->connect[id1.i].seg[i].layer:LAY_RAT_LINE, 
								i?n1->connect[id1.i].seg[i].width:0);
	}
	if (end_pin2 >= 0)
	{
		n2->connect[ic2].vtx[n2->connect[ic2].nsegs].x = end_pin2X;
		n2->connect[ic2].vtx[n2->connect[ic2].nsegs].y = end_pin2Y;
		n2->connect[ic2].seg[n2->connect[ic2].nsegs-1].layer = LAY_RAT_LINE;
		n2->connect[ic2].seg[n2->connect[ic2].nsegs-1].width = 0;
		n2->connect[ic2].end_pin = e;
		n2->connect[ic2].end_pin_shape = esh;
		int lay = m_plist->GetPinLayer( n2->pin[end_pin2].part, esh );
		n2->connect[ic2].vtx[n2->connect[ic2].nsegs].pad_layer = lay;
		DrawConnection( n2, ic2 );
	}
	int rem1 = RemoveNetConnect( n1, id1.i, FALSE );
	int rem2 = RemoveNetConnect( n2, id2.i, FALSE ); 
}



// Swap 2 pins
//
void CNetList::SwapPins( cpart * part1, CString * pin_name1,
						cpart * part2, CString * pin_name2 )
{
	// get pin1 info
	int pin_index1 = part1->shape->GetPinIndexByName( *pin_name1, -1 );
	CPoint pin_pt1 = m_plist->GetPinPoint( part1, pin_index1, part1->side, part1->angle );
	int pin_lay1 = m_plist->GetPinLayer( part1, pin_index1 );
	cnet * net1 = m_plist->GetPinNet( part1, pin_name1 );
	int net1_pin_index = -1;
	if( net1 )
	{
		net1_pin_index = GetNetPinIndex( net1, &part1->ref_des, pin_name1 );
	}

	// get pin2 info
	int pin_index2 = part2->shape->GetPinIndexByName( *pin_name2, -1 );
	CPoint pin_pt2 = m_plist->GetPinPoint( part2, pin_index2, part2->side, part2->angle );
	int pin_lay2 = m_plist->GetPinLayer( part2, pin_index2 );
	cnet * net2 = m_plist->GetPinNet( part2, pin_name2 );
	int net2_pin_index = -1;
	if( net2 )
	{
		// find net2_pin_index for part2->pin2
		net2_pin_index = GetNetPinIndex( net2, &part2->ref_des, pin_name2 );
	}

	if( net1 == NULL && net2 == NULL )
	{
		// both pins unconnected, there is nothing to do
		return;
	}
	else if( net1 == net2 )
	{
		// both pins on same net
		for( int ic=0; ic<net1->nconnects; ic++ )
		{
			cconnect * c = &net1->connect[ic];
			int p1 = c->start_pin;
			int p2 = c->end_pin;
			int nsegs = c->nsegs;
			if( nsegs )
			{
				if( p1 == net1_pin_index && p2 == net2_pin_index )
					continue;
				else if( p1 == net2_pin_index && p2 == net1_pin_index )
					continue;
				BOOL ReDraw = FALSE;
				if( p1 == net1_pin_index )
				{
					// starting pin is on part, unroute first segment
					if( c->seg[0].layer != LAY_RAT_LINE )
					{
						c->seg[0].width = 0;
						c->seg[0].layer = LAY_RAT_LINE;
						MergeUnroutedSegments( net1, ic );
						nsegs = c->nsegs;
					}
					// modify vertex position and layer
					c->vtx[0].x = pin_pt2.x;
					c->vtx[0].y = pin_pt2.y;
					c->vtx[0].pad_layer = pin_lay2;
					c->seg[0].dl_el->visible = net1->visible;
					ReDraw = TRUE;
				}
				if( p2 == net1_pin_index )
				{
					// ending pin is on part, unroute last segment
					if( c->seg[nsegs-1].layer != LAY_RAT_LINE )
					{
						c->seg[nsegs-1].width = 0;
						c->seg[nsegs-1].layer = LAY_RAT_LINE;
						MergeUnroutedSegments( net1, ic );
						nsegs = c->nsegs;
					}
					// modify vertex position and layer
					c->vtx[nsegs].x = pin_pt2.x;
					c->vtx[nsegs].y = pin_pt2.y;
					c->vtx[nsegs].pad_layer = pin_lay2;
					c->seg[nsegs-1].dl_el->visible = net1->visible;
					ReDraw = TRUE;
				}
				if( p1 == net2_pin_index )
				{
					// starting pin is on part, unroute first segment
					if( c->seg[0].layer != LAY_RAT_LINE )
					{
						c->seg[0].width = 0;
						c->seg[0].layer = LAY_RAT_LINE;
						MergeUnroutedSegments( net2, ic );
						nsegs = c->nsegs;
					}
					// modify vertex position and layer
					c->vtx[0].x = pin_pt1.x;
					c->vtx[0].y = pin_pt1.y;
					c->vtx[0].pad_layer = pin_lay1;
					c->seg[0].dl_el->visible = net2->visible;
					ReDraw = TRUE;
				}
				if( p2 == net2_pin_index )
				{
					// ending pin is on part, unroute last segment
					if( c->seg[nsegs-1].layer != LAY_RAT_LINE )
					{
						c->seg[nsegs-1].width = 0;
						c->seg[nsegs-1].layer = LAY_RAT_LINE;
						MergeUnroutedSegments( net2, ic );
						nsegs = c->nsegs;
					}
					// modify vertex position and layer
					c->vtx[nsegs].x = pin_pt1.x;
					c->vtx[nsegs].y = pin_pt1.y;
					c->vtx[nsegs].pad_layer = pin_lay1;
					c->seg[nsegs-1].dl_el->visible = net2->visible;
					ReDraw = TRUE;
				}
				if( ReDraw )
					DrawConnection( net1, ic );
			}
		}
		// reassign pin1
		net2->pin[net2_pin_index].pin_name = *pin_name1;
		// reassign pin2
		net1->pin[net1_pin_index].pin_name = *pin_name2;
		SetAreaConnections( net1 );
		return;
	}

	// now move all part1->pin1 connections to part2->pin2
	// change part2->pin2->net to net1
	// change net1->pin->part to part2
	// change net1->pin->ref_des to part2->ref_des
	// change net1->pin->pin_name to pin_name2
	if( net1 )
	{
		// remove any stub traces with one segment
		for( int ic=0; ic<net1->nconnects; ic++ )
		{
			int nsegs = net1->connect[ic].nsegs;
			if( nsegs == 1)
			{
				int p1 = net1->connect[ic].start_pin;
				int p2 = net1->connect[ic].end_pin;
				if( p1 == net1_pin_index && p2 == cconnect::NO_END )
				{
					// stub trace with 1 segment, remove it
					RemoveNetConnect( net1, ic, FALSE );
					ic--;
					continue;	// next connection
				}
			}
		}
		// now check all connections
		for( int ic=0; ic<net1->nconnects; ic++ )
		{
			int nsegs = net1->connect[ic].nsegs;
			if( nsegs )
			{
				int p1 = net1->connect[ic].start_pin;
				int p2 = net1->connect[ic].end_pin;
				BOOL ReDraw = FALSE;
				if( p1 == net1_pin_index )
				{
					// starting pin is on part, unroute first segment
					if( net1->connect[ic].seg[0].layer != LAY_RAT_LINE )
					{
						net1->connect[ic].seg[0].width = 0;
						net1->connect[ic].seg[0].layer = LAY_RAT_LINE;
						MergeUnroutedSegments( net1, ic );
						nsegs = net1->connect[ic].nsegs;
					}
					// modify vertex position and layer
					net1->connect[ic].start_pin_shape = pin_index2;
					net1->connect[ic].vtx[0].x = pin_pt2.x;
					net1->connect[ic].vtx[0].y = pin_pt2.y;
					net1->connect[ic].vtx[0].pad_layer = pin_lay2;
					net1->connect[ic].seg[0].dl_el->visible = net1->visible;
					ReDraw = TRUE;
				}
				if( p2 == net1_pin_index )
				{
					// ending pin is on part, unroute last segment
					if( net1->connect[ic].seg[nsegs-1].layer != LAY_RAT_LINE )
					{
						net1->connect[ic].seg[nsegs-1].width = 0;
						net1->connect[ic].seg[nsegs-1].layer = LAY_RAT_LINE;
						MergeUnroutedSegments( net1, ic );
						nsegs = net1->connect[ic].nsegs;
					}
					// modify vertex position and layer
					net1->connect[ic].end_pin_shape = pin_index2;
					net1->connect[ic].vtx[nsegs].x = pin_pt2.x;
					net1->connect[ic].vtx[nsegs].y = pin_pt2.y;
					net1->connect[ic].vtx[nsegs].pad_layer = pin_lay2;
					net1->connect[ic].seg[nsegs-1].dl_el->visible = net1->visible;
					ReDraw = TRUE;
				}
				if( ReDraw )
					DrawConnection( net1, ic );
			}
		}
		// reassign pin2 to net1
		net1->pin[net1_pin_index].part = part2;
		net1->pin[net1_pin_index].ref_des = part2->ref_des;
		net1->pin[net1_pin_index].pin_name = *pin_name2;
		for( int get_index=part2->shape->GetPinIndexByName( *pin_name2, -1 ); get_index>=0; get_index=part2->shape->GetPinIndexByName( *pin_name2, get_index ) )
			part2->pin[get_index].net = net1;
	}
	else
	{
		// pin2 is unconnected
		for( int get_index=part2->shape->GetPinIndexByName( *pin_name2, -1 ); get_index>=0; get_index=part2->shape->GetPinIndexByName( *pin_name2, get_index ) )
			part2->pin[get_index].net = NULL;
	}
	// now move all part2->pin2 connections to part1->pin1
	// change part1->pin1->net to net2
	// change net2->pin->part to part1
	// change net2->pin->ref_des to part1->ref_des
	// change net2->pin->pin_name to pin_name1
	if( net2 )
	{
		// second pin is connected
		// remove any stub traces with one segment
		for( int ic=0; ic<net2->nconnects; ic++ )
		{
			int nsegs = net2->connect[ic].nsegs;
			if( nsegs == 1 )
			{
				int p1 = net2->connect[ic].start_pin;
				int p2 = net2->connect[ic].end_pin;
				if( p1 == net2_pin_index && p2 == cconnect::NO_END )
				{
					// stub trace with 1 segment, remove it
					RemoveNetConnect( net2, ic, FALSE );
					ic--;
					continue;
				}
			}
		}
		// now check all connections
		for( int ic=0; ic<net2->nconnects; ic++ )
		{
			int nsegs = net2->connect[ic].nsegs;
			if( nsegs )
			{
				int p1 = net2->connect[ic].start_pin;
				int p2 = net2->connect[ic].end_pin;
				BOOL ReDraw = FALSE;
				if( p1 == net2_pin_index )
				{
					// starting pin is on part, unroute first segment
					if( net2->connect[ic].seg[0].layer != LAY_RAT_LINE )
					{
						net2->connect[ic].seg[0].width = 0;
						net2->connect[ic].seg[0].layer = LAY_RAT_LINE;
						MergeUnroutedSegments( net2, ic );
						nsegs = net2->connect[ic].nsegs;
					}
					// modify vertex position and layer
					net2->connect[ic].start_pin_shape = pin_index1;
					net2->connect[ic].vtx[0].x = pin_pt1.x;
					net2->connect[ic].vtx[0].y = pin_pt1.y;
					net2->connect[ic].vtx[0].pad_layer = pin_lay1;
					net2->connect[ic].seg[0].dl_el->visible = net2->visible;
					ReDraw = TRUE;
				}
				if( p2 == net2_pin_index )
				{
					// ending pin is on part, unroute last segment
					if( net2->connect[ic].seg[nsegs-1].layer != LAY_RAT_LINE )
					{
						net2->connect[ic].seg[nsegs-1].width = 0;
						net2->connect[ic].seg[nsegs-1].layer = LAY_RAT_LINE;
						MergeUnroutedSegments( net2, ic );
						nsegs = net2->connect[ic].nsegs;
					}
					// modify vertex position and layer
					net2->connect[ic].end_pin_shape = pin_index1;
					net2->connect[ic].vtx[nsegs].x = pin_pt1.x;
					net2->connect[ic].vtx[nsegs].y = pin_pt1.y;
					net2->connect[ic].vtx[nsegs].pad_layer = pin_lay1;
					net2->connect[ic].seg[nsegs-1].dl_el->visible = net2->visible;
					ReDraw = TRUE;
				}
				if( ReDraw )
					DrawConnection( net2, ic );
			}
		}
		// reassign pin1 to net2
		net2->pin[net2_pin_index].part = part1;
		net2->pin[net2_pin_index].ref_des = part1->ref_des;
		net2->pin[net2_pin_index].pin_name = *pin_name1;
		for( int get_index=part1->shape->GetPinIndexByName( *pin_name1, -1 ); get_index>=0; get_index=part1->shape->GetPinIndexByName( *pin_name1, get_index ) )
			part1->pin[get_index].net = net2;
	}
	else
	{
		// pin2 is unconnected
		for( int get_index=part1->shape->GetPinIndexByName( *pin_name1, -1 ); get_index>=0; get_index=part1->shape->GetPinIndexByName( *pin_name1, get_index ) )
			part1->pin[get_index].net = NULL;
	}
	SetAreaConnections( net1 );
	SetAreaConnections( net2 );
}


// Part moved, so unroute starting and ending segments of connections
// to this part, and update positions of endpoints
// Undraw and Redraw any changed connections
// 
int CNetList::PartMoved( cpart * part , BOOL UNROUTE_SEGMENTS, BOOL bDraw )
{
	// first, mark all nets and connections unmodified
	cnet * net;
	net = GetFirstNet();
	while( net )
	{
		net->utility = 0;
		for( int ic=0; ic<net->nconnects; ic++ )
			net->connect[ic].utility = 0;
		net = GetNextNet(/*LABEL*/);
	}
	// disable drawing/undrawing 
	CDisplayList * old_dlist = m_dlist;
	m_dlist = 0;

	// find nets that connect to this part
	for( int ip=0; ip<part->shape->m_padstack.GetSize(); ip++ ) 
	{
		net = (cnet*)part->pin[ip].net;
		if( net && net->utility == 0 )
		{
			for( int ic=0; ic<net->nconnects; ic++ )
			{
				cconnect * c = &net->connect[ic];
				int nsegs = c->nsegs;
				if( nsegs )
				{
					// check this connection
					int p1 = c->start_pin;
					CString pin_name1 = net->pin[p1].pin_name;
					int pin_index1 = c->start_pin_shape;//GetPinIndexByNameForPart( part, pin_name1, c->vtx[0].x, c->vtx[0].y );
					int p2 = c->end_pin;
					cseg * s0 = &c->seg[0];
					cvertex * v0 = &c->vtx[0];
					if( net->pin[p1].part == part )
					{
						// start pin is on part, unroute first segment
						net->utility = 1;	// mark net modified
						c->utility = 1;		// mark connection modified
						if ( UNROUTE_SEGMENTS &&
							 net->connect[ic].seg[0].selected == 0 &&
							 net->connect[ic].vtx[0].selected == 0)
						{
								net->connect[ic].seg[0].width = 0;
								net->connect[ic].seg[0].layer = LAY_RAT_LINE;
								MergeUnroutedSegments( net, ic );
						}
						nsegs = c->nsegs;
						// modify vertex[0] position and layer
						v0->x = part->pin[pin_index1].x;
						v0->y = part->pin[pin_index1].y;
						if( part->shape->m_padstack[pin_index1].hole_size )
						{
							// through-hole pad
							v0->pad_layer = LAY_PAD_THRU;
						}
						else if( part->side == 0 && part->shape->m_padstack[pin_index1].top.shape != PAD_NONE
							|| part->side == 1 && part->shape->m_padstack[pin_index1].bottom.shape != PAD_NONE )
						{
							// SMT pad on top
							v0->pad_layer = LAY_TOP_COPPER;
						}
						else
						{
							// SMT pad on bottom
							v0->pad_layer = LAY_BOTTOM_COPPER;
						}
						if( part->pin[pin_index1].net != net )
							part->pin[pin_index1].net = net;
					}
					if( p2 != cconnect::NO_END )
					{
						if( net->pin[p2].part == part )
						{
							// end pin is on part, unroute last segment
							net->utility = 1;	// mark net modified
							c->utility = 1;		// mark connection modified
							if (UNROUTE_SEGMENTS &&
								net->connect[ic].seg[nsegs-1].selected == 0 &&
								net->connect[ic].vtx[nsegs].selected == 0) 
							{
									net->connect[ic].seg[nsegs-1].width = 0;
									net->connect[ic].seg[nsegs-1].layer = LAY_RAT_LINE;
									MergeUnroutedSegments( net, ic );
							}
							nsegs = c->nsegs;
							// modify vertex position and layer
							CString pin_name2 = net->pin[p2].pin_name;
							int pin_index2 = c->end_pin_shape;//GetPinIndexByNameForPart( part, pin_name2, c->vtx[c->nsegs].x, c->vtx[c->nsegs].y );
							c->vtx[nsegs].x = part->pin[pin_index2].x;
							c->vtx[nsegs].y = part->pin[pin_index2].y;
							if( part->shape->m_padstack[pin_index2].hole_size )
							{
								// through-hole pad
								c->vtx[nsegs].pad_layer = LAY_PAD_THRU;
							}
							else if( part->side == 0 && part->shape->m_padstack[pin_index2].top.shape != PAD_NONE 
								|| part->side == 0 && part->shape->m_padstack[pin_index2].top.shape != PAD_NONE )
							{
								// SMT pad, part on top
								c->vtx[nsegs].pad_layer = LAY_TOP_COPPER;
							}
							else
							{
								// SMT pad, part on bottom
								c->vtx[nsegs].pad_layer = LAY_BOTTOM_COPPER;
							}
							if( part->pin[pin_index2].net != net )
								part->pin[pin_index2].net = net;
						}
					}
				}
			}
		}
	}
	// now redraw connections
	m_dlist = old_dlist;
	if( m_dlist && bDraw )
	{
		cnet * net;
		net = GetFirstNet();
		while( net )
		{
			if( net->utility )
			{
				for( int ic=0; ic<net->nconnects; ic++ )
				{
					if( net->connect[ic].utility )
						DrawConnection( net, ic );
				}
			}
			net = GetNextNet(/*LABEL*/);
		}
	}
	return 0;
}

// Part footprint changed, check new pins and positions
// If changed, unroute starting and ending segments of connections
// to this part, and update positions of endpoints
// 
int CNetList::PartFootprintChanged( cpart * part )
{
	POSITION pos;
	CString name;
	void * ptr;

	// first, clear existing net assignments to part pins
	for( int ip=0; ip<part->pin.GetSize(); ip++ )
		part->pin[ip].net = NULL;

	// find nets which connect to this part
	cnet * net = GetFirstNet();
	while( net )
	{
		// check each connection in net
		for( int ic=net->nconnects-1; ic>=0; ic-- )
		{
			cconnect * c = &net->connect[ic];
			int nsegs = c->nsegs;
			if( nsegs )
			{
				int p1 = c->start_pin;
				int p2 = c->end_pin;
				if( net->pin[p1].part != part )
				{
					// connection doesn't start on this part
					if( p2 == cconnect::NO_END )
						continue; // stub trace, ignore it
					if( net->pin[p2].part != part )
						continue;	// doesn't end on part, ignore it
				}
				CString pin_name1 = net->pin[p1].pin_name;
				if( net->pin[p1].part == part )
				{
					int old_x = c->vtx[0].x;
					int old_y = c->vtx[0].y;
					// starting pin is on part, see if this pin still exists
					int pin_index1 = GetPinIndexByNameForPart( part, pin_name1, old_x, old_y );
					if( pin_index1 == -1 )
					{
						// no, remove connection
						RemoveNetConnect( net, ic, FALSE );
						continue;
					}
					// yes, rehook pin to net
					part->pin[pin_index1].net = net;
					// update start_pin_shape
					c->start_pin_shape = pin_index1;
					// see if position or pad type has changed	
					int old_layer = c->seg[0].layer;
					int new_x = part->pin[pin_index1].x;
					int new_y = part->pin[pin_index1].y;
					int new_layer = LAY_TOP_COPPER;
					if( part->shape->m_padstack[pin_index1].hole_size )
						new_layer = LAY_PAD_THRU;
					else if( part->side == 0 )
					{
						if( part->shape->m_padstack[pin_index1].top.shape )
							new_layer = LAY_TOP_COPPER;
						else if( part->shape->m_padstack[pin_index1].bottom.shape )
							new_layer = LAY_BOTTOM_COPPER;
					}
					else
					{
						if( part->shape->m_padstack[pin_index1].top.shape )
							new_layer = LAY_BOTTOM_COPPER;
						else if( part->shape->m_padstack[pin_index1].bottom.shape )
							new_layer = LAY_TOP_COPPER;
					}
					c->vtx[0].pad_layer = new_layer;
					BOOL layer_ok = (new_layer == old_layer) || (part->shape->m_padstack[pin_index1].hole_size > 0);
					// see if pin position has changed
					if( old_x != new_x || old_y != new_y || !layer_ok )
					{
						
						// yes, unroute if necessary and update connection
						if( old_layer != LAY_RAT_LINE && !layer_ok )
						{
							net->connect[ic].seg[0].width = 0;
							net->connect[ic].seg[0].layer = LAY_RAT_LINE;
							MergeUnroutedSegments( net, ic );
							nsegs = c->nsegs;
						}
						// modify vertex position
						c->vtx[0].x = new_x;
						c->vtx[0].y = new_y;
						c->seg[0].dl_el->visible = net->visible;
						DrawSegment( net, ic, 0 );
					}
				}
				if( p2 == cconnect::NO_END )
					continue;
				CString pin_name2 = net->pin[p2].pin_name;
				if( net->pin[p2].part == part )
				{
					int old_x = c->vtx[nsegs].x;
					int old_y = c->vtx[nsegs].y;
					// ending pin is on part, see if this pin still exists
					int pin_index2 = GetPinIndexByNameForPart( part, pin_name2, old_x, old_y );
					if( pin_index2 == -1 )
					{
						// no, remove connection
						RemoveNetConnect( net, ic, FALSE );
						continue;
					}
					// rehook pin to net
					part->pin[pin_index2].net = net;
					// update end_pin_shape
					c->end_pin_shape = pin_index2;
					// see if position has changed	
					int old_layer = c->seg[nsegs-1].layer;
					int new_x = part->pin[pin_index2].x;
					int new_y = part->pin[pin_index2].y;
					int new_layer = LAY_TOP_COPPER;
					if( part->shape->m_padstack[pin_index2].hole_size )
						new_layer = LAY_PAD_THRU;
					else if( part->side == 0 )
					{
						if( part->shape->m_padstack[pin_index2].top.shape )
							new_layer = LAY_TOP_COPPER;
						else if( part->shape->m_padstack[pin_index2].bottom.shape )
							new_layer = LAY_BOTTOM_COPPER;
					}
					else
					{
						if( part->shape->m_padstack[pin_index2].top.shape )
							new_layer = LAY_BOTTOM_COPPER;
						else if( part->shape->m_padstack[pin_index2].bottom.shape )
							new_layer = LAY_TOP_COPPER;
					}
					c->vtx[nsegs].pad_layer = new_layer;
					BOOL layer_ok = (new_layer == old_layer) || (part->shape->m_padstack[pin_index2].hole_size > 0);
					if( old_x != new_x || old_y != new_y || !layer_ok )
					{
						
						// yes, unroute if necessary and update connection
						if( c->seg[nsegs-1].layer != LAY_RAT_LINE && !layer_ok )
						{
							net->connect[ic].seg[nsegs-1].width = 0;
							net->connect[ic].seg[nsegs-1].layer = LAY_RAT_LINE;
							MergeUnroutedSegments( net, ic );
							nsegs = c->nsegs;
						}
						// modify vertex position
						c->vtx[nsegs].x = new_x;
						c->vtx[nsegs].y = new_y;
						c->seg[nsegs-1].dl_el->visible = net->visible;
						DrawSegment( net, ic, nsegs-1 );
					}
				}
			}
		}
		// now see if new connections need to be added
		for( int ip=0; ip<net->pin.GetSize(); ip++ )
		{
			if( net->pin[ip].ref_des == part->ref_des )
			{
				for( int pin_index=part->shape->GetPinIndexByName(net->pin[ip].pin_name,-1);
						 pin_index>=0;
						 pin_index=part->shape->GetPinIndexByName(net->pin[ip].pin_name,pin_index))
				{
					// pin exists, see if connected to net
					if( part->pin[pin_index].net != net || net->pin[ip].part != part )
					{
						// no, make connection
						part->pin[pin_index].net = net;
						net->pin[ip].part = part;
					}
				}
			}
		}
		RemoveOrphanBranches( net, 0 );
		net = GetNextNet(/*LABEL*/);
	}
	return 0;
}

// Part deleted, so unroute and remove all connections to this part
// and remove all references from netlist
// 
int CNetList::PartDeleted( cpart * part, BOOL bSetAreas )
{
	// find nets which connect to this part, remove pins and adjust areas
	POSITION pos;
	CString name;
	void * ptr;

	for( cnet * net=GetFirstNet(); net; net=GetNextNet(/*LABEL*/) )
		net->utility = 0;


	// find nets which connect to this part
	for( pos = m_map.GetStartPosition(); pos != NULL; )
	{
		m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		for( int ip=0; ip<net->pin.GetSize(); )
		{
			if( net->pin[ip].ref_des == part->ref_des )
			{
				RemoveNetPin( net, &net->pin[ip].ref_des, &net->pin[ip].pin_name, FALSE );
				net->utility = 1;	// mark for SetAreaConnections
			}
			else
				ip++;
		}
		RemoveOrphanBranches( net, 0 );
	}
	if( bSetAreas )
	{
		for( cnet * net=GetFirstNet(); net; net=GetNextNet(/*LABEL*/) )
			if( net->utility )
				SetAreaConnections( net );
	}
	return 0;
}

// Part reference designator changed
// replace all references from netlist
// 
int CNetList::PartRefChanged( CString * old_ref_des, CString * new_ref_des )
{
	// find nets which connect to this part, adjust pin names
	POSITION pos;
	CString name;
	void * ptr;
	for( pos = m_map.GetStartPosition(); pos != NULL; )
	{
		m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		if( net )
			for( int ip=0; ip<net->pin.GetSize(); ip++ )
			{
				if( net->pin[ip].ref_des == *new_ref_des )
					return 1; //fail
			}
	}
	// find nets which connect to this part
	for( pos = m_map.GetStartPosition(); pos != NULL; )
	{
		m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		for( int ip=0; ip<net->pin.GetSize(); ip++ )
		{
			if( net->pin[ip].ref_des == *old_ref_des )
				net->pin[ip].ref_des = *new_ref_des;
		}
	}
	return 0;
}

int CNetList::PartCheckConnect( cpart * part )
{
	POSITION pos;
	CString name;
	void * ptr;
	for( pos = m_map.GetStartPosition(); pos != NULL; )
	{
		m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		for( int ip=0; ip<net->pin.GetSize(); ip++ )
			if( net->pin[ip].part == part )
				return TRUE;
	}
	return FALSE;
}

// Part disconnected, so unroute and remove all connections to this part
// Also remove net pointers from part->pins
// and part pointer from net->pins
// Do not remove pins from netlist, however
// 
int CNetList::PartDisconnected( cpart * part, BOOL bSetAreas )
{
	return PartDeleted( part, bSetAreas );
}

// utility function used by OptimizeConnections()
//
void AddPinsToGrid( char * grid, int p1, int p2, int npins )
{
#define GRID(a,b) grid[a*npins+b]
#define COPY_ROW(a,b) for(int k=0;k<npins;k++){ if( GRID(a,k) ) GRID(b,k) = 1; }

	// add p2 to row p1
	GRID(p1,p2) = 1;
	// now copy row p2 into p1
	COPY_ROW(p2,p1);
	// now copy row p1 into each row connected to p1
	for( int ip=0; ip<npins; ip++ )
		if( GRID(p1,ip) )
			COPY_ROW(p1, ip);
}

// optimize all unrouted connections
//
void CNetList::OptimizeConnections( BOOL bBelowPinCount, int pin_count, BOOL bVisibleNetsOnly )
{
	// traverse map
	POSITION pos;
	CString name;
	void * ptr;
	for( pos = m_map.GetStartPosition(); pos != NULL; )
	{
		m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		if( !bVisibleNetsOnly || net->visible )
			OptimizeConnections( net, -1, bBelowPinCount, pin_count, bVisibleNetsOnly );
	}
}

// optimize all unrouted connections for a part
//
void CNetList::OptimizeConnections( cpart * part, BOOL bBelowPinCount, int pin_count, BOOL bVisibleNetsOnly )
{
	MarkAllNets(0);
	// find nets which connect to this part
	cnet * net;
	if( part->shape )
	{
		// mark all nets unoptimized
		for( int ip=0; ip<part->shape->m_padstack.GetSize(); ip++ )
		{
			net = (cnet*)part->pin[ip].net;
			if( net )
				net->utility = 1;
		}
		// optimize each net and mark it optimized so it won't be repeated
		for( int ip=0; ip<part->shape->m_padstack.GetSize(); ip++ )
		{
			net = (cnet*)part->pin[ip].net;
			if( net )
			{
				if( net->utility == 1 )
				{
					OptimizeConnections( net, -1, bBelowPinCount, pin_count, bVisibleNetsOnly );
					net->utility = 0;
				}
			}
		}
	}
}

// optimize the unrouted connections for a net
// if ic_track >= 0, returns new ic corresponding to old ic or -1 if unable
//
int CNetList::OptimizeConnections( cnet * net, int ic_track, BOOL bBelowPinCount, 
								  int pin_count, BOOL bVisibleNetsOnly )
{
#ifdef PROFILE
	StartTimer();	//****
#endif
	if( !net )
		return ic_track;

	// set merge
	for (int icon=0; icon<net->nconnects; icon++)
	{
		cconnect * c = &net->connect[icon];
		c->m_merge = -1;
	}
	BOOL bTraceFound;
	do{
		bTraceFound = FALSE;
		for (int icon=0; icon<net->nconnects; icon++)
		{
			cconnect * c = &net->connect[icon];
			if( c->m_merge >= 0 )
				continue;
			else if( c->end_pin >= 0 )
			{
				int m1 = net->pin[c->start_pin].part->m_merge;
				int m2 = net->pin[c->end_pin].part->m_merge;
				if( m1 == m2 && m1 >= 0 )
				{
					c->m_merge = m1;
					bTraceFound = TRUE;
				}
			}
			else if( c->vtx[c->nsegs].tee_ID )
			{
				int IC=0, IV=0;
				// find header
				if( FindTeeVertexInNet( net, c->vtx[c->nsegs].tee_ID, &IC, &IV ) )
				{
					int m1 = net->pin[c->start_pin].part->m_merge;
					int m2 = net->connect[IC].m_merge;
					if( m1 == m2 && m1 >= 0 )
					{
						c->m_merge = m1;
						bTraceFound = TRUE;
					}
				}
			}
			else if( net->pin[c->start_pin].part->m_merge >= 0 )
			{
				c->m_merge = net->pin[c->start_pin].part->m_merge;
				bTraceFound = TRUE;
			}
		}
	}while( bTraceFound );

	// see if we need to do this
	if( bVisibleNetsOnly && net->visible == 0 )
		return ic_track;
	if( bBelowPinCount && net->npins >= pin_count )
		return ic_track;
	if (!m_dlist->m_vis[LAY_RAT_LINE])
		return ic_track;

	// get number of pins N and make grid[NxN] array and pair[N*2] array
	int npins = net->npins;
	char * grid = (char*)calloc( npins*npins, sizeof(char) );
	for( int ip=0; ip<npins; ip++ )
		grid[ip*npins+ip] = 1;
	CArray<int> pair;			// use collection class because size is unknown,
	pair.SetSize( 2*npins );	// although this should be plenty

	// first, flag ic_track if requested
	int ic_new = -1;
	if( ic_track >= 0 && ic_track < net->nconnects )
	{
		for( int ic=0; ic<net->nconnects; ic++ )
			net->connect[ic].utility = 0;
		net->connect[ic_track].utility = 1;		// flag connection
	}

	// go through net, deleting unrouted and unlocked connections
	// and recording pins of routed or locked connections
	for( int ic=0; ic<net->nconnects; /* ic++ is deferred */ )
	{
		cconnect * c = &net->connect[ic];
		int routed = 0;
		if( c->nsegs )
		{
			if( c->nsegs > 1 
				|| c->seg[0].layer != LAY_RAT_LINE 
				|| c->end_pin == cconnect::NO_END )
				routed = 1;
			int p1, p2;
			if( routed || c->locked || c->utility || c->seg[0].selected )
			{
				// routed or locked...don't delete connection
				// record pins in pair[] and grid[]
				p1 = c->start_pin;
				p2 = c->end_pin;
				if( p2 != cconnect::NO_END )
				{
					AddPinsToGrid( grid, p1, p2, npins );
				}
				// if a stub, record connection to tee
				else
				{
					if( int id = c->vtx[c->nsegs].tee_ID )
					{
						int ic=0;
						int iv=0;
						BOOL bFound = FindTeeVertexInNet( net, id, &ic, &iv );
						if( !bFound )
							RemoveOrphanBranches( net, id );
						else
						{
						// get start of tee trace
						cconnect * tee_c = &net->connect[ic];
						int tee_p1 = tee_c->start_pin;
						AddPinsToGrid( grid, p1, tee_p1, npins );
						}
					}
				}
				// increment counter
				ic++;
			}
			else
			{
				// unrouted and unlocked, so delete connection
				// don't advance ic or n_routed
				RemoveNetConnect( net, ic, FALSE );
			}
		}
		else ic++;
	}

	// find ic_track if still present
	if( ic_track >= 0 )
	{
		for( int ic=0; ic<net->nconnects; ic++ )
		{
			if( net->connect[ic].utility == 1 )
			{
				ic_new = ic;
				break;
			}
		}
	}

	// now add pins connected to copper areas
	for( int ia=0; ia<net->nareas; ia++ )
	{
		SetAreaConnections( net, ia );
	}
	for( int ia=0; ia<net->nareas; ia++ )
	{	
		BOOL FF = FALSE;
		RECT r = net->area[ia].poly->GetBounds();
		int alay = net->area[ia].poly->GetLayer();
		double s = abs((double)(r.right-r.left)*(double)(r.top-r.bottom));
		for( int ip=net->area[ia].npins-1; ip>=0; ip-- )
		{
			FF = FALSE;
			cpart * PRT = net->pin[net->area[ia].pin[ip]].part;
			CString PIN = net->pin[net->area[ia].pin[ip]].pin_name;			
			for( int na2=0; na2<net->nareas; na2++ )
			{
				if( ia != na2 && alay == net->area[na2].poly->GetLayer() )
				{
					for( int ip2=net->area[na2].npins-1; ip2>=0; ip2-- )
					{
						cpart * PRT2 = net->pin[net->area[na2].pin[ip2]].part;
						CString PIN2 = net->pin[net->area[na2].pin[ip2]].pin_name;
						if( PRT == PRT2 )
							if( PIN.Compare(PIN2) == 0 )
							{
								RECT r2 = net->area[na2].poly->GetBounds();
								double s2 = abs((double)(r2.right-r2.left)*(double)(r2.top-r2.bottom));
								if( s2 < s )
								{
									if( net->area[ia].dl_thermal[ip]->dlist )
										net->area[ia].dl_thermal[ip]->dlist->Remove( net->area[ia].dl_thermal[ip] );
									net->area[ia].dl_thermal.RemoveAt(ip);
									net->area[ia].pin.RemoveAt(ip);
									net->area[ia].npins--;
									FF = TRUE;
									break;
								}
							}
					}
					if( FF )
						break;
				}
			}
		}
		for( int iv=net->area[ia].nvias-1; iv>=0; iv-- )
		{
			FF = FALSE;
			int CON = net->area[ia].vcon[iv];
			int VTX = net->area[ia].vtx[iv];
			for( int na2=0; na2<net->nareas; na2++ )
			{
				if( ia != na2 && net->area[na2].poly->GetW() < 0 && alay == net->area[na2].poly->GetLayer() )
				{
					for( int iv2=net->area[na2].nvias-1; iv2>=0; iv2-- )
					{
						int CON2 = net->area[na2].vcon[iv2];
						int VTX2 = net->area[na2].vtx[iv2];
						if( CON == CON2 )
							if( VTX == VTX2 )
							{
								RECT r2 = net->area[na2].poly->GetBounds();
								double s2 = abs((double)(r2.right-r2.left)*(double)(r2.top-r2.bottom));
								if( s2 < s )
								{
									if( net->area[ia].dl_via_thermal[iv]->dlist )
										net->area[ia].dl_via_thermal[iv]->dlist->Remove( net->area[ia].dl_via_thermal[iv] );
									net->area[ia].dl_via_thermal.RemoveAt(iv);
									net->area[ia].vcon.RemoveAt(iv);
									net->area[ia].vtx.RemoveAt(iv);
									net->area[ia].nvias--;
									FF = TRUE;
									break;
								}
							}
					}
					if( FF )
						break;
				}
			}
		}
	}
	for( int ia=0; ia<net->nareas; ia++ )
	{
		if( (net->area[ia].npins + net->area[ia].nvias) > 1 )
		{
			int p1, p2, ic;
			if( net->area[ia].npins > 0 )
				p1 = net->area[ia].pin[0];
			else
			{
				ic = net->area[ia].vcon[0];
				cconnect * c = &net->connect[ic];
				p1 = c->start_pin;
			}
			for( int ip=1; ip<net->area[ia].npins; ip++ )
			{
				p2 = net->area[ia].pin[ip];
				if( p2 != p1 )
				{
					AddPinsToGrid( grid, p1, p2, npins );
				}
			}
			for( int iv=0; iv<net->area[ia].nvias; iv++ )
			{
				ic = net->area[ia].vcon[iv];
				cconnect * c = &net->connect[ic];
				p2 = c->start_pin;
				if( p2 != p1 )
				{
					AddPinsToGrid( grid, p1, p2, npins );
				}
				if( c->end_pin != cconnect::NO_END )
				{
					p2 = c->end_pin;
					if( p2 != p1 )
					{
						AddPinsToGrid( grid, p1, p2, npins );
					}
				}
			}
		}
	}
#ifdef PROFILE
	double time1 = GetElapsedTime();
	StartTimer();
#endif

	// now optimize the unrouted and unlocked connections
	long num_loops = 0;
	int n_optimized = 0;
	int min_p1, min_p2, flag;
	double min_dist;

	// create arrays of pin params for efficiency
	CArray<BOOL>legal;
	CArray<double>x, y;
	CArray<double>d;
	x.SetSize(npins);
	y.SetSize(npins);
	d.SetSize(npins*npins);
	legal.SetSize(npins);
	CPoint p;
	for( int ip=0; ip<npins; ip++ )
	{
		legal[ip] = FALSE;
		cpart * part = net->pin[ip].part;
		if( part )
			if( part->shape )
			{	
				CString pin_name = net->pin[ip].pin_name;
				for( int pin_index=part->shape->GetPinIndexByName(pin_name,-1); pin_index>=0; pin_index=part->shape->GetPinIndexByName(pin_name,pin_index))
				{
					if (	part->shape->m_padstack[pin_index].hole_size || 
							part->shape->m_padstack[pin_index].top.shape || 
							part->shape->m_padstack[pin_index].bottom.shape )
					{
						p = m_plist->GetPinPoint( net->pin[ip].part, pin_index, net->pin[ip].part->side, net->pin[ip].part->angle );
						x[ip] = p.x;
						y[ip] = p.y;
						legal[ip] = TRUE;
						break;
					}
				}
			}
	}
	for( int p1=0; p1<npins; p1++ )
	{
		for( int p2=0; p2<p1; p2++ )
		{
			if( legal[p1] && legal[p2] )
			{
				double dist = sqrt((x[p1]-x[p2])*(x[p1]-x[p2])+(y[p1]-y[p2])*(y[p1]-y[p2]));
				d[p1*npins+p2] = dist;
				d[p2*npins+p1] = dist;
			}
		}
	}

	// make array of distances for all pin pairs p1 and p2
	// where p2<p1 and index = (p1)*(p1-1)/2
	// first, get number of legal pins
	int n_legal = 0;
	for( int p1=0; p1<npins; p1++ )
		if( legal[p1] )
			n_legal++;

	int n_elements = (n_legal*(n_legal-1))/2;
	int * numbers = (int*)calloc( n_elements, sizeof(int) );
	int * index = (int*)calloc( n_elements, sizeof(int) );
	int i = 0;
	for( int p1=1; p1<npins; p1++ )
	{
		for( int p2=0; p2<p1; p2++ )
		{
			if( legal[p1] && legal[p2] )
			{
				index[i] = p1*npins + p2;
				double number = d[p1*npins+p2];
				if( number > INT_MAX )
					ASSERT(0);
				numbers[i] = number;
				i++;
				if( i > n_elements )
					ASSERT(0);
			}
		}
	}
	// sort
	::q_sort(numbers, index, 0, n_elements - 1);
	for( int i=0; i<n_elements; i++ )
	{
		int dd = numbers[i];
		if( i>0 )
		{
			if( dd < numbers[i-1] )
				ASSERT(0);
		}
	}

	// now make connections, shortest first
	for( int i=0; i<n_elements; i++ )
	{
		int p1 = index[i]/npins;
		int p2 = index[i]%npins;
		// find shortest connection between unconnected pins
		if( legal[p1] && legal[p2] && !grid[p1*npins+p2] )
		{
			// connect p1 to p2
			AddPinsToGrid( grid, p1, p2, npins );
			pair.SetAtGrow(n_optimized*2, p1);	
			pair.SetAtGrow(n_optimized*2+1, p2);		
			n_optimized++;
		}
	}
	free( numbers );
	free( index );

	// add new optimized connections
	for( int ic=0; ic<n_optimized; ic++ )
	{
		// make new connection with a single unrouted segment
		int p1 = pair[ic*2];
		int p2 = pair[ic*2+1];
		AddNetConnect( net, p1, p2 );
	}

	free( grid );

#ifdef PROFILE
	double time2 = GetElapsedTime();
	if( net->name == "GND" )
	{
		CString mess;
		mess.Format( "net \"%s\", %d pins\nloops = %ld\ntime1 = %f\ntime2 = %f", 
			net->name, net->npins, num_loops, time1, time2 );
		AfxMessageBox( mess );
	}
#endif
	
	return ic_new;
}

// reset pointers on part pins for net
//
int CNetList::RehookPartsToNet( cnet * net )
{
	for( int ip=0; ip<net->npins; ip++ )
	{
		CString ref_des = net->pin[ip].ref_des;
		CString pin_name = net->pin[ip].pin_name;
		cpart * part = m_plist->GetPart( ref_des );
		if( part )
		{
			if( part->shape )
			{
				for( int pin_index=part->shape->GetPinIndexByName(pin_name,-1); pin_index>=0; pin_index=part->shape->GetPinIndexByName(pin_name,pin_index))
					part->pin[pin_index].net = net;
			}
		}
	}
	return 0;
}

void CNetList::SetViaVisible( cnet * net, int ic, int iv, BOOL visible )
{
	cconnect * c = &net->connect[ic];
	cvertex * v = &c->vtx[iv];

	if( v->dl_el )
		v->dl_el->visible = visible;

	if( v->dl_hole )
		v->dl_hole->visible = visible;
}


void HighlightDrcCircles( int Value)
{

}

// start dragging end vertex of a stub trace to move it
//
void CNetList::StartDraggingEndVertex( CDC * pDC, cnet * net, int ic, int ivtx, int crosshair )
{
	cconnect * c = &net->connect[ic];
	m_dlist->CancelHighLight();
	c->seg[ivtx-1].dl_el->visible = 0;
	SetViaVisible( net, ic, ivtx, FALSE );
	for( int ia=0; ia<net->nareas; ia++ )
		for( int iv=0; iv<net->area[ia].nvias; iv++ )
			if( net->area[ia].vcon[iv] == ic )
				if( net->area[ia].vtx[iv] == ivtx )
					if( net->area[ia].dl_via_thermal[iv] != 0 )
						net->area[ia].dl_via_thermal[iv]->visible = 0;
	m_dlist->StartDraggingLine( pDC,
		c->vtx[ivtx-1].x,
		c->vtx[ivtx-1].y,
		c->vtx[ivtx-1].x,
		c->vtx[ivtx-1].y,
		c->seg[ivtx-1].layer, 
		c->seg[ivtx-1].width,
		c->seg[ivtx-1].layer,
		0, 0, crosshair, DSS_STRAIGHT, IM_NONE );
}

// cancel dragging end vertex of a stub trace
//
void CNetList::CancelDraggingEndVertex( cnet * net, int ic, int ivtx )
{
	cconnect * c = &net->connect[ic];
	m_dlist->StopDragging();
	c->seg[ivtx-1].dl_el->visible = 1;
	SetViaVisible( net, ic, ivtx, TRUE );
	for( int ia=0; ia<net->nareas; ia++ )
		for( int iv=0; iv<net->area[ia].nvias; iv++ )
			if( net->area[ia].vcon[iv] == ic )
				if( net->area[ia].vtx[iv] == ivtx )
					if( net->area[ia].dl_via_thermal[iv] != 0 )
						net->area[ia].dl_via_thermal[iv]->visible = 1;
}

// move end vertex of a stub trace
//
void CNetList::MoveEndVertex( cnet * net, int ic, int ivtx, int x, int y )
{
	cconnect * c = &net->connect[ic];
	m_dlist->StopDragging();
	c->vtx[ivtx].x = x;
	c->vtx[ivtx].y = y;
	c->seg[ivtx-1].dl_el->visible = 1;
	c->vtx[ivtx].x = x;
	c->vtx[ivtx].y = y;
	DrawSegment( net, ic, ivtx-1 );
	for( int ia=0; ia<net->nareas; ia++ )
		SetAreaConnections( net, ia );
}

// move vertex
//
void CNetList::MoveVertex( cnet * net, int ic, int ivtx, int x, int y )
{
	cconnect * c = &net->connect[ic];
	if( ivtx > c->nsegs )
		ASSERT(0);
	cvertex * v = &c->vtx[ivtx];
	m_dlist->StopDragging();
	int lx = v->x;
	int ly = v->y;
	v->x = x;
	v->y = y;
	if( ivtx )
		DrawSegment( net, ic, ivtx-1 );
	if( ivtx < c->nsegs )
		DrawSegment( net, ic, ivtx );
	if( v->tee_ID && ivtx < c->nsegs )
	{
		// this is a tee-point in a trace
		// move other vertices connected to it
		int id = v->tee_ID;
		for( int icc=0; icc<net->nconnects; icc++ )
		{
			if( net->connect[icc].end_pin == cconnect::NO_END )
				if( net->connect[icc].vtx[net->connect[icc].nsegs].tee_ID )
					RepairBranch(net,icc,TRUE);
		}
	}
	CMainFrame * pMainWnd = (CMainFrame*)AfxGetMainWnd();
	CFreePcbView * m_View = (CFreePcbView*)pMainWnd->GetActiveView();
	if( m_View )
	{
		m_View->VertexMoved();
	}
}

// Start dragging trace vertex
//
int CNetList::StartDraggingVertex( CDC * pDC, cnet * net, int ic, int ivtx,
								   int x, int y, int crosshair )
{
	// cancel previous selection and make segments and via invisible
	cconnect * c =&net->connect[ic];
	cvertex * v = &c->vtx[ivtx];
	m_dlist->CancelHighLight();
	c->seg[ivtx-1].dl_el->visible = 0;
	c->seg[ivtx].dl_el->visible = 0;
	SetViaVisible( net, ic, ivtx, FALSE );
	for( int ia=0; ia<net->nareas; ia++ )
	{
		carea * a = &net->area[ia];
		for( int iv=0; iv<a->nvias; iv++ )
		{
			int vic = a->vcon[iv];
			int viv = a->vtx[iv];
			if( a->vcon[iv] == ic && a->vtx[iv] == ivtx && a->dl_via_thermal[iv] != 0 )
				net->area[ia].dl_via_thermal[iv]->visible = 0;
		}
	}

	// if tee connection, also drag tee segment(s)
	if( v->tee_ID && ivtx < c->nsegs )
	{
		int ntsegs = 0;
		// find all tee segments
		for( int icc=0; icc<net->nconnects; icc++ )
		{
			cconnect * cc = &net->connect[icc];
			if( cc != c && cc->end_pin == cconnect::NO_END ) 
			{
				cvertex * vv = &cc->vtx[cc->nsegs];
				if( vv->tee_ID == v->tee_ID )
				{
					ntsegs++;
				}
			}
		}
		m_dlist->MakeDragRatlineArray( ntsegs, 0 );
		// now add them one-by-one
		for( int icc=0; icc<net->nconnects; icc++ )
		{
			cconnect * cc = &net->connect[icc];
			if( cc != c && cc->end_pin == cconnect::NO_END )
			{
				cvertex * vv = &cc->vtx[cc->nsegs];
				if( vv->tee_ID == v->tee_ID )
				{
					CPoint pi, pf;
					pi.x = cc->vtx[cc->nsegs-1].x;
					pi.y = cc->vtx[cc->nsegs-1].y;
					pf.x = 0;
					pf.y = 0;
					m_dlist->AddDragRatline( pi, pf );
					cc->seg[cc->nsegs-1].dl_el->visible = 0;
				}
			}
		}
		m_dlist->StartDraggingArray( pDC, 0, 0 );
	}

	// start dragging
	int xi = c->vtx[ivtx-1].x;
	int yi = c->vtx[ivtx-1].y;
	int xf = c->vtx[ivtx+1].x;
	int yf = c->vtx[ivtx+1].y;
	int layer1 = c->seg[ivtx-1].layer;
	int layer2 = c->seg[ivtx].layer;
	int w1 = c->seg[ivtx-1].width;
	int w2 = c->seg[ivtx].width;
	m_dlist->StartDraggingLineVertex( pDC, x, y, xi, yi, xf, yf, layer1, 
								layer2, w1, w2, DSS_STRAIGHT, DSS_STRAIGHT, 
								0, 0, 0, 0, crosshair );
	return 0;
}

// Start moving trace segment
//
int CNetList::StartMovingSegment( CDC * pDC, cnet * net, int ic, int ivtx,
								   int x, int y, int crosshair, int use_third_segment )
{
	// cancel previous selection and make segments and via invisible
	cconnect * c =&net->connect[ic];
	cvertex * v = &c->vtx[ivtx];
	m_dlist->CancelHighLight();
	c->seg[ivtx-1].dl_el->visible = 0;
	c->seg[ivtx].dl_el->visible = 0;
	SetViaVisible( net, ic, ivtx,   FALSE );
	SetViaVisible( net, ic, ivtx+1, FALSE );
	if(use_third_segment)
	{
		c->seg[ivtx+1].dl_el->visible = 0;
	}
	for( int ia=0; ia<net->nareas; ia++ )
	{
		carea * a = &net->area[ia];
		for( int iv=0; iv<a->nvias; iv++ )
		{
			int vic = a->vcon[iv];
			int viv = a->vtx[iv];
			if( a->vcon[iv] == ic && (a->vtx[iv] == ivtx - 1 || a->vtx[iv]== ivtx || a->vtx[iv] == ivtx + 1)  && a->dl_via_thermal[iv] != 0 )
				net->area[ia].dl_via_thermal[iv]->visible = 0;
		}
	}

	// start dragging
	ASSERT(ivtx > 0);

	int	xb = c->vtx[ivtx-1].x;
	int	yb = c->vtx[ivtx-1].y;
	int xi = c->vtx[ivtx  ].x;
	int yi = c->vtx[ivtx  ].y;
	int xf = c->vtx[ivtx+1].x;
	int yf = c->vtx[ivtx+1].y;

	int layer0 = c->seg[ivtx-1].layer;
	int layer1 = c->seg[ivtx].layer;

	int w0 = c->seg[ivtx-1].width;
	int w1 = c->seg[ivtx].width;

	int xe = 0, ye = 0;
	int layer2 = 0;
	int w2 = 0;
	if(use_third_segment)
	{
		xe = c->vtx[ivtx+2].x;
		ye = c->vtx[ivtx+2].y;
		layer2 = c->seg[ivtx+1].layer;
		w2 = c->seg[ivtx+1].width;
	}
	m_dlist->StartDraggingLineSegment( pDC, x, y, xb, yb, xi, yi, xf, yf, xe, ye,
									layer0, layer1, layer2,
									w0,		w1,		w2,
									DSS_STRAIGHT, DSS_STRAIGHT, use_third_segment?DSS_STRAIGHT:DSS_NONE,
									0, 0, 0, 
									crosshair );
	return 0;
}

// Start dragging trace segment
//
int CNetList::StartDraggingSegment( CDC * pDC, cnet * net, int ic, int iseg,
								   int x, int y, int layer1, int layer2, int w, 
								   int layer_no_via, int via_w, int via_hole_w, int dir,
								   int crosshair )
{
	// cancel previous selection and make segment invisible
	cconnect * c =&net->connect[ic];
	m_dlist->CancelHighLight();
	c->seg[iseg].dl_el->visible = 0;
	// start dragging
	int xi = c->vtx[iseg].x;
	int yi = c->vtx[iseg].y;
	int xf = c->vtx[iseg+1].x;
	int yf = c->vtx[iseg+1].y;
	m_dlist->StartDraggingLineVertex( pDC, x, y, xi, yi, xf, yf, layer1, 
								layer2, w, 1, DSS_STRAIGHT, DSS_STRAIGHT, 
								layer_no_via, via_w, via_hole_w, dir, crosshair );
	return 0;
}

// Start dragging new vertex in existing trace segment
//
int CNetList::StartDraggingSegmentNewVertex( CDC * pDC, cnet * net, int ic, int iseg,
								   int x, int y, int layer, int w, int crosshair )
{
	// cancel previous selection and make segment invisible
	cconnect * c =&net->connect[ic];
	m_dlist->CancelHighLight();
	c->seg[iseg].dl_el->visible = 0;
	// start dragging
	int xi = c->vtx[iseg].x;
	int yi = c->vtx[iseg].y;
	int xf = c->vtx[iseg+1].x;
	int yf = c->vtx[iseg+1].y;
	m_dlist->StartDraggingLineVertex( pDC, x, y, xi, yi, xf, yf, layer, 
								layer, w, w, DSS_STRAIGHT, DSS_STRAIGHT, 
								layer, 0, 0, 0, crosshair );
	return 0;
}

// Start dragging stub trace segment, iseg is index of new segment
//
void CNetList::StartDraggingStub( CDC * pDC, cnet * net, int ic, int iseg,
								   int x, int y, int layer1, int w, 
								   int layer_no_via, int via_w, int via_hole_w,
								   int crosshair, int inflection_mode )
{
	cconnect * c = &net->connect[ic];
	m_dlist->CancelHighLight();
	SetViaVisible( net, ic, iseg, FALSE );
	for( int ia=0; ia<net->nareas; ia++ )
		for( int iv=0; iv<net->area[ia].nvias; iv++ )
			if( net->area[ia].vcon[iv] == ic )
				if( net->area[ia].dl_via_thermal[iv] != 0 )
					net->area[ia].dl_via_thermal[iv]->visible = 0;
	// start dragging, start point is preceding vertex
	int xi = c->vtx[iseg].x;
	int yi = c->vtx[iseg].y;
	m_dlist->StartDraggingLine( pDC, x, y, xi, yi, layer1, 
		w, layer_no_via, via_w, via_hole_w, 
		crosshair, DSS_STRAIGHT, inflection_mode );

}

// Cancel dragging stub trace segment
//
void CNetList::CancelDraggingStub( cnet * net, int ic, int iseg )
{
	cconnect * c = &net->connect[ic];
	SetViaVisible( net, ic, iseg, TRUE );
	for( int ia=0; ia<net->nareas; ia++ )
		for( int iv=0; iv<net->area[ia].nvias; iv++ )
			if( net->area[ia].vcon[iv] == ic )
				if( net->area[ia].dl_via_thermal[iv] != 0 )
					net->area[ia].dl_via_thermal[iv]->visible = 1;
}

// Start dragging copper area corner to move it
//
int CNetList::StartDraggingAreaCorner( CDC *pDC, cnet * net, int iarea, int icorner, int x, int y, int crosshair )
{
	net->area[iarea].poly->StartDraggingToMoveCorner( pDC, icorner, x, y, crosshair );
	return 0;
}

// Start dragging inserted copper area corner
//
int CNetList::StartDraggingInsertedAreaCorner( CDC *pDC, cnet * net, int iarea, int icorner, int x, int y, int crosshair )
{
	net->area[iarea].poly->StartDraggingToInsertCorner( pDC, icorner, x, y, crosshair );
	return 0;
}

// Cancel dragging inserted area corner
//
int CNetList::CancelDraggingInsertedAreaCorner( cnet * net, int iarea, int icorner )
{
	net->area[iarea].poly->CancelDraggingToInsertCorner( icorner );
	return 0;
}

// Cancel dragging area corner
//
int CNetList::CancelDraggingAreaCorner( cnet * net, int iarea, int icorner )
{
	net->area[iarea].poly->CancelDraggingToMoveCorner( icorner );
	return 0;
}

// Cancel dragging segment
//
int CNetList::CancelDraggingSegment( cnet * net, int ic, int iseg )
{
	// make segment visible
	cconnect * c =&net->connect[ic];
	c->seg[iseg].dl_el->visible = 1;
	m_dlist->StopDragging();
	return 0;
}

// Cancel dragging vertex
//
int CNetList::CancelDraggingVertex( cnet * net, int ic, int ivtx )
{
	// make segments and via visible
	cconnect * c =&net->connect[ic];
	cvertex * v = &c->vtx[ivtx];
	c->seg[ivtx-1].dl_el->visible = 1;
	c->seg[ivtx].dl_el->visible = 1;
	SetViaVisible( net, ic, ivtx, TRUE );
	for( int ia=0; ia<net->nareas; ia++ )
		for( int iv=0; iv<net->area[ia].nvias; iv++ )
			if( net->area[ia].vcon[iv] == ic )
				if( net->area[ia].dl_via_thermal[iv] != 0 )
					net->area[ia].dl_via_thermal[iv]->visible = 1;
	// if tee, make connecting stubs visible
	if( v->tee_ID )
	{
		for( int icc=0; icc<net->nconnects; icc++ )
		{
			cconnect * cc = &net->connect[icc];
			if( cc != c && cc->end_pin == cconnect::NO_END )
			{
				cvertex * vv = &cc->vtx[cc->nsegs];
				if( vv->tee_ID == v->tee_ID )
				{
					cc->seg[cc->nsegs-1].dl_el->visible = 1;
				}
			}
		}
	}

	m_dlist->StopDragging();
	return 0;
}

// Cancel moving segment:
//
int CNetList::CancelMovingSegment( cnet * net, int ic, int ivtx )
{
	// cancel previous selection and make segments and via invisible
	cconnect * c =&net->connect[ic];
	cvertex * v = &c->vtx[ivtx];

	// make segments and via visible
	c->seg[ivtx-1].dl_el->visible = 1;
	c->seg[ivtx].dl_el->visible = 1;
	if( m_dlist->Dragging_third_segment() )
		c->seg[ivtx+1].dl_el->visible = 1;
	SetViaVisible( net, ic, ivtx,   TRUE );
	SetViaVisible( net, ic, ivtx+1, TRUE );
	for( int ia=0; ia<net->nareas; ia++ )
	{
		carea * a = &net->area[ia];
		for( int iv=0; iv<a->nvias; iv++ )
		{
			int vic = a->vcon[iv];
			int viv = a->vtx[iv];
			if( a->vcon[iv] == ic && (a->vtx[iv] == ivtx - 1 || a->vtx[iv]== ivtx || a->vtx[iv] == ivtx + 1)  && a->dl_via_thermal[iv] != 0 )
				net->area[ia].dl_via_thermal[iv]->visible = 1;
		}
	}
	m_dlist->StopDragging();
	return 0;
}


// Cancel dragging vertex inserted into segment
//
int CNetList::CancelDraggingSegmentNewVertex( cnet * net, int ic, int iseg )
{
	// make segment visible
	cconnect * c =&net->connect[ic];
	c->seg[iseg].dl_el->visible = 1;
	m_dlist->StopDragging();
	return 0;
}

// returns: VIA_NO_CONNECT if no via
//			VIA_TRACE if via connects to a trace segment on this layer
//			VIA_AREA if via connects to copper area
//
int CNetList::GetViaConnectionStatus( cnet * net, int ic, int iv, int layer )
{
	if( iv == 0 )
		ASSERT(0);

	int status = VIA_NO_CONNECT;
	cconnect * c = &net->connect[ic];
	cvertex * v = &c->vtx[iv];

	// check for end vertices of traces to pads
	if( iv == 0 )
		return status;
	if( !getbit( v->layer_bit, layer ) )
		return status;
	if( c->end_pin != cconnect::NO_END  && iv == c->nsegs )
		return status;

	// check for normal via pad
	if( v->via_w == 0 && v->tee_ID == 0 )
		return status;

	// check for via pad at end of branch
	if( v->tee_ID && iv == c->nsegs && c->seg[iv-1].layer == layer )
		if( !TeeViaNeeded( net, v->tee_ID ) )
			return status;

	// check for trace connection to via pad
	c = &net->connect[ic];
	if( c->seg[iv-1].layer == layer )
		status |= VIA_TRACE;
	else if( iv < c->nsegs )
	{
		if( c->seg[iv].layer == layer )
			status |= VIA_TRACE;
	}
	// branches...
	if( v->tee_ID && getbit(status, VIA_TRACE) == 0 )
	{
		int tic = -1;
		int tiv = iv;
		while( FindTeeVertexInNet( net, v->tee_ID, &tic, &tiv ) )
		{
			if( net->connect[tic].seg[tiv-1].layer == layer )
			{
				status |= VIA_TRACE;
				break;
			}
		}
	}

	// see if it connects to any area in this net on this layer
	for( int ia=0; ia<net->nareas; ia++ )
	{
		// next area
		carea * a = &net->area[ia];
		if( a->poly->GetLayer() == layer )
		{
			// area is on this layer, loop through via connections to area
			for( int ivia=0; ivia<a->nvias; ivia++ )
			{
				if( a->vcon[ivia] == ic && a->vtx[ivia] == iv )
				{
					// via connects to area
					status |= VIA_AREA;
					break;
				}
			}
		}
	}
	return status;
}

// get via parameters for vertex
// note: if the vertex is the end-vertex of a branch, the via parameters
// will be taken from the tee-vertex that the branch connects to
//
void CNetList::GetViaPadInfo( cnet * net, int ic, int iv, int layer,
		int * pad_w, int * pad_hole_w, int * connect_status )
{
	cconnect * c = &net->connect[ic];
	cvertex * v = &c->vtx[iv];
	int con_status = GetViaConnectionStatus( net, ic, iv, layer );
	int w = v->via_w;
	int hole_w = v->via_hole_w;
	if( layer > LAY_BOTTOM_COPPER )
	{
		// inner layer
		if( con_status == VIA_NO_CONNECT )
		{
			w = 0;	// no connection, no pad
		}
		else if( v->tee_ID && iv == c->nsegs )
		{
			// end-vertex of branch, find tee-vertex that it connects to
			int tee_ic=0, tee_iv=0;
			if( FindTeeVertexInNet( net, v->tee_ID, &tee_ic, &tee_iv ) )
			{
				w = net->connect[tee_ic].vtx[tee_iv].via_w;
				hole_w = net->connect[tee_ic].vtx[tee_iv].via_hole_w;
			}
			else
				ASSERT(0);
		}
		else
			w = hole_w + 2*m_annular_ring;	// pad = annular ring
	}
	if( w )
		if( getbit( v->layer_bit, layer ) == 0 )
		{
			w = 0;
			hole_w = 0;
			con_status = VIA_NO_CONNECT;
		}
	if( pad_w )
		*pad_w = abs(w);
	if( pad_hole_w )
		*pad_hole_w = abs(hole_w);
	if( connect_status )
		*connect_status = con_status;
}

// Test for a hit on a vertex in a routed or partially-routed trace
// If layer == 0, ignore layer
//
BOOL CNetList::TestForHitOnVertex( cnet * net, int layer, int x, int y, 
		cnet ** hit_net, int * hit_ic, int * hit_iv )
{
	// loop through all connections
	for( int ic=0; ic<net->nconnects; ic++ )
	{
		cconnect * c = &net->connect[ic];
		for( int iv=1; iv<c->nsegs; iv++ )
		{
			cvertex * v = &c->vtx[iv];
			cseg * pre_s = &c->seg[iv-1];
			cseg * post_s = &c->seg[iv];
			if( v->via_w > 0 || layer == 0 || layer == pre_s->layer || layer == post_s->layer
				|| (pre_s->layer == LAY_RAT_LINE && post_s->layer == LAY_RAT_LINE) )
			{
				int test_w = max( v->via_w, pre_s->width );
				test_w = max( test_w, post_s->width );
				test_w = max( test_w, 10*NM_PER_MIL );		// in case widths are zero
				double dx = x - v->x;
				double dy = y - v->y;
				double d = sqrt( dx*dx + dy*dy );
				if( d < test_w/2 )
				{
					*hit_net = net;
					*hit_ic = ic;
					*hit_iv = iv;
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

// add empty copper area to net
// return index to area (zero-based)
//
int CNetList::AddArea( cnet * net, int layer, int x, int y, int hatch, int N_CORNERS )
{
	net->area.SetSize( net->nareas+1 );
	net->area[net->nareas].Initialize( m_dlist );
	id area_id( ID_NET, ID_AREA, net->nareas );
	net->area[net->nareas].poly->Start( layer, 1, 5*NM_PER_MIL, x, y, 
		hatch, &area_id, net, N_CORNERS );
	net->area[net->nareas].selected = 0;
	net->nareas++;
	return net->nareas-1;
}

// add empty copper area to net, inserting at net->area[iarea]
//
void CNetList::InsertArea( cnet * net, int iarea, int layer, int w, int x, int y, int hatch, int SIZE )
{
	// make new area and insert it into area array
	carea test;
	net->area.InsertAt( iarea, test ) ;
	net->area[iarea].Initialize( m_dlist );
	id area_id( ID_NET, ID_AREA, iarea );
	net->area[iarea].poly->Start( layer, w, 5*NM_PER_MIL, x, y,
		hatch, &area_id, net, SIZE );
	net->nareas++;
	RenumberAreas( net );
}

// add corner to copper area, apply style to preceding side
//
int CNetList::AppendAreaCorner( cnet * net, int iarea, int x, int y, int style, BOOL bDraw )
{
	net->area[iarea].poly->AppendCorner( x, y, style, bDraw );
	return 0;
}

// insert corner into copper area, apply style to preceding side
//
int CNetList::InsertAreaCorner( cnet * net, int iarea, int icorner, 
							int x, int y, int style, BOOL bDraw )
{
	if( icorner == net->area[iarea].poly->GetNumCorners() && !net->area[iarea].poly->GetClosed() )
	{
		net->area[iarea].poly->AppendCorner( x, y, style, bDraw );
		ASSERT(0);	// this is now an error, should be using AppendAreaCorner
	}
	else
	{
		net->area[iarea].poly->InsertCorner( icorner, x, y, 0 );
		net->area[iarea].poly->SetSideStyle( icorner, style, bDraw );
	}
	return 0;
}

// move copper area corner
//
void CNetList::MoveAreaCorner( cnet * net, int iarea, int icorner, int x, int y, BOOL bDraw )
{
	int xb = net->area[iarea].poly->GetX(icorner); 
	int yb = net->area[iarea].poly->GetY(icorner); 
	net->area[iarea].poly->MoveCorner( icorner, x, y, bDraw );
}

// highlight
//
void CNetList::HighlightAreaCorner( cnet * net, int iarea, int icorner, int w )
{
	net->area[iarea].poly->HighlightCorner( icorner, w );
}

// get copper area corner coords
//
CPoint CNetList::GetAreaCorner( cnet * net, int iarea, int icorner )
{
	CPoint pt;
	pt.x = net->area[iarea].poly->GetX( icorner );
	pt.y = net->area[iarea].poly->GetY( icorner );
	return pt;
}

// complete copper area contour by adding line to first corner
//
int CNetList::CompleteArea( cnet * net, int iarea, int style )
{
	int al = net->area[iarea].poly->GetLayer();
	if( al == LAY_SM_TOP || al == LAY_SM_BOTTOM )
		net->area[iarea].poly->SetLayer( al+2 );
	net->area[iarea].poly->Close( style );
	return 0;
}

// set connections for all areas
//
void CNetList::SetAreaConnections()
{
	POSITION pos;
	CString name;
	void * ptr;
	for( pos = m_map.GetStartPosition(); pos != NULL; )
	{
		m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		SetAreaConnections( net );
	}
}

// set connections for all areas
//
void CNetList::SetAreaConnections( cnet * net)
{
	if( net )
	{
		for( int ia=0; ia<net->nareas; ia++ )
			SetAreaConnections( net, ia );
	}
}

// set area connections for all nets on a part
// should be used when a part is added or moved
//
void CNetList::SetAreaConnections( cpart * part )
{
	// find nets which connect to this part and adjust areas
	for( int ip=0; ip<part->shape->m_padstack.GetSize(); ip++ )
	{
		cnet * net = (cnet*)part->pin[ip].net;
		if( net )
		{
			int set_area_flag = 1;
			// see if this net already encountered
			for( int ipp=0; ipp<ip; ipp++ )
				if( (cnet*)part->pin[ipp].net == net )
					set_area_flag = 0;
			// set area connections for net
			if( set_area_flag )
				SetAreaConnections( net );
		}
	}
}





// set arrays of pins and stub traces connected to area
// does not modify connect[] array
//
void CNetList::SetAreaConnections( cnet * net, int iarea, BOOL HIGHLIGHT)
{
	carea * area = &net->area[iarea];
	// zero out previous arrays
	for( int ip=0; ip<area->dl_thermal.GetSize(); ip++ )
		m_dlist->Remove( area->dl_thermal[ip] );
	for( int is=0; is<area->dl_via_thermal.GetSize(); is++ )
		m_dlist->Remove( area->dl_via_thermal[is] );
	area->npins = 0;
	area->nvias = 0;
	area->pin.SetSize(0);
	area->dl_thermal.SetSize(0);
	area->vcon.SetSize(0);
	area->vtx.SetSize(0);
	area->dl_via_thermal.SetSize(0);
	if( area->poly->GetHatch() == CPolyLine::NO_HATCH && area->poly->GetW() == 0 )
		return;//  area Ghost
	// test all pins in net for being inside copper area 
	id id( ID_NET, ID_AREA, iarea, ID_PIN_X );
	int area_layer = area->poly->GetLayer();	// layer of copper area
	CPoint p;
	cpart * part;
	CString part_pin_name;
	RECT A;
	A =	area->poly->GetBounds();
	int bEnableChangeMerge = !(area->poly->GetMerge() + 1);
	for( int ip=0; ip<net->npins; ip++ )
	{
		part = (cpart*)net->pin[ip].part;
		if( part )
		{					
			if( part->shape && part->drawn )
			{
				part_pin_name = net->pin[ip].pin_name;
				for( int pin_index=part->shape->GetPinIndexByName(part_pin_name,-1); pin_index>=0; pin_index=part->shape->GetPinIndexByName(part_pin_name,pin_index))
				{
					p = m_plist->GetPinPoint( part, pin_index, part->side, part->angle );
					if (p.x < A.left)	continue;
					if (p.x > A.right)	continue;
					if (p.y < A.bottom)	continue;
					if (p.y > A.top)	continue;
					// see if pin allowed to connect to area
					int pin_layer = m_plist->GetPinLayer( part, pin_index );
					if( pin_layer != LAY_PAD_THRU && pin_layer != area_layer )
							continue;	// not on area layer
					// see if pad allowed to connect
					padstack * ps = &part->shape->m_padstack[pin_index];
					pad * ppad = &ps->inner;
					if( (part->side == 0 && area_layer == LAY_TOP_COPPER)
						|| (part->side == 1 && area_layer == LAY_BOTTOM_COPPER) )
						ppad = &ps->top;
					else if( (part->side == 1 && area_layer == LAY_TOP_COPPER)
						|| (part->side == 0 && area_layer == LAY_BOTTOM_COPPER) )
						ppad = &ps->bottom;
					if( ppad->connect_flag == PAD_CONNECT_NEVER )
						continue;	// pad never allowed to connect
					if( ppad->connect_flag == PAD_CONNECT_DEFAULT && !ps->hole_size && !m_bSMT_connect )
						continue;	// pad uses project option not to connect SMT pads
					if( pin_layer != LAY_PAD_THRU && ppad->shape == PAD_NONE )
						continue;	// no SMT pad defined (this should not happen)
					// see if pad is inside copper area
					if( m_dlist && area->poly->TestPointInside( p.x, p.y ) )
					{
						// pin is inside copper area
						cnet * part_pin_net = part->pin[pin_index].net;
						if (HIGHLIGHT) 
							m_plist->SelectPad( part, pin_index, 1, 0 );
						if( part_pin_net != net )
						{
							ASSERT(0);	// inconsistency between part->pin->net and net->pin->part
							CString t = part->ref_des;
							//...
						}
						area->pin.SetSize( area->npins+1 );
						area->pin[area->npins] = ip;
						id.ii = ip;
						RECT pr;
						part->shape->GetPadBounds( pin_index, &pr );
						int w = 0;
						if( part->shape->m_padstack[pin_index].hole_size )
							w = part->shape->m_padstack[pin_index].hole_size;
						w = max(5*NM_PER_MIL,w*2/3);
						int l_map = 0;
						setbit( l_map, LAY_RAT_LINE );
						RECT rect;
						rect.left =   p.x-w/2;
						rect.right =  p.x+w/2;
						rect.bottom = p.y-w/2;
						rect.top =	  p.y+w/2;
						dl_element * dl = m_dlist->Add(	id, net, l_map, DL_X, net->visible,
														&rect, w/10, NULL, 0 );
						if( pin_layer != LAY_PAD_THRU || !area->m_dlist->m_vis[pin_layer])
							dl->visible = 0;	
						area->dl_thermal.SetAtGrow(area->npins, dl );
						area->npins++;
						
						// set merge
						if( area_layer <= LAY_BOTTOM_COPPER && bEnableChangeMerge )
						{
							if( area->npins == 1 )
								area->poly->SetMerge( part->m_merge );
							if( area->poly->GetMerge() != part->m_merge )
								area->poly->SetMerge( -1 );
						}
					}
				}
			}
		}
	}
	// test all vias in traces for being inside copper area,
	// also test all end-vertices of non-branch stubs for being on same layer
	id.sst = ID_STUB_X;
	for( int ic=0; ic<net->nconnects; ic++ ) 
	{
		cconnect * c = &net->connect[ic];
		int nsegs = c->nsegs;
		int nvtx = nsegs;
		if( c->end_pin == cconnect::NO_END )
			nvtx++;
		for( int iv=1; iv<nvtx; iv++ )
		{
			cvertex * v = &c->vtx[iv];
			if (v->x < A.left)	continue;
			if (v->x > A.right)	continue;
			if (v->y < A.bottom)continue;
			if (v->y > A.top)	continue;
			if( ( v->via_w && getbit( v->layer_bit, area_layer ) ) || c->seg[iv-1].layer == area->poly->GetLayer() )
			{
				// via or on same layer as copper area
				int x = v->x;
				int y = v->y;
				if( m_dlist && area->poly->TestPointInside( x, y ) )
				{
					// end point of trace is inside copper area
					area->vcon.SetSize( area->nvias+1 );
					area->vcon[area->nvias] = ic;
					area->vtx.SetSize( area->nvias+1 );
					area->vtx[area->nvias] = iv;
					if (HIGHLIGHT) 
						HighlightVertex( net, ic, iv, 0, 999999 );
					id.ii = ic;
					int w = v->via_hole_w;
					w = (float)w*2/3;
					if( !w )
					{
						w = c->seg[iv-1].width/2;
					}
					int l_map = 0;
					setbit( l_map, LAY_RAT_LINE );
					RECT rect;
					rect.left =   x-w/2;
					rect.right =  x+w/2;
					rect.bottom = y-w/2;
					rect.top =	  y+w/2;
					dl_element * dl = m_dlist->Add( id, net, l_map, DL_X, net->visible,
													&rect, w/10, NULL, 0 );
					if( !v->via_w )
					{
						if ( !area->m_dlist->m_vis[c->seg[iv-1].layer] 
							 || iv != (nvtx-1) 
							 || c->end_pin != cconnect::NO_END 
							 || v->tee_ID)
							dl->visible = 0;	
					}
					area->dl_via_thermal.SetAtGrow(area->nvias, dl );
					area->nvias++;

					// set merge
					if( area->npins == 0 && area_layer <= LAY_BOTTOM_COPPER && bEnableChangeMerge )
					{
						if( area->nvias == 1 )
							area->poly->SetMerge( c->m_merge );
						if( area->poly->GetMerge() != c->m_merge )
							area->poly->SetMerge( -1 );
					}
				}
			}
		}
	}
}

// see if a point on a layer is inside a copper area in a net
// if layer == LAY_PAD_THRU, matches any layer
// if so, return iarea
//
BOOL CNetList::TestPointInArea( cnet * net, int x, int y, int layer, int * iarea )
{
	BOOL invert = FALSE;
	if (layer < 0)
	{
		invert = TRUE;
		layer = -layer;
	}
	if( layer < LAY_TOP_COPPER )
		layer += 2;
	for( int ia=0; ia<net->nareas; ia++ )
	{
		carea * a = &net->area[ia];
		///if( a->poly->GetHatch() == CPolyLine::NO_HATCH && a->poly->GetW() == 0 )
		///	continue;//  area Ghost
		if( ( a->poly->GetLayer() == layer && !invert ) || ( a->poly->GetLayer() != layer && invert ) || layer == LAY_PAD_THRU )
		if( a->poly->TestPointInside( x, y ) )
		{
			if( iarea )
				*iarea = ia;
			return TRUE;
		}
	}
	return FALSE;
}

// remove copper area from net
//
int CNetList::RemoveArea( cnet * net, int iarea )
{
	net->area[iarea].poly->Undraw();
	net->area.RemoveAt( iarea );
	net->nareas--;
	RenumberAreas( net );
	return 0;
}

// Get pointer to net with given name
//
cnet * CNetList::GetNetPtrByName( CString * name )
{
	// find element with name
	void * ptr;
	cnet * net;
	if( m_map.Lookup( *name, ptr ) )
	{
		net = (cnet*)ptr;
		return net;
	}
	return 0;
}
	
// Select copper area side
//
void CNetList::SelectAreaSide( cnet * net, int iarea, int iside )
{
	m_dlist->CancelHighLight();
	net->area[iarea].poly->HighlightSide( iside );
}

// Select copper area corner
//
void CNetList::SelectAreaCorner( cnet * net, int iarea, int icorner )
{
	m_dlist->CancelHighLight();
	net->area[iarea].poly->HighlightCorner( icorner );
}

// Set style for area side
//
void CNetList::SetAreaSideStyle( cnet * net, int iarea, int iside, int style )
{
	m_dlist->CancelHighLight();
	net->area[iarea].poly->SetSideStyle( iside, style );
	net->area[iarea].poly->HighlightSide( iside );
}


void CNetList::HighlightNetVertices( cnet * net, BOOL vias_only, BOOL IncludeEndVias )
{
	for( int ic=0; ic<net->nconnects; ic++ )
	{
		if (net->connect[ic].nsegs == 1 && net->connect[ic].seg[0].layer == LAY_RAT_LINE ){}		
		else 
		{
			cconnect * c = &net->connect[ic];
			int n_seg = c->seg.GetSize();
			for( int is=1; is<=n_seg; is++ )
				if (vias_only && !c->vtx[is].via_w)
					continue;
				else if( is == n_seg )
				{
					if( IncludeEndVias && (m_dlist->m_vis[net->connect[ic].seg[is-1].layer] != 0 || c->vtx[is].via_w) )
						HighlightVertex( net, ic, is );
				}
				else if( m_dlist->m_vis[net->connect[ic].seg[is].layer] != 0 || 
					     m_dlist->m_vis[net->connect[ic].seg[is-1].layer] != 0 )
					HighlightVertex( net, ic, is );
				else if (c->vtx[is].via_w) 
					HighlightVertex( net, ic, is );
		}
	}
}


// Select all connections in net
//
void CNetList::HighlightNetConnections( cnet * net, int bTRANSPARENT, BOOL W0, int ic, int is )
{
	for( int icon=0; icon<net->nconnects; icon++ )
	{		
		if( icon != ic ) 
			HighlightConnection( net, icon, -1, W0, 0, 0, bTRANSPARENT );
		else
			HighlightConnection( net, icon, is, W0, 0, 0, bTRANSPARENT );
	}
}

// Select connection
//
void CNetList::HighlightConnection( cnet * net, int ic, int bTRANSPARENT )
{
	cconnect * c = &net->connect[ic];
	for( int is=0; is<c->seg.GetSize(); is++ )
		HighlightSegment( net, ic, is, 0, bTRANSPARENT );
}


// Select segments of trace
//
void CNetList::HighlightConnection( cnet * net, int ic, int is, BOOL W0, BOOL hv1, BOOL hv2, int transparent )
{
	cconnect * c =&net->connect[ic];
	for(int iseg=0; iseg<c->nsegs; iseg++)
	{
		if(iseg==is)
			continue;
		c->seg[iseg].dl_el->transparent = 0;
		if( !W0 )
		{
			m_dlist->HighLight( c->seg[iseg].dl_el );
			if( transparent )
				c->seg[iseg].dl_el->transparent = c->seg[iseg].layer;
		}
		else
		{
			dl_element * el = m_dlist->Cpy( c->seg[iseg].dl_el );
			el->el_w = 1;
			el->map_orig_layer = el->layers_bitmap;
			el->layers_bitmap = 0;
			m_dlist->HighLight( el );
			if( transparent )
				el->transparent = c->seg[iseg].layer;
		}
	}
	if( hv1 )
		HighlightVertex( net, ic, 0 );
	if( hv2 )
		HighlightVertex( net, ic, c->nsegs );
}

// Select segment
//
void CNetList::HighlightSegment( cnet * net, int ic, int iseg, int fClearance, int bTRANSPARENT, BOOL W0 )
{
	cconnect * c =&net->connect[ic];
	if( iseg >= c->nsegs )
		ASSERT(0);
	if( c->seg[iseg].dl_el )
	{
		c->seg[iseg].dl_el->transparent = 0;
		if( bTRANSPARENT || W0 || c->seg[iseg].layer == LAY_RAT_LINE )
		{
			dl_element * sel = m_dlist->Cpy( c->seg[iseg].dl_el );
			sel->layers_bitmap = 0;
			if( bTRANSPARENT )
				sel->transparent = bTRANSPARENT;
			if( c->seg[iseg].layer == LAY_RAT_LINE )
				sel->el_w = 10;
			else if( W0 )
				sel->el_w = 1;
			m_dlist->HighLight( sel );
		}
		else
		{
			m_dlist->HighLight( c->seg[iseg].dl_el );
		}
		if (fClearance)
		{
			dl_element * el = m_dlist->Cpy( c->seg[iseg].dl_el );
			el->layers_bitmap = 0;
			el->el_w = (c->seg[iseg].width+2*fClearance)/m_pcbu_per_wu;
			el->transparent = TRANSPARENT_BACKGND;
			m_dlist->HighLight( el );
			el->map_orig_layer = c->seg[iseg].dl_el->layers_bitmap;
			if (c->vtx[iseg].via_w)
				HighlightVertex( net, ic, iseg, 0, fClearance );
			if (c->vtx[iseg+1].via_w)
				HighlightVertex( net, ic, iseg+1, 0, fClearance );
		}
	}
}

// Select vertex
//
void CNetList::HighlightVertex( cnet * net, int ic, int ivtx, int fLines, int fClearance )
{
	cconnect * c =&net->connect[ic];
	if( c->vtx[ivtx].dl_hole )	
	{
		int l_map = c->vtx[ivtx].dl_hole->layers_bitmap;
		int x = c->vtx[ivtx].x;
		int y = c->vtx[ivtx].y;
		int w = c->vtx[ivtx].via_w;
		int wh = c->vtx[ivtx].via_hole_w;
		int dw = (w-wh)/4;
		int neww = w/2-dw;
		id id(0,0,0,0,0);
		RECT nr = rect( x-neww, y-neww, x+neww, y+neww );
		CPoint P[2];
		P[0].x = x;
		P[0].y = y+neww;
		P[1].x = x-neww;
		P[1].y = y;
		dl_element * el = m_dlist->Add( id, NULL, 0, DL_ARC_CCW, 1, &nr, dw*2, P, 2 );
		m_dlist->HighLight( el );
		el->map_orig_layer = l_map;
		P[0].x = x-neww;
		P[0].y = y;
		P[1].x = x;
		P[1].y = y-neww;
		el = m_dlist->Add( id, NULL, 0, DL_ARC_CCW, 1, &nr, dw*2, P, 2 );
		m_dlist->HighLight( el );
		el->map_orig_layer = l_map;
		P[0].x = x;
		P[0].y = y-neww;
		P[1].x = x+neww;
		P[1].y = y;
		el = m_dlist->Add( id, NULL, 0, DL_ARC_CCW, 1, &nr, dw*2, P, 2 );
		m_dlist->HighLight( el );
		el->map_orig_layer = l_map;
		P[0].x = x+neww;
		P[0].y = y;
		P[1].x = x;
		P[1].y = y+neww;
		el = m_dlist->Add( id, NULL, 0, DL_ARC_CCW, 1, &nr, dw*2, P, 2 );
		m_dlist->HighLight( el );
		el->map_orig_layer = l_map;
	}
	else if( c->vtx[ivtx].dl_el )
		m_dlist->HighLight( c->vtx[ivtx].dl_el );
	if( fClearance && c->vtx[ivtx].via_w )
	{
		int w /*= max( c->seg[max(ivtx-1,0)].width, c->seg[min(ivtx,c->nsegs-1)].width );
			w */= c->vtx[ivtx].via_w;

		dl_element * el = m_dlist->Cpy( c->vtx[ivtx].dl_el );
		el->gtype = DL_HOLLOW_CIRC;
		RECT * rct = m_dlist->Get_Rect( el, NULL );
		SwellRect( rct, fClearance/m_pcbu_per_wu );
		el->el_w = 0;
		el->map_orig_layer = el->layers_bitmap;
		el->layers_bitmap = 0;
		m_dlist->HighLight( el );
		el->transparent = TRANSPARENT_HILITE;
	}
	if (fLines)
		AddHighlightLines( c->vtx[ivtx].x, c->vtx[ivtx].y, fLines );
}

// Highlight all sides of area
//
void CNetList::HighlightAreaSides( cnet * net, int ia, int w, int bTRANSPARENT )
{
	carea * a = &net->area[ia];
	int nsides = a->poly->GetNumSides();
	for( int is=0; is<nsides; is++ )
		a->poly->HighlightSide( is, w, bTRANSPARENT );
}

// Highlight entire net
//
void CNetList::HighlightNet( cnet * net, int bTRANSPARENT )
{
	int TL=0;
	for( int ia=0;ia<net->nareas; ia++ )
	{
		if( bTRANSPARENT )
			TL = net->area[ia].poly->GetLayer();
		HighlightAreaSides( net, ia, abs(net->area[ia].poly->GetW()), TL );
		net->area[ia].poly->TransparentHatch( TL );

	}
}

// force a via on a vertex
//
int CNetList::ForceVia( cnet * net, int ic, int ivtx, BOOL set_areas, int vw, int vh )
{
	cconnect * c = &net->connect[ic];
	c->vtx[ivtx].force_via_flag = 1;
	ReconcileVia( net, ic, ivtx, TRUE, vw, vh );
	if( set_areas )
		SetAreaConnections( net );
	return 0;
}

// remove forced via on a vertex
// doesn't modify adjacent segments
//
int CNetList::UnforceVia( cnet * net, int ic, int ivtx, BOOL set_areas )
{
	cconnect * c = &net->connect[ic];
	c->vtx[ivtx].force_via_flag = 0;
	if( ReconcileVia( net, ic, ivtx) == 0 )
	{
		net->connect[ic].vtx[ivtx].via_w = 0;
		net->connect[ic].vtx[ivtx].via_hole_w = 0;
		DrawVia( net, ic, ivtx );
	}
	if( set_areas )
		SetAreaConnections( net );
	return 0;
}

// Reconcile via with preceding and following segments
// if a via is needed, use defaults for adjacent segments 
//
int CNetList::ReconcileVia( cnet * net, int ic, int ivtx, BOOL bDrawVia, int vw, int vh )
{
	cconnect * c = &net->connect[ic];
	cvertex * v = &c->vtx[ivtx];
	BOOL via_needed = FALSE;
	// see if via needed
	if( v->force_via_flag && ivtx ) 
	{
		via_needed = TRUE;
	}
	// if branch
	else if( c->end_pin == cconnect::NO_END && ivtx == c->nsegs )
	{
		// end vertex of a stub trace
		if( v->tee_ID )
		{
			// this is a branch, reconcile the main tee
			int tee_ic=0;
			int tee_iv=0;
			BOOL bFound = FindTeeVertexInNet( net, v->tee_ID, &tee_ic, &tee_iv );
			if( bFound )
			{
				if( net->connect[tee_ic].vtx[tee_iv].via_w == 0 )
					if( net->connect[tee_ic].seg[tee_iv-1].layer != net->connect[ic].seg[ivtx-1].layer )
						if( net->connect[tee_ic].seg[tee_iv-1].layer >= LAY_TOP_COPPER )
							if( net->connect[ic].seg[ivtx-1].layer >= LAY_TOP_COPPER )
							{
								if( ReconcileVia( net, tee_ic, tee_iv, bDrawVia, vw, vh ) )
									DrawConnection( net, tee_ic );
								return TRUE;
							}	
			}
		}
	}
	// first and last vertex are part pads
	else if( ivtx == 0 || ivtx == c->nsegs )
	{
		return 0;
	}
	// if tee_ID!=0( header trace )
	else if( v->tee_ID )
	{
		if( TeeViaNeeded( net, v->tee_ID ) )
			via_needed = TRUE;
	}
	else // layer1 != layer2
	{
		c->vtx[ivtx].pad_layer = 0;
		cseg * s1 = &c->seg[ivtx-1];
		cseg * s2 = &c->seg[ivtx];
		if( s1->layer != s2->layer && 
			s1->layer >= LAY_TOP_COPPER && 
			s2->layer >= LAY_TOP_COPPER )
		{
			via_needed = TRUE;
		}
	}
	//
	if( via_needed )
	{
		// via needed, make sure it exists or create it
		if( v->via_w == 0 || v->via_hole_w == 0 )
		{
			// via doesn't already exist, set via width and hole width
			// using project or net defaults
			int w=c->seg[ivtx-1].width, via_w=0, via_hole_w=0;
			if ( vw && vh )
			{
				via_w = vw;
				via_hole_w = vh;
			}
			else	
				GetWidths( net, &w, &via_w, &via_hole_w );
			// set parameters for via
			v->via_w = via_w;
			v->via_hole_w = via_hole_w;
		}
		else if( vw && vh && (v->via_w != vw || v->via_hole_w != vh) )
		{
			v->via_w = vw;
			v->via_hole_w = vh;
		}
	}
	if( m_dlist && bDrawVia )
		DrawVia( net, ic, ivtx );
	return via_needed;
}

// write nets to file
//
int CNetList::WriteNets( CStdioFile * file )
{
	CString line;
	cvertex * v;
	cseg * s;
	cnet * net;

	try
	{
		line.Format( "[nets]\n\n" );
		file->WriteString( line );

		// traverse map
		POSITION pos;
		CString name;
		void * ptr;
		for( pos = m_map.GetStartPosition(); pos != NULL; )
		{
			m_map.GetNextAssoc( pos, name, ptr ); 
			net = (cnet*)ptr;
			line.Format( "net: \"%s\" %d %d %d %d %d %d %d\n", 
							net->name, net->npins, net->nconnects, net->nareas,
							net->def_w, net->def_via_w, net->def_via_hole_w,
							net->visible );
			file->WriteString( line );
			for( int ip=0; ip<net->npins; ip++ )
			{
				line.Format( "  pin: %d %s.%s\n", ip+1, 
					net->pin[ip].ref_des, net->pin[ip].pin_name );
				file->WriteString( line );
			}
			for( int ic=0; ic<net->nconnects; ic++ ) 
			{
				cconnect * c = &net->connect[ic]; 
				line.Format( "  connect: %d %d %d %d %d %d\n", ic+1, 
					c->start_pin,
					c->end_pin, c->nsegs, c->locked, c->m_merge );
				file->WriteString( line );
				int nsegs = c->nsegs;
				for( int is=0; is<=nsegs; is++ )
				{
					v = &(c->vtx[is]);
					line.Format( "    vtx: %d %d %d %d %d %d %d %d\n", 
									is+1, v->x, v->y, v->pad_layer, v->force_via_flag, 
									v->via_w, v->via_hole_w, v->tee_ID );
					file->WriteString( line );
					if( is<nsegs )
					{
						
						s = &(c->seg[is]);
						line.Format( "    seg: %d %d %d 0 0\n", 
							is+1, s->layer, s->width );
						file->WriteString( line );
					}
				}
			}
			for( int ia=0; ia<net->nareas; ia++ )
			{
				line.Format( "  area: %d %d %d %d %d %d %d\n", ia+1, 
					net->area[ia].poly->GetNumCorners(),
					net->area[ia].poly->GetLayer(),
					net->area[ia].poly->GetHatch(),
					net->area[ia].poly->GetW(),
					net->area[ia].poly->GetMerge(),
					net->area[ia].selected
					);
				file->WriteString( line );
				int end;
				int max_c = net->area[ia].poly->GetNumCorners();
				for( int icor=0; icor<max_c; icor++ )
				{
					if (icor == (max_c-1) )
						end = 1;
					else if (net->area[ia].poly->GetNumContour(icor) < net->area[ia].poly->GetNumContour(icor+1) )
						end = 1;
					else 
						end = 0;
					line.Format( "    corner: %d %d %d %d %d\n", icor+1,
						net->area[ia].poly->GetX( icor ),
						net->area[ia].poly->GetY( icor ),
						net->area[ia].poly->GetSideStyle( icor ),
						end
						);
					file->WriteString( line );
				}
			}
			file->WriteString( "\n" );
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

// read netlist from file
// throws err_str on error
//
int CNetList::ReadNets( CStdioFile * pcb_file, double read_version, int * InLayer )
{
	int err, pos, np;
	CArray<CString> p;
	CString in_str, key_str;

	// find beginning of [nets] section
	do
	{
		err = pcb_file->ReadString( in_str );
		if( !err )
		{
			// error reading pcb file
			CString mess;
			mess.Format( "Unable to find [nets] section in file" );
			AfxMessageBox( mess );
			return 0;
		}
		in_str.Trim();
	}
	while( in_str != "[nets]" );

	//check ratlines
	BOOL bRatline = 0;
	BOOL ch_connect = 0;
	// get each net in [nets] section
	ClearTeeIDs();
	while( 1 )
	{
		pos = (long)pcb_file->GetPosition();
		err = pcb_file->ReadString( in_str );
		if( !err )
		{
			CString * err_str = new CString( "unexpected EOF in project file" );// throw
			throw err_str;
		}
		in_str.Trim();
		if( in_str.Left(1) == "[" && in_str != "[nets]" )
		{
			pcb_file->Seek( pos, CFile::begin );
			return 1;		// start of next section, reset position and exit
		}
		else if( in_str.Left(4) == "net:" )
		{
			np = ParseKeyString( &in_str, &key_str, &p );
			CString net_name = p[0].Left(MAX_NET_NAME_SIZE);
			net_name.Trim();
			int npins = my_atoi( &p[1] ); 
			int nconnects = my_atoi( &p[2] );
			int nareas = my_atoi( &p[3] );
			int def_width = my_atoi( &p[4] );
			int def_via_w = my_atoi( &p[5] );
			int def_via_hole_w = my_atoi( &p[6] );
			int visible = 1;
			if( np >= 9 )
				visible = my_atoi( &p[7] );
			cnet * net = AddNet( net_name, def_width, def_via_w, def_via_hole_w );
			net->visible = visible;
			for( int ip=0; ip<npins; ip++ )
			{
				err = pcb_file->ReadString( in_str );
				if( !err )
				{
					CString * err_str = new CString( "unexpected EOF in project file" );// throw
					throw err_str;
				}
				np = ParseKeyString( &in_str, &key_str, &p );
				if( key_str != "pin" || np < 3 )
				{
					CString * err_str = new CString( "error parsing [nets] section of project file" );// throw
					throw err_str;
				}
				CString pin_str = p[1].Left(CShape::MAX_PIN_NAME_SIZE);
				int dot_pos = pin_str.FindOneOf( "." );
				CString ref_str = pin_str.Left( dot_pos );
				CString pin_num_str = pin_str.Right( pin_str.GetLength()-dot_pos-1 );
				AddNetPin( net, &ref_str, &pin_num_str, FALSE );
			}
			for( int ic=0; ic<nconnects; ic++ )
			{
				ch_connect = 1;
				err = pcb_file->ReadString( in_str );
				if( !err )
				{
					CString * err_str = new CString( "unexpected EOF in project file" );// throw
					throw err_str;
				}
				np = ParseKeyString( &in_str, &key_str, &p );
				if( key_str != "connect" || np < 6 )
				{
					CString * err_str = new CString( "error parsing [nets] section of project file" );// throw
					throw err_str;
				}
				int start_pin = my_atoi( &p[1] );
				int end_pin = my_atoi( &p[2] );
				// check for fatal errors
				CString test_ref_des = net->pin[start_pin].ref_des;
				cpart * test_part = net->pin[start_pin].part;
				if( !test_part )
				{
					CString * err_str = new CString( "fatal error in net \"" );// throw
					*err_str += net_name + "\"";
					*err_str += "\r\n\rpart \"" + test_ref_des + "\" doesn't exist";
					throw err_str;
				}
				else if( !test_part->shape )
				{
					CString * err_str = new CString( "fatal error in net \"" );// throw
					*err_str += net_name + "\"";
					*err_str += "\r\n\rpart \"" + test_ref_des + "\" doesn't haved a footprint";
					throw err_str;
				}
				else
				{
					CString test_pin_name = net->pin[start_pin].pin_name;
					int pin_index = test_part->shape->GetPinIndexByName( test_pin_name, -1 );
					if( pin_index == -1 )
					{
						CString * err_str = new CString( "fatal error in net \"" );// throw
						*err_str += net_name + "\"";
						*err_str += "\r\n\r\npin \"" + test_pin_name + "\"";
						*err_str += " doesn't exist in footprint \"" + test_part->shape->m_name + "\"";
						*err_str += " for part \"" + test_ref_des + "\"";
						throw err_str;
					}
				}
				int nsegs = my_atoi( &p[3] );
				int locked = 0;
				if ( np >= 6 )
					locked = my_atoi( &p[4] );
				int merge = -1;
				if ( np >= 7 )
					merge = my_atoi( &p[5] );

				// skip first vertex
				err = pcb_file->ReadString( in_str );
				if( !err )
				{
					CString * err_str = new CString( "unexpected EOF in project file" );// throw
					throw err_str;
				}
				np = ParseKeyString( &in_str, &key_str, &p );
				if( key_str != "vtx" || np < 8 )
				{
					CString * err_str = new CString( "error parsing [nets] section of project file" );// throw
					throw err_str;
				}
				int v0x = my_atoi( &p[1] ); 
				int v0y = my_atoi( &p[2] ); 
				int nc;
				if( end_pin != cconnect::NO_END )
					nc = AddNetConnect( net, start_pin, end_pin, v0x, v0y );
				else
					nc = AddNetStub( net, start_pin, v0x, v0y );
				
				if( nc == -1 )
				{
					// invalid connection, remove it with this ugly code
					ic--;
					nconnects--;
					net->connect.SetSize( ic+1 );
					for( int i=0; i<nsegs*2+1; i++ )
					{
						err = pcb_file->ReadString( in_str );
						if( !err )
						{
							CString * err_str = new CString( "unexpected EOF in project file" );// throw
							throw err_str;
						}
					}
				}
				else
				{
					net->connect[ic].vtx[0].x = v0x;
					net->connect[ic].vtx[0].y = v0y;
					net->connect[ic].locked = locked;
					net->connect[ic].m_merge = merge;

					// now add all segments
					int test_not_done = 1;
					int pre_via_w, pre_via_hole_w;
					for( int is=0; is<nsegs; is++ )
					{
						// read segment data
						err = pcb_file->ReadString( in_str );
						if( !err )
						{
							CString * err_str = new CString( "unexpected EOF in project file" );// throw
							throw err_str;
						}
						np = ParseKeyString( &in_str, &key_str, &p );
						if( key_str != "seg" || np < 6 )
						{
							CString * err_str = new CString( "error parsing [nets] section of project file" );// throw
							throw err_str;
						}
						int layer = my_atoi( &p[1] ); 
						if( read_version < 1.3 )
							layer += 2;
						if( read_version < 2.0199 )
							layer += 3;
						int seg_width = my_atoi( &p[2] ); 
						// read following vertex data
						err = pcb_file->ReadString( in_str );
						if( !err )
						{
							CString * err_str = new CString( "unexpected EOF in project file" );// throw
							throw err_str;
						}
						np = ParseKeyString( &in_str, &key_str, &p );
						if( key_str != "vtx" || np < 8 )
						{
							CString * err_str = new CString( "error parsing [nets] section of project file" );// throw
							throw err_str;
						}
						if( test_not_done )
						{
							// only add segments if we are not done
							int x = my_atoi( &p[1] ); 
							int y = my_atoi( &p[2] ); 
							int pad_lay = my_atoi( &p[3] );

							if( read_version < 2.0199 )
								pad_lay += 3;
							int force_via_flag = my_atoi( &p[4] ); 
							int via_w = my_atoi( &p[5] ); 
							int via_hole_w = my_atoi( &p[6] );
							int tee_ID = 0;
							if( np == 9 )
							{
								tee_ID = my_atoi( &p[7] );
								if( tee_ID )
									AddTeeID( tee_ID );
							}
							if( end_pin != cconnect::NO_END )
							{
								if( is == (nsegs-1) )
								{
									// last segment of pin-pin connection 
									// force segment to end on pin
									cpart * end_part = net->pin[end_pin].part;
									int index = GetPinIndexByNameForPart( end_part, net->pin[end_pin].pin_name, x, y );
									net->connect[ic].end_pin_shape = index;	
									net->connect[ic].vtx[nsegs].x = x;
									net->connect[ic].vtx[nsegs].y = y;
									net->connect[ic].seg[nsegs-1].layer = layer;
									SetSegmentWidth( net, ic, is, seg_width, 0, 0 );
									//
									int gpl = m_plist->GetPinLayer( end_part, index );
									net->connect[ic].vtx[nsegs].pad_layer = gpl;
								}
								else
									test_not_done = InsertSegment( net, ic, is, x, y, layer, seg_width, 0, 0, 0 );
							}
							else
							{
								AppendSegment( net, ic, x, y, layer, seg_width );
								// set widths of following vertex
								net->connect[ic].vtx[is+1].via_w = via_w;
								net->connect[ic].vtx[is+1].via_hole_w = via_hole_w;
							}
							if( layer == LAY_RAT_LINE )
								bRatline = 1;
							//** this code is for bug in versions before 1.313
							if( force_via_flag )
							{
								if( end_pin == cconnect::NO_END && is == nsegs-1 )
									ForceVia( net, ic, is+1 );
								else if( read_version > 1.312001 )	// i.e. 1.313 or greater
									ForceVia( net, ic, is+1 );
							}
							net->connect[ic].vtx[is+1].tee_ID = tee_ID;
							if( is != 0 )
							{
								// set widths of preceding vertex
								net->connect[ic].vtx[is].via_w = pre_via_w;
								net->connect[ic].vtx[is].via_hole_w = pre_via_hole_w;
								if( m_dlist )
									DrawVia( net, ic, is );
							}
							pre_via_w = via_w;
							pre_via_hole_w = via_hole_w;
						}
					}
				}
			}
			for( int ia=0; ia<nareas; ia++ )
			{
				err = pcb_file->ReadString( in_str );
				if( !err )
				{
					CString * err_str = new CString( "unexpected EOF in project file" );// throw
					throw err_str;
				}
				np = ParseKeyString( &in_str, &key_str, &p );
				if( key_str != "area" || np < 4 )
				{
					CString * err_str = new CString( "error parsing [nets] section of project file" );// throw
					throw err_str;
				}
				int na = my_atoi( &p[0] );
				if( (na-1) != ia )
				{
					CString * err_str = new CString( "error parsing [nets] section of project file" );// throw
					throw err_str;
				}
				int ncorners = my_atoi( &p[1] );
				int file_layer = my_atoi( &p[2] );
				int layer = file_layer;//in_layer[file_layer]; 
				if( read_version < 1.3 )
					layer += 2;
				if( read_version < 2.0199 )
					layer += 3;
				int hatch = 0;
				if( np >= 5 )
					hatch = my_atoi( &p[3] );
				int width = 0;
				if( np >= 6 )
					width = my_atoi( &p[4] );
				int merge = -1;
				if( np >= 7 )
					merge = my_atoi( &p[5] );
				int last_side_style = CPolyLine::STRAIGHT;
				for( int icor=0; icor<ncorners; icor++ )
				{
					err = pcb_file->ReadString( in_str );
					if( !err )
					{
						CString * err_str = new CString( "unexpected EOF in project file" );// throw
						throw err_str;
					}
					np = ParseKeyString( &in_str, &key_str, &p );
					if( key_str != "corner" || np < 4 )
					{
						CString * err_str = new CString( "error parsing [nets] section of project file" );// throw
						throw err_str;
					} 
					int ncor = my_atoi( &p[0] );
					if( (ncor-1) != icor )
					{
						CString * err_str = new CString( "error parsing [nets] section of project file" );// throw
						throw err_str;
					}
					int x = my_atoi( &p[1] );
					int y = my_atoi( &p[2] );
					if( icor == 0 )
					{
						AddArea( net, layer, x, y, hatch, ncorners );
						net->area[ia].poly->SetW(width);
						net->area[ia].poly->SetMerge(merge);
					}
					else
						AppendAreaCorner( net, ia, x, y, last_side_style, FALSE );
					if( np >= 5 )
						last_side_style = my_atoi( &p[3] );
					else
						last_side_style = CPolyLine::STRAIGHT;
					int end_cont = 0;
					if( np >= 6 )
						end_cont = my_atoi( &p[4] );
					if( icor == (ncorners-1) )
					{
						CompleteArea( net, ia, last_side_style );
					}
					else if (end_cont)
					{
						net->area[ia].poly->Close( last_side_style, FALSE, FALSE );
					}
				}
			}
			for(  int ia=nareas-1; ia>=0; ia-- )
			{
				if( net->area[ia].poly->GetNumCorners() < 3 )
					RemoveArea( net, ia );
			}
			CleanUpConnections( net );
			RemoveOrphanBranches( net, 0 );
			SetAreaConnections( net );
		}	
	}
	if( ch_connect == 0 || bRatline )
		return 1;
	return 0;
}

// undraw via
//
void CNetList::UndrawVia( cnet * net, int ic, int iv )
{
	cconnect * c = &net->connect[ic];
	cvertex * v = &c->vtx[iv];
	m_dlist->Remove( v->dl_el );
	m_dlist->Remove( v->dl_hole );
	v->dl_el = NULL;
	v->dl_hole = NULL;
}

// draw vertex
//	i.e. draw selection box, draw via if needed
//
int CNetList::DrawVia( cnet * net, int ic, int iv )
{
	cconnect * c = &net->connect[ic];
	cvertex * v = &c->vtx[iv];
	if( v->m_dlist == NULL || c->nsegs == 0 )
	{
		ASSERT(0);
		v->m_dlist = m_dlist;
	}
	// undraw previous via and selection box 
	UndrawVia( net, ic, iv );
	//
	RECT rect;
	id vid( ID_NET, ID_CONNECT, ic, ID_VERTEX, iv );
	int l_map = 0;
	v->layer_bit = 0;
	if( iv && v->via_w )
	{
		// draw via		
		int min_layer, max_layer;
		int n_iv = min( c->nsegs-1, iv );
		min_layer = min( c->seg[iv-1].layer, c->seg[n_iv].layer );
		max_layer = max( c->seg[iv-1].layer, c->seg[n_iv].layer );
		if( v->tee_ID )
		{
			int tee_ic = ic; 
			int tee_iv = iv;
			while( FindTeeVertexInNet( net, v->tee_ID, &tee_ic, &tee_iv ) )
			{
				min_layer = min( min_layer, net->connect[tee_ic].seg[tee_iv-1].layer );
				max_layer = max( max_layer, net->connect[tee_ic].seg[tee_iv-1].layer );
			}
		}
		if( min_layer >= LAY_TOP_COPPER )
			setbit( l_map, min_layer );
		if( max_layer >= LAY_TOP_COPPER )
			setbit( l_map, max_layer );
		if( v->m_micro )
		{			
			if( min_layer == LAY_BOTTOM_COPPER )
			{
				setbit( v->layer_bit, LAY_BOTTOM_COPPER );	
				for( int il=max_layer; il<max_layer+m_layers-2; il++ )
					setbit( v->layer_bit, il );
			}
			else if( min_layer == LAY_TOP_COPPER )
			{
				setbit( v->layer_bit, LAY_TOP_COPPER );
				if( max_layer == LAY_BOTTOM_COPPER )
					setbit( v->layer_bit, LAY_BOTTOM_COPPER );
				for( int il=LAY_BOTTOM_COPPER+1; il<=max_layer; il++ )
					setbit( v->layer_bit, il );
			}
			else
			{
				for( int il=min_layer; il<=max_layer; il++ )
					setbit( v->layer_bit, il );
			}
		}
		else
		{
			for( int il=0; il<m_layers; il++ )
				setbit( v->layer_bit, il+LAY_TOP_COPPER );
			setbit( l_map, LAY_TOP_COPPER );
			setbit( l_map, LAY_BOTTOM_COPPER );
		}
		//
		// add
		rect.left =   v->x-v->via_hole_w/2;
		rect.right =  v->x+v->via_hole_w/2;
		rect.bottom = v->y-v->via_hole_w/2;
		rect.top =	  v->y+v->via_hole_w/2;
		int hole = 0;
		setbit( hole, LAY_PAD_THRU );
		v->dl_hole = m_dlist->Add(	vid, net, hole, DL_HOLE, 1, 
									&rect, 0, NULL, 0 );	
		rect.left =   v->x-v->via_w/2;
		rect.right =  v->x+v->via_w/2;
		rect.bottom = v->y-v->via_w/2;
		rect.top =	  v->y+v->via_w/2;
		v->dl_el = m_dlist->Add(	vid, net, l_map, DL_CIRC, 1, 
									&rect, 0, NULL, 0 );	
		m_dlist->AddSelector( v->dl_el );
	}
	else
	{
		int W, lay;
		// 
		if( iv == 0 )
		{
			lay = c->vtx[iv].pad_layer;
			W = c->seg[iv].width*2/5;
		}
		else if( iv == c->nsegs && c->end_pin >= 0 )
		{
			lay = c->vtx[iv].pad_layer;
			W = c->seg[iv-1].width*2/5;
		}
		else if( iv < c->nsegs )
		{
			lay = c->seg[iv].layer;
			W = c->seg[iv].width*2/5;
		}
		else
		{
			lay = c->seg[iv-1].layer;
			W = c->seg[iv-1].width*2/5;
		}

		W = max( abs(W), 2*PCBU_PER_MIL );
		setbit( l_map, lay );
		if( !v->tee_ID || iv < c->nsegs )
		{
			rect.left =   v->x-W;
			rect.right =  v->x+W;
			rect.bottom = v->y-W;
			rect.top =	  v->y+W;
			v->dl_el = m_dlist->Add(	vid, net, l_map, DL_CIRC, 0, 
										&rect, 0, NULL, 0 );
			if( iv && ( iv<c->nsegs || c->end_pin == cconnect::NO_END ) )
				m_dlist->AddSelector( v->dl_el );
		}
	}
	return 0;
}







void CNetList::SetNetVisibility( cnet * net, BOOL visible )
{
	if( net->visible == visible )
		return;
	else if( visible )
	{
		// make segments visible and enable selection items
		for( int ic=0; ic<net->nconnects; ic++ )
		{
			cconnect * c = &net->connect[ic];
			for( int is=0; is<c->nsegs; is++ )
			{
				c->seg[is].dl_el->visible = TRUE;
			}
		}
		// make thermals visible
		for( int ia=0; ia<net->nareas; ia++ )
		{
			for( int ip=0; ip<net->area[ia].npins; ip++ )
			{
				net->area[ia].dl_thermal[ip]->visible = TRUE;
			}
		}
	}
	else
	{
		// make ratlines invisible and disable selection items
		for( int ic=0; ic<net->nconnects; ic++ )
		{
			cconnect * c = &net->connect[ic];
			for( int is=0; is<c->nsegs; is++ )
			{
				if( c->seg[is].layer == LAY_RAT_LINE )
				{
					c->seg[is].dl_el->visible = FALSE;
				}
			}
		}
		// make thermals invisible
		for( int ia=0; ia<net->nareas; ia++ )
		{
			for( int ip=0; ip<net->area[ia].npins; ip++ )
			{
				net->area[ia].dl_thermal[ip]->visible = FALSE;
			}
		}
	}
	net->visible = visible;
}

BOOL CNetList::GetNetVisibility( cnet * net )
{
	return net->visible;
}

// export netlist data into a netlist_info structure so that it can
// be edited in a dialog
//
void CNetList::ExportNetListInfo( netlist_info * nl )
{
	// make copy of netlist data so that it can be edited
	POSITION pos;
	CString name;
	void * ptr;
	CString str;
	int i = 0;
	nl->SetSize( m_map.GetSize() );
	for( pos = m_map.GetStartPosition(); pos != NULL; )
	{
		m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		(*nl)[i].name = net->name;
		(*nl)[i].net = net;
		(*nl)[i].visible = GetNetVisibility( net );
		(*nl)[i].w = net->def_w;
		(*nl)[i].v_w = net->def_via_w;
		(*nl)[i].v_h_w = net->def_via_hole_w;
		(*nl)[i].apply_trace_width = FALSE;
		(*nl)[i].apply_via_width = FALSE;
		(*nl)[i].modified = FALSE;
		(*nl)[i].deleted = FALSE;
		(*nl)[i].ref_des.SetSize(0);
		(*nl)[i].pin_name.SetSize(0);
		// now make copy of pin arrays
		(*nl)[i].ref_des.SetSize( net->npins );
		(*nl)[i].pin_name.SetSize( net->npins );
		for( int ip=0; ip<net->npins; ip++ )
		{
			(*nl)[i].ref_des[ip] = net->pin[ip].ref_des;
			(*nl)[i].pin_name[ip] = net->pin[ip].pin_name;
		}
		i++;
	}
}

// import netlist_info data back into netlist
//
void CNetList::ImportNetListInfo( netlist_info * nl, int flags, CDlgLog * log,
								 int def_w, int def_w_v, int def_w_v_h )
{
	CString mess;
	if ( def_w < 0 )
		def_w = -def_w;
	if ( def_w_v < 0 )
		def_w_v = -def_w_v;
	if ( def_w_v_h < 0 )
		def_w_v_h = -def_w_v_h;
	// loop through netlist_info and remove any nets that flagged for deletion
	int n_info_nets = nl->GetSize();
	for( int i=0; i<n_info_nets; i++ )
	{
		cnet * net = (*nl)[i].net;
		if( (*nl)[i].deleted && net )
		{
			// net was deleted, remove it
			if( log )
			{
				mess.Format( "  Removing net \"%s\"\r\n", net->name );
				log->AddLine( mess );
			}
			RemoveNet( net );
			(*nl)[i].net = NULL;
		}
	}

	// now handle any nets that were renamed 
	// assumes that the new name is not a duplicate
	for( int i=0; i<n_info_nets; i++ )
	{
		cnet * net = (*nl)[i].net;
		if( net )
		{
			CString new_name = (*nl)[i].name;
			CString old_name = net->name;
			if( old_name != new_name )
			{
				m_map.RemoveKey( old_name );
				m_map.SetAt( new_name, net );
				net->name = new_name;	// rename net
			}
		}
	}

	// now check for existing nets that are not in netlist_info
	CArray<cnet*> delete_these;
	cnet * net = GetFirstNet();
	while( net )
	{
		// check if in netlist_info
		BOOL bFound = FALSE;
		for( int i=0; i<nl->GetSize(); i++ )
		{
			if( (*nl)[i].name.Compare(net->name) == 0 )
			{
				bFound = TRUE;
				break;
			}
		}
		if( !bFound )
		{
			// net is not in netlist_info
			if( flags & KEEP_NETS )
			{
				if( log )
				{
					mess.Format( "  Keeping net \"%s\", not in imported netlist\r\n", net->name );
					log->AddLine( mess );
				}
			}
			else
			{
				if( log )
				{
					mess.Format( "  Removing net \"%s\"\r\n", net->name );
					log->AddLine( mess );
				}
				delete_these.Add( net );	// flag for deletion
			}
		}
		net = GetNextNet(/*LABEL*/);
	}
	// delete them
	for( int i=0; i<delete_these.GetSize(); i++ )
	{
		RemoveNet( delete_these[i] );
	}

	// now reloop, adding and modifying nets and deleting pins as needed
	for( int i=0; i<n_info_nets; i++ )
	{
		// ignore info nets marked for deletion
		if( (*nl)[i].deleted )
			continue;

		// try to find existing net with this name
		cnet * net = (*nl)[i].net;	// net from netlist_info (may be NULL)
		cnet * old_net = NULL;
		old_net = GetNetPtrByName( &(*nl)[i].name );
		if( net == NULL && old_net == NULL )
		{
			// no existing net, add to netlist
			if( (*nl)[i].w == -1 )
				(*nl)[i].w = 0;
			if( (*nl)[i].v_w == -1 )
				(*nl)[i].v_w = 0;
			if( (*nl)[i].v_h_w == -1 )
				(*nl)[i].v_h_w = 0;
			net = AddNet( (*nl)[i].name, 
				(*nl)[i].w, (*nl)[i].v_w, (*nl)[i].v_h_w );
			(*nl)[i].net = net;
		}
		else if( net == NULL && old_net != NULL )
		{
			// no net from netlist_info but existing net with same name
			// use existing net and modify it
			(*nl)[i].modified = TRUE;
			net = old_net;
			(*nl)[i].net = net;
			if( (*nl)[i].w != -1 )
				net->def_w = (*nl)[i].w;
			if( (*nl)[i].v_w != -1 )
				net->def_via_w = (*nl)[i].v_w;
			if( (*nl)[i].v_h_w != -1 )
				net->def_via_hole_w = (*nl)[i].v_h_w;
		}
		else
		{
			// net from netlist_info and existing net have the same name
			if( net != old_net )
				ASSERT(0);	// make sure that they are actually the same net
			// modify existing net parameters, unless undefined
			if( (*nl)[i].w != -1 )
				net->def_w = (*nl)[i].w;
			if( (*nl)[i].v_w != -1 )
				net->def_via_w = (*nl)[i].v_w;
			if( (*nl)[i].v_h_w != -1 )
				net->def_via_hole_w = (*nl)[i].v_h_w;
		}

		// now set pin lists
		net->name = (*nl)[i].name;
		// now loop through net pins, deleting any which were removed
		for( int ipn=0; ipn<net->npins; )
		{
			CString ref_des = net->pin[ipn].ref_des;
			CString pin_name = net->pin[ipn].pin_name;
			BOOL pin_present = FALSE;
			for( int ip=0; ip<(*nl)[i].ref_des.GetSize(); ip++ )
			{
				if( ref_des == (*nl)[i].ref_des[ip]
				&& pin_name == (*nl)[i].pin_name[ip] )
				{
					// pin in net found in netlist_info
					pin_present = TRUE;
					break;
				}
			}
			if( !pin_present )
			{
				// pin in net but not in netlist_info 
				if( flags & KEEP_PARTS_AND_CON )
				{
					// we may want to preserve this pin
					cpart * part = m_plist->GetPart( ref_des );
					if( !part )
						RemoveNetPin( net, &ref_des, &pin_name, FALSE );
					else if( !part->bPreserve )
						RemoveNetPin( net, &ref_des, &pin_name, FALSE );
					else
					{
						// preserve the pin
						ipn++;
					}
				}
				else
				{
					// delete it from net
					if( log )
					{
						mess.Format( "    Removing pin %s.%s from net \"%s\"\r\n", 
							ref_des, pin_name, net->name  );
						log->AddLine( mess );
					}
					RemoveNetPin( net, &ref_des, &pin_name, FALSE );
				}
			}
			else
			{
				ipn++;
			}
		}
	}

	// now reloop and add any pins that were added to netlist_info, 
	// and delete any duplicates
	// separate loop to ensure that pins were deleted from all nets
	for( int i=0; i<n_info_nets; i++ )
	{
		cnet * net = (*nl)[i].net;
		if( net && !(*nl)[i].deleted && (*nl)[i].modified )
		{
			// loop through local pins, adding any new ones to net
			int n_local_pins = (*nl)[i].ref_des.GetSize();
			for( int ipl=0; ipl<n_local_pins; ipl++ )
			{
				// delete this pin from any other nets
				cnet * test_net = GetFirstNet();
				while( test_net )
				{
					if( test_net != net )
					{
						// test for duplicate pins
						for( int test_ip=test_net->npins-1; test_ip>=0; test_ip-- )
						{
							if( test_net->pin[test_ip].ref_des == (*nl)[i].ref_des[ipl] 
							&& test_net->pin[test_ip].pin_name == (*nl)[i].pin_name[ipl] )
							{
								if( log )
								{
									mess.Format( "    Removing pin %s.%s from net \"%s\"\r\n", 
										test_net->pin[test_ip].ref_des,
										test_net->pin[test_ip].pin_name,
										test_net->name  );
									log->AddLine( mess );
								}
								RemoveNetPin( test_net, test_ip );
							}
						}
					}
					test_net = GetNextNet(/*LABEL*/);
				}
				// now test for pin already present in net
				BOOL pin_present = FALSE;
				for( int ipp=0; ipp<net->npins; ipp++ )
				{
					if( net->pin[ipp].ref_des == (*nl)[i].ref_des[ipl]
					&& net->pin[ipp].pin_name == (*nl)[i].pin_name[ipl] )
					{
						// pin in local array found in net
						pin_present = TRUE;
						break;
					}
				}
				if( !pin_present )
				{
					// pin not in net, add it
					AddNetPin( net, &(*nl)[i].ref_des[ipl], &(*nl)[i].pin_name[ipl] );
					if( log )
					{
						mess.Format( "    Adding pin %s.%s to net \"%s\"\r\n", 
							(*nl)[i].ref_des[ipl],
							(*nl)[i].pin_name[ipl],
							net->name  );
						log->AddLine( mess );
					}
				}
			}
		}
	}

	// now set visibility and apply new widths, if requested
	for( int i=0; i<n_info_nets; i++ )
	{
		cnet * net = (*nl)[i].net;
		if( net )
		{
			SetNetVisibility( net, (*nl)[i].visible ); 
			if( (*nl)[i].apply_trace_width )
			{
				int w = (*nl)[i].w;
				if( !w )
					w = def_w;
				SetNetWidth( net, w, 0, 0 ); 
			}
			if( (*nl)[i].apply_via_width )
			{
				int w_v = (*nl)[i].v_w;
				int w_v_h = (*nl)[i].v_h_w;
				if( !w_v )
					w_v = def_w_v;
				if( !w_v_h )
					w_v_h = def_w_v_h;
				SetNetWidth( net, 0, w_v, w_v_h ); 
			}
		}
	}
	CleanUpAllConnections();
}

// Copy all data from another netlist (except display elements)
//
void CNetList::Copy( CNetList * src_nl )
{
	RemoveAllNets();
	cnet * src_net = src_nl->GetFirstNet();
	while( src_net )
	{
		cnet * net = AddNet( src_net->name, 0, 0, 0 );
		net->pin.SetSize( src_net->npins );
		for( int ip=0; ip<src_net->npins; ip++ )
		{
			// add pin but don't modify part->pin->net
			//net->pin[ip] = src_net->pin[ip];
			net->pin[ip].part = NULL;
			if( m_plist )
				net->pin[ip].part = src_net->pin[ip].part;
			net->pin[ip].pin_name = src_net->pin[ip].pin_name;
			net->pin[ip].ref_des = src_net->pin[ip].ref_des;
			net->pin[ip].utility = src_net->pin[ip].utility;
		}
		net->npins = src_net->npins;
		for( int ia=0; ia<src_net->nareas; ia++ )
		{
			carea * src_a = &src_net->area[ia];
			CPolyLine * src_poly = src_a->poly;
			ia = AddArea( net, src_poly->GetLayer(), src_poly->GetX(0),
						  src_poly->GetY(0), src_poly->GetHatch() );
			if( ia == -1 )
				continue;
			carea * a = &net->area[ia];	
			CPolyLine * poly = a->poly;
			poly->Copy( src_poly );
			poly->SetDisplayList( NULL );
			a->npins = src_a->npins;
			a->pin.SetSize( a->npins );
			for( int ip=0; ip<a->npins; ip++ )
				a->pin[ip] = src_a->pin[ip];
			a->nvias = src_a->nvias;
			a->vcon.SetSize( a->nvias );
			a->vtx.SetSize( a->nvias );
			for( int ic=0; ic<a->nvias; ic++ )
			{
				a->vcon[ic] = src_a->vcon[ic];
				a->vtx[ic] = src_a->vtx[ic];
			}
		}
		for( int ic=0; ic<src_net->nconnects; ic++ )
		{
			cconnect * src_c = &src_net->connect[ic];
			int icn = 0;
			if( src_c->end_pin == cconnect::NO_END )
				icn = AddNetStub( net, src_c->start_pin );
			else
				icn = AddNetConnect( net, src_c->start_pin, src_c->end_pin );
			if( icn == -1 )
				continue;
			cconnect * c = &net->connect[icn];
			c->m_merge = src_c->m_merge;
			c->seg.SetSize( src_c->nsegs );
			for( int is=0; is<src_c->nsegs; is++ )
			{
				c->seg[is] = src_c->seg[is];
				c->seg[is].dl_el = NULL;
			}
			c->nsegs = src_c->nsegs;
			c->vtx.SetSize( c->nsegs+1 );
			for( int iv=0; iv<=c->nsegs; iv++ )
			{
				cvertex * v = &c->vtx[iv];
				cvertex * src_v = &src_c->vtx[iv];
				v->x = src_v->x;
				v->y = src_v->y;
				v->pad_layer = src_v->pad_layer;
				v->force_via_flag = src_v->force_via_flag;
				v->via_w = src_v->via_w;
				v->via_hole_w = src_v->via_hole_w;
				v->tee_ID = src_v->tee_ID;
				v->layer_bit = src_v->layer_bit;
				v->m_micro = src_v->m_micro;
			}
		}
		net->utility = src_net->utility;
		src_net = src_nl->GetNextNet(/*LABEL*/);
	}
}

// reassign copper elements to new layers
// enter with layer[] = table of new copper layers for each old copper layer
//
void CNetList::ReassignCopperLayers( int n_new_layers, int * layer )
{
	if( m_layers < 1 || m_layers > 16 )
		ASSERT(0);
	cnet * net = GetFirstNet();
	while( net )
	{
		for( int ic=0; ic<net->nconnects; ic++ )
		{
			cconnect * c = &net->connect[ic];
			for( int is=0; is<c->nsegs; is++ )
			{
				cseg * s = &c->seg[is];
				int old_layer = s->layer;
				if( old_layer >= LAY_TOP_COPPER )
				{
					int index = old_layer - LAY_TOP_COPPER;
					int new_layer = layer[index];
					if( new_layer == -1 )
					{
						// delete this layer
						net->connect[ic].seg[is].width = 0;
						net->connect[ic].seg[is].layer = LAY_RAT_LINE;
					}
					else
						s->layer = new_layer + LAY_TOP_COPPER;
				}
			}
			// check for first or last segments connected to SMT pins
			cvertex * v = &c->vtx[0];
			cseg * s = &c->seg[0];
			if( v->pad_layer && v->pad_layer != LAY_PAD_THRU && s->layer != v->pad_layer )
			{
				net->connect[ic].seg[0].width = 0;
				net->connect[ic].seg[0].layer = LAY_RAT_LINE;
			}
			if( c->end_pin != cconnect::NO_END )
			{
				v = &c->vtx[c->nsegs];
				s = &c->seg[c->nsegs-1];
				if( v->pad_layer && v->pad_layer != LAY_PAD_THRU && s->layer != v->pad_layer )
				{
					net->connect[ic].seg[c->nsegs-1].width = 0;
					net->connect[ic].seg[c->nsegs-1].layer = LAY_RAT_LINE;
				}
			}
			MergeUnroutedSegments( net, ic );
		}
		CleanUpConnections( net );
		for( int ia=net->nareas-1; ia>=0; ia-- )
		{
			CPolyLine * poly = net->area[ia].poly;
			int old_layer = poly->GetLayer();
			int index = old_layer - LAY_TOP_COPPER;
			int new_layer = layer[index];
			if( new_layer == -1 )
			{
				// delete this area
				RemoveArea( net, ia );
			}
			else
			{
				poly->SetLayer( new_layer + LAY_TOP_COPPER );
				poly->Draw();
			}
		}
		CombineAllAreasInNet( net, TRUE, FALSE );
		net = GetNextNet(/*LABEL*/);
	}
	m_layers = n_new_layers;
}

// When net names change, try to restore traces and areas from previous netlist
//
void CNetList::RestoreConnectionsAndAreas( CNetList * old_nl, int flags, CDlgLog * log )
{
	// loop through old nets
	old_nl->MarkAllNets( 0 );
	cnet * old_net = old_nl->GetFirstNet();
	while( old_net )
	{
		if( flags & KEEP_AREAS )
		{
			BOOL bMoveIt = FALSE;
			cnet * new_area_net = NULL;
			// see if copper areas can be moved because all pins are on the same new net
			for( int ia=0; ia<old_net->nareas; ia++ )
			{		
				carea * old_a = &old_net->area[ia];
				for( int ip=0; ip<old_a->npins; ip++ )
				{
					int old_pin_index = old_a->pin[ip];
					if( old_pin_index >= old_net->pin.GetSize() || old_pin_index < 0 )
					{
						//ASSERT(0);
					}
					else
					{
						cpin * old_pin = &old_net->pin[old_pin_index];
						cpart * new_pin_part = m_plist->GetPart( old_pin->ref_des );
						cnet * new_pin_net = NULL;
						if( new_pin_part )
							new_pin_net = m_plist->GetPinNet( new_pin_part, &old_pin->pin_name );
						if( new_pin_net )
						{
							bMoveIt = TRUE;
							new_area_net = new_pin_net;
							break;
						}
					}
				}
				if(!bMoveIt) for( int iv=0; iv<old_a->nvias; iv++ )
				{
					int i_con = old_a->vcon[iv];
					if( i_con >= old_net->connect.GetSize() || i_con < 0 )
					{
						//ASSERT(0);
					}
					else
					{
						int old_pin_index = old_net->connect[i_con].start_pin;
						cpin * old_pin = &old_net->pin[old_pin_index];
						cpart * new_pin_part = m_plist->GetPart( old_pin->ref_des );
						cnet * new_pin_net = NULL;
						if( new_pin_part )
							new_pin_net = m_plist->GetPinNet( new_pin_part, &old_pin->pin_name );
						if( new_pin_net )
						{
							bMoveIt = TRUE;
							new_area_net = new_pin_net;
							break;
						}
					}
				}
				if( bMoveIt )
					break;
			}
			if( bMoveIt ) 
				if( old_net->name.Compare( new_area_net->name ) )
					for( int iarea=0; iarea<old_net->nareas; iarea++ )
					{
						carea * old_a = &old_net->area[iarea];
						if( log )
						{
							CString line;
							if( !bMoveIt )
								line.Format( "  Removing copper area on old net \"%s\"\r\n",
								old_net->name );
							else
								line.Format( "  Moving copper area from old net \"%s\" to new net \"%s\"\r\n",
								old_net->name, new_area_net->name );
							log->AddLine( line );
						}
						// move the area onto the new net
						CPolyLine * old_poly = old_a->poly;
						cnet * net = new_area_net;
						int ia = AddArea( net, old_poly->GetLayer(), old_poly->GetX(0),
							old_poly->GetY(0), old_poly->GetHatch() );
						carea * a = &net->area[ia];
						CPolyLine * poly = a->poly;
						poly->Copy( old_poly );
						id p_id( ID_NET, ID_AREA, ia, 0, 0 );
						poly->SetId( &p_id );
						poly->SetPtr( net );
						poly->SetMerge( old_poly->GetMerge() );
						poly->Draw( m_dlist );
						//
						//
						//
						cnet * rem_net = GetNetPtrByName(&old_net->name);
						if( rem_net && rem_net != old_net )
							if( rem_net->nareas > iarea )
								if( rem_net->area[iarea].poly->GetNumCorners() && old_net->area[iarea].poly->GetNumCorners() )
								if( rem_net->area[iarea].poly->GetX(0) == old_net->area[iarea].poly->GetX(0) )
									if( rem_net->area[iarea].poly->GetY(0) == old_net->area[iarea].poly->GetY(0) )
										RemoveArea( rem_net, iarea );
					}
		}
		if( flags & (KEEP_TRACES | KEEP_STUBS) )
		{
			BOOL bRestored = TRUE;	// flag to indicate at least one connection restored
			while( bRestored )
			{
				bRestored = FALSE;	
				// loop through old connections
				for( int old_ic=0; old_ic<old_net->nconnects; old_ic++ )
				{
					cconnect * old_c = &old_net->connect[old_ic];
					if( old_c->utility )
						continue;	// ignore if already flagged 
					if( old_c->nsegs == 1 && old_c->seg[0].layer == LAY_RAT_LINE )
					{
						old_c->utility = 1;
						continue;	// ignore pure ratline connections
					}
					// check net of starting pin
					cpin * old_start_pin = &old_net->pin[old_c->start_pin];
					cpart * new_start_part = m_plist->GetPart( old_start_pin->ref_des );
					cnet * new_start_net = NULL;
					if( new_start_part )
						new_start_net = m_plist->GetPinNet( new_start_part, &old_start_pin->pin_name );
					if( !new_start_net )
					{
						old_c->utility = 1;
						continue;	// ignore if start pin not on any net
					}
					if( new_start_net->name == old_net->name )
					{
						old_c->utility = 1;
						continue;	// ignore if start pin has not changed net
					}
					// check position of starting pin
					int index = GetPinIndexByNameForPart( new_start_part, old_start_pin->pin_name, old_c->vtx[0].x, old_c->vtx[0].y );
					CPoint st_p = m_plist->GetPinPoint( new_start_part, index, new_start_part->side, new_start_part->angle );
					int st_l = m_plist->GetPinLayer( new_start_part, index );
					if( st_p.x != old_c->vtx[0].x || st_p.y != old_c->vtx[0].y )
					{
						old_c->utility = 1;
						continue;	// ignore if start pin position has changed
					}
					if( st_l != LAY_PAD_THRU && old_c->seg[0].layer != LAY_RAT_LINE && st_l != old_c->seg[0].layer )
					{
						old_c->utility = 1;
						continue;	// ignore if start pin layer doesn't match first segment
					}
					// see if we should move trace to new net
					cpin * old_end_pin = NULL;
					if( old_c->end_pin == cconnect::NO_END )
					{
						// stub trace
						int tee_ID = old_c->vtx[old_c->nsegs].tee_ID;
						if( !(flags & KEEP_STUBS) && tee_ID == 0 )
						{
							old_c->utility = 1;
							continue;	// ignore if we are not moving stubs
						}
						int ci=0, vi=0;
						if( tee_ID && !FindTeeVertexInNet( new_start_net, tee_ID,&ci,&vi ) )
							continue;	// branch, but tee_ID not found in net
					}
					else
					{
						// normal trace
						if( flags & KEEP_TRACES )
						{
							old_end_pin = &old_net->pin[old_c->end_pin];
							// see if end pin still exists and is on the same new net
							cpart * new_end_part = m_plist->GetPart( old_end_pin->ref_des );
							cnet * new_end_net = NULL;
							if( new_end_part )
								new_end_net = m_plist->GetPinNet( new_end_part, &old_end_pin->pin_name );
							if( new_start_net != new_end_net )
							{
								old_c->utility = 1;
								continue;
							}
							// check position of end pin
							int index = GetPinIndexByNameForPart( new_end_part, old_end_pin->pin_name, old_c->vtx[old_c->nsegs].x, old_c->vtx[old_c->nsegs].y );
							CPoint e_p = m_plist->GetPinPoint( new_end_part, index, new_end_part->side, new_end_part->angle );
							int e_l = m_plist->GetPinLayer( new_end_part, index );
							if( e_p.x != old_c->vtx[old_c->nsegs].x || e_p.y != old_c->vtx[old_c->nsegs].y )
							{
								old_c->utility = 1;
								continue;	// ignore if end pin position has changed
							}
							if( e_l != LAY_PAD_THRU && old_c->seg[old_c->nsegs-1].layer != LAY_RAT_LINE && e_l != old_c->seg[old_c->nsegs-1].layer )
							{
								old_c->utility = 1;
								continue;	// ignore if end pin layer doesn't match last segment
							}
						}
					}
					// Restore it in new net
					if( log )
					{
						CString line; 
						if( old_c->end_pin == cconnect::NO_END )
						{
							// branch or stub
							int tee_id = old_c->vtx[old_c->nsegs].tee_ID;
							if( !tee_id )
								line.Format( "  Moving stub trace from %s.%s to new net \"%s\"\r\n",
								old_start_pin->ref_des, old_start_pin->pin_name, new_start_net->name );
							else
								line.Format( "  Moving branch from %s.%s to new net \"%s\"\r\n",
								old_start_pin->ref_des, old_start_pin->pin_name, new_start_net->name );
						}
						else
						{
							// pin-pin trace
							line.Format( "  Moving trace from %s.%s to %s.%s to new net \"%s\"\r\n",
								old_start_pin->ref_des, old_start_pin->pin_name,
								old_end_pin->ref_des, old_end_pin->pin_name, 
								new_start_net->name );
						}
						log->AddLine( line );
					}
					cnet * net = new_start_net;
					int ic = -1;
					int new_start_pin_index = GetNetPinIndex( net, &old_start_pin->ref_des, &old_start_pin->pin_name );
					if( old_c->end_pin == cconnect::NO_END )
					{
						ic = AddNetStub( net, new_start_pin_index );
					}
					else
					{
						int new_end_pin_index = GetNetPinIndex( net, &old_end_pin->ref_des, &old_end_pin->pin_name );
						ic = AddNetConnect( net, new_start_pin_index, new_end_pin_index );
					}
					if( ic > -1 )
					{
						old_c->utility = 1;		// flag as already drawn
						bRestored = TRUE;		// and a trace was restored
						UndrawConnection( net, ic );
						cconnect * c = &net->connect[ic];
						c->seg.SetSize( old_c->nsegs );
						for( int is=0; is<old_c->nsegs; is++ )
						{
							c->seg[is] = old_c->seg[is];
							c->seg[is].dl_el = NULL;
						}
						c->nsegs = old_c->nsegs;
						c->vtx.SetSize( old_c->nsegs+1 );
						for( int iv=0; iv<=old_c->nsegs; iv++ )
						{
							cvertex * v = &c->vtx[iv];
							cvertex * src_v = &old_c->vtx[iv];
							v->x = src_v->x;
							v->y = src_v->y;
							v->pad_layer = src_v->pad_layer;
							v->force_via_flag = src_v->force_via_flag;
							v->via_w = src_v->via_w;
							v->via_hole_w = src_v->via_hole_w;
							v->tee_ID = src_v->tee_ID;
							v->m_micro = src_v->m_micro;
							if( v->tee_ID && iv < old_c->nsegs )
								AddTeeID( v->tee_ID );
							v->dl_el = NULL;
							v->dl_hole = NULL;
							v->m_dlist = m_dlist;
						}
						c->m_merge = old_c->m_merge;
						DrawConnection( net, ic );
					}
				}
			}
		}		
		old_net = old_nl->GetNextNet(/*LABEL*/);
	}
}

undo_con * CNetList::CreateConnectUndoRecord( cnet * net, int icon, BOOL set_areas )
{
	// calculate size needed, get memory
	cconnect * c = &net->connect[icon];
	int seg_offset = sizeof(undo_con);
	int vtx_offset = seg_offset + sizeof(undo_seg)*(c->nsegs);
	int size = vtx_offset + sizeof(undo_vtx)*(c->nsegs+1);
	void * ptr = malloc( size );
	undo_con * con = (undo_con*)ptr;
	undo_seg * seg = (undo_seg*)(seg_offset+(UINT)ptr);
	undo_vtx * vtx = (undo_vtx*)(vtx_offset+(UINT)ptr);
	con->uid = c->m_uid;
	con->size = size;
	strcpy( con->net_name, net->name );
	con->merge_name = c->m_merge;
	con->start_pin = c->start_pin; 
	con->end_pin = c->end_pin;
	con->nsegs = c->nsegs;
	con->locked = c->locked;
	con->set_areas_flag = set_areas;
	con->seg_offset = seg_offset;
	con->vtx_offset = vtx_offset;
	for( int is=0; is<c->nsegs; is++ )
	{
		seg[is].uid = c->seg[is].m_uid;
		seg[is].layer = c->seg[is].layer;
		seg[is].width = c->seg[is].width;
	}
	for( int iv=0; iv<=con->nsegs; iv++ )
	{
		vtx[iv].uid = c->vtx[iv].m_uid;
		vtx[iv].x = c->vtx[iv].x;
		vtx[iv].y = c->vtx[iv].y;
		vtx[iv].pad_layer = c->vtx[iv].pad_layer;
		vtx[iv].force_via_flag = c->vtx[iv].force_via_flag;
		vtx[iv].tee_ID = c->vtx[iv].tee_ID;
		vtx[iv].via_w = c->vtx[iv].via_w;
		vtx[iv].via_hole_w = c->vtx[iv].via_hole_w;
		vtx[iv].micro = c->vtx[iv].m_micro;
	}
	con->nlist = this;
	return con;
}

// callback function for undoing connections
// note that this is declared static, since it is a callback
//
void CNetList::ConnectUndoCallback( int type, void * ptr, BOOL undo )
{
	if( undo )
	{
		undo_con * con = (undo_con*)ptr;
		CNetList * nl = con->nlist;
		if( type == UNDO_CONNECT_MODIFY )
		{
			// now recreate connection
			CString temp = con->net_name;
			cnet * net = nl->GetNetPtrByName( &temp ); 
			if( net )
			{
				// get segment and vertex pointers
				undo_seg * seg = (undo_seg*)((UINT)ptr+con->seg_offset);
				undo_vtx * vtx = (undo_vtx*)((UINT)ptr+con->vtx_offset);
				// now add connection
				int nc;
				if( con->nsegs )
				{
					if( con->end_pin != cconnect::NO_END )
						nc = nl->AddNetConnect( net, con->start_pin, con->end_pin, vtx[0].x, vtx[0].y, vtx[con->nsegs].x, vtx[con->nsegs].y );
					else
						nc = nl->AddNetStub( net, con->start_pin, vtx[0].x, vtx[0].y );
					cconnect * c = &net->connect[nc];
					c->m_uid = con->uid;
					c->m_merge = con->merge_name;
					for( int is=0; is<con->nsegs; is++ )
					{
						if( con->end_pin != cconnect::NO_END )
						{
							// pin-pin trace
							nl->InsertSegment( net, nc, is, vtx[is+1].x, vtx[is+1].y,
								seg[is].layer, seg[is].width, seg[is].via_w, seg[is].via_hole_w, 0 );
						}
						else
						{
							// stub trace
							nl->AppendSegment( net, nc, vtx[is+1].x, vtx[is+1].y,
								seg[is].layer, seg[is].width );
						}
					}
					for( int is=0; is<con->nsegs; is++ )
					{
						c->seg[is].m_uid = seg[is].uid;
						c->vtx[is+1].m_uid = vtx[is+1].uid;
						c->vtx[is+1].via_w = vtx[is+1].via_w;
						c->vtx[is+1].via_hole_w = vtx[is+1].via_hole_w;
						c->vtx[is+1].m_micro = vtx[is+1].micro;
						if( vtx[is+1].force_via_flag )
							nl->ForceVia( net, nc, is+1, FALSE );
						c->vtx[is+1].tee_ID = vtx[is+1].tee_ID;
						if( vtx[is+1].tee_ID )
							nl->AddTeeID( vtx[is+1].tee_ID );
						nl->ReconcileVia( net, nc, is+1 );
					}
					// other parameters
					net->connect[nc].locked = con->locked; 
					nl->DrawConnection( net, nc );
				}
			}
		}
		else
			ASSERT(0);
	}
	free( ptr );
}

// Create undo record for a net
// Just saves the name and pin list
// Assumes that connection undo records will be created separately
// for all connections
//
undo_net * CNetList::CreateNetUndoRecord( cnet * net )
{
	int size = sizeof(undo_net) + net->npins*sizeof(undo_pin);
	undo_net * undo = (undo_net*)malloc( size );
	strcpy( undo->name, net->name );
	undo->npins = net->npins;
	undo_pin * un_pin = (undo_pin*)((UINT)undo + sizeof(undo_net));
	for( int ip=0; ip<net->npins; ip++ )
	{
		strcpy( un_pin[ip].ref_des, net->pin[ip].ref_des );
		strcpy( un_pin[ip].pin_name, net->pin[ip].pin_name );
	}
	undo->nlist = this;
	undo->size = size;
	return undo;
}

// callback function for undoing modifications to nets
// removes all connections, and regenerates pin array
// assumes that subsequent callbacks will regenerate connections
// and copper areas
//
void CNetList::NetUndoCallback( int type, void * ptr, BOOL undo )
{
	if( undo )
	{
		// remove all connections from net 
		// assuming that they will be replaced by subsequent undo items
		// do not remove copper areas
		undo_net * undo = (undo_net*)ptr;
		undo_pin * un_pin = (undo_pin*)((UINT)ptr + sizeof(undo_net));
		CNetList * nl = undo->nlist;
		CString temp = undo->name;
		cnet * net = nl->GetNetPtrByName( &temp );
		if( net )
		{
			if( type == UNDO_NET_OPTIMIZE )
			{
				// re-optimize the net
				nl->OptimizeConnections( net, -1, FALSE, -1, FALSE );
			}
			else if( type == UNDO_NET_ADD )
			{
				// just delete the net
				nl->RemoveNet( net );
			}
			else if( type == UNDO_NET_MODIFY )
			{
				// restore the net
				for( int ic=(net->nconnects-1); ic>=0; ic-- )
					nl->RemoveNetConnect( net, ic, FALSE );

				// replace pin data
				net->pin.SetSize(0);
				net->npins = 0;
				for( int ip=0; ip<undo->npins; ip++ )
				{
					CString ref_str( un_pin[ip].ref_des );
					CString pin_name( un_pin[ip].pin_name );
					nl->AddNetPin( net, &ref_str, &pin_name, FALSE );
				}
				nl->RehookPartsToNet( net );
			}
			else
				ASSERT(0);
		}
		// adjust connections to areas
//**		if( net->nareas )
//**			nl->SetAreaConnections( net );
	}
	free( ptr );
}

// create undo record for area
// only includes closed contours
//
undo_area * CNetList::CreateAreaUndoRecord( cnet * net, int iarea, int type )
{
	undo_area * un_a;
	if( type == CNetList::UNDO_AREA_ADD || type == CNetList::UNDO_AREA_CLEAR_ALL )
	{
		un_a = (undo_area*)malloc(sizeof(undo_area));
		strcpy( un_a->net_name, net->name );
		un_a->nlist = this;
		un_a->iarea = iarea;
		return un_a;
	}
	CPolyLine * p = net->area[iarea].poly;
	int n_cont = p->GetNumContours();
	if( !p->GetClosed() )
		n_cont--;
	int nc = p->GetContourEnd( n_cont-1 ) + 1;
	if( type == CNetList::UNDO_AREA_ADD )
		un_a = (undo_area*)malloc(sizeof(undo_area));
	else if( type == CNetList::UNDO_AREA_DELETE 
		|| type == CNetList::UNDO_AREA_MODIFY )
		un_a = (undo_area*)malloc(sizeof(undo_area)+nc*sizeof(undo_corner));
	else
		ASSERT(0);
	un_a->size = sizeof(undo_area)+nc*sizeof(undo_corner);
	strcpy( un_a->net_name, net->name );
	un_a->merge_name = net->area[iarea].poly->GetMerge();
	un_a->iarea = iarea;
	un_a->ncorners = nc;
	un_a->layer = p->GetLayer();
	un_a->hatch = p->GetHatch();
	un_a->w = p->GetW();
	un_a->sel_box_w = p->GetSelBoxSize();
	undo_corner * un_c = (undo_corner*)((UINT)un_a + sizeof(undo_area));
	int st = 0;
	for( int ic=0; ic<nc; ic++ )
	{
		un_c[ic].x = p->GetX( ic );
		un_c[ic].y = p->GetY( ic );
		un_c[ic].num_contour = p->GetNumContour( ic );
		un_c[ic].style = p->GetSideStyle( ic );
	}
	un_a->nlist = this;
	return un_a;
}

// callback function for undoing areas
// note that this is declared static, since it is a callback
//
void CNetList::AreaUndoCallback( int type, void * ptr, BOOL undo )
{
	if( undo )
	{
		undo_area * a = (undo_area*)ptr;
		CNetList * nl = a->nlist;
		CString temp = a->net_name;
		cnet * net = nl->GetNetPtrByName( &temp );
		if( net )
		{
			if( type == UNDO_AREA_CLEAR_ALL )
			{
				// delete all areas in this net
				for( int ia=net->area.GetSize()-1; ia>=0; ia-- )
					nl->RemoveArea( net, ia );
			}
			else if( type == UNDO_AREA_ADD )
			{
				// delete selected area
				nl->RemoveArea( net, a->iarea );
			}
			else if( type == UNDO_AREA_MODIFY 
					|| type == UNDO_AREA_DELETE )
			{
				undo_corner * c = (undo_corner*)((UINT)ptr+sizeof(undo_area));
				if( type == UNDO_AREA_MODIFY )
				{
					// remove area
					nl->RemoveArea( net, a->iarea );
				}
				// now recreate area at its original iarea in net->area[iarea]
				nl->InsertArea( net, a->iarea, a->layer, a->w, c[0].x, c[0].y, a->hatch );
				for( int ic=1; ic<a->ncorners; ic++ )
				{
					nl->AppendAreaCorner( net, a->iarea, 
						c[ic].x, c[ic].y, c[ic-1].style, FALSE ); 
					if( ic == (a->ncorners-1) )
					{
						nl->CompleteArea( net, a->iarea, c[ic].style );
						net->area[a->iarea].poly->SetMerge( a->merge_name );
					}
					else if( c[ic].num_contour < c[ic+1].num_contour )
						net->area[a->iarea].poly->Close( c[ic].style, FALSE, FALSE );
				}
			}
			else
				ASSERT(0);
		}
	}
	free( ptr );
}

// cross-check netlist with partlist, report results in logstr
//
int CNetList::CheckNetlist( CString * logstr )
{
	CString str;
	int nwarnings = 0;
	int nerrors = 0;
	int nfixed = 0;
	CMapStringToPtr net_map;
	CMapStringToPtr pin_map;

	*logstr += "***** Checking Nets *****\r\n";

	// traverse map
	POSITION pos;
	CString name;
	void * ptr;
	for( pos = m_map.GetStartPosition(); pos != NULL; )
	{
		// next net
		m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		CString net_name = net->name;
		if( net_map.Lookup( net_name, ptr ) )
		{
			str.Format( "ERROR: Net \"%s\" is duplicate\r\n", net_name );
			str += "    ###   To fix this, delete one instance of the net, then save and re-open project\r\n";
			*logstr += str;
			nerrors++;
		}
		else
			net_map.SetAt( net_name, NULL );
		int npins = net->pin.GetSize();
		if( npins == 0 )
		{
			str.Format( "Warning: Net \"%s\": has no pins\r\n", net->name );
			*logstr += str;
			nwarnings++;
		}
		else if( npins == 1 )
		{
			str.Format( "Warning: Net \"%s\": has single pin\r\n", net->name );
			*logstr += str;
			nwarnings++;
		}
		//
		for( int ip=0; ip<net->pin.GetSize(); ip++ )
		{
			// next pin in net
			CString * ref_des = &net->pin[ip].ref_des;
			CString * pin_name = &net->pin[ip].pin_name;
			/*CString pin_id = *ref_des + "." + *pin_name;
			void * ptr;
			BOOL test = pin_map.Lookup( pin_id, ptr );
			cnet * dup_net = (cnet*)ptr;
			// test in current net
			for( int ip2=ip+1; ip2<net->pin.GetSize(); ip2++ )
			{
				if( net->pin[ip2].ref_des.Compare(*ref_des) == 0 &&
					net->pin[ip2].pin_name.Compare(*pin_name) == 0 )
				{
					dup_net = net;
					test |= 1;
				}
			}	
			if( test )
			{
				if( dup_net->name.Compare(net_name) == 0 )
				{
					str.Format( "ERROR: Net \"%s\": pin \"%s\" is duplicate, corrected.\r\n", 
						net->name, pin_id );
					*logstr += str;
					// reassign all connections
					// find index of first instance of pin
					int first_index = -1;
					for( int iip=0; iip<net->pin.GetSize(); iip++ )
					{
						if( net->pin[iip].ref_des == *ref_des && net->pin[iip].pin_name == *pin_name )
						{
							first_index = iip;
							break;
						}
					}
					if( first_index == -1 )
						ASSERT(0);
					// reassign connections
					for( int ic=0; ic<net->connect.GetSize(); ic++ )
					{
						cconnect * c = &net->connect[ic];
						if( c->start_pin == ip )
							c->start_pin = first_index;
						if( c->end_pin == ip )
							c->end_pin = first_index;
					}
					// remove pin
					RemoveNetPin( net, ip );
					RehookPartsToNet( net );
					str.Format( "              Fixed: Connections repaired\r\n" );
					*logstr += str;
					nerrors++;
					nfixed++;
					continue;		// no further testing on this pin
				}
				else
				{
					str.Format( "ERROR: Net \"%s\": pin \"%s\" already assigned to net \"%s\"\r\n", 
						net->name, pin_id, dup_net->name );
					str += "    ###   To fix this, delete pin from one of these nets, then save and re-open project\r\n";
					nerrors++;
					*logstr += str;
				}
			}
			else
				pin_map.SetAt( pin_id, net );
*/
			cpart * part = net->pin[ip].part;
			if( !part )
			{
				// net->pin->part == NULL, find out why
				// see if part exists in partlist
				cpart * test_part = m_plist->GetPart( *ref_des );
				if( !test_part )
				{
					// no
					str.Format( "Warning: Net \"%s\": pin \"%s.%s\" not connected, part doesn't exist\r\n", 
						net->name, *ref_des, *pin_name, net->name );
					*logstr += str;
					nwarnings++;
				}
				else
				{
					// yes, see if it has footprint
					if( !test_part->shape )
					{
						// no
						str.Format( "Warning: Net \"%s\": pin \"%s.%s\" connected, part doesn't have footprint\r\n", 
							net->name, *ref_des, *pin_name, net->name );
						*logstr += str;
						nwarnings++;
					}
					else
					{
						// yes, see if pin exists
						int pin_index = test_part->shape->GetPinIndexByName( *pin_name, -1 );
						if( pin_index == -1 )
						{
							// no
							str.Format( "ERROR: Net \"%s\": pin \"%s.%s\" not connected, but part exists although pin doesn't\r\n", 
								net->name, *ref_des, *pin_name, net->name );
							str += "    ###   To fix this, fix any other errors then save and re-open project\r\n";
							*logstr += str;
							nerrors++;
						}
						else
						{
							// yes
							str.Format( "ERROR: Net \"%s\": pin \"%s.%s\" not connected, but part and pin exist\r\n", 
								net->name, *ref_des, *pin_name, net->name );
							str += "    ###   To fix this, fix any other errors then save and re-open project\r\n";
							*logstr += str;
							nerrors++;
						}
					}
				}
			}
			else
			{
				// net->pin->part exists, check parameters
				if( part->ref_des.Compare(*ref_des) )
				{
					// net->pin->ref_des != net->pin->part->ref_des
					str.Format( "ERROR: Net \"%s\": pin \"%s.%s\" connected to wrong part %s\r\n",
						net->name, *ref_des, *pin_name, part->ref_des );
					str += "    ###   To fix this, fix any other errors then save and re-open project\r\n";
					*logstr += str;
					nerrors++;
				}
				else
				{
					cpart * partlist_part = m_plist->GetPart( *ref_des );
					if( !partlist_part )
					{
						// net->pin->ref_des not found in partlist
						str.Format( "ERROR: Net \"%s\": pin \"%s.%s\" connected but part not in partlist\r\n",
							net->name, *ref_des, *pin_name );
						*logstr += str;
						nerrors++;
					}
					else
					{
						if( part != partlist_part )
						{
							// net->pin->ref_des found in partlist, but doesn't match net->pin->part
							str.Format( "ERROR: Net \"%s\": pin \"%s.%s\" connected but net->pin->part doesn't match partlist\r\n",
								net->name, *ref_des, *pin_name );
							str += "    ###   To fix this, fix any other errors then save and re-open project\r\n";
							*logstr += str;
							nerrors++;
						}
						else
						{
							if( !part->shape )
							{
								// part matches, but no footprint
								str.Format( "Warning: Net \"%s\": pin \"%s.%s\" connected but part doesn't have footprint\r\n",
									net->name, *ref_des, *pin_name );
								*logstr += str;
								nwarnings++;
							}
							else
							{
								int pin_index = part->shape->GetPinIndexByName( *pin_name, -1 );
								if( pin_index == -1 )
								{
									// net->pin->pin_name doesn't exist in part
									str.Format( "Warning: Net \"%s\": pin \"%s.%s\" connected but part doesn't have pin\r\n",
										net->name, *ref_des, *pin_name );
									*logstr += str;
									nwarnings++;
								}
								else
								{
									cnet * part_pin_net = part->pin[pin_index].net;
									if( part_pin_net != net )
									{
										// part->pin->net != net 
										str.Format( "ERROR: Net \"%s\": pin \"%s.%s\" connected but part->pin->net doesn't match\r\n",
											net->name, *ref_des, *pin_name );
										str += "    ###   To fix this, fix any other errors then save and re-open project\r\n";
										*logstr += str;
										nerrors++;
									}
									else
									{
										// OK, all is well, peace on earth
									}
								}
							}
						}
					}
				}
			}
		}
		// now check connections
		for( int ic=0; ic<net->connect.GetSize(); ic++ )
		{
			cconnect * c = &net->connect[ic];
			if( c->nsegs == 0 )
			{
				str.Format( "ERROR: Net \"%s\": connection with no segments\r\n",
					net->name );
				*logstr += str;
				RemoveNetConnect( net, ic, FALSE );
				str.Format( "              Fixed: Connection removed\r\n",
					net->name );
				*logstr += str;
				nerrors++;
				nfixed++;
			}
			else if( c->start_pin == c->end_pin && c->vtx[0].x == c->vtx[c->nsegs].x && c->vtx[0].y == c->vtx[c->nsegs].y )
			{
				str.Format( "ERROR: Net \"%s\": connection from pin to itself\r\n",
					net->name );
				*logstr += str;
				RemoveNetConnect( net, ic, FALSE );
				str.Format( "              Fixed: Connection removed\r\n",
					net->name );
				*logstr += str;
				nerrors++;
				nfixed++;
			}
		}
	}
	str.Format( "***** %d ERROR(S), %d FIXED, %d WARNING(S) *****\r\n",
		nerrors, nfixed, nwarnings );
	*logstr += str;
	return nerrors;
}

// cross-check netlist with partlist, report results in logstr
//
int CNetList::CheckConnectivity( CString * logstr )
{
	CString str;
	int nwarnings = 0;
	int nerrors = 0;
	int nfixed = 0;
	CMapStringToPtr net_map;
	CMapStringToPtr pin_map;

	// traverse map
	POSITION pos;
	CString name;
	void * ptr;
	for( pos = m_map.GetStartPosition(); pos != NULL; )
	{
		// next net
		m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		CString net_name = net->name;
		// now check connections
		for( int ic=0; ic<net->connect.GetSize(); ic++ )
		{
			cconnect * c = &net->connect[ic];
			if( c->nsegs == 0 )
			{
				str.Format( "ERROR: Net \"%s\": connection with no segments\r\n",
					net->name );
				*logstr += str;
				RemoveNetConnect( net, ic, FALSE );
				str.Format( "              Fixed: Connection removed\r\n",
					net->name );
				*logstr += str;
				nerrors++;
				nfixed++;
			} 
			else if( c->start_pin == c->end_pin && c->vtx[0].x == c->vtx[c->nsegs].x && c->vtx[0].y == c->vtx[c->nsegs].y )
			{
				str.Format( "ERROR: Net \"%s\": connection from pin to itself\r\n",
					net->name );
				*logstr += str;
				RemoveNetConnect( net, ic, FALSE );
				str.Format( "              Fixed: Connection removed\r\n",
					net->name );
				*logstr += str;
				nerrors++;
				nfixed++;
			}
			else
			{
				// check for unrouted or partially routed connection
				for( int is=0; is<c->nsegs; is++ )
				{
					if( c->seg[is].layer == LAY_RAT_LINE )
					{
						CString start_pin, end_pin;
						int istart = c->start_pin;
						start_pin = net->pin[istart].ref_des + "." + net->pin[istart].pin_name;
						int iend = c->end_pin;
						if( iend == cconnect::NO_END )
						{
							str.Format( "Net \"%s\": partially routed stub trace from %s\r\n",
								net->name, start_pin );
							*logstr += str;
							nerrors++;
						}
						else
						{
							end_pin = net->pin[iend].ref_des + "." + net->pin[iend].pin_name;
							if( c->nsegs == 1 )
							{
								str.Format( "Net \"%s\": unrouted connection from %s to %s\r\n",
									net->name, start_pin, end_pin );
								*logstr += str;
								nerrors++;
							}
							else
							{
								str.Format( "Net \"%s\": partially routed connection from %s to %s\r\n",
									net->name, start_pin, end_pin );
								*logstr += str;
								nerrors++;
							}
						}
						break;
					}
				}
			}
		}
	}
	return nerrors;
}

// Test an area for self-intersection.
// Returns:
//	-1 if arcs intersect other sides
//	 0 if no intersecting sides
//	 1 if intersecting sides, but no intersecting arcs
// Also sets utility2 flag of area with return value
//
int CNetList::TestAreaPolygon( cnet * net, int iarea )
{	
	CPolyLine * p = net->area[iarea].poly;
	// first, check for sides intersecting other sides, especially arcs 
	BOOL bInt = FALSE;
	BOOL bArcInt = FALSE;
	int n_cont = p->GetNumContours();
	// make bounding rect for each contour
	CArray<RECT> cr;
	cr.SetSize( n_cont );
	for( int icont=0; icont<n_cont; icont++ )
		cr[icont] = p->GetCornerBounds( icont );
	for( int icont=0; icont<(n_cont-1); icont++ )
	{
		int is_start = p->GetContourStart(icont);
		int is_end = p->GetContourEnd(icont);
		for( int is=is_start; is<=is_end; is++ )
		{
			int is_prev = is - 1;
			if( is_prev < is_start )
				is_prev = is_end;
			int is_next = is + 1;
			if( is_next > is_end )
				is_next = is_start;
			int style = p->GetSideStyle( is );
			int x1i = p->GetX( is );
			int y1i = p->GetY( is );
			int x1f = p->GetX( is_next );
			int y1f = p->GetY( is_next );
			// check for intersection with any other sides
			for( int icont2=(icont+1); icont2<n_cont; icont2++ )
			{
				if( cr[icont].left > cr[icont2].right
					|| cr[icont].bottom > cr[icont2].top
					|| cr[icont2].left > cr[icont].right
					|| cr[icont2].bottom > cr[icont].top )
				{
					// rectangles don't overlap, do nothing
				}
				else
				{
					int is2_start = p->GetContourStart(icont2);
					int is2_end = p->GetContourEnd(icont2);
					for( int is2=is2_start; is2<=is2_end; is2++ )
					{
						int is2_prev = is2 - 1;
						if( is2_prev < is2_start )
							is2_prev = is2_end;
						int is2_next = is2 + 1;
						if( is2_next > is2_end )
							is2_next = is2_start;
						if( icont != icont2 || (is2 != is && is2 != is_prev && is2 != is_next && is != is2_prev && is != is2_next ) )
						{
							int style2 = p->GetSideStyle( is2 );
							int x2i = p->GetX( is2 );
							int y2i = p->GetY( is2 );
							int x2f = p->GetX( is2_next );
							int y2f = p->GetY( is2_next );
							if ( min(x1i,x1f) > max(x2i,x2f) ) continue;
							if ( min(x2i,x2f) > max(x1i,x1f) ) continue;
							if ( min(y1i,y1f) > max(y2i,y2f) ) continue;
							if ( min(y2i,y2f) > max(y1i,y1f) ) continue;
							int ret = FindSegmentIntersections( x1i, y1i, x1f, y1f, style, x2i, y2i, x2f, y2f, style2 );
							if( ret )
							{
								// intersection between non-adjacent sides
								bInt = TRUE;
								if( style != CPolyLine::STRAIGHT || style2 != CPolyLine::STRAIGHT )
								{
									bArcInt = TRUE;
									break;
								}
							}
						}
					}
				}
				if( bArcInt )
					break;
			}
			if( bArcInt )
				break;
		}
		if( bArcInt )
			break;
	}
	if( bArcInt )
		net->area[iarea].utility2 = -1;
	else if( bInt )
		net->area[iarea].utility2 = 1;
	else 
		net->area[iarea].utility2 = 0;
	return net->area[iarea].utility2;
}

int CNetList::TestAreaPolygon( cnet * net, int iarea, int test_contour, int test_corner )
{	
	CPolyLine * p = net->area[iarea].poly;
	int n_cor = p->GetNumCorners();
	int max_cont = p->GetNumContours();
	if ( test_contour >= max_cont ) // error
	{
		net->area[iarea].utility2 = 0;
		return net->area[iarea].utility2;
	}
	if( test_corner >= 0 || test_contour >= 0 )
		max_cont = min( max_cont,(test_contour+2) );
	BOOL bOut = FALSE;
	BOOL bArcOut = FALSE;
	if( max_cont >= 2 )
	{
		// Set Utility
		for (int i=0; i<p->GetNumContours(); i++)
			p->SetUtility(p->GetContourStart(i),0);
		//
		// check for intersection with any other sides
		int nc = max(0,test_contour);
		int nc2;
		time_t gnow = time(0);
		for( int i=p->GetContourStart(nc); nc<(max_cont-1); i++ )
		{
			time_t now = time(0);
			if( now-10 > gnow )
			{
				AfxMessageBox("This area of copper has many contours when "\
					"checking for the integrity of the fill, so the testing "\
					"of the integrity of this copper area will be stopped. "\
					"You can create a fill for this area of copper, "\
					"then this check will not be needed.", MB_ICONWARNING);
				return INT_MAX;
			}
			RECT r = p->GetCornerBounds(nc);
			if ( test_contour >= 0 )
				nc2 = 0;
			else
				nc2 = nc + 1;
			if( test_corner >= 0 )
				i = p->GetIndexCornerBack(test_corner);
			for( int i2=p->GetContourStart(nc2); nc2<p->GetNumContours(); i2++ )
			{
				if( nc != nc2 )
				{
					RECT r2 = p->GetCornerBounds(nc2);	
					if ( RectsIntersection(r,r2) != -1 )
					{
						BOOL bInt = FALSE;
						int iend;
						if( test_corner >= 0)
							iend = test_corner;
						else
							iend =  p->GetContourEnd(nc);					
						int iend2 = p->GetContourEnd(nc2);
						for (int ii=i; ii<=iend; ii++)
						{
							int style = p->GetSideStyle(ii);
							int x1i = p->GetX(ii);
							int y1i = p->GetY(ii);
							int x1f = p->GetX(p->GetIndexCornerNext(ii));
							int y1f = p->GetY(p->GetIndexCornerNext(ii));
							for (int ii2=i2; ii2<=iend2; ii2++)
							{
								int style2 = p->GetSideStyle(ii2);
								int x2i = p->GetX(ii2);
								int y2i = p->GetY(ii2);
								int x2f = p->GetX(p->GetIndexCornerNext(ii2));
								int y2f = p->GetY(p->GetIndexCornerNext(ii2));
								if ( min(x1i,x1f) > max(x2i,x2f) ) continue;
								if ( min(x2i,x2f) > max(x1i,x1f) ) continue;
								if ( min(y1i,y1f) > max(y2i,y2f) ) continue;
								if ( min(y2i,y2f) > max(y1i,y1f) ) continue;
								int ret = FindSegmentIntersections( x1i, y1i, x1f, y1f, style, x2i, y2i, x2f, y2f, style2 );
								if( ret )
								{
									// intersection between non-adjacent sides
									bOut = TRUE;
									bInt = TRUE;
									p->SetUtility(i,1);
									p->SetUtility(i2,1);
									if( style != CPolyLine::STRAIGHT || style2 != CPolyLine::STRAIGHT )
										bArcOut = TRUE;
									break;
								}
							}
							if ( bInt )
								break;
						}
					}
				}
				i2 = p->GetContourEnd(nc2);
				nc2++;		
			}
			i = p->GetContourEnd(nc);
			nc++;
		}
	}
	if( bArcOut )
		net->area[iarea].utility2 = -1;
	else if( bOut )
		net->area[iarea].utility2 = 1;
	else 
		net->area[iarea].utility2 = 0;
	return net->area[iarea].utility2;
}

// Process an area that has been modified, by clipping its polygon against itself.
// This may change the number and order of copper areas in the net.
// If bMessageBoxInt == TRUE, shows message when clipping occurs.
// If bMessageBoxArc == TRUE, shows message when clipping can't be done due to arcs.
// Returns:
//	-1 if arcs intersect other sides, so polygon can't be clipped
//	 0 if no intersecting sides
//	 1 if intersecting sides
// Also sets net->area->utility1 flags if areas are modified
//
int CNetList::ClipAreaPolygon( cnet * net, int iarea, int corner, 
							  BOOL bMessageBoxArc, BOOL bMessageBoxInt, BOOL bRetainArcs, int*nContours )
{	
	CPolyLine * p = net->area[iarea].poly;
	int a_merge = p->GetMerge();
	int num_cor = p->GetNumCorners();
	if ( corner >= num_cor )
		ASSERT(0);
	int test;
	if( corner >= 0 && corner < num_cor )
		test = TestAreaPolygon( net, iarea, p->GetNumContour(corner), corner );	// this sets utility2 flag
	else
		test = TestAreaPolygon( net, iarea, -1, -1 );	// this sets utility2 flag
	if( test == -1 && bRetainArcs )
		test = 1;
	if( test == -1 )
	{
		// arc intersections, don't clip unless bRetainArcs == FALSE
		if( bMessageBoxArc && bDontShowSelfIntersectionArcsWarning == FALSE )
		{
			CString str;
			str.Format( "Area %d of net \"%s\" has arcs intersecting other sides.\n",
				iarea+1, net->name );
			str += "This may cause problems with other editing operations,\n";
			str += "such as adding cutouts. It can't be fixed automatically.\n";
			str += "Manual correction is recommended.\n";
			CDlgMyMessageBox dlg;
			dlg.Initialize( str );
			dlg.DoModal();
			bDontShowSelfIntersectionArcsWarning = dlg.bDontShowBoxState;
		}
		return -1;	// arcs intersect with other sides, error
	}

	// mark all areas as unmodified except this one
	for( int ia=0; ia<net->nareas; ia++ )
		net->area[ia].utility = 0;
	net->area[iarea].utility = 1;

	if( test == 1 )
	{
		// non-arc intersections, clip the polygon
		if( bMessageBoxInt && bDontShowSelfIntersectionWarning == FALSE)
		{
			CString str;
			str.Format( "Area %d of net \"%s\" is self-intersecting and will be clipped.\n",
				iarea+1, net->name );
			str += "This may result in splitting the area.\n";
			str += "If the area is complex, this may take a few seconds.";
			CDlgMyMessageBox dlg;
			dlg.Initialize( str );
			dlg.DoModal();
			bDontShowSelfIntersectionWarning = dlg.bDontShowBoxState;
		}
	}
//** TODO test for cutouts outside of area	
if( test == 1 )
	{
		p->Undraw();
		CPolyLine * ok_cont = new CPolyLine;//ok
		CPolyLine * del_cont = new CPolyLine;//ok
		ok_cont->SetDisplayList( net->m_dlist );
		id area_id = p->GetId();
		BOOL start1=TRUE, start2=TRUE;
		int nc = p->GetNumContours();
		int hatch = p->GetHatch();
		int layer = p->GetLayer(); 
		int w = p->GetW();
		for (int ico=0; ico<nc; ico++)
		{
			int ist = p->GetContourStart(ico);
			int ien = p->GetContourEnd(ico);
			if ((ien-ist) < 2 )
				ASSERT(0);
			if ( p->GetUtility(ist) == 1 || ico == 0 )
			{	
				for(int iadd=ist; iadd<=ien; iadd++)
				{
					if ( start1 )
					{
						ok_cont->Start( layer, w, 5*NM_PER_MIL, p->GetX(iadd), p->GetY(iadd), 
															hatch, &area_id, net, num_cor );
						start1 = 0;
					}
					else
						ok_cont->AppendCorner(p->GetX(iadd),p->GetY(iadd),p->GetSideStyle(iadd-1),FALSE);
				}
				ok_cont->Close(p->GetSideStyle(ien),FALSE);
			}
			else
			{
				for(int iadd=ist; iadd<=ien; iadd++)
				{
					if ( start2 )
					{
						del_cont->Start( layer, w, 5*NM_PER_MIL, p->GetX(iadd), p->GetY(iadd), 
															hatch, &area_id, net, num_cor );
						start2 = 0;
					}
					else
						del_cont->AppendCorner(p->GetX(iadd),p->GetY(iadd),p->GetSideStyle(iadd-1),FALSE);
				}
				del_cont->Close(p->GetSideStyle(ien),FALSE);
			}
		}
		CArray<CPolyLine*> * pa = new CArray<CPolyLine*>;
		int n_poly = ok_cont->NormalizeWithGpc( pa, bRetainArcs );
		if( nContours )
			*nContours = n_poly;
		if( n_poly > 1 )
		{
			for( int ip=0; ip<pa->GetSize(); ip++ )
			{
				// create new copper area and copy poly into it
				CPolyLine * new_p = pa->GetAt(ip);
				int ia = AddArea( net, 0, 0, 0, 0 );
				// remove the poly that was automatically created for the new area
				// and replace it with a poly from NormalizeWithGpc
				net->area[ia].poly->FreeGpcPoly();
				net->area[ia].poly->Undraw();
				delete net->area[ia].poly;
				net->area[ia].poly = new_p;
				int numc = del_cont->GetNumContours();
				for( int con=0; con<numc; con++ )
				{
					int st = del_cont->GetContourStart(con);
					int end = del_cont->GetContourEnd(con);
					if ( new_p->TestPointInside( del_cont->GetX(st),del_cont->GetY(st) ))
					{
						for (int ic=st; ic<=end; ic++)
							net->area[ia].poly->AppendCorner(del_cont->GetX(ic),del_cont->GetY(ic),del_cont->GetSideStyle(del_cont->GetIndexCornerBack(ic)),FALSE);
						net->area[ia].poly->Close(del_cont->GetSideStyle(end),FALSE);
					}
				}
				net->area[ia].poly->SetDisplayList( net->m_dlist );
				net->area[ia].poly->SetHatch( hatch );
				net->area[ia].poly->SetLayer( layer );
				net->area[ia].poly->SetMerge( a_merge );
				id p_id( ID_NET, ID_AREA, ia );
				net->area[ia].poly->SetId( &p_id );
				for( int c=(net->area[ia].poly->GetNumContours()-1); c>=0; c-- )
					net->area[ia].poly->RecalcRectC(c);
				net->area[ia].poly->FreeGpcPoly();
				net->area[ia].poly->Draw();
				net->area[ia].utility = 1;
			}
		}
		delete pa;
		delete net->area[iarea].poly;
		net->area[iarea].poly = ok_cont;
		int numc = del_cont->GetNumContours();
		for( int con=0; con<numc; con++ )
		{
			int st = del_cont->GetContourStart(con);
			int end = del_cont->GetContourEnd(con);
			if ( ok_cont->TestPointInside( del_cont->GetX(st),del_cont->GetY(st) ))
			{
				for (int ic=st; ic<=end; ic++)
					net->area[iarea].poly->AppendCorner(del_cont->GetX(ic),del_cont->GetY(ic),del_cont->GetSideStyle(del_cont->GetIndexCornerBack(ic)),FALSE);
				net->area[iarea].poly->Close(del_cont->GetSideStyle(end),FALSE);
			}
		}
		delete del_cont;
		for( int c=(net->area[iarea].poly->GetNumContours()-1); c>=0; c-- )
			net->area[iarea].poly->RecalcRectC(c);
		net->area[iarea].poly->Draw();
		net->area[iarea].utility = 1;
	}
	return test;
}

// Process an area that has been modified, by clipping its polygon against
// itself and the polygons for any other areas on the same net.
// This may change the number and order of copper areas in the net.
// If bMessageBox == TRUE, shows message boxes when clipping occurs.
// Returns:
//	-1 if arcs intersect other sides, so polygon can't be clipped
//	 0 if no intersecting sides
//	 1 if intersecting sides, polygon clipped
//
int CNetList::AreaPolygonModified( cnet * net, int iarea, BOOL bMessageBoxArc, BOOL bMessageBoxInt, int corner )
{	
	// clip polygon against itself
	int test = ClipAreaPolygon( net, iarea, corner, bMessageBoxArc, bMessageBoxInt );
	if( test == -1 )
		return test;
	// now see if we need to clip against other areas
	BOOL bCheckAllAreas = FALSE;
	if( test == 1 )
		bCheckAllAreas = TRUE;
	else
	{
		if ( corner >= 0 )
			bCheckAllAreas = TestAreaIntersections( net, iarea, corner );
		else
			bCheckAllAreas = TestAreaIntersections( net, iarea );
	}
	if( bCheckAllAreas )
		CombineAllAreasInNet( net, bMessageBoxInt, TRUE );
	return bCheckAllAreas;
}

// Checks all copper areas in net for intersections, combining them if found
// If bUseUtility == TRUE, don't check areas if both utility flags are 0
// Sets utility flag = 1 for any areas modified
// If an area has self-intersecting arcs, doesn't try to combine it
//
int CNetList::CombineAllAreasInNet( cnet * net, BOOL bMessageBox, BOOL bUseUtility )
{
	if( net->nareas > 1 )
	{
		// start by testing all area polygons to set utility2 flags
		for( int ia=0; ia<net->nareas; ia++ )
			TestAreaPolygon( net, ia, -1, -1 );
		// now loop through all combinations
		BOOL message_shown = FALSE;
		for( int ia1=0; ia1<net->nareas-1; ia1++ ) 
		{
			// legal polygon
			RECT b1 = net->area[ia1].poly->GetCornerBounds(0);
			BOOL mod_ia1 = FALSE;
			for( int ia2=net->nareas-1; ia2 > ia1; ia2-- )
			{
				if( net->area[ia1].poly->GetLayer() == net->area[ia2].poly->GetLayer() &&
					net->area[ia1].poly->GetHatch() == net->area[ia2].poly->GetHatch() &&
					net->area[ia1].poly->GetW() == net->area[ia2].poly->GetW() &&
					net->area[ia1].utility2 != -1 && 
					net->area[ia2].utility2 != -1 )
				{
					RECT b2 = net->area[ia2].poly->GetCornerBounds(0);
					if( !( b1.left > b2.right || b1.right < b2.left
						|| b1.bottom > b2.top || b1.top < b2.bottom ) )
					{
						// check ia2 against 1a1 
						if( net->area[ia1].utility || net->area[ia2].utility || bUseUtility == FALSE )
						{
							int ret = TestAreaIntersection( net, ia1, ia2 );
							if( ret == 1 || ret == 2 )
							{
								CString str;
								str.Format( "Areas %d and %d of net \"%s\" intersect and may be combined.\n",
									ia1+1, ia2+1, net->name );
								str += "If they are complex, this may take a few seconds.\n";
								str += "Combine these copper areas?";
								int qw = AfxMessageBox( str, MB_YESNO );
								if( qw != IDYES)
									ret = 0;
							}
							if( ret == 1 || ret == 2 )
							{
								ret = CombineAreas( net, ia1, ia2 );
								mod_ia1 = TRUE;
							}
							else
							{
								/*if( bMessageBox && bDontShowIntersectionArcsWarning == FALSE )
								{
									CString str;
									str.Format( "Areas %d and %d of net \"%s\" intersect, but some of the intersecting sides are arcs.\n",
										ia1+1, ia2+1, net->name );
									str += "Therefore, these areas can't be combined.";
									CDlgMyMessageBox dlg;
									dlg.Initialize( str );
									dlg.DoModal();
									bDontShowIntersectionArcsWarning = dlg.bDontShowBoxState;
								}*/
							}
						}
					}
				}
			}
			if( mod_ia1 )
				ia1--;		// if modified, we need to check it again
		}
	}
	return 0;
}

// Check for intersection of copper area with other areas in same net
//
BOOL CNetList::TestAreaIntersections( cnet * net, int ia, int corner )
{
	CPolyLine * poly1 = net->area[ia].poly;
	RECT b1;
	int back_c, next_c;
	back_c = poly1->GetIndexCornerBack( corner );
	next_c = poly1->GetIndexCornerNext( corner );
	b1.left =	min(min(poly1->GetX(back_c), poly1->GetX(next_c) ), poly1->GetX(corner) );
	b1.right =	max(max(poly1->GetX(back_c), poly1->GetX(next_c) ), poly1->GetX(corner) );
	b1.bottom = min(min(poly1->GetY(back_c), poly1->GetY(next_c) ), poly1->GetY(corner) );
	b1.top =	max(max(poly1->GetY(back_c), poly1->GetY(next_c) ), poly1->GetY(corner) );
	for( int ia2=0; ia2<net->nareas; ia2++ )
	{
		if( ia != ia2 )
		{
			// see if polygons are on same layer
			CPolyLine * poly2 = net->area[ia2].poly;
			if( poly1->GetLayer() != poly2->GetLayer() )
				continue;

			// test bounding rects
			RECT b2 = poly2->GetCornerBounds(0);
			if (b1.bottom > b2.top) continue;
			if (b1.top < b2.bottom) continue;
			if (b1.left > b2.right) continue;
			if (b1.right < b2.left) continue;

			// test for intersecting segments
			BOOL bInt = FALSE;
			BOOL bArcInt = FALSE;
			int ist1 = poly1->GetContourStart( poly1->GetNumContour(corner) );
			int ien1 = poly1->GetContourEnd( poly1->GetNumContour(corner) );
			int ic1 = back_c;
			for( int step=0; step<2; step++ )
			{
				int xi1 = poly1->GetX(ic1);
				int yi1 = poly1->GetY(ic1);
				int xf1, yf1, style1;
				if( ic1 < ien1 )
				{
					xf1 = poly1->GetX(ic1+1);
					yf1 = poly1->GetY(ic1+1);
				}
				else
				{
					xf1 = poly1->GetX(ist1);
					yf1 = poly1->GetY(ist1);
				}
				style1 = poly1->GetSideStyle( ic1 );
				for( int icont2=0; icont2<poly2->GetNumContours(); icont2++ )
				{
					// test bounding rects
					b2 = poly2->GetCornerBounds(icont2);
					if (b1.bottom > b2.top) continue;
					if (b1.top < b2.bottom) continue;
					if (b1.left > b2.right) continue;
					if (b1.right < b2.left) continue;
					//
					int is2 = poly2->GetContourStart( icont2 );
					int ie2 = poly2->GetContourEnd( icont2 );
					for( int ic2=is2; ic2<=ie2; ic2++ )
					{
						int xi2 = poly2->GetX(ic2);
						int yi2 = poly2->GetY(ic2);
						int xf2, yf2, style2;
						if( ic2 < ie2 )
						{
							xf2 = poly2->GetX(ic2+1);
							yf2 = poly2->GetY(ic2+1);
						}
						else
						{
							xf2 = poly2->GetX(is2);
							yf2 = poly2->GetY(is2);
						}
						//
						if ( min(xi1,xf1) > max(xi2,xf2) ) continue;
						if ( min(xi2,xf2) > max(xi1,xf1) ) continue;
						if ( min(yi1,yf1) > max(yi2,yf2) ) continue;
						if ( min(yi2,yf2) > max(yi1,yf1) ) continue;
						style2 = poly2->GetSideStyle( ic2 );
						int n_int = FindSegmentIntersections( xi1, yi1, xf1, yf1, style1,
							xi2, yi2, xf2, yf2, style2 );
						if( n_int )
							return TRUE;
					}
				}
				ic1 = corner;
			}
		}
	}
	return FALSE;
}

BOOL CNetList::TestAreaIntersections( cnet * net, int ia )
{
	CPolyLine * poly1 = net->area[ia].poly;
	for( int ia2=0; ia2<net->nareas; ia2++ )
	{
		if( ia != ia2 )
		{
			// see if polygons are on same layer
			CPolyLine * poly2 = net->area[ia2].poly;
			if( poly1->GetLayer() != poly2->GetLayer() )
				continue;

			// test bounding rects
			RECT b1 = poly1->GetCornerBounds(0);
			RECT b2 = poly2->GetCornerBounds(0);
			if(    b1.bottom > b2.top
				|| b1.top < b2.bottom
				|| b1.left > b2.right
				|| b1.right < b2.left )
				continue;

			// test for intersecting segments
			BOOL bInt = FALSE;
			BOOL bArcInt = FALSE;
			for( int icont1=0; icont1<poly1->GetNumContours(); icont1++ )
			{
				int ist1 = poly1->GetContourStart( icont1 );
				int ien1 = poly1->GetContourEnd( icont1 );	
				for( int ic1=ist1; ic1<=ien1; ic1++ )
				{
					int xi1 = poly1->GetX(ic1);
					int yi1 = poly1->GetY(ic1);
					int xf1, yf1, style1;
					if( ic1 < ien1 )
					{
						xf1 = poly1->GetX(ic1+1);
						yf1 = poly1->GetY(ic1+1);
					}
					else
					{
						xf1 = poly1->GetX(ist1);
						yf1 = poly1->GetY(ist1);
					}
					style1 = poly1->GetSideStyle( ic1 );
					for( int icont2=0; icont2<poly2->GetNumContours(); icont2++ )
					{
						int is2 = poly2->GetContourStart( icont2 );
						int ie2 = poly2->GetContourEnd( icont2 );
						for( int ic2=is2; ic2<=ie2; ic2++ )
						{
							int xi2 = poly2->GetX(ic2);
							int yi2 = poly2->GetY(ic2);
							int xf2, yf2, style2;
							if( ic2 < ie2 )
							{
								xf2 = poly2->GetX(ic2+1);
								yf2 = poly2->GetY(ic2+1);
							}
							else
							{
								xf2 = poly2->GetX(is2);
								yf2 = poly2->GetY(is2);
							}
							style2 = poly2->GetSideStyle( ic2 );
							if ( min(xi1,xf1) > max(xi2,xf2) ) continue;
							if ( min(xi2,xf2) > max(xi1,xf1) ) continue;
							if ( min(yi1,yf1) > max(yi2,yf2) ) continue;
							if ( min(yi2,yf2) > max(yi1,yf1) ) continue;
							int n_int = FindSegmentIntersections( xi1, yi1, xf1, yf1, style1,
								xi2, yi2, xf2, yf2, style2 );
							if( n_int )
								return TRUE;
						}
					}
				}
			}
		}
	}
	return FALSE;
}

// Test for intersection of 2 copper areas
// ia2 must be > ia1
// returns: 0 if no intersection
//			1 if intersection
//			2 if arcs intersect
//
int CNetList::TestAreaIntersection( cnet * net, int ia1, int ia2 )
{
	return net->area[ia1].poly->TestPolygonIntersection( net->area[ia2].poly );
}





// If possible, combine 2 copper areas
// ia2 must be > ia1
// returns: 0 if no intersection
//			1 if intersection
//			2 if arcs intersect
//
int CNetList::CombineAreas( cnet * net, int ia1, int ia2 )
{
	if( ia2 <= ia1 )
		ASSERT(0);
#if 0
	// test for intersection
	int test = TestAreaIntersection( net, ia1, ia2 );
	if( test != 1 )
		return test;	// no intersection
#endif

	// polygons intersect, combine them
	CPolyLine * poly1 = net->area[ia1].poly;
	CPolyLine * poly2 = net->area[ia2].poly;
	int COUNT_CORNERS = poly1->GetNumCorners() + poly2->GetNumCorners();
	CArray<CArc> arc_array1;
	CArray<CArc> arc_array2;
	poly1->MakeGpcPoly( -1, &arc_array1 );
	poly2->MakeGpcPoly( -1, &arc_array2 );
	int n_ext_cont1 = 0;
	for( int ic=0; ic<poly1->GetGpcPoly()->num_contours; ic++ )
		if( !((poly1->GetGpcPoly()->hole)[ic]) )
			n_ext_cont1++;
	int n_ext_cont2 = 0;
	for( int ic=0; ic<poly2->GetGpcPoly()->num_contours; ic++ )
		if( !((poly2->GetGpcPoly()->hole)[ic]) )
			n_ext_cont2++;

	gpc_polygon * union_gpc = new gpc_polygon;//ok
	gpc_polygon_clip( GPC_UNION, poly1->GetGpcPoly(), poly2->GetGpcPoly(), union_gpc );

	// get number of outside contours
	int n_union_ext_cont = 0;
	for( int ic=0; ic<union_gpc->num_contours; ic++ )
		if( !((union_gpc->hole)[ic]) )
			n_union_ext_cont++;

	// if no intersection, free new gpc and return
	if( n_union_ext_cont == n_ext_cont1 + n_ext_cont2 )
	{
		gpc_free_polygon( union_gpc );
		delete union_gpc;
		return 0;
	}

	// intersection, replace ia1 with combined areas and remove ia2
	RemoveArea( net, ia2 );
	int hatch = net->area[ia1].poly->GetHatch();
	int m_a_merge = net->area[ia1].poly->GetMerge();
	id a_id = net->area[ia1].poly->GetId();
	int layer = net->area[ia1].poly->GetLayer();
	int w = net->area[ia1].poly->GetW();
	int sel_box = net->area[ia1].poly->GetSelBoxSize();
	RemoveArea( net, ia1 );
	// create area with external contour
	for( int ic=0; ic<union_gpc->num_contours; ic++ )
	{
		if( !(union_gpc->hole)[ic] )
		{
			// external contour, replace this poly
			for( int i=0; i<union_gpc->contour[ic].num_vertices; i++ )
			{
				int x = ((union_gpc->contour)[ic].vertex)[i].x;
				int y = ((union_gpc->contour)[ic].vertex)[i].y;
				if( i==0 )
				{
					InsertArea( net, ia1, layer, w, x, y, hatch, COUNT_CORNERS );
					net->area[ia1].poly->SetMerge( m_a_merge );
				}
				else
					AppendAreaCorner( net, ia1, x, y, CPolyLine::STRAIGHT, FALSE );
			}
			if (union_gpc->num_contours == 1)
				CompleteArea( net, ia1, CPolyLine::STRAIGHT );
			else
				net->area[ia1].poly->Close(CPolyLine::STRAIGHT, FALSE, FALSE);
		}
	}
	// add holes
	for( int ic=0; ic<union_gpc->num_contours; ic++ )
	{
		if( (union_gpc->hole)[ic] )
		{
			// hole
			for( int i=0; i<union_gpc->contour[ic].num_vertices; i++ )
			{
				int x = ((union_gpc->contour)[ic].vertex)[i].x;
				int y = ((union_gpc->contour)[ic].vertex)[i].y;
				AppendAreaCorner( net, ia1, x, y, CPolyLine::STRAIGHT, FALSE );
			}
			if (ic == (union_gpc->num_contours-1) )
				CompleteArea( net, ia1, CPolyLine::STRAIGHT );
			else
				net->area[ia1].poly->Close(CPolyLine::STRAIGHT, FALSE, FALSE);
		}
	}
	net->area[ia1].utility = 1;
	net->area[ia1].poly->RestoreArcs( &arc_array1 ); 
	net->area[ia1].poly->RestoreArcs( &arc_array2 );
	for( int c=(net->area[ia1].poly->GetNumContours()-1); c>=0; c-- )
		net->area[ia1].poly->RecalcRectC(c);
	net->area[ia1].poly->Draw();
	gpc_free_polygon( union_gpc );
	delete union_gpc;
	return 1;
}

void CNetList::SetWidths( int w, int via_w, int via_hole_w )
{
	m_def_w = w; 
	m_def_via_w = via_w; 
	m_def_via_hole_w = via_hole_w;
}

void CNetList::GetWidths( cnet * net, int * w, int * via_w, int * via_hole_w )
{
	if( net->def_w == 0 )
		*w = m_def_w;
	else
		*w = net->def_w;
	if( net->def_via_w == 0 )
		*via_w = m_def_via_w;
	else
		*via_w = net->def_via_w;

	if( net->def_via_hole_w == 0 )
		*via_hole_w = m_def_via_hole_w;
	else
		*via_hole_w = net->def_via_hole_w;

}


// retunn number of selected
int CNetList::GetSelCount(cnet * net)
{
	int cnt=0;
	for( int i=0; i<net->nconnects; i++ )
	{
		for( int ii=0; ii<net->connect[i].nsegs; ii++ )
		{
			if( net->connect[i].seg[ii].selected )
				cnt++;
			if( net->connect[i].vtx[ii].selected )
				cnt++;
		}
	}
	for( int i=0; i<net->nareas; i++ )
	{
		CPolyLine * p = net->area[i].poly;
		for( int ii=p->GetNumCorners()-1; ii>=0; ii-- )
		{
			if( p->GetSel(ii) )
				cnt++;
			if( p->GetSideSel(ii) )
				cnt++;
		}
	}
	return cnt;
}

// get bounding rectangle for all net elements
//
BOOL CNetList::GetNetBoundaries( RECT * r, BOOL bForSelected )
{
	BOOL bValid = FALSE;
	cnet * net = GetFirstNet();
	RECT br;
	br.bottom = INT_MAX;
	br.left = INT_MAX;
	br.top = INT_MIN;
	br.right = INT_MIN;
	while( net )
	{
		for( int ic=0; ic<net->nconnects; ic++ )
		{
			int size = net->connect[ic].nsegs;
			if( size <= 0 )
			{
				ASSERT(0);
			}
			for( int is=0; is<size; is++ )
			{
				cvertex * v = &net->connect[ic].vtx[is+1];
				cseg * s = &net->connect[ic].seg[is];
				if( v->selected || s->selected || bForSelected == 0 )
				{
					br.bottom = min( br.bottom, v->y - v->via_w/2 );
					br.top = max( br.top, v->y + v->via_w/2 );
					br.left = min( br.left, v->x - v->via_w/2 );
					br.right = max( br.right, v->x + v->via_w/2 );
					bValid = TRUE;
					if( is == 0 )
					{
						v = &net->connect[ic].vtx[0];
						br.bottom = min( br.bottom, v->y - v->via_w/2 );
						br.top = max( br.top, v->y + v->via_w/2 );
						br.left = min( br.left, v->x - v->via_w/2 );
						br.right = max( br.right, v->x + v->via_w/2 );
					}
				}
			}
		}
		for( int ia=0; ia<net->nareas; ia++ )
		{
			CPolyLine * p = net->area[ia].poly;
			int w = p->GetW();
			for( int cor=0; cor<net->area[ia].poly->GetNumCorners(); cor++ )
			{
				int ax = p->GetX(cor);
				int ay = p->GetY(cor);
				if( bForSelected == 0 )
				{
					br.bottom = min( br.bottom, ay-w );
					br.top = max( br.top, ay+w );
					br.left = min( br.left, ax-w );
					br.right = max( br.right, ax+w );
					bValid = TRUE;
				}
				else if( p->GetSel(cor) || p->GetSideSel(cor) || p->GetSideSel(p->GetIndexCornerBack(cor)) )
				{
					br.bottom = min( br.bottom, ay-w );
					br.top = max( br.top, ay+w );
					br.left = min( br.left, ax-w );
					br.right = max( br.right, ax+w );
					bValid = TRUE;
				}
			}
		}
		net = GetNextNet(/*LABEL*/);
	}
	*r = br;
	return bValid;
}

// Remove all tee IDs from list
//
void CNetList::ClearTeeIDs()
{
	m_tee.RemoveAll();
}

// Find an ID and return array position or -1 if not found
//
int CNetList::FindTeeID( int id )
{
	for( int i=0; i<m_tee.GetSize(); i++ )
		if( m_tee[i] == id )
			return i;
	return -1;
}

// Assign a new ID and add to list
//
int CNetList::GetNewTeeID()
{
	int id;
	srand( (unsigned)time( NULL ) );
	do
	{
		id = rand();
	}while( id != 0 && FindTeeID(id) != -1 );
	m_tee.Add( id );
	return id;
}

// Remove an ID from the list
//
void CNetList::RemoveTeeID( int id )
{
	int i = FindTeeID( id );
	if( i >= 0 )
		m_tee.RemoveAt(i);
}

// Add tee_ID to list
//
void CNetList::AddTeeID( int id )
{
	if( id == 0 )
		return;
	if( FindTeeID( id ) == -1 )
		m_tee.Add( id );
}




//  Find the main tee vertex for a tee_ID 
//	return FALSE if not found
//	return TRUE if found, set ic and iv
//
BOOL CNetList::FindTeeVertexInNet( cnet * net, int id, int * ic, int * iv )
{	
	if (!ic || !iv)
		ASSERT(0);
	if ( *ic == 0 && *iv == 0 )
	{// find header	
		for( int icc=0; icc<net->nconnects; icc++ )
		{
			cconnect * c = &net->connect[icc];
			for( int ivv=1; ivv<c->nsegs; ivv++ )
			{
				if( c->vtx[ivv].tee_ID == id )
				{
					if( ic )
						*ic = icc;
					if( iv )
						*iv = ivv;
					return TRUE;
				}
			}
		}
	}
	else
	{// find branches
		for( int icc=*ic+1; icc<net->nconnects; icc++ )
		{
			cconnect * c = &net->connect[icc];
			if( c->vtx[c->nsegs].tee_ID == id )
			{
				if( ic )
					*ic = icc;
				if( iv )
					*iv = c->nsegs;
				return TRUE;
			}
		}
	}
	return FALSE;
}


// find tee vertex in any net
//
BOOL CNetList::FindTeeVertex( int id, cnet ** net, int * ic, int * iv )
{
	cnet * tnet = GetFirstNet();
	while( tnet )
	{
		*ic=0;
		*iv=0;
		BOOL bFound = FindTeeVertexInNet( tnet, id, ic, iv );
		if( bFound )
		{
			CancelNextNet();
			*net = tnet;
			return TRUE;
		}
		tnet = GetNextNet();
	}
	return FALSE;
}




// Disconnect branch from tee, remove tee if no more branches
// Returns TRUE if tee still exists, FALSE if destroyed
//
BOOL CNetList::DisconnectBranch( cnet * net, int ic )
{
	cconnect * c = &net->connect[ic];
	int id = c->vtx[c->nsegs].tee_ID;
	if( !id )
		return FALSE;
	else
	{
		c->vtx[c->nsegs].tee_ID = 0;
		ReconcileVia( net, ic, c->nsegs );
		int ic=0, iv=0;
		if( FindTeeVertexInNet( net, id, &ic, &iv ) )
		{
			ReconcileVia( net, ic, iv );
		}
		return RemoveOrphanBranches( net, id );
	}
}

// Remove tee-vertex from net
// Don't change stubs connected to it
// return connection number of tee vertex or -1 if not found
//
int CNetList::RemoveTee( cnet * net, int id, int numconnect )
{
	int tee_ic = -1;
	for( int ic=net->nconnects-1; ic>=0; ic-- )
	{
		cconnect * c = &net->connect[ic];
		int redraw = 0;
		for( int iv=0; iv<=c->nsegs; iv++ )
		{
			cvertex * v = &c->vtx[iv];
			if( v->tee_ID == id )
			{
				if (ic == numconnect || numconnect == -1)
				{
					v->tee_ID = 0;
					////ReconcileVia( net, ic, iv );
					tee_ic = ic;
					redraw = 1;
				}
			}
		}
		if( redraw )
			DrawConnection( net, ic );
	}
	RemoveTeeID( id );
	return tee_ic;
}

// see if a tee vertex needs a via
//
BOOL CNetList::TeeViaNeeded( cnet * net, int id )
{
	int layer = 0;
	for( int ic=0; ic<net->nconnects; ic++ )
	{
		cconnect * c = &net->connect[ic];
		for( int iv=1; iv<=c->nsegs; iv++ )
		{
			cvertex * v = &c->vtx[iv];
			if( v->tee_ID == id )
			{
				int seg_layer = c->seg[iv-1].layer;
				if( seg_layer >= LAY_TOP_COPPER )
				{
					if( layer == 0 )
						layer = seg_layer;
					else if( layer != seg_layer )
						return TRUE;
				}
				if( iv < c->nsegs )
				{
					seg_layer = c->seg[iv].layer;
					if( seg_layer >= LAY_TOP_COPPER )
					{
						if( layer == 0 )
							layer = seg_layer;
						else if( layer != seg_layer )
							return TRUE;
					}
				}
			}
		}
	}
	return FALSE;
}

// Finds branches without tees
// If possible, combines branches into a new trace
// If id == 0, check all ids for this net
// If bRemoveSegs, removes branches entirely instead of disconnecting them
// returns TRUE if corrections required, FALSE if not
// Note that the connect[] array may be changed by this function
//
int CNetList::RemoveOrphanBranches( cnet * net, int id, BOOL CombineOrphan, int via, int hole )
{
	if ( id == 0 )
	{
		// not an orphan
		return -1;
	}
	int n_tee_middle = 0;
	int n_tee_edge = 0;
	int ic1=-1, ic2=-1, iv1=-1, iv2=-1;
	for( int ic=net->nconnects-1; ic>=0; ic-- )
	{
		cconnect * c = &net->connect[ic];
		for( int iv=c->nsegs; iv>=0; iv-- )
		{
			if (c->vtx[iv].tee_ID == id)
			{
				if (iv == c->nsegs)
				{
					n_tee_edge++;
					if (ic1 == -1)
					{
						ic1 = ic;
						iv1 = iv;
					}
					else
					{
						ic2 = ic;
						iv2 = iv;
					}
				}
				else
					n_tee_middle++;
			}
		}
	}
	if ( n_tee_middle == 1 && n_tee_edge )
	{
		// not an orphan
		return -1;
	}
	else if ( n_tee_middle == 0 && n_tee_edge >= 2 && CombineOrphan)
	{
		// remove tee
		if( n_tee_edge == 2 )
			RemoveTee(net,id);
		if( net->connect[ic1].vtx[net->connect[ic1].nsegs].via_hole_w == 0 )
		{
			net->connect[ic1].vtx[net->connect[ic1].nsegs].via_hole_w = net->connect[ic2].vtx[net->connect[ic2].nsegs].via_hole_w;
			net->connect[ic1].vtx[net->connect[ic1].nsegs].via_w = net->connect[ic2].vtx[net->connect[ic2].nsegs].via_w;
		}
		for (int ic=net->connect[ic2].nsegs; ic>0; ic--)
		{
			cseg * s = &net->connect[ic2].seg[ic-1];
			cvertex * v = &net->connect[ic2].vtx[ic-1];
			//cvertex * post_v = &net->connect[ic2].vtx[ic];
			AppendSegment(net, ic1, v->x, 
									v->y, 
									s->layer, 
									s->width, v->via_w, v->via_hole_w, TRUE );
			net->connect[ic1].vtx[net->connect[ic1].nsegs].tee_ID = v->tee_ID;
		}
		ReconcileVia( net, ic1, iv1, 0, via, hole );
		net->connect[ic1].end_pin = net->connect[ic2].start_pin;
		net->connect[ic1].end_pin_shape = net->connect[ic2].start_pin_shape;
		net->connect[ic1].vtx[net->connect[ic1].nsegs].pad_layer = net->connect[ic2].vtx[0].pad_layer;
		DrawConnection( net, ic1 );
		RemoveNetConnect( net, ic2 );
		return ic2;
	}
	else
	{
		// remove tee
		RemoveTee(net,id);
		return -1;
	}
}




// recursive function for routing
//
// globals:
int g_score;					// score for current route
int g_num_steps;				// num paths in current route
int g_path_index[100];			// list of paths for current route
int g_path_end[100];			// list of path ends for current route
int g_best_score;				// best score found so far
int g_best_num_steps;			// num paths in best route
int g_best_path_index[100];		// list of paths for best route
int g_best_path_end[100];		// list of path ends for best route
//
int RouteToPin( int step, 
				cnode * node, 
				CArray<cnode> * nodes, 
				CArray<cpath> * paths,
				CDlgLog * log )
{
	g_num_steps = step;			// number of steps so far
	int old_score = g_score;	// score so far
	if( step == 0 )
	{
		// if this is the first step, reset globals
		g_score = 0;
		g_best_score = -1;	
		g_best_num_steps = 0;
	}
	else if( node->type == NPIN )
	{
		// SUCCESS, this node is a pin
		// if g_score > g_best_score, save this route
		if( g_score > g_best_score )
		{
			// first, restore used flags of previous best route
			if( g_best_score > -1 )
			{
				for( int i=0; i<g_best_num_steps; i++ )
				{
					cpath * path = &(*paths)[g_best_path_index[i]];
					path->n_used--;		
				}
			}
			// now save new best route
			g_best_score = g_score;
			g_best_num_steps = g_num_steps;
			for( int i=0; i<step; i++ )
			{
				g_best_path_index[i] = g_path_index[i];
				g_best_path_end[i] = g_path_end[i];
				cpath * path = &(*paths)[g_path_index[i]];
				path->n_used++;		
			}
		}
		return g_best_score;
	}

	// count available paths from this node
	int npaths = node->path_index.GetSize();
	int num_unused_paths = 0;
	for( int ip=0; ip<npaths; ip++ )
	{
		int path_index = node->path_index[ip];
		cpath * path = &(*paths)[path_index];
		if( !path->n_used )
			num_unused_paths++;
	}
	if( num_unused_paths == 0 )
	{
		// can't go on, but didn't reach pin
	}
	else
	{
		// increase score if this node has > 1 available path
		int new_score;
		if( num_unused_paths > 1 )
			new_score = old_score + 1;	
		else
			new_score = old_score;
		// now try all available paths
		for( int ip=0; ip<npaths; ip++ )
		{
			int path_index = node->path_index[ip];
			int path_start = node->path_end[ip];	// start of path
			int path_end = 1 - node->path_end[ip];  // end of path
			cpath * path = &(*paths)[path_index];
			if( !path->n_used )
			{
				// try this path, get next node and continue routing
				int next_inode = path->GetInode( path_end );
				path->n_used++;
				cnode * next_node = &(*nodes)[next_inode];
				g_path_index[step] = path_index;
				g_path_end[step] = path_start;
				g_score = new_score;
				RouteToPin( step+1, next_node, nodes, paths, log );
				path->n_used--;
			}
		}
	}
	g_num_steps = step;		// revert num steps
	g_score = old_score;	// revert score
	return g_best_score;	// return best score found so far
}

// import routing data from autorouter for a net
//
void CNetList::ImportNetRouting( CString * name, 
								CArray<cnode> * nodes, 
								CArray<cpath> * paths,
								int tolerance,
								CDlgLog * log,
								BOOL bVerbose )
{
	CString mess;
	cnet * net = GetNetPtrByName( name );
	if( net == NULL )
	{
		ASSERT(0);
		return;
	}

	// unroute all traces
	for( int ic=net->nconnects-1; ic>=0; ic-- )
		RemoveNetConnect( net, ic, FALSE );
	SetAreaConnections( net );

	// add all pins in net to list of nodes
	for( int ip=0; ip<net->npins; ip++ ) 
	{
		cpin * net_pin = &net->pin[ip];
		int index = net_pin->part->shape->GetPinIndexByName( net_pin->pin_name, -1 );
		int layer = m_plist->GetPinLayer( net_pin->part, index );
		CPoint p = m_plist->GetPinPoint( net_pin->part, index, net_pin->part->side, net_pin->part->angle );
		int inode = nodes->GetSize();
		nodes->SetSize( inode+1 );
		cnode * node = &(*nodes)[inode];
		node->bUsed = FALSE;
		node->layer = layer;
		node->type = NPIN;
		node->x = p.x;
		node->y = p.y;
		node->pin_index = ip;
	}
	// now hook up paths and nodes
	for( int ipath=0; ipath<paths->GetSize(); ipath++ )
	{
		cpath * path = &(*paths)[ipath];
		for( int iend=0; iend<2; iend++ )
		{
			cpath_pt * pt = &path->pt[0];	// first point in path
			if( iend == 1 )
				pt = &path->pt[path->pt.GetSize()-1];	// last point
			// search all nodes for match
			BOOL bMatch = FALSE;
			int inode;
			for( inode=0; inode<nodes->GetSize(); inode++ )
			{
				cnode * node = &(*nodes)[inode];
				if( abs(pt->x-node->x)<tolerance && abs(pt->y-node->y)<tolerance 
					&& ( path->layer == node->layer || node->layer == LAY_PAD_THRU ) )
				{
					// match, hook it up
					int ip = node->path_index.GetSize();
					node->path_index.SetSize(ip+1);
					node->path_end.SetSize(ip+1);
					node->path_index[ip] = ipath;
					node->path_end[ip] = iend;
					pt->inode = inode;
					pt->x = node->x;
					pt->y = node->y;
					bMatch = TRUE;
					break;
				}
			}
			if( !bMatch )
			{
				// node not found, make new junction node
				inode = nodes->GetSize();
				nodes->SetSize(inode+1);
				cnode * node = &(*nodes)[inode];
				node->bUsed = FALSE;
				node->layer = path->layer;
				node->type = NJUNCTION;
				node->x = pt->x;
				node->y = pt->y;
				int ip = node->path_index.GetSize();
				node->path_index.SetSize(ip+1);
				node->path_end.SetSize(ip+1);
				node->path_index[ip] = ipath;
				node->path_end[ip] = iend;
				pt->inode = inode;
			}
		}
	}
	// dump data
	if( log )
	{
		mess.Format( "\r\nrouting net \"%s\"\r\n", *name );
		log->AddLine( mess );
		if( bVerbose )
		{
			mess.Format( "num nodes = %d\r\n", nodes->GetSize() );
			log->AddLine( mess );
			for( int inode=0; inode<nodes->GetSize(); inode++ )
			{
				cnode * node = &(*nodes)[inode];
				CString type_str = "none";
				if( node->type == NPIN )
					type_str = "pin";
				else if( node->type == NVIA )
					type_str = "via";
				else if( node->type == NJUNCTION )
					type_str = "junction";
				mess.Format( "  node %d: %s x=%d y=%d layer=%d npaths=%d\r\n",
					inode, type_str, node->x/NM_PER_MIL, node->y/NM_PER_MIL, node->layer, node->path_index.GetSize() );
				log->AddLine( mess );
			}
		}
	}
	// start routing
	for( int ipass=0; ipass<3; ipass++ )
	{
		// if ipass == 0, route stubs
		// if ipass == 1, route pin-pin traces
		// if ipass == 2, route branches
		for( int inode=0; inode<nodes->GetSize(); inode++ ) 
		{
			cnode * node = &(*nodes)[inode];
			if( ( ipass == 0 && node->type != NPIN && node->path_index.GetSize() == 1 ) 
				|| ( ipass == 1 && node->type == NPIN ) 
				|| ( ipass == 2 && node->type != NPIN && node->path_index.GetSize() > 2 ) )
			{
				int num_unused_paths = 0;
				int npaths = node->path_index.GetSize();
				for( int ip=0; ip<npaths; ip++ )
				{
					int path_index = node->path_index[ip];
					cpath * path = &(*paths)[path_index];
					if( !path->n_used )
						num_unused_paths++;
				}
				BOOL bFailed = (num_unused_paths == 0);		// fails if no paths
				while( !bFailed )
				{
					// route trace
					int score = RouteToPin( 0, node, nodes, paths, NULL );
					if( score == -1 )
						bFailed = TRUE;
					if( !bFailed )
					{
						// add routed trace to project
						if( ipass == 0 )
							mess = "stub: ";
						else if( ipass == 1 )
							mess = "pin-pin: ";
						else if( ipass == 2 )
							mess = "branch: ";
						CString str;
						// create new connection
						int ic = -1;
						cconnect * c = NULL;
						int is = 0;
						for( int istep=g_best_num_steps-1; istep>=0; istep-- )
						{
							// iterate backward through steps, so we always start on pin
							int path_index = g_best_path_index[istep];
							int path_end = g_best_path_end[istep];
							cpath * path = &(*paths)[path_index];
							int next_inode = path->GetInode( path_end );
							int prev_inode = path->GetInode( 1-path_end );
							cnode * prev_node = &(*nodes)[prev_inode];
							cnode * next_node = &(*nodes)[next_inode];
							if( istep == g_best_num_steps-1 )
							{
								// first step, create connection
								if( ipass == 0 || ipass == 2 )
									ic = AddNetStub( net, prev_node->pin_index );
								else if( ipass == 1 )
								{
									int last_path_index = g_best_path_index[0];
									int last_path_end = g_best_path_end[0];
									cpath * last_path = &(*paths)[last_path_index];
									int last_inode = last_path->GetInode( last_path_end );
									cnode * last_node = &(*nodes)[last_inode];
									ic = AddNetConnect( net, prev_node->pin_index, last_node->pin_index );
								}
								str.Format( "n%d", prev_inode );
								mess += str;
							}
							if( ic > -1 )
								c = &net->connect[ic];
							else
								ASSERT(0);
							
							// iterate through points in path
							int n_pts = path->pt.GetSize()-1;
							if( n_pts )
							{
								net->connect[ic].vtx[0].x = path->pt[0].x;
								net->connect[ic].vtx[0].y = path->pt[0].y;
							}
							for( int ipt=0; ipt<n_pts; ipt++ )
							{
								int next_pt = ipt+1;
								if( path_end == 0 )
									next_pt = n_pts - ipt - 1;
								int x = path->pt[next_pt].x;
								int y = path->pt[next_pt].y;
								int layer = path->layer;
								int width = path->width;
								if( ipass == 0 || ipass == 2 )
									AppendSegment( net, ic, x, y, layer, width );
								else if( ipass == 1 )
									InsertSegment( net, ic, is, x, y, layer, width, 0, 0, 0 );
								is++;
							}
							// force all vias
							if( next_node->type == NVIA )
							{
								// use via width from node and default hole width
								int w, via_w, via_hole_w;
								GetWidths( net, &w, &via_w, &via_hole_w );
								c->vtx[is].via_w = next_node->via_w;
								c->vtx[is].via_hole_w = via_hole_w;
								ForceVia( net, ic, is, FALSE );
							}
							str.Format( "-n%d", next_inode );
							mess += str;
						}
						if( ipass == 2 )
						{
							int tee_id = GetNewTeeID();
							cconnect * c = &net->connect[ic];
							int end_v = c->nsegs;
							cvertex * v = &c->vtx[end_v];
							v->tee_ID = tee_id;
						}
						mess += "\r\n";
						if( log && bVerbose )
							log->AddLine( mess );
						// at this point, all imported vias are forced
						// unforce the ones between segments on different layers that don't 
						// connect to copper areas
						for( int iv=1; iv<c->nsegs; iv++ )
						{
							cvertex * v = &c->vtx[iv];
							cseg * pre_seg = &c->seg[iv-1];
							cseg * post_seg = &c->seg[iv];
							if( pre_seg->layer != post_seg->layer && !TestPointInArea( net, v->x, v->y, LAY_PAD_THRU, NULL ) )
								UnforceVia( net, ic, iv, FALSE );
						}

					}
				}
			}
		}
	}
	// now hook up branches to traces
	for( int ic=0; ic<net->nconnects; ic++ )
	{
		cconnect * c = &net->connect[ic];
		if( c->end_pin == cconnect::NO_END )
		{
			cvertex * end_v = &c->vtx[c->nsegs];
			cseg * end_seg = &c->seg[c->nsegs-1];
			int tee_id = end_v->tee_ID;
			if( tee_id )
			{
				//  find trace with corresponding vertex
				for( int icc=0; icc<net->nconnects; icc++ )
				{
					cconnect * trace_c = &net->connect[icc];
					for( int iv=1; iv<trace_c->nsegs; iv++ )
					{
						cvertex * trace_v = &trace_c->vtx[iv];
						cseg * trace_seg = &trace_c->seg[iv-1];
						if( trace_v->x == end_v->x && trace_v->y == end_v->y
							&& ( trace_v->via_w || end_v->via_w 
							|| end_seg->layer == trace_seg->layer ) ) 
						{
							// make a tee-vertex and connect branch
							if( trace_v->tee_ID )
								tee_id = trace_v->tee_ID;
							trace_v->tee_ID = tee_id;
							end_v->tee_ID = tee_id;
							end_v->force_via_flag = FALSE;
							end_v->via_w = 0;
							end_v->via_hole_w = 0;
							ReconcileVia( net, icc, iv );
						}
					}
				}
			}
		}
	}
	// test for unrouted paths
	BOOL bFailed = FALSE;
	for( int ipath=0; ipath<paths->GetSize(); ipath++ )
	{
		cpath * path = &(*paths)[ipath];
		if( path->n_used == 0 )
		{
			if( log )
			{
				CString str;
				mess.Format( "error: path %d failed to route", ipath );
				cnode * node = &(*nodes)[path->GetInode(0)];
				CString type_str = "pin";
				if( node->type == NVIA )
					type_str = "via";
				else if( node->type == NJUNCTION )
					type_str = "junction";
				cnode * node_end = &(*nodes)[path->GetInode(1)];
				str.Format( ", %s at x=%d y=%d",
					type_str, node->x/NM_PER_MIL, node->y/NM_PER_MIL );
				mess += str;
				node = &(*nodes)[path->GetInode(1)];
				type_str = "pin";
				if( node->type == NVIA )
					type_str = "via";
				else if( node->type == NJUNCTION )
					type_str = "junction";
				str.Format( " to %s at x=%d y=%d, layer %d\r\n",
					type_str, node->x/NM_PER_MIL, node->y/NM_PER_MIL,
					path->layer );
				mess += str;
				log->AddLine( mess );
			}
			bFailed = TRUE;
		}
	}
	if( !bFailed && log ) 
		log->AddLine( "success: all paths routed\r\n" );
	SetAreaConnections( net );
}




void CNetList::AddHighlightLines( int X, int Y, int mode )
{	// mode == 1: Vertical line only
	// mode == 2: Horizontal line only
	// mode == 3: Vertical&Horizontal lines
	// mode == 4: Vertical&Horizontal lines
#define hghlght_np 400.0
	CPoint pts[(int)hghlght_np];
	RECT WR = m_dlist->GetWindowRect();
	double d = ((double)WR.right-(double)WR.left)/(hghlght_np/4.0-1.0);
	if( WR.right-WR.left < abs(WR.top-WR.bottom) )
		d = abs((double)WR.top-(double)WR.bottom)/(hghlght_np/4.0-1.0);
	int ii = 0;
	if (mode != 1)
	{
		for (double x=WR.left; x<WR.right; x += d)
		{
			pts[ii].x = x*m_pcbu_per_wu;
			pts[ii].y = Y;
			ii++;
			pts[ii].x = (x+d/3)*m_pcbu_per_wu;
			pts[ii].y = Y;
			ii++;
		}
	}
	if (mode != 2)
	{
		for (double y=WR.bottom; y<WR.top; y += d)
		{
			pts[ii].x = X;
			pts[ii].y = y*m_pcbu_per_wu;
			ii++;
			pts[ii].x = X;
			pts[ii].y = (y+d/3)*m_pcbu_per_wu;
			ii++;
		}		
	}
	WR.left		*= m_pcbu_per_wu;
	WR.right	*= m_pcbu_per_wu;
	WR.bottom	*= m_pcbu_per_wu;
	WR.top		*= m_pcbu_per_wu;
	id id(0,0,0,0,0);
	dl_element * el = m_dlist->Add( id, NULL, 0, DL_LINES_ARRAY, 1, &WR, 0, pts, ii );
	int top_l = m_dlist->GetTopLayer();
	setbit( el->map_orig_layer, top_l );
	m_dlist->HighLight( el );
}

