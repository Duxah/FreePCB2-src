// definition of ID structure used by FreePCB
//
#pragma once

// struct id : this structure is used to identify PCB design elements
// such as instances of parts or nets, and their subelements
// Each element will have its own id.
// An id is attached to each item of the Display List so that it can
// be linked back to the PCB design element which drew it.
// These are mainly used to identify items selected by clicking the mouse
//
// In general: 
//		id.type	= type of PCB element (e.g. part, net, text)
//		id.st	= subelement type (e.g. part pad, net connection)
//		id.i	= subelement index (zero-based)
//		id.sst	= subelement of subelement (e.g. net connection segment)
//		id.ii	= subsubelement index (zero-based)
//
// For example, the id for segment 0 of connection 4 of net 12 would be
//	id = { ID_NET, 12, ID_CONNECT, 4, ID_SEG, 0 };
//
//
class id {
public:
	// constructor
	id( int qt=0, int qst=0, int qis=0, int qsst=0, int qiis=0 ) 
	{ type=qt; st=qst; i=qis; sst=qsst; ii=qiis; } 
	// operators
	friend int operator ==(id id1, id id2)
	{ return (id1.type==id2.type 
			&& id1.st==id2.st 
			&& id1.sst==id2.sst 
			&& id1.i==id2.i 
			&& id1.ii==id2.ii ); 
	}
	// member functions
	void Clear() 
	{ type=0; st=0; i=0; sst=0; ii=0; } 
	void Set( int qt, int qst=0, int qis=0, int qsst=0, int qiis=0 ) 
	{ type=qt; st=qst; i=qis; sst=qsst; ii=qiis; } 
	// member variables
	unsigned int type;	// type of element
	unsigned int st;	// type of subelement
	unsigned int i;		// index of subelement
	unsigned int sst;	// type of subsubelement
	unsigned int ii;	// index of subsubelement
};


// these are constants used in ids
// root types
enum {
	ID_NONE = 0,	// an undefined type or st (or an error)
	ID_PART,		// part
	ID_PART_LINES,	// part ref, value
	ID_NET,			// net
	ID_TEXT,		// free-standing text
	ID_DRC,			// DRC error
	ID_CENTROID,	// centroid of footprint
	ID_GLUE,		// adhesive spot
	ID_POLYLINE,	// polylines
	ID_MULTI		// if multiple selections
};

// subtypes
enum {
	// subtypes of ID_PART
	ID_PAD = 1,			// pad_stack in a part
	ID_OUTLINE,			// part outline
	ID_REF_TXT,			// text showing ref num for part
	ID_VALUE_TXT,		// text showing value for part
	ID_FP_TXT,			// free text in footprint
	ID_ORIG,			// part origin
	ID_SEL_RECT,		// selection rectangle for part
	ID_SEL_REF_TXT,		// selection rectangle for ref text
	ID_SEL_VALUE_TXT,	// selection rectangle for value text

	// subtypes of ID_TEXT  
	ID_TXT,				

	// subtypes of ID_NET
	ID_ENTIRE_NET,
	ID_CONNECT,		// connection
	ID_AREA,		// copper area

	// subtypes of polylines
	ID_GRAPHIC,		// graphic lines
	ID_BOARD,		// board outline
	ID_SM_CUTOUT,	// cutout for solder mask

	// subtypes of ID_DRC
	// for subsubtypes, use types in DesignRules.h
	ID_DRE,

	// subtypes of ID_CENTROID
	ID_CENT,

	// subtypes of ID_GLUE
	ID_SPOT
};


// subsubtypes 
enum {
	// subsubtypes of ID_NET.ID_CONNECT
	//ID_ENTIRE_CONNECT = 0,
	ID_SEG = 1,	
	ID_VERTEX,	
	ID_VIA,
	// for pad
	ID_HOLE,
	// subsubtypes of ID_NET.ID_AREA, ID_POLYLINE
	ID_SIDE,
	ID_CORNER,
	ID_HATCH,
	ID_PIN_X,	// only used by ID_AREA
	ID_STUB_X,	// only used by ID_AREA
	ID_STROKE			// stroke for text or outlines
};




