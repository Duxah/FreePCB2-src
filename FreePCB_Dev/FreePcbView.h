// FreePcbView.h : interface of the CFreePcbView class
//  
/////////////////////////////////////////////////////////////////////////////


#if !defined(AFX_FREEPCBVIEW_H__BE1CA173_E2B9_4252_8422_0B9767B01566__INCLUDED_)
#define AFX_FREEPCBVIEW_H__BE1CA173_E2B9_4252_8422_0B9767B01566__INCLUDED_

#pragma once
#include "stdafx.h"
#include "afxwin.h"
#include "DisplayList.h"
#include "FreePcbDoc.h"   

class CFreePcbView; 
  
// cursor modes
enum {
	CUR_NONE_SELECTED = 0,		// nothing selected
	CUR_OP_CORNER_SELECTED,		// corner of outline sel.  
	CUR_OP_SIDE_SELECTED,		// edge of outline sel. 
	CUR_PART_SELECTED,			// part selected
	CUR_REF_SELECTED,			// ref text in part selected
	CUR_VALUE_SELECTED,			// value in part selected
	CUR_PAD_SELECTED,			// pad in part selected
	CUR_SEG_SELECTED,			// trace segment selected
	CUR_RAT_SELECTED,			// unrouted trace segment selected
	CUR_VTX_SELECTED,			// trace vertex selected
	CUR_END_VTX_SELECTED,		// end vertex of stub trace selected
	CUR_CONNECT_SELECTED,		// entire connection selected
	CUR_NET_SELECTED,			// entire net selected
	CUR_TEXT_SELECTED,			// free text selected
	CUR_AREA_CORNER_SELECTED,	// corner of copper area selected
	CUR_AREA_SIDE_SELECTED,		// edge of copper area selected
	CUR_DRE_SELECTED,			// DRC error selected
	CUR_GROUP_SELECTED,			// multiple parts selected
	CUR_NUM_SELECTED_MODES,		// number of SELECTED modes
	// dragging modes
	CUR_ADD_OP,		// dragging starting point of outline
	CUR_DRAG_OP_1,	// dragging first corner of outline
	CUR_DRAG_OP,		// dragging next corner of outline
	CUR_DRAG_OP_INSERT,	// dragging corner being inserted
	CUR_DRAG_OP_MOVE,	// dragging corner being moved
	CUR_DRAG_PART,		// dragging part
	CUR_DRAG_REF,		// dragging ref text of part
	CUR_DRAG_VALUE,		// dragging value of part
	CUR_DRAG_RAT,		// dragging ratline for trace segment
	CUR_DRAG_VTX,		// dragging trace vertex
	CUR_DRAG_VTX_INSERT,	// dragging new vertex being inserted
	CUR_DRAG_END_VTX,	// dragging end vertex being moved
	CUR_DRAG_TEXT,		// dragging text box
	CUR_ADD_AREA,		// setting starting point for copper area
	CUR_DRAG_AREA_1,	// dragging first corner for copper area
	CUR_DRAG_AREA,		// dragging next corner for copper area
	CUR_DRAG_AREA_INSERT,	// dragging area corner being inserted
	CUR_DRAG_AREA_MOVE,		// dragging area corner being moved
	CUR_ADD_AREA_CUTOUT,	// setting starting point for area cutout
	CUR_DRAG_AREA_CUTOUT_1,	// dragging first corner for area cutout
	CUR_DRAG_AREA_CUTOUT,	// dragging next corner for area cutout
	CUR_DRAG_STUB,		// dragging ratline to next stub endpoint
	CUR_DRAG_CONNECT,	// dragging ratline to new connection
	CUR_DRAG_RAT_PIN,	// dragging ratline to new end pin of trace
	CUR_MOVE_ORIGIN,	// dragging new origin
	CUR_DRAG_GROUP,		// dragging a group of parts/segments
	CUR_DRAG_GROUP_ADD,	// dragging a group being added
	CUR_DRAG_MEASURE_1,	// dragging the start of measurement tool
	CUR_DRAG_MEASURE_2,	// dragging the end of measurement tool
	CUR_MOVE_SEGMENT,	// move a segment, leaving it connected to its ends
	CUR_NUM_MODES		// number of modes
};

