//
#pragma once

struct DesignRules
{
	BOOL bCheckUnrouted;
	int trace_width; 
	int pad_pad; 
	int pad_trace;
	int trace_trace; 
	int hole_copper; 
	int annular_ring_pins;
	int annular_ring_vias;
	int board_edge_copper;
	int board_edge_hole;
	int hole_hole;
	int copper_copper;
};

class DRError
{
public:
	DRError();
	~DRError();
	enum {				// subtypes of errors 
		PAD_PAD = 0,
		PAD_OP,
		PAD_PADHOLE,		
		PADHOLE_PADHOLE,	
		SEG_PAD,			
		SEG_PADHOLE,	
		VIA_PAD,			
		VIA_PADHOLE,		
		VIAHOLE_PAD,		
		VIAHOLE_PADHOLE,	
		SEG_SEG,
		SEG_OP,
		SEG_VIA,			
		SEG_VIAHOLE,	
		VIA_VIA,			
		VIA_VIAHOLE,		
		VIAHOLE_VIAHOLE,	
		TRACE_WIDTH,	
		RING_PAD,			
		RING_VIA,			
		BOARDEDGE_PAD,			
		BOARDEDGE_PADHOLE,	
		BOARDEDGE_VIA,			
		BOARDEDGE_VIAHOLE,		
		BOARDEDGE_TRACE,	
		BOARDEDGE_COPPERAREA,	
		COPPERAREA_COPPERAREA,
		COPPERAREA_INSIDE_COPPERAREA,
		COPPERAREAFULL_TRACE,
		COPPERAREAFULL_OP,
		COPPERAREAFULL_VIA,
		COPPERAREAFULL_VIA_INSIDE,
		COPPERAREAFULL_PAD,
		UNROUTED
	};
	int layer;				// layer (if pad error)
	id m_id;				// id, using subtypes above
	CString str;			// descriptive string
	CString name1, name2;	// names of nets or parts tested
	id id1, id2;			// ids of items tested
	int x, y;				// position of error
	int selected;
	dl_element * dl_el;		// DRC graphic
	dl_element * dl_sel;	// DRC graphic selector
};

class DRErrorList
{
public:
	DRErrorList();
	~DRErrorList();
	void SetLists( CPartList * pl, CNetList * nl, CDisplayList * dl, CTextList * tl, CArray<CPolyLine>* poly );
	void Clear();
	DRError * Add( long index, int st, CString * str, 
		CString * name1, CString * name2, id id1, id id2,
		int x1, int y1, int x2, int y2, int w, int layer );
	void Remove( DRError * dre );
	void HighLight( DRError * dre );
	void MakeSolidCircles();
	void MakeHollowCircles();

public:
	CPartList * m_plist;
	CNetList * m_nlist;
	CDisplayList * m_dlist;
	CTextList * m_tlist;
	CArray<CPolyLine> * m_poly;
	CPtrList list;
};