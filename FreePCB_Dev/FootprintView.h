// FootprintView.h : interface of the CFootprintView class
//
/////////////////////////////////////////////////////////////////////////////


#if !defined(AFX_FOOTPRINTVIEW_H__BE1CA173_E2B9_4252_8422_0B9767B01566__INCLUDED_)
#define AFX_FOOTPRINTVIEW_H__BE1CA173_E2B9_4252_8422_0B9767B01566__INCLUDED_

#pragma once
#include "stdafx.h"
#include <afxcoll.h>
#include <afxtempl.h>
#include "DisplayList.h"
#include "FreePcbDoc.h"

class CFootprintView;

// cursor modes
enum {
	CUR_FP_NONE_SELECTED = 0,	// nothing selected
	CUR_FP_PAD_SELECTED,		// pad selected
	CUR_FP_REF_SELECTED,		// ref selected
	CUR_FP_VALUE_SELECTED,		// value selected
	CUR_FP_POLY_CORNER_SELECTED, // outline poly corner selected
	CUR_FP_POLY_SIDE_SELECTED,	// outline poly side selected
	CUR_FP_TEXT_SELECTED,		// text string selected
	CUR_FP_CENTROID_SELECTED,	// centroid
	CUR_FP_ADHESIVE_SELECTED,	// glue spot
	CUR_FP_GROUP_SELECTED,
	CUR_FP_NUM_SELECTED_MODES,	// number of selection modes
	CUR_FP_DRAG_PAD,			// dragging pad to move it
	CUR_FP_DRAG_GROUP,
	CUR_FP_DRAG_REF,			// dragging ref text to move it
	CUR_FP_DRAG_VALUE,			// dragging value text to move it
	CUR_FP_ADD_POLY,			// dragging first corner of new poly
	CUR_FP_DRAG_POLY_1,			// dragging second corner of new poly
	CUR_FP_DRAG_POLY,			// dragging next corner of new poly
	CUR_FP_DRAG_POLY_MOVE,		// dragging corner to move it
	CUR_FP_DRAG_POLY_INSERT,	// dragging corner to insert it
	CUR_FP_DRAG_TEXT,			// dragging text to move it
	CUR_FP_MOVE_ORIGIN,			// dragging origin
	CUR_FP_DRAG_CENTROID,		// dragging centroid
	CUR_FP_DRAG_ADHESIVE,		// dragging glue spot
	CUR_FP_NUM_MODES			// number of cursor modes
};