// function key options
enum {
	FK_NONE = 0,
	FK_MOVE_PART,
	FK_MOVE_REF,
	FK_MOVE_VALUE,
	FK_ROTATE_PART,
	FK_ROTATE_PART_CCW,
	FK_ROTATE_REF,
	FK_ROTATE_REF_CCW,
	FK_ROTATE_VALUE,
	FK_ROTATE_VALUE_CCW,
	FK_SIDE,
	FK_ROUTE,
	FK_UNROUTE,
	FK_REROUTE,
	FK_COMPLETE,
	FK_ADD_PART,
	FK_ADD_NET,
	FK_ADD_TEXT,
	FK_ADD_GRAPHIC,
	FK_REDO_RATLINES,
	FK_ADD_AREA,
	FK_DELETE_PART,
	FK_DELETE_VERTEX,
	FK_MOVE_VERTEX,
	FK_MOVE_CORNER,
	FK_ADD_CORNER,
	FK_DELETE_CORNER,
	FK_ADD_CONNECT,
	FK_DETACH_NET,
	FK_ATTACH_NET,
	FK_DELETE_CONNECT,
	FK_FORCE_VIA,
	FK_SET_WIDTH,
	FK_LOCK_CONNECT,
	FK_UNLOCK_CONNECT,
	FK_MOVE_TEXT,
	FK_ROTATE_TEXT,
	FK_DELETE_TEXT,
	FK_POLY_STRAIGHT,
	FK_POLY_ARC_CW,
	FK_POLY_ARC_CCW,
	FK_EDIT_PART,
	FK_EDIT_FOOTPRINT,
	FK_GLUE_PART,
	FK_UNGLUE_PART,
	FK_UNDO,
	FK_SET_SIZE,
	FK_SET_PARAMS,
	FK_START_STUB,
	FK_EDIT_TEXT,
	FK_SET_POSITION,
	FK_DELETE_OUTLINE,
	FK_DELETE_AREA,
	FK_DELETE_CUTOUT,
	FK_ADD_SEGMENT,
	FK_ADD_VIA,
	FK_DELETE_VIA,
	FK_DELETE_SEGMENT,
	FK_UNROUTE_TRACE,
	FK_UNROUTE_OF_VIAS,
	FK_SELECT_IN_LAYER,
	FK_CHANGE_PIN,
	FK_AREA_CUTOUT,
	FK_CHANGE_LAYER,
	FK_EDIT_NET,
	FK_MOVE_GROUP,
	FK_DELETE_GROUP,
	FK_ROTATE_GROUP,
	FK_ROTATE_GROUP_1,
	FK_ROTATE_GROUP_5,
	FK_ROTATE_GROUP_45,
	FK_ROTATE_GROUP_90,
	FK_ROTATE_GROUP__1,
	FK_ROTATE_GROUP__5,
	FK_ROTATE_GROUP__45,
	FK_ROTATE_GROUP__90,
	FK_VIA_SIZE,
	FK_ADD_VERTEX,
	FK_SIDE_STYLE,
	FK_EDIT_AREA,
	FK_MOVE_SEGMENT,
	//add 08-04-2016
	FK_BRANCH_TO_VIA,
	FK_BRANCH_TO_SEG,
	FK_DISABLE_BRANCH,
	FK_INSERT_SEGMENT,
	FK_ALIGN_SEGMENTS,
	FK_BACK_WIDTH,
	FK_NEXT_WIDTH,
	FK_AS_PAD,
	FK_INSERT_VERTICES,
	FK_MERGE_GROUP,
	FK_TAKE_APART,
	FK_SELECT_CONTOUR,
	FK_TEST_ON_CONTACT,
	FK_DEL_WITH_MERGE,
	FK_SEL_BETWEEN,
	FK_RADIUS_DOWN,
	FK_RADIUS_UP,
	FK_SET_CLEARANCE,
	//add 25-08-2016
	FK_ALIGN_SIDES,
	FK_ALIGN_X,
	FK_ALIGN_Y,
	FK_ALIGN_MIDDLE,        
	FK_ALIGN_MIDLINE,
	FK_ALIGN_MIDDLE_X,
	FK_ALIGN_MIDDLE_Y,
	FK_CLEARANCE_DOWN,
	FK_CLEARANCE_UP,
	FK_OK,
	FK_COPY_TRACES,
	FK_START_CUTOUT,
	FK_START_SIMILAR,
	FK_DELETE_SIDES,
	FK_REF_AUTO_LOC,
	FK_PARTS_AUTO_LOC,
	FK_REFS_SIZE,
	FK_VALUES_SIZE,
	FK_EDIT_SILK,
	FK_SET_LINES_VIS,
	FK_SET_LINES_INVIS,
	FK_REPOUR,
	FK_HATCH_STYLE,
	FK_CHECK_TRACES,
	FK_ADD_LINE,
	FK_CANCEL_HILITE,
	FK_GRID_STYLE,
	FK_SHOW_M,
	FK_SPLIT_NET,
	FK_VALUES_VIS,
	FK_VALUES_INVIS,
	FK_INSIDE_POLYLINE,
	//
	FK_NUM_OPTIONS,
	FK_ARROW
};

