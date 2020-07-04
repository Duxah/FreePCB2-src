// Shape.h : interface for the CShape class
//
#pragma once

#include "stdafx.h"
#include "PolyLine.h"
#include "DisplayList.h"
#include "SMFontUtil.h"
#include "TextList.h"

class CTextList;

#define CENTROID_WIDTH 30*NM_PER_MIL	// width of centroid symbol
#define DEFAULT_GLUE_WIDTH 15*NM_PER_MIL	// width of default glue spot

// pad shapes
enum {
	PAD_NONE = 0,
	PAD_ROUND,
	PAD_SQUARE,
	PAD_RECT,
	PAD_RRECT,
	PAD_OVAL,
	PAD_OCTAGON,
	PAD_DEFAULT = 99
};

// pad area connect flags
enum {
	PAD_CONNECT_DEFAULT = 0,
	PAD_CONNECT_NEVER,
	PAD_CONNECT_THERMAL,
	PAD_CONNECT_NOTHERMAL
};

// error returns
enum
{
	PART_NOERR = 0,
	PART_ERR_TOO_MANY_PINS
};

// centroid types
enum CENTROID_TYPE
{
	CENTROID_DEFAULT = 0,	// center of pads
	CENTROID_DEFINED		// defined by user
};

// glue spot position types 
enum GLUE_POS_TYPE
{
	GLUE_POS_CENTROID,	// at centroid
	GLUE_POS_DEFINED	// defined by user
};

// structure describing pad flags
struct flag
{
	unsigned int mask : 1;
	unsigned int area : 2;
};

// structure describing adhesive spot
struct glue
{
	GLUE_POS_TYPE type;
	int w, x_rel, y_rel, selected;
};

// structure describing stroke (ie. line segment)
struct stroke
{
	int w, xi, yi, xf, yf;	// thickness + endpoints
	int type;				// CDisplayList g_type
	dl_element * dl_el;		// pointer to graphic elements for stroke;
	void * ptr;
};

// structure describing mounting hole
// only used during conversion of Ivex files
struct mtg_hole
{
	int pad_shape;		// used for pad on top and bottom
	int x, y, diam, pad_diam;
};


// structure describing pad
class pad
{
public:
	pad();
	BOOL operator==(pad p);
	int shape;	// see enum above
	int size_X, size_Y, radius;
	int connect_flag;	// only for copper pads
};

// padstack is pads and hole associated with a pin
class padstack
{
public:
	padstack();
	BOOL operator==(padstack p);
	BOOL exists;				// only used when converting Ivex footprints or editing
	CString name;				// identifier such as "1" or "B24"
	CString pDescription;		
	int hole_size;		// 0 = no hole (i.e SMT)
	int x_rel, y_rel;	// position relative to part origin
	int angle;			// orientation: 0=left, 90=top, 180=right, 270=bottom
	pad top, top_mask, top_paste;
	pad bottom, bottom_mask, bottom_paste;
	pad inner;
	RECT bounds;
	int selected;
};