// function key options
enum {
	FK_FP_NONE = 0,
	FK_FP_MOVE_PAD,
	FK_FP_MOVE_REF,
	FK_FP_ROTATE_PAD_CW,
	FK_FP_ROTATE_PAD_CCW,
	FK_FP_ROTATE_REF,
	FK_FP_SIDE,
	FK_FP_ROUTE,
	FK_FP_UNROUTE,
	FK_FP_REROUTE,
	FK_FP_COMPLETE,
	FK_FP_ADD_PAD,
	FK_FP_ADD_NET,
	FK_FP_ADD_TEXT,
	FK_FP_ADD_POLYLINE,
	FK_FP_REDO_RATLINES,
	FK_FP_ADD_AREA,
	FK_FP_DELETE_PAD,
	FK_FP_DELETE_VERTEX,
	FK_FP_MOVE_VERTEX,
	FK_FP_MOVE_CORNER,
	FK_FP_ADD_CORNER,
	FK_FP_DELETE_CORNER,
	FK_FP_ADD_CONNECT,
	FK_FP_DETACH_NET,
	FK_FP_ATTACH_NET,
	FK_FP_DELETE_CONNECT,
	FK_FP_FORCE_VIA,
	FK_FP_SET_WIDTH,
	FK_FP_LOCK_CONNECT,
	FK_FP_UNLOCK_CONNECT,
	FK_FP_MOVE_TEXT,
	FK_FP_ROTATE_TEXT,
	FK_FP_DELETE_TEXT,
	FK_FP_POLY_STRAIGHT,
	FK_FP_POLY_ARC_CW,
	FK_FP_POLY_ARC_CCW,
	FK_FP_EDIT_PAD,
	FK_FP_GLUE_PART,
	FK_FP_UNGLUE_PART,
	FK_FP_UNDO,
	FK_FP_EDIT_PROPERTIES,
	FK_FP_START_STUB,
	FK_FP_EDIT_TEXT,
	FK_FP_SET_POSITION,
	FK_FP_VISIBLE,
	FK_FP_INVISIBLE,
	FK_FP_DELETE_POLYLINE,
	FK_FP_EDIT_CENTROID,
	FK_FP_MOVE_CENTROID,
	FK_FP_MOVE_VALUE,
	FK_FP_ROTATE_VALUE,
	FK_FP_CLOSE,
	FK_FP_ROTATE_CENTROID,
	FK_FP_EDIT_ADHESIVE,
	FK_FP_MOVE_ADHESIVE,
	FK_FP_DELETE_ADHESIVE,
	FK_FP_ALIGN,
	FK_FP_ALIGN_X,
	FK_FP_ALIGN_Y,
	FK_FP_SET_ORIGIN,
	FK_FP_CHANGE_HATCH,
	// for group
	FK_FP_DELETE_GROUP,
	FK_FP_MIRROR_GROUP,
	FK_FP_DUPLICATE_GROUP,
	FK_FP_ROTATE_GROUP,
	FK_FP_ROTATE_1,
	FK_FP_ROTATE__1,
	FK_FP_ROTATE_45,
	FK_FP_ROTATE__45,
	FK_FP_MOVE_GROUP,
	//
	FK_FP_SELECT_SIMILAR,
	FK_FP_OP_MAKE_COPPER,
	FK_FP_OP_MAKE_SILK,
	FK_FP_OP_MAKE_NOTES,
	FK_FP_NUM_OPTIONS
};

// function key menu strings
static char fk_fp_str[FK_FP_NUM_OPTIONS*2+2][32] = 
{ 
	"",			"",
	" Move",	" Pad",
	" Move",	" Ref Text",
	" Rotate",	" CW",
	" Rotate",	" CCW",
	" Rotate",	" Ref Text",
	" Change",	" Side",
	" Route",	" Segment",
	" Unroute",	" Segment",
	" Reroute",	" Segment",
	" Complete"," Segment",
	" Add",		" Pin",
	" Add",		" Net",
	" Add",		" Text",
	" Add",		" Polyline",
	" Recalc.",	" Ratlines",
	" Add",		" Area",
	" Delete",  " Pad",
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
	" Edit",	" Pad",
	" Glue",	" Part",
	" Unglue",	" Part",
	" Undo",	"",
	" Edit",	" Params",
	" Start",	" Stub",
	" Edit",	" Text",
	" Set",		" Position",
	" Show",	" Line",
	" Hide",	" Line",
	" Delete",	" Polyline",
	" Edit",	" Centroid",
	" Move",	" Centroid",
	" Move",	" Value",
	" Rotate",	" Value",
	" Return",	" to PCB",
	" Rotate",	" Axis",
	" Edit",	" Adhesive",
	" Move",	" Adhesive",
	" Delete",	" Adhesive",
	" Align",   " Sides",
	" Align",   " Prev X",
	" Align",   " Prev Y",
	" Set",     " Origin",
	" Change",  " Hatch",
	" Delete",  " Group",
	" Mirror",  " Group",
	" Duplicate",  " Group",
	" Rotation",  " Mode",
	" Rotate",  " +1 Deg..",
	" Rotate",  " -1 Deg..",
	" Rotate",  " +45 Deg..",
	" Rotate",  " -45 Deg..",
	" Move",    " Group",
	" Select",  " Similar",
	" Move to", " Copper",
	" Move to", " Silk",
	" Move to", " Notes",
	" ****",	" ****"
};