// function key menu strings
const char fk_str[FK_NUM_OPTIONS*2+2][32] =     
{ 
	"",			"",
	" Move",	" Part",
	" Move",	" Ref Text",
	" Move",	" Value",
	" Rotate",	" Part CW",
	" Rotate",	" Part CCW",
	" Rotate",	" CW",
	" Rotate",	" CCW",
	" Rotate",	" CW",
	" Rotate",	" CCW",
	" Change",	" Side",
	" Route",	" Segment",
	" Unroute",	" Segment",
	" Reroute",	" Segment",
	" Complete"," Segment",
	" Add",		" Part",
	" Add",		" Net",
	" Add",		" Text",
	" Add",		" Graphics",
	" Recalc.",	" Ratlines",
	" Add",		" Area",
	" Delete",  " Part",
	" Delete",  " Vertex",
	" Move",	" Vertex",
	" Move",	" Corner",
	" Add",		" Corner",
	" Delete",	" Corner",
	" Connect",	" Pin",
	" Detach",	" Net",
	" Set",		" Net",
	" Delete",	" Connect",
	" Force",	" Via",
	" Set",		" Width",
	" Lock",	" Connect",
	" Unlock",	" Connect",
	" Move",	" Text",
	" Rotate",	" Text",
	" Delete",	" Text",
	" Straight"," Line",
	" Arc",		" (CW)",
	" Arc",		" (CCW)",
	" Edit",	" Part",
	" Edit",	" Footprint",
	" Glue",	" Part",
	" Unglue",	" Part",
	" Undo",	"",
	" Set",		" Size",
	" Set",		" Params",
	" Start",	" Stub",
	" Edit",	" Text",
	" Set",		" Position",
	" Delete",	" Outline",
	" Delete",	" Area",
	" Delete",	" Cutout",
	" Add",		" Segment",
	" Add",		" Via",
	" Delete",	" Via",
	" Delete",	" Segment",
	" Unroute",	" Trace",
	" Unroute",	" Of Vias",
	" Select",  " In layer",
	" Change",	" Pin",
	" Add",		" Cutout",
	" Change",	" Layer",
	" Edit",	" Net",
	" Move",	" Group",
	" Delete",	" Group",
	" Rotation"," Mode",
	" Rotate",	"+1 Deg..",
	" Rotate",	"+5 Deg..",
	" Rotate",	"+45 Deg..",
	" Rotate",	"+90 Deg..",
	" Rotate",	"-1 Deg..",
	" Rotate",	"-5 Deg..",
	" Rotate",	"-45 Deg..",
	" Rotate",	"-90 Deg..",
	" Set",		" Via Size",
	" Add",		" Vertex",
	" Set Side"," Style",
	" Edit",	" Area",
	" Move",	" Segment",
/*-----add 08-04-2016-----*/
	" Branch",  " To Via",
	" Branch",  " To Line",
	" Disable", " Branch",
    " Insert",  " Segment",
	" Align",   " Segments",
	" Back",    " Width",
	" Next",    " Width",
	" As",      " Pad",
	" Insert",  " Vertices",
	" Merge",   " Group",
	" Delete",  " Merge",
	" Select",  " Contours",
	" Test On", " Contact",
	" Delete",  " Bridge",
	" Select",  " Between",
	" Radius",  " Less",
	" Radius",  " More",
	" Set ",    "Clearance", 
/*-----add 25-08-2016-----*/          
	" Align",   " Sides",
	" Align by"," Prev X",
	" Align by"," Prev Y",
	" Align",   " Between",
	" Middle",  " Of Line",
	" Between", " Prev X's",
	" Between", " Prev Y's",
	" Less",    "",
	" More",    "",
	" Return",  "",
	" Copy",    " Traces",
	" Start",   " Cutout",
	" Start",   " Similar..",
	" Delete",  " Sides",
	" Ref",   " AutoPos..",
	" Part",   " AutoPos..",
	" Ref",     " Size",
	" Value",   " Size",
	" Edit",    " Silk",
	" Set Line"," Visible",
	" Set Line"," InVisible",
	" Area",    " Repour",
	" Layer,",  " Hatch",
	" Check",   " Traces",
	" Repeat",  " Line",
	" Cancel",  " Highlight",
	" Grid",    " Style",
	" Show",    " Merges",
	" Split",   " Net",
	" Visible", " Values",
	" Invisible"," Values",
	" Inside",  " Polyline",
	"",	        ""
};

// selection masks
enum {	SEL_MASK_PARTS = 0,
		SEL_MASK_REF,
		SEL_MASK_PINS,
		SEL_MASK_CON,
		SEL_MASK_VIA,
		SEL_MASK_AREAS,
		SEL_MASK_TEXT,
		SEL_MASK_OP,
		SEL_MASK_DRC,
		SEL_MASK_MERGES,
		NUM_SEL_MASKS
};

// snap modes
enum {	SM_GRID_POINTS,	// snap to grid points
		SM_GRID_LINES	// snap to grid lines
};