// CShape class represents a footprint
//
class CShape
{
	// if variables are added, remember to modify Copy!
public:
	enum { MAX_NAME_SIZE = 59 };	// max. characters
	enum { MAX_PIN_NAME_SIZE = 39 };
	enum { MAX_VALUE_SIZE = 39 };
	CString m_name;		// name of shape (e.g. "DIP20")
	CString m_author;
	CString m_package;
	CString m_desc;
	BOOL bDrawn;
	int origin_moved_X, origin_moved_Y;
	int m_units;		// units used for original definition (MM, NM or MIL)
	RECT selection;		// selection rectangle
	int m_ref_size, m_ref_xi, m_ref_yi, m_ref_angle;	// ref text
	int m_ref_w;						// thickness of stroke for ref text
	int m_value_size, m_value_xi, m_value_yi, m_value_angle;	// value text
	int m_value_w;						// thickness of stroke for value text
	CENTROID_TYPE m_centroid_type;		// type of centroid
	int m_centroid_x, m_centroid_y;		// position of centroid
	int m_centroid_angle;				// angle of centroid (CCW)
	CArray<padstack> m_padstack;		// array of padstacks for shape
	CArray<CPolyLine> m_outline_poly;	// array of polylines for part outline
	CTextList * m_tl;					// list of text strings
	CArray<glue> m_glue;		// array of adhesive dots

public:
	CShape();
	~CShape();
	void Clear();
	int MakeFromString( CString name, CString str );
	int MakeFromFile( CStdioFile * in_file, CString name, CString file_path, int pos );
	int WriteFootprint( CStdioFile * file );
	int GetNumPins();
	int GetPinIndexByName( LPCTSTR name, int prev_i );
	CString GetPinNameByIndex( int ip );
	CString GetPinDescriptionByIndex( int ip );
	RECT GetBounds( BOOL bIncludeLineWidths, BOOL bThruPadsOnly=0 );
	void GetPadBounds( int i, RECT * PR );
	RECT GetCornerBounds();
	RECT GetPadRowBounds( int i, int num );
	CPoint GetDefaultCentroid();
	RECT GetAllPadBounds();
	BOOL GetHoleFlag();
	int Copy( CShape * shape );	// copy all data from shape
	int Import( CShape * shape );	// copy all data from shape
	BOOL Compare( CShape * shape );	// compare shapes, return true if same
	HENHMETAFILE CreateMetafile( CMetaFileDC * mfDC, CDC * pDC, int x_size, int y_size );
	HENHMETAFILE CreateWarningMetafile( CMetaFileDC * mfDC, CDC * pDC, int x_size, int y_size );

	// CShape class represents a footprint whose elements can be edited
	// EditShape
public:
	CDisplayList * m_dlist;
	CArray<dl_element*> m_hole_el;			// hole display element 
	CArray<dl_element*> m_pad_top_el;		// top pad display element 
	CArray<dl_element*> m_pad_inner_el;		// inner pad display element 
	CArray<dl_element*> m_pad_bottom_el;	// bottom pad display element 
	CArray<dl_element*> m_pad_top_mask_el;
	CArray<dl_element*> m_pad_top_paste_el;
	CArray<dl_element*> m_pad_bottom_mask_el;
	CArray<dl_element*> m_pad_bottom_paste_el;
	CArray<dl_element*> m_pad_sel;		// pad selector
	CArray<dl_element*> m_ref_el;		// strokes for "REF"
	CArray<dl_element*> m_value_el;     // strokes for "VALUE"
	dl_element * m_ref_sel;				// ref selector			
	dl_element * m_value_sel;			// value selector
	dl_element * m_centroid_el;			// centroid
	dl_element * m_centroid_sel;		// centroid selector
	CArray<dl_element*> m_dot_el;		// adhesive dots
	CArray<dl_element*> m_dot_sel;		// adhesive dot selectors

public: 
	void Draw( CDisplayList * dlist, SMFontUtil * fontutil );
	void Undraw();
	void SelectPad( int i );
	void StartDraggingPad( CDC * pDC, int i );
	void CancelDraggingPad( int i );
	void StartDraggingPadRow( CDC * pDC, int i, int num );
	void CancelDraggingPadRow( int i, int num );
	void StartDraggingGroup( CDC * pDC, RECT * R );
	void CancelDraggingGroup();
	void SelectRef();
	void StartDraggingRef( CDC * pDC );
	void CancelDraggingRef();
	void SelectValue();
	void StartDraggingValue( CDC * pDC );
	void CancelDraggingValue();
	void SelectAdhesive( int idot );
	void StartDraggingAdhesive( CDC * pDC, int idot );
	void CancelDraggingAdhesive( int idot );
	void SelectCentroid();
	void StartDraggingCentroid( CDC * pDC );
	void CancelDraggingCentroid();
	void ShiftToInsertPadName( CString * astr, int n );
	BOOL GenerateSelectionRectangle( RECT * r );

};