class CFootprintView : public CView
{
public: // create from serialization only
	CFootprintView();
	DECLARE_DYNCREATE(CFootprintView)

// Attributes
public:
	CFreePcbDoc* GetDocument();

// member variables
public:
	CFreePcbDoc * m_Doc;	// the document
	CDisplayList * m_dlist;	// the display list

	// Windows fonts
	CFont m_small_font;

	// cursor mode
	int m_cursor_mode;		// see enum above       

	// debug flag
	int m_debug_flag;
	
	// previous
	int ppx;
	int ppy;
	int prevx;
	int prevy;

	// for group
	POINT m_start_point;
	int m_dx;
	int m_dy;

	// flag to indicate that a newly-created item is being dragged,
	// as opposed to an existing item
	// if so, right-clicking or ESC will delete item not restore it
	BOOL m_dragging_new_item;
	int m_drag_num_pads;

	// mode for drawing new polyline segments
	BOOL m_polyline_closed_flag;
	int m_polyline_style;	// STRAIGHT, ARC_CW or ARC_CCW
	int m_polyline_width;

	// flag to disable context menu on right-click,
	// if right-click handled some other way
	int m_disable_context_menu;

	// selected item
	id m_sel_id;		// id of selected item
	int m_sel_layer;	// layer of selected item
	CText * m_sel_text;	// pointer to selected text

	// display coordinate mapping
	double m_pcbu_per_pixel;	// pcb units per pixel
	double m_org_x;				// x-coord of left side of screen in pcb units
	double m_org_y;				// y-coord of bottom of screen in pcb units

	// grids
	CPoint m_snap_angle_ref;		// reference point for snap angle

	// window parameters
	CRect m_client_r;	// in device coords
	int m_left_pane_w;		// width of pane at left of screen for layer selection, etc.
	int m_bottom_pane_h;	// height of pane at bottom of screen for key assignments, etc.
	CRgn m_pcb_rgn;		// region for the pcb
	BOOL m_left_pane_invalid;	// flag to erase and redraw left pane

	// mouse
	CPoint m_last_mouse_point;	// last mouse position
	CPoint m_last_cursor_point;	// last cursor position (may be different from mouse)

	// parameters for dragging selection rectangle
	BOOL m_bLButtonDown;
	BOOL m_bDraggingRect;
	CPoint m_start_pt;
	CRect m_drag_rect, m_last_drag_rect;
	RECT m_sel_rect;		// rectangle used for selection

	// function key shortcuts
	int m_fkey_option[12];
	int m_fkey_command[12];
	char m_fkey_str[24][32];

	// memory DC and bitmap
	BOOL m_memDC_created;
	CDC m_memDC;
	CBitmap m_bitmap;
	CBitmap * m_old_bitmap;
	CRect m_bitmap_rect;

	// footprint
	CShape m_fp;	// footprint being edited

	// units (mil or mm)
	int m_units;

	// active copper layer
	int m_active_layer;

	// undo stack
	CArray<CShape*> undo_stack;
	CArray<CShape*> redo_stack;

// Operations
public:
	void InitInstance( CShape * fp );

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFootprintView)
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
	virtual ~CFootprintView();
	void InitializeView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	int SetDCToWorldCoords( CDC * pDC );
	CPoint ScreenToPCB( CPoint point );
	CPoint PCBToScreen( CPoint point );
	CPoint WindowToPCB( CPoint point );
	void SetCursorMode( int mode );
	void SetFKText( int mode );
	void HandleKeyPress(UINT nChar, UINT nRepCnt, UINT nFlags);
	void DrawBottomPane();
	int ShowCursor();
	int ShowSelectStatus();
	int ShowActiveLayer();
	void SetWindowTitle( CString * str );
	void CancelSelection();
	//int SetWidth( int mode );
	int GetWidthsForSegment( int * w, int * via_w, int * via_hole_w );
	BOOL CurNone();
	BOOL CurSelected();
	BOOL CurDragging();
	BOOL CurDraggingPlacement();
	void SnapCursorPoint( CPoint wp );
	void InvalidateLeftPane(){ m_left_pane_invalid = TRUE; }
	void FootprintModified( BOOL flag, BOOL force = FALSE, BOOL clear_redo=TRUE );
	void FootprintNameChanged( CString * str );
	void MoveOrigin( int x, int y );
	void ClearUndo();
	void ClearRedo();
	void PushUndo();
	void PushRedo();
	void Undo();
	void UndoNoRedo();
	void Redo();
	void EnableUndo( BOOL bEnable );
	void EnableRedo( BOOL bEnable );
	void SelectItemsInRect( CRect r );
	void HighlightGroup();
	void GroupDelete();
	void MoveGroup( int dx, int dy );
	void RotateGroup( int ang );
	void MirrorGroup();
	void DuplicateGroup();
	void SelectSimilar();
	void MoveToLayer( int layer );
protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CFootprintView)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
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
//	afx_msg void OnSysChar(UINT nChar, UINT nRepCnt, UINT nFlags);
//	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point );
	afx_msg void OnPadMove( int i, int num = 1 );
	afx_msg void OnPadEdit( int i );
	afx_msg void OnPadDelete( int i );
	afx_msg void OnRefMove();
	afx_msg void OnGroupMove();
	afx_msg void OnPolylineCornerMove();
	afx_msg void OnPolylineCornerEdit();
	afx_msg void OnPolylineCornerDelete();
	afx_msg void OnPolylineSideAddCorner();
	afx_msg void OnPolylineDelete();
	void PolylineDelete();
	LONG OnChangeVisibleGrid( UINT wp, LONG lp );
	LONG OnChangePlacementGrid( UINT wp, LONG lp );
	LONG OnChangeSnapAngle( UINT wp, LONG lp );
	LONG OnChangeUnits( UINT wp, LONG lp );
	afx_msg void OnRefProperties();
	void RefProperties();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPolylineSideConvertToStraightLine();
	afx_msg void OnPolylineSideConvertToArcCw();
	afx_msg void OnPolylineSideConvertToArcCcw();
	afx_msg void OnAddPin();       
	afx_msg void OnFootprintFileSaveAs();
	afx_msg void OnAddPolyline();
	void AddPolyline( id * m_id=NULL );
	afx_msg void OnFootprintFileImport();
	afx_msg void OnFootprintFileClose();
	afx_msg void OnFootprintFileNew();
	afx_msg void OnViewEntireFootprint();
	afx_msg void OnEditUndo();
	afx_msg void OnFpMove();
	afx_msg void OnFpEditproperties();
	afx_msg void OnFpDelete();
	afx_msg void OnFpToolsFootprintwizard();
	afx_msg void OnToolsFootprintLibraryManager();
	afx_msg void OnAddText();
	afx_msg void OnFpTextEdit();
	afx_msg void OnFpTextMove();
	afx_msg void OnFpTextDelete();
	afx_msg void OnToolsMoveOriginFP();
	afx_msg void OnEditRedo();
	afx_msg void OnCentroidEdit();
	afx_msg void OnCentroidMove();
	afx_msg void OnAddSlot();
	afx_msg void OnAddHole();
	afx_msg void OnAddValueText();
	afx_msg void OnValueEdit();
	afx_msg void OnValueMove();
	afx_msg void OnAddAdhesive();
	afx_msg void OnAdhesiveEdit();
	afx_msg void OnAdhesiveMove();
	afx_msg void OnAdhesiveDrag();
	afx_msg void OnAdhesiveDelete();
	afx_msg void OnCentroidRotateAxis();
};

#ifndef _DEBUG  // debug version
inline CFreePcbDoc* CFootprintView::GetDocument()
   { return (CFreePcbDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FREEPCBVIEW_H__BE1CA173_E2B9_4252_8422_0B9767B01566__INCLUDED_)