// selection mask menu strings
const char sel_mask_str[NUM_SEL_MASKS][32] = 
{
	"parts",
	"ref, value",
	"pins",
	"traces, ratlines",
	"vertices, vias",
	"copper areas",
	"texts",
	"polylines",
	"DRC errors",
	"merges"
};

// descriptor for undo/redo
struct undo_descriptor {
	CFreePcbView * view;	// the view class
	CUndoList * list;		// undo or redo list
	int type;				// type of operation
	CString name1, name2;	// can be used for parts, nets, etc.
	int int1, int2;			// parameter
	CString str1;			// parameter
	void * ptr;				// careful with this
};

// group descriptor
struct undo_group_descriptor {
	CFreePcbView * view;	// the view class
	CUndoList * list;		// undo or redo list
	int type;				// type of operation
	CArray<CString> str;	// array strings with names of items in group
	CArray<id> m_id;		// array of item ids
};

class CFreePcbView : public CView
{
public:
	enum {		
		// undo types
		UNDO_PART = 1,			// redo for ADD
		UNDO_PART_AND_NETS,		// redo for DELETE and MODIFY
		UNDO_2_PARTS_AND_NETS,	// redo
		UNDO_NET,				// debug flag
		UNDO_NET_AND_CONNECTIONS,	// redo for MODIFY
		UNDO_CONNECTION,		// debug flag
		UNDO_AREA,				// redo for ADD, DELETE, MODIFY
		UNDO_ALL_AREAS_IN_NET,	// redo
		UNDO_ALL_AREAS_IN_2_NETS,	// redo
		UNDO_NET_AND_CONNECTIONS_AND_AREA,	// debug flag
		UNDO_NET_AND_CONNECTIONS_AND_AREAS,	// ASSERT
		UNDO_ALL_NETS_AND_CONNECTIONS_AND_AREAS, // debug flag
		UNDO_ALL_NETS,			// debug flag
		UNDO_MOVE_ORIGIN,		// redo
		UNDO_ALL_OP,				// redo
		UNDO_TEXT,					// redo
		UNDO_GROUP,
		// lower-level
		UNDO_OP_CLEAR_ALL,
		UNDO_OP,
		UNDO_OP_ADD,
		UNDO_GROUP_MODIFY,
		UNDO_GROUP_DELETE,
		UNDO_GROUP_ADD
	};

public: // create from serialization only
	CFreePcbView();
	DECLARE_DYNCREATE(CFreePcbView)

// Attributes
public:
	CFreePcbDoc* GetDocument();

// member variables
public:
	CFreePcbDoc * m_Doc;	// the document
	CDisplayList * m_dlist;	// the display list
    BOOL g_bShow_Ratline_Warning;
	BOOL g_bShow_nl_lock_Warning;
	// Windows fonts
	CFont m_small_font;

	// cursor mode
	int m_cursor_mode;		// see enum above

	// debug flag
	int m_debug_flag;

	// routing width trace 
	int m_routing_width;

    // user clearance between segments 
	int m_seg_clearance;

	// distance of measurement
	int m_measure_dist;

	// insert seg length
	int m_insert_seg_len;

	// previous
	int prevx;
	int ppx;
	int prev_middle_x;
	//
	float prev_m_ang;

	// previous
	int prevy;
	int ppy;
	int prev_middle_y;

	// enable branch
	int en_branch;

	// flag of copy traces
	BOOL fCopyTraces;

	// flag add repour
	int fRepour;

	// page of fk_text
	int m_page;
	     
	// if merge selected 
	CString m_sel_merge_name;

	// flag to indicate that a newly-created item is being dragged,
	// as opposed to an existing item
	// if so, right-clicking or ESC will delete item not restore it
	BOOL m_dragging_new_item;

	// parameters for dragging selection rectangle
	BOOL m_bLButtonDown;
	BOOL m_bDraggingRect;
	CPoint m_start_pt;
	CRect m_drag_rect, m_last_drag_rect;
	RECT m_sel_rect;		// rectangle used for selection

	// mode for drawing new polyline segments
	int m_polyline_style;	// STRAIGHT, ARC_CW or ARC_CCW
	int m_polyline_hatch;	// NONE, DIAGONAL_FULL or DIAGONAL_EDGE
	int m_polyline_layer;	// layer being drawn
	int m_polyline_width;	//    
	int m_polyline_closed;  //

	// flag to disable context menu on right-click,
	// if right-click handled some other way
	int m_disable_context_menu;

	// selection mask
	int m_sel_mask;
	id m_mask_id[NUM_SEL_MASKS];

	// selected items
	id m_sel_id;			// id of selected item

	cpart * m_sel_part;		// pointer to part, if selected
	cnet * m_sel_net;		// pointer to net, if selected
	CText * m_sel_text;		// pointer to text, if selected
	DRError * m_sel_dre;	// pointer to DRC error, if selected
	int m_prev_sel_merge;
	int m_sel_layer;		// layer of selected item
	int m_sel_count;		// number of selected items
	int m_sel_flags;		// mask obj

	// mask selected obj
	enum{ 
		FLAG_SEL_PART = 0,
		FLAG_SEL_NET,		
		FLAG_SEL_AREA,		
		FLAG_SEL_CONNECT,	
		FLAG_SEL_TEXT,		
		FLAG_SEL_OP,		
		FLAG_SEL_DRE,
		FLAG_SEL_VTX
	};
#define PART_ONLY		1
#define NET_ONLY		14 // if areas+seg
#define AREA_ONLY		6  
#define CONNECT_ONLY	10 // if seg
#define TEXT_ONLY		16
#define OP_ONLY			32
#define DRE_ONLY		64
#define VTX_ONLY		138 

#define ID_PART_DEF		ID_PART,ID_SEL_RECT,0,0,0
#define ID_REF_DEF		ID_PART_LINES,ID_SEL_REF_TXT,0,0,0
#define ID_VALUE_DEF	ID_PART_LINES,ID_SEL_VALUE_TXT,0,0,0
#define ID_TEXT_DEF	    ID_TEXT,ID_TXT,0,0,0
	     
#define m_sel_ic m_sel_id.i							// index of selected connection
#define m_sel_ia m_sel_id.i							// index of selected area
#define m_sel_is m_sel_id.ii						// index of selected side, segment, or corner
#define m_sel_iv m_sel_id.ii						// index of selected vertex
#define m_sel_con m_sel_net->connect[m_sel_ic]		// selected connection
#define m_sel_seg m_sel_con.seg[m_sel_is]			// selected side or segment
#define m_sel_last_seg m_sel_con.seg[m_sel_is-1]		// last seg
#define m_sel_next_seg m_sel_con.seg[m_sel_is+1]		// next seg
#define m_sel_last_vtx m_sel_con.vtx[m_sel_is-1]		// last vertex
#define m_sel_vtx m_sel_con.vtx[m_sel_is]			// selected vertex
#define m_sel_next_vtx m_sel_con.vtx[m_sel_is+1]	// next vertex
#define m_sel_next_next_vtx m_sel_con.vtx[m_sel_is+2]	// next vertex after that
#define m_sel_start_pin m_sel_net->pin[m_sel_con.start_pin]
#define m_sel_end_pin m_sel_net->pin[m_sel_con.end_pin]

	// direction of routing
	int m_dir;			// 0 = forward, 1 = back

	// display coordinate mapping
	double m_pcbu_per_pixel;	// pcb units per pixel
	double m_org_x;				// x-coord of left side of screen in pcb units
	double m_org_y;				// y-coord of bottom of screen in pcb units

	// grid stuff
	CPoint m_snap_angle_ref;	// reference point for snap angle
	int m_snap_mode;			// snap mode
	int m_inflection_mode;		// inflection mode for routing

	// window parameters
	CPoint m_client_origin;	// coordinates of (0,0) in screen coords
	CRect m_client_r;		// in device coords
	int m_left_pane_w;		// width of pane at left of screen for layer selection, etc.
	int m_bottom_pane_h;	// height of pane at bottom of screen for key assignments, etc.
	CRgn m_pcb_rgn;			// region for the pcb
	BOOL m_left_pane_invalid;	// flag to erase and redraw left pane

	// active layer for routing and placement
	int m_active_layer;

	// starting point for a new copper area 
	int m_area_start_x;
	int m_area_start_y;
	
	// mouse
	CPoint m_last_mouse_point;	// last mouse position
	CPoint m_last_cursor_point;	// last cursor position (may be different from mouse)
	CPoint m_from_pt;			// for dragging rect, origin
	CPoint m_to_pt;				// for dragging segment, endpoint of this segment
	CPoint m_last_pt;			// for dragging segment
	CPoint m_next_pt;			// for dragging segment

	// function key shortcuts
	int m_fkey_option[12];
	int m_fkey_command[12];
	char m_fkey_str[24][32];

	// memory DC and bitmap
	BOOL m_botDC_created;
	BOOL m_topDC_created;
	CDC m_botDC;
	CDC m_topDC;
	CBitmap m_bitmap1;
	CBitmap m_bitmap2;
	HBITMAP m_old_bitmap1;
	HBITMAP m_old_bitmap2;
	CRect m_bitmap_rect1;
	CRect m_bitmap_rect2;
	int m_draw_layer;

// Operations
public:
	void InitInstance();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFreePcbView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CFreePcbView();
	void InitializeView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	
	void SetMainMenu( BOOL bAll );
	int SetDCToWorldCoords( CDC * pDC );
	void SetCursorMode( int mode );
	void SetFKText( int mode );  
	void DrawBottomPane();    
	int ShowCursor();
	int ShowSelectStatus();
	void ShowRelativeDistance( int dx, int dy );
	void ShowRelativeDistance( int x, int y, int dx, int dy );
	int ShowActiveLayer(int n_layers, BOOL swCASE=0);
	int SelectPart( cpart * part, BOOL bPins=TRUE );
	void CancelSelection( BOOL hDialog=TRUE );
	int SetWidth( int mode );
	int GetWidthsForSegment( int * w, int * via_w, int * via_hole_w, int EX=-1 );
	void ChangeTraceLayer( int mode, int old_layer=0 );
	void SetSelMaskArray( int mask );
	void MoveSegment (cnet * sel_net, int sel_ic, int sel_is, int dx, int dy, BOOL b_45=0 );
	int  InsertSegment( cnet * sel_n, int con, int seg, BOOL FULL_LENGTH );
	int SegmentMovable();
	void SnapCursorPoint( CPoint wp, UINT nFlags );
	void InvalidateLeftPane(){ m_left_pane_invalid = TRUE; }
		void OnExternalChangeFootprint( CShape * fp );
	void HandleKeyPress(UINT nChar, UINT nRepCnt, UINT nFlags);
	void CFreePcbView::TryToReselectAreaCorner( int x, int y );
	void ReselectNetItemIfConnectionsChanged( int new_ic );
	void OnRefShowPart( cpart * p, int highlight );
	void SelectSimilarParts( BOOL includeValues );
	void SelectMergeSegments( cpart * mP );
	void VertexMoved();
	void ShowPinState(cpart * p, int pin);

	// for group
	BOOL GetSelectedItem();	
	BOOL ThisGroupContainsGluedParts();
	void MarkAllOutlinePoly( int utility, int layer );
	void MoveGroup( int dx, int dy, BOOL unroute );
	void RotateGroup( int angle, BOOL unroute );
	void RotateGroup( int angle );
	void MergeGroup();
	void MergeGroup(int merge0);
	void ExplodeGroup();	
	int  NewSelect( void * ptr, id * sid, BOOL bSet_cursor_mode, BOOL bInvert );// Set sel element 
	int  UnSelect( void * ptr, id * sid, BOOL bSET_CURSOR_MODE=0 );				// Unselect element														
	CString NewSelectM( cpart * ptr, int number=-1 );								// M-elements selection
	void SelectItemsInRect( CRect r, UINT nFlags );
	void TurnGroup ();
	void MoveOrigin( int x_off, int y_off );
	void DeleteGroup( BOOL wMerge );
	void FindGroupCenter();
	void HighlightGroup(BOOL bPins=TRUE);
	void StartDraggingGroup( BOOL bAdd=FALSE, int x=0, int y=0 );
	void CancelDraggingGroup();
	void OnInsidePolyline();
	void UnselectGluedPartsInGroup();
	void SelectGluedParts();
	BOOL GluedPartsInGroup();
	BOOL TestSelElements(int mode);
	//
	enum	{
		FOR_FK_SET_CLEARANCE = 0,
		FOR_FK_APPROXIM_ARC,
	    FOR_FK_RADIUS_UP_DOWN,
		FOR_FK_INSERT_SEGMENT,
		FOR_FK_CONVERT_TO_LINE,
		FOR_FK_SELECT_BETWEEN,
		FOR_FK_ALIGN_SEGMENTS,
		FOR_FK_PARTS_AUTO_LOC
	};
	void SelectBetween();
	void SelectContour();
	int DeleteWithRecalculationPoint (CPolyLine * p, cconnect * c, BOOL WITH_MERGE);
	int GetSelCount();
	cnet * FindNEXTSegmentOfGroup ( float angle, id * _id, cnet * sel_net, id * f_id, cnet * f_net );
	BOOL find_side_for_set_clearance( CPolyLine * pp, id * _id, int Y, float angle, int min_len );
	void SetUpReferences();
	void SetUpParts ();
	void OnSetClearance();
	void ApproximArc();
	int  ArcApp( CPolyLine * po, int forArcOnly, cnet * m_net, int m_i );
	void TracesRadiusUpDown(BOOL UP);
	void AlignSegments( cnet * n, int ic, int iv, BOOL mirror, float ang=0 );
	void AlignSides( int type, int ic, int iv);
	void OnGroupPaste( BOOL bwDialog, BOOL bSaveMerges=0 );
	void OnInfoBoxMess( int command, int n_str, CArray<CString> *str );
	CWnd * OnInfoBoxSendMess( CString mess );

	
	// check mode
	BOOL CurNone();
	BOOL CurSelected();
	BOOL CurDragging();
	BOOL CurDraggingRouting();
	BOOL CurDraggingPlacement();
	
	// undo
	void SaveUndoInfoForPart( cpart * part, int type, CString * new_ref_des, BOOL new_event, CUndoList * list );
	void SaveUndoInfoForPartAndNets( cpart * part, int type, CString * new_ref_des, BOOL new_event, CUndoList * list );
	void SaveUndoInfoFor2PartsAndNets( cpart * part1, cpart * part2, BOOL new_event, CUndoList * list );
	void SaveUndoInfoForNet( cnet * net, int type, BOOL new_event, CUndoList * list );
	void SaveUndoInfoForNetAndConnections( cnet * net, int type, BOOL new_event, CUndoList * list );
	void SaveUndoInfoForConnection( cnet * net, int ic, BOOL new_event, CUndoList * list );
	void SaveUndoInfoForArea( cnet * net, int iarea, int type, BOOL new_event, CUndoList * list );
	void SaveUndoInfoForAllAreasInNet( cnet * net, BOOL new_event, CUndoList * list );
	void SaveUndoInfoForAllAreasIn2Nets( cnet * net1, cnet * net2, BOOL new_event, CUndoList * list );
	void SaveUndoInfoForNetAndConnectionsAndArea( cnet * net, int iarea, int type, BOOL new_event, CUndoList * list );
	void SaveUndoInfoForNetAndConnectionsAndAreas( cnet * net, CUndoList * list, int UNDO_AREA=CNetList::UNDO_AREA_MODIFY );
	void SaveUndoInfoForAllNets( BOOL new_event, CUndoList * list );
	void SaveUndoInfoForMoveOrigin( int x_off, int y_off, CUndoList * list );
	void SaveUndoInfoForOutlinePoly( int type, BOOL new_event, CUndoList * list );
	void SaveUndoInfoForText( CText * text, int type, BOOL new_event, CUndoList * list );
	void SaveUndoInfoForText( undo_text * u_text, int type, BOOL new_event, CUndoList * list );
	int SaveUndoInfoForGroup( int type, CUndoList * list, BOOL wMerge=0, BOOL bMessageBox=1 );
	void *  CreateUndoDescriptor( CUndoList * list, int type,
		CString * name1, CString * name2, int int1, int int2, CString * str1, void * ptr );
	static void UndoCallback( int type, void * ptr, BOOL undo );
	static void UndoGroupCallback( int type, void * ptr, BOOL undo );

	
protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CFreePcbView)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point );
	////afx_msg void OnRangeCmds(UINT nID);
	afx_msg void OnPartMove();
	afx_msg void OnTextAdd();
	afx_msg void OnTextDelete();
	afx_msg void OnTextMove();
	afx_msg void OnPartGlue();
	afx_msg void OnPartUnglue();
	afx_msg void OnPartDelete();
	void DeletePart();
	afx_msg void OnPartOptimize();
	afx_msg void OnPartRemoveMerge();
	afx_msg void OnSelectSimilarParts();
	afx_msg void OnSelectSimilarPackages();
	afx_msg void OnRefMove();
	afx_msg void OnPadOptimize();
	afx_msg void OnPadAddToNet();
	afx_msg void OnPadDetachFromNet();
	afx_msg void OnPadConnectToPin();
	afx_msg void OnSegmentSetWidth();
	afx_msg void OnSegmentUnroute();
	afx_msg void OnRatlineRoute();   
	afx_msg void OnRatlineOptimize();
	afx_msg void OnVertexMove();
	afx_msg void OnVertexConnectToPin();
	afx_msg void OnVertexSize();
	afx_msg void OnVertexDelete();
	afx_msg void OnRatlineComplete();
	afx_msg void OnRatlineSetWidth();
	afx_msg void OnRatlineDeleteConnection();
	afx_msg void OnRatlineLockConnection();
	afx_msg void OnRatlineUnlockConnection();
	afx_msg void OnRatlineChangeEndPin();
	afx_msg void OnTextEdit();
	afx_msg void OnAddBoardOutline();
	void AddBoardOutline();
	afx_msg void OnAddSMCutout();
	void AddSMCutout();
	afx_msg void OnAddOutlinePoly();
	void AddOutlinePoly( BOOL bREPEAT, BOOL bEDIT=0 );
	afx_msg void OnOPHatchStyle();
	void OPHatchStyle();
	afx_msg void OnOPSetWidth();
	int  OPSetWidth();
	afx_msg void OnOPCornerMove();
	afx_msg void OnOPCornerEdit();
	afx_msg void OnOPCornerDelete();
	afx_msg void OnOPSideAddCorner();
	afx_msg void OnOPDeleteOutline();
	afx_msg void OnOPSideConvertToStraightLine();
	afx_msg void OnOPSideConvertToArcCw();
	afx_msg void OnOPSideConvertToArcCcw();
	afx_msg void OnPadStartStubTrace();
	afx_msg void OnSegmentDelete();
	afx_msg void OnEndVertexMove();
	afx_msg void OnEndVertexAddSegments();
	afx_msg void OnEndVertexAddConnection();
	afx_msg void OnEndVertexDelete();
	afx_msg void OnEndVertexEdit();
	afx_msg void OnAreaCornerMove();
	afx_msg void OnAreaCornerDelete();
	afx_msg void OnAreaCornerDeleteArea();
	afx_msg void OnAreaSideAddCorner();
	afx_msg void OnAreaSideDeleteArea();
	void AreaSideDeleteArea();
	afx_msg void OnAddArea();
	void AddArea();
	afx_msg void OnAreaAddCutout();
	void AreaAddCutout();
	afx_msg void OnAreaDeleteCutout();
	void AreaDeleteCutout();
	afx_msg void OnEndVertexAddVia();
	afx_msg void OnEndVertexRemoveVia();
	afx_msg void OnSegmentDeleteTrace();
	afx_msg void OnAreaCornerProperties();
	afx_msg void OnVertexProperties();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnUnrouteTrace();
	afx_msg void OnViewEntireBoard();
	void ViewEntireBoard();
	afx_msg void OnViewAllElements();
	RECT ViewAllElements();
	afx_msg void OnAreaEdgeHatchStyle();
	afx_msg void OnPartEditFootprint();
	afx_msg void OnPartEditThisFootprint();
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnViewFindpart();
	afx_msg void OnFootprintWizard();
	afx_msg void OnFootprintEditor();
	afx_msg void OnCheckPartsAndNets();
	afx_msg void OnDrc();
	afx_msg void OnClearDRC();
	afx_msg void OnViewAll();
	afx_msg void OnPartChangeSide();
	afx_msg void OnPartRotate();
	afx_msg void OnNetSetWidth();
	afx_msg void OnConnectSetWidth();
	afx_msg void OnConnectUnroutetrace();
	afx_msg void OnConnectDeletetrace();
	afx_msg void OnSegmentChangeLayer();
	afx_msg void OnConnectChangeLayer();
	afx_msg void OnNetChangeLayer();
	afx_msg void OnNetEditnet(); 
	void Editnet( int i_pin=-1 ); 
	afx_msg void OnToolsMoveOrigin();
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnGroupMove();
	afx_msg void OnAddSimilarArea();
	void AddSimilarArea();
	afx_msg void OnSegmentAddVertex();
	LONG OnChangeVisibleGrid( UINT wp, LONG lp );
	LONG OnChangePlacementGrid( UINT wp, LONG lp );
	LONG OnChangeRoutingGrid( UINT wp, LONG lp );
	LONG OnChangeSnapAngle( UINT wp, LONG lp );
	LONG OnChangeUnits( UINT wp, LONG lp );
	afx_msg void OnAreaEdit();
	afx_msg void OnAreaEdgeApplyClearances();
	afx_msg void OnGroupSaveToFile();
	afx_msg void OnGroupStaticHighlight();
	afx_msg void OnGroupCancelHighlight();
	void GroupCancelHighlight();
	afx_msg void OnApproximationArc();
	afx_msg void OnGroupAlignParts();
	afx_msg void OnGroupCopy();
	afx_msg void OnGroupCut();
	afx_msg void OnGroupDelete();
	afx_msg void OnEditCopy();
	afx_msg void OnEditPaste();
	afx_msg void OnEditCut();
	afx_msg void OnGroupRotate();
	afx_msg void OnAreaSideStyle();
	afx_msg void OnValueMove();
	afx_msg void OnValueProperties();
	afx_msg void OnRefProperties();
	void RefProperties();
	afx_msg void OnValueShowPart();
	afx_msg void OnRefShowPart();
	afx_msg void OnPartEditValue();
	afx_msg void OnPartRotateCCW();
	afx_msg void OnRefRotateCW();
	afx_msg void OnRefRotateCCW();
	afx_msg void OnValueRotateCW();
	afx_msg void OnValueRotateCCW();
	afx_msg void OnSegmentMove();
	afx_msg void OnProjectSelectArcElements();
	afx_msg void OnProjectSelectViaElements();
	afx_msg void OnProjectRunInfoBox();
	afx_msg void OnSetOriginToSelectedItem();
	void SetOriginToSelectedItem();
	void ProjectRunInfoBox();
};

#ifndef _DEBUG  // debug version in FreePcbView.cpp
inline CFreePcbDoc* CFreePcbView::GetDocument()
   { return (CFreePcbDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FREEPCBVIEW_H__BE1CA173_E2B9_4252_8422_0B9767B01566__INCLUDED_)
