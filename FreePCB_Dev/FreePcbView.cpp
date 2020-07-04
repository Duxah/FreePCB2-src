// FreePcbView.cpp : implementation of the CFreePcbView class
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
#include "DlgAddText.h"
#include "DlgAssignNet.h"
#include "DlgSetSegmentWidth.h"
#include "DlgEditBoardCorner.h"
#include "DlgAddArea.h"
#include "DlgRefText.h"
#include "MyToolBar.h"
#include <Mmsystem.h>
#include <sys/timeb.h>
#include <time.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include "freepcbview.h"
#include "DlgAddPart.h"
#include "DlgAddMerge.h"
#include "DlgSetAreaHatch.h"
#include "DlgDupFootprintName.h" 
#include "DlgFindPart.h"
#include "DlgAddMaskCutout.h"
#include "DlgChangeLayer.h"
#include "DlgEditNet.h"
#include "DlgMoveOrigin.h"
#include "DlgMyMessageBox.h"
#include "DlgVia.h"
#include "DlgAreaLayer.h"
#include "DlgGroupPaste.h"
#include "DlgSideStyle.h"
#include "DlgAddPoly.h"
#include "DlgValueText.h"
#include "FreePcbDoc.h"
#include "RTcall.h"

// globals
extern CFreePcbApp theApp;
BOOL t_pressed = FALSE;
BOOL n_pressed = FALSE;
int gTotalArrowMoveX = 0;
int gTotalArrowMoveY = 0;
BOOL gShiftKeyDown = FALSE;
BOOL gLastKeyWasArrow = FALSE;
int prev_sel_count = 0;
long long groupAverageX=0, groupAverageY=0;
int groupNumberItems=0;

HCURSOR my_cursor = LoadCursor( NULL, IDC_CROSS );

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define ZOOM_RATIO 1.4

// constants for function key menu
#define FKEY_OFFSET_X 110
#define FKEY_OFFSET_Y 4
#define	FKEY_R_W 70
#define FKEY_R_H 30
#define FKEY_STEP (FKEY_R_W+5)
#define FKEY_GAP 20
#define FKEY_SEP_W 16

// constants for drag stub mode
#define DISABLE_BRANCH		0
#define BRANCH_TO_VERTEX	1
#define BRANCH_TO_LINE		2
#define mod_active_layer		 (m_active_layer<LAY_TOP_COPPER?m_active_layer+2:m_active_layer)

// constants for layer list
#define VSTEP 14

// macro for approximating angles to 1 degree accuracy
#define APPROX(angle,ref) ((angle > ref-M_PI/360) && (angle < ref+M_PI/360))

// these must be changed if context menu is edited
enum {
	CONTEXT_NONE = 0,
	CONTEXT_PART,
	CONTEXT_REF_TEXT,
	CONTEXT_PAD,
	CONTEXT_SEGMENT,
	CONTEXT_RATLINE,
	CONTEXT_VERTEX,
	CONTEXT_TEXT,
	CONTEXT_AREA_CORNER,
	CONTEXT_AREA_EDGE,
	CONTEXT_OP_CORNER,
	CONTEXT_OP_SIDE,
	CONTEXT_END_VERTEX,
	CONTEXT_FP_PAD,
	CONTEXT_CONNECT,
	CONTEXT_NET,
	CONTEXT_GROUP,
	CONTEXT_VALUE_TEXT
};

/////////////////////////////////////////////////////////////////////////////
// CFreePcbView

IMPLEMENT_DYNCREATE(CFreePcbView, CView)

BEGIN_MESSAGE_MAP(CFreePcbView, CView)
	//{{AFX_MSG_MAP(CFreePcbView)
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_KEYDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_SYSKEYDOWN()
	ON_WM_SYSKEYUP()
	ON_WM_MOUSEWHEEL()
	//}}AFX_MSG_MAP
	ON_WM_CONTEXTMENU()
	
ON_COMMAND(ID_PART_MOVE, OnPartMove)
ON_COMMAND(ID_NONE_ADDTEXT, OnTextAdd)
ON_COMMAND(ID_TEXT_DELETE, OnTextDelete)
ON_COMMAND(ID_TEXT_MOVE, OnTextMove)
ON_COMMAND(ID_PART_GLUE, OnPartGlue)
ON_COMMAND(ID_PART_UNGLUE, OnPartUnglue)
ON_COMMAND(ID_PART_DELETE, OnPartDelete)
ON_COMMAND(ID_PART_OPTIMIZE, OnPartOptimize)
ON_COMMAND(ID_REMOVEMERGE, OnPartRemoveMerge)
ON_COMMAND(ID_SELECT_SIMILAR, OnSelectSimilarParts)
ON_COMMAND(ID_SELECT_SIMILAR_P, OnSelectSimilarPackages)
ON_COMMAND(ID_REF_MOVE, OnRefMove)
ON_COMMAND(ID_PAD_OPTIMIZERATLINES, OnPadOptimize)
ON_COMMAND(ID_PAD_ADDTONET, OnPadAddToNet)
ON_COMMAND(ID_PAD_DETACHFROMNET, OnPadDetachFromNet)
ON_COMMAND(ID_PAD_CONNECTTOPIN, OnPadConnectToPin)
ON_COMMAND(ID_SEGMENT_SETWIDTH, OnSegmentSetWidth)
ON_COMMAND(ID_SEGMENT_UNROUTE, OnSegmentUnroute)
ON_COMMAND(ID_RATLINE_ROUTE, OnRatlineRoute)
ON_COMMAND(ID_RATLINE_OPTIMIZE, OnRatlineOptimize)
ON_COMMAND(ID_VERTEX_MOVE, OnVertexMove)
ON_COMMAND(ID_VERTEX_DELETE, OnVertexDelete)
ON_COMMAND(ID_VERTEX_SETSIZE, OnVertexSize)
ON_COMMAND(ID_RATLINE_COMPLETE, OnRatlineComplete)
ON_COMMAND(ID_RATLINE_SETWIDTH, OnRatlineSetWidth)
ON_COMMAND(ID_RATLINE_DELETECONNECTION, OnRatlineDeleteConnection)
ON_COMMAND(ID_RATLINE_LOCKCONNECTION, OnRatlineLockConnection)
ON_COMMAND(ID_RATLINE_UNLOCKCONNECTION, OnRatlineUnlockConnection)
ON_COMMAND(ID_TEXT_EDIT, OnTextEdit)
ON_COMMAND(ID_ADD_POLYLINE, OnAddOutlinePoly)
ON_COMMAND(ID_OP_CORNER_MOVE, OnOPCornerMove)
ON_COMMAND(ID_OP_CORNER_EDIT, OnOPCornerEdit)
ON_COMMAND(ID_OP_CORNER_DELETECORNER, OnOPCornerDelete)
ON_COMMAND(ID_OP_CORNER_DELETEOUTLINE, OnOPDeleteOutline)
ON_COMMAND(ID_OP_SIDE_INSERTCORNER, OnOPSideAddCorner)
ON_COMMAND(ID_OP_SIDE_DELETEOUTLINE, OnOPDeleteOutline)
ON_COMMAND(ID_PAD_STARTSTUBTRACE, OnPadStartStubTrace)
ON_COMMAND(ID_SEGMENT_DELETE, OnSegmentDelete)
ON_COMMAND(ID_ENDVERTEX_MOVE, OnEndVertexMove)
ON_COMMAND(ID_ENDVERTEX_ADDSEGMENTS, OnEndVertexAddSegments)
ON_COMMAND(ID_ENDVERTEX_ADDCONNECTION, OnEndVertexAddConnection)
ON_COMMAND(ID_ENDVERTEX_DELETE, OnEndVertexDelete)
ON_COMMAND(ID_ENDVERTEX_EDIT, OnEndVertexEdit)
ON_COMMAND(ID_AREACORNER_MOVE, OnAreaCornerMove)
ON_COMMAND(ID_AREACORNER_DELETE, OnAreaCornerDelete)
ON_COMMAND(ID_AREACORNER_DELETEAREA, OnAreaCornerDeleteArea)
ON_COMMAND(ID_AREAEDGE_ADDCORNER, OnAreaSideAddCorner)
ON_COMMAND(ID_AREAEDGE_DELETE, OnAreaSideDeleteArea)
ON_COMMAND(ID_AREAEDGE_DELETECUTOUT, OnAreaDeleteCutout)
ON_COMMAND(ID_AREACORNER_DELETECUTOUT, OnAreaDeleteCutout)
ON_COMMAND(ID_ADD_AREA, OnAddArea)
ON_COMMAND(ID_NONE_ADDCOPPERAREA, OnAddArea)
ON_COMMAND(ID_ENDVERTEX_ADDVIA, OnEndVertexAddVia)
ON_COMMAND(ID_ENDVERTEX_REMOVEVIA, OnEndVertexRemoveVia)
ON_COMMAND(ID_ENDVERTEX_SETSIZE, OnVertexSize)
ON_MESSAGE( WM_USER_VISIBLE_GRID, OnChangeVisibleGrid )
ON_COMMAND(ID_ADD_TEXT, OnTextAdd)
ON_COMMAND(ID_SEGMENT_DELETETRACE, OnSegmentDeleteTrace)
ON_COMMAND(ID_AREACORNER_PROPERTIES, OnAreaCornerProperties)
ON_COMMAND(ID_REF_PROPERTIES, OnRefProperties)
ON_COMMAND(ID_VERTEX_PROPERITES, OnVertexProperties)
ON_WM_ERASEBKGND()
ON_COMMAND(ID_OP_SIDE_CONVERTTOSTRAIGHTLINE, OnOPSideConvertToStraightLine)
ON_COMMAND(ID_OP_SIDE_CONVERTTOARC_CW, OnOPSideConvertToArcCw)
ON_COMMAND(ID_OP_SIDE_CONVERTTOARC_CCW, OnOPSideConvertToArcCcw)
ON_COMMAND(ID_SEGMENT_UNROUTETRACE, OnUnrouteTrace)
ON_COMMAND(ID_VERTEX_UNROUTETRACE, OnUnrouteTrace)
ON_COMMAND(ID_VIEW_ENTIREBOARD, OnViewEntireBoard)
ON_COMMAND(ID_VIEW_ALLELEMENTS, OnViewAllElements)
ON_COMMAND(ID_AREAEDGE_HATCHSTYLE, OnAreaEdgeHatchStyle)
ON_COMMAND(ID_PART_EDITFOOTPRINT, OnPartEditThisFootprint)
ON_COMMAND(ID_PART_SET_REF, OnRefProperties)
ON_COMMAND(ID_RATLINE_CHANGEPIN, OnRatlineChangeEndPin)
ON_WM_KEYUP()
ON_COMMAND(ID_VIEW_FINDPART, OnViewFindpart)
ON_COMMAND(ID_NONE_FOOTPRINTWIZARD, OnFootprintWizard)
ON_COMMAND(ID_NONE_FOOTPRINTEDITOR, OnFootprintEditor)
ON_COMMAND(ID_NONE_CHECKPARTSANDNETS, OnCheckPartsAndNets)
ON_COMMAND(ID_NONE_DRC, OnDrc)
ON_COMMAND(ID_NONE_CLEARDRCERRORS, OnClearDRC)
ON_COMMAND(ID_NONE_VIEWALL, OnViewAllElements)
ON_COMMAND(ID_OP_HATCHSTYLE, OnOPHatchStyle)
ON_COMMAND(ID_PART_CHANGESIDE, OnPartChangeSide)
ON_COMMAND(ID_PART_ROTATE, OnPartRotate)
ON_COMMAND(ID_AREAEDGE_ADDCUTOUT, OnAreaAddCutout)
ON_COMMAND(ID_AREACORNER_ADDCUTOUT, OnAreaAddCutout)
ON_COMMAND(ID_NET_SETWIDTH, OnNetSetWidth)
ON_COMMAND(ID_CONNECT_SETWIDTH, OnConnectSetWidth)
ON_COMMAND(ID_CONNECT_UNROUTETRACE, OnConnectUnroutetrace)
ON_COMMAND(ID_CONNECT_DELETETRACE, OnConnectDeletetrace)
ON_COMMAND(ID_SEGMENT_CHANGELAYER, OnSegmentChangeLayer)
ON_COMMAND(ID_SEGMENT_ADDVERTEX, OnSegmentAddVertex)
ON_COMMAND(ID_CONNECT_CHANGELAYER, OnConnectChangeLayer)
ON_COMMAND(ID_NET_CHANGELAYER, OnNetChangeLayer)
ON_COMMAND(ID_NET_EDITNET, OnNetEditnet)
ON_COMMAND(ID_TOOLS_MOVEORIGIN, OnToolsMoveOrigin)
ON_WM_LBUTTONUP()
ON_COMMAND(ID_GROUP_MOVE, OnGroupMove)
ON_COMMAND(ID_AREACORNER_ADDNEWAREA, OnAddSimilarArea)
ON_COMMAND(ID_AREAEDGE_ADDNEWAREA, OnAddSimilarArea)
ON_COMMAND(ID_AREAEDGE_CHANGELAYER, OnAreaEdit)
ON_COMMAND(ID_AREACORNER_CHANGELAYER, OnAreaEdit)
ON_COMMAND(ID_AREAEDGE_APPLYCLEARANCES, OnAreaEdgeApplyClearances)
ON_COMMAND(ID_GROUP_SAVETOFILE, OnGroupSaveToFile)
ON_COMMAND(ID_STATIC_HIGHLIGHT, OnGroupStaticHighlight)
ON_COMMAND(ID_CANCEL_HIGHLIGHT, OnGroupCancelHighlight)
ON_COMMAND(ID_ALIGN_PARTS, OnGroupAlignParts)
ON_COMMAND(ID_APPROXIMATION_ARC, OnApproximationArc)
ON_COMMAND(ID_GROUP_COPY, OnGroupCopy)
ON_COMMAND(ID_GROUP_CUT, OnGroupCut)
ON_COMMAND(ID_GROUP_DELETE, OnGroupDelete)
ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
ON_COMMAND(ID_VERTEX_CONNECTTOPIN, OnVertexConnectToPin)
ON_COMMAND(ID_EDIT_CUT, OnEditCut)
ON_COMMAND(ID_EDIT_SAVEGROUPTOFILE, OnGroupSaveToFile)
ON_COMMAND(ID_GROUP_ROTATE, OnGroupRotate)
ON_WM_SETCURSOR()
ON_WM_MOVE()
ON_COMMAND(ID_AREA_SIDESTYLE, OnAreaSideStyle)
ON_COMMAND(ID_VALUE_MOVE, OnValueMove)
ON_COMMAND(ID_VALUE_CHANGESIZE, OnValueProperties)
ON_COMMAND(ID_REF_SHOWPART, OnRefShowPart)
ON_COMMAND(ID_VALUE_SHOWPART, OnValueShowPart)
ON_COMMAND(ID_PART_EDITVALUE, OnPartEditValue)
ON_COMMAND(ID_PART_ROTATECOUNTERCLOCKWISE, OnPartRotateCCW)
ON_COMMAND(ID_REF_ROTATECW, OnRefRotateCW)
ON_COMMAND(ID_REF_ROTATECCW, OnRefRotateCCW)
ON_COMMAND(ID_VALUE_ROTATECW, OnValueRotateCW)
ON_COMMAND(ID_VALUE_ROTATECCW, OnValueRotateCCW)
ON_COMMAND(ID_SEGMENT_MOVE, OnSegmentMove)
ON_COMMAND(ID_SEL_ARC_EL, OnProjectSelectArcElements)
ON_COMMAND(ID_SEL_VIA_EL, OnProjectSelectViaElements)
ON_COMMAND(ID_RUN_INFO_BOX, OnProjectRunInfoBox)
ON_COMMAND(ID_TOOLS_SETORIGIN, OnSetOriginToSelectedItem)
//ON_COMMAND_RANGE(1, ID_MAX_NUM_COMMANDS, OnRangeCmds)
END_MESSAGE_MAP()


//===============================================================================================
/////////////////////////////////////////////////////////////////////////////
// CFreePcbView construction/destruction

CFreePcbView::CFreePcbView()
{
	// GetDocument() is not available at this point
	m_small_font.CreateFont( 14, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
		OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, "Arial" );
#if 0
	m_small_font.CreateFont( 10, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
		OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, "MS Sans Serif" );
#endif

	m_Doc = NULL;
	m_dlist = 0;
	m_last_mouse_point.x = 0;
	m_last_mouse_point.y = 0;
	m_last_cursor_point.x = 0;
	m_last_cursor_point.y = 0;
	m_left_pane_w = 110;	// the left pane on screen is this wide (pixels)
	m_bottom_pane_h = 40;	// the bottom pane on screen is this high (pixels)
	m_botDC_created = FALSE;
	m_topDC_created = FALSE;
	m_dragging_new_item = FALSE;
	m_bDraggingRect = FALSE;
	m_bLButtonDown = FALSE;
	CalibrateTimer();
 }
//===============================================================================================
// initialize the view object
// this code can't be placed in the constructor, because it depends on document
// don't try to draw window until this function has been called
// need only be called once
//
void CFreePcbView::InitInstance()
{
	// this should be called from InitInstance function of CApp,
	// after the document is created
	m_Doc = GetDocument();
	ASSERT_VALID(m_Doc);
	m_Doc->m_edit_footprint = FALSE;
	m_dlist = m_Doc->m_dlist;
	InitializeView();
	CRect screen_r;
	GetWindowRect( &screen_r );
	m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h,
		m_pcbu_per_pixel, m_org_x, m_org_y );
	for(int i=0; i<m_Doc->m_num_layers; i++ )
		m_dlist->SetLayerRGB( i, m_Doc->m_rgb[i][0], m_Doc->m_rgb[i][1], m_Doc->m_rgb[i][2] );
	ShowSelectStatus();
	ShowActiveLayer(MAX_LAYERS-LAY_TOP_COPPER);
	m_Doc->m_view = this;
	// set up array of mask ids
	m_mask_id[SEL_MASK_PARTS].Set( ID_PART, ID_SEL_RECT );
	m_mask_id[SEL_MASK_REF].Set( ID_PART_LINES );
	m_mask_id[SEL_MASK_PINS].Set( ID_PART, ID_PAD );
	m_mask_id[SEL_MASK_CON].Set( ID_NET, ID_CONNECT, 0, ID_SEG );
	m_mask_id[SEL_MASK_VIA].Set( ID_NET, ID_CONNECT, 0, ID_VERTEX );
	m_mask_id[SEL_MASK_AREAS].Set( ID_NET, ID_AREA );
	m_mask_id[SEL_MASK_TEXT].Set( ID_TEXT );
	m_mask_id[SEL_MASK_OP].Set( ID_POLYLINE );
	m_mask_id[SEL_MASK_DRC].Set( ID_DRC );
}
//===============================================================================================
// initialize view with defaults for a new project
// should be called each time a new project is created
//
void CFreePcbView::InitializeView()
{
	if( !m_dlist )
		ASSERT(0);

	// set defaults
	m_prev_sel_merge = -1;
	SetCursorMode( CUR_NONE_SELECTED );
	m_sel_id.Clear();
	m_sel_layer = 0;
	m_dir = 0;
	m_routing_width = _2540*100;
	m_debug_flag = 0;
	m_dragging_new_item = 0;
	m_active_layer = LAY_TOP_COPPER;
	m_bDraggingRect = FALSE;
	m_bLButtonDown = FALSE;
	m_sel_mask = 0xffff;
	SetSelMaskArray( m_sel_mask );
	m_inflection_mode = IM_90_45;
	m_snap_mode = SM_GRID_POINTS;
	fCopyTraces = FALSE;	
	fRepour = FALSE;
	en_branch = 0;
	m_page = 1;
	// default screen coords in world units (i.e. display units)
	m_pcbu_per_pixel = 5.0*PCBU_PER_MIL;	// 5 mils per pixel
	m_org_x = -100.0*PCBU_PER_MIL;			// lower left corner of window
	m_org_y = -100.0*PCBU_PER_MIL;
	// previous
	prevx = 0;
	ppx = 0;
	prev_middle_x = 0;
	// previous
	prevy = 0;
	ppy = 0;
	prev_middle_y = 0;
	prev_m_ang = -1;

	// clearance
	m_seg_clearance = 0;
	// grid defaults
	m_Doc->m_snap_angle = 45;
	// lines
	m_polyline_style = 0;
	m_polyline_hatch = 0;
	m_polyline_layer = 0;
	m_polyline_width = 0;
	m_polyline_closed = 0;
	//
	m_sel_merge_name = "";
	m_left_pane_invalid = TRUE;
	g_bShow_Ratline_Warning = TRUE;
	g_bShow_nl_lock_Warning = TRUE;

	//InitializeView
	m_draw_layer = ENABLE_CHANGE_DRAW_LAYER; // InitializeView
	Invalidate( FALSE );
}
//===============================================================================================
// destructor
CFreePcbView::~CFreePcbView()
{
}
//===============================================================================================
BOOL CFreePcbView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}
//===============================================================================================
/////////////////////////////////////////////////////////////////////////////
// CFreePcbView drawing

void CFreePcbView::OnDraw(CDC* pDC)
{
	// get client rectangle
	GetClientRect( &m_client_r );
	RECT r = m_client_r;
	// clear screen to black if no project open
	if( !m_Doc )
	{
		pDC->FillSolidRect( m_client_r, RGB(0,0,0) );
		return;
	}
	if( !m_Doc->m_project_open )
	{
		pDC->FillSolidRect( m_client_r, RGB(0,0,0) );
		return;
	}
	m_client_r.left += m_left_pane_w;
	// draw stuff on left pane
	int y_off = 10;
	int x_off = 10;
	if( m_left_pane_invalid )
	{
		// erase previous contents if changed
		CBrush brush( RGB(255, 255, 255) );
		CPen pen( PS_SOLID, 1, RGB(255, 255, 255) );
		CBrush * old_brush = pDC->SelectObject( &brush );
		CPen * old_pen = pDC->SelectObject( &pen );

		// erase left pane
		r.right = m_left_pane_w;
		//r.bottom -= m_bottom_pane_h;
		pDC->Rectangle( &r );
		// erase bottom pane
		r = m_client_r;
		r.top = r.bottom - m_bottom_pane_h;
		pDC->Rectangle( &r );
		pDC->SelectObject( old_brush );
		pDC->SelectObject( old_pen );
		m_left_pane_invalid = FALSE;
		//
		CFont * old_font = pDC->SelectObject( &m_small_font );
		int index_to_active_layer;
		for( int i=0; i<m_Doc->m_num_layers; i++ )
		{
			// i = position index
			r.left = x_off;
			r.right = x_off+12;
			r.top = i*VSTEP+y_off;
			r.bottom = i*VSTEP+12+y_off;
			// il = layer index, since copper layers are displayed out of order
			int il = i;
			//
			CBrush brush( RGB(m_Doc->m_rgb[il][0], m_Doc->m_rgb[il][1], m_Doc->m_rgb[il][2]) );
			if( m_Doc->m_vis[il] )
			{
				// if layer is visible, draw colored rectangle
				CBrush * old_brush = pDC->SelectObject( &brush );
				pDC->Rectangle( &r );
				pDC->SelectObject( old_brush );
			}
			else
			{
				// if layer is invisible, draw box with X
				pDC->Rectangle( &r );
				pDC->MoveTo( r.left, r.top );
				pDC->LineTo( r.right, r.bottom );
				pDC->MoveTo( r.left, r.bottom );
				pDC->LineTo( r.right, r.top );
			}
			r.left += 20;
			r.right += 120;
			r.bottom += 5;
			pDC->DrawText( &layer_str[il][0], -1, &r, 0 );
			if( il >= LAY_SM_TOP )
			{
				CString num_str;
				num_str.Format( "[%c]*", layer_char[i-LAY_SM_TOP] );
				RECT nr = r;
				nr.left = nr.right - 55;
				pDC->DrawText( num_str, -1, &nr, DT_TOP );
			}
			RECT ar = r;
			ar.left = 2;
			ar.right = 8;
			ar.bottom -= 5;
			if( il == m_active_layer )
			{
				// draw arrowhead
				pDC->MoveTo( ar.left, ar.top+1 );
				pDC->LineTo( ar.right-1, (ar.top+ar.bottom)/2 );
				pDC->LineTo( ar.left, ar.bottom-1 );
				pDC->LineTo( ar.left, ar.top+1 );
			}
			else
			{
				// erase arrowhead
				pDC->FillSolidRect( &ar, RGB(255,255,255) );
			}
		}
		r.left = x_off;
		r.bottom += VSTEP*2;
		r.top += VSTEP*2;
		pDC->DrawText( "SELECTION MASK", -1, &r, DT_TOP );
		y_off = r.bottom;
		for( int i=0; i<NUM_SEL_MASKS; i++ )
		{
			// i = position index
			r.left = x_off;
			r.right = x_off+12;
			r.top = i*VSTEP+y_off;
			r.bottom = i*VSTEP+12+y_off;
			CBrush green_brush( RGB(0, 255, 0) );
			CBrush red_brush( RGB(255, 0, 0) );
			if( m_sel_mask & (1<<i) )
			{
				// if mask is selected is visible, draw green rectangle
				CBrush * old_brush = pDC->SelectObject( &green_brush );
				pDC->Rectangle( &r );
				pDC->SelectObject( old_brush );
			}
			else
			{
				// if mask not selected, draw red
				CBrush * old_brush = pDC->SelectObject( &red_brush );
				pDC->Rectangle( &r );
				pDC->SelectObject( old_brush );
			}
			r.left += 20;
			r.right += 120;
			r.bottom += 5;
			pDC->DrawText( sel_mask_str[i], -1, &r, DT_TOP );
		}
		r.left = x_off;
		r.bottom += VSTEP*2;
		r.top += VSTEP*2;
		pDC->DrawText( "* Use these", -1, &r, DT_TOP );
		r.bottom += VSTEP;
		r.top += VSTEP;
		pDC->DrawText( "keys to change", -1, &r, DT_TOP );
		r.bottom += VSTEP;
		r.top += VSTEP;
		pDC->DrawText( "routing layer", -1, &r, DT_TOP );
	
		// draw function keys on bottom pane
		DrawBottomPane();
	}

	//** this is for testing only, needs to be converted to PCB coords
#if 0
	if( b_update_rect )
	{
		// clip to update rectangle
		CRgn rgn;
		rgn.CreateRectRgn( m_left_pane_w + update_rect.left,
			update_rect.bottom,
			m_left_pane_w + update_rect.right,
			update_rect.top  );
		pDC->SelectClipRgn( &rgn );
	}
	else
#endif
	{
		// clip to pcb drawing region
		pDC->SelectClipRgn( &m_pcb_rgn );
	}

	// now draw the display list
	SetDCToWorldCoords( pDC );
	m_dlist->Draw( pDC, m_draw_layer );
	m_draw_layer = ENABLE_CHANGE_DRAW_LAYER;// after OnDraw
}
//===============================================================================================
/////////////////////////////////////////////////////////////////////////////
// CFreePcbView printing

BOOL CFreePcbView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}
//===============================================================================================
void CFreePcbView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}
//===============================================================================================
void CFreePcbView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}
//===============================================================================================
/////////////////////////////////////////////////////////////////////////////
// CFreePcbView diagnostics

#ifdef _DEBUG
void CFreePcbView::AssertValid() const
{
	CView::AssertValid();
}
//===============================================================================================
void CFreePcbView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
//===============================================================================================
CFreePcbDoc* CFreePcbView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CFreePcbDoc)));
	return (CFreePcbDoc*)m_pDocument;
}
#endif //_DEBUG
//===============================================================================================
/////////////////////////////////////////////////////////////////////////////
// CFreePcbView message handlers

// Window was resized
//
void CFreePcbView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);

	// update client rect and create clipping region
	GetClientRect( &m_client_r );
	m_pcb_rgn.DeleteObject();
	m_pcb_rgn.CreateRectRgn( m_left_pane_w, m_client_r.bottom-m_bottom_pane_h,
		m_client_r.right, m_client_r.top );

	// update display mapping for display list
	if( m_dlist )
	{
		CRect screen_r;
		GetWindowRect( &screen_r );
		m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel,
					m_org_x, m_org_y );
	}

	// create memory DC and DDB
	if( !m_botDC_created && m_client_r.right != 0 )
	{
		CDC * pDC = GetDC();
		m_botDC.CreateCompatibleDC( pDC );
		m_botDC_created = TRUE;
		m_bitmap1.CreateCompatibleBitmap( pDC, m_client_r.right, m_client_r.bottom );
		m_old_bitmap1 = (HBITMAP)::SelectObject( m_botDC.m_hDC, m_bitmap1.m_hObject );		
	}
	else if( m_botDC_created && (m_bitmap_rect1 != m_client_r) )
	{
		CDC * pDC = GetDC();
		::SelectObject(m_botDC.m_hDC, m_old_bitmap1 );
		m_bitmap1.DeleteObject();
		m_bitmap1.CreateCompatibleBitmap( pDC, m_client_r.right, m_client_r.bottom );
		m_old_bitmap1 = (HBITMAP)::SelectObject( m_botDC.m_hDC, m_bitmap1.m_hObject );		
	}
	//
	if( !m_topDC_created && m_client_r.right != 0 )
	{
		CDC * pDC = GetDC();
		m_topDC.CreateCompatibleDC( pDC );
		m_topDC_created = TRUE;
		m_bitmap2.CreateCompatibleBitmap( pDC, m_client_r.right, m_client_r.bottom );
		m_old_bitmap2 = (HBITMAP)::SelectObject( m_topDC.m_hDC, m_bitmap2.m_hObject );
		m_bitmap_rect2 = m_client_r;
		ReleaseDC( pDC );
	}
	else if( m_topDC_created && (m_bitmap_rect2 != m_client_r) )
	{
		CDC * pDC = GetDC();
		::SelectObject(m_topDC.m_hDC, m_old_bitmap2 );
		m_bitmap2.DeleteObject();
		m_bitmap2.CreateCompatibleBitmap( pDC, m_client_r.right, m_client_r.bottom );
		m_old_bitmap2 = (HBITMAP)::SelectObject( m_topDC.m_hDC, m_bitmap2.m_hObject );	
		m_bitmap_rect2 = m_client_r;
		ReleaseDC( pDC );
	}
	
}
//===============================================================================================
// Left mouse button released, we should probably do something
//
void CFreePcbView::OnLButtonUp(UINT nFlags, CPoint point)
{
	CDC * pDC = NULL;
	CPoint tp = m_dlist->WindowToPCB( point );
	if( !m_bLButtonDown )
	{
		// this avoids problems with opening a project with the button held down
		CView::OnLButtonUp(nFlags, point);
		goto goodbye;
	}

	m_bLButtonDown = FALSE;
	gLastKeyWasArrow = FALSE;	// cancel series of arrow keys

	if( m_bDraggingRect )
	{
		// we were dragging selection rect, handle it
		m_last_drag_rect.NormalizeRect();
		CPoint tl = m_dlist->WindowToPCB( m_last_drag_rect.TopLeft() );
		CPoint br = m_dlist->WindowToPCB( m_last_drag_rect.BottomRight() );
		m_sel_rect = CRect( tl, br );
		// control key held down
		if( m_sel_count < 2 )
			CancelSelection();
		SelectItemsInRect( m_sel_rect, nFlags );
		m_bDraggingRect = FALSE;
		CView::OnLButtonUp(nFlags, point);
		goto goodbye;
	}

	if( point.y > (m_client_r.bottom-m_bottom_pane_h) )
	{
		// clicked in bottom pane, test for hit on function key rectangle
		for( int i=0; i<9; i++ )
		{
			CRect r( FKEY_OFFSET_X+i*FKEY_STEP+(i/4)*FKEY_GAP,
				m_client_r.bottom-FKEY_OFFSET_Y-FKEY_R_H,
				FKEY_OFFSET_X+i*FKEY_STEP+(i/4)*FKEY_GAP+FKEY_R_W,
				m_client_r.bottom-FKEY_OFFSET_Y );
			if( r.PtInRect( point ) )
			{
				// fake function key pressed
				int nChar = i + 112;
				HandleKeyPress( nChar, 0, 0 );
				return;
			}
		}
	}
	else if( point.x < m_left_pane_w )
	{
		// clicked in left pane
		CRect r = m_client_r;
		int y_off = 10;
		int x_off = 10;
		for( int i=0; i<m_Doc->m_num_layers; i++ )
		{
			// i = position index
			// il = layer index, since copper layers are displayed out of order
			int il = i;
			//
			// get color square
			r.left = x_off;
			r.right = x_off+12;
			r.top = i*VSTEP+y_off;
			r.bottom = i*VSTEP+12+y_off;
			if( r.PtInRect( point ) && il > LAY_BACKGND )
			{
				// clicked in color square
				m_Doc->m_vis[il] = !m_Doc->m_vis[il];
				m_dlist->SetLayerVisible( il, m_Doc->m_vis[il] );
				if( il == LAY_RAT_LINE && m_Doc->m_vis[il] && g_bShow_Ratline_Warning )
				{
					CDlgMyMessageBox dlg;
					dlg.Initialize( "Ratlines turned back on, but may not be up to date.\n\nPress F9 to recalculate." );
					dlg.DoModal();
					g_bShow_Ratline_Warning = !dlg.bDontShowBoxState;
				}
				if ( m_cursor_mode == CUR_NONE_SELECTED )
					m_Doc->m_nlist->OptimizeConnections( m_Doc->m_auto_ratline_disable, m_Doc->m_auto_ratline_min_pins, FALSE );
			}
			else
			{
				// get layer name rect
				r.left += 20;
				r.right += 120;
				r.bottom += 5;
				if( r.PtInRect( point ) )
				{
					// clicked on layer name
					if( i >= LAY_SM_TOP && i < LAY_SM_TOP + 8 )
					{
						int nChar = '1' + i - LAY_SM_TOP;
						HandleKeyPress( nChar, 0, 0 );
					}
					else
					{
						switch( i )
						{
						case LAY_SM_TOP:		HandleKeyPress( '1', 0, 0 ); break;
						case LAY_SM_BOTTOM:		HandleKeyPress( '2', 0, 0 ); break;
						case LAY_TOP_COPPER:	HandleKeyPress( '3', 0, 0 ); break;
						case LAY_TOP_COPPER+1:	HandleKeyPress( '4', 0, 0 ); break;
						case LAY_TOP_COPPER+2:	HandleKeyPress( '5', 0, 0 ); break;
						case LAY_TOP_COPPER+3:	HandleKeyPress( '6', 0, 0 ); break;
						case LAY_TOP_COPPER+4:	HandleKeyPress( '7', 0, 0 ); break;
						case LAY_TOP_COPPER+5:	HandleKeyPress( '8', 0, 0 ); break;
						case LAY_TOP_COPPER+6:	HandleKeyPress( 'Q', 0, 0 ); break;
						case LAY_TOP_COPPER+7:	HandleKeyPress( 'W', 0, 0 ); break;
						case LAY_TOP_COPPER+8:	HandleKeyPress( 'E', 0, 0 ); break;
						case LAY_TOP_COPPER+9:	HandleKeyPress( 'R', 0, 0 ); break;
						case LAY_TOP_COPPER+10: HandleKeyPress( 'T', 0, 0 ); break;
						case LAY_TOP_COPPER+11: HandleKeyPress( 'Y', 0, 0 ); break;
						case LAY_TOP_COPPER+12: HandleKeyPress( 'U', 0, 0 ); break;
						case LAY_TOP_COPPER+13: HandleKeyPress( 'I', 0, 0 ); break;
						case LAY_TOP_COPPER+14: HandleKeyPress( 'O', 0, 0 ); break;
						case LAY_TOP_COPPER+15: HandleKeyPress( 'P', 0, 0 ); break;
						}
					}
				}
			}
		}
		y_off = r.bottom + 2*VSTEP;
		for( int i=0; i<NUM_SEL_MASKS; i++ )
		{
			// get color square
			r.left = x_off;
			r.right = x_off+12+120;
			r.top = i*VSTEP+y_off;
			r.bottom = i*VSTEP+12+y_off;
			if( r.PtInRect( point ) )
			{
				// clicked in color square or name
				invbit( m_sel_mask, i );
				if( nFlags & MK_CONTROL )
					m_sel_mask = ~m_sel_mask;
				SetSelMaskArray( m_sel_mask );
			}
		}
		InvalidateLeftPane();
		Invalidate(FALSE);
		return;
	}
	else
	{
		// clicked in PCB pane
		if(	CurNone() || CurSelected() )
		{
			m_draw_layer = LAY_HILITE;  // LButtonUp if( CurNone() || CurSelected() )
			// see if new item selected
			CPoint p = m_dlist->WindowToPCB( point );
			id sid(0,0,0,0,0);
			void * sel_ptr = NULL;
			if( m_sel_count == 1 )
			{
				if( m_sel_id.type == ID_PART || m_sel_id.type == ID_PART_LINES )
					sel_ptr = m_sel_part;
				else if( m_sel_id.type == ID_NET )
					sel_ptr = m_sel_net;
				else if( m_sel_id.type == ID_TEXT )
					sel_ptr = m_sel_text;
				else if( m_sel_id.type == ID_DRC )
					sel_ptr = m_sel_dre;
			}

			// save masks in case they are changed
			id old_mask_pins = m_mask_id[SEL_MASK_PINS];
			id old_mask_ref = m_mask_id[SEL_MASK_REF];
			if( (nFlags & MK_CONTROL) && m_mask_id[SEL_MASK_PARTS].ii == 0xfffe )
			{
				// if control key pressed and parts masked, also mask pins and ref
				m_mask_id[SEL_MASK_PINS].ii = 0xfffe;
				m_mask_id[SEL_MASK_REF].ii = 0xfffe;
			}
			m_sel_layer = m_active_layer;
			if( m_sel_layer < LAY_TOP_COPPER )
				m_sel_layer += 2;
			void * ptr = m_dlist->TestSelect( p.x, p.y, &sid, &m_sel_layer, &m_sel_id, sel_ptr,
				m_mask_id, NUM_SEL_MASKS );

			// restore mask
			m_mask_id[SEL_MASK_PINS] = old_mask_pins;
			m_mask_id[SEL_MASK_REF] = old_mask_ref;

			// check for second pad selected while holding down 's'
			SHORT kc = GetKeyState('S');
			if( kc & 0x8000 && m_cursor_mode == CUR_PAD_SELECTED )
			{
				if( sid.type == ID_PART && 
					sid.st == ID_PAD && 
					m_sel_id.type == ID_PART && 
					m_sel_id.st == ID_PAD )
				{
					CString mess;
					// OK, now swap pads
					cpart * part1 = m_sel_part;
					CString pin_name1 = part1->shape->GetPinNameByIndex( m_sel_id.i );
					cnet * net1 = m_Doc->m_plist->GetPinNet(part1, &pin_name1);
					CString net_name1 = "unconnected";
					if( net1 )
						net_name1 = net1->name;
					cpart * part2 = (cpart*)ptr;
					CString pin_name2 = part2->shape->GetPinNameByIndex( sid.i );
					cnet * net2 = m_Doc->m_plist->GetPinNet(part2, &pin_name2);
					CString net_name2 = "unconnected";
					if( net2 )
						net_name2 = net2->name;
					if( net1 == NULL && net2 == NULL )
					{
						AfxMessageBox( "No connections to swap" );
						return;
					}
					int ret = IDCANCEL;
					if( m_Doc->m_netlist_completed )
					{
						mess = "Changing pin net is not possible because netlist is protected. You can remove protection through the menu Project-->Nets-->Netlist Protected";
						if( g_bShow_nl_lock_Warning )
						{
							CDlgMyMessageBox dlg;
							dlg.Initialize( mess );
							dlg.DoModal();
							g_bShow_nl_lock_Warning = !dlg.bDontShowBoxState;
						}
					}
					else
					{
						mess.Format( "Swap %s.%s (\"%s\") and %s.%s (\"%s\") ?",
							part1->ref_des, pin_name1, net_name1,
							part2->ref_des, pin_name2, net_name2 );
						ret = AfxMessageBox( mess, MB_OKCANCEL );
					}
					if( ret == IDOK )
					{
						m_draw_layer = DISABLE_CHANGE_DRAW_LAYER;//swap
						SaveUndoInfoFor2PartsAndNets( part1, part2, TRUE, m_Doc->m_undo_list );
						m_Doc->m_nlist->SwapPins( part1, &pin_name1, part2, &pin_name2 );
						m_Doc->ProjectModified( TRUE );
						ShowSelectStatus();
					}
					goto goodbye;
				}
			}
			// Swap connects...
			else if	( kc & 0x8000 && m_cursor_mode == CUR_SEG_SELECTED )
			{
				if( sid.type == ID_NET && sid.st == ID_CONNECT && sid.sst == ID_SEG )
				{
					m_draw_layer = DISABLE_CHANGE_DRAW_LAYER;  // Swap connects
					cnet * n2 = (cnet*)	ptr;
					CString mess;
					mess.Format( "Swap connect %d (\"%s\") and connect %d (\"%s\") ?",
								m_sel_ic, m_sel_net->name, 
								sid.i, n2->name );
					int ret = AfxMessageBox( mess, MB_OKCANCEL );
					if( ret == IDOK )
					{
						m_draw_layer = DISABLE_CHANGE_DRAW_LAYER;//swap
						SaveUndoInfoForNetAndConnectionsAndAreas( m_sel_net, m_Doc->m_undo_list );
						SaveUndoInfoForNetAndConnectionsAndAreas( n2, m_Doc->m_undo_list );
						m_Doc->m_nlist->SwapConnects( m_sel_net, n2, m_sel_id, sid );
						CancelSelection();
						m_Doc->m_nlist->OptimizeConnections(	m_Doc->m_auto_ratline_disable,
																m_Doc->m_auto_ratline_min_pins, TRUE  );		
						m_Doc->ProjectModified( TRUE );
					}
					goto goodbye;
				}
			}

			// CTRL key
			if( nFlags & MK_CONTROL )
				if( sid.type == ID_PART && sid.st == ID_PAD )
					sid.st = ID_SEL_RECT;

			// cancel sel
			if( sid.type == ID_NONE && !nFlags )
				goto cancel_selection_and_goodbye;

			// select merge el
			int el_merge = -1;
			BOOL flag_m = FALSE;
			if( sid.type == ID_PART && sid.st == ID_SEL_RECT && getbit( m_sel_mask, SEL_MASK_MERGES ) )
			{
				cpart * prt = (cpart*)ptr;
				if( prt )
				{
					el_merge = prt->m_merge;
					if( el_merge >= 0 )
					{
						if( nFlags & MK_CONTROL )
						{
							SelectMergeSegments( prt );
						}
						else
						{
							if( m_cursor_mode != CUR_GROUP_SELECTED )
								CancelSelection();
							NewSelectM( prt );		
							HighlightGroup();
							CString AllRef="";
							m_Doc->m_plist->GetSelParts( &AllRef );
							OnInfoBoxSendMess( "part_list " + AllRef );
							flag_m = TRUE;
						}
					}
				}
			}
			// select el
			if( !flag_m )
			{
				if( !nFlags && m_sel_count )
					CancelSelection();
				prev_sel_count = 0;
				if( sid.type != ID_NONE )
					el_merge = NewSelect( ptr, &sid, 1, 1 );
			}

			// selected count == 0
			if( m_sel_count == 0 )
				goto cancel_selection_and_goodbye;

			// return if group sel
			if( m_cursor_mode == CUR_GROUP_SELECTED )
				goto goodbye;

			if( sid.type == ID_PART )
			{
				if( (GetKeyState('N') & 0x8000) && sid.st == ID_PAD )
				{
					// pad selected and if "n" held down, select net
					cnet * net = m_Doc->m_plist->GetPinNet( m_sel_part, sid.i );
					if( net )
					{
						CancelSelection();
						m_sel_net = net;
						m_sel_id = net->id;
						m_sel_id.type = ID_NET;
						m_sel_id.st = ID_ENTIRE_NET;
						m_Doc->m_nlist->HighlightNetConnections( net );
						m_Doc->m_plist->HighlightAllPadsOnNet( net, 0, 0 );
						SetCursorMode( CUR_NET_SELECTED );
					}
				}
			}
			else if( m_sel_id.type == ID_NET )
			{
				if( m_sel_id.st == ID_CONNECT && m_sel_id.sst == ID_SEG )
				{
					// select connect
					BOOL h2 = true;
					if( m_sel_net->connect[m_sel_id.i].vtx[m_sel_id.ii+1].via_w )
						h2 = 0;
					m_Doc->m_nlist->HighlightConnection( m_sel_net, m_sel_id.i, m_sel_id.ii, 0, 0, 0, mod_active_layer );
					if( m_sel_seg.layer != LAY_RAT_LINE )
					{
						m_insert_seg_len = Distance(	m_sel_net->connect[m_sel_id.i].vtx[m_sel_id.ii].x,
														m_sel_net->connect[m_sel_id.i].vtx[m_sel_id.ii].y,
														m_sel_net->connect[m_sel_id.i].vtx[m_sel_id.ii+1].x,
														m_sel_net->connect[m_sel_id.i].vtx[m_sel_id.ii+1].y);
					}
				}
				else if( m_sel_id.st == ID_CONNECT && m_sel_id.sst == ID_VERTEX )
				{
					// select areas
					cconnect * c = &m_sel_net->connect[m_sel_id.i];
					if( c->end_pin == cconnect::NO_END && m_sel_id.ii == c->nsegs )
					{
						if ( m_sel_vtx.via_w )
							//for (int na=0; na<m_sel_net->nareas; na++)
								m_Doc->m_nlist->HighlightNet( m_sel_net, TRANSPARENT_HILITE );
					}
				}
			}
		}
		else 
		{
			m_draw_layer = mod_active_layer;  // LButtonUp if drag_modes
			if( m_cursor_mode == CUR_DRAG_PART )
			{			
				// complete move
				m_draw_layer = DISABLE_CHANGE_DRAW_LAYER; // CUR_DRAG_PART
				CPoint p = m_dlist->WindowToPCB( point );
				m_Doc->m_plist->StopDragging();
				int old_angle = m_Doc->m_plist->GetAngle( m_sel_part );
				int angle = old_angle + m_dlist->GetDragAngle();
				angle = angle % 360;
				int old_side = m_sel_part->side;
				int side = old_side + m_dlist->GetDragSide();
				if( side > 1 )
					side = side - 2;

				m_dragging_new_item = FALSE;

				// now move it
				m_sel_part->glued = 0;
				m_Doc->m_plist->Move(	m_sel_part, m_last_cursor_point.x, m_last_cursor_point.y,
										angle, side );
				m_Doc->m_nlist->PartMoved( m_sel_part , TRUE );
				m_Doc->m_nlist->OptimizeConnections( m_sel_part, m_Doc->m_auto_ratline_disable, 
													 m_Doc->m_auto_ratline_min_pins );
				if( m_sel_id.st == ID_PAD )
				{
					SetCursorMode( CUR_PAD_SELECTED );
					NewSelect( m_sel_part, & m_sel_id, TRUE, 0 );
				}
				else
				{
					SetCursorMode( CUR_PART_SELECTED );
					SelectPart( m_sel_part );
				}
				SetFKText( m_cursor_mode );
				m_Doc->ProjectModified( TRUE );
				/*#if 0
							//** for testing only
							RECT test_rect( 0, 0, 100, 100 );
							InvalidateRect( test_rect, FALSE );
							OnDraw( GetDC() );
				#endif*/
			}
			else if( m_cursor_mode == CUR_DRAG_GROUP || m_cursor_mode == CUR_DRAG_GROUP_ADD )
			{
				// complete move
				m_Doc->m_dlist->StopDragging();
				m_Doc->m_dlist->CancelHighLight();
				int dx = m_last_cursor_point.x - m_from_pt.x;
				int dy = m_last_cursor_point.y - m_from_pt.y;
				//
				if (fCopyTraces == FALSE)
				{
					MoveGroup( dx, dy , TRUE);
				}
				else
				{
					fCopyTraces = FALSE;
					if( getbit(m_sel_flags, FLAG_SEL_NET) && getbit(m_sel_flags, FLAG_SEL_CONNECT) )
						for(cnet* net=m_Doc->m_nlist->GetFirstNet(); net; net=m_Doc->m_nlist->GetNextNet(/*LABEL*/)) 
						{
							if( net->selected )
								for (int icon=net->nconnects-1; icon>=0; icon--)
									if( net->connect[icon].m_selected )
									{
										id Sid( ID_NET, ID_CONNECT, icon, ID_SEG, 0 );
										int x1 = net->connect[icon].vtx[0].x + dx;
										int y1 = net->connect[icon].vtx[0].y + dy;
										int l1 = net->connect[icon].vtx[0].pad_layer;
										int x2 = net->connect[icon].vtx[net->connect[icon].nsegs].x + dx;
										int y2 = net->connect[icon].vtx[net->connect[icon].nsegs].y + dy;
										int l2 = net->connect[icon].vtx[net->connect[icon].nsegs].pad_layer;
										id sel_id;	// id of selected item
										id pad_id( ID_PART, ID_PAD, 0, 0, 0 );	// test for hit on pad
										void * ptr = m_dlist->TestSelect( x1, y1, &sel_id, &m_sel_layer, NULL, NULL, &pad_id );
										if( ptr && sel_id.type == ID_PART && sel_id.st == ID_PAD )
										{
											// see if we can connect to this pin
											cpart * pp1 = (cpart*)ptr;
											cnet * nn1 = pp1->pin[sel_id.i].net;
											CString pin_name = pp1->shape->GetPinNameByIndex( sel_id.i );
											if( m_Doc->m_plist->TestHitOnPad( pp1, &pin_name, x1, y1, l1 ) >= 0 )
											{
												if (!nn1)
												{
													CString s = net->name+"_$Cn";
													nn1 = m_Doc->m_nlist->GetNetPtrByName(&s);
													if (!nn1)
														nn1 = m_Doc->m_nlist->AddNet( s, net->def_w, net->def_via_w, net->def_via_hole_w );
													m_Doc->m_nlist->AddNetPin(nn1, &pp1->ref_des, &pin_name, 0 ); 
												}
												int ip = m_Doc->m_nlist->GetNetPinIndex(nn1, &pp1->ref_des, &pin_name);
												CPoint p0 = m_Doc->m_plist->GetPinPoint( pp1, sel_id.i, pp1->side, pp1->angle );
												int new_ic = m_Doc->m_nlist->AddNetStub( nn1, ip, p0.x, p0.y );
												for (int seg=0; seg<=net->connect[Sid.i].nsegs; seg++)
												{
													int w,l;
													if (seg > 0)
													{
														l = net->connect[Sid.i].seg[seg-1].layer;
														w = net->connect[Sid.i].seg[seg-1].width;
													}
													else
													{
														l = net->connect[Sid.i].seg[seg].layer;
														w = net->connect[Sid.i].seg[seg].width;
													}
													m_Doc->m_nlist->AppendSegment( nn1, new_ic, net->connect[Sid.i].vtx[seg].x+dx,
																								net->connect[Sid.i].vtx[seg].y+dy,l,w);
												}
												void * ptr = m_dlist->TestSelect( x2, y2, &sel_id, &m_sel_layer, NULL, NULL, &pad_id );
												if( ptr && sel_id.type == ID_PART && sel_id.st == ID_PAD )
												{
													cpart * pp2 = (cpart*)ptr;
													cnet * nn2 = pp2->pin[sel_id.i].net;
													pin_name = pp2->shape->GetPinNameByIndex( sel_id.i );
													if( m_Doc->m_plist->TestHitOnPad( pp2, &pin_name, x2, y2, l2 ) >= 0 )
													{
														if (!nn2)
														{
															m_Doc->m_nlist->AddNetPin(nn1, &pp2->ref_des, &pin_name, 0 );
															nn2 = nn1;
														}
														if ( nn2 == nn1 )
														{
															nn1->connect[new_ic].end_pin = m_Doc->m_nlist->GetNetPinIndex(nn2, &pp2->ref_des, &pin_name);
															nn1->connect[new_ic].end_pin_shape = sel_id.i;
															nn1->connect[new_ic].vtx[nn1->connect[new_ic].nsegs].pad_layer = l2;
														}
													}
												}
												for( int ii=0; ii<nn1->connect[new_ic].nsegs; ii++ )
												{
													id sel( ID_NET, ID_CONNECT, new_ic, ID_SEG, ii );
													NewSelect( nn1, &sel, 0, 0 );
												}
											}
										}
										for( int ii=0; ii<net->connect[icon].nsegs; ii++ )
										{
											if( net->connect[icon].seg[ii].selected )
											{
												id sel( ID_NET, ID_CONNECT, icon, ID_SEG, ii );
												UnSelect( net, &sel );
											}
											if( net->connect[icon].vtx[ii+1].selected )
											{
												id sel( ID_NET, ID_CONNECT, icon, ID_VERTEX, ii+1 );
												UnSelect( net, &sel );
											}
										}
									}
						}
					SaveUndoInfoForGroup( UNDO_GROUP_ADD, m_Doc->m_undo_list );
				}
				if( getbit(m_sel_flags,FLAG_SEL_PART) && m_sel_count == 1 )
					SetCursorMode( CUR_PART_SELECTED );
				else
					SetCursorMode( CUR_GROUP_SELECTED );
				m_dlist->SetLayerVisible( LAY_RAT_LINE, m_Doc->m_vis[LAY_RAT_LINE] );
				m_Doc->m_nlist->OptimizeConnections(	m_Doc->m_auto_ratline_disable,
													m_Doc->m_auto_ratline_min_pins, TRUE  );
				HighlightGroup();
				m_Doc->ProjectModified( TRUE );
			}
			else if( m_cursor_mode == CUR_MOVE_ORIGIN )
			{
				// complete move
				SetCursorMode( CUR_NONE_SELECTED );
				CPoint p = m_dlist->WindowToPCB( point );
				m_Doc->m_dlist->StopDragging();
				SaveUndoInfoForMoveOrigin( -m_last_cursor_point.x, -m_last_cursor_point.y, m_Doc->m_undo_list );
				MoveOrigin( -m_last_cursor_point.x, -m_last_cursor_point.y );
				OnViewAllElements();
				m_Doc->ProjectModified( TRUE );
				return;
			}
			else if( m_cursor_mode == CUR_DRAG_REF )
			{
				// complete move
				SetCursorMode( CUR_REF_SELECTED );
				CPoint p = m_dlist->WindowToPCB( point );
				m_Doc->m_plist->StopDragging();
				int drag_angle = m_dlist->GetDragAngle();
				// if part on bottom of board, drag angle is CCW instead of CW
				if( m_Doc->m_plist->GetSide( m_sel_part ) && drag_angle )
					drag_angle = 360 - drag_angle;
				int angle = m_Doc->m_plist->GetRefAngle( m_sel_part ) + drag_angle;
				if( angle >= 360 )
					angle = angle - 360;
				// save undo info
				SaveUndoInfoForPart( m_sel_part, CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
				// now move it
				m_Doc->m_plist->MoveRefText( m_sel_part, m_last_cursor_point.x, m_last_cursor_point.y,
					angle, m_sel_part->m_ref_size, m_sel_part->m_ref_w );
				m_Doc->m_plist->SelectRefText( m_sel_part );
				m_Doc->m_plist->SelectPads( m_sel_part, 1, m_active_layer, TRANSPARENT_BACKGND );
				m_Doc->ProjectModified( TRUE );
				m_draw_layer = 0;
			}
			else if( m_cursor_mode == CUR_DRAG_VALUE )
			{
				// complete move
				SetCursorMode( CUR_VALUE_SELECTED );
				CPoint p = m_dlist->WindowToPCB( point );
				m_Doc->m_plist->StopDragging();
				int drag_angle = m_dlist->GetDragAngle();
				// if part on bottom of board, drag angle is CCW instead of CW
				if( m_Doc->m_plist->GetSide( m_sel_part ) && drag_angle )
					drag_angle = 360 - drag_angle;
				int angle = m_Doc->m_plist->GetValueAngle( m_sel_part ) + drag_angle;
				if( angle >= 360 )
					angle = angle - 360;
				// save undo info
				SaveUndoInfoForPart( m_sel_part, CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
				// now move it
				m_Doc->m_plist->MoveValueText( m_sel_part, m_last_cursor_point.x, m_last_cursor_point.y,
					angle, m_sel_part->m_value_size, m_sel_part->m_value_w );
				m_Doc->m_plist->SelectValueText( m_sel_part );
				m_Doc->m_plist->SelectPads( m_sel_part, 1, m_active_layer, TRANSPARENT_BACKGND );
				m_Doc->ProjectModified( TRUE );
				m_draw_layer = 0;
			}
			else if ( m_cursor_mode == CUR_DRAG_RAT )
			{
				// routing a ratline, add segment(s)
				pDC = GetDC();
				SetDCToWorldCoords( pDC );
				pDC->SelectClipRgn( &m_pcb_rgn );
				// get trace widths
				int w = m_routing_width;
				int via_w;
				int via_hole_w;
				if( m_dir )
				{
					via_w = m_sel_next_vtx.via_w;
					via_hole_w = m_sel_next_vtx.via_hole_w;
				}
				else
				{
					via_w = m_sel_vtx.via_w;
					via_hole_w = m_sel_vtx.via_hole_w;
				}
				if( via_w == 0 || via_hole_w == 0 )
					GetWidthsForSegment( &w, &via_w, &via_hole_w );
				cconnect * c = &m_sel_net->connect[m_sel_ic];
				// test for termination of trace
				if( c->end_pin == cconnect::NO_END && m_sel_is == c->nsegs-1 && m_dir == 0
					&& c->vtx[c->nsegs].tee_ID )
				{
					// routing branch to tee-vertex, test for hit on tee-vertex
					cnet * hit_net;
					int hit_ic, hit_iv;
					BOOL bHit = m_Doc->m_nlist->TestForHitOnVertex( m_sel_net, 0,
						m_last_cursor_point.x, m_last_cursor_point.y,
						&hit_net, &hit_ic, &hit_iv );
					if( bHit && hit_net == m_sel_net )
					{
						int tee_ic=0, tee_iv=0;
						BOOL bTeeFound = m_Doc->m_nlist->FindTeeVertexInNet( m_sel_net, c->vtx[c->nsegs].tee_ID,
							&tee_ic, &tee_iv );
						if( bTeeFound && tee_ic == hit_ic && tee_iv == hit_iv )
						{
							// now route to tee-vertex
							SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
							CPoint pi = m_snap_angle_ref;
							CPoint pf = m_last_cursor_point;
							CPoint pp = GetInflectionPoint( pi, pf, m_inflection_mode );
							BOOL insert_flag = FALSE;
							if( pp != pi )
							{
								insert_flag = m_Doc->m_nlist->InsertSegment( m_sel_net, m_sel_ic, m_sel_is, pp.x, pp.y, 
																			 mod_active_layer, w, via_w, via_hole_w, m_dir );
								if( !insert_flag )
								{
									// hit end-vertex of segment, terminate routing
									goto cancel_selection_and_goodbye;
								}
								if( m_dir == 0 )
									m_sel_is++;
							}
							insert_flag = m_Doc->m_nlist->InsertSegment( m_sel_net, m_sel_ic, m_sel_is,
																		 m_last_cursor_point.x, m_last_cursor_point.y,
																		 mod_active_layer, w, via_w, via_hole_w, m_dir );
							if( !insert_flag )
							{
								// hit end-vertex of segment, terminate routing
								goto cancel_selection_and_goodbye;
							}
							if( m_dir == 0 )
								m_sel_is++;
							// finish trace if necessary
							m_Doc->m_nlist->RouteSegment(	m_sel_net, m_sel_ic, m_sel_is,
															mod_active_layer, w, via_w, via_hole_w );
							m_Doc->m_nlist->ReconcileVia( m_sel_net, tee_ic, tee_iv, TRUE, via_w, via_hole_w );
							m_Doc->m_nlist->OptimizeConnections( m_sel_net, m_sel_ic, m_Doc->m_auto_ratline_disable,
																 m_Doc->m_auto_ratline_min_pins, TRUE  );
							goto cancel_selection_and_goodbye;
						}
					}
				}
				else if( m_dir == 0 && c->vtx[m_sel_is+1].tee_ID || m_dir == 1 && c->vtx[m_sel_is].tee_ID )
				{
					// routing ratline to tee-vertex
					int tee_iv = m_sel_is + 1 - m_dir;
					int TEE = c->vtx[tee_iv].tee_ID;
					cnet * hit_net;
					int hit_ic, hit_iv;
					BOOL bHit = m_Doc->m_nlist->TestForHitOnVertex( m_sel_net, 0,
						m_last_cursor_point.x, m_last_cursor_point.y,
						&hit_net, &hit_ic, &hit_iv );
					if( bHit && hit_net == m_sel_net && hit_ic == m_sel_ic && hit_iv == tee_iv )
					{
						// now route to tee-vertex
						SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
						CPoint pi = m_snap_angle_ref;
						CPoint pf = m_last_cursor_point;
						CPoint pp = GetInflectionPoint( pi, pf, m_inflection_mode );
						BOOL insert_flag = FALSE;
						if( pp != pi )
						{
							insert_flag = m_Doc->m_nlist->InsertSegment(  m_sel_net, m_sel_ic, m_sel_is, pp.x, pp.y, 
																		  mod_active_layer, w, via_w, via_hole_w, m_dir );
							if( !insert_flag )
							{
								// hit end-vertex of segment, terminate routing
								goto cancel_selection_and_goodbye;
							}
							if( m_dir == 0 )
								m_sel_is++;
						}
						insert_flag = m_Doc->m_nlist->InsertSegment( m_sel_net, m_sel_ic, m_sel_is,
																	 m_last_cursor_point.x, m_last_cursor_point.y,
																	 mod_active_layer, w, via_w, via_hole_w, m_dir );
						if( !insert_flag )
						{
							// hit end-vertex of segment, terminate routing
							goto cancel_selection_and_goodbye;
						}
						if( m_dir == 0 )
							m_sel_is++;
						// finish trace if necessary
						m_Doc->m_nlist->RouteSegment(	m_sel_net, m_sel_ic, m_sel_is,
														mod_active_layer, w, via_w, via_hole_w );
						m_Doc->m_nlist->OptimizeConnections( m_sel_net, m_sel_ic, m_Doc->m_auto_ratline_disable,
														 m_Doc->m_auto_ratline_min_pins, TRUE  );
						goto cancel_selection_and_goodbye;
					}
				}
				else if( m_sel_is == 0 && m_dir == 1 || m_sel_is == c->nsegs-1 && m_dir == 0 )
				{
					// routing ratline at end of trace, test for hit on any pad in net
					int part_pin_i=-1;
					int ip = m_Doc->m_nlist->TestHitOnAnyPadInNet( m_last_cursor_point.x,
						m_last_cursor_point.y,
						m_active_layer, m_sel_net, &part_pin_i );
					int ns = m_sel_con.nsegs;
					if( ip != -1 )
					{
						// hit on pad in net, see if this is our starting pad
						if( ns < 3 && (m_dir == 0 && ip == m_sel_net->connect[m_sel_ic].start_pin
							|| m_dir == 1 && ip == m_sel_net->connect[m_sel_ic].end_pin) )
						{
							// starting pin with too few segments, don't route to pin
						}
						else
						{
							// route to pin
							// see if this is our destination
							if( !(m_dir == 0 && ip == m_sel_net->connect[m_sel_ic].start_pin
								|| m_dir == 1 && ip == m_sel_net->connect[m_sel_ic].end_pin) )
							{
								// no, change connection to this pin unless it is the starting pin
								cpart * hit_part = m_sel_net->pin[ip].part;
								CString * hit_pin_name = &m_sel_net->pin[ip].pin_name;
								m_Doc->m_nlist->ChangeConnectionPin( m_sel_net, m_sel_ic, 1-m_dir, m_sel_net, hit_part, hit_pin_name, m_last_cursor_point.x, m_last_cursor_point.y );
							}
							// now route to destination pin
							SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
							CPoint pi = m_snap_angle_ref;
							CPoint pf = m_last_cursor_point;
							CPoint pp = GetInflectionPoint( pi, pf, m_inflection_mode );
							BOOL insert_flag = FALSE;
							if( pp != pi )
							{
								insert_flag = m_Doc->m_nlist->InsertSegment( m_sel_net, m_sel_ic, m_sel_is, pp.x, pp.y, 
																			 mod_active_layer, w, via_w, via_hole_w, m_dir );
								if( !insert_flag )
								{
									// hit end-vertex of segment, terminate routing
									goto cancel_selection_and_goodbye;
								}
								if( m_dir == 0 )
									m_sel_is++;
							}
							insert_flag = m_Doc->m_nlist->InsertSegment( m_sel_net, m_sel_ic, m_sel_is,
																		 m_last_cursor_point.x, m_last_cursor_point.y,
																		 mod_active_layer, w, via_w, via_hole_w, m_dir );
							if( !insert_flag )
							{
								// hit end-vertex of segment, terminate routing
								goto cancel_selection_and_goodbye;
							}
							if( m_dir == 0 )
								m_sel_is++;
							// finish trace to pad if necessary
							m_Doc->m_nlist->RouteSegment(	m_sel_net, m_sel_ic, m_sel_is,
															mod_active_layer, w, via_w, via_hole_w );
							m_Doc->m_nlist->DrawConnection( m_sel_net, m_sel_ic );
							m_Doc->m_nlist->OptimizeConnections( m_sel_net, m_sel_ic, m_Doc->m_auto_ratline_disable,
														 m_Doc->m_auto_ratline_min_pins, TRUE  );
							goto cancel_selection_and_goodbye;
						}
					}
				}
				// trace was not terminated, insert segment and continue routing
				SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
				CPoint pi = m_snap_angle_ref;
				CPoint pf = m_last_cursor_point;
				CPoint pp = GetInflectionPoint( pi, pf, m_inflection_mode );
				BOOL insert_flag = FALSE;
				if( pp != pi )
				{
					insert_flag = m_Doc->m_nlist->InsertSegment( m_sel_net, m_sel_ic, m_sel_is, pp.x, pp.y, 
																 mod_active_layer, 
																 w, via_w, via_hole_w, m_dir );
					if( !insert_flag )
					{
						// hit end-vertex of segment, terminate routing
						goto cancel_selection_and_goodbye;
					}				
					if( m_dir == 0 )
						m_sel_is++;
				}
				insert_flag = m_Doc->m_nlist->InsertSegment( m_sel_net, m_sel_ic, m_sel_is,
															 m_last_cursor_point.x, m_last_cursor_point.y,
															mod_active_layer, 
															 w, via_w, via_hole_w, m_dir );
				if( !insert_flag )
				{
					// hit end-vertex of segment, terminate routing
					goto cancel_selection_and_goodbye;
				}			
				if( m_dir == 0 )
					m_sel_is++;
				m_Doc->m_nlist->StartDraggingSegment( pDC, m_sel_net,
					m_sel_id.i, m_sel_is,
					m_last_cursor_point.x, m_last_cursor_point.y, m_active_layer,
					LAY_SELECTION, w,
					m_active_layer, via_w, via_hole_w, m_dir, 2 );
				m_Doc->m_nlist->HighlightNet( m_sel_net, TRANSPARENT_LAYER );
				m_Doc->m_nlist->HighlightNetConnections( m_sel_net, TRANSPARENT_LAYER, 0, m_sel_ic, m_sel_is );
				m_Doc->m_plist->HighlightAllPadsOnNet( m_sel_net, 1, mod_active_layer, TRANSPARENT_LAYER );
				m_snap_angle_ref = m_last_cursor_point;
				m_Doc->ProjectModified( TRUE );
			}
			else if( m_cursor_mode == CUR_DRAG_VTX_INSERT )
			{
				// add trace segment and vertex
				pDC = GetDC();
				SetDCToWorldCoords( pDC );
				pDC->SelectClipRgn( &m_pcb_rgn );
				m_dlist->StopDragging();

				// make undo record
				SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
				int layer = m_sel_net->connect[m_sel_ic].seg[m_sel_is].layer;
				int w = m_sel_net->connect[m_sel_ic].seg[m_sel_is].width;
				int insert_flag = m_Doc->m_nlist->InsertSegment( m_sel_net, m_sel_ic, m_sel_is,
					m_last_cursor_point.x, m_last_cursor_point.y,
					layer, w, 0, 0, m_dir );
				m_sel_id.Set(ID_NET, ID_CONNECT, m_sel_ic, ID_VERTEX, m_sel_is+1);
				m_Doc->m_nlist->HighlightVertex( m_sel_net, m_sel_ic, m_sel_iv );
				SetCursorMode(CUR_VTX_SELECTED);
				m_Doc->ProjectModified( TRUE );
				VertexMoved();//CUR_DRAG_VTX_INSERT
			}
			else if( m_cursor_mode == CUR_ADD_OP )
			{
				// place first corner of board outline
				pDC = GetDC();
				SetDCToWorldCoords( pDC );
				pDC->SelectClipRgn( &m_pcb_rgn );
				CPoint p;
				p = m_last_cursor_point;
				// ID
				m_sel_id.st = ID_GRAPHIC;
				if( m_polyline_layer == LAY_SM_TOP || m_polyline_layer == LAY_SM_BOTTOM )
					m_sel_id.st = ID_SM_CUTOUT;
				else if( m_polyline_layer == LAY_BOARD_OUTLINE )
					m_sel_id.st = ID_BOARD;
				//
				if( m_sel_id.ii )
				{
					m_Doc->m_outline_poly[m_sel_id.i].AppendCorner( p.x, p.y, m_polyline_style, TRUE );
				}
				else
				{
					m_Doc->m_outline_poly[m_sel_id.i].Start( m_polyline_layer, m_polyline_width, 20*NM_PER_MIL, p.x, p.y, m_polyline_hatch, &m_sel_id, NULL );
					m_sel_id.ii = 0;
				}
				m_sel_id.sst = ID_CORNER;
				
				m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
				SetCursorMode( CUR_DRAG_OP_1 );
				m_Doc->ProjectModified( TRUE );
				m_snap_angle_ref = m_last_cursor_point;
			}
			else if( m_cursor_mode == CUR_DRAG_OP_1 )
			{
				m_draw_layer = m_Doc->m_outline_poly[m_sel_id.i].GetLayer();//CUR_DRAG_OP_1
				// place second corner of outline
				pDC = GetDC();
				SetDCToWorldCoords( pDC );
				pDC->SelectClipRgn( &m_pcb_rgn );
				CPoint p;
				p = m_last_cursor_point;
				m_Doc->m_outline_poly[m_sel_id.i].AppendCorner( p.x, p.y, m_polyline_style );
				m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
				m_sel_id.ii++;
				SetCursorMode( CUR_DRAG_OP );
				m_Doc->ProjectModified( TRUE );
				m_snap_angle_ref = m_last_cursor_point;
			}
			else if( m_cursor_mode == CUR_DRAG_OP )
			{
				m_draw_layer = m_Doc->m_outline_poly[m_sel_id.i].GetLayer(); // CUR_DRAG_OP
				// place subsequent corners of board outline
				pDC = GetDC();
				SetDCToWorldCoords( pDC );
				pDC->SelectClipRgn( &m_pcb_rgn );
				CPoint p;
				p = m_last_cursor_point;
				if( p.x == m_Doc->m_outline_poly[m_sel_id.i].GetX(0)
					&& p.y == m_Doc->m_outline_poly[m_sel_id.i].GetY(0) )
				{
					// this point is the start point, close the polyline and quit
					SaveUndoInfoForOutlinePoly( UNDO_OP, TRUE, m_Doc->m_undo_list );
					m_Doc->m_outline_poly[m_sel_id.i].Close( m_polyline_style );
					SetCursorMode( CUR_NONE_SELECTED );
					m_Doc->m_dlist->StopDragging();
				}
				else
				{
					// add corner to polyline
					m_Doc->m_outline_poly[m_sel_id.i].AppendCorner( p.x, p.y, m_polyline_style );
					m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
					m_sel_id.ii++;
					m_snap_angle_ref = m_last_cursor_point;
				}
				m_Doc->ProjectModified( TRUE );
			}
			else if( m_cursor_mode == CUR_DRAG_OP_INSERT )
			{
				m_draw_layer = m_Doc->m_outline_poly[m_sel_id.i].GetLayer();// CUR_DRAG_OP_INSERT
				pDC = GetDC();
				SetDCToWorldCoords( pDC );
				pDC->SelectClipRgn( &m_pcb_rgn );
				CPoint p;
				p = m_last_cursor_point;
				m_dlist->StopDragging();
				SaveUndoInfoForOutlinePoly( UNDO_OP, TRUE, m_Doc->m_undo_list );
				int inext = m_Doc->m_outline_poly[m_sel_id.i].GetIndexCornerNext( m_sel_id.ii );
				m_Doc->m_outline_poly[m_sel_id.i].InsertCorner( inext, p.x, p.y );
				m_Doc->m_outline_poly[m_sel_id.i].HighlightCorner( inext );
				SetCursorMode( CUR_OP_CORNER_SELECTED );
				m_sel_id.Set( ID_POLYLINE, m_sel_id.st, m_sel_id.i, ID_CORNER, inext );
				m_Doc->ProjectModified( TRUE );
			}
			else if( m_cursor_mode == CUR_DRAG_OP_MOVE )
			{
				m_draw_layer = m_Doc->m_outline_poly[m_sel_id.i].GetLayer();//CUR_DRAG_OP_MOVE
				pDC = GetDC();
				SetDCToWorldCoords( pDC );
				pDC->SelectClipRgn( &m_pcb_rgn );
				CPoint p;
				p = m_last_cursor_point;
				m_dlist->StopDragging();
				SaveUndoInfoForOutlinePoly( UNDO_OP, TRUE, m_Doc->m_undo_list );
				m_Doc->m_outline_poly[m_sel_id.i].MoveCorner( m_sel_id.ii, p.x, p.y );
				m_Doc->m_outline_poly[m_sel_id.i].HighlightCorner( m_sel_id.ii, m_Doc->m_outline_poly[m_sel_id.i].GetW() );
				SetCursorMode( CUR_OP_CORNER_SELECTED );
				m_Doc->ProjectModified( TRUE );
			}
			else if( m_cursor_mode == CUR_ADD_AREA )
			{
				m_draw_layer = LAY_HILITE;//CUR_ADD_AREA
				pDC = GetDC();
				SetDCToWorldCoords( pDC );
				pDC->SelectClipRgn( &m_pcb_rgn );
				CPoint p;
				p = m_last_cursor_point;
				int m_old_merge = -1;
				if( m_sel_ia < m_sel_net->nareas )
					m_old_merge = m_sel_net->area[m_sel_ia].poly->GetMerge();
				int iarea = m_Doc->m_nlist->AddArea( m_sel_net, mod_active_layer, p.x, p.y, m_polyline_hatch );
				if( iarea >= 0 )
				{
					m_sel_net->area[iarea].poly->SetW( m_polyline_width );
					m_sel_net->area[iarea].poly->SetMerge( m_old_merge );
					m_sel_id.Set( m_sel_net->id.type, ID_AREA, iarea, ID_CORNER, 1 );
					m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
					SetCursorMode( CUR_DRAG_AREA_1 );
					m_Doc->ProjectModified( TRUE );
					m_snap_angle_ref = m_last_cursor_point;
				}
			}
			else if( m_cursor_mode == CUR_DRAG_AREA_1 )
			{
				m_draw_layer = m_sel_net->area[m_sel_ia].poly->GetLayer();// CUR_DRAG_AREA_1
				pDC = GetDC();
				SetDCToWorldCoords( pDC );
				pDC->SelectClipRgn( &m_pcb_rgn );
				CPoint p;
				p = m_last_cursor_point;
				m_Doc->m_nlist->AppendAreaCorner( m_sel_net, m_sel_ia, p.x, p.y, m_polyline_style );
				m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
				m_sel_id.ii = 2;
				SetCursorMode( CUR_DRAG_AREA );
				m_Doc->ProjectModified( TRUE );
				m_snap_angle_ref = m_last_cursor_point;
			}
			else if( m_cursor_mode == CUR_DRAG_AREA )
			{
				m_draw_layer = m_sel_net->area[m_sel_ia].poly->GetLayer(); // CUR_DRAG_AREA
				pDC = GetDC();
				SetDCToWorldCoords( pDC );
				pDC->SelectClipRgn( &m_pcb_rgn );
				CPoint p;
				p = m_last_cursor_point;
				if( p.x == m_sel_net->area[m_sel_id.i].poly->GetX(0)
					&& p.y == m_sel_net->area[m_sel_id.i].poly->GetY(0) )
				{
					// cursor point is first point, close area
					SaveUndoInfoForAllAreasInNet( m_sel_net, TRUE, m_Doc->m_undo_list );
					m_Doc->m_nlist->CompleteArea( m_sel_net, m_sel_ia, m_polyline_style );

					m_Doc->m_nlist->AreaPolygonModified( m_sel_net, m_sel_ia, TRUE, TRUE );
					m_Doc->m_dlist->StopDragging();
					m_Doc->m_nlist->OptimizeConnections( m_sel_net, -1, m_Doc->m_auto_ratline_disable,
															m_Doc->m_auto_ratline_min_pins, TRUE );	
					CancelSelection();
				}
				else
				{
					// add cursor point
					m_Doc->m_nlist->AppendAreaCorner( m_sel_net, m_sel_ia, p.x, p.y, m_polyline_style );
					m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
					m_sel_id.ii = m_sel_id.ii + 1;
					SetCursorMode( CUR_DRAG_AREA );
					m_snap_angle_ref = m_last_cursor_point;
				}
				m_Doc->ProjectModified( TRUE );
			}
			else if( m_cursor_mode == CUR_DRAG_AREA_MOVE )
			{
				m_draw_layer = m_sel_net->area[m_sel_ia].poly->GetLayer(); // CUR_DRAG_AREA_MOVE
				SaveUndoInfoForAllAreasInNet( m_sel_net, TRUE, m_Doc->m_undo_list );
				pDC = GetDC();
				SetDCToWorldCoords( pDC );
				pDC->SelectClipRgn( &m_pcb_rgn );
				CPoint p;
				p = m_last_cursor_point;
				m_dlist->StopDragging();
				m_Doc->m_nlist->MoveAreaCorner( m_sel_net, m_sel_ia, m_sel_is, p.x, p.y );
				int ret = m_Doc->m_nlist->AreaPolygonModified( m_sel_net, m_sel_ia, FALSE, TRUE, m_sel_is );
				if( ret == -1 )
				{
					// error
					AfxMessageBox( "Error: Unable to clip polygon due to intersecting arc" );
					CancelSelection();
					m_Doc->OnEditUndo();
				}
				else
				{
					m_Doc->m_nlist->OptimizeConnections( m_sel_net, -1, m_Doc->m_auto_ratline_disable,
															m_Doc->m_auto_ratline_min_pins, TRUE );
					TryToReselectAreaCorner( p.x, p.y );
				}
				m_Doc->ProjectModified( TRUE );
			}
			else if( m_cursor_mode == CUR_DRAG_AREA_INSERT )
			{
				m_draw_layer = m_sel_net->area[m_sel_ia].poly->GetLayer(); // CUR_DRAG_AREA_INSERT
				SaveUndoInfoForAllAreasInNet( m_sel_net, TRUE, m_Doc->m_undo_list );
				pDC = GetDC();
				SetDCToWorldCoords( pDC );
				pDC->SelectClipRgn( &m_pcb_rgn );
				CPoint p;
				p = m_last_cursor_point;
				m_dlist->StopDragging();
				m_Doc->m_nlist->InsertAreaCorner( m_sel_net, m_sel_ia, m_sel_net->area[m_sel_ia].poly->GetIndexCornerNext(m_sel_is), p.x, p.y, CPolyLine::STRAIGHT, TRUE );
				int ret = m_Doc->m_nlist->AreaPolygonModified( m_sel_net, m_sel_ia, FALSE, TRUE, m_sel_is );
				if( ret == -1 )
				{
					// error
					AfxMessageBox( "Error: Unable to clip polygon due to intersecting arc" );
					CancelSelection();
					m_Doc->OnEditUndo();
				}
				else
				{
					m_Doc->m_nlist->OptimizeConnections( m_sel_net, -1, m_Doc->m_auto_ratline_disable,
															m_Doc->m_auto_ratline_min_pins, TRUE );
					TryToReselectAreaCorner( p.x, p.y );
				}
				m_Doc->ProjectModified( TRUE );
			}
			else if( m_cursor_mode == CUR_ADD_AREA_CUTOUT )
			{
				m_draw_layer = LAY_HILITE;// CUR_ADD_AREA_CUTOUT
				pDC = GetDC();
				SetDCToWorldCoords( pDC );
				pDC->SelectClipRgn( &m_pcb_rgn );
				CPoint p;
				p = m_last_cursor_point;
				int ia = m_sel_id.i;
				carea * a = &m_sel_net->area[ia];
				m_Doc->m_nlist->AppendAreaCorner( m_sel_net, ia, p.x, p.y, m_polyline_style );
				m_Doc->m_nlist->HighlightAreaSides( m_sel_net, ia, 0 );
				m_sel_id.Set( m_sel_net->id.type, ID_AREA, ia, ID_CORNER, a->poly->GetNumCorners()-1 );
				m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
				SetCursorMode( CUR_DRAG_AREA_CUTOUT_1 );
				if( fRepour )
				{
					fRepour = m_Doc->m_nlist->AddArea( m_sel_net, a->poly->GetLayer(), p.x, p.y, CPolyLine::DIAGONAL_EDGE );
				}
				m_Doc->ProjectModified( TRUE );
				m_snap_angle_ref = m_last_cursor_point;
			}
			else if( m_cursor_mode == CUR_DRAG_AREA_CUTOUT_1 )
			{
				m_draw_layer = m_sel_net->area[m_sel_ia].poly->GetLayer();// CUR_DRAG_AREA_CUTOUT_1
				pDC = GetDC();
				SetDCToWorldCoords( pDC );
				pDC->SelectClipRgn( &m_pcb_rgn );
				CPoint p;
				p = m_last_cursor_point;
				m_Doc->m_nlist->AppendAreaCorner( m_sel_net, m_sel_ia, p.x, p.y, m_polyline_style );
				m_Doc->m_nlist->HighlightAreaSides( m_sel_net, m_sel_ia, 0 );
				m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
				m_sel_id.ii = 2;
				SetCursorMode( CUR_DRAG_AREA_CUTOUT );
				if( fRepour )
				{
					m_Doc->m_nlist->AppendAreaCorner( m_sel_net, fRepour, p.x, p.y, m_polyline_style );
				}
				m_Doc->ProjectModified( TRUE );
				m_snap_angle_ref = m_last_cursor_point;
			}
			else if( m_cursor_mode == CUR_DRAG_AREA_CUTOUT )
			{
				m_draw_layer = m_sel_net->area[m_sel_ia].poly->GetLayer();// CUR_DRAG_AREA_CUTOUT
				pDC = GetDC();
				SetDCToWorldCoords( pDC );
				pDC->SelectClipRgn( &m_pcb_rgn );
				CPoint p;
				p = m_last_cursor_point;
				CPolyLine * poly = m_sel_net->area[m_sel_id.i].poly;
				int icontour = poly->GetContour( poly->GetNumCorners()-1 );
				int istart = poly->GetContourStart( icontour );
				if( p.x == poly->GetX(istart)
					&& p.y == poly->GetY(istart) )
				{
					// cursor point is first point, close area
					SaveUndoInfoForAllAreasInNet( m_sel_net, TRUE, m_Doc->m_undo_list );
					m_Doc->m_nlist->CompleteArea( m_sel_net, m_sel_ia, m_polyline_style );
					m_Doc->m_dlist->StopDragging();
					int n_old_areas = m_sel_net->area.GetSize();
					int m_old_merge = poly->GetMerge();
					int ret = m_Doc->m_nlist->ClipAreaPolygon( m_sel_net, m_sel_ia, -1, FALSE, FALSE  );
					// update pointer
					poly = m_sel_net->area[m_sel_ia].poly;
					// set merge
					poly->SetMerge( m_old_merge );
					if( fRepour )
					{
						m_Doc->m_nlist->CompleteArea( m_sel_net, fRepour, m_polyline_style );
						fRepour = 0;
					}
					if( ret == -1 )
					{
						// error
						AfxMessageBox( "Error: Unable to clip polygon due to intersecting arc" );
						////m_Doc->OnEditUndo();
					}
					else
					{
						m_Doc->m_nlist->OptimizeConnections(  m_sel_net, -1, m_Doc->m_auto_ratline_disable,
															m_Doc->m_auto_ratline_min_pins, TRUE  );
					}	
					CancelSelection();
				}
				else
				{
					// add cursor point
					m_Doc->m_nlist->AppendAreaCorner( m_sel_net, m_sel_ia, p.x, p.y, m_polyline_style );
					m_Doc->m_nlist->HighlightAreaSides( m_sel_net, m_sel_ia, 0 );
					m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
					m_sel_id.ii = m_sel_id.ii + 1;
					SetCursorMode( CUR_DRAG_AREA_CUTOUT );
					m_snap_angle_ref = m_last_cursor_point;
					if( fRepour )
					{
						m_Doc->m_nlist->AppendAreaCorner( m_sel_net, fRepour, p.x, p.y, m_polyline_style );
					}
				}
				m_Doc->ProjectModified( TRUE );
			}
			else if( m_cursor_mode == CUR_ADD_AREA_CUTOUT )
			{
				m_draw_layer = LAY_HILITE;//CUR_ADD_AREA_CUTOUT
				pDC = GetDC();
				SetDCToWorldCoords( pDC );
				pDC->SelectClipRgn( &m_pcb_rgn );
				CPoint p;
				p = m_last_cursor_point;
				int ia = m_sel_id.i;
				carea * a = &m_sel_net->area[ia];
				m_Doc->m_nlist->AppendAreaCorner( m_sel_net, ia, p.x, p.y, m_polyline_style );
				m_sel_id.Set( m_sel_net->id.type, ID_AREA, ia, ID_CORNER, a->poly->GetNumCorners()-1 );
				m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
				SetCursorMode( CUR_DRAG_AREA_1 );
				m_Doc->ProjectModified( TRUE );
				m_snap_angle_ref = m_last_cursor_point;
			}
			else if( m_cursor_mode == CUR_DRAG_VTX )
			{
				// move vertex by modifying adjacent segments and reconciling via
				SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
				CPoint p;
				p = m_last_cursor_point;
				m_Doc->m_nlist->MoveVertex( m_sel_net, m_sel_ic, m_sel_is, p.x, p.y );
				//m_Doc->m_nlist->SetAreaConnections( m_sel_net );
				m_Doc->m_nlist->HighlightVertex( m_sel_net, m_sel_ic, m_sel_iv );
				SetCursorMode( CUR_VTX_SELECTED );
				m_Doc->ProjectModified( TRUE );
			}
			else if( m_cursor_mode == CUR_MOVE_SEGMENT )
			{
				// move vertex by modifying adjacent segments and reconciling via
				m_Doc->m_dlist->StopDragging();
				SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
				CPoint cpi;
				CPoint cpf;
				m_Doc->m_dlist->Get_Endpoints(&cpi, &cpf);
				int ic = m_sel_id.i;
				int ivtx = m_sel_id.ii;
				m_Doc->m_nlist->MoveVertex( m_sel_net, m_sel_ic, m_sel_is,   cpi.x, cpi.y );
				ASSERT(cpi != cpf);			// Should be at least one grid snap apart.
				m_Doc->m_nlist->MoveVertex( m_sel_net, m_sel_ic, m_sel_is+1, cpf.x, cpf.y );
				m_Doc->m_nlist->DrawConnection( m_sel_net, m_sel_ic );
				m_insert_seg_len = (int)sqrt((double)(cpi.x-cpf.x)*(double)(cpi.x-cpf.x) + (double)(cpi.y-cpf.y)*(double)(cpi.y-cpf.y));
				m_Doc->m_nlist->HighlightSegment ( m_sel_net, m_sel_ic, m_sel_is );
				SetCursorMode( CUR_SEG_SELECTED );
				m_Doc->ProjectModified( TRUE );
			}
			else if( m_cursor_mode == CUR_DRAG_END_VTX )
			{
				// move end-vertex of stub trace
				m_Doc->m_dlist->StopDragging();
				SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
				CPoint p;
				p = m_last_cursor_point;
				m_Doc->m_nlist->MoveEndVertex( m_sel_net, m_sel_id.i, m_sel_id.ii, p.x, p.y );
				for( int con=0; con<m_sel_net->nconnects; con++ )
				{
					if( con == m_sel_id.i )
						continue;
					cconnect * c = &m_sel_net->connect[con];
					for( int seg=1; seg<=c->nsegs; seg++ )
					{
						if( m_sel_con.seg[m_sel_id.ii-1].layer == c->seg[seg-1].layer )
						{// same layers				
							if( abs( c->vtx[seg].x - p.x ) < (c->seg[seg-1].width>>2) && 
								abs( c->vtx[seg].y - p.y ) < (c->seg[seg-1].width>>2) )
							{
								if( m_sel_iv )
								{
									SetCursorMode( CUR_DRAG_STUB );
									m_bLButtonDown = TRUE;
									en_branch = BRANCH_TO_VERTEX;
									m_snap_angle_ref.x = m_sel_vtx.x;
									m_snap_angle_ref.y = m_sel_vtx.y;
									m_routing_width = m_sel_con.seg[m_sel_iv-1].width;
									m_active_layer = m_sel_con.seg[m_sel_iv-1].layer;
									OnLButtonUp(nFlags, point);
								}
								return; 
							}
						}
					}
				}
				for( int con=0; con<m_sel_net->nconnects; con++ )
				{
					if( con == m_sel_id.i )
						continue;
					cconnect * c = &m_sel_net->connect[con];
					for( int seg=1; seg<=c->nsegs; seg++ )
					{
						if( m_sel_con.seg[m_sel_id.ii-1].layer != c->seg[seg-1].layer )
						{// diff layers
							if( abs( c->vtx[seg].x - p.x ) < (c->seg[seg-1].width>>2) && 
								abs( c->vtx[seg].y - p.y ) < (c->seg[seg-1].width>>2) )
							{
								if( m_sel_iv )
								{
									SetCursorMode( CUR_DRAG_STUB );
									m_bLButtonDown = TRUE;
									en_branch = BRANCH_TO_VERTEX;
									m_snap_angle_ref.x = m_sel_vtx.x;
									m_snap_angle_ref.y = m_sel_vtx.y;
									m_routing_width = m_sel_con.seg[m_sel_iv-1].width;
									m_active_layer = m_sel_con.seg[m_sel_iv-1].layer;
									OnLButtonUp(nFlags, point);
								}
								return; 
							}
						}
					}
				}
				SetCursorMode( CUR_END_VTX_SELECTED );
				if( m_Doc->m_vis[LAY_RAT_LINE] )
					m_sel_ic = m_Doc->m_nlist->OptimizeConnections( m_sel_net, m_sel_ic, m_Doc->m_auto_ratline_disable, m_Doc->m_auto_ratline_min_pins );
				m_Doc->m_dlist->CancelHighLight();
				if( m_sel_ic >= 0 )
					m_Doc->m_nlist->HighlightVertex( m_sel_net, m_sel_ic, m_sel_iv );
				m_Doc->ProjectModified( TRUE );
			}
			else if( m_cursor_mode == CUR_DRAG_CONNECT )
			{
				// dragging ratline to make a new connection
				// test for hit on pin
				CPoint p = m_dlist->WindowToPCB( point );
				id sel_id;	// id of selected item
				id pad_id( ID_PART, ID_PAD, 0, 0, 0 );	// force selection of pad
				void * ptr = m_dlist->TestSelect( p.x, p.y, &sel_id, &m_sel_layer, NULL, NULL, &pad_id );
				if( ptr )
				{
					if( sel_id.type == ID_PART && sel_id.st == ID_PAD )
					{
						// hit on pin
						cpart * new_sel_part = (cpart*)ptr;
						cnet * new_sel_net = (cnet*)new_sel_part->pin[sel_id.i].net;
						if( m_sel_id.type == ID_NET  && m_sel_id.st == ID_CONNECT
							&& m_sel_id.sst == ID_VERTEX )
						{
							// dragging ratline from vertex of a trace
							if( new_sel_net && new_sel_net != m_sel_net )
							{
								// pin assigned to different net, can't connect it
								CString mess;
								mess.Format( "You are trying to connect a trace to a pin on a different net\nYou must detach the pin from the net first" );
								AfxMessageBox( mess );
							}
							else if( m_sel_con.end_pin == cconnect::NO_END
								&& m_sel_iv == m_sel_con.nsegs )
							{
								BOOL np1 = FALSE;
								if( new_sel_part->shape )
									if( new_sel_part->shape->GetNumPins() != 1 )
										np1 = TRUE;
								if( new_sel_net == 0 && m_Doc->m_netlist_completed && np1 )
								{
									if( g_bShow_nl_lock_Warning )
									{
										CString str = "Sorry, netlist is protected. You can remove protection through the menu Project-->Nets-->Netlist Protected";
										CDlgMyMessageBox dlg;
										dlg.Initialize( str );
										dlg.DoModal();
										g_bShow_nl_lock_Warning = !dlg.bDontShowBoxState;
									}
								}
								else
								{
									SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
									SaveUndoInfoForPart( new_sel_part, CPartList::UNDO_PART_MODIFY, NULL, FALSE, m_Doc->m_undo_list );
									// dragging ratline from end of stub trace
									CString pin_name = new_sel_part->shape->GetPinNameByIndex( sel_id.i );
									if( new_sel_net == 0 )
									{
										m_Doc->m_nlist->AddNetPin( m_sel_net, &new_sel_part->ref_des,
											&pin_name );
									}
									CPoint p = m_Doc->m_plist->GetPinPoint( new_sel_part, sel_id.i, new_sel_part->side, new_sel_part->angle );
									int pin = m_Doc->m_nlist->GetNetPinIndex( m_sel_net, &new_sel_part->ref_des, &pin_name );
									if( pin == m_sel_con.start_pin )
									{
										// trying to connect pin to itself
										goto goodbye;
									}
									// convert to regular pin-pin trace
									if(  m_sel_con.vtx[m_sel_iv].tee_ID )
										ASSERT(0);	// error, should not be a branch end or a tee-vertex
									m_Doc->m_nlist->AppendSegment( m_sel_net, m_sel_ic, p.x, p.y,
										LAY_RAT_LINE, 0 );
									m_sel_con.vtx[m_sel_iv].force_via_flag = 0;
									m_sel_con.end_pin = pin;
									m_sel_con.end_pin_shape = sel_id.i;
									cvertex * end_v = &m_sel_con.vtx[m_sel_con.nsegs];
									end_v->pad_layer = m_Doc->m_plist->GetPinLayer( new_sel_part, sel_id.i );
									m_Doc->m_nlist->DrawConnection( m_sel_net, m_sel_ic );
									m_sel_ic = m_Doc->m_nlist->OptimizeConnections(  m_sel_net, m_sel_ic, m_Doc->m_auto_ratline_disable,
																		  m_Doc->m_auto_ratline_min_pins, TRUE  );						
									m_Doc->m_dlist->StopDragging();
									m_sel_id.Set( ID_NET, ID_CONNECT, m_sel_ic, ID_SEG, m_sel_is );
									m_Doc->m_dlist->CancelHighLight();
									m_Doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, m_sel_is, m_seg_clearance );
									SetCursorMode( CUR_RAT_SELECTED );
									m_Doc->ProjectModified( TRUE );	
								}
							}
							else
							{
								BOOL np1 = FALSE;
								if( new_sel_part->shape )
									if( new_sel_part->shape->GetNumPins() != 1 )
										np1 = TRUE;
								if( new_sel_net == 0 && m_Doc->m_netlist_completed && np1 )
								{
									if( g_bShow_nl_lock_Warning )
									{
										CString str = "Sorry, netlist is protected. You can remove protection through the menu Project-->Nets-->Netlist Protected";
										CDlgMyMessageBox dlg;
										dlg.Initialize( str );
										dlg.DoModal();
										g_bShow_nl_lock_Warning = !dlg.bDontShowBoxState;
									}
								}
								else
								{
									// dragging ratline from any other vertex to pin
									// add new stub trace from pin to tee
									SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
									SaveUndoInfoForPart( new_sel_part, CPartList::UNDO_PART_MODIFY, NULL, FALSE, m_Doc->m_undo_list );
									CString pin_name = new_sel_part->shape->GetPinNameByIndex( sel_id.i );
									CPoint pt1 = m_Doc->m_plist->GetPinPoint( new_sel_part, sel_id.i, new_sel_part->side, new_sel_part->angle );
									if( new_sel_net == 0 )
									{
										m_Doc->m_nlist->AddNetPin( m_sel_net, &new_sel_part->ref_des,
											&pin_name );
									}
									int p1 = m_Doc->m_nlist->GetNetPinIndex( m_sel_net, &new_sel_part->ref_des, &pin_name );
									int ic = m_Doc->m_nlist->AddNetStub( m_sel_net, p1, pt1.x, pt1.y );
									m_Doc->m_nlist->AppendSegment( m_sel_net, ic, m_sel_vtx.x, m_sel_vtx.y, LAY_RAT_LINE,
										1 );
									int id = m_sel_vtx.tee_ID;
									if( id == 0 )
									{
										id = m_Doc->m_nlist->GetNewTeeID();
										m_sel_vtx.tee_ID = id;
									}
									m_sel_net->connect[ic].vtx[1].tee_ID = m_sel_vtx.tee_ID;
									m_Doc->m_nlist->DrawConnection( m_sel_net, ic );
									ic = m_Doc->m_nlist->OptimizeConnections(  m_sel_net, ic, m_Doc->m_auto_ratline_disable,
											    						  m_Doc->m_auto_ratline_min_pins, TRUE  );			
									m_Doc->m_dlist->StopDragging();
									m_sel_id.Set( ID_NET, ID_CONNECT, ic, ID_SEG, 0 );
									m_Doc->m_dlist->CancelHighLight();
									m_Doc->m_nlist->HighlightSegment( m_sel_net, ic, 0, m_seg_clearance );
									SetCursorMode( CUR_RAT_SELECTED );
									m_Doc->ProjectModified( TRUE );	
								}
							}
						}
						else if( m_sel_id.type == ID_PART  && m_sel_id.st == ID_PAD )
						{
							// connecting pin to pin
							cnet * from_sel_net = (cnet*)m_sel_part->pin[m_sel_id.i].net;
							if( new_sel_net && from_sel_net && (new_sel_net != from_sel_net) )
							{
								// pins assigned to different nets, can't connect them
								CString mess;
								mess.Format( "You are trying to connect pins on different nets.\nCombine this nets?\nWARNING! This operation has no undo action." );
								int ret = AfxMessageBox( mess, MB_YESNO );
								if( ret == IDYES )
								{
									m_Doc->m_dlist->StopDragging();
									CancelSelection();
									CNetList nL(NULL,NULL);
									nL.Copy(m_Doc->m_nlist);
									for( cnet * n=nL.GetFirstNet(); n; n = nL.GetNextNet(/*LABEL*/) )
									{
										if( n->name.Compare(new_sel_net->name) == 0 )
											continue;
										if( n->name.Compare(from_sel_net->name) == 0 )
											continue;
										nL.RemoveNet(n);
									}
									m_Doc->ProjectCombineNets(&nL);	
									m_Doc->ResetUndoState();
								}
								return;
							}
							else
							{
								// see if we are trying to connect a pin to itself
								if( new_sel_part == m_sel_part && m_sel_id.i == sel_id.i )
								{
									// yes, forget it
									goto goodbye;
								}
								// we can connect these pins
								SaveUndoInfoForPart( m_sel_part, CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
								SaveUndoInfoForPart( new_sel_part, CPartList::UNDO_PART_MODIFY, NULL, FALSE, m_Doc->m_undo_list );

								// GetPinNameByIndex
								CString pin_name1 = m_sel_part->shape->GetPinNameByIndex( m_sel_id.i );
								CString pin_name2 = new_sel_part->shape->GetPinNameByIndex( sel_id.i );
								int np1 = m_sel_part->shape->GetNumPins();
								int np2 = new_sel_part->shape->GetNumPins();
								//
								if(m_Doc->m_netlist_completed == 0 || np1 == 1 || np2 == 1) 
								{
									if( new_sel_net != from_sel_net )
									{
										// one pin is unassigned, assign it to net
										if( !new_sel_net && (np2 == 1 || m_Doc->m_netlist_completed == 0))
										{
											// connecting to unassigned pin, assign it
											SaveUndoInfoForNetAndConnections( from_sel_net, CNetList::UNDO_NET_MODIFY, FALSE, m_Doc->m_undo_list );
											CString pin_name = new_sel_part->shape->GetPinNameByIndex( sel_id.i );
											m_Doc->m_nlist->AddNetPin( from_sel_net,
												&new_sel_part->ref_des,
												&pin_name );
											new_sel_net = from_sel_net;
										}
										else if( !from_sel_net && (np1 == 1 || m_Doc->m_netlist_completed == 0))
										{
											// connecting from unassigned pin, assign it
											SaveUndoInfoForNetAndConnections( new_sel_net, CNetList::UNDO_NET_MODIFY, FALSE, m_Doc->m_undo_list );
											CString pin_name = m_sel_part->shape->GetPinNameByIndex( m_sel_id.i );
											m_Doc->m_nlist->AddNetPin( new_sel_net,
												&m_sel_part->ref_des,
												&pin_name );
											from_sel_net = new_sel_net;
										}
									}
									else if( !new_sel_net && !m_sel_part->pin[m_sel_id.i].net )
									{
										// connecting 2 unassigned pins, select net
										DlgAssignNet assign_net_dlg;
										assign_net_dlg.m_map = &m_Doc->m_nlist->m_map;
										assign_net_dlg.m_nLOCK = m_Doc->m_netlist_completed;
										int ret = assign_net_dlg.DoModal();
										if( ret != IDOK )
										{
											m_Doc->m_dlist->StopDragging();
											SetCursorMode( CUR_PAD_SELECTED );
											goto goodbye;
										}
										CString name = assign_net_dlg.m_net_str;
										void * ptr;
										int test = m_Doc->m_nlist->m_map.Lookup( name, ptr );
										if( test )
										{
											// assign pins to existing net
											new_sel_net = (cnet*)ptr;
											SaveUndoInfoForNetAndConnections( new_sel_net,
												CNetList::UNDO_NET_MODIFY, FALSE, m_Doc->m_undo_list );
											m_Doc->m_nlist->AddNetPin( new_sel_net,
												&m_sel_part->ref_des,
												&pin_name1 );
											m_Doc->m_nlist->AddNetPin( new_sel_net,
												&new_sel_part->ref_des,
												&pin_name2 );
										}
										else
										{
											// make new net
											new_sel_net = m_Doc->m_nlist->AddNet( (char*)(LPCTSTR)name, 0, 0, 0 );
											SaveUndoInfoForNetAndConnections( new_sel_net,
												CNetList::UNDO_NET_ADD, FALSE, m_Doc->m_undo_list );									
											m_Doc->m_nlist->AddNetPin( new_sel_net,
												&m_sel_part->ref_des,
												&pin_name1 );
											m_Doc->m_nlist->AddNetPin( new_sel_net,
												&new_sel_part->ref_des,
												&pin_name2 );
										}
									}
								}
								// find pins in net and connect them
								int p1 = -1;
								int p2 = -1;
								if( new_sel_net )
								{
									p1 = m_Doc->m_nlist->GetNetPinIndex( new_sel_net, &m_sel_part->ref_des, &pin_name1 );
									p2 = m_Doc->m_nlist->GetNetPinIndex( new_sel_net, &new_sel_part->ref_des, &pin_name2 );
								}
								if( p1>=0 && p2>=0 )
								{
									CPoint pt1 = m_Doc->m_plist->GetPinPoint( m_sel_part, m_sel_id.i, m_sel_part->side, m_sel_part->angle );
									CPoint pt2 = m_Doc->m_plist->GetPinPoint( new_sel_part, sel_id.i, new_sel_part->side, new_sel_part->angle );
									m_Doc->m_nlist->AddNetConnect( new_sel_net, p1, p2, pt1.x, pt1.y, pt2.x, pt2.y );
								}
								else if( g_bShow_nl_lock_Warning && m_Doc->m_netlist_completed )
								{
									CString str = "\nIt is impossible to connect a pin if netlist is protected (except parts with one pin only). You can remove protection through the menu Project-->Nets-->Netlist Protected";
									CDlgMyMessageBox dlg;
									dlg.Initialize( str );
									dlg.DoModal();
									g_bShow_nl_lock_Warning = !dlg.bDontShowBoxState;
								}
								m_dlist->StopDragging();

								if( m_sel_count && m_sel_id.type == ID_PART &&
									m_sel_id.st == ID_PAD && m_sel_id.i < m_sel_part->shape->GetNumPins() )
								{
									m_Doc->m_dlist->CancelHighLight();
									m_Doc->m_plist->SelectPad( m_sel_part, m_sel_id.i, 0, 0 );
									SetCursorMode( CUR_PAD_SELECTED );
								}
								else
									CancelSelection();
							}
							m_Doc->ProjectModified( TRUE );
						}
					}
				}
			}
			else if( m_cursor_mode == CUR_DRAG_RAT_PIN )
			{
				// see if pad selected
				CPoint p = m_dlist->WindowToPCB( point );
				id sel_id;	// id of selected item
				id pad_id( ID_PART, ID_PAD, 0, 0, 0 );	// force selection of pad
				void * ptr = m_dlist->TestSelect( p.x, p.y, &sel_id, &m_sel_layer, NULL, NULL, &pad_id );
				if( ptr )
				{
					if( sel_id.type == ID_PART )
					{
						if( sel_id.st == ID_PAD )
						{
							// see if we can connect to this pin
							cpart * new_sel_part = (cpart*)ptr;
							cnet * new_sel_net = (cnet*)new_sel_part->pin[sel_id.i].net;
							CString pin_name = new_sel_part->shape->GetPinNameByIndex( sel_id.i );
							if( new_sel_net == 0 )
							{
								if( m_Doc->m_netlist_completed == 0 || new_sel_part->shape->GetNumPins() == 1 )
								{
									new_sel_net = m_sel_net;
									// unassigned pin, assign it
									SaveUndoInfoForPart( new_sel_part, CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
									SaveUndoInfoForNetAndConnections( m_sel_net,
										CNetList::UNDO_NET_MODIFY, FALSE, m_Doc->m_undo_list );
									m_Doc->m_nlist->AddNetPin( m_sel_net, &new_sel_part->ref_des, &pin_name );
								}
								else if( g_bShow_nl_lock_Warning )
								{
									CString str = "Sorry, netlist is protected.\nIt's impossible change connect to pin if netlist is protected (except parts with one pin only). You can remove protection through the menu Project-->Nets-->Netlist Protected";
									CDlgMyMessageBox dlg;
									dlg.Initialize( str );
									dlg.DoModal();
									g_bShow_nl_lock_Warning = !dlg.bDontShowBoxState;
									return;
								}
							}
							else
							{
								// pin already assigned to this net
								SaveUndoInfoForNetAndConnections( m_sel_net,
									CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
								if( new_sel_net && (new_sel_net != m_sel_net) )
								{
									SaveUndoInfoForNetAndConnections( new_sel_net,
									CNetList::UNDO_NET_MODIFY, FALSE, m_Doc->m_undo_list );
								}
							}
							m_sel_id.i = m_Doc->m_nlist->ChangeConnectionPin( m_sel_net, m_sel_ic, m_sel_is, new_sel_net, new_sel_part, &pin_name, p.x, p.y );
							if( m_sel_net != new_sel_net )
								m_sel_is = 0;
							m_sel_net = new_sel_net;
							m_dlist->StopDragging();
							m_dlist->CancelHighLight();
							SetCursorMode( CUR_RAT_SELECTED );
							m_Doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, m_sel_is, m_seg_clearance );
							m_Doc->m_nlist->SetAreaConnections( m_sel_net );
							m_Doc->ProjectModified( TRUE );
						}
					}
				}
			}
			else if( m_cursor_mode == CUR_DRAG_STUB )
			{
				int routing_layer = mod_active_layer;
				m_sel_layer = routing_layer;
				id save_sel_id = m_sel_id;
				cnet * save_net = m_sel_net;
				if( m_sel_is != 0 )
				{
					// if first vertex, we have already saved
					SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
				}
				int w=m_routing_width;
				int via_w=0;
				int via_hole_w=0;
				GetWidthsForSegment( &w, &via_w, &via_hole_w );
				// see if cursor on pad
				CPoint p = m_last_cursor_point;	
				if( en_branch == BRANCH_TO_LINE )
				{
					for( int step=0; step<2; step++ )
					{
						for( int con=0; con<m_sel_net->nconnects; con++ )
						{
							if( con == m_sel_id.i )
								continue;
							cconnect * c = &m_sel_net->connect[con];
							for( int seg=0; seg<c->nsegs; seg++ )
							{
								CPoint p1,p2;
								p1.x = c->vtx[seg].x;
								p1.y = c->vtx[seg].y;
								p2.x = c->vtx[seg+1].x;
								p2.y = c->vtx[seg+1].y;
								int d = GetPointToLineSegmentDistance(p.x,p.y,p1.x,p1.y,p2.x,p2.y);
								if( d < c->seg[seg].width/2 && ((step == 0 && c->seg[seg].layer == m_sel_layer) || step) )
								{
									float a1 = Angle(	p.x,p.y,m_sel_vtx.x,m_sel_vtx.y  );
									float a2 = Angle(	p2.x,p2.y,p1.x,p1.y  );
									if (InRange((int)abs(a1-a2)%180, 2, 178) )
									{
										float x = m_sel_vtx.x - p1.x;
										float y = m_sel_vtx.y - p1.y;
										Rotate_Vertex( &x, &y, -a1 );
										float newY = y;
										float newX = -y*tan((90-a1+a2)*M_PI/180.0);
										Rotate_Vertex( &newX, &newY, a1 );
										newX += p1.x;
										newY += p1.y;
										int insert_flag =	m_Doc->m_nlist->InsertSegment( m_sel_net, con, seg,
															newX, newY,
															c->seg[seg].layer, c->seg[seg].width, 0, 0, 0 );

										//
										CPoint pi = m_snap_angle_ref;
										CPoint pf = m_last_cursor_point;
										CPoint pp = GetInflectionPoint( pi, pf, m_inflection_mode );
										if( pp != pi )
										{
											m_sel_id.ii = m_Doc->m_nlist->AppendSegment( m_sel_net, m_sel_ic,
												pp.x, pp.y, routing_layer, w );
										}
										m_sel_id.ii = m_Doc->m_nlist->AppendSegment( m_sel_net, m_sel_ic,
															newX, newY, routing_layer, w, via_w, via_hole_w  );
										// set tee_ID for end-vertex and remove selector
										int idd = c->vtx[seg+1].tee_ID;
										if( !idd )
											idd = m_Doc->m_nlist->GetNewTeeID();
										c->vtx[seg+1].tee_ID = idd;
										m_sel_con.vtx[m_sel_con.nsegs].tee_ID = idd;
										if( m_sel_con.nsegs > 1 )
											AlignSegments( m_sel_net, m_sel_ic, m_sel_con.nsegs-1, FALSE );
										m_Doc->m_nlist->ReconcileVia( m_sel_net, m_sel_ic, m_sel_con.nsegs, 1, via_w, via_hole_w );
										m_Doc->m_nlist->DrawConnection( m_sel_net, m_sel_ic );
										m_dlist->StopDragging();
										int ret = AfxMessageBox( "Branch created" );
										m_sel_ic = m_Doc->m_nlist->OptimizeConnections( m_sel_net, m_sel_ic, m_Doc->m_auto_ratline_disable,
																		 m_Doc->m_auto_ratline_min_pins, TRUE  );
										if( m_sel_ic >= 0 )
											m_sel_iv = m_sel_con.nsegs - 1;
										else
											m_sel_iv = 0;
										if( m_sel_iv > 0 )
										{
											SetCursorMode( CUR_VTX_SELECTED );
											m_Doc->m_dlist->CancelHighLight();
											m_Doc->m_nlist->HighlightVertex( m_sel_net, m_sel_ic, m_sel_iv );
										}
										else
											CancelSelection();
										m_Doc->ProjectModified( TRUE );
										goto goodbye;
									}
								}
							}
						}
					}
				}
				if( en_branch == BRANCH_TO_VERTEX )
				{
					//for same layer
					for( int con=0; con<m_sel_net->nconnects; con++ )
					{
						if( con == m_sel_id.i )
							continue;
						cconnect * c = &m_sel_net->connect[con];
						for( int seg=1; seg<c->nsegs; seg++ )
						{
							CPoint p1;
							p1.x = c->vtx[seg].x;
							p1.y = c->vtx[seg].y;
							int d = Distance(p.x,p.y,p1.x,p1.y);
							if( d < c->seg[seg].width/2 || d < c->seg[seg-1].width/2 )
								if( c->vtx[seg].via_w || c->seg[seg].layer == routing_layer || c->seg[seg-1].layer == routing_layer )
								{
									CPoint pi = m_snap_angle_ref;
									CPoint pf = m_last_cursor_point;
									CPoint pp = GetInflectionPoint( pi, pf, m_inflection_mode );
									if( abs(pp.x - m_sel_vtx.x) > _2540 || abs(pp.y - m_sel_vtx.y) > _2540 )
									{
										m_Doc->m_nlist->AppendSegment( m_sel_net, m_sel_ic,
											pp.x, pp.y, routing_layer, w );
									}
									m_sel_id.ii = m_Doc->m_nlist->AppendSegment( m_sel_net, m_sel_ic, p1.x, p1.y, routing_layer, w ) + 1;
									// set tee_ID for end-vertex and remove selector
									int idd = c->vtx[seg].tee_ID;
									if( !idd )
										idd = m_Doc->m_nlist->GetNewTeeID();
									c->vtx[seg].tee_ID = idd;
									m_sel_con.vtx[m_sel_con.nsegs].tee_ID = idd;
									m_Doc->m_nlist->ReconcileVia( m_sel_net, m_sel_ic, m_sel_con.nsegs, 1, via_w, via_hole_w );
									m_Doc->m_nlist->DrawConnection( m_sel_net, m_sel_ic );
									m_dlist->StopDragging();
									int ret = AfxMessageBox( "Branch created" );
									m_sel_ic = m_Doc->m_nlist->OptimizeConnections( m_sel_net, m_sel_ic, m_Doc->m_auto_ratline_disable,
																	 m_Doc->m_auto_ratline_min_pins, TRUE  );
									if( m_sel_ic >= 0 )
										m_sel_iv = m_sel_con.nsegs - 1;
									else
										m_sel_iv = 0;
									if( m_sel_iv > 0 )
									{
										SetCursorMode( CUR_VTX_SELECTED );
										m_Doc->m_dlist->CancelHighLight();
										m_Doc->m_nlist->HighlightVertex( m_sel_net, m_sel_ic, m_sel_iv );
									}
									else
										CancelSelection();
									m_Doc->ProjectModified( TRUE );
									goto goodbye;
								}
						}
					}
					//for diff layer
					for( int con=0; con<m_sel_net->nconnects; con++ )
					{
						if( con == m_sel_id.i )
							continue;
						cconnect * c = &m_sel_net->connect[con];
						for( int seg=1; seg<c->nsegs; seg++ )
						{
							CPoint p1;
							p1.x = c->vtx[seg].x;
							p1.y = c->vtx[seg].y;
							int d = Distance(p.x,p.y,p1.x,p1.y);
							if( d < c->seg[seg].width/2 || d < c->seg[seg-1].width/2 )
								if( c->seg[seg].layer != routing_layer && c->seg[seg-1].layer != routing_layer )
								{
									CPoint pi = m_snap_angle_ref;
									CPoint pf = m_last_cursor_point;
									CPoint pp = GetInflectionPoint( pi, pf, m_inflection_mode );
									if( abs(pp.x - m_sel_vtx.x) > _2540 || abs(pp.y - m_sel_vtx.y) > _2540 )
									{
										m_Doc->m_nlist->AppendSegment( m_sel_net, m_sel_ic,
											pp.x, pp.y, routing_layer, w );
									}
									m_sel_id.ii = m_Doc->m_nlist->AppendSegment( m_sel_net, m_sel_ic, p1.x, p1.y, routing_layer, w ) + 1;
									// set tee_ID for end-vertex and remove selector
									int idd = c->vtx[seg].tee_ID;
									if( !idd )
										idd = m_Doc->m_nlist->GetNewTeeID();
									c->vtx[seg].tee_ID = idd;
									m_sel_con.vtx[m_sel_con.nsegs].tee_ID = idd;
									m_Doc->m_nlist->ReconcileVia( m_sel_net, m_sel_ic, m_sel_con.nsegs, 1, via_w, via_hole_w );
									m_Doc->m_nlist->DrawConnection( m_sel_net, m_sel_ic );
									m_dlist->StopDragging();
									int ret = AfxMessageBox( "Branch created" );
									m_sel_ic = m_Doc->m_nlist->OptimizeConnections( m_sel_net, m_sel_ic, m_Doc->m_auto_ratline_disable,
																	 m_Doc->m_auto_ratline_min_pins, TRUE  );
									if( m_sel_ic >= 0 )
										m_sel_iv = m_sel_con.nsegs - 1;
									else
										m_sel_iv = 0;
									if( m_sel_iv > 0 )
									{
										SetCursorMode( CUR_VTX_SELECTED );
										m_Doc->m_dlist->CancelHighLight();
										m_Doc->m_nlist->HighlightVertex( m_sel_net, m_sel_ic, m_sel_iv );
									}
									else
										CancelSelection();
									m_Doc->ProjectModified( TRUE );
									goto goodbye;
								}
						}
					}
				}
				if( en_branch == DISABLE_BRANCH )
				{
					id sel_id;	// id of selected item
					id pad_id( ID_PART, ID_PAD, 0, 0, 0 );	// test for hit on pad
					void * ptr = m_dlist->TestSelect( p.x, p.y, &sel_id, &m_sel_layer, NULL, NULL, &pad_id );
					if( ptr && sel_id.type == ID_PART && sel_id.st == ID_PAD )
					{
						// see if we can connect to this pin
						cpart * new_sel_part = (cpart*)ptr;
						cnet * new_sel_net = (cnet*)new_sel_part->pin[sel_id.i].net;
						CString pin_name = new_sel_part->shape->GetPinNameByIndex( sel_id.i );
						if( new_sel_net == NULL && m_Doc->m_netlist_completed )
						{
							// Fail(...)
						}
						else if( new_sel_part == m_sel_net->pin[m_sel_con.start_pin].part && sel_id.i == m_sel_con.start_pin_shape && m_sel_is < 1 )
						{
							// Fail(...)
							int test = 0;
						}
						else if( m_Doc->m_plist->TestHitOnPad( new_sel_part, &pin_name, p.x, p.y, routing_layer ) >= 0 )
						{
							// check for starting pad of stub trace
							cpart * origin_part = m_sel_start_pin.part;
							CString * origin_pin_name = &m_sel_start_pin.pin_name;
							///if( origin_part != new_sel_part || origin_pin_name->Compare(pin_name) )
							///{
								// not starting pad
								if( new_sel_net && (new_sel_net != m_sel_net) )
								{
									// pin assigned to different net, can't connect it
									CString mess;
									mess.Format( "You are trying to connect to a pin on a different net" );
									AfxMessageBox( mess );
									return;
								}
								else if( new_sel_net == 0 )
								{
									// unassigned pin, assign it
									SaveUndoInfoForPart( new_sel_part, CPartList::UNDO_PART_MODIFY, NULL, FALSE, m_Doc->m_undo_list );
									m_Doc->m_nlist->AddNetPin( m_sel_net, &new_sel_part->ref_des, &pin_name );
								}
								else
								{
									// pin already assigned to this net
								}
								CPoint pi = m_snap_angle_ref;
								CPoint pf = m_last_cursor_point;
								CPoint pp = GetInflectionPoint( pi, pf, m_inflection_mode );
								if( pp != pi )
								{
									m_sel_id.ii = m_Doc->m_nlist->AppendSegment( m_sel_net, m_sel_ic,
										pp.x, pp.y, routing_layer, w, via_w, via_hole_w );
								}
								m_sel_id.ii = m_Doc->m_nlist->AppendSegment( m_sel_net, m_sel_ic,
									m_last_cursor_point.x, m_last_cursor_point.y, routing_layer, w, via_w, via_hole_w );
								CPoint pin_point(m_sel_con.vtx[0].x,m_sel_con.vtx[0].y);
								if( m_last_cursor_point != pin_point )
								{
									m_sel_id.ii = m_Doc->m_nlist->AppendSegment( m_sel_net, m_sel_ic,
										pin_point.x, pin_point.y, routing_layer, w, via_w, via_hole_w );
								}
								m_Doc->m_nlist->ChangeConnectionPin( m_sel_net, m_sel_ic, TRUE, m_sel_net, new_sel_part, &pin_name, p.x, p.y );
								m_dlist->StopDragging();
								m_Doc->m_nlist->OptimizeConnections( m_sel_net, m_sel_ic, m_Doc->m_auto_ratline_disable,
																	 m_Doc->m_auto_ratline_min_pins, TRUE  );
								m_dlist->CancelHighLight();
								CancelSelection();
								m_Doc->ProjectModified( TRUE );
								goto goodbye;
							///}
						}
					}
				}
				m_sel_id = save_sel_id;
				m_sel_net = save_net;
				// come here if not connecting to anything
				pDC = GetDC();
				SetDCToWorldCoords( pDC );
				pDC->SelectClipRgn( &m_pcb_rgn );
				m_sel_vtx.force_via_flag = 0;
				CPoint pi = m_snap_angle_ref;
				CPoint pf = m_last_cursor_point;
				CPoint pp = GetInflectionPoint( pi, pf, m_inflection_mode );
				if( pp != pi )
				{
					m_sel_id.ii = m_Doc->m_nlist->AppendSegment( m_sel_net, m_sel_ic,
						pp.x, pp.y, routing_layer, w );
				}
				m_sel_id.ii = m_Doc->m_nlist->AppendSegment( m_sel_net, m_sel_ic,
						m_last_cursor_point.x, m_last_cursor_point.y, routing_layer, w, via_w, via_hole_w );
				m_dlist->StopDragging();
				m_sel_id.ii++;
				m_Doc->m_nlist->StartDraggingStub( pDC, m_sel_net, m_sel_ic, m_sel_is,
					m_last_cursor_point.x, m_last_cursor_point.y, routing_layer, w, routing_layer,
					via_w, via_hole_w, 2, m_inflection_mode );
				m_snap_angle_ref = m_last_cursor_point;
				m_Doc->ProjectModified( TRUE );
				m_dlist->CancelHighLight();
				if ( en_branch == BRANCH_TO_VERTEX ) 
				{
					m_Doc->m_nlist->HighlightNetVertices( m_sel_net, FALSE, FALSE );
				}
				else if ( en_branch == BRANCH_TO_LINE ) 
				{
					m_Doc->m_nlist->HighlightNetConnections( m_sel_net ); 
				}
				else
				{
					m_Doc->m_nlist->HighlightNet( m_sel_net, TRANSPARENT_HILITE );
					m_Doc->m_plist->HighlightAllPadsOnNet( m_sel_net, 1, m_active_layer );
				}
			}
			else if( m_cursor_mode == CUR_DRAG_TEXT )
			{
				CPoint p;
				p = m_last_cursor_point;
				m_dlist->StopDragging();
				if( !m_dragging_new_item )
					SaveUndoInfoForText( m_sel_text, CTextList::UNDO_TEXT_MODIFY, TRUE, m_Doc->m_undo_list );
				int old_angle = m_sel_text->m_angle;
				int angle = old_angle + m_dlist->GetDragAngle();
				if( angle>270 )
					angle = angle - 360;
				int old_mirror = m_sel_text->m_mirror;
				int mirror = (old_mirror + m_dlist->GetDragSide())%2;
				BOOL negative = m_sel_text->m_bNegative;
				int layer = m_sel_text->m_layer;
				m_Doc->m_tlist->MoveText( m_sel_text, m_last_cursor_point.x, m_last_cursor_point.y,
					angle, mirror, negative, layer );
				if( m_dragging_new_item )
				{
					SaveUndoInfoForText( m_sel_text, CTextList::UNDO_TEXT_ADD, TRUE, m_Doc->m_undo_list );
					m_dragging_new_item = FALSE;
				}
				SetCursorMode( CUR_TEXT_SELECTED );
				m_Doc->m_tlist->HighlightText( m_sel_text );
				m_Doc->ProjectModified( TRUE );
			}
			else if( m_cursor_mode == CUR_DRAG_MEASURE_1 )
			{
				m_measure_dist = 0;
				m_from_pt = m_last_cursor_point;
				m_dlist->MakeDragRatlineArray( 1, 1 );
				m_dlist->AddDragRatline( m_from_pt, zero ); 
				SetCursorMode( CUR_DRAG_MEASURE_2 );
			}
			else if( m_cursor_mode == CUR_DRAG_MEASURE_2 )
			{
				int dst = Distance( m_from_pt.x, m_from_pt.y, m_last_cursor_point.x, m_last_cursor_point.y );
				float cur_ang = Angle( m_from_pt.x, m_from_pt.y, m_last_cursor_point.x, m_last_cursor_point.y );
				m_measure_dist += dst;
				// line
				CPoint P[2];
				P[0] = m_from_pt;
				P[1] = m_last_cursor_point;
				dl_element * el = m_Doc->m_dlist->Add( NULL, NULL, 0, DL_LINE, TRUE, NULL, 1, P, 2 );
				m_Doc->m_dlist->HighLight( el );
				setbit( el->map_orig_layer, m_active_layer );
				el->el_static = TRUE;
				el->transparent = TRANSPARENT_HILITE;
				// text
				int font_size = min( NM_PER_MIL*10, dst/10 );
				int stroke_width = font_size/6;
				CString str;
				::MakeCStringFromDimension( &str, dst, m_Doc->m_units, 0, 0, 0 );
				CText * ht = m_Doc->m_tlist->AddText( m_last_cursor_point.x, m_last_cursor_point.y, 0/*angle*/, 0/*mirror*/, 0/*bNegative*/,
															  m_active_layer, font_size, stroke_width, &str );
				m_Doc->m_tlist->HighlightText(ht,TRANSPARENT_HILITE);
				m_Doc->m_tlist->RemoveText( ht );
				::MakeCStringFromDimension( &str, m_measure_dist, m_Doc->m_units, 0, 0, 0 );
				ht = m_Doc->m_tlist->AddText( m_last_cursor_point.x, m_last_cursor_point.y-(font_size*2), 0/*angle*/, 0/*mirror*/, 0/*bNegative*/,
															  m_active_layer, font_size, stroke_width, &str );
				m_Doc->m_tlist->HighlightText(ht,TRANSPARENT_HILITE);
				m_Doc->m_tlist->RemoveText( ht );
				if( prev_m_ang >= 0 )
				{
					float diff_ang = cur_ang - prev_m_ang;
					if( diff_ang < 0 )
						diff_ang += 360;
					if( diff_ang > 180 )
						diff_ang = 360 - diff_ang;
					diff_ang *= NM_PER_MM;
					::MakeCStringFromDimension( &str, diff_ang, MM, 0, 0, 0, 2, 0 );
					ht = m_Doc->m_tlist->AddText( m_last_cursor_point.x, m_last_cursor_point.y-(font_size*4), 0/*angle*/, 0/*mirror*/, 0/*bNegative*/,
															  m_active_layer, font_size, stroke_width, &str );
					m_Doc->m_tlist->HighlightText(ht,TRANSPARENT_HILITE);
					m_Doc->m_tlist->RemoveText( ht );
				}
				//
				prev_m_ang = cur_ang - 180;
				if( prev_m_ang < 0 )
					prev_m_ang += 360;
				m_from_pt = m_last_cursor_point;
				m_dlist->MakeDragRatlineArray( 1, 1 );
				m_dlist->AddDragRatline( m_from_pt, zero ); 
			}
		}
		goto goodbye;

cancel_selection_and_goodbye:
		m_dlist->StopDragging();
		CancelSelection();
	}

goodbye:
	if(!( m_cursor_mode == CUR_PART_SELECTED || m_cursor_mode == CUR_GROUP_SELECTED || m_cursor_mode == CUR_PAD_SELECTED))
		OnInfoBoxSendMess( "part_list ");
	ShowSelectStatus();
	if( pDC )
		ReleaseDC( pDC );
	Invalidate( FALSE );//OnLButtonUp
	CView::OnLButtonUp(nFlags, point);
}
//===============================================================================================
// left double-click
//
void CFreePcbView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
#if 0
	if( m_cursor_mode == CUR_PART_SELECTED )
	{
		SetCursorMode( CUR_DRAG_PART );
		CDC *pDC = GetDC();
		pDC->SelectClipRgn( &m_pcb_rgn );
		SetDCToWorldCoords( pDC );
		CPoint p = m_last_mouse_point;
		m_dlist->StartDraggingSelection( pDC, p.x, p.y );
	}
	if( m_cursor_mode == CUR_REF_SELECTED )
	{
		SetCursorMode( CUR_DRAG_REF );
		CDC *pDC = GetDC();
		pDC->SelectClipRgn( &m_pcb_rgn );
		SetDCToWorldCoords( pDC );
		CPoint p = m_last_mouse_point;
		m_dlist->StartDraggingSelection( pDC, p.x, p.y );
	}
#endif
	CView::OnLButtonDblClk(nFlags, point);
}
//===============================================================================================
// right mouse button
//
void CFreePcbView::OnRButtonDown(UINT nFlags, CPoint point)
{
	m_draw_layer = LAY_HILITE;//OnRButtonDown
	m_disable_context_menu = 1;
	if( m_cursor_mode == CUR_DRAG_PART )
	{
		m_draw_layer = DISABLE_CHANGE_DRAW_LAYER;//CUR_DRAG_PART
		m_Doc->m_plist->CancelDraggingPart( m_sel_part );
		if( m_dragging_new_item )
		{
			m_Doc->OnEditUndo();	// remove the part
		}
		else
		{
			if( m_sel_id.st == ID_PAD )
			{
				SetCursorMode( CUR_PAD_SELECTED );
				NewSelect( m_sel_part, &m_sel_id, TRUE,0);
			}
			else
			{
				SetCursorMode( CUR_PART_SELECTED );
				SelectPart( m_sel_part );
			}
		}
		m_dragging_new_item = FALSE;
		m_Doc->ProjectModified( TRUE );
	}
	else if( m_cursor_mode == CUR_DRAG_REF )
	{
		m_draw_layer = m_sel_part->side+LAY_TOP_COPPER;
		m_Doc->m_plist->CancelDraggingRefText( m_sel_part );
		SetCursorMode( CUR_REF_SELECTED );
		m_Doc->m_plist->SelectRefText( m_sel_part );
		m_Doc->m_plist->SelectPads( m_sel_part, 1, m_active_layer, TRANSPARENT_BACKGND );
	}
	else if( m_cursor_mode == CUR_DRAG_VALUE )
	{
		m_draw_layer = m_sel_part->side+LAY_TOP_COPPER;
		m_Doc->m_plist->CancelDraggingValue( m_sel_part );
		SetCursorMode( CUR_VALUE_SELECTED );
		m_Doc->m_plist->SelectValueText( m_sel_part );
		m_Doc->m_plist->SelectPads( m_sel_part, 1, m_active_layer, TRANSPARENT_BACKGND );
	}
	else if( m_cursor_mode == CUR_DRAG_RAT )
	{
		m_Doc->m_nlist->CancelDraggingSegment( m_sel_net, m_sel_ic, m_sel_is );
		m_sel_ic = m_Doc->m_nlist->OptimizeConnections( m_sel_net, m_sel_ic, m_Doc->m_auto_ratline_disable,
													 m_Doc->m_auto_ratline_min_pins, TRUE  );
		m_Doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, m_sel_is, m_seg_clearance );
		SetCursorMode( CUR_RAT_SELECTED );
	}
	else if( m_cursor_mode == CUR_DRAG_RAT_PIN )
	{
		m_dlist->StopDragging();
		m_sel_seg.dl_el->visible = TRUE;
		m_Doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, m_sel_is );
		SetCursorMode( CUR_RAT_SELECTED );
	}
	else if( m_cursor_mode == CUR_DRAG_VTX )
	{
		VertexMoved();
		m_Doc->m_nlist->CancelDraggingVertex( m_sel_net, m_sel_ic, m_sel_is );
		SetCursorMode( CUR_VTX_SELECTED );
		m_Doc->m_nlist->HighlightVertex( m_sel_net, m_sel_ic, m_sel_is );
	}
	else if( m_cursor_mode == CUR_MOVE_SEGMENT )
	{
		VertexMoved();
		m_Doc->m_nlist->CancelMovingSegment( m_sel_net, m_sel_ic, m_sel_is );
		SetCursorMode( CUR_SEG_SELECTED );
		m_Doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, m_sel_is );
	}
	else if( m_cursor_mode == CUR_DRAG_VTX_INSERT )
	{
		m_draw_layer = DISABLE_CHANGE_DRAW_LAYER; //CUR_DRAG_VTX_INSERT
		m_Doc->m_nlist->CancelDraggingSegmentNewVertex( m_sel_net, m_sel_ic, m_sel_is );
		SetCursorMode( CUR_SEG_SELECTED );
		m_Doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, m_sel_is );
	}
	else if( m_cursor_mode == CUR_DRAG_END_VTX )
	{
		VertexMoved();
		m_Doc->m_nlist->CancelDraggingEndVertex( m_sel_net, m_sel_ic, m_sel_is );
		SetCursorMode( CUR_END_VTX_SELECTED );
		m_Doc->m_dlist->CancelHighLight();
		m_Doc->m_nlist->HighlightVertex( m_sel_net, m_sel_ic, m_sel_is );
	}
	else if( m_cursor_mode == CUR_DRAG_CONNECT )
	{
		m_Doc->m_dlist->StopDragging();
		if( m_sel_id.type == ID_PART )
		{
			m_Doc->m_dlist->CancelHighLight();
			m_Doc->m_plist->SelectPad( m_sel_part, m_sel_id.i, 0, 0 );
			SetCursorMode( CUR_PAD_SELECTED );
		}
		else if( m_sel_id.type == ID_NET && m_sel_id.st == ID_CONNECT && m_sel_id.sst == ID_VERTEX )
		{
			// select vertex
			m_dlist->CancelHighLight();
			m_Doc->m_nlist->HighlightVertex( m_sel_net, m_sel_id.i, m_sel_id.ii );
			cconnect * c = &m_sel_net->connect[m_sel_id.i];
			if( c->end_pin == cconnect::NO_END && m_sel_id.ii == c->nsegs )
				SetCursorMode( CUR_END_VTX_SELECTED );
			else
				SetCursorMode( CUR_VTX_SELECTED );
		}
		else 
		{
			CancelSelection();
		}
	}
	else if( m_cursor_mode == CUR_DRAG_TEXT )
	{
		m_draw_layer = m_sel_text->m_layer;
		m_Doc->m_tlist->CancelDraggingText( m_sel_text );
		if( m_dragging_new_item )
		{
			m_Doc->m_tlist->RemoveText( m_sel_text );
			CancelSelection();
			m_dragging_new_item = 0;
		}
		else
		{
			SetCursorMode( CUR_TEXT_SELECTED );
		}
	}
	else if( m_cursor_mode == CUR_ADD_OP
		  || m_cursor_mode == CUR_DRAG_OP_1
		  || (m_cursor_mode == CUR_DRAG_OP && m_Doc->m_outline_poly[m_sel_id.i].GetNumCorners()<2 && m_polyline_closed ) )
	{
		m_draw_layer = LAY_HILITE;//CUR_ADD_OP
		// dragging first, second or third corner of board outline
		// just delete it (if necessary) and cancel
		m_dlist->StopDragging();
		if( m_cursor_mode != CUR_ADD_OP )
		{
			if( m_sel_id.ii < 2 )
				OnOPDeleteOutline();
			else
				m_Doc->m_outline_poly[m_sel_id.i].RemoveContour( m_Doc->m_outline_poly[m_sel_id.i].GetNumContours()-1 );
		}
		//	
		CancelSelection();
	}
	else if( m_cursor_mode == CUR_DRAG_OP )
	{
		m_draw_layer = m_Doc->m_outline_poly[m_sel_id.i].GetLayer();//CUR_DRAG_OP
		// dragging fourth or higher corner of board outline, close it
		m_dlist->StopDragging();
		SaveUndoInfoForOutlinePoly( UNDO_OP, TRUE, m_Doc->m_undo_list );
		if( m_polyline_layer == LAY_BOARD_OUTLINE ||
			m_polyline_layer == LAY_SM_TOP ||
			m_polyline_layer == LAY_SM_BOTTOM ||
			m_Doc->m_outline_poly[m_sel_id.i].GetNumContour(m_sel_id.ii) ||
			m_polyline_closed )
			m_Doc->m_outline_poly[m_sel_id.i].Close( m_polyline_style );
		m_Doc->ProjectModified( TRUE );
		CancelSelection();
	}
	else if( m_cursor_mode == CUR_DRAG_OP_INSERT )
	{
		m_draw_layer = m_Doc->m_outline_poly[m_sel_id.i].GetLayer();//CUR_DRAG_OP_INSERT
		m_dlist->StopDragging();
		m_Doc->m_outline_poly[m_sel_id.i].MakeVisible();
		m_Doc->m_outline_poly[m_sel_id.i].HighlightSide( m_sel_id.ii );
		SetCursorMode( CUR_OP_SIDE_SELECTED );
	}
	else if( m_cursor_mode == CUR_DRAG_OP_MOVE )
	{
		m_draw_layer = m_Doc->m_outline_poly[m_sel_id.i].GetLayer();//CUR_DRAG_OP_MOVE
		// get indexes for preceding and following corners
		m_dlist->StopDragging();
		m_Doc->m_outline_poly[m_sel_id.i].MakeVisible();
		SetCursorMode( CUR_OP_CORNER_SELECTED );
		m_Doc->m_outline_poly[m_sel_id.i].HighlightCorner( m_sel_id.ii );
	}
	else if( m_cursor_mode == CUR_ADD_AREA )
	{
		m_dlist->StopDragging();
		SetCursorMode( CUR_NONE_SELECTED );
	}
	else if( m_cursor_mode == CUR_DRAG_AREA_1
		  || (m_cursor_mode == CUR_DRAG_AREA && m_sel_id.ii<2) )
	{
		m_draw_layer = m_sel_net->area[m_sel_ia].poly->GetLayer();//CUR_DRAG_AREA_1
		m_dlist->StopDragging();
		m_Doc->m_nlist->RemoveArea( m_sel_net, m_sel_ia );
		CancelSelection();
	}
	else if( m_cursor_mode == CUR_DRAG_AREA)
	{
		m_draw_layer = m_sel_net->area[m_sel_ia].poly->GetLayer();//CUR_DRAG_AREA
		m_dlist->StopDragging();
		int icont = m_sel_net->area[m_sel_ia].poly->GetNumContours()-1;
		int st = m_sel_net->area[m_sel_ia].poly->GetContourStart(icont);
		int end = m_sel_net->area[m_sel_ia].poly->GetContourEnd(icont);
		if( end-st < 2 )
			m_Doc->m_nlist->RemoveArea( m_sel_net, m_sel_ia );
		else
		{
			SaveUndoInfoForAllAreasInNet( m_sel_net, TRUE, m_Doc->m_undo_list );
			m_Doc->m_nlist->CompleteArea( m_sel_net, m_sel_ia, m_polyline_style );
			int ret = m_Doc->m_nlist->AreaPolygonModified( m_sel_net, m_sel_ia, TRUE, TRUE );
			if( ret == 1 )
				m_draw_layer = DISABLE_CHANGE_DRAW_LAYER;
			m_Doc->m_nlist->OptimizeConnections(    m_sel_net, -1, m_Doc->m_auto_ratline_disable,
													m_Doc->m_auto_ratline_min_pins, TRUE  );
		}
		
		CancelSelection();
	}
	else if( m_cursor_mode == CUR_ADD_AREA_CUTOUT )
	{
		m_draw_layer = LAY_HILITE;//CUR_ADD_AREA_CUTOUT
		fRepour = 0;
		m_dlist->StopDragging();
		SetCursorMode( CUR_NONE_SELECTED );
	}
	else if( m_cursor_mode == CUR_DRAG_AREA_CUTOUT_1
		  || (m_cursor_mode == CUR_DRAG_AREA_CUTOUT && m_sel_id.ii<2) )
	{
		m_draw_layer = m_sel_net->area[m_sel_ia].poly->GetLayer();//CUR_ADD_AREA_CUTOUT_1
		m_dlist->StopDragging();
		CPolyLine * poly = m_sel_net->area[m_sel_id.i].poly;
		int ncont = poly->GetNumContours();
		poly->RemoveContour(ncont-1);
		CancelSelection();
	}
	else if( m_cursor_mode == CUR_DRAG_AREA_CUTOUT )
	{
		m_draw_layer = DISABLE_CHANGE_DRAW_LAYER;//m_sel_net->area[m_sel_ia].poly->GetLayer();//CUR_DRAG_AREA_CUTOUT
		m_dlist->StopDragging();
		SetCursorMode( CUR_NONE_SELECTED );
		SaveUndoInfoForAllAreasInNet( m_sel_net, TRUE, m_Doc->m_undo_list );
		m_Doc->m_nlist->CompleteArea( m_sel_net, m_sel_ia, m_polyline_style );
		CPolyLine * poly = m_sel_net->area[m_sel_id.i].poly;
		int m_old_merge = poly->GetMerge();
		// ClipAreaPolygon
		int ret = m_Doc->m_nlist->ClipAreaPolygon( m_sel_net, m_sel_ia, -1, FALSE, FALSE  );
		// update pointer
		poly = m_sel_net->area[m_sel_ia].poly;
		// set merge
		poly->SetMerge( m_old_merge );
		if( fRepour )
		{
			m_Doc->m_nlist->CompleteArea( m_sel_net, fRepour, m_polyline_style );
			fRepour = 0;
		}
		if( ret == -1 )
		{
			// error
			AfxMessageBox( "Error: Unable to clip polygon due to intersecting arc" );
			////m_Doc->OnEditUndo();
		}
		CancelSelection();
	}
	else if( m_cursor_mode == CUR_DRAG_AREA_INSERT )
	{
		m_draw_layer = m_sel_net->area[m_sel_ia].poly->GetLayer();//CUR_DRAG_AREA_INSERT
		m_Doc->m_nlist->CancelDraggingInsertedAreaCorner( m_sel_net, m_sel_ia, m_sel_is );
		m_Doc->m_nlist->SelectAreaSide( m_sel_net, m_sel_ia, m_sel_is );
		SetCursorMode( CUR_AREA_SIDE_SELECTED );
	}
	else if( m_cursor_mode == CUR_DRAG_AREA_MOVE )
	{
		m_draw_layer = m_sel_net->area[m_sel_ia].poly->GetLayer();//CUR_DRAG_AREA_MOVE
		m_Doc->m_nlist->CancelDraggingAreaCorner( m_sel_net, m_sel_ia, m_sel_is );
		m_Doc->m_nlist->SelectAreaCorner( m_sel_net, m_sel_ia, m_sel_is );
		SetCursorMode( CUR_AREA_CORNER_SELECTED );
	}
	else if( m_cursor_mode == CUR_DRAG_STUB )
	{
		m_draw_layer = mod_active_layer;//CUR_DRAG_STUB
		m_dlist->StopDragging();
		if( m_sel_id.ii == 0 )
			m_Doc->m_nlist->RemoveNetConnect( m_sel_net, m_sel_ic );
		else
		{
			m_Doc->m_nlist->CancelDraggingStub( m_sel_net, m_sel_ic, m_sel_is );
			m_dlist->CancelHighLight();
			int x = m_sel_net->connect[m_sel_ic].vtx[m_sel_iv].x;
			int y = m_sel_net->connect[m_sel_ic].vtx[m_sel_iv].y;
			int pin = m_sel_net->connect[m_sel_ic].start_pin;
			CString pin_n = m_sel_net->pin[pin].pin_name;
			cpart * part = m_sel_net->pin[pin].part;
			int sh_i=m_Doc->m_nlist->GetPinIndexByNameForPart( part, pin_n, x, y );
			int xp = part->pin[sh_i].x;
			int yp = part->pin[sh_i].y;
			BOOL test1 = m_Doc->m_nlist->TestPointInArea( m_sel_net, x, y, -m_active_layer, NULL );
			BOOL test2 = m_Doc->m_nlist->TestPointInArea( m_sel_net, x, y, m_active_layer, NULL );
			BOOL test3 = m_Doc->m_nlist->TestPointInArea( m_sel_net, xp, yp, m_active_layer, NULL );
			if( (test1 && !test2) || (test1 && test2 && test3) )
			{   // add a via and optimize, this also optimizes and selects via
				SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
				int w = m_routing_width;
				int vw=0, vh=0;
				GetWidthsForSegment(&w,&vw,&vh);
				m_Doc->m_nlist->ForceVia( m_sel_net, m_sel_ic, m_sel_is, FALSE, vw, vh );
			}
			m_Doc->m_nlist->DrawConnection( m_sel_net, m_sel_ic );
		}
		if( m_sel_id.ii == 0 )
			CancelSelection();
		else
		{
			m_sel_ic = m_Doc->m_nlist->OptimizeConnections( m_sel_net, m_sel_ic, m_Doc->m_auto_ratline_disable,
													 m_Doc->m_auto_ratline_min_pins, TRUE  );
			m_sel_id.sst = ID_VERTEX;
			m_Doc->m_nlist->HighlightVertex( m_sel_net, m_sel_ic, m_sel_iv ); 
			SetCursorMode(CUR_END_VTX_SELECTED);
		}
	}
	else if( m_cursor_mode == CUR_DRAG_GROUP )
	{
		CancelDraggingGroup();
		m_dlist->SetLayerVisible( LAY_RAT_LINE, m_Doc->m_vis[LAY_RAT_LINE] );
	}
	else if( m_cursor_mode == CUR_DRAG_GROUP_ADD )
	{
		CancelDraggingGroup();
		m_dlist->SetLayerVisible( LAY_RAT_LINE, m_Doc->m_vis[LAY_RAT_LINE] );
		if (!fCopyTraces)
			m_Doc->OnEditUndo();
	}
	else if( m_cursor_mode == CUR_DRAG_MEASURE_1 || m_cursor_mode == CUR_DRAG_MEASURE_2 )
	{
		m_dlist->StopDragging();
		SetCursorMode( CUR_NONE_SELECTED );
	}
	else
	{
		m_disable_context_menu = 0;
	}
	ShowSelectStatus();
	CView::OnRButtonDown(nFlags, point);
	Invalidate( FALSE );//OnRButtonDown
}
//===============================================================================================
// System Key on keyboard pressed down
//
void CFreePcbView::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if( nChar == 121 )
		OnKeyDown( nChar, nRepCnt, nFlags);
	else
		CView::OnSysKeyDown(nChar, nRepCnt, nFlags);
}
//===============================================================================================
// System Key on keyboard pressed up
//
void CFreePcbView::OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if( nChar != 121 )
		CView::OnSysKeyUp(nChar, nRepCnt, nFlags);
}
//===============================================================================================
// Key pressed up
//
void CFreePcbView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if( nChar == VK_SHIFT )
	{
		gShiftKeyDown = FALSE;
	}
	if( m_cursor_mode == CUR_AREA_SIDE_SELECTED && nChar == VK_F6 )
	{
		m_dlist->CancelHighLight();
		int w = m_sel_net->area[m_sel_ia].poly->GetW();
		m_sel_net->area[m_sel_ia].poly->HighlightSide( m_sel_is, w );
		m_draw_layer = -1;
		Invalidate( FALSE );
	}
	if( nChar == 'D' )
	{
		// 'd'
		//if( m_Doc->m_dlg_log )
		//	m_Doc->m_dlg_log->ShowWindow( SW_HIDE );
		m_Doc->m_drelist->MakeHollowCircles();
		m_draw_layer = -1;
		Invalidate( FALSE );//MakeHollowCircles
	}
	else if( nChar == VK_SHIFT || nChar == VK_CONTROL )
	{
		if( m_cursor_mode == CUR_DRAG_RAT || m_cursor_mode == CUR_DRAG_STUB )
		{
			// routing a trace segment, set mode
			if( nChar == VK_CONTROL )
				m_snap_mode = SM_GRID_POINTS;
			if( nChar == VK_SHIFT && m_Doc->m_snap_angle == 45 )
				m_inflection_mode = IM_90_45;
			m_dlist->SetInflectionMode( m_inflection_mode );
		}
	}
	CView::OnKeyUp(nChar, nRepCnt, nFlags);
}
//===============================================================================================
// Key pressed down
//
void CFreePcbView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if( nChar == VK_SHIFT )
		gShiftKeyDown = TRUE;
	if( nChar == 'D' )
	{
		// 'd'
		m_Doc->m_drelist->MakeSolidCircles();
		Invalidate( FALSE ); 
	}
	else if( nChar == VK_SHIFT || nChar == VK_CONTROL )
	{
		if( m_cursor_mode == CUR_DRAG_RAT || m_cursor_mode == CUR_DRAG_STUB )
		{
			// routing a trace segment, set mode
			if( nChar == VK_CONTROL )	// ctrl
				m_snap_mode = SM_GRID_LINES;
			if( nChar == VK_SHIFT && m_Doc->m_snap_angle == 45 )	// shift
				m_inflection_mode = IM_45_90;
			m_dlist->SetInflectionMode( m_inflection_mode );
		}
	}
	else
	{
		HandleKeyPress( nChar, nRepCnt, nFlags );
	}

	// don't pass through SysKey F10
	if( nChar != 121 )
		CView::OnKeyDown(nChar, nRepCnt, nFlags);
}
//===============================================================================================
// Key on keyboard pressed down
//
void CFreePcbView::HandleKeyPress(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	m_draw_layer = -1;// HandleKeyPress 
	//
	// get cursor position and convert to PCB coords
	CPoint mpt;
	GetCursorPos( &mpt );		// cursor pos in screen coords
	CPoint p = m_dlist->ScreenToPCB( mpt );	// convert to PCB coords
	if( m_bDraggingRect )
		goto goodbye;
	if( nChar == 'C' && m_cursor_mode == CUR_SEG_SELECTED )
	{
		// toggle segment through straight and curved shapes
		cconnect * c = &m_sel_net->connect[m_sel_ic];
		cseg * s = &c->seg[m_sel_is];
		cvertex * pre_v = &m_sel_net->connect[m_sel_ic].vtx[m_sel_iv];
		cvertex * post_v = &m_sel_net->connect[m_sel_ic].vtx[m_sel_iv+1];
		int dx = post_v->x - pre_v->x;
		int dy = post_v->y - pre_v->y;
		if( dx == 0 || dy == 0 || s->layer == LAY_RAT_LINE )
		{
			// ratline or vertical or horizontal segment, must be straight
		}
		else
		{
			// toggle through straight or curved options
			SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
			m_Doc->m_nlist->UndrawConnection( m_sel_net, m_sel_ic );
			m_Doc->m_nlist->DrawConnection( m_sel_net, m_sel_ic );
			ShowSelectStatus();
			m_Doc->ProjectModified( TRUE );
		}
		goto goodbye;
	}
	if( nChar == 'F' && ( m_cursor_mode == CUR_VTX_SELECTED || m_cursor_mode == CUR_END_VTX_SELECTED ) )
	{
		static int ex_i = -1;
		static int pre_i = 0;
		static int pre_ii = 0;
		if( pre_i != m_sel_id.i || pre_ii != m_sel_id.ii )
			ex_i = -1;
		pre_i = m_sel_id.i;
		pre_ii = m_sel_id.ii;
		SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
		int w=m_sel_net->def_w;
		int vw=m_sel_vtx.via_w;
		int vh=m_sel_vtx.via_hole_w;
		//BOOL b = vw;
		if (m_sel_is)
			w=m_sel_last_seg.width;
		ex_i = GetWidthsForSegment(&w,&vw,&vh,ex_i);
		m_Doc->m_nlist->ForceVia( m_sel_net, m_sel_ic, m_sel_iv, FALSE, vw, vh );
		if( m_Doc->m_vis[LAY_RAT_LINE] )
			OnRatlineOptimize();
		ShowSelectStatus();
		SetFKText( m_cursor_mode );
		m_Doc->ProjectModified( TRUE );
		goto goodbye;
	}
	if( nChar == 'U' && ( m_cursor_mode == CUR_VTX_SELECTED || m_cursor_mode == CUR_END_VTX_SELECTED ) )
	{
		SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
		m_Doc->m_nlist->UnforceVia( m_sel_net, m_sel_ic, m_sel_iv );
		if( m_cursor_mode == CUR_END_VTX_SELECTED
			&& m_sel_con.seg[m_sel_iv-1].layer == LAY_RAT_LINE
			&& m_sel_vtx.tee_ID == 0 )
		{
			m_Doc->m_nlist->RemoveSegment( m_sel_net, m_sel_ic, m_sel_iv-1 );
			CancelSelection();
		}
		else
		{
			m_Doc->m_nlist->MergeUnroutedSegments( m_sel_net, m_sel_ic );
			if( m_Doc->m_vis[LAY_RAT_LINE] )
				OnRatlineOptimize();
			ShowSelectStatus();
			SetFKText( m_cursor_mode );
		}
		m_Doc->ProjectModified( TRUE );
		goto goodbye;
	}
	if( nChar == 'T' && (	m_cursor_mode == CUR_VTX_SELECTED || 
							m_cursor_mode == CUR_SEG_SELECTED ||
							m_cursor_mode == CUR_GROUP_SELECTED) )
	{
		// "t" pressed, select trace
		if ( m_sel_count == 1 )
		{
			m_Doc->m_nlist->HighlightConnection( m_sel_net, m_sel_ic );
			id ID(ID_NET, ID_CONNECT, m_sel_ic, ID_SEG, 0);
			for(int is=0; is<m_sel_net->connect[m_sel_ic].nsegs; is++)
			{
				ID.ii = is;
				NewSelect(m_sel_net, &ID, 0,0 );
			}
			SetCursorMode( CUR_CONNECT_SELECTED );
		}
		else
		{
			if ( m_sel_flags == CONNECT_ONLY )
			{
				for(cnet* n=m_Doc->m_nlist->GetFirstNet(); n; n=m_Doc->m_nlist->GetNextNet(/*LABEL*/))
					for (int ci=0; ci<n->nconnects; ci++)
						if( n->connect[ci].m_selected )
							for(int cs=0; cs<n->connect[ci].nsegs; cs++)
								m_Doc->m_nlist->HighlightConnection( n, ci );
				SetCursorMode( CUR_GROUP_SELECTED );
			}
			else
				CancelSelection();
		}
		goto goodbye;
	}
	if( nChar == 'N' )
	{
		// "n" pressed, select net
		if(    m_cursor_mode == CUR_VTX_SELECTED
			|| m_cursor_mode == CUR_END_VTX_SELECTED
			|| m_cursor_mode == CUR_RAT_SELECTED
			|| m_cursor_mode == CUR_SEG_SELECTED
			|| m_cursor_mode == CUR_CONNECT_SELECTED 
			|| m_cursor_mode == CUR_AREA_CORNER_SELECTED 
			|| m_cursor_mode == CUR_AREA_SIDE_SELECTED 
			|| m_cursor_mode == CUR_PAD_SELECTED
			|| m_cursor_mode == CUR_TEXT_SELECTED )
		{
			if( m_cursor_mode == CUR_PAD_SELECTED )
			{
				// pad selected and if "n" held down, select net
				cnet * net = m_Doc->m_plist->GetPinNet( m_sel_part, m_sel_id.i );
				if( net )
				{
					m_sel_net = net;
					m_sel_id = net->id;
				}
			}
			if( m_cursor_mode == CUR_TEXT_SELECTED )
			{
				// pad selected and if "n" held down, select net
				cnet * net = m_Doc->m_nlist->GetNetPtrByName( &m_sel_text->m_str );
				if( net )
				{
					m_sel_net = net;
					m_sel_id = net->id;
				}
			}
			if( m_sel_net )
			{
				m_sel_id.st = ID_ENTIRE_NET;
				m_Doc->m_dlist->CancelHighLight();
				m_Doc->m_nlist->HighlightNet( m_sel_net );
				m_Doc->m_nlist->HighlightNetConnections( m_sel_net );
				m_Doc->m_nlist->HighlightNetVertices( m_sel_net, TRUE );
				m_Doc->m_plist->HighlightAllPadsOnNet( m_sel_net, 0, 0 );
				int map_orig_layer = 0;
				RECT rct = m_Doc->m_dlist->GetHighlightedBounds( &map_orig_layer );
				if( rct.right > rct.left )
				{
					id ID(0,0,0,0,0);
					dl_element * el_bounds = m_Doc->m_dlist->Add( ID, NULL, 0, DL_HOLLOW_RECT, TRUE, &rct, 0, NULL, 0 );
					m_Doc->m_dlist->HighLight( el_bounds );
					el_bounds->map_orig_layer = map_orig_layer;
				}
				SetCursorMode( CUR_NET_SELECTED );
			}
		}
		goto goodbye;
	}
	if( nChar == VK_ESCAPE )
	{
		// ESC key, if something selected, cancel it
		// otherwise, fake a right-click
		int cs = CurSelected();
		if( m_page > 1 && cs )
		{
			m_page = 1;
			SetFKText(m_cursor_mode);
		}
		else if( cs )
		{
			CancelSelection();
			OnInfoBoxSendMess( "part_list ");
		}
		else
			OnRButtonDown( nFlags, CPoint(0,0) );
		goto goodbye;
	}
	if( nChar == 'M' )
	{
		if( !CurDragging() )
		{
			CancelSelection();
			SetCursorMode( CUR_DRAG_MEASURE_1 );
			m_dlist->StartDraggingArray( pDC, m_last_mouse_point.x, m_last_mouse_point.y, 1 ); 
		}
		else if( m_cursor_mode == CUR_DRAG_MEASURE_1 || m_cursor_mode == CUR_DRAG_MEASURE_2 )
		{
			m_dlist->StopDragging();
			SetCursorMode( CUR_NONE_SELECTED );
		}
	}
	if( nChar == 'L' )
	{
		m_Doc->OnViewLog();
	}
	if( nChar == 'H' )
	{
		m_Doc->m_dlist->HighlightAll();
	}
	if( nChar == VK_BACK )
	{
		// backspace, see if we are routing
		if( m_cursor_mode == CUR_DRAG_RAT )
		{
			// backup, if possible, by unrouting preceding segment and changing active layer
			if( m_dir == 0 && m_sel_is > 0 )
			{
				// routing forward
				if( m_sel_net->connect[m_sel_ic].vtx[m_sel_is].tee_ID )
				{
					AfxMessageBox( "tee-vertex reached" );
				}
				else
				{
					SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
					m_sel_net->connect[m_sel_ic].seg[m_sel_is-1].width = 0;
					m_sel_net->connect[m_sel_ic].seg[m_sel_is-1].layer = LAY_RAT_LINE;
					m_Doc->m_nlist->MergeUnroutedSegments( m_sel_net, m_sel_ic );
					m_sel_is = min( max(m_sel_is-1,0), m_sel_con.nsegs-1 );
					while( m_sel_is>0 )
						if( m_sel_con.seg[m_sel_is].width )
							m_sel_is--;
						else
							break;

					ShowSelectStatus();
					m_last_mouse_point.x = m_sel_net->connect[m_sel_ic].vtx[m_sel_is].x;
					m_last_mouse_point.y = m_sel_net->connect[m_sel_ic].vtx[m_sel_is].y;
					CPoint p = m_dlist->PCBToScreen( m_last_mouse_point );
					SetCursorPos( p.x, p.y );
					OnRatlineRoute();
					int new_active_layer = m_sel_net->connect[m_sel_ic].seg[m_sel_is].layer;
					if( m_sel_is )
						new_active_layer = m_sel_net->connect[m_sel_ic].seg[m_sel_is-1].layer;
					// set layer visible
					m_Doc->m_vis[new_active_layer] = TRUE;
					m_Doc->m_dlist->m_vis[new_active_layer] = TRUE;
				}
			}
			else if( m_dir == 1 && m_sel_is < m_sel_net->connect[m_sel_ic].nsegs-1 )
			{
				// routing backward, not at end of stub trace
				if( m_sel_net->connect[m_sel_ic].vtx[m_sel_is+1].tee_ID )
				{
					AfxMessageBox( "tee-vertex reached" );
				}
				else
				{
					SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
					m_Doc->m_nlist->CancelDraggingSegment( m_sel_net, m_sel_ic, m_sel_is );	
					m_sel_net->connect[m_sel_ic].seg[m_sel_is+1].width = 0;
					m_sel_net->connect[m_sel_ic].seg[m_sel_is+1].layer = LAY_RAT_LINE;
					m_Doc->m_nlist->MergeUnroutedSegments( m_sel_net, m_sel_ic );
					ShowSelectStatus();
					m_last_mouse_point.x = m_sel_net->connect[m_sel_ic].vtx[m_sel_is+1].x;
					m_last_mouse_point.y = m_sel_net->connect[m_sel_ic].vtx[m_sel_is+1].y;
					CPoint p = m_dlist->PCBToScreen( m_last_mouse_point );
					SetCursorPos( p.x, p.y );
					OnRatlineRoute();
					int new_active_layer = m_sel_net->connect[m_sel_ic].seg[m_sel_is].layer;
					if( m_sel_is < (m_sel_con.nsegs-1) )
						new_active_layer = m_sel_net->connect[m_sel_ic].seg[m_sel_is+1].layer;
					// set layer visible
					m_Doc->m_vis[new_active_layer] = TRUE;
					m_Doc->m_dlist->m_vis[new_active_layer] = TRUE;
				}
			}
		}
		else if( m_cursor_mode == CUR_DRAG_STUB )
		{
			// routing stub trace
			if( m_sel_is > 1 )
			{
				if( m_sel_net->connect[m_sel_ic].vtx[m_sel_is-1].tee_ID )
				{
					AfxMessageBox( "tee-vertex reached" );
				}
				else
				{
					SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
					int sel_ic = m_Doc->m_nlist->RemoveSegment( m_sel_net, m_sel_ic, m_sel_is-1 );
					if( sel_ic >= 0 )
					{
						m_sel_ic = sel_ic;
						int ns = m_sel_con.nsegs;
						m_sel_is = ns;
						ShowSelectStatus();
						m_last_mouse_point.x = m_sel_net->connect[m_sel_ic].vtx[m_sel_is].x;
						m_last_mouse_point.y = m_sel_net->connect[m_sel_ic].vtx[m_sel_is].y;
						CPoint p = m_dlist->PCBToScreen( m_last_mouse_point );
						SetCursorPos( p.x, p.y );
						OnEndVertexAddSegments();
						int new_active_layer = m_sel_net->connect[m_sel_ic].seg[m_sel_is-1].layer;
						// set layer visible
						m_Doc->m_vis[new_active_layer] = TRUE;
						m_Doc->m_dlist->m_vis[new_active_layer] = TRUE;
					}
				}
			}
			else
			{
				if( m_sel_net->connect[m_sel_ic].vtx[m_sel_is].tee_ID )
				{
					AfxMessageBox( "tee-vertex reached" );
				}
				else
				{
					m_Doc->m_nlist->CancelDraggingStub( m_sel_net, m_sel_ic, m_sel_is );
					SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
					cpart * sel_part = m_Doc->m_plist->GetPart( m_sel_start_pin.ref_des );
					int i = m_Doc->m_nlist->GetPinIndexByNameForPart( m_sel_start_pin.part, m_sel_start_pin.pin_name, m_sel_con.vtx[0].x, m_sel_con.vtx[0].y );
					m_Doc->m_nlist->UndrawConnection( m_sel_net, m_sel_ic );
					m_Doc->m_nlist->RemoveNetConnect( m_sel_net, m_sel_ic );
					CancelSelection();
					m_sel_net = NULL;
					m_sel_part = sel_part;
					m_sel_id = sel_part->m_id;
					m_sel_id.st = ID_PAD;
					m_sel_id.i = i;
					m_Doc->m_plist->SelectPad( sel_part, i, 0, 0 );
					OnPadStartStubTrace();
				}
			}
		}
		goto goodbye;
	}
	int fk = FK_NONE;
	int dx = 0;
	int dy = 0;

	// test for pressing key to change layer
	char test_char = nChar;
	if( test_char >= 97 )
		test_char = '1' + nChar - 97;
	char * ch = strchr( layer_char, nChar );
	if( ch )
	{
		int ilayer = ch - layer_char;
		if( ilayer < (m_Doc->m_num_copper_layers+2) )
		{
			// OK, shortcut key to a copper layer
			int new_active_layer = ilayer + LAY_SM_TOP;
			//
			if( !gShiftKeyDown )
			{
				BOOL switch_silk = FALSE;
				// shift key not held down, change active layer for routing
				if( m_cursor_mode == CUR_DRAG_RAT || m_cursor_mode == CUR_DRAG_STUB )
				{
					// if we are routing, change layer
					pDC->SelectClipRgn( &m_pcb_rgn );
					SetDCToWorldCoords( pDC );
					if( m_sel_id.ii == 0 && m_dir == 0 )
					{
						// we are trying to change first segment from pad
						int pin_index = m_Doc->m_nlist->GetPinIndexByNameForPart( m_sel_start_pin.part, m_sel_start_pin.pin_name, m_sel_con.vtx[0].x, m_sel_con.vtx[0].y );
						if( m_sel_net->pin[m_sel_con.start_pin].part->shape->m_padstack[pin_index].hole_size == 0)
						{
							// SMT pad, this is illegal;
							new_active_layer = -1;
							//PlaySound( TEXT("CriticalStop"), 0, 0 );
						}
					}
					else if( m_sel_id.ii == (m_sel_con.nsegs-1) && m_dir == 1 )
					{
						// we are trying to change last segment to pad
						if( m_sel_con.end_pin >= 0 )
						{
							int pin_index = m_Doc->m_nlist->GetPinIndexByNameForPart(	m_sel_end_pin.part, 
																						m_sel_end_pin.pin_name, 
																						m_sel_con.vtx[m_sel_con.nsegs].x, 
																						m_sel_con.vtx[m_sel_con.nsegs].y );
							if( m_sel_net->pin[m_sel_con.end_pin].part->shape->m_padstack[pin_index].hole_size == 0)
							{
								// SMT pad
								new_active_layer = -1;
								//PlaySound( TEXT("CriticalStop"), 0, 0 );
							}
						}
					}
					if( new_active_layer != -1 )
					{
						int lay = (new_active_layer<LAY_TOP_COPPER?new_active_layer+2:new_active_layer);
						m_dlist->ChangeRoutingLayer( pDC, lay, LAY_SELECTION, 0 );
						m_Doc->m_vis[lay] = TRUE;
						m_Doc->m_dlist->m_vis[lay] = TRUE;
						if( m_active_layer == lay )
							switch_silk = TRUE;
						else if( lay != new_active_layer && (m_Doc->m_vis[LAY_SM_TOP] || m_Doc->m_vis[LAY_SM_BOTTOM]) )
							m_active_layer = new_active_layer;
						else
							m_active_layer = lay;
						InvalidateLeftPane();
						if( m_sel_net )
						{
							if( (m_cursor_mode == CUR_DRAG_STUB && !en_branch) || m_cursor_mode == CUR_DRAG_RAT )
							{
								m_dlist->CancelHighLight();
								m_Doc->m_plist->HighlightAllPadsOnNet(	m_sel_net, 1, new_active_layer );
							}
							else if ( en_branch == BRANCH_TO_VERTEX ) 
							{	
								m_Doc->m_nlist->HighlightNetVertices( m_sel_net, FALSE );
							}
							else if( en_branch == BRANCH_TO_LINE )
							{
								m_Doc->m_nlist->HighlightNetConnections( m_sel_net );
							}
						}
					}
				}
				else
				{
					BOOL bOk = 0;
					if( (new_active_layer == LAY_SM_TOP || new_active_layer == LAY_SM_BOTTOM) &&
						(!m_Doc->m_vis[LAY_SM_TOP] && !m_Doc->m_vis[LAY_SM_BOTTOM]) )
					{
						new_active_layer += 2;
						InvalidateLeftPane();
					}
					if( m_Doc->m_vis[new_active_layer] )
						bOk = 1;
					else if( (new_active_layer == LAY_SM_TOP || new_active_layer == LAY_SM_BOTTOM) &&
							 (m_Doc->m_vis[LAY_SM_TOP] || m_Doc->m_vis[LAY_SM_BOTTOM]) )
						bOk = 1;
					else if(new_active_layer == LAY_TOP_COPPER && m_active_layer == LAY_SM_BOTTOM && m_Doc->m_vis[LAY_BOTTOM_COPPER])
						bOk = 1;
					else if(new_active_layer == LAY_BOTTOM_COPPER && m_active_layer == LAY_SM_TOP && m_Doc->m_vis[LAY_TOP_COPPER])
						bOk = 1;
					if( bOk )
					{
						m_Doc->m_vis[new_active_layer] = TRUE;
						m_Doc->m_dlist->m_vis[new_active_layer] = TRUE;
						if( m_active_layer == new_active_layer )
							switch_silk = TRUE;
						else
							m_active_layer = new_active_layer;			
						InvalidateLeftPane();
					}
				}
				if( m_active_layer == LAY_SM_TOP )
				{
					if( m_Doc->m_vis[LAY_REFINE_BOT] )
					{
						m_Doc->m_dlist->m_vis[LAY_REFINE_TOP] =	TRUE;
						m_Doc->m_vis[LAY_REFINE_TOP] = TRUE;
					}
					if( m_Doc->m_vis[LAY_SILK_BOTTOM] )
					{
						m_Doc->m_dlist->m_vis[LAY_SILK_TOP] = TRUE;
						m_Doc->m_vis[LAY_SILK_TOP] = TRUE;
					}	
					if( m_Doc->m_vis[LAY_BOTTOM_COPPER] )
					{
						m_Doc->m_dlist->m_vis[LAY_TOP_COPPER] =	TRUE;
						m_Doc->m_vis[LAY_TOP_COPPER] = TRUE;
					}
					m_Doc->m_dlist->m_vis[LAY_SM_TOP] =	TRUE;
					m_Doc->m_vis[LAY_SM_TOP] = TRUE;
					//
					m_Doc->m_dlist->m_vis[LAY_REFINE_BOT] =		FALSE;
					m_Doc->m_dlist->m_vis[LAY_SILK_BOTTOM] =	FALSE;
					m_Doc->m_dlist->m_vis[LAY_SM_BOTTOM] =		FALSE;
					m_Doc->m_dlist->m_vis[LAY_BOTTOM_COPPER] =	FALSE;
					m_Doc->m_vis[LAY_REFINE_BOT] =		FALSE;
					m_Doc->m_vis[LAY_SILK_BOTTOM] =		FALSE;
					m_Doc->m_vis[LAY_SM_BOTTOM] =		FALSE;
					m_Doc->m_vis[LAY_BOTTOM_COPPER] =	FALSE;
				}
				else if( m_active_layer == LAY_SM_BOTTOM )
				{
					if( m_Doc->m_vis[LAY_REFINE_TOP] )
					{
						m_Doc->m_dlist->m_vis[LAY_REFINE_BOT] =	TRUE;
						m_Doc->m_vis[LAY_REFINE_BOT] = TRUE;
					}
					if( m_Doc->m_vis[LAY_SILK_TOP] )
					{
						m_Doc->m_dlist->m_vis[LAY_SILK_BOTTOM] = TRUE;   
						m_Doc->m_vis[LAY_SILK_BOTTOM] =	TRUE;
					}
					if( m_Doc->m_vis[LAY_TOP_COPPER] )
					{
						m_Doc->m_dlist->m_vis[LAY_BOTTOM_COPPER] = TRUE;
						m_Doc->m_vis[LAY_BOTTOM_COPPER] = TRUE;	
					}
					m_Doc->m_dlist->m_vis[LAY_SM_BOTTOM] = TRUE;   
					m_Doc->m_vis[LAY_SM_BOTTOM] = TRUE; 
					//
					m_Doc->m_dlist->m_vis[LAY_REFINE_TOP] =		FALSE;
					m_Doc->m_dlist->m_vis[LAY_SILK_TOP] =		FALSE;
					m_Doc->m_dlist->m_vis[LAY_SM_TOP] =			FALSE;
					m_Doc->m_dlist->m_vis[LAY_TOP_COPPER] =		FALSE;
					m_Doc->m_vis[LAY_REFINE_TOP] =		FALSE;
					m_Doc->m_vis[LAY_SILK_TOP] =		FALSE;
					m_Doc->m_vis[LAY_SM_TOP] =			FALSE;
					m_Doc->m_vis[LAY_TOP_COPPER] =		FALSE; 
					
				}
				ShowActiveLayer(m_Doc->m_num_copper_layers,switch_silk);
				if( m_cursor_mode == CUR_DRAG_END_VTX )
				{
					m_Doc->m_dlist->CancelHighLight();
					m_Doc->m_nlist->HighlightNetVertices( m_sel_net, FALSE, FALSE );
				}
				goto goodbye;
			}
			else
			{
				// shift key held down, change layer if item selected 
				if( m_cursor_mode == CUR_SEG_SELECTED )
				{
					SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
					int w=m_sel_seg.width;
					int via_w = m_Doc->m_via_w;
					int via_hole_w = m_Doc->m_via_hole_w;
					GetWidthsForSegment( &w, &via_w, &via_hole_w );
					m_Doc->m_nlist->ChangeSegmentLayer( m_sel_net, m_sel_ic, m_sel_is, (new_active_layer<LAY_TOP_COPPER?new_active_layer+2:new_active_layer), via_w, via_hole_w );
					m_Doc->ProjectModified( TRUE );
				}
				else if( m_cursor_mode == CUR_CONNECT_SELECTED )
				{
					SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
					m_Doc->m_nlist->UndrawConnection( m_sel_net, m_sel_ic );
					cconnect * c = &m_sel_net->connect[m_sel_ic];
					int nl = new_active_layer;
					if( new_active_layer<LAY_TOP_COPPER )
						nl += 2;
					for( int is=0; is<c->nsegs; is++ )
					{
						cseg * seg = &c->seg[is];
						seg->layer = nl;
					}
					m_Doc->m_nlist->DrawConnection( m_sel_net, m_sel_ic );
					m_Doc->ProjectModified( TRUE );
				}
				else if( m_cursor_mode == CUR_AREA_CORNER_SELECTED 
					|| m_cursor_mode == CUR_AREA_SIDE_SELECTED )
				{
					if( new_active_layer<LAY_TOP_COPPER )
					{
						int ret = AfxMessageBox( "To copy the copper area to solder mask layer?", MB_YESNO );
						if( ret == IDYES )
						{
							int gsz = m_Doc->m_outline_poly.GetSize();
							m_Doc->m_outline_poly.SetSize(gsz+1);
							id sm_id( ID_POLYLINE, ID_SM_CUTOUT, gsz );						
							m_Doc->m_outline_poly[gsz].Copy(m_sel_net->area[m_sel_ia].poly);							
							m_Doc->m_outline_poly[gsz].SetId(&sm_id);
							m_Doc->m_outline_poly[gsz].SetLayer(new_active_layer);	
							m_Doc->m_outline_poly[gsz].SetDisplayList( m_Doc->m_dlist );
						}
					}
					else
					{
						SaveUndoInfoForAllAreasInNet( m_sel_net, TRUE, m_Doc->m_undo_list );
						carea * a = &m_sel_net->area[m_sel_ia];
						a->poly->Undraw();
						a->poly->SetLayer( new_active_layer );
						a->poly->Draw( m_dlist );
						int ret = m_Doc->m_nlist->AreaPolygonModified( m_sel_net, m_sel_ia, TRUE, TRUE );
						if( ret == -1 )
						{
							// error
							AfxMessageBox( "Error: Unable to clip polygon due to intersecting arc" );
							m_Doc->OnEditUndo();
						}
						else
						{
							m_Doc->m_nlist->OptimizeConnections(  m_sel_net, -1, m_Doc->m_auto_ratline_disable,
															m_Doc->m_auto_ratline_min_pins, TRUE  );
						}
					}
					CancelSelection();
					m_Doc->ProjectModified( TRUE );
				}
				goto goodbye;
			}
		}
	}

	// continue
	if( nChar >= 112 && nChar <= 123 )
	{
		// function key pressed
		fk = m_fkey_option[nChar-112];
	}
	if( nChar >= VK_LEFT && nChar <= VK_DOWN )
	{
		// arrow key
		BOOL bShift;
		SHORT kc = GetKeyState( VK_SHIFT );
		if( kc < 0 )
			bShift = TRUE;
		else
			bShift = FALSE;
		fk = FK_ARROW;
		int d;
		if( bShift && m_Doc->m_units == MM )
			d = 10000;		// 0.01 mm
		else if( bShift && m_Doc->m_units == MIL )
			d = 25400;		// 1 mil
		else if( m_sel_id.type == ID_NET && m_Doc->m_routing_grid_spacing > 999 )
			d = m_Doc->m_routing_grid_spacing;
		else
			d = m_Doc->m_part_grid_spacing;
		if( nChar == VK_LEFT )
			dx -= d;
		else if( nChar == VK_RIGHT ) 
			dx += d;
		else if( nChar == VK_UP ) 
			dy += d;
		else if( nChar == VK_DOWN ) 
			dy -= d;
	}
	else
		gLastKeyWasArrow = FALSE;

	if( fk == FK_ARROW )
	{
		if( m_cursor_mode == CUR_PAD_SELECTED )
		{		
			cpart * mp = m_sel_part;
			UnSelect( m_sel_part, &m_sel_id );
			id sel_id( ID_PART_DEF );
			NewSelect( mp, &sel_id, TRUE, 0 );
		}
		else if( m_cursor_mode == CUR_AREA_SIDE_SELECTED ||
			     m_cursor_mode == CUR_OP_SIDE_SELECTED )
		{
			SetCursorMode(CUR_GROUP_SELECTED);
		}
	}
	switch( m_cursor_mode )
	{
	case  CUR_NONE_SELECTED:
		if( fk == FK_ADD_AREA )
			AddArea();
		else if( fk == FK_ADD_TEXT )
			OnTextAdd();
		else if( fk == FK_ADD_PART )
			m_Doc->OnAddPart();
		else if( fk == FK_ADD_LINE )
			AddOutlinePoly(TRUE);
		else if( fk == FK_SHOW_M )
		{
			CancelSelection();
			BOOL bF = 0;
			int nm = m_Doc->m_mlist->GetSize();
			for( int im=0; im<nm; im++ )
			{
				RECT mr;
				mr.left = mr.bottom = INT_MAX;
				mr.right = mr.top = -INT_MAX;
				for( cpart * p=m_Doc->m_plist->GetFirstPart(); p; p=m_Doc->m_plist->GetNextPart(p) )
				{
					RECT pr;
					if( p->shape )
						if( p->m_merge == im )
							if( m_Doc->m_vis[LAY_TOP_COPPER+p->side] )
								if( m_Doc->m_plist->GetPartBoundingRect(p,&pr) )
									SwellRect( &mr, pr );
				}
				if( mr.right > mr.left )
				{
					id ID(ID_TEXT_DEF);
					CString str = m_Doc->m_mlist->GetMerge(im);
					int font_size = (mr.right - mr.left)/(str.GetLength()+1);
					int stroke_width = font_size/6;
					CPoint mid( mr.left+font_size/2, (mr.top+mr.bottom)/2 - font_size/2 );			
					CText * ht = m_Doc->m_tlist->AddText( mid.x, mid.y, 0/*angle*/, 0/*mirror*/, 0/*bNegative*/,
														  LAY_PAD_THRU, font_size, stroke_width, &str );
					m_Doc->m_dlist->HighLight(ht->dl_el);
					clrbit( ht->dl_el->layers_bitmap, LAY_PAD_THRU );
					m_Doc->m_dlist->Cpy( ht->dl_el );
					m_Doc->m_tlist->RemoveText( ht );
					// Rectangle
					ID.Set(0,0,0,0,0);
					dl_element * el_bounds = m_Doc->m_dlist->Add( ID, NULL, 0, DL_RECT, TRUE, &mr, 0, NULL, 0 );
					el_bounds->transparent = TRANSPARENT_BACKGND;
					m_Doc->m_dlist->HighLight( el_bounds );
					setbit( el_bounds->map_orig_layer, mod_active_layer );
					//
					bF = 1;
				}
			}
			if( bF )
				SetCursorMode(CUR_GROUP_SELECTED);
		}
		else if( fk == FK_CHECK_TRACES )
			m_Doc->OnToolsCheckTraces();
		else if( fk == FK_CANCEL_HILITE )
			m_Doc->OnToolsCheckTraces();
		else if( fk == FK_GRID_STYLE )
		{
			if( m_Doc->m_dlist )
				m_Doc->m_dlist->SetGridStyle(1-m_Doc->m_dlist->GetGridStyle());
			Invalidate(FALSE);
		}
		else if( fk == FK_REDO_RATLINES )
		{
			SaveUndoInfoForAllNets( TRUE, m_Doc->m_undo_list );
			//			StartTimer();
			m_Doc->m_nlist->OptimizeConnections(0,0,0);
			//			double time = GetElapsedTime();
		}
		break;

	case CUR_PART_SELECTED:
		if( fk == FK_ARROW )
		{
			if( !gLastKeyWasArrow )
			{
				if( m_sel_part->glued )
				{
					int ret = AfxMessageBox( "This part is glued, do you want to unglue it ?  ", MB_YESNO );
					if( ret == IDYES )
					{
						m_sel_part->glued = 0;
						SetFKText( m_cursor_mode );
					}
					else
						goto goodbye;
				}
				SaveUndoInfoForPartAndNets( m_sel_part,
					CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
				gTotalArrowMoveX = 0;
				gTotalArrowMoveY = 0;
				gLastKeyWasArrow = TRUE;
			}
			for (cnet * n = m_Doc->m_nlist->GetFirstNet(); n; n = m_Doc->m_nlist->GetNextNet(/*LABEL*/) )
			{
				for (int ic=0; ic<n->nconnects; ic++)
				{
					if( n->connect[ic].nsegs )
					{
						int ip = n->connect[ic].start_pin;
						int ipe = n->connect[ic].end_pin;
						if ( n->pin[ip].part == m_sel_part )
							if( n->connect[ic].vtx[1].tee_ID == NULL )
								if (n->connect[ic].seg[0].layer >= LAY_TOP_COPPER)
									if( /*ipe == cconnect::NO_END ||*/ n->connect[ic].nsegs > 1 )	
									{
										float a = abs(Angle(	n->connect[ic].vtx[1].x,
															n->connect[ic].vtx[1].y,
															n->connect[ic].vtx[0].x,
															n->connect[ic].vtx[0].y, NULL ));
										if( a < 0.5 || a > 179.5 ||
											InRange( a, 44.5, 45.5 ) ||
										    InRange( a, 89.5, 90.5 ) || 
											InRange( a, 134.5, 135.5) ) 
											MoveSegment( n, ic, 0, dx, dy, TRUE );
									}
						if (ipe >= 0 && ipe < n->npins)
							if ( n->pin[ipe].part == m_sel_part )
								if( n->connect[ic].nsegs > 1 )
									if( n->connect[ic].vtx[n->connect[ic].nsegs-1].tee_ID == NULL )
										if( n->connect[ic].seg[(n->connect[ic].nsegs-1)].layer >= LAY_TOP_COPPER)
										{
											int a = abs(Angle(	n->connect[ic].vtx[n->connect[ic].nsegs].x,
															n->connect[ic].vtx[n->connect[ic].nsegs].y,
															n->connect[ic].vtx[n->connect[ic].nsegs-1].x,
															n->connect[ic].vtx[n->connect[ic].nsegs-1].y, NULL ));
											if( a < 0.5 || a > 179.5 ||
												InRange( a, 44.5, 45.5 ) ||
												InRange( a, 89.5, 90.5 ) || 
												InRange( a, 134.5, 135.5) )
												MoveSegment( n, ic, (n->connect[ic].nsegs-1), dx, dy, TRUE );
										}
					}
				}
			}
			m_dlist->CancelHighLight();
			m_Doc->m_plist->Move( m_sel_part,
				m_sel_part->x+dx,
				m_sel_part->y+dy,
				m_sel_part->angle,
				m_sel_part->side );
			m_Doc->m_nlist->PartMoved( m_sel_part , FALSE );
			m_Doc->m_nlist->OptimizeConnections( m_sel_part, m_Doc->m_auto_ratline_disable, 
										m_Doc->m_auto_ratline_min_pins );
			gTotalArrowMoveX += dx;
			gTotalArrowMoveY += dy;
			m_Doc->m_plist->SelectPads( m_sel_part, m_seg_clearance, m_active_layer, TRANSPARENT_BACKGND );
			ShowRelativeDistance( m_sel_part->x, m_sel_part->y, 
				gTotalArrowMoveX, gTotalArrowMoveY );
			m_Doc->ProjectModified( TRUE );
		}
		else if( fk == FK_DELETE_PART || nChar == VK_DELETE )
			DeletePart();
		else if( fk == FK_EDIT_PART )
			m_Doc->PartProperties();
		else if( fk == FK_EDIT_FOOTPRINT )
		{
			m_Doc->m_edit_footprint = TRUE;
			OnPartEditFootprint();
		}
		else if( fk == FK_GLUE_PART )
		{
			OnPartGlue();
			m_Doc->m_dlist->CancelHighLight();
			SelectPart(m_sel_part);
		}
		else if( fk == FK_UNGLUE_PART )
		{
			OnPartUnglue();
			m_Doc->m_dlist->CancelHighLight();
			SelectPart(m_sel_part);
		}
		else if( fk == FK_MOVE_PART )
			OnPartMove();
		else if( fk == FK_ROTATE_GROUP )
		{
			SetCursorMode( CUR_GROUP_SELECTED );
			m_page = 3;
			SetFKText( m_cursor_mode );
		}
		else if( fk == FK_REDO_RATLINES )
			OnPartOptimize();
		else if ( fk == FK_TAKE_APART )
		{
			NewSelectM( NULL, m_sel_part->m_merge );
			Invalidate( FALSE );
			UpdateWindow();
			ExplodeGroup();
		}
		break;

	case CUR_REF_SELECTED:
		if( fk == FK_ARROW )
		{
			if( !gLastKeyWasArrow )
			{
				SaveUndoInfoForPart( m_sel_part, CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
				gTotalArrowMoveX = 0;
				gTotalArrowMoveY = 0;
				gLastKeyWasArrow = TRUE;
			}
			m_dlist->CancelHighLight();
			CPoint ref_pt = m_Doc->m_plist->GetRefPoint( m_sel_part );
			m_Doc->m_plist->MoveRefText( m_sel_part,
										ref_pt.x + dx,
										ref_pt.y + dy,
										m_sel_part->m_ref_angle,
										m_sel_part->m_ref_size,
										m_sel_part->m_ref_w );
			gTotalArrowMoveX += dx;
			gTotalArrowMoveY += dy;
			m_Doc->m_plist->SelectRefText( m_sel_part );
			ShowRelativeDistance( m_Doc->m_plist->GetRefPoint(m_sel_part).x,
				m_Doc->m_plist->GetRefPoint(m_sel_part).y,
				gTotalArrowMoveX, gTotalArrowMoveY );
			m_Doc->ProjectModified( TRUE );
		}
		else if( fk == FK_SET_PARAMS )
			RefProperties();
		else if( fk == FK_MOVE_REF )
			OnRefMove();
		else if( fk == FK_ROTATE_REF )
			OnRefRotateCW();
		else if( fk == FK_ROTATE_REF_CCW )
			OnRefRotateCCW();
		break;

	case CUR_VALUE_SELECTED:
		if( fk == FK_ARROW )
		{
			if( !gLastKeyWasArrow )
			{
				SaveUndoInfoForPart( m_sel_part, CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
				gTotalArrowMoveX = 0;
				gTotalArrowMoveY = 0;
				gLastKeyWasArrow = TRUE;
			}
			m_dlist->CancelHighLight();
			CPoint val_pt = m_Doc->m_plist->GetValuePoint( m_sel_part );
			m_Doc->m_plist->MoveValueText( m_sel_part,
										val_pt.x + dx,
										val_pt.y + dy,
										m_sel_part->m_value_angle,
										m_sel_part->m_value_size,
										m_sel_part->m_value_w );
			gTotalArrowMoveX += dx;
			gTotalArrowMoveY += dy;
			m_Doc->m_plist->SelectValueText( m_sel_part );
			ShowRelativeDistance( m_Doc->m_plist->GetValuePoint(m_sel_part).x,
				m_Doc->m_plist->GetValuePoint(m_sel_part).y,
				gTotalArrowMoveX, gTotalArrowMoveY );
			m_Doc->ProjectModified( TRUE );
		}
		else if( fk == FK_SET_PARAMS )
			OnValueProperties();
		else if( fk == FK_MOVE_VALUE )
			OnValueMove();
		else if( fk == FK_ROTATE_VALUE )
			OnValueRotateCW();
		else if( fk == FK_ROTATE_VALUE_CCW )
			OnValueRotateCCW();
		break;

	case CUR_RAT_SELECTED:
		if( fk == FK_SET_WIDTH )
			OnRatlineSetWidth();
		else if( fk == FK_LOCK_CONNECT )
			OnRatlineLockConnection();
		else if( fk == FK_UNLOCK_CONNECT )
			OnRatlineUnlockConnection();
		else if( fk == FK_ROUTE )
			OnRatlineRoute();
		else if( fk == FK_CHANGE_PIN )
			OnRatlineChangeEndPin();
		else if( fk == FK_UNROUTE_TRACE )
			OnUnrouteTrace();
		else if( fk == FK_DELETE_SEGMENT || nChar == VK_DELETE )
			OnSegmentDelete();
		else if( fk == FK_DELETE_CONNECT )
			OnSegmentDeleteTrace();
		else if( fk == FK_REDO_RATLINES )
			OnRatlineOptimize();
		else if( fk == FK_SPLIT_NET )
		{	
			if( m_Doc->m_project_modified )
			{
				AfxMessageBox( "The operation of bifurcating net is not possible to return, so now you need to save the file" );
			}
			else
			{
				cnet * nn = m_Doc->m_nlist->SplitNet( m_sel_net, m_sel_ic );
				CancelSelection();
			}
		}
		break;

	case  CUR_SEG_SELECTED:
		if( fk == FK_ARROW )
		{
			if(!SegmentMovable())
			{
				//PlaySound( TEXT("CriticalStop"), 0, 0 );
				break;
			}
			if( !gLastKeyWasArrow )
			{
				SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
				gTotalArrowMoveX = 0;
				gTotalArrowMoveY = 0;
				gLastKeyWasArrow = TRUE;
			}
			m_dlist->CancelHighLight();
			MoveSegment( m_sel_net, m_sel_ic, m_sel_is, dx, dy );
			ShowRelativeDistance( m_sel_vtx.x, m_sel_vtx.y, gTotalArrowMoveX, gTotalArrowMoveY );
			m_Doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, m_sel_is, m_seg_clearance );
			m_Doc->ProjectModified( TRUE );
		}
		if( fk == FK_SET_WIDTH )
			OnSegmentSetWidth();
		else if( fk == FK_CHANGE_LAYER )
			OnSegmentChangeLayer();
		else if( fk == FK_ADD_VERTEX )
			OnSegmentAddVertex();
		else if( fk == FK_MOVE_SEGMENT)
			OnSegmentMove();
		else if( fk == FK_UNROUTE )
			OnSegmentUnroute();
		else if( fk == FK_DELETE_SEGMENT || nChar == VK_DELETE )
			OnSegmentDelete();
		else if( fk == FK_UNROUTE_TRACE )
			OnUnrouteTrace();
		else if( fk == FK_SELECT_IN_LAYER )
		{
			id sel( ID_NET, ID_CONNECT, m_sel_ic, ID_SEG, 0 );
			if( m_sel_is > 0 )
				for( int ii=m_sel_is-1; ii>=0; ii-- )
				{
					sel.ii = ii;
					if( m_sel_seg.layer == m_sel_con.seg[ii].layer )
						NewSelect( m_sel_net, & sel, 0, 0 );
					else
						break;
				}
			if( m_sel_is+1 < m_sel_con.nsegs )
				for( int ii=m_sel_is+1; ii<m_sel_con.nsegs; ii++ )
				{
					sel.ii = ii;
					if( m_sel_seg.layer == m_sel_con.seg[ii].layer )
						NewSelect( m_sel_net, & sel, 0, 0 );
					else
						break;
				}
			if( m_sel_count > 1 )
			{
				m_Doc->m_dlist->CancelHighLight();
				SetCursorMode( CUR_GROUP_SELECTED );
				HighlightGroup();
			}
		}
		else if( fk == FK_DELETE_CONNECT )
			OnSegmentDeleteTrace();
		break;

	case  CUR_VTX_SELECTED:
		if( fk == FK_ARROW )
		{
			if( !gLastKeyWasArrow )
			{
				SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
				gTotalArrowMoveX = 0;
				gTotalArrowMoveY = 0;
				gLastKeyWasArrow = TRUE;
			}
			m_dlist->CancelHighLight();
			m_Doc->m_nlist->MoveVertex( m_sel_net, m_sel_ic, m_sel_is,
										m_sel_vtx.x + dx, m_sel_vtx.y + dy );
			gTotalArrowMoveX += dx;
			gTotalArrowMoveY += dy;
			ShowRelativeDistance( m_sel_vtx.x, m_sel_vtx.y, gTotalArrowMoveX, gTotalArrowMoveY );
			m_Doc->m_nlist->HighlightVertex( m_sel_net, m_sel_ic, m_sel_is, 0, m_seg_clearance);
			m_Doc->ProjectModified( TRUE );
		}
		else if( fk == FK_SET_POSITION )
			OnVertexProperties();
		else if( fk == FK_VIA_SIZE )
			OnVertexSize();
		else if( fk == FK_MOVE_VERTEX )
			OnVertexMove();
		else if( fk == FK_ALIGN_SEGMENTS )
		{
			AlignSegments(m_sel_net, m_sel_id.i, m_sel_id.ii, TRUE);
			m_Doc->ProjectModified( TRUE );
		}
		else if( fk == FK_ALIGN_X || fk == FK_ALIGN_Y )
		{
			if ((ppx||ppy)&&(prevx||prevy))
			{
				int merge = m_sel_con.m_merge;
				if( merge == m_prev_sel_merge )
					merge = -1;
				if( merge >= 0 )
				{
					CString ps;
					ps.Format("This trace is connected to other objects through the \"MERGE\" property. Move all objects of the group %s?", m_Doc->m_mlist->GetMerge( merge ) );
					if( AfxMessageBox( ps, MB_YESNO ) == IDNO )
					{
						merge = -1;
					}
				}
				if( fk == FK_ALIGN_X)
				{
					if( merge >= 0 )
					{
						int vx = m_sel_vtx.x;
						CancelSelection();
						NewSelectM( NULL, merge );
						MoveGroup( ppx-vx, 0, FALSE );
					}
					else
					{
						SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
						m_Doc->m_nlist->MoveVertex( m_sel_net, m_sel_id.i, m_sel_id.ii, ppx, prevy );
					}
					prevx = ppx;
				}
				else
				{
					if( merge >= 0 )
					{
						int vy = m_sel_vtx.y;
						CancelSelection();
						NewSelectM( NULL, merge );
						MoveGroup( 0, ppy-vy, FALSE );					
					}
					else
					{
						SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
						m_Doc->m_nlist->MoveVertex( m_sel_net, m_sel_id.i, m_sel_id.ii, prevx, ppy );					
					}
					prevy = ppy;
				}
				m_dlist->CancelHighLight();
				HighlightGroup();
				m_Doc->ProjectModified( TRUE );
			}
		}
		else if( fk == FK_ALIGN_MIDDLE )
		{
			if ( m_page == 2 )
			{
				m_page = 1;
				SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
				m_dlist->CancelHighLight();
				m_Doc->m_nlist->MoveVertex( m_sel_net, m_sel_id.i, m_sel_id.ii, prev_middle_x, prev_middle_y );
				m_Doc->m_nlist->HighlightVertex( m_sel_net, m_sel_id.i, m_sel_id.ii );
				m_Doc->ProjectModified( TRUE );
			}
			else
				m_page = 2;
			SetFKText( m_cursor_mode );
		}
		else if( fk == FK_ALIGN_MIDLINE )
		{
			if( m_sel_id.ii && m_sel_id.ii < m_sel_con.nsegs )
			{
				int x1 = m_sel_con.vtx[m_sel_id.ii-1].x;
				int y1 = m_sel_con.vtx[m_sel_id.ii-1].y;
				int x2 = m_sel_con.vtx[m_sel_id.ii+1].x;
				int y2 = m_sel_con.vtx[m_sel_id.ii+1].y;
				SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
				m_dlist->CancelHighLight();
				m_Doc->m_nlist->MoveVertex( m_sel_net, m_sel_id.i, m_sel_id.ii, (x1+x2)/2, (y1+y2)/2 );
				m_Doc->m_nlist->HighlightVertex( m_sel_net, m_sel_id.i, m_sel_id.ii );
				m_Doc->ProjectModified( TRUE );
				prevx = ppx;
				prevy = ppy;
				m_page = 1;
				SetFKText( m_cursor_mode );
			}
		}
		else if( fk == FK_ALIGN_MIDDLE_X )
		{
			SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
			m_dlist->CancelHighLight();
			m_Doc->m_nlist->MoveVertex( m_sel_net, m_sel_id.i, m_sel_id.ii, prev_middle_x, prevy );
			m_Doc->m_nlist->HighlightVertex( m_sel_net, m_sel_id.i, m_sel_id.ii );
			m_Doc->ProjectModified( TRUE );
			m_page = 1;
			SetFKText( m_cursor_mode );
		}
		else if( fk == FK_ALIGN_MIDDLE_Y )
		{
			SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
			m_dlist->CancelHighLight();
			m_Doc->m_nlist->MoveVertex( m_sel_net, m_sel_id.i, m_sel_id.ii, prevx, prev_middle_y );
			m_Doc->m_nlist->HighlightVertex( m_sel_net, m_sel_id.i, m_sel_id.ii );
			m_Doc->ProjectModified( TRUE );
			m_page = 1;
			SetFKText( m_cursor_mode );
		}
		else if( fk == FK_INSERT_SEGMENT )
		{
			SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
			InsertSegment(m_sel_net, m_sel_ic, m_sel_is, TRUE);
			int new_ic = m_sel_ic;
			int new_is = m_sel_is;
			cnet * net = m_sel_net;
			new_ic = m_Doc->m_nlist->OptimizeConnections( m_sel_net, m_sel_ic, m_Doc->m_auto_ratline_disable, 
										m_Doc->m_auto_ratline_min_pins );
			CancelSelection();
			// select segment
			m_sel_id.Set( ID_NET, ID_CONNECT, new_ic, ID_SEG, new_is );
			NewSelect( net, &m_sel_id, 1, 0 );
			OnSegmentMove();
		}
		else if( fk == FK_INSERT_VERTICES )
		{
			float vX = m_sel_vtx.x;
			float vY = m_sel_vtx.y;
			float memx = vX;
			float memy = vY;
			float lastX = m_sel_last_vtx.x;
			float lastY = m_sel_last_vtx.y;
			float nextX = m_sel_next_vtx.x;
			float nextY = m_sel_next_vtx.y;
			float cursorX = m_last_cursor_point.x;
			float cursorY = m_last_cursor_point.y;
			float a1 = Angle( lastX,lastY,vX,vY );
			float a2 = Angle( nextX,nextY,vX,vY );
			float a3 = Angle( cursorX,cursorY,vX,vY );
			if (InRange((int)abs(a1-a2)%180, 20, 160))
			{
				BOOL external = FALSE;
				BOOL f1 = FALSE;
				BOOL f2 = FALSE;
				Rotate_Vertex(&lastX,&lastY,-a1);
				Rotate_Vertex(&nextX,&nextY,-a1);
				Rotate_Vertex(&cursorX,&cursorY,-a1);
				if (( nextY > lastY && cursorY < lastY )
					|| ( nextY < lastY && cursorY > lastY ))
					f1 = TRUE;
				Rotate_Vertex(&lastX,&lastY,a1-a2);
				Rotate_Vertex(&nextX,&nextY,a1-a2);
				Rotate_Vertex(&cursorX,&cursorY,a1-a2);
				if (( lastY > nextY && cursorY < nextY )
					|| ( lastY < nextY && cursorY > nextY ))
					f2 = TRUE;
				if ( f1 && f2 )
					external = TRUE;
				float x = m_last_cursor_point.x - m_sel_vtx.x;
				float y = m_last_cursor_point.y - m_sel_vtx.y;
				Rotate_Vertex( &x, &y, -a1 );
				float newY2 = y;
				float newX2;
				newX2 = -y*tan((90-a1+a2)*M_PI/180.0);
				Rotate_Vertex( &newX2, &newY2, a1 );
				newX2 += m_sel_vtx.x;
				newY2 += m_sel_vtx.y;
				x = m_last_cursor_point.x - m_sel_vtx.x;
				y = m_last_cursor_point.y - m_sel_vtx.y;
				Rotate_Vertex( &x, &y, -a2 );
				float newY1 = y;
				float newX1;
				newX1 = y*tan((90-a1+a2)*M_PI/180.0);
				Rotate_Vertex( &newX1, &newY1, a2 );
				newX1 += m_sel_vtx.x;
				newY1 += m_sel_vtx.y;
				if (external)
				{
					float buf;
					buf = newX1;
					newX1 = newX2;
					newX2 = buf;
					buf = newY1;
					newY1 = newY2;
					newY2 = buf;
				}
				// make undo record
				SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
				m_Doc->m_nlist->MoveVertex( m_sel_net, m_sel_ic, m_sel_is, newX1, newY1 );
				int layer = m_sel_net->connect[m_sel_ic].seg[m_sel_is].layer;
				int w = m_sel_net->connect[m_sel_ic].seg[m_sel_is].width;
				int insert_flag;
				// new vertex
				insert_flag = m_Doc->m_nlist->InsertSegment( m_sel_net, m_sel_ic, m_sel_is,
					m_last_cursor_point.x, m_last_cursor_point.y,
					layer, w, 0, 0, 0 );
				insert_flag = m_Doc->m_nlist->InsertSegment( m_sel_net, m_sel_ic, m_sel_is+1,
					newX2, newY2,
					layer, w, 0, 0, 0 );
				if (external)
				{
					insert_flag = m_Doc->m_nlist->InsertSegment( m_sel_net, m_sel_ic, m_sel_is,
						newX1, newY1, layer, w, 0, 0, 0 );
					AlignSegments( m_sel_net, m_sel_ic, m_sel_is, FALSE, 45.0);
					insert_flag = m_Doc->m_nlist->InsertSegment( m_sel_net, m_sel_ic, m_sel_is+3,
						newX2, newY2, layer, w, 0, 0, 0 );
					AlignSegments( m_sel_net, m_sel_ic, m_sel_is+4, FALSE, 45.0);
				}
				if (f1 && !f2)
					AlignSegments( m_sel_net, m_sel_ic, m_sel_is, FALSE, 45.0);
				if (f2 && !f1)
					AlignSegments( m_sel_net, m_sel_ic, m_sel_is+2, FALSE, 45.0);  
				//
				m_sel_ic = m_Doc->m_nlist->OptimizeConnections( m_sel_net, m_sel_ic, m_Doc->m_auto_ratline_disable, 
																m_Doc->m_auto_ratline_min_pins );
				cnet * n = m_Doc->m_nlist->GetFirstNet();
				m_sel_con.utility = TRUE;
				m_sel_net = 0;
				m_sel_id.Clear();
				if (n)
				{	
					int active_lay = m_active_layer;
					if( active_lay < LAY_TOP_COPPER )
						active_lay += 2;
					double len, min_len;
					min_len = DEFAULT;
					do
					{
						for (int c=0; c<n->nconnects; c++)
						{
							for (int s=1; s<n->connect[c].nsegs; s++)
							{
								vX = n->connect[c].vtx[s].x;
								vY = n->connect[c].vtx[s].y;
								if (!(active_lay == n->connect[c].seg[s].layer || active_lay == n->connect[c].seg[s-1].layer))
									continue;
								if (m_dlist->m_vis[n->connect[c].seg[s].layer] == 0 || m_dlist->m_vis[n->connect[c].seg[s-1].layer] == 0)
									continue;
								len = (vX - memx)*(vX - memx) + (vY - memy)*(vY - memy);
								len = sqrt(len);
								if (len < min_len)
								{
									if ( n->connect[c].utility == 0)
									{
										min_len = len;
										m_sel_net = n;
										m_sel_id.Set( ID_NET, ID_CONNECT, c, ID_VERTEX, s );
									}
								}
							}
						}
					}while( (n = m_Doc->m_nlist->GetNextNet(/*LABEL*/) ) != NULL );
				}
				m_dlist->CancelHighLight();
				if (m_sel_net)
				{
					m_Doc->m_nlist->HighlightVertex(m_sel_net,m_sel_id.i,m_sel_id.ii);
					SetCursorMode(CUR_VTX_SELECTED);
				}
				else	
					CancelSelection();
				m_Doc->ProjectModified( TRUE );
			}
		}
		else if( fk == FK_ADD_CONNECT )
			OnVertexConnectToPin();
		else if( fk == FK_DELETE_VERTEX || nChar == VK_DELETE )
		{
			OnVertexDelete();
			CancelSelection();
		}
		else if( fk == FK_UNROUTE_TRACE )
			OnUnrouteTrace();
		else if( fk == FK_DELETE_CONNECT )
			OnSegmentDeleteTrace();
		else if( fk == FK_REDO_RATLINES )
			OnRatlineOptimize();
		break;

	case  CUR_END_VTX_SELECTED:
		if( fk == FK_ARROW )
		{
			if( !gLastKeyWasArrow )
			{
				SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
				gTotalArrowMoveX = 0;
				gTotalArrowMoveY = 0;
				gLastKeyWasArrow = TRUE;
			}
			m_dlist->CancelHighLight();
			m_Doc->m_nlist->MoveVertex( m_sel_net, m_sel_ic, m_sel_is,
										m_sel_vtx.x + dx, m_sel_vtx.y + dy );
			gTotalArrowMoveX += dx;
			gTotalArrowMoveY += dy;
			ShowRelativeDistance( m_sel_vtx.x, m_sel_vtx.y, gTotalArrowMoveX, gTotalArrowMoveY );
			m_Doc->m_nlist->HighlightVertex( m_sel_net, m_sel_ic, m_sel_is, 0, m_seg_clearance );
			m_Doc->ProjectModified( TRUE );
		}
		else if( fk == FK_SET_POSITION )
			OnVertexProperties();
		else if( fk == FK_ADD_CONNECT )
			OnVertexConnectToPin();
		else if( fk == FK_MOVE_VERTEX )
			OnEndVertexMove();
		else if( fk == FK_DELETE_VERTEX || nChar == VK_DELETE )
			OnVertexDelete();
		else if( fk == FK_ADD_SEGMENT )
			OnEndVertexAddSegments();
		else if( fk == FK_ADD_VIA )
			OnEndVertexAddVia();
		else if( fk == FK_VIA_SIZE )
			OnVertexSize();
		else if( fk == FK_DELETE_CONNECT )
			OnSegmentDeleteTrace();
		else if( fk == FK_REDO_RATLINES )
			OnRatlineOptimize();
		else if( fk == FK_ALIGN_X || fk == FK_ALIGN_Y )
		{
			if ((ppx||ppy)&&(prevx||prevy))
			{
				SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
				m_dlist->CancelHighLight();
				if( fk == FK_ALIGN_X)
				{
					m_Doc->m_nlist->MoveVertex( m_sel_net, m_sel_id.i, m_sel_id.ii, ppx, prevy );
					prevx = ppx;
				}
				else
				{
					m_Doc->m_nlist->MoveVertex( m_sel_net, m_sel_id.i, m_sel_id.ii, prevx, ppy );
					prevy = ppy;
				}
				m_Doc->m_nlist->HighlightVertex( m_sel_net, m_sel_id.i, m_sel_id.ii );
				m_Doc->ProjectModified( TRUE );
			}
		}
		else if( fk == FK_UNROUTE_TRACE )
			OnUnrouteTrace();
		break;

	case  CUR_CONNECT_SELECTED:
		if( fk == FK_SET_WIDTH )
			OnConnectSetWidth();
		else if( fk == FK_CHANGE_LAYER )
			OnConnectChangeLayer();
		else if( fk == FK_UNROUTE_TRACE )
			OnUnrouteTrace();
		else if( fk == FK_REDO_RATLINES )
			OnRatlineOptimize();	//**
		else if( fk == FK_DELETE_CONNECT || nChar == VK_DELETE )
			OnSegmentDeleteTrace();
		break;

	case  CUR_NET_SELECTED:
		if( fk == FK_SET_WIDTH )
			OnNetSetWidth();
		else if( fk == FK_CHANGE_LAYER )
			OnNetChangeLayer();
		else if( fk == FK_EDIT_NET )
			OnNetEditnet();
		else if( fk == FK_REDO_RATLINES )
			OnRatlineOptimize();
		break;

	case  CUR_PAD_SELECTED:
		 if(fk == FK_ALIGN_X || 
			fk == FK_ALIGN_Y || 
			fk == FK_ALIGN_MIDDLE ||
			fk == FK_ALIGN_MIDDLE_X ||
			fk == FK_ALIGN_MIDDLE_Y )
		{
			if ( fk == FK_ALIGN_MIDDLE && m_page == 1 )
			{
				m_page = 2;
				SetFKText(m_cursor_mode);
			}
			else if ((ppx||ppy)&&(prevx||prevy))
			{
				m_dlist->CancelHighLight();
				int merge = -1;
				merge = m_sel_part->m_merge;
				if( m_prev_sel_merge == m_sel_part->m_merge )
						merge = -1;
				if( merge >= 0 )
				{
					CString ps;
					ps.Format("This part is connected to other objects through the \"MERGE\" property. Move all objects of the group %s?", m_Doc->m_mlist->GetMerge( merge ) );
					if( AfxMessageBox( ps, MB_YESNO ) == IDNO )
					{
						merge = -1;
					}
				}
				if( fk == FK_ALIGN_X )
				{
					if ( merge >= 0 )
					{
						int x = (ppx - m_sel_part->pin[m_sel_id.i].x);
						CancelSelection();
						NewSelectM( NULL, merge );
						MoveGroup( x, 0, FALSE );
					}
					else
					{
						SaveUndoInfoForPartAndNets( m_sel_part, CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
						m_Doc->m_plist->Move( m_sel_part, (ppx + m_sel_part->x - m_sel_part->pin[m_sel_id.i].x), m_sel_part->y, m_sel_part->angle, m_sel_part->side );
						m_Doc->m_nlist->PartMoved( m_sel_part , FALSE );
					}
					prevx = ppx;
				}
				else if( fk == FK_ALIGN_Y)
				{
					if ( merge >= 0 )
					{
						int y = (ppy - m_sel_part->pin[m_sel_id.i].y);
						CancelSelection();
						NewSelectM( NULL, merge );
						MoveGroup( 0, y, FALSE );
					}
					else
					{
						SaveUndoInfoForPartAndNets( m_sel_part, CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
						m_Doc->m_plist->Move( m_sel_part, m_sel_part->x, (ppy + m_sel_part->y - m_sel_part->pin[m_sel_id.i].y), m_sel_part->angle, m_sel_part->side );
						m_Doc->m_nlist->PartMoved( m_sel_part , FALSE );
					}
					prevy = ppy;
				}
				else if( fk == FK_ALIGN_MIDDLE)
				{
					if ( merge >= 0 )
					{
						int x = (prev_middle_x - m_sel_part->pin[m_sel_id.i].x);
						int y = (prev_middle_y - m_sel_part->pin[m_sel_id.i].y);
						CancelSelection();
						NewSelectM( NULL, merge );
						MoveGroup( x, y, FALSE );
					}
					else
					{
						SaveUndoInfoForPartAndNets( m_sel_part, CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
						m_Doc->m_plist->Move(	m_sel_part, 
											(prev_middle_x + m_sel_part->x - m_sel_part->pin[m_sel_id.i].x), 
											(prev_middle_y + m_sel_part->y - m_sel_part->pin[m_sel_id.i].y), 
											m_sel_part->angle, m_sel_part->side );
						m_Doc->m_nlist->PartMoved( m_sel_part , FALSE );
					}
					prevx = ppx;
					prevy = ppy;
				}
				else if( fk == FK_ALIGN_MIDDLE_X)
				{
					if ( merge >= 0 )
					{
						int x = (prev_middle_x - m_sel_part->pin[m_sel_id.i].x);
						CancelSelection();
						NewSelectM( NULL, merge );
						MoveGroup( x, 0, FALSE );
					}
					else
					{
						SaveUndoInfoForPartAndNets( m_sel_part, CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
						m_Doc->m_plist->Move(	m_sel_part, 
											(prev_middle_x + m_sel_part->x - m_sel_part->pin[m_sel_id.i].x), 
											m_sel_part->y, 
											m_sel_part->angle, m_sel_part->side );
						m_Doc->m_nlist->PartMoved( m_sel_part , FALSE );
					}
					prevx = ppx;
				}
				else if( fk == FK_ALIGN_MIDDLE_Y)
				{
					if ( merge >= 0 )
					{
						int y = (prev_middle_y - m_sel_part->pin[m_sel_id.i].y);
						CancelSelection();
						NewSelectM( NULL, merge );
						UpdateWindow();
						MoveGroup( 0, y, FALSE );
					}
					else
					{
						SaveUndoInfoForPartAndNets( m_sel_part, CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
						m_Doc->m_plist->Move(	m_sel_part, 
											m_sel_part->x, 
											(prev_middle_y + m_sel_part->y - m_sel_part->pin[m_sel_id.i].y), 
											m_sel_part->angle, m_sel_part->side );
						m_Doc->m_nlist->PartMoved( m_sel_part , FALSE );
					}
					prevy = ppy;
				}
				if( !(m_sel_id.type == ID_NET && m_sel_id.st == ID_CONNECT) )
					m_Doc->m_nlist->OptimizeConnections(	m_Doc->m_auto_ratline_disable, 
															m_Doc->m_auto_ratline_min_pins );
				if( m_sel_count > 1 )
				{
					SetCursorMode(CUR_GROUP_SELECTED);
					m_Doc->m_dlist->CancelHighLight();
					HighlightGroup();
				}
				else if( m_sel_part && m_sel_id.i < m_sel_part->shape->GetNumPins() )
					m_Doc->m_plist->SelectPad( m_sel_part, m_sel_id.i, 0, 0 );
				m_Doc->ProjectModified( TRUE );
			}
		}
		else if( fk == FK_ATTACH_NET )
			OnPadAddToNet();
		else if( fk == FK_ADD_CONNECT )
			OnPadConnectToPin();
		else if( fk == FK_START_STUB )
			OnPadStartStubTrace();
		else if( fk == FK_MOVE_PART )
			OnPartMove();
		else if( fk == FK_EDIT_PART )
			m_Doc->PartProperties();
		else if( fk == FK_EDIT_FOOTPRINT )
		{
			m_Doc->m_edit_footprint = TRUE;
			OnPartEditFootprint();
		}
		else if( fk == FK_EDIT_NET )
		{
			m_sel_net = m_sel_part->pin[m_sel_id.i].net;
			if( m_sel_net )
			{
				CString nm = m_sel_part->shape->GetPinNameByIndex( m_sel_id.i );
				int ip = m_Doc->m_nlist->GetNetPinIndex( m_sel_net, &m_sel_part->ref_des, &nm );
				m_sel_id.Set(ID_NET,ID_ENTIRE_NET,0,0,0);
				SetCursorMode( CUR_NET_SELECTED );
				Editnet(ip);
			}
		}
		else if( fk == FK_DETACH_NET )
			OnPadDetachFromNet();
		else if( fk == FK_REDO_RATLINES )
			OnPadOptimize();
		else if( fk == FK_ROTATE_GROUP )
		{
			cpart * mp = m_sel_part;
			UnSelect( m_sel_part, &m_sel_id,TRUE);
			m_sel_id.Set( ID_PART_DEF );
			NewSelect( mp, &m_sel_id,TRUE,0);
			SetCursorMode( CUR_GROUP_SELECTED );
			m_page = 3;
			SetFKText( m_cursor_mode );
		}
		break;

	case CUR_TEXT_SELECTED:
		if( fk == FK_ARROW )
		{
			if( !gLastKeyWasArrow )
			{
				SaveUndoInfoForText( m_sel_text, CTextList::UNDO_TEXT_MODIFY, TRUE, m_Doc->m_undo_list );
				gTotalArrowMoveX = 0;
				gTotalArrowMoveY = 0;
				gLastKeyWasArrow = TRUE;
			}
			m_dlist->CancelHighLight();
			m_Doc->m_tlist->MoveText( m_sel_text,
						m_sel_text->m_x + dx, m_sel_text->m_y + dy,
						m_sel_text->m_angle, m_sel_text->m_mirror,
						m_sel_text->m_bNegative, m_sel_text->m_layer );
			gTotalArrowMoveX += dx;
			gTotalArrowMoveY += dy;
			ShowRelativeDistance( m_sel_text->m_x, m_sel_text->m_y, 
				gTotalArrowMoveX, gTotalArrowMoveY );
			m_Doc->m_tlist->HighlightText( m_sel_text );
			m_Doc->ProjectModified( TRUE );
		}
		else if( fk == FK_EDIT_TEXT )
			OnTextEdit();
		else if( fk == FK_MOVE_TEXT )
			OnTextMove();
		else if( fk == FK_ROTATE_REF )
		{
			m_sel_text->m_angle += 5;
			if( m_sel_text->m_angle >= 360 )
				m_sel_text->m_angle -= 360;
			m_sel_text->Undraw();
			m_sel_text->Draw( m_Doc->m_dlist, m_Doc->m_smfontutil );
			m_Doc->m_tlist->HighlightText( m_sel_text );
			m_Doc->ProjectModified( TRUE );
		}
		else if( fk == FK_ROTATE_REF_CCW )
		{
			m_sel_text->m_angle -= 5;
			if( m_sel_text->m_angle < 0 )
				m_sel_text->m_angle += 360;
			m_sel_text->Undraw();
			m_sel_text->Draw( m_Doc->m_dlist, m_Doc->m_smfontutil );
			m_Doc->m_tlist->HighlightText( m_sel_text );
			m_Doc->ProjectModified( TRUE );
		}
		else if( fk == FK_DELETE_TEXT || nChar == VK_DELETE )
			OnTextDelete();
		break;

	case CUR_OP_CORNER_SELECTED:
		if( fk == FK_ARROW )
		{
			if( !gLastKeyWasArrow )
			{
				SaveUndoInfoForOutlinePoly( UNDO_OP, TRUE, m_Doc->m_undo_list );
				gTotalArrowMoveX = 0;
				gTotalArrowMoveY = 0;
				gLastKeyWasArrow = TRUE;
			}
			CPolyLine * poly = &m_Doc->m_outline_poly[m_sel_id.i];
			poly->MoveCorner( m_sel_is,
				poly->GetX( m_sel_is ) + dx,
				poly->GetY( m_sel_is ) + dy );
			m_dlist->CancelHighLight();
			gTotalArrowMoveX += dx;
			gTotalArrowMoveY += dy;
			ShowRelativeDistance( poly->GetX( m_sel_is ), poly->GetY( m_sel_is ),
				gTotalArrowMoveX, gTotalArrowMoveY );
			poly->HighlightCorner( m_sel_is );
			m_Doc->ProjectModified( TRUE );
		}
		else if( fk == FK_SET_POSITION )
			OnOPCornerEdit();
		else if( fk == FK_ALIGN_SIDES )
		{
			AlignSides(m_sel_id.type, m_sel_id.i, m_sel_id.ii);
			m_Doc->ProjectModified( TRUE );
		}
		else if( fk == FK_MOVE_CORNER )
			OnOPCornerMove();
		else if(	fk == FK_ALIGN_X || 
					fk == FK_ALIGN_Y ||
					fk == FK_ALIGN_MIDDLE ||
					fk == FK_ALIGN_MIDDLE_X ||
					fk == FK_ALIGN_MIDDLE_Y )
		{
			if ( fk == FK_ALIGN_MIDDLE && m_page == 1 )
			{
				m_page = 2;
				SetFKText(m_cursor_mode);
			}
			else if ((ppx||ppy)&&(prevx||prevy))
			{
				SaveUndoInfoForOutlinePoly( UNDO_OP, TRUE, m_Doc->m_undo_list );
				m_dlist->CancelHighLight();
				int mer = m_Doc->m_outline_poly[m_sel_id.i].GetMerge();
				if( mer >= 0 )
				{
					CString ps;
					ps.Format("This polyline is connected to other objects through the \"MERGE\" property. Move all objects of the group %s?", m_Doc->m_mlist->GetMerge( mer ) );
					if( AfxMessageBox( ps, MB_YESNO ) == IDNO )
					{
						mer = -1;
					}
				}
				int bx = m_Doc->m_outline_poly[m_sel_id.i].GetX(m_sel_id.ii);
				int by = m_Doc->m_outline_poly[m_sel_id.i].GetY(m_sel_id.ii);
				if( mer >= 0 )
				{
					CancelSelection();
					NewSelectM(NULL,mer);
				}
				if( fk == FK_ALIGN_X)
				{
					if( mer >= 0 )
						MoveGroup(ppx-bx,prevy-by,TRUE);
					else
						m_Doc->m_outline_poly[m_sel_id.i].MoveCorner( m_sel_id.ii, ppx, prevy );
					prevx = ppx;
					prevy = ppy;
				}
				else if( fk == FK_ALIGN_Y)
				{
					if( mer >= 0 )
						MoveGroup(prevx-bx,ppy-by,TRUE);
					else
						m_Doc->m_outline_poly[m_sel_id.i].MoveCorner( m_sel_id.ii, prevx, ppy );
					prevx = ppx;
					prevy = ppy;
				}
				else if( fk == FK_ALIGN_MIDDLE_X)
				{
					if( mer >= 0 )
						MoveGroup(prev_middle_x-bx,prevy-by,TRUE);
					else
						m_Doc->m_outline_poly[m_sel_id.i].MoveCorner( m_sel_id.ii, prev_middle_x, prevy );
					prevx = ppx;
				}
				else if( fk == FK_ALIGN_MIDDLE_Y)
				{
					if( mer >= 0 )
						MoveGroup(prevx-bx,prev_middle_y-by,TRUE);
					else
						m_Doc->m_outline_poly[m_sel_id.i].MoveCorner( m_sel_id.ii, prevx, prev_middle_y );
					prevy = ppy;
				}
				else 
				{
					if( mer >= 0 )
						MoveGroup(prev_middle_x-bx,prev_middle_y-by,TRUE);
					else
						m_Doc->m_outline_poly[m_sel_id.i].MoveCorner( m_sel_id.ii, prev_middle_x, prev_middle_y );
					prevx = ppx;
					prevy = ppy;
				}
				m_Doc->m_dlist->CancelHighLight();
				if( mer == -1 )
					m_Doc->m_outline_poly[m_sel_id.i].HighlightCorner( m_sel_id.ii);
				else
				{
					SetCursorMode(CUR_GROUP_SELECTED);
					HighlightGroup();
					m_Doc->m_nlist->SetAreaConnections();
				}
				m_Doc->ProjectModified( TRUE );
				m_page = 1;
				SetFKText( m_cursor_mode );
			}
		}
		else if( fk == FK_ALIGN_MIDLINE )
		{
			int ip = m_Doc->m_outline_poly[m_sel_id.i].GetIndexCornerBack(m_sel_id.ii);
			int in = m_Doc->m_outline_poly[m_sel_id.i].GetIndexCornerNext(m_sel_id.ii);
			int x1 = m_Doc->m_outline_poly[m_sel_id.i].GetX(ip);
			int y1 = m_Doc->m_outline_poly[m_sel_id.i].GetY(ip);
			int x2 = m_Doc->m_outline_poly[m_sel_id.i].GetX(in);
			int y2 = m_Doc->m_outline_poly[m_sel_id.i].GetY(in);
			m_Doc->m_outline_poly[m_sel_id.i].MoveCorner( m_sel_id.ii, (x1+x2)/2, (y1+y2)/2 );
			prevx = ppx;
			prevy = ppy;
			m_Doc->m_dlist->CancelHighLight();
			m_Doc->m_outline_poly[m_sel_id.i].HighlightCorner( m_sel_id.ii);
			m_page = 1;
			SetFKText( m_cursor_mode );
		}
		else if( fk == FK_DELETE_CORNER || nChar == VK_DELETE )
			OnOPCornerDelete();
		else if( fk == FK_DELETE_OUTLINE )
			OnOPDeleteOutline();
		break;

	case CUR_OP_SIDE_SELECTED:
		if( fk == FK_POLY_STRAIGHT )
			OnOPSideConvertToStraightLine();
		else if( fk == FK_POLY_ARC_CW )
			OnOPSideConvertToArcCw();
		else if( fk == FK_POLY_ARC_CCW )
			OnOPSideConvertToArcCcw();
		else if( fk == FK_ADD_CORNER )
			OnOPSideAddCorner();
		else if( fk == FK_AREA_CUTOUT )
			AddOutlinePoly(0);
		else if( fk == FK_SELECT_CONTOUR )
			SelectContour();
		else if( fk == FK_HATCH_STYLE )
			OPHatchStyle();
		else if( fk == FK_DELETE_OUTLINE || nChar == VK_DELETE )
			OnOPDeleteOutline();
		else if( fk == FK_DELETE_CUTOUT )
		{
			SaveUndoInfoForOutlinePoly( UNDO_OP, TRUE, m_Doc->m_undo_list );
			int nc = m_Doc->m_outline_poly[m_sel_id.i].GetNumContour(m_sel_id.ii);
			m_Doc->m_outline_poly[m_sel_id.i].RemoveContour(nc);
			CancelSelection();
			m_Doc->ProjectModified( TRUE );
		}
		else if( fk == FK_SET_WIDTH )
		{
			m_dlist->CancelHighLight();
			OPSetWidth();
			HighlightGroup();
		}
		else if( fk == FK_INSIDE_POLYLINE )
			OnInsidePolyline();
		break;

	case CUR_AREA_CORNER_SELECTED:
		if( fk == FK_ARROW )
		{
			if( !gLastKeyWasArrow )
			{
				SaveUndoInfoForAllAreasInNet( m_sel_net, TRUE, m_Doc->m_undo_list );
				gTotalArrowMoveX = 0;
				gTotalArrowMoveY = 0;
				gLastKeyWasArrow = TRUE;
			}
			CPolyLine * poly = m_sel_net->area[m_sel_ic].poly;
			CPoint p;
			p.x = poly->GetX(m_sel_is)+dx;
			p.y = poly->GetY(m_sel_is)+dy;
			m_Doc->m_nlist->MoveAreaCorner( m_sel_net, m_sel_ic, m_sel_is, p.x, p.y );
			gTotalArrowMoveX += dx;
			gTotalArrowMoveY += dy;
			ShowRelativeDistance( p.x, p.y, gTotalArrowMoveX, gTotalArrowMoveY );
			TryToReselectAreaCorner( p.x, p.y );
			m_Doc->ProjectModified( TRUE );
		}
		else if( fk == FK_EDIT_AREA )
			OnAreaEdit();
		else if( fk == FK_SET_POSITION )
			OnAreaCornerProperties();
		else if( fk == FK_ALIGN_SIDES )
		{
			AlignSides(m_sel_id.type, m_sel_id.i, m_sel_id.ii);
			m_Doc->ProjectModified( TRUE );
		}
		else if( fk == FK_MOVE_CORNER )
			OnAreaCornerMove();
		else if( fk == FK_ALIGN_X || fk == FK_ALIGN_Y )
		{
			if ((ppx||ppy)&&(prevx||prevy))
			{	
				int merge = m_sel_net->area[m_sel_ia].poly->GetMerge();
				if( m_prev_sel_merge == merge )
					merge = -1;
				if( merge >= 0 )
				{
					CString ps;
					ps.Format("This trace is connected to other objects through the \"MERGE\" property. Move all objects of the group %s?", m_Doc->m_mlist->GetMerge( merge ) );
					if( AfxMessageBox( ps, MB_YESNO ) == IDNO )
					{
						merge = -1;
					}
				}
				if( merge >= 0 )
				{
					int vx = ppx;
					int vy = ppy;
					if( fk == FK_ALIGN_X)
						vx = m_sel_net->area[m_sel_ia].poly->GetX(m_sel_is);
					else
						vy = m_sel_net->area[m_sel_ia].poly->GetY(m_sel_is);
					CancelSelection();
					NewSelectM( NULL, merge );
					MoveGroup( ppx-vx, ppy-vy, FALSE );

				}
				else if( m_sel_count == 1 )
				{
					SaveUndoInfoForAllAreasInNet( m_sel_net, TRUE, m_Doc->m_undo_list );
					if( fk == FK_ALIGN_X)
						m_Doc->m_nlist->MoveAreaCorner( m_sel_net, m_sel_id.i, m_sel_id.ii, ppx, prevy );
					else
						m_Doc->m_nlist->MoveAreaCorner( m_sel_net, m_sel_id.i, m_sel_id.ii, prevx, ppy );
				}
				if( fk == FK_ALIGN_X)
					prevx = ppx;
				else
					prevy = ppy;
				m_dlist->CancelHighLight();
				HighlightGroup();
				m_Doc->ProjectModified( TRUE );
			}
		}
		else if( fk == FK_DELETE_CORNER )
			OnAreaCornerDelete();
		else if( fk == FK_DELETE_AREA )
			OnAreaCornerDeleteArea();
		else if( fk == FK_START_CUTOUT )
		{
			int x = m_sel_net->area[m_sel_ia].poly->GetX(m_sel_is);
			int y = m_sel_net->area[m_sel_ia].poly->GetY(m_sel_is);
			AreaAddCutout();
			m_Doc->ProjectModified( TRUE );
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			m_Doc->m_nlist->AppendAreaCorner( m_sel_net, m_sel_ia, x, y, m_polyline_style );
			m_dlist->StartDraggingArc( pDC, m_polyline_style, x, y, x, y, LAY_SELECTION, 1, 2 );
			m_sel_id.ii = 2;
			SetCursorMode( CUR_DRAG_AREA_CUTOUT_1 );
			m_Doc->ProjectModified( TRUE );
			m_last_cursor_point.x = x;
			m_last_cursor_point.y = y;
			m_snap_angle_ref = m_last_cursor_point;
		}
		else if( fk == FK_START_SIMILAR )
		{
			int x = m_sel_net->area[m_sel_ia].poly->GetX(m_sel_is);
			int y = m_sel_net->area[m_sel_ia].poly->GetY(m_sel_is);
			AddSimilarArea();
			int a_w = m_sel_net->area[m_sel_ia].poly->GetW();
			m_sel_ia = m_Doc->m_nlist->AddArea( m_sel_net, 
												m_sel_net->area[m_sel_ia].poly->GetLayer(),
												x,y,
												m_sel_net->area[m_sel_ia].poly->GetHatch() );
			m_sel_net->area[m_sel_ia].poly->SetW( a_w );
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			m_Doc->m_nlist->AppendAreaCorner( m_sel_net, m_sel_ia, x, y, m_polyline_style );
			m_dlist->StartDraggingArc( pDC, m_polyline_style, x, y, x, y, LAY_SELECTION, 1, 2 );
			m_sel_id.ii = 2;
			SetCursorMode( CUR_DRAG_AREA );
			m_Doc->ProjectModified( TRUE );
			m_snap_angle_ref = m_last_cursor_point;
		}
		else if( fk == FK_DELETE_CUTOUT )
			AreaDeleteCutout();
		else if( nChar == VK_DELETE )
			OnAreaCornerDelete();
		break;

	case CUR_AREA_SIDE_SELECTED:
		if( fk == FK_SIDE_STYLE )
			OnAreaSideStyle();
		else if( fk == FK_EDIT_AREA )
			OnAreaEdit();
		else if( fk == FK_POLY_ARC_CCW )
		{
			SaveUndoInfoForArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_MODIFY, TRUE, m_Doc->m_undo_list );
			m_polyline_style = CPolyLine::ARC_CCW;
			m_Doc->m_nlist->SetAreaSideStyle( m_sel_net, m_sel_ia, m_sel_is, m_polyline_style );
			m_Doc->m_nlist->SetAreaConnections( m_sel_net, m_sel_ia );
			m_Doc->m_nlist->OptimizeConnections(  m_sel_net, -1, m_Doc->m_auto_ratline_disable,
														m_Doc->m_auto_ratline_min_pins, TRUE  );
			SetFKText( m_cursor_mode );
			m_Doc->ProjectModified( TRUE );
		}
		else if( fk == FK_ADD_CORNER )
			OnAreaSideAddCorner();
		else if( fk == FK_SELECT_CONTOUR )
			SelectContour();
		else if ( fk == FK_TEST_ON_CONTACT )
		{
			m_Doc->m_nlist->SetAreaConnections(m_sel_net, m_sel_ia, TRUE);
		}
		else if( fk == FK_DELETE_AREA )
			AreaSideDeleteArea();
		else if( fk == FK_AREA_CUTOUT )
			AreaAddCutout();
		else if( fk == FK_DELETE_CUTOUT )
			AreaDeleteCutout();
		else if( nChar == VK_DELETE )
		{
			CPolyLine * poly = m_sel_net->area[m_sel_ia].poly;
			if( poly->GetContour( m_sel_id.ii ) > 0 )
				AreaDeleteCutout();
			else
				AreaSideDeleteArea();
		}
		break;

	case CUR_DRE_SELECTED:
		if( nChar == VK_DELETE )
		{	
			m_Doc->m_drelist->Remove( m_sel_dre );
			CancelSelection();
		}
		break;

	case CUR_GROUP_SELECTED:
		if( fk == FK_ARROW )
		{
			m_dlist->CancelHighLight();
			if( !gLastKeyWasArrow )
			{
				gTotalArrowMoveX = 0;
				gTotalArrowMoveY = 0;
				gLastKeyWasArrow = TRUE;
			}
			MoveGroup( dx, dy, FALSE );
			gTotalArrowMoveX += dx;
			gTotalArrowMoveY += dy;
			HighlightGroup();
			ShowRelativeDistance( gTotalArrowMoveX, gTotalArrowMoveY );
			m_Doc->ProjectModified( TRUE );
		}
		else if( fk == FK_EDIT_PART )
		{
			m_Doc->PartProperties();
		}
		else if( fk == FK_EDIT_SILK )
		{
			m_page = 2;
			SetFKText( m_cursor_mode );
		}
		else if( fk == FK_SET_LINES_VIS || fk == FK_SET_LINES_INVIS )
		{
			if( prev_sel_count != m_sel_count )
				SaveUndoInfoForGroup( UNDO_GROUP_MODIFY, m_Doc->m_undo_list );
			for( cpart * p=m_Doc->m_plist->GetFirstPart(); p; p=m_Doc->m_plist->GetNextPart(p) )
			{
				if( p->selected )
					m_Doc->m_plist->SetLinesVis(p,fk==FK_SET_LINES_VIS?1:0);
			}
			for( cpart * p=m_Doc->m_plist->GetFirstPart(); p; p=m_Doc->m_plist->GetNextPart(p) )
			{
				if( p->shape )
					m_Doc->m_plist->DrawPart(p);
			}		
			m_Doc->ProjectModified( TRUE );
		}
		else if( fk == FK_UNROUTE )
			OnSegmentUnroute();
		else if( fk == FK_REFS_SIZE )
			RefProperties();
		else if( fk == FK_VALUES_SIZE )
			OnValueProperties();
		else if( fk == FK_VALUES_VIS || fk == FK_VALUES_INVIS )
		{
			for( cpart * p=m_Doc->m_plist->GetFirstPart(); p; p=m_Doc->m_plist->GetNextPart(p) )
			{
				if( p->selected )
				{
					if( fk == FK_VALUES_VIS )
						p->m_value_vis = 1;
					else
						p->m_value_vis = 0;
					m_Doc->m_plist->DrawPart(p);
				}
			}
			HighlightGroup();
			if( fk == FK_VALUES_VIS )
				m_draw_layer = mod_active_layer; // FK_VALUES_VIS
		}
		else if( fk == FK_SELECT_CONTOUR )
			SelectContour();
		else if( fk == FK_MOVE_GROUP )
			OnGroupMove();
		else if ( fk == FK_SIDE )
			TurnGroup();
		else if ( fk == FK_TAKE_APART )
			ExplodeGroup();
		else if ( fk == FK_MERGE_GROUP )
			MergeGroup();
		else if( fk == FK_DELETE_GROUP || nChar == VK_DELETE )
			DeleteGroup(0);
		else if ( fk == FK_DEL_WITH_MERGE )
			DeleteGroup(1);
		else if(fk == FK_ROTATE_GROUP)
		{
			m_page = 3;
			SetFKText( m_cursor_mode );
		}
		else if(fk == FK_ROTATE_GROUP_1)
			RotateGroup(1);
		else if(fk == FK_ROTATE_GROUP_5)
			RotateGroup(5);
		else if(fk == FK_ROTATE_GROUP_45)
			RotateGroup(45);
		else if(fk == FK_ROTATE_GROUP_90)
			RotateGroup(90);
		else if(fk == FK_ROTATE_GROUP__1)
			RotateGroup(-1);
		else if(fk == FK_ROTATE_GROUP__5)
			RotateGroup(-5);
		else if(fk == FK_ROTATE_GROUP__45)
			RotateGroup(-45);
		else if(fk == FK_ROTATE_GROUP__90)
			RotateGroup(-90);
		else if(fk == FK_SET_CLEARANCE)
			OnSetClearance();
		else if( fk == FK_COPY_TRACES)
		{
			fCopyTraces = TRUE;
			OnGroupMove();
		}
		else if (fk == FK_RADIUS_DOWN)
		{
			if( prev_sel_count != m_sel_count )
				SaveUndoInfoForGroup( UNDO_GROUP_MODIFY, m_Doc->m_undo_list );
			TracesRadiusUpDown(FALSE);
			m_Doc->ProjectModified( TRUE );
		}
		else if (fk == FK_RADIUS_UP)
		{
			if( prev_sel_count != m_sel_count )	
				SaveUndoInfoForGroup( UNDO_GROUP_MODIFY, m_Doc->m_undo_list );
			TracesRadiusUpDown(TRUE);
			m_Doc->ProjectModified( TRUE );
		}
		else if( fk == FK_INSERT_SEGMENT || fk == FK_ALIGN_SEGMENTS )
		{
			SaveUndoInfoForGroup( UNDO_GROUP_MODIFY, m_Doc->m_undo_list );
			if( getbit( m_sel_flags, FLAG_SEL_NET ) )
				for( cnet * m_net=m_Doc->m_nlist->GetFirstNet(); m_net; m_net=m_Doc->m_nlist->GetNextNet(/*LABEL*/) )
				{
					if( m_net->selected )
					{
						for( int con=0; con<m_net->nconnects; con++ )
						{
							if( m_net->connect[con].m_selected )
							{
								for( int seg=m_net->connect[con].nsegs-1; seg>=0; seg-- )
								{
									if( fk == FK_ALIGN_SEGMENTS )
									{
										if( m_net->connect[con].vtx[seg+1].selected )
											if( m_sel_count > 1 )
												AlignSegments(m_net, con, seg+1, FALSE);
											else
												AlignSegments(m_net, con, seg+1, TRUE);
									}
									else if( m_net->connect[con].seg[seg].selected && seg > 0 ) 
									{
										id Sid( ID_NET, ID_CONNECT, con, ID_SEG, seg );
										NewSelect( m_net, &Sid, 0, 1 );
										BOOL INS = FALSE;
										if ( seg+1 < m_net->connect[con].nsegs )
										{
											if ( InsertSegment( m_net, con, seg+1, FALSE) )
											{
												Sid.ii = seg+1;
												NewSelect( m_net, &Sid, 0, 0 );
												INS = TRUE;
											}
										}
										if ( InsertSegment( m_net, con, seg, INS) )
										{
											Sid.ii = seg;
											NewSelect( m_net, &Sid, 0, 0 );
										}
									}
								}
							}
						}
					}
				}
			m_Doc->m_dlist->CancelHighLight();
			HighlightGroup();
			m_Doc->ProjectModified( TRUE );
			SetFKText ( m_cursor_mode );
		}
		else if( fk == FK_SET_WIDTH )
		{
			if( m_sel_flags == PART_ONLY )
			{
				CString str,wid;
				::MakeCStringFromDimension( &wid, m_Doc->m_min_silkscreen_stroke_wid, m_Doc->m_units, TRUE, FALSE, FALSE, 3 );
				str.Format( "Set the width of outlines in footprints %s ?", wid );
				int ret = AfxMessageBox( str, MB_YESNO );
				if( ret == IDYES )
				{
					POSITION pos;
					CString key;
					void * ptr;
					for( pos = m_Doc->m_footprint_cache_map.GetStartPosition(); pos != NULL; )
					{
						m_Doc->m_footprint_cache_map.GetNextAssoc( pos, key, ptr );
						CShape * sh = (CShape*)ptr;
						if(sh)
							for( int io=sh->m_outline_poly.GetSize()-1; io>=0; io-- )
								sh->m_outline_poly[io].SetW(m_Doc->m_min_silkscreen_stroke_wid);
					}
					for( cpart * p=m_Doc->m_plist->GetFirstPart(); p; p=m_Doc->m_plist->GetNextPart(p) )
					{
						if( p->shape )
							m_Doc->m_plist->DrawPart(p);
					}
				}
			}
			else
				SetWidth( 0 );
			m_Doc->m_dlist->CancelHighLight();
			HighlightGroup();
		}
		else if(fk == FK_SEL_BETWEEN)
		{
			SelectBetween();
			HighlightGroup();
			SetFKText( m_cursor_mode );
		}
		else if ( fk == FK_REF_AUTO_LOC )
		{
			SetUpReferences();	
			m_Doc->ProjectModified( TRUE );
		}
		else if ( fk == FK_PARTS_AUTO_LOC )
		{
			SaveUndoInfoForGroup( UNDO_GROUP_MODIFY, m_Doc->m_undo_list );
			SetUpParts();	
			m_Doc->ProjectModified( TRUE );
		}
		else if( fk == FK_CLEARANCE_UP )
		{
			if( m_Doc->m_routing_grid_spacing > 999 )
				m_seg_clearance += m_Doc->m_routing_grid_spacing;
			else
				m_seg_clearance += m_Doc->m_part_grid_spacing;
			OnSetClearance();
		}
		else if( fk == FK_CLEARANCE_DOWN )
		{
			if( m_Doc->m_routing_grid_spacing > 999 )
				m_seg_clearance -= m_Doc->m_routing_grid_spacing;
			else
				m_seg_clearance -= m_Doc->m_part_grid_spacing;
			OnSetClearance();
		}
		else if( fk == FK_OK )
		{
			if ( m_sel_count )
			{
				m_page = 1;
				SetFKText(m_cursor_mode);
			}
			else
				SetCursorMode(CUR_NONE_SELECTED);
			m_Doc->ProjectModified( TRUE );
		}
		break;
	case CUR_ADD_AREA_CUTOUT:
		if( fk == FK_REPOUR )
		{
			fRepour = 1;
			m_page = 2;
			SetFKText(m_cursor_mode);
		}
		break;
	case CUR_DRAG_RAT:
		if( fk == FK_COMPLETE )
		{
			OnRatlineComplete();
		}
		else if (fk == FK_BACK_WIDTH || fk == FK_NEXT_WIDTH || fk == FK_AS_PAD )	
		{
			int n_w=INT_MAX, b_w=0;
			for( int sz=0; sz<m_Doc->m_w.GetSize(); sz++)
			{
				int gw = abs(m_Doc->m_w[sz]);
				if ( gw > m_routing_width && gw < n_w )
					n_w = gw;
				if ( gw < m_routing_width && gw > b_w )
					b_w = gw;
			}
			if (fk == FK_BACK_WIDTH && b_w)
				m_routing_width = b_w;
			if (fk == FK_NEXT_WIDTH && n_w < INT_MAX )
				m_routing_width = n_w;
			//
			int w=m_sel_net->def_w;
			int via_w = m_Doc->m_via_w;
			int via_hole_w = m_Doc->m_via_hole_w;
			if (m_sel_is)
				w=m_sel_last_seg.width;
			GetWidthsForSegment( &w, &via_w, &via_hole_w );
			w = m_routing_width;
			if ( fk == FK_AS_PAD )
			{
				cpart * sel_part = NULL;
				int i = -1;
				if( m_dir == 0 || m_sel_con.end_pin == cconnect::NO_END )
				{
					sel_part = m_sel_start_pin.part;
					i = m_sel_con.start_pin_shape;
				}
				else
				{
					sel_part = m_sel_end_pin.part;
					i = m_sel_con.end_pin_shape;
				}
				if( sel_part && i >= 0 )
				{
					if (sel_part->pin[i].dl_sel)
					{
						dl_element * el = sel_part->pin[i].dl_sel;
						CPoint P[4];
						int np = 4;
						m_Doc->m_dlist->Get_Points( el, P, &np ); 
						if( np == 4 )
						{
							int _l = Distance( P[0].x, P[0].y, P[1].x, P[1].y );
							int _h = Distance( P[1].x, P[1].y, P[2].x, P[2].y );
							w = min( _l,_h );
						}
						else
						{
							RECT Get;
							m_Doc->m_dlist->Get_Rect( el, &Get );
							int _l = Get.right - Get.left;
							int _h = Get.top - Get.bottom;
							w = min( _l,_h );
						}
					}
				}
				if( m_routing_width != w-m_Doc->m_mask_clearance*2 )
					w -= m_Doc->m_mask_clearance*2;
				m_routing_width = w;
			}
			m_dlist->StopDragging();
			if( m_sel_id.ii > 0 ) 
				m_Doc->m_nlist->CancelDraggingSegment( m_sel_net, m_sel_ic, m_sel_is );
			int last_seg_layer = 0;
			if( m_dir == 0 && m_sel_id.ii > 0 )
				last_seg_layer = m_sel_con.seg[m_sel_id.ii-1].layer;
			else if( m_dir == 1 && m_sel_id.ii < (m_sel_con.nsegs-1) )
				last_seg_layer = m_sel_con.seg[m_sel_id.ii+1].layer;
			m_Doc->m_nlist->StartDraggingSegment(	pDC, m_sel_net, m_sel_ic, m_sel_is,
													m_last_cursor_point.x, m_last_cursor_point.y, m_active_layer,
													LAY_SELECTION, w,
													last_seg_layer, via_w, via_hole_w, m_dir, 2 );
			//
		}
		break;

	case CUR_DRAG_STUB:
		if (fk == FK_BRANCH_TO_VIA)	
		{
			m_dlist->CancelHighLight();
			m_Doc->m_nlist->HighlightNetVertices( m_sel_net, FALSE );
			en_branch = 1;
			SetFKText(m_cursor_mode);
		}
		if (fk == FK_BRANCH_TO_SEG)	
		{
			m_dlist->CancelHighLight();
			m_Doc->m_nlist->HighlightNetConnections( m_sel_net );
			en_branch = 2;
			SetFKText(m_cursor_mode);
		}
		else if (fk == FK_DISABLE_BRANCH)	
		{
			m_dlist->CancelHighLight();
			m_Doc->m_nlist->HighlightNet( m_sel_net, TRANSPARENT_HILITE );
			m_Doc->m_plist->HighlightAllPadsOnNet( m_sel_net, 1, m_active_layer );
			en_branch = 0;
			SetFKText(m_cursor_mode);
		}
		else if ( fk == FK_BACK_WIDTH || fk == FK_NEXT_WIDTH || fk == FK_AS_PAD )	
		{
			if ( fk == FK_AS_PAD )
			{
				cpart * p = m_sel_start_pin.part;
				CString np = m_sel_start_pin.pin_name;
				int ip = m_sel_con.start_pin_shape;
				// get size pin
				int w = 0;
				if (p && ip >= 0)
				{
					if( p->pin[ip].dl_sel )
					{
						dl_element * el = p->pin[ip].dl_sel;
						CPoint P[4];
						int np = 4;
						m_Doc->m_dlist->Get_Points( el, P, &np ); 
						if( np == 4 )
						{
							int _l = Distance(  P[0].x, P[0].y, P[1].x, P[1].y );
							int _h = Distance(  P[1].x, P[1].y, P[2].x, P[2].y );
							w = min( _l,_h );
							//w -= w%NM_PER_MIL;
						}
						else
						{
							RECT Get;
							m_Doc->m_dlist->Get_Rect( el, &Get );
							int _l = Get.right - Get.left;
							int _h = Get.top - Get.bottom;
							w = min( _l,_h );
							//w -= w%NM_PER_MIL;
						}
					}
				}
				if( m_routing_width != w-m_Doc->m_mask_clearance*2 )
					w -= m_Doc->m_mask_clearance*2;
				if( w<=0 )
					goto goodbye;
				m_routing_width = w;
			}
			else
			{
				int n_w=INT_MAX, b_w=0;
				for( int sz=0; sz<m_Doc->m_w.GetSize(); sz++)
				{
					int gw = abs(m_Doc->m_w[sz]);
					if ( gw > m_routing_width && gw < n_w )
						n_w = gw;
					if ( gw < m_routing_width && gw > b_w )
						b_w = gw;
				}
				if (fk == FK_BACK_WIDTH && b_w)
					m_routing_width = b_w;
				if (fk == FK_NEXT_WIDTH && n_w < INT_MAX )
					m_routing_width = n_w;
			}
			//
			int w=m_sel_net->def_w;
			int via_w = m_Doc->m_via_w;
			int via_hole_w = m_Doc->m_via_hole_w;
			if (m_sel_is)
				w=m_sel_last_seg.width;
			GetWidthsForSegment( &w, &via_w, &via_hole_w );
			w = m_routing_width;
			m_dlist->StopDragging();
			if( m_sel_id.ii > 0 )
			{
				m_Doc->m_nlist->CancelDraggingStub( m_sel_net, m_sel_ic, m_sel_is );
			}
			m_Doc->m_nlist->StartDraggingStub( pDC, m_sel_net, m_sel_ic, m_sel_is,
				m_last_cursor_point.x, m_last_cursor_point.y, 
				mod_active_layer, w, 
				mod_active_layer,
				via_w, via_hole_w, 2, m_inflection_mode );
			m_dlist->CancelHighLight();
			if ( en_branch == BRANCH_TO_VERTEX ) 
			{	
				m_Doc->m_nlist->HighlightNetVertices( m_sel_net, FALSE );
			}
			else if( en_branch == BRANCH_TO_LINE )
			{
				m_Doc->m_nlist->HighlightNetConnections( m_sel_net );
			}
			else 
			{
				m_Doc->m_nlist->HighlightNet( m_sel_net );
				m_Doc->m_plist->HighlightAllPadsOnNet( m_sel_net, 1, m_active_layer );
			}
			SetFKText(m_cursor_mode);
		}
		break;

	case  CUR_DRAG_PART:
		if( fk == FK_ROTATE_PART )
			m_dlist->IncrementDragAngle( pDC );
		if( fk == FK_ROTATE_PART_CCW )
		{
			m_dlist->IncrementDragAngle( pDC );
			m_dlist->IncrementDragAngle( pDC );
			m_dlist->IncrementDragAngle( pDC );
		}
		else if( fk == FK_SIDE )
			m_dlist->FlipDragSide( pDC );
		else if( fk == FK_LOCK_CONNECT || fk == FK_UNLOCK_CONNECT )
		{
			for( cnet * n = m_Doc->m_nlist->GetFirstNet(); n; n = m_Doc->m_nlist->GetNextNet(/*LABEL*/) )
				for( int icon=0; icon<n->nconnects; icon++ )
				{
					if( n->pin[n->connect[icon].start_pin].part == m_sel_part )
					{
						if( fk == FK_LOCK_CONNECT )
							n->connect[icon].locked = TRUE;
						else
							n->connect[icon].locked = FALSE;
					}
					if( n->connect[icon].end_pin >= 0 )
						if( n->pin[n->connect[icon].end_pin].part == m_sel_part )
						{
							if( fk == FK_LOCK_CONNECT )
								n->connect[icon].locked = TRUE;
							else
								n->connect[icon].locked = FALSE;
						}
				}
		}
		else if( fk == FK_REDO_RATLINES || fk == FK_UNROUTE_OF_VIAS || fk == FK_UNROUTE_TRACE || fk == FK_DELETE_CONNECT )
		{
			SaveUndoInfoForPartAndNets( m_sel_part, CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
			m_Doc->m_plist->StopDragging();
			int old_angle = m_Doc->m_plist->GetAngle( m_sel_part );
			int angle = old_angle + m_dlist->GetDragAngle();
			angle = angle % 360;
			int old_side = m_sel_part->side;
			int side = old_side + m_dlist->GetDragSide();
			if( side > 1 )
				side = side - 2;
			m_Doc->m_plist->Move(	m_sel_part, m_last_cursor_point.x, m_last_cursor_point.y,
									angle, side );
			m_Doc->m_nlist->PartMoved( m_sel_part , FALSE );
			m_Doc->m_nlist->OptimizeConnections( m_sel_part, m_Doc->m_auto_ratline_disable, 
											 m_Doc->m_auto_ratline_min_pins, FALSE );
			CDC *pDC = GetDC();
			pDC->SelectClipRgn( &m_pcb_rgn );
			SetDCToWorldCoords( pDC );
			if( fk == FK_UNROUTE_OF_VIAS )
			{
				for( cnet * m_net=m_Doc->m_nlist->GetFirstNet(); m_net; m_net=m_Doc->m_nlist->GetNextNet(/*LABEL*/) )
				{
					for( int ic=0; ic<m_net->nconnects; ic++ )
					{
						BOOL bMer = 0;
						if( m_net->pin[m_net->connect[ic].start_pin].part == m_sel_part )
						{		
							for( int is=0; is<m_net->connect[ic].nsegs; is++ )
							{
								if( !m_net->connect[ic].vtx[is].tee_ID && !m_net->connect[ic].vtx[is].via_w )
								{
									m_net->connect[ic].seg[is].layer = LAY_RAT_LINE;
									m_net->connect[ic].seg[is].width = 0;
									bMer = 1;
								}
								else
									break;
							}
						}
						if( m_net->connect[ic].end_pin >= 0 )
							if( m_net->pin[m_net->connect[ic].end_pin].part == m_sel_part )
							{
								for( int is=m_net->connect[ic].nsegs-1; is>=0; is-- )
								{
									if( !m_net->connect[ic].vtx[is+1].tee_ID && !m_net->connect[ic].vtx[is+1].via_w )
									{
										m_net->connect[ic].seg[is].layer = LAY_RAT_LINE;
										m_net->connect[ic].seg[is].width = 0;
										bMer = 1;
									}
									else
										break;
								}
							}
						if( bMer )
							m_Doc->m_nlist->MergeUnroutedSegments( m_net, ic );
					}
				}
			}
			else if( fk == FK_UNROUTE_TRACE )
			{
				for( cnet * m_net=m_Doc->m_nlist->GetFirstNet(); m_net; m_net=m_Doc->m_nlist->GetNextNet(/*LABEL*/) )
				{
					for( int ic=0; ic<m_net->nconnects; ic++ )
					{
						if( m_net->pin[m_net->connect[ic].start_pin].part == m_sel_part )
						{
							m_Doc->m_nlist->UnrouteNetConnect( m_net, ic );
						}
						if( m_net->connect[ic].end_pin >= 0 )
							if( m_net->pin[m_net->connect[ic].end_pin].part == m_sel_part )
							{
								m_Doc->m_nlist->UnrouteNetConnect( m_net, ic );
							}
					}
				}
			}
			else if( fk == FK_DELETE_CONNECT )
			{
				for( cnet * m_net=m_Doc->m_nlist->GetFirstNet(); m_net; m_net=m_Doc->m_nlist->GetNextNet(/*LABEL*/) )
				{
					for( int ic=m_net->nconnects-1; ic>=0; ic-- )
					{
						if( m_net->pin[m_net->connect[ic].start_pin].part == m_sel_part )
						{
							m_Doc->m_nlist->RemoveNetConnect( m_net, ic );
						}
						else if( m_net->connect[ic].end_pin >= 0 )
						{
							if( m_net->pin[m_net->connect[ic].end_pin].part == m_sel_part )
							{
								m_Doc->m_nlist->RemoveNetConnect( m_net, ic );
							}
						}
					}
				}
			}
			m_Doc->m_plist->StartDraggingPart(	pDC, m_sel_part, m_Doc->m_vis[LAY_RAT_LINE], 
												m_Doc->m_auto_ratline_disable, m_Doc->m_auto_ratline_min_pins,0 );
		}
		break;

	case  CUR_DRAG_REF:
		if( fk == FK_ROTATE_REF )
			m_dlist->IncrementDragAngle( pDC );
		break;

	case  CUR_DRAG_VALUE:
		if( fk == FK_ROTATE_VALUE )
			m_dlist->IncrementDragAngle( pDC );
		break;

	case  CUR_DRAG_TEXT:
		if( fk == FK_ROTATE_TEXT )
			m_dlist->IncrementDragAngle( pDC );
		break;

	case  CUR_DRAG_OP:
	case  CUR_DRAG_OP_1:
		if( fk == FK_POLY_STRAIGHT )
		{
			m_polyline_style = CPolyLine::STRAIGHT;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		else if( fk == FK_POLY_ARC_CW )
		{
			m_polyline_style = CPolyLine::ARC_CW;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		else if( fk == FK_POLY_ARC_CCW )
		{
			m_polyline_style = CPolyLine::ARC_CCW;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		break;

	case  CUR_DRAG_AREA:
	case  CUR_DRAG_AREA_1:
	case  CUR_DRAG_AREA_CUTOUT:
	case  CUR_DRAG_AREA_CUTOUT_1:
		if( fk == FK_POLY_STRAIGHT )
		{
			m_polyline_style = CPolyLine::STRAIGHT;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		else if( fk == FK_POLY_ARC_CW )
		{
			m_polyline_style = CPolyLine::ARC_CW;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		else if( fk == FK_POLY_ARC_CCW )
		{
			m_polyline_style = CPolyLine::ARC_CCW;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		break;
	}	// end switch

	if( nChar == ' ' )
	{
		// space bar pressed, center window on cursor then center cursor
		m_org_x = p.x - ((m_client_r.right-m_left_pane_w)*m_pcbu_per_pixel)/2;
		m_org_y = p.y - ((m_client_r.bottom-m_bottom_pane_h)*m_pcbu_per_pixel)/2;
		CRect screen_r;
		GetWindowRect( &screen_r );
		m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel,
			m_org_x, m_org_y );
		p = m_dlist->PCBToScreen( p );
		//p.x += (mpt.x-p.x)/2;
		//p.y += (mpt.y-p.y)/2;
		//if( abs(mpt.x-p.x) > 10 || abs(mpt.y-p.y) > 10 )
		SetCursorPos( p.x, p.y-3 );
		//else
		//	SetCursorPos( cp.x, cp.y );
	}
	else if( nChar == VK_HOME )
	{
		// home key pressed, ViewAllElements
		OnViewAllElements();
		ShowSelectStatus();
		return;
	}
	else if( nChar == 33 )
	{
		// PgUp pressed, zoom in
		if( m_pcbu_per_pixel > 254 )
		{
			m_pcbu_per_pixel = m_pcbu_per_pixel/ZOOM_RATIO;
			m_org_x = p.x - ((m_client_r.right-m_left_pane_w)*m_pcbu_per_pixel)/2;
			m_org_y = p.y - ((m_client_r.bottom-m_bottom_pane_h)*m_pcbu_per_pixel)/2;
			CRect screen_r;
			GetWindowRect( &screen_r );
			m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel,
				m_org_x, m_org_y );
			p = m_dlist->PCBToScreen( p );
			SetCursorPos( p.x, p.y );
		}
	}
	else if( nChar == 34 )
	{
		// PgDn pressed, zoom out
		// first, make sure that window boundaries will be OK
		int org_x = p.x - ((m_client_r.right-m_left_pane_w)*m_pcbu_per_pixel*ZOOM_RATIO)/2;
		int org_y = p.y - ((m_client_r.bottom-m_bottom_pane_h)*m_pcbu_per_pixel*ZOOM_RATIO)/2;
		int max_x = org_x + (m_client_r.right-m_left_pane_w)*m_pcbu_per_pixel*ZOOM_RATIO;
		int max_y = org_y + (m_client_r.bottom-m_bottom_pane_h)*m_pcbu_per_pixel*ZOOM_RATIO;
		if( org_x > -PCB_BOUND && org_x < PCB_BOUND && max_x > -PCB_BOUND && max_x < PCB_BOUND
			&& org_y > -PCB_BOUND && org_y < PCB_BOUND && max_y > -PCB_BOUND && max_y < PCB_BOUND )
		{
			// OK, do it
			m_org_x = org_x;
			m_org_y = org_y;
			m_pcbu_per_pixel = m_pcbu_per_pixel*ZOOM_RATIO;
			CRect screen_r;
			GetWindowRect( &screen_r );
			m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel,
				m_org_x, m_org_y );
			p = m_dlist->PCBToScreen( p );
			SetCursorPos( p.x, p.y );
		}
	}
goodbye:
	ReleaseDC( pDC );
	if( gLastKeyWasArrow == FALSE )
		ShowSelectStatus();
	Invalidate( FALSE );//handlekeypress
}
//===============================================================================================
// Mouse moved
//
void CFreePcbView::OnMouseMove(UINT nFlags, CPoint point)
{
	static int cicle = 1;
	static BOOL bCursorOn = TRUE;
	static BOOL OnMouseMoveComplete = TRUE;
	if (!OnMouseMoveComplete)
		return;
	OnMouseMoveComplete = FALSE; 
	CMainFrame * pMain = (CMainFrame*) AfxGetApp()->m_pMainWnd;
	if( m_bLButtonDown )
	{
		CString str = "To Right - SELECT  To Left - UNSELECT";
		pMain->DrawStatus( 3, &str );
	}
	else if (m_cursor_mode == CUR_NONE_SELECTED || 
			m_cursor_mode == CUR_DRAG_PART || 
			m_cursor_mode == CUR_DRAG_CONNECT ||
			m_cursor_mode == CUR_DRAG_RAT_PIN )
	{ 
		CPoint p = m_dlist->WindowToPCB( point );
		static id sid(0,0,0,0,0);
		static cpart * prev_p=NULL;
		id pad_id( ID_PART, ID_PAD, 0, 0, 0 );	// test for hit on pad
		id sid_prev = sid;
		int lay = mod_active_layer;
		void * ptr = m_dlist->TestSelect( p.x, p.y, &sid, &lay, NULL, NULL, &pad_id );
		cpart * m_sel_part_pin = (cpart*)ptr;
		if( sid.type != sid_prev.type ||
			sid.st != sid_prev.st ||
			sid.i != sid_prev.i ||
			m_sel_part_pin != prev_p )
		{
			//
			if( sid.st == ID_PAD && sid.type == ID_PART &&
			(m_sel_part_pin != m_sel_part || m_cursor_mode != CUR_DRAG_PART))
			{
				CString x_str, y_str, w_str, hole_str, thrml_str, str;
				CString pName = m_sel_part_pin->shape->GetPinNameByIndex(sid.i);
				cnet * pin_net = (cnet*)m_sel_part_pin->pin[sid.i].net;
				int u = m_Doc->m_units;
				int x = m_sel_part_pin->pin[sid.i].x;
				int y = m_sel_part_pin->pin[sid.i].y;
				int intx=0, inty=0;
				if( m_sel_part_pin->pin[sid.i].dl_sel )
					{
						int pad_type=0, pad_x=0, pad_y=0, pad_w=0, pad_l=0, pad_r=0, pad_hole=0, pad_angle=0, pad_connect=0;
						cnet * pad_net=NULL;
						int check = m_Doc->m_plist->GetPadDrawInfo( m_sel_part_pin, sid.i, m_active_layer, 0, 0, 0, 0,
														&pad_type, &pad_x, &pad_y, &pad_w, &pad_l, &pad_r, 
														&pad_hole, &pad_angle, &pad_net, &pad_connect );
						if( check )
						{
							intx = pad_w;
							inty = pad_l;
						}
					}
				int dHole = m_sel_part_pin->shape->m_padstack[sid.i].hole_size;
				int topType = m_sel_part_pin->shape->m_padstack[sid.i].top.shape;
				int topThrml = m_sel_part_pin->shape->m_padstack[sid.i].top.connect_flag;
				switch(topThrml)
				{
					case 0: thrml_str = "default"; 
						break;
					case 1: thrml_str = "noConnect"; 
						break;
					case 2: thrml_str = "Thermal"; 
						break;
					case 3: thrml_str = "noTherm"; 
						break;
					default: thrml_str = "not found";  
				}
				::MakeCStringFromDimension( &x_str, abs(intx), u, FALSE, FALSE, FALSE, u==MIL?0:3 );
				::MakeCStringFromDimension( &y_str, abs(inty), u, FALSE, FALSE, FALSE, u==MIL?0:3 );
				::MakeCStringFromDimension( &hole_str, dHole, u, FALSE, FALSE, FALSE, u==MIL?0:3 );
				CString pDesc = m_sel_part_pin->shape->GetPinDescriptionByIndex(sid.i);
				if( pDesc.GetLength() )
				{
					pDesc = "( " + pDesc;
					pDesc += " )";
				}
				if( pin_net )
				{
					// pad attached to net
					if (dHole)
						str.Format( "pin %s.%s%s on net \"%s\", size %sx%s, hole %s",
						m_sel_part_pin->ref_des,
						m_sel_part_pin->shape->GetPinNameByIndex(sid.i),
						pDesc,
						pin_net->name, 
						x_str,
						y_str,
						hole_str);
					else if (topType)
						str.Format( "pin %s.%s%s on net \"%s\", size %sx%s, Thrml %s",
						m_sel_part_pin->ref_des,
						m_sel_part_pin->shape->GetPinNameByIndex(sid.i),
						pDesc,
						pin_net->name, 
						x_str,
						y_str,
						thrml_str);
				}
				else
				{
					// pad not attached to a net
					str.Format( "pin %s.%s%s unconnected, size %sx%s, hole %s",
						m_sel_part_pin->ref_des,
						m_sel_part_pin->shape->GetPinNameByIndex(sid.i),
						pDesc,
						x_str,
						y_str,
						hole_str);
				}
				pMain->DrawStatus( 3, &str );
				m_Doc->m_dlist->CancelHighLight();
				int pl = m_Doc->m_plist->GetPinLayer(m_sel_part_pin,sid.i);
				if( pl == LAY_PAD_THRU )
					pl = mod_active_layer;
				if( m_cursor_mode == CUR_NONE_SELECTED )
					m_Doc->m_plist->SelectPad( m_sel_part_pin, sid.i, 1, pl );				
				else
					m_Doc->m_plist->SelectPad( m_sel_part_pin, sid.i, 1, LAY_SELECTION );
				ShowPinState( m_sel_part_pin, sid.i );
				CString net = "Unconnected";
				if( m_sel_part_pin->pin[sid.i].net )
				{
					net = m_sel_part_pin->pin[sid.i].net->name;
					if( m_cursor_mode != CUR_DRAG_PART )
					{
						//m_Doc->m_nlist->HighlightNet( m_sel_part_pin->pin[sid.i].net, TRANSPARENT_LAYER );
						//m_Doc->m_nlist->HighlightNetConnections( m_sel_part_pin->pin[sid.i].net, TRANSPARENT_LAYER );
						m_Doc->m_plist->HighlightAllPadsOnNet( m_sel_part_pin->pin[sid.i].net, 1, m_active_layer, m_active_layer, sid.i, m_sel_part_pin );
					}
				}
				if( m_cursor_mode == CUR_DRAG_CONNECT ||
						m_cursor_mode == CUR_DRAG_RAT_PIN )
				{
					CString pin = m_sel_part_pin->ref_des + "." + m_sel_part_pin->shape->m_padstack[sid.i].name;
					OnInfoBoxSendMess("part_list " + pin + " \"" + net + "\"");
				}
			}
			else 
			{
				if( m_dlist->Get_Selected() )
				{
					m_dlist->CancelHighLight();				
					CMainFrame * pMain = (CMainFrame*) AfxGetApp()->m_pMainWnd;
					CString str = "No selection";
					pMain->DrawStatus( 3, &str );
					if( m_cursor_mode == CUR_DRAG_CONNECT ||
						m_cursor_mode == CUR_DRAG_RAT_PIN )
					{
						//OnInfoBoxSendMess("part_list ");
						if( m_sel_net )
							m_Doc->m_plist->HighlightAllPadsOnNet( m_sel_net, 1, m_active_layer, TRANSPARENT_LAYER );
						else if( m_sel_part )
							if( m_sel_part->pin[m_sel_id.i].net )
								m_Doc->m_plist->HighlightAllPadsOnNet( m_sel_part->pin[m_sel_id.i].net, 1, m_active_layer, TRANSPARENT_LAYER );
						
					}
				}
			}
			prev_p = m_sel_part_pin;
			if( m_draw_layer == ENABLE_CHANGE_DRAW_LAYER )
				if( point.y < (m_client_r.bottom-m_bottom_pane_h) )
					m_draw_layer = LAY_HILITE;//OnMouseMove
			CDC * pDC = GetDC();
			Invalidate(FALSE);//OnMouseMove
			if( m_cursor_mode == CUR_NONE_SELECTED )
				m_sel_id.Set ( ID_PART, 0, 0, 0, 0 );
		}
	}
	//
	if( (nFlags & MK_LBUTTON) && m_bLButtonDown )
	{
		double d = abs(point.x-m_start_pt.x) + abs(point.y-m_start_pt.y);
		if( m_bDraggingRect
			|| (d > 10 && !CurDragging() ) )
		{
			// we are dragging a selection rect
			SIZE s1;
			s1.cx = s1.cy = 1;
			m_drag_rect.TopLeft() = m_start_pt;
			m_drag_rect.BottomRight() = point;
			m_drag_rect.NormalizeRect();
			CDC * pDC = GetDC();
			if( !m_bDraggingRect )
			{
				//start dragging rect
				pDC->DrawDragRect( &m_drag_rect, s1, NULL, s1 );
			}
			else
			{
				// continue dragging rect
				pDC->DrawDragRect( &m_drag_rect, s1, &m_last_drag_rect, s1 );
			}
			m_bDraggingRect  = TRUE;
			m_last_drag_rect = m_drag_rect;
			ReleaseDC( pDC );
		}
	}
	m_last_mouse_point = m_dlist->WindowToPCB( point );
	SnapCursorPoint( m_last_mouse_point, nFlags );
	// check for cursor hiding
	CMainFrame * frm = (CMainFrame*)AfxGetMainWnd();
	if( !CurDragging() )
		frm->m_bHideCursor = FALSE;		// disable cursor hiding
	else if( !frm->m_bHideCursor )
	{
		// enable cursor hiding and set rect
		CRect r = frm->m_client_rect;
		r.left += m_left_pane_w;
		r.bottom -= m_bottom_pane_h;
		frm->SetHideCursor( TRUE, &r );
	}
	OnMouseMoveComplete = TRUE;
}
//===============================================================================================
/////////////////////////////////////////////////////////////////////////
// Utility functions
//
// Set the device context to world coords
//
int CFreePcbView::SetDCToWorldCoords( CDC * pDC )
{
	m_dlist->SetDCToWorldCoords( pDC, &m_botDC, &m_topDC, m_org_x, m_org_y );
	return 0;
}
//===============================================================================================
// Set cursor mode, update function key menu if necessary
//
void CFreePcbView::SetCursorMode( int mode )
{	
	if( mode != m_cursor_mode )
	{	
		SetFKText( mode );
		m_cursor_mode = mode;
		ShowSelectStatus();
		if( CurDragging() )
			SetMainMenu( FALSE );
		else if( m_Doc->m_project_open )
			SetMainMenu( TRUE );
		return;
	}
	else if( mode == CUR_GROUP_SELECTED )
		SetFKText( mode );
}
//===============================================================================================
// Set function key shortcut text
//
void CFreePcbView::SetFKText( int mode )
{
	for( int i=0; i<12; i++ )
	{
		m_fkey_option[i] = 0;
		m_fkey_command[i] = 0;
	}
	switch( mode )
	{
	case CUR_NONE_SELECTED:
		if( m_Doc->m_project_open )
		{
			m_fkey_option[0] = FK_ADD_PART;
			m_fkey_option[1] = FK_ADD_AREA;
			m_fkey_option[2] = FK_ADD_TEXT;
			if( m_polyline_layer )
				m_fkey_option[3] = FK_ADD_LINE;
			//
			m_fkey_option[5] = FK_SHOW_M;
			m_fkey_option[6] = FK_CHECK_TRACES;
			m_fkey_option[7] = FK_GRID_STYLE;
			m_fkey_option[8] = FK_REDO_RATLINES;
		}
		break;

	case CUR_PART_SELECTED:
		m_fkey_option[0] = FK_EDIT_PART;
		m_fkey_option[1] = FK_EDIT_FOOTPRINT;
		if( m_sel_part->glued )
			m_fkey_option[2] = FK_UNGLUE_PART;
		else
			m_fkey_option[2] = FK_GLUE_PART;
		if( !m_sel_part->glued )
		{
			m_fkey_option[3] = FK_MOVE_PART;
			m_fkey_option[5] = FK_ROTATE_GROUP;
		}
		if( m_sel_part->m_merge >= 0 )
			m_fkey_option[7] = FK_TAKE_APART;
		m_fkey_option[8] = FK_REDO_RATLINES;
		break;

	case CUR_REF_SELECTED:
		m_fkey_option[0] = FK_SET_PARAMS;
		m_fkey_option[1] = FK_ROTATE_REF;
		m_fkey_option[2] = FK_ROTATE_REF_CCW;
		m_fkey_option[3] = FK_MOVE_REF;
		break;

	case CUR_VALUE_SELECTED:
		m_fkey_option[0] = FK_SET_PARAMS;
		m_fkey_option[1] = FK_ROTATE_VALUE;
		m_fkey_option[2] = FK_ROTATE_VALUE_CCW;
		m_fkey_option[3] = FK_MOVE_VALUE;
		break;

	case CUR_PAD_SELECTED:
		if ( m_page == 1 )
		{
			prev_middle_x = (ppx + prevx)/2;
			prev_middle_y = (ppy + prevy)/2;
			ppx = prevx;
			ppy = prevy;
			prevx = m_sel_part->pin[m_sel_id.i].x;
			prevy = m_sel_part->pin[m_sel_id.i].y;
			m_fkey_option[0] = FK_EDIT_PART;
			m_fkey_option[1] = FK_ADD_CONNECT;
			m_fkey_option[2] = FK_START_STUB;
			if( !m_sel_part->glued )
				m_fkey_option[3] = FK_MOVE_PART;
			if( m_sel_part->pin[m_sel_id.i].net )
				m_fkey_option[4] = FK_EDIT_NET;
			else
				m_fkey_option[4] = FK_ATTACH_NET;
			if( !m_sel_part->glued )
			{
				m_fkey_option[5] = FK_ROTATE_GROUP;
				m_fkey_option[6] = FK_ALIGN_X;
				m_fkey_option[7] = FK_ALIGN_Y;
				m_fkey_option[8] = FK_ALIGN_MIDDLE;
			}
		}
		else if ( m_page == 2 )
		{
			m_fkey_option[5] = FK_ALIGN_MIDDLE;
			m_fkey_option[6] = FK_ALIGN_MIDDLE_X;
			m_fkey_option[7] = FK_ALIGN_MIDDLE_Y;
		}
		break;

	case CUR_TEXT_SELECTED:
		m_fkey_option[0] = FK_EDIT_TEXT;
		m_fkey_option[1] = FK_ROTATE_REF;
		m_fkey_option[2] = FK_ROTATE_REF_CCW;
		m_fkey_option[3] = FK_MOVE_TEXT;
		m_fkey_option[7] = FK_DELETE_TEXT;
		break;

	case CUR_OP_CORNER_SELECTED:
		if ( m_page == 1 )
		{
			prev_middle_x = (ppx + prevx)/2;
			prev_middle_y = (ppy + prevy)/2;
			ppx = prevx;
			ppy = prevy;
			prevx = m_Doc->m_outline_poly[m_sel_id.i].GetX(m_sel_id.ii);
			prevy = m_Doc->m_outline_poly[m_sel_id.i].GetY(m_sel_id.ii);
			//
			m_fkey_option[0] = FK_SET_POSITION;
			m_fkey_option[2] = FK_ALIGN_SIDES;
			m_fkey_option[3] = FK_MOVE_CORNER;
			m_fkey_option[4] = FK_ALIGN_MIDDLE;
			m_fkey_option[6] = FK_ALIGN_X;
			m_fkey_option[7] = FK_ALIGN_Y;
			m_fkey_option[8] = FK_DELETE_CORNER;
		}
		else if ( m_page == 2 )
		{
			m_fkey_option[4] = FK_ALIGN_MIDLINE;
			m_fkey_option[5] = FK_ALIGN_MIDDLE;
			m_fkey_option[6] = FK_ALIGN_MIDDLE_X;
			m_fkey_option[7] = FK_ALIGN_MIDDLE_Y;
		}
		break;

	case CUR_OP_SIDE_SELECTED:
			m_fkey_option[0] = FK_SET_WIDTH;
		if( m_Doc->m_outline_poly[m_sel_id.i].GetSideStyle(m_sel_id.ii) == CPolyLine::STRAIGHT )
		{
			m_fkey_option[1] = FK_POLY_ARC_CW;
			m_fkey_option[2] = FK_ADD_CORNER;
		}
		else if( m_Doc->m_outline_poly[m_sel_id.i].GetSideStyle(m_sel_id.ii) == CPolyLine::ARC_CW )
			m_fkey_option[1] = FK_POLY_ARC_CCW;
		else
			m_fkey_option[1] = FK_POLY_STRAIGHT;
		m_fkey_option[4] = FK_SELECT_CONTOUR;
		m_fkey_option[5] = FK_HATCH_STYLE;
		if( m_Doc->m_outline_poly[m_sel_id.i].GetClosed() && m_Doc->m_outline_poly[m_sel_id.i].GetNumCorners() > 2 )
			m_fkey_option[6] = FK_AREA_CUTOUT;
		m_fkey_option[7] = FK_DELETE_OUTLINE;
		m_fkey_option[8] = FK_INSIDE_POLYLINE;
		if( m_Doc->m_outline_poly[m_sel_id.i].GetNumContour(m_sel_id.ii) )
			m_fkey_option[8] = FK_DELETE_CUTOUT;
		break;

	case CUR_AREA_CORNER_SELECTED:
		prev_middle_x = (ppx + prevx)/2;
		prev_middle_y = (ppy + prevy)/2;
		ppx = prevx;
		ppy = prevy;
		prevx = m_sel_net->area[m_sel_id.i].poly->GetX(m_sel_id.ii);
		prevy = m_sel_net->area[m_sel_id.i].poly->GetY(m_sel_id.ii);
		//
		m_fkey_option[0] = FK_SET_POSITION;
		m_fkey_option[1] = FK_EDIT_AREA;
		m_fkey_option[2] = FK_ALIGN_SIDES;
		m_fkey_option[3] = FK_MOVE_CORNER;
		m_fkey_option[4] = FK_START_CUTOUT;
		m_fkey_option[5] = FK_START_SIMILAR;
		{
			CPolyLine * poly = m_sel_net->area[m_sel_ia].poly;
			m_fkey_option[6] = FK_AREA_CUTOUT;
		}
		m_fkey_option[6] = FK_ALIGN_X;
		m_fkey_option[7] = FK_ALIGN_Y;
		//m_fkey_option[7] = FK_DELETE_AREA;
		m_fkey_option[8] = FK_DELETE_CORNER;
		break;

	case CUR_AREA_SIDE_SELECTED:
		m_fkey_option[0] = FK_SIDE_STYLE;
		m_fkey_option[1] = FK_EDIT_AREA;
		m_fkey_option[4] = FK_SELECT_CONTOUR;
		m_fkey_option[5] = FK_TEST_ON_CONTACT;
		{
			int style = m_sel_net->area[m_sel_id.i].poly->GetSideStyle(m_sel_id.ii);
			if( style == CPolyLine::STRAIGHT )
				m_fkey_option[2] = FK_ADD_CORNER;
		}
		{
			CPolyLine * poly = m_sel_net->area[m_sel_ia].poly;
			m_fkey_option[6] = FK_AREA_CUTOUT;
		}
		m_fkey_option[7] = FK_DELETE_AREA;
		break;

	case CUR_SEG_SELECTED:
		m_fkey_option[0] = FK_SET_WIDTH;
		m_fkey_option[1] = FK_CHANGE_LAYER;
		m_fkey_option[2] = FK_ADD_VERTEX;
		m_fkey_option[4] = FK_UNROUTE;
		if(SegmentMovable())
			m_fkey_option[3] = FK_MOVE_SEGMENT;
		if( m_sel_con.end_pin == cconnect::NO_END )
		{
			if( m_sel_con.nsegs == (m_sel_id.ii+1) || m_sel_next_vtx.tee_ID )
				m_fkey_option[8] = FK_DELETE_SEGMENT;
		}
		else
		{
			// normal trace
			m_fkey_option[8] = FK_DELETE_SEGMENT;
		}
		m_fkey_option[5] = FK_UNROUTE_TRACE;
		m_fkey_option[6] = FK_SELECT_IN_LAYER;
		m_fkey_option[7] = FK_DELETE_CONNECT;
		break;

	case CUR_RAT_SELECTED:
		m_fkey_option[0] = FK_SET_WIDTH;
		if( m_sel_con.locked )
			m_fkey_option[2] = FK_UNLOCK_CONNECT;
		else
			m_fkey_option[2] = FK_LOCK_CONNECT;
		m_fkey_option[3] = FK_ROUTE;
		if (m_sel_con.nsegs > 1) 
		{
			m_fkey_option[5] = FK_UNROUTE_TRACE;
			if ( m_sel_id.ii == 0 || (m_sel_id.ii == (m_sel_con.nsegs-1) && m_sel_con.end_pin >= 0) ) 
			{
				m_fkey_option[4] = FK_CHANGE_PIN;
			}
		}
		if( m_sel_con.nsegs == 1 )
			m_fkey_option[6] = FK_SPLIT_NET;
		m_fkey_option[7] = FK_DELETE_CONNECT;
		if( m_sel_con.end_pin >= 0 || m_sel_is == m_sel_con.nsegs-1 || m_sel_next_vtx.tee_ID )
			m_fkey_option[8] = FK_DELETE_SEGMENT;
		break;

	case CUR_VTX_SELECTED:
		if( m_page == 1 )
		{
			prev_middle_x = (ppx + prevx)/2;
			prev_middle_y = (ppy + prevy)/2;
			ppx = prevx;
			ppy = prevy;
			prevx = m_sel_vtx.x;
			prevy = m_sel_vtx.y;
			//
			m_fkey_option[0] = FK_SET_POSITION;
			m_fkey_option[1] = FK_ADD_CONNECT;
			if( m_sel_vtx.via_w )
				m_fkey_option[4] = FK_VIA_SIZE;
			else if ( !m_sel_vtx.tee_ID )
				m_fkey_option[4] = FK_INSERT_SEGMENT;
			m_fkey_option[3] = FK_MOVE_VERTEX;
			m_fkey_option[5] = FK_ALIGN_MIDDLE;
			m_fkey_option[2] = FK_ALIGN_SEGMENTS;
			if ( !m_sel_vtx.via_w && !m_sel_vtx.tee_ID )
				m_fkey_option[5] = FK_INSERT_VERTICES;
			m_fkey_option[6] = FK_ALIGN_X;
			m_fkey_option[7] = FK_ALIGN_Y;
			m_fkey_option[8] = FK_DELETE_VERTEX;
		}
		else if ( m_page == 2 )
		{
			m_fkey_option[4] = FK_ALIGN_MIDLINE;
			m_fkey_option[5] = FK_ALIGN_MIDDLE;
			m_fkey_option[6] = FK_ALIGN_MIDDLE_X;
			m_fkey_option[7] = FK_ALIGN_MIDDLE_Y;
		}
		break;

	case CUR_END_VTX_SELECTED:
		prev_middle_x = (ppx + prevx)/2;
		prev_middle_y = (ppy + prevy)/2;
		ppx = prevx;
		ppy = prevy;
		prevx = m_sel_vtx.x;
		prevy = m_sel_vtx.y;
		//
		m_fkey_option[0] = FK_SET_POSITION;
		m_fkey_option[1] = FK_ADD_CONNECT;
		m_fkey_option[2] = FK_ADD_SEGMENT;
		m_fkey_option[3] = FK_MOVE_VERTEX;
		if( m_sel_vtx.via_w )
			m_fkey_option[4] = FK_VIA_SIZE;
		else
			m_fkey_option[4] = FK_ADD_VIA;	
		if( m_sel_vtx.via_w )
			m_fkey_option[5] = FK_UNROUTE_TRACE;
		m_fkey_option[6] = FK_ALIGN_X;
		m_fkey_option[7] = FK_ALIGN_Y;
		m_fkey_option[8] = FK_DELETE_VERTEX;
		break;

	case CUR_CONNECT_SELECTED:
		m_fkey_option[0] = FK_SET_WIDTH;
		m_fkey_option[1] = FK_CHANGE_LAYER;
		m_fkey_option[5] = FK_UNROUTE_TRACE;
		m_fkey_option[7] = FK_DELETE_CONNECT;
		m_fkey_option[8] = FK_REDO_RATLINES;
		break;

	case CUR_NET_SELECTED:
		m_fkey_option[0] = FK_SET_WIDTH;
		m_fkey_option[1] = FK_CHANGE_LAYER;
		m_fkey_option[4] = FK_EDIT_NET;
		m_fkey_option[8] = FK_REDO_RATLINES;
		break;

	case CUR_GROUP_SELECTED:
		if ( m_page == 1 )
		{
			if( m_Doc->m_num_copper_layers > 1 )
				m_fkey_option[2] = FK_SIDE;
			m_fkey_option[3] = FK_MOVE_GROUP;
			if( TestSelElements( FOR_FK_PARTS_AUTO_LOC ) )
				m_fkey_option[4] = FK_PARTS_AUTO_LOC;
			m_fkey_option[5] = FK_ROTATE_GROUP;
			if( m_sel_flags != VTX_ONLY )
			{
				m_fkey_option[7] = FK_DELETE_GROUP;
				m_fkey_option[8] = FK_DEL_WITH_MERGE;	
			}
			//
			if ( m_sel_flags == CONNECT_ONLY || m_sel_flags == VTX_ONLY )
			{
				// key F1
				if( m_sel_flags != VTX_ONLY )
					m_fkey_option[0] = FK_SET_WIDTH;

				// key F2 F3
				if ( TestSelElements( FOR_FK_RADIUS_UP_DOWN ) )
				{
					m_fkey_option[1] = FK_RADIUS_DOWN;
					m_fkey_option[2] = FK_RADIUS_UP;
				}
				else if ( TestSelElements( FOR_FK_ALIGN_SEGMENTS ) )
					m_fkey_option[2] = FK_ALIGN_SEGMENTS;

				// key F4
				if ( TestSelElements( FOR_FK_SET_CLEARANCE ) )
					m_fkey_option[3] = FK_SET_CLEARANCE;
				else if( m_sel_flags != VTX_ONLY )
					m_fkey_option[3] = FK_COPY_TRACES;

				// key F5
				if( m_sel_flags != VTX_ONLY )
					m_fkey_option[4] = FK_UNROUTE;

				// key F6
				if( TestSelElements( FOR_FK_SELECT_BETWEEN ) )
					m_fkey_option[5] = FK_SEL_BETWEEN;

				// key F7
				if( TestSelElements( FOR_FK_INSERT_SEGMENT ) )
					m_fkey_option[6] = FK_INSERT_SEGMENT;		
			}
			else 
			{
				if ( !getbit( m_sel_flags, FLAG_SEL_PART ) &&
					 !getbit( m_sel_flags, FLAG_SEL_TEXT ) &&
					 !getbit( m_sel_flags, FLAG_SEL_DRE ) &&
					 !getbit( m_sel_flags, FLAG_SEL_VTX ))
					m_fkey_option[0] = FK_SET_CLEARANCE;
				if( m_sel_merge_name.GetLength() )
					m_fkey_option[7] = FK_TAKE_APART;
				if( getbit( m_sel_mask, SEL_MASK_MERGES ) )
				{
					if( m_sel_merge_name.GetLength() )
					{
						m_fkey_option[1] = FK_MERGE_GROUP;
					}
					else if( !m_Doc->m_mlist->MergeSelected() )
					{
						if( m_sel_flags != NET_ONLY )
						m_fkey_option[1] = FK_MERGE_GROUP;
					}
				}
				if ( m_sel_flags == PART_ONLY )
				{
					m_fkey_option[0] = FK_EDIT_PART;
					m_fkey_option[6] = FK_REF_AUTO_LOC;
					m_fkey_option[8] = FK_EDIT_SILK;
				}
				else if ( getbit( m_sel_flags, FLAG_SEL_OP ) || getbit( m_sel_flags, FLAG_SEL_AREA ) )
					m_fkey_option[4] = FK_SELECT_CONTOUR;
			}
		}
		else if ( m_page == 2 )
		{
			if ( m_sel_flags == PART_ONLY )
			{
				m_fkey_option[0] = FK_SET_WIDTH;
				m_fkey_option[1] = FK_SET_LINES_VIS;
				m_fkey_option[2] = FK_SET_LINES_INVIS;
				m_fkey_option[4] = FK_REFS_SIZE;
				m_fkey_option[5] = FK_VALUES_SIZE;	
				m_fkey_option[6] = FK_VALUES_VIS;	
				m_fkey_option[7] = FK_VALUES_INVIS;	
			}
			else
			{
				m_fkey_option[1] = FK_CLEARANCE_DOWN;
				m_fkey_option[2] = FK_CLEARANCE_UP;
			}
			m_fkey_option[8] = FK_OK;
		}
		else if ( m_page == 3 )
		{
			m_fkey_option[0] = FK_ROTATE_GROUP__1;
			m_fkey_option[1] = FK_ROTATE_GROUP_1;
			m_fkey_option[2] = FK_ROTATE_GROUP__5;
			m_fkey_option[3] = FK_ROTATE_GROUP_5;
			m_fkey_option[4] = FK_ROTATE_GROUP__45;
			m_fkey_option[5] = FK_ROTATE_GROUP_45;
			m_fkey_option[6] = FK_ROTATE_GROUP__90;
			m_fkey_option[7] = FK_ROTATE_GROUP_90;		
			m_fkey_option[8] = FK_OK;
		}
		break;
	case CUR_ADD_AREA_CUTOUT:
		if( m_page == 1 )
			m_fkey_option[6] = FK_REPOUR;
		break;
	case CUR_DRAG_PART:
		m_fkey_option[0] = FK_SIDE;
		m_fkey_option[1] = FK_ROTATE_PART;
		m_fkey_option[2] = FK_ROTATE_PART_CCW;
		if( !m_dragging_new_item )
		{
			m_fkey_option[3] = FK_REDO_RATLINES;
			m_fkey_option[4] = FK_UNROUTE_OF_VIAS;
			m_fkey_option[5] = FK_UNROUTE_TRACE;
			m_fkey_option[6] = FK_LOCK_CONNECT;
			m_fkey_option[7] = FK_UNLOCK_CONNECT;
			m_fkey_option[8] = FK_DELETE_CONNECT;
		}
		break;

	case CUR_DRAG_REF:
		m_fkey_option[2] = FK_ROTATE_REF;
		break;

	case CUR_DRAG_VALUE:
		m_fkey_option[2] = FK_ROTATE_VALUE;
		break;

	case CUR_DRAG_TEXT:
		m_fkey_option[2] = FK_ROTATE_TEXT;
		break;

	case CUR_DRAG_VTX:
		break;

	case CUR_DRAG_RAT:
		m_fkey_option[0] = FK_BACK_WIDTH;
		m_fkey_option[1] = FK_NEXT_WIDTH;
		m_fkey_option[2] = FK_AS_PAD;
		m_fkey_option[3] = FK_COMPLETE;		
		break;

	case CUR_DRAG_STUB:
		m_fkey_option[0] = FK_BACK_WIDTH;
		m_fkey_option[1] = FK_NEXT_WIDTH;
		m_fkey_option[2] = FK_AS_PAD;
		m_fkey_option[4] = FK_DISABLE_BRANCH;
		m_fkey_option[5] = FK_BRANCH_TO_SEG;
		m_fkey_option[6] = FK_BRANCH_TO_VIA;
		break;

	case CUR_DRAG_AREA_1:
		m_fkey_option[0] = FK_POLY_STRAIGHT;
		m_fkey_option[1] = FK_POLY_ARC_CW;
		m_fkey_option[2] = FK_POLY_ARC_CCW;
		break;

	case CUR_DRAG_AREA_CUTOUT:
		m_fkey_option[0] = FK_POLY_STRAIGHT;
		m_fkey_option[1] = FK_POLY_ARC_CW;
		m_fkey_option[2] = FK_POLY_ARC_CCW;
		break;

	case CUR_DRAG_AREA_CUTOUT_1:
		m_fkey_option[0] = FK_POLY_STRAIGHT;
		m_fkey_option[1] = FK_POLY_ARC_CW;
		m_fkey_option[2] = FK_POLY_ARC_CCW;
		break;

	case CUR_DRAG_AREA:
		m_fkey_option[0] = FK_POLY_STRAIGHT;
		m_fkey_option[1] = FK_POLY_ARC_CW;
		m_fkey_option[2] = FK_POLY_ARC_CCW;
		break;

	case CUR_DRAG_OP:
		m_fkey_option[0] = FK_POLY_STRAIGHT;
		m_fkey_option[1] = FK_POLY_ARC_CW;
		m_fkey_option[2] = FK_POLY_ARC_CCW;
		break;

	case CUR_DRAG_OP_1:
		m_fkey_option[0] = FK_POLY_STRAIGHT;
		m_fkey_option[1] = FK_POLY_ARC_CW;
		m_fkey_option[2] = FK_POLY_ARC_CCW;
		break;
	}

	for( int i=0; i<12; i++ )
	{
		strcpy( m_fkey_str[2*i],   fk_str[2*m_fkey_option[i]] );
		strcpy( m_fkey_str[2*i+1], fk_str[2*m_fkey_option[i]+1] );
	}
	DrawBottomPane();
}
//===============================================================================================
int CFreePcbView::SegmentMovable(void)
{
	// see if this is the end of the road, if so, can't move it:
	{
		int x = m_sel_vtx.x;
		int y = m_sel_vtx.y;
		int layer = m_sel_seg.layer;
		/*BOOL test = m_Doc->m_nlist->TestHitOnConnectionEndPad( x, y, m_sel_net,
			m_sel_id.i, layer, 1 );*/
		if( /*test ||*/ m_sel_vtx.tee_ID || m_sel_id.ii == 0 || (m_sel_id.ii == (m_sel_con.nsegs-1) && m_sel_con.end_pin != cconnect::NO_END ) )
		{
			return FALSE;
		}
	}

	// see if end vertex of this segment is in end pad of connection
	{
		int x = m_sel_next_vtx.x;
		int y = m_sel_next_vtx.y;
		int layer = m_sel_seg.layer;
		/*BOOL test = m_Doc->m_nlist->TestHitOnConnectionEndPad( x, y, m_sel_net,
			m_sel_id.i, layer, 0 );*/
		if( /*test ||*/ m_sel_next_vtx.tee_ID)
		{
			return FALSE;
		}
	}
	return TRUE;
}
//===============================================================================================
// Draw bottom pane
//
void CFreePcbView::DrawBottomPane()
{
	if( !m_Doc )
		return;
	if( !m_Doc->m_project_open )
		return;

	CDC * pDC = GetDC();
	CFont * old_font = pDC->SelectObject( &m_small_font );

	// get client rectangle
	GetClientRect( &m_client_r );

	// draw labels for function keys at bottom of client area
	for( int j=0; j<3; j++ )
	{
		for( int i=0; i<4; i++ )
		{
			int ifn = j*4+i;
			if( ifn < 9 )
			{
				CRect r( FKEY_OFFSET_X+ifn*FKEY_STEP+j*FKEY_GAP,
					m_client_r.bottom-FKEY_OFFSET_Y-FKEY_R_H,
					FKEY_OFFSET_X+ifn*FKEY_STEP+j*FKEY_GAP+FKEY_R_W,
					m_client_r.bottom-FKEY_OFFSET_Y );
				pDC->Rectangle( &r );
				pDC->MoveTo( r.left+FKEY_SEP_W, r.top );
				pDC->LineTo( r.left+FKEY_SEP_W, r.top + FKEY_R_H/2 + 1 );
				pDC->MoveTo( r.left, r.top + FKEY_R_H/2 );
				pDC->LineTo( r.left+FKEY_SEP_W, r.top + FKEY_R_H/2 );
				r.top += 1;
				r.left += 2;
				char fkstr[3] = "F1";
				fkstr[1] = '1' + j*4+i;
				pDC->DrawText( fkstr, -1, &r, 0 );
				r.left += FKEY_SEP_W;
				char * str1 = &m_fkey_str[2*ifn][0];
				char * str2 = &m_fkey_str[2*ifn+1][0];
				pDC->DrawText( str1, -1, &r, 0 );
				r.top += FKEY_R_H/2 - 2;
				pDC->DrawText( str2, -1, &r, 0 );
			}
		}
	}
	pDC->SelectObject( old_font );
	ReleaseDC( pDC );
}
//===============================================================================================
void CFreePcbView::ShowRelativeDistance( int dx, int dy )
{
	CString str;
	CMainFrame * pMain = (CMainFrame*) AfxGetApp()->m_pMainWnd;
	double d = sqrt( (double)dx*dx + (double)dy*dy ) + m_measure_dist;  
	if( m_Doc->m_units == MIL )
		str.Format( "dx = %.1f, dy = %.1f, d = %.2f", (double)dx/NM_PER_MIL, (double)dy/NM_PER_MIL, d/NM_PER_MIL );
	else
		str.Format( "dx = %.3f, dy = %.3f, d = %.3f", (double)dx/1000000.0, (double)dy/1000000.0, d/1000000.0 );
	pMain->DrawStatus( 3, &str );
}
//===============================================================================================
void CFreePcbView::ShowRelativeDistance( int x, int y, int dx, int dy )
{
	CString str;
	CMainFrame * pMain = (CMainFrame*) AfxGetApp()->m_pMainWnd;
	double d = sqrt( (double)dx*dx + (double)dy*dy );  
	if( m_Doc->m_units == MIL )
		str.Format( "x = %.1f, y = %.1f, dx = %.1f, dy = %.1f, d = %.2f",
		(double)x/NM_PER_MIL, (double)y/NM_PER_MIL,
		(double)dx/NM_PER_MIL, (double)dy/NM_PER_MIL, d/NM_PER_MIL );
	else
		str.Format( "x = %.3f, y = %.3f, dx = %.3f, dy = %.3f, d = %.3f", 
		x/1000000.0, y/1000000.0,
		dx/1000000.0, dy/1000000.0, d/1000000.0 );
	pMain->DrawStatus( 3, &str );
}
//===============================================================================================
// display selected item in status bar
//
int CFreePcbView::ShowSelectStatus()
{
//#define SHOW_UIDS

	CString x_str, y_str, w_str, hole_str, via_w_str, via_hole_str;
	int u = m_Doc->m_units;

	CMainFrame * pMain = (CMainFrame*) AfxGetApp()->m_pMainWnd;
	if( !pMain )
		return 1;
	
	CString str="";

	switch( m_cursor_mode )
	{
	case CUR_NONE_SELECTED:
		str.Format( "No selection (%d)", m_Doc->m_nlist->m_pos_i );
		break;

	case CUR_DRE_SELECTED:
		str.Format( "DRE %s", m_sel_dre->str );
		break;

	case CUR_OP_CORNER_SELECTED:
		::MakeCStringFromDimension( &x_str, m_Doc->m_outline_poly[m_sel_id.i].GetX(m_sel_id.ii), u, FALSE, FALSE, FALSE, u==MIL?1:3 );
		::MakeCStringFromDimension( &y_str, m_Doc->m_outline_poly[m_sel_id.i].GetY(m_sel_id.ii), u, FALSE, FALSE, FALSE, u==MIL?1:3 );
		if( m_Doc->m_outline_poly[m_sel_id.i].GetClosed() )
			str.Format( "polyline %d, corner %d, x=%s, y=%s",
						 m_sel_id.i+1, m_sel_id.ii+1, x_str, y_str );
		else
			str.Format( "outline %d, corner %d, x=%s, y=%s",
						 m_sel_id.i+1, m_sel_id.ii+1, x_str, y_str );
		break;

	case CUR_OP_SIDE_SELECTED:
		{
			CString style_str, len;
			int lr = m_Doc->m_outline_poly[m_sel_id.i].GetLayer();
			int x = m_Doc->m_outline_poly[m_sel_id.i].GetX(m_sel_id.ii);
			int y = m_Doc->m_outline_poly[m_sel_id.i].GetY(m_sel_id.ii);
			int xn = m_Doc->m_outline_poly[m_sel_id.i].GetX(m_Doc->m_outline_poly[m_sel_id.i].GetIndexCornerNext(m_sel_id.ii));
			int yn = m_Doc->m_outline_poly[m_sel_id.i].GetY(m_Doc->m_outline_poly[m_sel_id.i].GetIndexCornerNext(m_sel_id.ii));
			double dx = xn-x;
			double dy = yn-y;
			double dlen = sqrt(dx*dx + dy*dy);
			::MakeCStringFromDimension( &len, dlen, u, TRUE, FALSE, FALSE, u==MIL?1:3 );
			if( m_Doc->m_outline_poly[m_sel_id.i].GetSideStyle( m_sel_id.ii ) == CPolyLine::STRAIGHT )
				style_str = "straight";
			else if( m_Doc->m_outline_poly[m_sel_id.i].GetSideStyle( m_sel_id.ii ) == CPolyLine::ARC_CW )
				style_str = "arc(cw)";
			else if( m_Doc->m_outline_poly[m_sel_id.i].GetSideStyle( m_sel_id.ii ) == CPolyLine::ARC_CCW )
				style_str = "arc(ccw)";
			if( m_Doc->m_outline_poly[m_sel_id.i].GetClosed() )
				str.Format( "%s: polyline %d, side %d of %d, %s, len= %s", layer_str[lr], m_sel_id.i+1, m_sel_id.ii+1,
							m_Doc->m_outline_poly[m_sel_id.i].GetNumCorners(), style_str, len );
			else
				str.Format( "%s: outline %d, side %d of %d, %s, len= %s", layer_str[lr], m_sel_id.i+1, m_sel_id.ii+1,
							m_Doc->m_outline_poly[m_sel_id.i].GetNumCorners(), style_str, len );
		}
		break;

	case CUR_PART_SELECTED:
		{
			CString side = "top"; 
			if( m_sel_part->side ) 
				side = "bottom";
			::MakeCStringFromDimension( &x_str, m_sel_part->x, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			::MakeCStringFromDimension( &y_str, m_sel_part->y, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			int rep_angle = ::GetReportedAngleForPart( m_sel_part->angle, 
				m_sel_part->shape->m_centroid_angle, m_sel_part->side );
			str.Format( "part %s \"%s\", x=%s, y=%s, angle %d, %s",
				m_sel_part->ref_des, m_sel_part->shape->m_name, 
				x_str, y_str, 
				rep_angle,
				side );
			int a = ::GetPartAngleForReportedAngle( rep_angle, 
				m_sel_part->shape->m_centroid_angle, m_sel_part->side );
			if( a != (m_sel_part->angle) )
				ASSERT(0);
		}
		break;

	case CUR_REF_SELECTED:
		str.Format( "ref text: %s", m_sel_part->ref_des ); 
		break;

	case CUR_VALUE_SELECTED:
		str.Format( "value: %s of part %s", m_sel_part->value, m_sel_part->ref_des );
		break;

	case CUR_PAD_SELECTED:
		{
			cnet * pin_net = (cnet*)m_sel_part->pin[m_sel_id.i].net;
			::MakeCStringFromDimension( &x_str, m_sel_part->pin[m_sel_id.i].x, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			::MakeCStringFromDimension( &y_str, m_sel_part->pin[m_sel_id.i].y, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			if( pin_net )
			{
				// pad attached to net
				str.Format( "pin %s.%s on net \"%s\", x=%s, y=%s",
					m_sel_part->ref_des,
					m_sel_part->shape->GetPinNameByIndex(m_sel_id.i),
					pin_net->name, x_str, y_str );
			}
			else
			{
				// pad not attached to a net
				str.Format( "pin %s.%s unconnected, x=%s, y=%s",
					m_sel_part->ref_des,
					m_sel_part->shape->GetPinNameByIndex(m_sel_id.i),
					x_str, y_str );
			}
		}
		break;

	case CUR_SEG_SELECTED:
	case CUR_RAT_SELECTED:
	case CUR_DRAG_STUB:
	case CUR_DRAG_RAT:
		{
			// get length
			CString len_str="", an_str="";
			double len=0, segment_angle=0;
			if (m_cursor_mode == CUR_SEG_SELECTED || m_cursor_mode == CUR_RAT_SELECTED )
			{
			segment_angle = Angle( m_sel_next_vtx.x, m_sel_next_vtx.y, m_sel_vtx.x, m_sel_vtx.y );
			if (segment_angle < 0.0)
				segment_angle = 360.0 + segment_angle;
			if (segment_angle > 180.0)
				segment_angle = 360.0 - segment_angle;
			if (segment_angle > 90.0)
				segment_angle = 180.0 - segment_angle;
			if (segment_angle > 45.0)
				segment_angle = 90.0 - segment_angle;
			segment_angle *= NM_PER_MM;
			double x =  m_sel_vtx.x;
			double y =  m_sel_vtx.y;
			double xn = m_sel_next_vtx.x;
			double yn = m_sel_next_vtx.y;
			len = sqrt( (x-xn)*(x-xn) + (y-yn)*(y-yn) );	
			::MakeCStringFromDimension( &len_str, len, u, TRUE, TRUE, FALSE, u==MIL?1:3 );
			::MakeCStringFromDimension( &an_str, segment_angle, u, FALSE, TRUE, FALSE, 1 );
			}
			int ep = m_sel_con.end_pin;
			if( m_sel_con.end_pin == cconnect::NO_END )
			{
				if( m_cursor_mode == CUR_DRAG_STUB )
				{
					// stub trace segment
					str.Format( "net \"%s\" stub(%d) from %s.%s",
						m_sel_net->name, m_sel_id.i+1,
						m_sel_start_pin.ref_des,
						m_sel_start_pin.pin_name
						);
				}
				else
				{
					// stub trace segment
#ifdef SHOW_UIDS
					if( m_sel_con.vtx[m_sel_con.nsegs].tee_ID )
						str.Format( "net \"%s\" branch(%d) to %s.%s, seg %d, width %d (T%d) uid %d",
							m_sel_net->name, m_sel_id.i+1,
							m_sel_start_pin.ref_des,
							m_sel_start_pin.pin_name,
							m_sel_id.ii+1,
							m_sel_seg.width/NM_PER_MIL,
							m_sel_con.vtx[m_sel_con.nsegs].tee_ID,
							m_sel_seg.m_uid
						);
					else
						str.Format( "net \"%s\" stub(%d) from %s.%s, seg %d, width %d uid %d",
							m_sel_net->name, m_sel_id.i+1,
							m_sel_start_pin.ref_des,
							m_sel_start_pin.pin_name,
							m_sel_id.ii+1,
							m_sel_seg.width/NM_PER_MIL,
							m_sel_seg.m_uid
						);
#else
					if( m_sel_is < m_sel_con.nsegs )
					{
						if( m_sel_con.vtx[m_sel_con.nsegs].tee_ID )
							str.Format( "net \"%s\" branch(%d) to %s.%s, angle=%s, width=%d, len=%s(T%d)",
								m_sel_net->name, m_sel_id.i+1,
								m_sel_start_pin.ref_des,
								m_sel_start_pin.pin_name,
								an_str,
								m_sel_seg.width/NM_PER_MIL, len_str,
								m_sel_con.vtx[m_sel_con.nsegs].tee_ID
							);
						else
							str.Format( "net \"%s\" stub(%d) from %s.%s, angle=%s, width=%d, len=%s",
								m_sel_net->name, m_sel_id.i+1,
								m_sel_start_pin.ref_des,
								m_sel_start_pin.pin_name,
								an_str,
								m_sel_seg.width/NM_PER_MIL,
								len_str
							);
					}
#endif
				}
			}
			else
			{
				// normal connected trace segment
				CString locked_flag = "";
				if( m_sel_con.locked )
					locked_flag = " (L)";
#ifdef SHOW_UIDS
				if( m_sel_con.nsegs == 1 && m_sel_seg.layer == LAY_RAT_LINE )
				{
					str.Format( "net \"%s\" connection(%d) %s.%s-%s.%s%s, seg %d, width %d uid %d",
						m_sel_net->name, m_sel_id.i+1,
						m_sel_start_pin.ref_des,
						m_sel_start_pin.pin_name,
						m_sel_end_pin.ref_des,
						m_sel_end_pin.pin_name,
						locked_flag, m_sel_id.ii+1,
						m_sel_seg.width/NM_PER_MIL,
						m_sel_seg.m_uid
						);
				}
				else
				{
					str.Format( "net \"%s\" trace(%d) %s.%s-%s.%s%s, seg %d, width %d uid %d",
						m_sel_net->name,  m_sel_id.i+1,
						m_sel_start_pin.ref_des,
						m_sel_start_pin.pin_name,
						m_sel_end_pin.ref_des,
						m_sel_end_pin.pin_name,
						locked_flag, m_sel_id.ii+1,
						m_sel_seg.width/NM_PER_MIL,
						m_sel_seg.m_uid
						);
				}
#else
				if( m_sel_con.nsegs == 1 && m_sel_seg.layer == LAY_RAT_LINE )
				{
					str.Format( "net \"%s\" connection(%d) %s.%s-%s.%s%s, width=%d",
						m_sel_net->name, m_sel_id.i+1,
						m_sel_start_pin.ref_des,
						m_sel_start_pin.pin_name,
						m_sel_end_pin.ref_des,
						m_sel_end_pin.pin_name,
						locked_flag,
						m_sel_seg.width/NM_PER_MIL
						);
				}
				else if( m_sel_is < m_sel_con.nsegs )
				{
					str.Format( "net \"%s\" trace(%d) %s.%s-%s.%s%s, angle=%s, width=%d, len=%s",
						m_sel_net->name,  m_sel_id.i+1,
						m_sel_start_pin.ref_des,
						m_sel_start_pin.pin_name,
						m_sel_end_pin.ref_des,
						m_sel_end_pin.pin_name,
						locked_flag, an_str,
						m_sel_seg.width/NM_PER_MIL, len_str
						);
				}
#endif
			}
		}
		break;

	case CUR_VTX_SELECTED:
		{
			CString locked_flag = "";
			if( m_sel_con.locked )
				locked_flag = " (L)";
			CString tee_flag = "";
			if( int id = m_sel_vtx.tee_ID )
				tee_flag.Format( " (T%d)", id );
			if( m_sel_vtx.force_via_flag )
				tee_flag = "(F)" + tee_flag;
			int via_w = m_sel_vtx.via_w;
			int uid = m_sel_vtx.m_uid;
			::MakeCStringFromDimension( &x_str, m_sel_vtx.x, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			::MakeCStringFromDimension( &y_str, m_sel_vtx.y, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			::MakeCStringFromDimension( &via_w_str, m_sel_vtx.via_w, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			::MakeCStringFromDimension( &via_hole_str, m_sel_vtx.via_hole_w, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
#ifdef SHOW_UIDS
			if( m_sel_con.end_pin == cconnect::NO_END )
			{
				// vertex of stub trace
				if( via_w )
				{
					// via
					if( m_sel_con.vtx[m_sel_con.nsegs].tee_ID )
						str.Format( "net \"%s\" branch(%d) to %s.%s, vertex %d, x %s, y %s, via %s/%s %s uid %d",
							m_sel_net->name, m_sel_id.i+1,
							m_sel_start_pin.ref_des,
							m_sel_start_pin.pin_name,
							m_sel_id.ii,
							x_str,
							y_str,
							via_w_str,
							via_hole_str,
							tee_flag,
							uid
							);
					else
						str.Format( "net \"%s\" stub(%d) from %s.%s, vertex %d, x %s, y %s, via %s/%s %s uid %d",
							m_sel_net->name, m_sel_id.i+1,
							m_sel_start_pin.ref_des,
							m_sel_start_pin.pin_name,
							m_sel_id.ii,
							x_str,
							y_str,
							via_w_str,
							via_hole_str,
							tee_flag,
							uid
							);
				}
				else
				{
					// no via
					if( m_sel_con.vtx[m_sel_con.nsegs].tee_ID )
						str.Format( "net \"%s\" branch(%d) to %s.%s, vertex %d, x %s, y %s %s uid %d",
							m_sel_net->name, m_sel_id.i+1,
							m_sel_start_pin.ref_des,
							m_sel_start_pin.pin_name,
							m_sel_id.ii,
							x_str,
							y_str,
							tee_flag,
							uid
							);
					else
						str.Format( "net \"%s\" stub(%d) from %s.%s, vertex %d, x %s, y %s %s uid %d",
							m_sel_net->name, m_sel_id.i+1,
							m_sel_start_pin.ref_des,
							m_sel_start_pin.pin_name,
							m_sel_id.ii,
							x_str,
							y_str,
							tee_flag,
							uid
							);
				}
			}
			else
			{
				// vertex of normal connected trace
				if( via_w )
				{
					// with via
					str.Format( "net \"%s\" trace(%d) %s.%s-%s.%s%s, vertex %d, x %s, y %s, via %s/%s %s uid %d",
						m_sel_net->name, m_sel_id.i+1,
						m_sel_start_pin.ref_des,
						m_sel_start_pin.pin_name,
						m_sel_end_pin.ref_des,
						m_sel_end_pin.pin_name,
						locked_flag,
						m_sel_id.ii,
						x_str,
						y_str,
						via_w_str,
						via_hole_str,
						tee_flag,
						uid
						);
				}
				else
				{
					// no via
					str.Format( "net \"%s\" trace(%d) %s.%s-%s.%s%s, vertex %d, x %s, y %s %s uid %d",
						m_sel_net->name, m_sel_id.i+1,
						m_sel_start_pin.ref_des,
						m_sel_start_pin.pin_name,
						m_sel_end_pin.ref_des,
						m_sel_end_pin.pin_name,
						locked_flag,
						m_sel_id.ii,
						x_str,
						y_str,
						tee_flag,
						uid
						);
				}
			}
#else
			if( m_sel_con.end_pin == cconnect::NO_END )
			{
				// vertex of stub trace
				if( via_w )
				{
					// via
					if( m_sel_con.vtx[m_sel_con.nsegs].tee_ID )
						str.Format( "net \"%s\" branch(%d) to %s.%s, vertex %d, x=%s, y=%s, via %s/%s %s",
							m_sel_net->name, m_sel_id.i+1,
							m_sel_start_pin.ref_des,
							m_sel_start_pin.pin_name,
							m_sel_id.ii,
							x_str,
							y_str,
							via_w_str,
							via_hole_str,
							tee_flag
							);
					else
						str.Format( "net \"%s\" stub(%d) from %s.%s, vertex %d, x=%s, y=%s, via %s/%s %s",
							m_sel_net->name, m_sel_id.i+1,
							m_sel_start_pin.ref_des,
							m_sel_start_pin.pin_name,
							m_sel_id.ii,
							x_str,
							y_str,
							via_w_str,
							via_hole_str,
							tee_flag
							);
				}
				else
				{
					// no via
					if( m_sel_con.vtx[m_sel_con.nsegs].tee_ID )
						str.Format( "net \"%s\" branch(%d) to %s.%s, vertex %d, x=%s, y=%s %s",
							m_sel_net->name, m_sel_id.i+1,
							m_sel_start_pin.ref_des,
							m_sel_start_pin.pin_name,
							m_sel_id.ii,
							x_str,
							y_str,
							tee_flag
							);
					else
						str.Format( "net \"%s\" stub(%d) from %s.%s, vertex %d, x=%s, y=%s %s",
							m_sel_net->name, m_sel_id.i+1,
							m_sel_start_pin.ref_des,
							m_sel_start_pin.pin_name,
							m_sel_id.ii,
							x_str,
							y_str,
							tee_flag
							);
				}
			}
			else
			{
				// vertex of normal connected trace
				if( via_w )
				{
					// with via
					str.Format( "net \"%s\" trace(%d) %s.%s-%s.%s%s, vertex %d, x=%s, y=%s, via %s/%s %s",
						m_sel_net->name, m_sel_id.i+1,
						m_sel_start_pin.ref_des,
						m_sel_start_pin.pin_name,
						m_sel_end_pin.ref_des,
						m_sel_end_pin.pin_name,
						locked_flag,
						m_sel_id.ii,
						x_str,
						y_str,
						via_w_str,
						via_hole_str,
						tee_flag
						);
				}
				else
				{
					// no via
					str.Format( "net \"%s\" trace(%d) %s.%s-%s.%s%s, vertex %d, x=%s, y=%s %s",
						m_sel_net->name, m_sel_id.i+1,
						m_sel_start_pin.ref_des,
						m_sel_start_pin.pin_name,
						m_sel_end_pin.ref_des,
						m_sel_end_pin.pin_name,
						locked_flag,
						m_sel_id.ii,
						x_str,
						y_str,
						tee_flag
						);
				}
			}
#endif
		}
		break;

	case CUR_END_VTX_SELECTED:
		{
			CString tee_flag = "";
			if( int id = m_sel_vtx.tee_ID )
				tee_flag.Format( " (T%d)", id );
			if( m_sel_vtx.force_via_flag )
				tee_flag = "(F)" + tee_flag;
			int uid = m_sel_vtx.m_uid;
			::MakeCStringFromDimension( &x_str, m_sel_vtx.x, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			::MakeCStringFromDimension( &y_str, m_sel_vtx.y, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			::MakeCStringFromDimension( &via_w_str, m_sel_vtx.via_w, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			::MakeCStringFromDimension( &via_hole_str, m_sel_vtx.via_hole_w, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
#ifdef SHOW_UIDS
			str.Format( "net \"%s\" stub(%d) end, x %s, y %s, via %s/%s %s uid %d",
				m_sel_net->name, m_sel_id.i+1,
				x_str,
				y_str,
				via_w_str,
				via_hole_str,
				tee_flag,
				uid
				);
#else
			str.Format( "net \"%s\" stub(%d) end, x=%s, y=%s, via %s/%s %s",
				m_sel_net->name, m_sel_id.i+1,
				x_str,
				y_str,
				via_w_str,
				via_hole_str,
				tee_flag
				);
#endif
		}
		break;

	case CUR_CONNECT_SELECTED:
		{
			CString locked_flag = "";
			if( m_sel_con.locked )
				locked_flag = " (L)";
			// get length of trace
			CString len_str;
			double len = 0;
			double last_x = m_sel_con.vtx[0].x;
			double last_y = m_sel_con.vtx[0].y;
			for( int iv=0; iv<=m_sel_con.nsegs; iv++ )
			{
				double x = m_sel_con.vtx[iv].x;
				double y = m_sel_con.vtx[iv].y;
				len += sqrt( (x-last_x)*(x-last_x) + (y-last_y)*(y-last_y) );
				last_x = x;
				last_y = y;
			}
			::MakeCStringFromDimension( &len_str, (int)len, u, TRUE, TRUE, FALSE, u==MIL?1:3 );
			if( m_sel_con.end_pin == cconnect::NO_END ) 
			{
				// stub or branch trace
				if( int id = m_sel_con.vtx[m_sel_con.nsegs].tee_ID )
				{
					CString tee_flag = "";
					tee_flag.Format( "(T%d)", id );
					str.Format( "net \"%s\" branch(%d) from %s.%s%s %s len=%s",
						m_sel_net->name, m_sel_id.i+1,
						m_sel_start_pin.ref_des,
						m_sel_start_pin.pin_name,
						locked_flag, tee_flag, len_str );
				}
				else
				{
					str.Format( "net \"%s\" stub(%d) from %s.%s%s len=%s",
						m_sel_net->name, m_sel_id.i+1,
						m_sel_start_pin.ref_des,
						m_sel_start_pin.pin_name,
						locked_flag, len_str );
				}
			}
			else
			{
				// normal trace
				str.Format( "net \"%s\" trace(%d) %s.%s-%s.%s%s len=%s",
					m_sel_net->name, m_sel_id.i+1,
					m_sel_start_pin.ref_des,
					m_sel_start_pin.pin_name,
					m_sel_end_pin.ref_des,
					m_sel_end_pin.pin_name,
					locked_flag, len_str );
			}
		}
		break;

	case CUR_NET_SELECTED:
		str.Format( "net \"%s\"", m_sel_net->name );
		break;

	case CUR_TEXT_SELECTED:
		{
			CString neg_str = "";
			if( m_sel_text->m_bNegative )
				neg_str = "(NEG)";
			str.Format( "Text \"%s\" %s", m_sel_text->m_str, neg_str ); 
			break;
		}

	case CUR_AREA_CORNER_SELECTED:
		{
			CPoint p = m_Doc->m_nlist->GetAreaCorner( m_sel_net, m_sel_ia, m_sel_is );
			::MakeCStringFromDimension( &x_str, p.x, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			::MakeCStringFromDimension( &y_str, p.y, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			str.Format( "\"%s\" copper area %d corner %d, x=%s, y=%s",
				m_sel_net->name, m_sel_id.i+1, m_sel_id.ii+1,
				x_str, y_str );
		}
		break;

	case CUR_AREA_SIDE_SELECTED:
		{
			CString len;
			int ic = m_sel_id.ii;
			int ia = m_sel_id.i;
			CPolyLine * p = m_sel_net->area[ia].poly;
			int x = p->GetX(ic);
			int y = p->GetY(ic);
			int xn = p->GetX(p->GetIndexCornerNext(ic));
			int yn = p->GetY(p->GetIndexCornerNext(ic));
			double dx = xn-x;
			double dy = yn-y;
			double dlen = sqrt(dx*dx + dy*dy);
			::MakeCStringFromDimension( &len, dlen, u, TRUE, FALSE, FALSE, u==MIL?1:3 );
			int ncont = p->GetContour(ic);
			if( ncont == 0 )
				str.Format( "\"%s\" copper area %d edge %d, len= %s", m_sel_net->name, ia+1, ic+1, len );
			else
			{
				str.Format( "\"%s\" copper area %d cutout %d edge %d, len= %s",
					m_sel_net->name, ia+1, ncont, ic+1-p->GetContourStart(ncont), len );
			}
		}
		break;

	case CUR_GROUP_SELECTED:
		if ( m_page  == 1 )
		{
			str.Format( "Group selected (%d)", m_sel_count );
			if ( m_sel_merge_name.GetLength() )
				str = m_sel_merge_name;
		}
		else if ( m_page  == 2 )
		{
			::MakeCStringFromDimension( &w_str, m_seg_clearance, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			str.Format( ("Clearance value: " + w_str) );
		}
		break;

	case CUR_ADD_OP:
		str.Format( "Placing first corner of outline" );
		break;

	case CUR_DRAG_OP_1:
		str.Format( "Placing second corner of outline" );
		break;

	case CUR_DRAG_OP:
		str.Format( "Placing corner %d of outline", m_sel_id.ii+2 );
		break;

	case CUR_DRAG_OP_INSERT:
		str.Format( "Inserting corner %d of outline", m_sel_id.ii+2 );
		break;

	case CUR_DRAG_OP_MOVE:
		str.Format( "Moving corner %d of outline", m_sel_id.ii+1 );
		break;

	case CUR_DRAG_PART:
		str.Format( "Moving part %s", m_sel_part->ref_des );
		break;

	case CUR_DRAG_REF:
		str.Format( "Moving ref text for part %s", m_sel_part->ref_des );
		break;

	case CUR_DRAG_VTX:
		str.Format( "Routing net \"%s\"", m_sel_net->name );
		break;

	case CUR_DRAG_END_VTX:
		str.Format( "Routing net \"%s\"", m_sel_net->name );
		break;

	case CUR_DRAG_TEXT:
		str.Format( "Moving text \"%s\"", m_sel_text->m_str );
		break;

	case CUR_ADD_AREA:
		str.Format( "Placing first corner of copper area" );
		break;

	case CUR_DRAG_AREA_1:
		str.Format( "Placing second corner of copper area" );
		break;

	case CUR_DRAG_AREA:
		str.Format( "Placing corner %d of copper area", m_sel_id.ii+1 );
		break;

	case CUR_DRAG_AREA_INSERT:
		str.Format( "Inserting corner %d of copper area", m_sel_id.ii+2 );
		break;

	case CUR_DRAG_AREA_MOVE:
		str.Format( "Moving corner %d of copper area", m_sel_id.ii+1 );
		break;

	case CUR_DRAG_CONNECT:
		if( m_sel_id.type == ID_PART )
			str.Format( "Adding connection to pin \"%s.%s\"",
			m_sel_part->ref_des,
			m_sel_part->shape->GetPinNameByIndex(m_sel_id.i) );
		else if( m_sel_id.type == ID_NET )
			str.Format( "Adding branch to trace \"%s.%d\"",
			m_sel_net->name,
			m_sel_id.i );
		break;

	case CUR_DRAG_MEASURE_1:
		str = "Measurement mode: left-click to start";
		break;

	}
	pMain->DrawStatus( 3, &str );
	return 0;
}
//===============================================================================================
// display cursor coords in status bar
//
int CFreePcbView::ShowCursor()
{
	CMainFrame * pMain = (CMainFrame*) AfxGetApp()->m_pMainWnd;
	if( !pMain )
		return 1;

	CString str;
	CPoint p;
	p = m_last_cursor_point;
	if( m_Doc->m_units == MIL )  
	{
		str.Format( "X: %8.1f", (double)m_last_cursor_point.x/PCBU_PER_MIL );
		pMain->DrawStatus( 1, &str );
		str.Format( "Y: %8.1f", (double)m_last_cursor_point.y/PCBU_PER_MIL );
		pMain->DrawStatus( 2, &str );
	}
	else
	{
		str.Format( "X: %8.3f", m_last_cursor_point.x/1000000.0 );
		pMain->DrawStatus( 1, &str );
		str.Format( "Y: %8.3f", m_last_cursor_point.y/1000000.0 );
		pMain->DrawStatus( 2, &str );
	}
	return 0;
}
//===============================================================================================
// display active layer in status bar and change layer order for DisplayList
//
int CFreePcbView::ShowActiveLayer(int n_layers, BOOL swCASE)
{
	static int CASE=0;
	if( swCASE )
		CASE = !CASE;
	CMainFrame * pMain = (CMainFrame*) AfxGetApp()->m_pMainWnd;
	if( !pMain )
		return 1;
	//
	//
	// DrawStatus
	CString str="Mask";
	if( m_active_layer == LAY_TOP_COPPER )
		str.Format( "Top" );
	else if( m_active_layer == LAY_BOTTOM_COPPER )
		str.Format( "Bottom" );
	else if( m_active_layer > LAY_BOTTOM_COPPER )
		str.Format( "Inner %d", m_active_layer - LAY_BOTTOM_COPPER );
	pMain->DrawStatus( 4, &str );
	//
	for( cnet * n=m_Doc->m_nlist->GetFirstNet(); n; n=m_Doc->m_nlist->GetNextNet(/*LABEL*/) )
	{
		for( int i=0; i<n->nconnects; i++ )
		{
			for( int ii=0; ii<n->connect[i].nsegs; ii++ )
			{
				if( n->connect[i].seg[ii].layer == LAY_RAT_LINE )
				{
					if( n->connect[i].seg[ii].dl_el )
					{
						if( n->visible || n->connect[i].nsegs > 1 ) 
							n->connect[i].seg[ii].dl_el->visible = 1;
					}
				}
			}
		}
	}
	//
	int shift = 0;
	int layer_bit = 0;
	// priority
	m_dlist->SetLayerDrawOrder( LAY_RAT_LINE, 0 );
	setbit( layer_bit, LAY_RAT_LINE );
	m_dlist->SetLayerDrawOrder( LAY_SELECTION, 1 );
	setbit( layer_bit, LAY_SELECTION );	
	m_dlist->SetLayerDrawOrder( LAY_HILITE, 2 );
	setbit( layer_bit, LAY_HILITE );
	m_dlist->SetLayerDrawOrder( LAY_PAD_THRU, 3 );
	setbit( layer_bit, LAY_PAD_THRU );
	m_dlist->SetLayerDrawOrder( LAY_DRC_ERROR, 4 );
	setbit( layer_bit, LAY_DRC_ERROR );
	int i=0;
	if( m_active_layer == LAY_SM_TOP )
	{
		//
		m_dlist->SetLayerDrawOrder( LAY_BOARD_OUTLINE, 5 );
		setbit( layer_bit, LAY_BOARD_OUTLINE );
		// scribing
		m_dlist->SetLayerDrawOrder( LAY_SCRIBING, 6 );
		setbit( layer_bit, LAY_SCRIBING );
		// notes on top
		m_dlist->SetLayerDrawOrder( LAY_REFINE_TOP, 3 );
		setbit( layer_bit, LAY_REFINE_TOP );
		m_dlist->SetLayerDrawOrder( LAY_PAD_THRU, 7 );
		// silk
		m_dlist->SetLayerDrawOrder( LAY_SILK_TOP, 8 );
		setbit( layer_bit, LAY_SILK_TOP );
		// mask
		m_dlist->SetLayerDrawOrder( LAY_SM_TOP, 9 );
		setbit( layer_bit, LAY_SM_TOP );
		//copper
		m_dlist->SetLayerDrawOrder( LAY_TOP_COPPER, 10 );
		setbit( layer_bit, LAY_TOP_COPPER );
		//copper
		m_dlist->SetLayerDrawOrder( LAY_BOTTOM_COPPER, 28 );
		setbit( layer_bit, LAY_BOTTOM_COPPER );
		// mask
		m_dlist->SetLayerDrawOrder( LAY_SM_BOTTOM, 29 );
		setbit( layer_bit, LAY_SM_BOTTOM );
		// silk
		m_dlist->SetLayerDrawOrder( LAY_SILK_BOTTOM, 30 );
		setbit( layer_bit, LAY_SILK_BOTTOM );
		// notes on bot
		m_dlist->SetLayerDrawOrder( LAY_REFINE_BOT, 31 );
		setbit( layer_bit, LAY_REFINE_BOT );
		//inners
		i = 11;
		for( int order=0; order<(LAY_TOP_COPPER+n_layers); order++ )
			if( getbit( layer_bit, order ) == 0 )
			{
				m_dlist->SetLayerDrawOrder( order, i );
				i++;
			}
	}
	else if( m_active_layer == LAY_SM_BOTTOM )
	{
		//
		m_dlist->SetLayerDrawOrder( LAY_BOARD_OUTLINE, 5 );
		setbit( layer_bit, LAY_BOARD_OUTLINE );
		// scribing
		m_dlist->SetLayerDrawOrder( LAY_SCRIBING, 6 );
		setbit( layer_bit, LAY_SCRIBING );
		// notes on bot
		m_dlist->SetLayerDrawOrder( LAY_REFINE_BOT, 3 );
		setbit( layer_bit, LAY_REFINE_BOT );
		m_dlist->SetLayerDrawOrder( LAY_PAD_THRU, 7 );
		// silk
		m_dlist->SetLayerDrawOrder( LAY_SILK_BOTTOM, 8 );
		setbit( layer_bit, LAY_SILK_BOTTOM );
		// mask
		m_dlist->SetLayerDrawOrder( LAY_SM_BOTTOM, 9 );
		setbit( layer_bit, LAY_SM_BOTTOM );
		//copper
		m_dlist->SetLayerDrawOrder( LAY_BOTTOM_COPPER, 10 );
		setbit( layer_bit, LAY_BOTTOM_COPPER );
		//copper
		m_dlist->SetLayerDrawOrder( LAY_TOP_COPPER, 28 );
		setbit( layer_bit, LAY_TOP_COPPER );
		// mask
		m_dlist->SetLayerDrawOrder( LAY_SM_TOP, 29 );
		setbit( layer_bit, LAY_SM_TOP );
		// silk
		m_dlist->SetLayerDrawOrder( LAY_SILK_TOP, 30 );
		setbit( layer_bit, LAY_SILK_TOP );
		// notes on top
		m_dlist->SetLayerDrawOrder( LAY_REFINE_TOP, 31 );
		setbit( layer_bit, LAY_REFINE_TOP );
		//inners
		i = 11;
		for( int order=0; order<(LAY_TOP_COPPER+n_layers); order++ )
			if( getbit( layer_bit, order ) == 0 )
			{
				m_dlist->SetLayerDrawOrder( order, i );
				i++;
			}
	}
	else if( m_active_layer == LAY_TOP_COPPER )
	{
		// copper 
		int tmp = 5;
		if( CASE )
		{
			// silk
			m_dlist->SetLayerDrawOrder( LAY_SILK_TOP, tmp );
			setbit( layer_bit, LAY_SILK_TOP );
			tmp++;
		}
		m_dlist->SetLayerDrawOrder( LAY_TOP_COPPER, tmp );
		setbit( layer_bit, LAY_TOP_COPPER );
		tmp++;
		// mask
		m_dlist->SetLayerDrawOrder( LAY_SM_TOP, tmp );
		setbit( layer_bit, LAY_SM_TOP );
		tmp++;
		if(!CASE )
		{
			// silk
			m_dlist->SetLayerDrawOrder( LAY_SILK_TOP, tmp );
			setbit( layer_bit, LAY_SILK_TOP );
		}
		//copper
		m_dlist->SetLayerDrawOrder( LAY_BOTTOM_COPPER, 28 );
		setbit( layer_bit, LAY_BOTTOM_COPPER );
		// mask
		m_dlist->SetLayerDrawOrder( LAY_SM_BOTTOM, 29 );
		setbit( layer_bit, LAY_SM_BOTTOM );
		// board
		m_dlist->SetLayerDrawOrder( LAY_BOARD_OUTLINE, 30 );
		setbit( layer_bit, LAY_BOARD_OUTLINE );
		// silk
		m_dlist->SetLayerDrawOrder( LAY_SILK_BOTTOM, 31 );
		setbit( layer_bit, LAY_SILK_BOTTOM );
		//inners
		i = 8;
		for( int order=0; order<(LAY_TOP_COPPER+n_layers); order++ )
			if( getbit( layer_bit, order ) == 0 )
			{
				m_dlist->SetLayerDrawOrder( order, i );
				i++;
			}
	}
	else if( m_active_layer == LAY_BOTTOM_COPPER )
	{
		int tmp = 5;
		if( CASE )
		{
			// silk
			m_dlist->SetLayerDrawOrder( LAY_SILK_BOTTOM, tmp );
			setbit( layer_bit, LAY_SILK_BOTTOM );
			tmp++;
		}
		//copper
		m_dlist->SetLayerDrawOrder( LAY_BOTTOM_COPPER, tmp );
		setbit( layer_bit, LAY_SM_BOTTOM );
		tmp++;
		// mask
		m_dlist->SetLayerDrawOrder( LAY_SM_BOTTOM, tmp );
		setbit( layer_bit, LAY_BOTTOM_COPPER );
		tmp++;
		if(!CASE )
		{
			// silk
			m_dlist->SetLayerDrawOrder( LAY_SILK_BOTTOM, tmp );
			setbit( layer_bit, LAY_SILK_BOTTOM );
		}
		//copper
		m_dlist->SetLayerDrawOrder( LAY_TOP_COPPER, 28 );
		setbit( layer_bit, LAY_TOP_COPPER );
		// mask
		m_dlist->SetLayerDrawOrder( LAY_SM_TOP, 29 );
		setbit( layer_bit, LAY_SM_TOP );
		// board
		m_dlist->SetLayerDrawOrder( LAY_BOARD_OUTLINE, 30 );
		setbit( layer_bit, LAY_BOARD_OUTLINE );
		// silk
		m_dlist->SetLayerDrawOrder( LAY_SILK_TOP, 31 );
		setbit( layer_bit, LAY_SILK_TOP );	
		//inners
		i = 8;
		for( int order=0; order<(LAY_TOP_COPPER+n_layers); order++ )
			if( getbit( layer_bit, order ) == 0 )
			{
				m_dlist->SetLayerDrawOrder( order, i );
				i++;
			}
	}
	else
	{
		m_dlist->SetLayerDrawOrder( LAY_BOARD_OUTLINE, 5 );
		setbit( layer_bit, LAY_BOARD_OUTLINE );
		// active
		m_dlist->SetLayerDrawOrder( m_active_layer, 6 );
		setbit( layer_bit, m_active_layer );
		// silk
		m_dlist->SetLayerDrawOrder( LAY_SILK_TOP, 26 );
		setbit( layer_bit, LAY_SILK_TOP );
		// mask
		m_dlist->SetLayerDrawOrder( LAY_SM_TOP, 27 );
		setbit( layer_bit, LAY_SM_TOP );
		//copper
		m_dlist->SetLayerDrawOrder( LAY_TOP_COPPER, 28 );
		setbit( layer_bit, LAY_TOP_COPPER );
		//copper
		m_dlist->SetLayerDrawOrder( LAY_BOTTOM_COPPER, 29 );
		setbit( layer_bit, LAY_BOTTOM_COPPER );
		// mask
		m_dlist->SetLayerDrawOrder( LAY_SM_BOTTOM, 30 );
		setbit( layer_bit, LAY_SM_BOTTOM );
		// silk
		m_dlist->SetLayerDrawOrder( LAY_SILK_BOTTOM, 31 );
		setbit( layer_bit, LAY_SILK_BOTTOM );
		//inners
		i = 7;
		for( int order=0; order<(LAY_TOP_COPPER+n_layers); order++ )
			if( getbit( layer_bit, order ) == 0 )
			{
				m_dlist->SetLayerDrawOrder( order, i );
				i++;
			}
	}
	for( int order=(LAY_TOP_COPPER+n_layers); order<MAX_LAYERS; order++ )
		if( getbit( layer_bit, order ) == 0 )
		{
			m_dlist->SetLayerDrawOrder( LAY_SELECTION, i );
			i++;
		}
	m_dlist->SetTopLayer( m_active_layer );
	//
	if( m_Doc->m_dlist->m_vis[LAY_BOTTOM_COPPER] == 0 || m_Doc->m_dlist->m_vis[LAY_TOP_COPPER] == 0 )
	{
		//
		// need to hide opposite side ratlines
		for( cnet * n=m_Doc->m_nlist->GetFirstNet(); n; n=m_Doc->m_nlist->GetNextNet() )
		{
			for( int i=0; i<n->nconnects; i++ )
			{
				int p2_layer = -1;
				cpart * prt = n->pin[n->connect[i].start_pin].part;
				int p_layer = m_Doc->m_plist->GetPinLayer( prt, n->connect[i].start_pin_shape );
				if( n->connect[i].end_pin != cconnect::NO_END )
				{
					prt = n->pin[n->connect[i].end_pin].part;
					p2_layer = m_Doc->m_plist->GetPinLayer( prt, n->connect[i].end_pin_shape );
				}
				for( int ii=0; ii<n->connect[i].nsegs; ii++ )
				{
					if( n->connect[i].seg[ii].layer == LAY_RAT_LINE )
					{
						if( n->connect[i].seg[ii].dl_el )
						{
							if( n->connect[i].end_pin == cconnect::NO_END )
							{
								if( ii == 0 && n->connect[i].nsegs == 1 && m_Doc->m_dlist->m_vis[p_layer] == 0 )
									n->connect[i].seg[ii].dl_el->visible = 0;
								else if( ii+1 < n->connect[i].nsegs )
								{
									if( m_Doc->m_vis[n->connect[i].seg[ii+1].layer] == 0 )
									{
										if( ii == 0 )
										{
											if( m_Doc->m_dlist->m_vis[p_layer] == 0 )
												n->connect[i].seg[ii].dl_el->visible = 0;
										}
										else if( m_Doc->m_vis[n->connect[i].seg[ii-1].layer] == 0 && 
												 n->connect[i].vtx[ii].via_w == 0 && 
												 n->connect[i].vtx[ii+1].via_w == 0 )
											n->connect[i].seg[ii].dl_el->visible = 0;
									}
								}
								else if( ii )
								{
									if( m_Doc->m_vis[n->connect[i].seg[ii-1].layer] == 0 )
										n->connect[i].seg[ii].dl_el->visible = 0;
								}
							}
							else
							{
								if( ii == 0 && n->connect[i].nsegs == 1 && 
									m_Doc->m_dlist->m_vis[p2_layer] == 0 &&
									m_Doc->m_dlist->m_vis[p_layer] == 0)
									n->connect[i].seg[ii].dl_el->visible = 0;
								else if( ii+1 < n->connect[i].nsegs )
								{
									if( m_Doc->m_vis[n->connect[i].seg[ii+1].layer] == 0 )
									{
										if( ii == 0 )
										{
											if( m_Doc->m_dlist->m_vis[p_layer] == 0 )
												n->connect[i].seg[ii].dl_el->visible = 0;
										}
										else if( m_Doc->m_vis[n->connect[i].seg[ii-1].layer] == 0 && 
												 n->connect[i].vtx[ii].via_w == 0 && 
												 n->connect[i].vtx[ii+1].via_w == 0 )
											n->connect[i].seg[ii].dl_el->visible = 0;
									}
								}
								else if( ii )
								{
									if( m_Doc->m_vis[n->connect[i].seg[ii-1].layer] == 0 && m_Doc->m_dlist->m_vis[p2_layer] == 0 )
										n->connect[i].seg[ii].dl_el->visible = 0;
								}
							}
						}
					}
				}
			}
		}
	}
	return 0;
}
//===============================================================================================
// handle mouse scroll wheel
//
BOOL CFreePcbView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	static  CPoint mem_p(0,0);

	// ignore if cursor not in window
	CRect wr;
	GetWindowRect( wr );
	if( pt.x < wr.left || pt.x > wr.right || pt.y < wr.top || pt.y > wr.bottom )
		return CView::OnMouseWheel(nFlags, zDelta, pt);

	// ignore if we are dragging a selection rect
	if( m_bDraggingRect )
		return CView::OnMouseWheel(nFlags, zDelta, pt);

	CRect screen_r;
	GetWindowRect( &screen_r );
	if( abs(mem_p.x-pt.x) > 30 || abs(mem_p.y-pt.y) > 30 )
	{
		// first wheel movement in a while
		// center window on cursor then center cursor
		CPoint p = m_dlist->ScreenToPCB( pt );	// convert to PCB coords
		m_org_x = p.x - ((m_client_r.right-m_left_pane_w)*m_pcbu_per_pixel)/2;
		m_org_y = p.y - ((m_client_r.bottom-m_bottom_pane_h)*m_pcbu_per_pixel)/2;	
		m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel, m_org_x, m_org_y );
		p = m_dlist->PCBToScreen(p);
		SetCursorPos( p.x, p.y );
	}
	else
	{
		// serial movements, zoom in or out
		if( zDelta > 0 && m_pcbu_per_pixel > NM_PER_MIL/1000 )
		{
			// wheel pushed, zoom in then center world coords and cursor
			CPoint p = m_dlist->ScreenToPCB( pt );	// convert to PCB coords
			m_pcbu_per_pixel = m_pcbu_per_pixel/ZOOM_RATIO;
			m_org_x = p.x - ((m_client_r.right-m_left_pane_w)*m_pcbu_per_pixel)/2;
			m_org_y = p.y - ((m_client_r.bottom-m_bottom_pane_h)*m_pcbu_per_pixel)/2;
			m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, 
								 m_pcbu_per_pixel, m_org_x, m_org_y );
			p = m_dlist->PCBToScreen(p);
			SetCursorPos( p.x, p.y );
		}
		else if( zDelta < 0 )
		{
			// wheel pulled, zoom out then center
			// first, make sure that window boundaries will be OK
			CPoint p = m_dlist->ScreenToPCB( pt );
			int org_x = p.x - ((m_client_r.right-m_left_pane_w)*m_pcbu_per_pixel*ZOOM_RATIO)/2;
			int org_y = p.y - ((m_client_r.bottom-m_bottom_pane_h)*m_pcbu_per_pixel*ZOOM_RATIO)/2;
			int max_x = org_x + (m_client_r.right-m_left_pane_w)*m_pcbu_per_pixel*ZOOM_RATIO;
			int max_y = org_y + (m_client_r.bottom-m_bottom_pane_h)*m_pcbu_per_pixel*ZOOM_RATIO;
			if( org_x > -PCB_BOUND && org_x < PCB_BOUND && max_x > -PCB_BOUND && max_x < PCB_BOUND
				&& org_y > -PCB_BOUND && org_y < PCB_BOUND && max_y > -PCB_BOUND && max_y < PCB_BOUND )
			{
				// OK, do it
				m_org_x = org_x;
				m_org_y = org_y;
				m_pcbu_per_pixel = m_pcbu_per_pixel*ZOOM_RATIO;
				m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, 
									 m_pcbu_per_pixel, m_org_x, m_org_y );
				p = m_dlist->PCBToScreen(p);
				SetCursorPos( p.x, p.y );
			}
		}
	}
	GetCursorPos( &mem_p );
	if( m_cursor_mode == CUR_DRAG_GROUP ||
		m_cursor_mode == CUR_DRAG_GROUP_ADD )
	{
		return 1;
	}
	else
	{
		Invalidate( FALSE );//OnMouseWheel
	}
	return CView::OnMouseWheel(nFlags, zDelta, pt);

}
//===============================================================================================
// SelectPart...this is called from FreePcbDoc when a new part is added
// selects the new part as long as the cursor is not dragging something
//
int CFreePcbView::SelectPart( cpart * part, BOOL bPins )
{
	if( !part )
		ASSERT(0);
	m_Doc->m_plist->HighlightPart(part,!bPins);
	if( bPins )
		m_Doc->m_plist->SelectPads( part, 1, LAY_TOP_COPPER + part->side );
	return 1;
}
//===============================================================================================
// cancel selection
//
void CFreePcbView::CancelSelection( BOOL hDialog )
{
	// reset m_prev_sel_merge
	if( getbit(m_sel_flags, FLAG_SEL_PART) || getbit(m_sel_flags, FLAG_SEL_NET)  )
		m_prev_sel_merge = -1;
	// get prev merge of part
	if( m_sel_id.type == ID_PART && m_sel_id.st == ID_PAD )
		if( m_sel_part )
			m_prev_sel_merge = m_sel_part->m_merge;
	// merge of connect
	if( m_sel_id.type == ID_NET && m_sel_id.st == ID_CONNECT )
		if( m_sel_net )
			if( m_sel_id.i < m_sel_net->nconnects )
				m_prev_sel_merge = m_sel_con.m_merge;
	// merge of area
	if( m_sel_id.type == ID_NET && m_sel_id.st == ID_AREA )
		if( m_sel_net )
			if( m_sel_ia < m_sel_net->nareas )
				m_prev_sel_merge = m_sel_net->area[m_sel_ia].poly->GetMerge();
	// remark selected elements
	if( getbit(m_sel_flags, FLAG_SEL_PART) )
		for(cpart* p=m_Doc->m_plist->GetFirstPart(); p; p=m_Doc->m_plist->GetNextPart(p))
			if( p->selected )
			{
				p->selected = FALSE;
				m_prev_sel_merge = p->m_merge;
			}
	// merge of poly
	if( m_sel_id.type == ID_POLYLINE )
	{
		if( m_sel_id.i < m_Doc->m_outline_poly.GetSize() )
			m_prev_sel_merge = m_Doc->m_outline_poly[m_sel_id.i].GetMerge();
	}
	if( getbit(m_sel_flags, FLAG_SEL_NET) )
	{
		for(cnet* n=m_Doc->m_nlist->GetFirstNet(); n; n=m_Doc->m_nlist->GetNextNet(/*LABEL*/))
		{
			if( n->selected )
			{
				n->selected = FALSE;
				if( getbit(m_sel_flags, FLAG_SEL_CONNECT) )
				{
					for (int i=0; i<n->nconnects; i++)
					{
						if( n->connect[i].m_selected )
						{
							//m_prev_sel_merge = n->connect[i].m_merge;
							n->connect[i].m_selected = FALSE;
							for(int ic=0; ic<n->connect[i].nsegs; ic++)
							{
								n->connect[i].seg[ic].selected = FALSE;
								n->connect[i].vtx[ic].selected = FALSE;
							}
							n->connect[i].vtx[n->connect[i].nsegs].selected = FALSE;
						}
					}
				}
				if( getbit(m_sel_flags, FLAG_SEL_AREA) )
				{
					for (int i=0; i<n->nareas; i++)
					{
						if( n->area[i].selected )
						{
							//m_prev_sel_merge = n->area[i].poly->GetMerge();
							n->area[i].selected = FALSE;
							for( int ic=n->area[i].poly->GetNumCorners()-1; ic>=0; ic-- )
							{
								n->area[i].poly->SetSideSel(ic,FALSE);
								n->area[i].poly->SetSel(ic,FALSE);
							}
						}
					}
				}
			}
		}
	}
	if( getbit(m_sel_flags, FLAG_SEL_TEXT) )
	{
		int it = 0;
		for(CText* t=m_Doc->m_tlist->GetFirstText(); t; t=m_Doc->m_tlist->GetNextText(&it))
			t->m_selected = FALSE;
	}
	if( getbit(m_sel_flags, FLAG_SEL_OP) )
		for( int item=m_Doc->m_outline_poly.GetSize()-1; item>=0; item-- )
		{
			int gns = m_Doc->m_outline_poly[item].GetNumCorners()-1;
			int cl = m_Doc->m_outline_poly[item].GetClosed();
			for(int ic=gns; ic>=0; ic--)
			{	
				m_Doc->m_outline_poly[item].SetSel( ic,FALSE );
				if( cl == 0 && ic == gns )
					continue;
				m_Doc->m_outline_poly[item].SetSideSel( ic,FALSE );
			}
		}
	// remove all texts from thru pad layer
	int it = 0;
	for( CText * t=m_Doc->m_tlist->GetFirstText(); t; t=m_Doc->m_tlist->GetNextText(&it) )
		if( t->m_layer == LAY_PAD_THRU )
		{
			if( m_Doc->m_tlist->RemoveText(t) )
				ASSERT(0);
			it--;
		}
	m_Doc->m_dlist->CancelHighLight();
	m_sel_part = 0;
	m_sel_net = 0;
	m_sel_text = 0;
	m_sel_dre = 0;
	m_sel_id.Clear();
	m_sel_flags = 0;
	m_sel_count = 0;
	prev_sel_count = 0;
	prev_m_ang = -1;
	///
	m_page = 1;
	m_sel_merge_name = "";
	gLastKeyWasArrow = FALSE;
	gShiftKeyDown = FALSE;
	// mark 0
	m_Doc->m_nlist->MarkAllNets(0);
	m_Doc->m_plist->MarkAllParts(0);
	MarkAllOutlinePoly(0,-1);

	// dlg log
	if( hDialog )
		m_Doc->m_dlg_log->ShowWindow( SW_HIDE );

	// mark merges
	m_Doc->m_mlist->mark0();
	// mode
	SetCursorMode( CUR_NONE_SELECTED );
	// reset mem_sel_count
	OnInfoBoxMess(0,0,NULL);
}

//===============================================================================================
// attempt to reselect area corner based on position
// should be used after areas are modified
void CFreePcbView::TryToReselectAreaCorner( int x, int y )
{
	m_dlist->CancelHighLight();
	for( int ia=0; ia<m_sel_net->nareas; ia++ )
	{
		for( int ic=0; ic<m_sel_net->area[ia].poly->GetNumCorners(); ic++ )
		{
			if( x == m_sel_net->area[ia].poly->GetX(ic)
				&& y == m_sel_net->area[ia].poly->GetY(ic) )
			{
				// found matching corner
				m_sel_id = id(ID_NET,ID_AREA,ia,ID_CORNER,ic);
				SetCursorMode( CUR_AREA_CORNER_SELECTED );
				m_Doc->m_nlist->HighlightAreaCorner( m_sel_net, ia, ic );
				return;
			}
		}
	}
	CancelSelection();
}
//===============================================================================================
// set trace width using dialog
// enter with:
//	mode = 0 if called with segment selected
//	mode = 1 if called with connection selected
//	mode = 2 if called with net selected
//
int CFreePcbView::SetWidth( int mode )
{
	if ( m_sel_flags != CONNECT_ONLY )
		return 0;
	if( m_sel_id.sst != ID_SEG )
	{
		CDlgVia dlg;
		dlg.Initialize( m_Doc->m_via_w, m_Doc->m_via_hole_w, &m_Doc->m_v_w, &m_Doc->m_v_h_w, m_sel_vtx.via_w, m_sel_vtx.via_hole_w, m_Doc->m_units );
		int ret = dlg.DoModal();
		if( ret == IDOK )
		{
			for( cnet * m_net=m_Doc->m_nlist->GetFirstNet(); m_net; m_net=m_Doc->m_nlist->GetNextNet(/*LABEL*/) )
			{
				for( int i=0; i<m_net->nconnects; i++ )
				{
					if( m_net->connect[i].m_selected )
					{
						for( int ii=1; ii<=m_net->connect[i].nsegs; ii++ )
						{
							if( m_net->connect[i].vtx[ii].selected )
							{
								if( ii < m_net->connect[i].nsegs || m_net->connect[i].end_pin == cconnect::NO_END )
								{
									m_net->connect[i].vtx[ii].via_w = dlg.m_via_w;
									m_net->connect[i].vtx[ii].via_hole_w = dlg.m_via_hole_w;
									m_dlist->CancelHighLight();
									m_Doc->m_nlist->DrawVia( m_net, i, ii );
									m_Doc->m_nlist->HighlightVertex( m_net, i, ii );
								}
							}
						}
					}
				}
			}
		}
		return 0;
	}
	// set parameters for dialog
	int w_seg=m_sel_seg.width, vw=0, vh=0;
	GetWidthsForSegment( &w_seg, &vw, &vh );
	DlgSetSegmentWidth dlg;
	dlg.m_w = &m_Doc->m_w;
	dlg.m_v_w = &m_Doc->m_v_w;
	dlg.m_v_h_w = &m_Doc->m_v_h_w;
	dlg.m_init_w = w_seg;
	dlg.m_init_via_w = vw;
	dlg.m_init_via_hole_w = vh;
	dlg.m_units = m_Doc->m_units;
	if( mode == 0 )
	{
		cseg * seg = &m_sel_seg;
		cconnect * con = &m_sel_con;
		int seg_w = seg->width;
		if( seg_w )
			dlg.m_init_w = seg_w;
		else if( m_sel_net->def_w )
			dlg.m_init_w = m_sel_net->def_w;
	}
	else
	{
		if( m_sel_net->def_w )
			dlg.m_init_w = m_sel_net->def_w;
	}

	// launch dialog
	dlg.m_mode = mode;
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		// returned with "OK"
		int w = dlg.m_width;
		int via_w = vw;
		int via_hole_w = vh;
		GetWidthsForSegment( &w, &via_w, &via_hole_w );
		if( dlg.m_via_width )
			via_w = dlg.m_via_width;
		if( dlg.m_hole_width )
			via_hole_w = dlg.m_hole_width;
		if( dlg.m_tv == 3 )
			w = 0;
		else if( dlg.m_tv == 2 )
			via_w = 0;

		// SaveUndoInfoForNetAndConnections
		SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );

		// set default values for net or connection
		if( dlg.m_def == 2 )
		{
			// set default for net
			if( w )
				m_sel_net->def_w = w;
			if( via_w )
			{
				m_sel_net->def_via_w = via_w;
				m_sel_net->def_via_hole_w = via_hole_w;
			}
		}
		if ( m_sel_count == 1 )
		{
			// apply new widths to net, connection or segment
			if( dlg.m_apply == 3 )
			{
				// apply width to net
				m_Doc->m_nlist->SetNetWidth( m_sel_net, w, via_w, via_hole_w );
			}
			else if( dlg.m_apply == 2 )
			{
				// apply width to connection
				m_Doc->m_nlist->SetConnectionWidth( m_sel_net, m_sel_ic, w, via_w, via_hole_w );
			}
			else if( dlg.m_apply == 1 )
			{
				// apply width to segment
				m_Doc->m_nlist->SetSegmentWidth( m_sel_net, m_sel_ic,
					m_sel_id.ii, w, via_w, via_hole_w );
			}
		}
		else if ( m_sel_count > 1 )
		{
			if( getbit(m_sel_flags, FLAG_SEL_NET) )
				for(cnet* n=m_Doc->m_nlist->GetFirstNet(); n; n=m_Doc->m_nlist->GetNextNet(/*LABEL*/))
					if( n->selected )
					{
						if( dlg.m_apply == 3 )
						{
							// apply width to net
							m_Doc->m_nlist->SetNetWidth( n, w, via_w, via_hole_w );
							continue;
						}
						if( getbit( m_sel_flags, FLAG_SEL_CONNECT ) )
							for (int i=0; i<n->nconnects; i++)
								if( n->connect[i].m_selected )
								{
									if( dlg.m_apply == 2 )
									{
										// apply width to connection
										m_Doc->m_nlist->SetConnectionWidth( n, i, w, via_w, via_hole_w );
										continue;
									}
									else if( dlg.m_apply == 1 )
									{
										// apply width to segment
										for( int ii=0; ii< n->connect[i].nsegs; ii++ )
											if( n->connect[i].seg[ii].selected )
												m_Doc->m_nlist->SetSegmentWidth( n, i, ii, w, via_w, via_hole_w );
									}

								}
					}	
		}
	}
	m_Doc->ProjectModified( TRUE );
	return 0;
}
//===============================================================================================
// Get trace and via widths
// tries default widths for net, then board
//===============================================================================================
int CFreePcbView::GetWidthsForSegment( int * w, int * via_w, int * via_hole_w, int EX )
{
	*via_w = abs(*via_w);
	*via_hole_w = abs(*via_hole_w);
	int save_v = *via_w;
	int save_h = *via_hole_w;
	int w_excl=0, h_excl=0;
	//reset
	*via_w = 0;
	// get seg w
	int seg_w = m_sel_net->def_w;
	if( !seg_w )
		seg_w = abs(m_Doc->m_trace_w);
	int iver = -1, excl = EX;

	// via
	if( m_sel_net->def_via_w && excl == -1 )
		*via_w = m_sel_net->def_via_w;
	else if ( *w == 0 )
		*via_w = abs(m_Doc->m_via_w);

	// hole
	if( m_sel_net->def_via_hole_w && excl == -1 )
		*via_hole_w = m_sel_net->def_via_hole_w;
	else if ( *w == 0 )
		*via_hole_w = abs(m_Doc->m_via_hole_w);

	// when *via_w == 0
	if( *via_w == 0 ) 
	{
		if( excl >= 0 && excl < m_Doc->m_v_h_w.GetSize() )
		{
			w_excl = abs(m_Doc->m_v_w[excl]);
			h_excl = abs(m_Doc->m_v_h_w[excl]);
			iver = excl;
		}
		int sz = m_Doc->m_w.GetSize();
		int i=excl+1;
		if( sz ) for ( int count=0; count<=sz; count++)
		{
			if( i >= sz )
				i = 0;
			if( excl == -1 )
			{
				if( *w == abs(m_Doc->m_w[i]) )
				{
					*via_w = abs(m_Doc->m_v_w[i]);
					*via_hole_w = abs(m_Doc->m_v_h_w[i]);
					iver = i;
					break; 
				}
			}
			else
			{
				if( w_excl != abs(m_Doc->m_v_w[i]) || h_excl != abs(m_Doc->m_v_h_w[i]) )
				{
					*via_w = abs(m_Doc->m_v_w[i]);
					*via_hole_w = abs(m_Doc->m_v_h_w[i]);
					iver = i;
					break; 
				}
			}
			i++;
		}
	}

	// iver default
	if( iver == -1 )
		iver = 0;

	// default values
	if( *w == 0 )
		*w = seg_w;
	if( *via_w == 0 )
		*via_w = abs(m_Doc->m_via_w);
	if( *via_hole_w == 0 )
		*via_hole_w = abs(m_Doc->m_via_hole_w);

	return iver;
}



void CFreePcbView::OnProjectSelectArcElements()
{
	for( cnet * n=m_Doc->m_nlist->GetFirstNet(); n; n = m_Doc->m_nlist->GetNextNet(/*LABEL*/) )
	{
		if( m_sel_mask & (1<<SEL_MASK_CON ) )
			for( int icc=0; icc<n->nconnects; icc++ )
			{
				cconnect * c = &n->connect[icc];		
				int iss = 0;
				int i_arc;
				do{
					i_arc = FindArcElements(c,&iss);
					for( int i_sel=0; i_sel<i_arc; i_sel++ )
					{
						id sid(ID_NET, ID_CONNECT, icc, ID_SEG, i_sel+iss);
						if( m_Doc->m_vis[c->seg[i_sel+iss].layer] )
							NewSelect(n,&sid,0,0);
					}
					iss++;
				}while( i_arc );
			}
		if( m_sel_mask & (1<<SEL_MASK_AREAS ) )
			for( int iar=0; iar<n->nareas; iar++ )
			{
				carea * a = &n->area[iar];
				CPolyLine * p = a->poly;
				if( m_Doc->m_vis[p->GetLayer()] == 0 )
					continue;
				for( int icont=p->GetNumContours()-1; icont>=0; icont-- )
				{
					int Cstart = p->GetContourStart(icont);
					int i = Cstart;
					int i_arc;
					BOOL bEnd;
					do{
						i_arc = FindArcElements(p,&i,&bEnd);
						for( int i_sel=0; i_sel<i_arc; i_sel++ )
						{
							id sid(ID_NET, ID_AREA, iar, ID_SIDE, i_sel+i);
							NewSelect(n,&sid,0,0);
						}
						i++;
					}while( i_arc && !bEnd );
				}
			}
	}
	if( m_sel_count )
	{
		HighlightGroup();
		SetCursorMode(CUR_GROUP_SELECTED);
	}
	Invalidate( FALSE );// arc el's
}

void CFreePcbView::OnProjectSelectViaElements()
{
	for( cnet * n=m_Doc->m_nlist->GetFirstNet(); n; n = m_Doc->m_nlist->GetNextNet(/*LABEL*/) )
	{
		for( int icc=0; icc<n->nconnects; icc++ )
		{
			cconnect * c = &n->connect[icc];
			for( int iss=1; iss<c->nsegs; iss++ )
			{
				CPoint pv,v,nv,nnv;
				pv.x = c->vtx[iss-1].x;
				pv.y = c->vtx[iss-1].y;
				v.x = c->vtx[iss].x;
				v.y = c->vtx[iss].y;
				nv.x = c->vtx[iss+1].x;
				nv.y = c->vtx[iss+1].y;
				int l1 = Distance(v.x,v.y,pv.x,pv.y);
				int l2 = Distance(nv.x,nv.y,v.x,v.y);
				float an1 = Angle(v.x,v.y,pv.x,pv.y);
				float an2 = Angle(nv.x,nv.y,v.x,v.y);
				float d_an = an2 - an1;
				if( d_an < -180.0 )
					d_an += 360.0;
				else if( d_an > 180.0 )
					d_an -= 360.0;
				if( d_an > 94.0 || d_an < -94.0 )
					if( !c->vtx[iss].via_w )
						if( !c->vtx[iss].tee_ID )
						{
							if( l2/10 < l1 && !c->vtx[iss+1].via_w )
							{
								id sid(ID_NET, ID_CONNECT, icc, ID_SEG, iss);
								if( !m_sel_count )
									NewSelect(n,&sid,1,0);
								else
									NewSelect(n,&sid,0,0);
							}
							if( l1/10 < l2 && !c->vtx[iss-1].via_w )
							{
								id sid(ID_NET, ID_CONNECT, icc, ID_SEG, iss-1);
								if( !m_sel_count )
									NewSelect(n,&sid,1,0);
								else
									NewSelect(n,&sid,0,0);
							}
						}
			}
		}
	}
	if( m_sel_count > 1 )
	{
		HighlightGroup();
		SetCursorMode(CUR_GROUP_SELECTED);
	}
	Invalidate( FALSE );// via el's
}

void CFreePcbView::ProjectRunInfoBox()
{
#define BUFSIZE	_2540/10
	CString command_str = m_Doc->m_app_dir + "\\FPC_EXE\\FreePCB_InfoBox\\ObjectManager.exe";
	m_Doc->m_dlg_log->AddLine( "Run: " + command_str + "\r\n" ); 
	if( /*FindWindow(NULL,"InfoBox") == NULL &&*/ !m_Doc->m_i_b )
	{
		char buffer[(int)BUFSIZE];
		int ret = GetFullPathName( command_str, BUFSIZE, buffer, NULL );
		if (ret)
		{
			//m_Doc->m_dlg_log->ShowWindow( SW_SHOW );
			Invalidate(FALSE);
			if ((UINT)ShellExecute(	NULL,"open",command_str,
									NULL,buffer,SW_SHOWNORMAL) <=32)
				m_Doc->m_dlg_log->AddLine( "Failed!\n" );
			else
			{
				m_Doc->m_dlg_log->AddLine( "Successfully!\n" );
				m_Doc->m_dlg_log->ShowWindow( SW_HIDE );
			}
			m_Doc->m_dlg_log->AddLine( "\r\n" ); 			
			//m_Doc->m_dlg_log->UpdateWindow();
			m_Doc->m_i_b = 1;
			//Invalidate(FALSE);
		}
	}
#undef BUFSIZE
}
void CFreePcbView::OnProjectRunInfoBox()
{
	m_Doc->m_i_b = 0;
	ProjectRunInfoBox();
}

void CFreePcbView::OnSetOriginToSelectedItem()
{
	SetOriginToSelectedItem();
	Invalidate( FALSE );
}
void CFreePcbView::SetOriginToSelectedItem()
{
	int x=0,y=0;
	if( m_cursor_mode == CUR_PART_SELECTED )
	{
		x = m_sel_part->x;
		y = m_sel_part->y;
	}
	else if( m_cursor_mode == CUR_PAD_SELECTED )
	{
		x = m_sel_part->pin [m_sel_id.i].x;
		y = m_sel_part->pin [m_sel_id.i].y;
	}
	else if( m_cursor_mode == CUR_OP_CORNER_SELECTED )
	{
		x = m_Doc->m_outline_poly[m_sel_id.i].GetX(m_sel_id.ii);
		y = m_Doc->m_outline_poly[m_sel_id.i].GetY(m_sel_id.ii);		
	}
	else if( m_cursor_mode == CUR_AREA_CORNER_SELECTED )
	{
		x = m_sel_net->area[m_sel_ia].poly->GetX(m_sel_is);
		y = m_sel_net->area[m_sel_ia].poly->GetY(m_sel_is);
	}
	else if( m_cursor_mode == CUR_VTX_SELECTED )
	{
		x = m_sel_vtx.x;
		y = m_sel_vtx.y;
	}
	else if( m_cursor_mode == CUR_END_VTX_SELECTED )
	{
		x = m_sel_vtx.x;
		y = m_sel_vtx.y;
	}
	SaveUndoInfoForMoveOrigin( -x, -y, m_Doc->m_undo_list );
	MoveOrigin( -x, -y );
	m_org_x -= x;
	m_org_y -= y;
	CRect screen_r;
	GetWindowRect( &screen_r );
	m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel, m_org_x, m_org_y );
	CancelSelection();
	SetCursorMode(CUR_NONE_SELECTED);
	m_Doc->ProjectModified( TRUE );
}

//===============================================================================================
//------------------ context-sensitive menu invoked by right-click ------------------------------
//===============================================================================================
void CFreePcbView::OnContextMenu(CWnd* pWnd, CPoint point )
{
	if( m_disable_context_menu )
	{
		// right-click already handled, don't pop up menu
		m_disable_context_menu = 0;
		return;
	}
	if( !m_Doc->m_project_open )	// no project open
		return;

	// OK, pop-up context menu
	CMenu menu;
	VERIFY(menu.LoadMenu(IDR_CONTEXT));
	CMenu* pPopup;
	int style;
	switch( m_cursor_mode )
	{
	case CUR_NONE_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_NONE);
		ASSERT(pPopup != NULL);
		if( m_Doc->m_dlist->Get_Selected()==0 )
			pPopup->EnableMenuItem( ID_CANCEL_HIGHLIGHT, MF_GRAYED );
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_OP_CORNER_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_OP_CORNER);
		ASSERT(pPopup != NULL);
		if( m_Doc->m_outline_poly[m_sel_id.i].GetNumCorners() < 4 )
				pPopup->EnableMenuItem( ID_OP_CORNER_DELETECORNER, MF_GRAYED );
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_OP_SIDE_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_OP_SIDE);
		ASSERT(pPopup != NULL);
		style = m_Doc->m_outline_poly[m_sel_id.i].GetSideStyle( m_sel_id.ii );
		if( style == CPolyLine::STRAIGHT )
		{
			int xi = m_Doc->m_outline_poly[m_sel_id.i].GetX( m_sel_id.ii );
			int yi = m_Doc->m_outline_poly[m_sel_id.i].GetY( m_sel_id.ii );
			int xf, yf;
			if( m_sel_id.ii != (m_Doc->m_outline_poly[m_sel_id.i].GetNumCorners()-1) )
			{
				xf = m_Doc->m_outline_poly[m_sel_id.i].GetX( m_sel_id.ii+1 );
				yf = m_Doc->m_outline_poly[m_sel_id.i].GetY( m_sel_id.ii+1 );
			}
			else
			{
				xf = m_Doc->m_outline_poly[m_sel_id.i].GetX( 0 );
				yf = m_Doc->m_outline_poly[m_sel_id.i].GetY( 0 );
			}
			if( xi == xf || yi == yf )
			{
				pPopup->EnableMenuItem( ID_OP_SIDE_CONVERTTOARC_CW, MF_GRAYED );
				pPopup->EnableMenuItem( ID_OP_SIDE_CONVERTTOARC_CCW, MF_GRAYED );
			}
			pPopup->EnableMenuItem( ID_OP_SIDE_CONVERTTOSTRAIGHTLINE, MF_GRAYED );
		}
		else if( style == CPolyLine::ARC_CW )
		{
			pPopup->EnableMenuItem( ID_OP_SIDE_CONVERTTOARC_CW, MF_GRAYED );
			pPopup->EnableMenuItem( ID_OP_SIDE_INSERTCORNER, MF_GRAYED );
		}
		else if( style == CPolyLine::ARC_CCW )
		{
			pPopup->EnableMenuItem( ID_OP_SIDE_CONVERTTOARC_CCW, MF_GRAYED );
			pPopup->EnableMenuItem( ID_OP_SIDE_INSERTCORNER, MF_GRAYED );
		}
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_PART_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_PART);
		ASSERT(pPopup != NULL);
		if( m_sel_part->glued )
			pPopup->EnableMenuItem( ID_PART_GLUE, MF_GRAYED );
		else
			pPopup->EnableMenuItem( ID_PART_UNGLUE, MF_GRAYED );
		if( m_sel_part->m_merge == -1 )
			pPopup->EnableMenuItem( ID_REMOVEMERGE, MF_GRAYED );
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_REF_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_REF_TEXT);
		ASSERT(pPopup != NULL);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_VALUE_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_VALUE_TEXT);
		ASSERT(pPopup != NULL);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_PAD_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_PAD);
		ASSERT(pPopup != NULL);
		if( m_sel_part->pin[m_sel_id.i].net )
			pPopup->EnableMenuItem( ID_PAD_ADDTONET, MF_GRAYED );
		else
			pPopup->EnableMenuItem( ID_PAD_DETACHFROMNET, MF_GRAYED );
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_SEG_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_SEGMENT);
		ASSERT(pPopup != NULL);

		if(!SegmentMovable())
				pPopup->EnableMenuItem( ID_SEGMENT_MOVE, MF_GRAYED );

		if( m_sel_con.end_pin == cconnect::NO_END
			&& m_sel_con.vtx[m_sel_con.nsegs].tee_ID == 0
			&& m_sel_con.vtx[m_sel_con.nsegs].force_via_flag == 0
			)
		{
			pPopup->EnableMenuItem( ID_SEGMENT_UNROUTETRACE, MF_GRAYED );
		}
		if( m_sel_con.end_pin == cconnect::NO_END
			&& m_sel_con.nsegs == (m_sel_id.ii+1)
			&& m_sel_con.vtx[m_sel_con.nsegs].tee_ID == 0
			&& m_sel_con.vtx[m_sel_con.nsegs].force_via_flag == 0
			)
		{
			// last segment of stub trace unless a tee or via
			pPopup->EnableMenuItem( ID_SEGMENT_UNROUTE, MF_GRAYED );
		}
		if( m_sel_con.end_pin != cconnect::NO_END
			|| m_sel_con.nsegs > (m_sel_id.ii+1)
			)
		{
			pPopup->EnableMenuItem( ID_SEGMENT_DELETE, MF_GRAYED );
		}
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_RAT_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_RATLINE);
		ASSERT(pPopup != NULL);
		if( m_sel_con.locked )
			pPopup->EnableMenuItem( ID_RATLINE_LOCKCONNECTION, MF_GRAYED );
		else
			pPopup->EnableMenuItem( ID_RATLINE_UNLOCKCONNECTION, MF_GRAYED );
		if( m_sel_con.end_pin == cconnect::NO_END )
			pPopup->EnableMenuItem( ID_SEGMENT_UNROUTETRACE, MF_GRAYED );
		if( m_sel_con.nsegs == 1
			|| !(m_sel_id.ii == 0 || m_sel_id.ii == (m_sel_con.nsegs-1) ) )
			pPopup->EnableMenuItem( ID_RATLINE_CHANGEPIN, MF_GRAYED );
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_VTX_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_VERTEX);
		ASSERT(pPopup != NULL);
		if( m_sel_vtx.via_w == 0 )
			pPopup->EnableMenuItem( ID_VERTEX_SETSIZE, MF_GRAYED );
		if( m_sel_con.end_pin == cconnect::NO_END
			&& m_sel_con.vtx[m_sel_con.nsegs].tee_ID == 0
			&& m_sel_con.vtx[m_sel_con.nsegs].force_via_flag == 0
			)
		{
			pPopup->EnableMenuItem( ID_VERTEX_UNROUTETRACE, MF_GRAYED );
		}
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_END_VTX_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_END_VERTEX);
		ASSERT(pPopup != NULL);
		if( m_sel_vtx.via_w )
			pPopup->EnableMenuItem( ID_ENDVERTEX_ADDVIA, MF_GRAYED );
		else
		{
			pPopup->EnableMenuItem( ID_ENDVERTEX_SETSIZE, MF_GRAYED );
			pPopup->EnableMenuItem( ID_ENDVERTEX_REMOVEVIA, MF_GRAYED );
		}
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_CONNECT_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_CONNECT);
		ASSERT(pPopup != NULL);
		if( m_sel_con.end_pin == cconnect::NO_END )
			pPopup->EnableMenuItem( ID_CONNECT_UNROUTETRACE, MF_GRAYED );
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_NET_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_NET);
		ASSERT(pPopup != NULL);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_TEXT_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_TEXT);
		ASSERT(pPopup != NULL);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_AREA_CORNER_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_AREA_CORNER);
		ASSERT(pPopup != NULL);
		{
			carea * area = &m_sel_net->area[m_sel_ia];
			if( area->poly->GetContour( m_sel_id.ii ) == 0 )
				pPopup->EnableMenuItem( ID_AREACORNER_DELETECUTOUT, MF_GRAYED );
			else
				pPopup->EnableMenuItem( ID_AREACORNER_ADDCUTOUT, MF_GRAYED );
		}
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_AREA_SIDE_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_AREA_EDGE);
		ASSERT(pPopup != NULL);
		{
			carea * area = &m_sel_net->area[m_sel_ia];
			if( area->poly->GetContour( m_sel_id.ii ) == 0 )
				pPopup->EnableMenuItem( ID_AREAEDGE_DELETECUTOUT, MF_GRAYED );
		}
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_GROUP_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_GROUP);
		ASSERT(pPopup != NULL);
		if( m_sel_flags != PART_ONLY )
			pPopup->EnableMenuItem( ID_ALIGN_PARTS, MF_GRAYED );
		if( getbit( m_sel_flags, FLAG_SEL_PART ) )
			pPopup->EnableMenuItem( ID_APPROXIMATION_ARC, MF_GRAYED );
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;
	}
}
//===============================================================================================
// add copper area
//
void CFreePcbView::OnAddArea()
{
	AddArea();
	Invalidate( FALSE );//OnAddArea
}
void CFreePcbView::AddArea()
{
	CDlgAddArea dlg;
	dlg.Initialize( m_Doc->m_nlist, m_Doc->m_num_layers, NULL, m_active_layer, -1, 0, m_Doc->m_units, FALSE);
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		if( !dlg.m_net )
		{
			CString str;
			str.Format( "Net \"%s\" not found", dlg.m_net_name );
			AfxMessageBox( str, MB_OK );
		}
		else
		{
			CDC *pDC = GetDC();
			pDC->SelectClipRgn( &m_pcb_rgn );
			SetDCToWorldCoords( pDC );
			m_dlist->CancelHighLight();
			SetCursorMode( CUR_ADD_AREA );
			// make layer visible
			m_active_layer = dlg.m_layer;
			m_Doc->m_vis[m_active_layer] = TRUE;
			m_dlist->SetLayerVisible( m_active_layer, TRUE );
			ShowActiveLayer(m_Doc->m_num_copper_layers);
			m_sel_net = dlg.m_net;
			m_sel_id.Clear();
			m_sel_ia = INT_MAX;
			m_dlist->StartDraggingArray( pDC, m_last_cursor_point.x,
				m_last_cursor_point.y, 2 );
			m_polyline_style = CPolyLine::STRAIGHT;
			m_polyline_hatch = dlg.m_hatch;
			m_polyline_width = 0;
			m_Doc->m_plist->HighlightAllPadsOnNet( dlg.m_net, 2, 0 );
			ReleaseDC( pDC );
		}
	}
}
//===============================================================================================
// add copper area cutout
//
void CFreePcbView::AreaAddCutout()
{
	// check if any non-straight sides
	BOOL bArcs = FALSE;
	CPolyLine * poly = m_sel_net->area[m_sel_ia].poly;
	int ns = poly->GetNumCorners();
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	m_dlist->CancelHighLight();
	SetCursorMode( CUR_ADD_AREA_CUTOUT );
	// make layer visible
	m_active_layer = m_sel_net->area[m_sel_ia].poly->GetLayer();
	m_Doc->m_vis[m_active_layer] = TRUE;
	m_dlist->SetLayerVisible( m_active_layer, TRUE );
	ShowActiveLayer(m_Doc->m_num_copper_layers);
	m_Doc->m_nlist->HighlightAreaSides( m_sel_net, m_sel_ia, 0 );
	m_dlist->StartDraggingArray( pDC, m_last_cursor_point.x,
		m_last_cursor_point.y, 2 );
	m_polyline_style = CPolyLine::STRAIGHT;
	ReleaseDC( pDC );
}
void CFreePcbView::OnAreaAddCutout()
{
	AreaAddCutout();
	Invalidate( FALSE );
}
//===============================================================================================
void CFreePcbView::AreaDeleteCutout()
{
	CPolyLine * poly = m_sel_net->area[m_sel_ia].poly;
	int icont = poly->GetContour( m_sel_id.ii );
	if( icont < 1 )
		ASSERT(0);
	SaveUndoInfoForArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_MODIFY, TRUE, m_Doc->m_undo_list );
	poly->RemoveContour( icont );
	m_Doc->m_nlist->SetAreaConnections( m_sel_net, m_sel_ia );
	m_Doc->m_nlist->OptimizeConnections(  m_sel_net, -1, m_Doc->m_auto_ratline_disable,
														m_Doc->m_auto_ratline_min_pins, TRUE  );
	CancelSelection();
	m_Doc->ProjectModified( TRUE );
}
void CFreePcbView::OnAreaDeleteCutout()
{
	AreaDeleteCutout();
	Invalidate( FALSE );
}
//===============================================================================================
// move part
//
void CFreePcbView::OnPartMove()
{
	// check for glue
	if( m_sel_part->glued )
	{
		int ret = AfxMessageBox( "This part is glued, do you want to unglue it ?  ", MB_YESNO );
		if( ret != IDYES )
			return;
	}
	// save undo info for part and attached nets
	if( !m_dragging_new_item )
		SaveUndoInfoForPartAndNets( m_sel_part,
		CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
	// drag part
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	// move cursor to part origin
	CPoint p;
	p.x  = m_sel_part->x;
	p.y  = m_sel_part->y;
	m_from_pt = p;
	CPoint cur_p = m_dlist->PCBToScreen( p );
	SetCursorPos( cur_p.x, cur_p.y );
	// start dragging
	m_Doc->m_dlist->CancelHighLight();
	BOOL bRatlines = m_Doc->m_vis[LAY_RAT_LINE];
	m_Doc->m_plist->StartDraggingPart( pDC, m_sel_part, bRatlines, 
					m_Doc->m_auto_ratline_disable, m_Doc->m_auto_ratline_min_pins );
	SetCursorMode( CUR_DRAG_PART );
	ReleaseDC( pDC );
}
//===============================================================================================
// add text string
//
void CFreePcbView::OnTextAdd()
{
	// create, initialize and show dialog
	CDlgAddText add_text_dlg;
	CString str = "";
	add_text_dlg.Initialize( 0, m_Doc->m_num_layers, 1, &str, m_Doc->m_units,
			LAY_SILK_TOP, 0, 0, 0, 0, 0, 0 );
	add_text_dlg.m_num_layers = m_Doc->m_num_layers;
	add_text_dlg.m_bDrag = 1;
	// defaults for dialog
	int ret = add_text_dlg.DoModal();
	if( ret == IDCANCEL )
		return;
	else
	{
		int x = add_text_dlg.m_x;
		int y = add_text_dlg.m_y;
		int mirror = add_text_dlg.m_bMirror;
		BOOL bNegative = add_text_dlg.m_bNegative;
		int angle = 0;
		int font_size = add_text_dlg.m_height;
		int stroke_width = add_text_dlg.m_width;
		int layer = add_text_dlg.m_layer;
		CString str = add_text_dlg.m_str;
		m_Doc->m_vis[layer] = TRUE;
		m_dlist->SetLayerVisible( layer, TRUE );

		// get cursor position and convert to PCB coords
		CPoint p;
		GetCursorPos( &p );		// cursor pos in screen coords
		p = m_dlist->ScreenToPCB( p );	// convert to PCB coords
		// set pDC to PCB coords
		CDC *pDC = GetDC();
		pDC->SelectClipRgn( &m_pcb_rgn );
		SetDCToWorldCoords( pDC );
		if( add_text_dlg.m_bDrag )
		{
			m_sel_text = m_Doc->m_tlist->AddText( p.x, p.y, angle, mirror, bNegative,
				layer, font_size, stroke_width, &str );
			m_dragging_new_item = 1;
			m_Doc->m_tlist->StartDraggingText( pDC, m_sel_text );
			SetCursorMode( CUR_DRAG_TEXT );
		}
		else
		{
			m_sel_text = m_Doc->m_tlist->AddText( x, y, angle, mirror, bNegative,
				layer, font_size,  stroke_width, &str );
			SaveUndoInfoForText( m_sel_text, CTextList::UNDO_TEXT_ADD, TRUE, m_Doc->m_undo_list );
			m_Doc->m_tlist->HighlightText( m_sel_text );
		}
		ReleaseDC( pDC );
	}
}
//===============================================================================================
// delete text ... enter with text selected
//
void CFreePcbView::OnTextDelete()
{
	SaveUndoInfoForText( m_sel_text, CTextList::UNDO_TEXT_DELETE, TRUE, m_Doc->m_undo_list );
	m_Doc->m_tlist->RemoveText( m_sel_text );
	CancelSelection();
	m_Doc->ProjectModified( TRUE );
}
//===============================================================================================
// move text, enter with text selected
//
void CFreePcbView::OnTextMove()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	// move cursor to text origin
	CPoint p;
	p.x  = m_sel_text->m_x;
	p.y  = m_sel_text->m_y;
	CPoint cur_p = m_dlist->PCBToScreen( p );
	SetCursorPos( cur_p.x, cur_p.y );
	// start moving
	m_dlist->CancelHighLight();
	m_dragging_new_item = 0;
	m_Doc->m_tlist->StartDraggingText( pDC, m_sel_text );
	SetCursorMode( CUR_DRAG_TEXT );
	ReleaseDC( pDC );
}
//===============================================================================================
// glue part
//
void CFreePcbView::OnPartGlue()
{
	SaveUndoInfoForPart( m_sel_part, CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
	m_sel_part->glued = 1;
	SetFKText( m_cursor_mode );
	m_Doc->ProjectModified( TRUE );
}
//===============================================================================================
// unglue part
//
void CFreePcbView::OnPartUnglue()
{
	if( AfxMessageBox( " Unglue part?", MB_YESNO ) == IDYES )
	{
		SaveUndoInfoForPart( m_sel_part, CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
		m_sel_part->glued = 0;
		SetFKText( m_cursor_mode );
		m_Doc->ProjectModified( TRUE );
	}
}
//===============================================================================================
//-------------------------------------- delete part --------------------------------------------
//===============================================================================================
void CFreePcbView::DeletePart()
{
	DeleteGroup(0);
	CancelSelection();
	m_Doc->ProjectModified( TRUE );
}
void CFreePcbView::OnPartDelete()
{
	DeletePart();
	Invalidate( FALSE );//OnPartDelete
}
//===============================================================================================
//-------------------------------- optimize all nets to part ------------------------------------
//===============================================================================================
void CFreePcbView::OnPartOptimize()
{
	SaveUndoInfoForPartAndNets( m_sel_part,
		CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
	m_Doc->m_nlist->OptimizeConnections( m_sel_part, FALSE, -1 );
	m_Doc->ProjectModified( TRUE );
}
//===============================================================================================
//---------------------------------- OnPartRemoveMerge ------------------------------------------
//===============================================================================================
void CFreePcbView::OnPartRemoveMerge()
{
	if( m_sel_id.type == ID_PART )
	{
		m_sel_part->m_merge = -1;
		for( cnet * n = m_Doc->m_nlist->GetFirstNet(); n; n = m_Doc->m_nlist->GetNextNet(/*LABEL*/) )
		{
			for( int ic=0; ic<n->nconnects; ic++ )
			{
				if( n->pin[n->connect[ic].start_pin].part == m_sel_part )
					n->connect[ic].m_merge = -1;
				if( n->connect[ic].end_pin >= 0 )
					if( n->pin[n->connect[ic].end_pin].part == m_sel_part )
						n->connect[ic].m_merge = -1;
			}
		}
		m_Doc->m_nlist->SetAreaConnections(m_sel_part);	
	}
	else if( m_sel_id.type == ID_NET && m_sel_id.st == ID_AREA )
	{
		m_sel_net->area[m_sel_id.i].poly->SetMerge(-1);
		m_Doc->m_nlist->SetAreaConnections(m_sel_net,m_sel_id.i);
		if( m_sel_net->area[m_sel_id.i].poly->GetMerge() >= 0 )
		{
			AfxMessageBox("This polygon cannot be detached from the merge, \
since all pins inside this polygon belong to this merge. \
You can disconnect only the polygon that is connected independently.");
		}
	}
	else if( m_sel_id.type == ID_TEXT && m_sel_text )
		m_sel_text->m_merge = -1;
	else if( m_sel_id.type == ID_POLYLINE )
		m_Doc->m_outline_poly[m_sel_id.i].SetMerge(-1);
	CancelSelection();
}
//===============================================================================================
//---------------------------------- OnSelectSimilarParts ---------------------------------------
//===============================================================================================
void CFreePcbView::OnSelectSimilarParts()
{
	SelectSimilarParts(1);
	Invalidate(FALSE);
}
//===============================================================================================
//---------------------------------- OnSelectSimilarPackages ------------------------------------
//===============================================================================================
void CFreePcbView::OnSelectSimilarPackages()
{
	SelectSimilarParts(0);
	Invalidate(FALSE);
}
//===============================================================================================
//--------------------------- move ref. designator text for part --------------------------------
//===============================================================================================
void CFreePcbView::OnRefMove()
{
	// move reference ID
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	// move cursor to part origin
	CPoint cur_p = m_dlist->PCBToScreen( m_last_cursor_point );
	SetCursorPos( cur_p.x, cur_p.y );
	m_dragging_new_item = 0;
	m_Doc->m_plist->StartDraggingRefText( pDC, m_sel_part );
	SetCursorMode( CUR_DRAG_REF );
	ReleaseDC( pDC );
}
//===============================================================================================
//----------------------------- optimize net for this pad ---------------------------------------
//===============================================================================================
void CFreePcbView::OnPadOptimize()
{
	cnet * pin_net = (cnet*)m_sel_part->pin[m_sel_id.i].net;
	if( pin_net )
	{
		m_Doc->m_nlist->OptimizeConnections( pin_net, -1, FALSE, -1, FALSE );
		m_Doc->ProjectModified( TRUE );
	}
}
//===============================================================================================
//---------------------------- start stub trace from this pad -----------------------------------
//===============================================================================================
void CFreePcbView::OnPadStartStubTrace()
{	
	cnet * net = (cnet*)m_sel_part->pin[m_sel_id.i].net;
	if( net == NULL )
	{
		OnPadAddToNet();
	}
	net = (cnet*)m_sel_part->pin[m_sel_id.i].net;
	if( net == NULL )
		return;
	OnInfoBoxSendMess( "part_list ");
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint pi = m_last_cursor_point;
	CString pin_name = m_sel_part->shape->GetPinNameByIndex( m_sel_id.i );
	CPoint p = m_Doc->m_plist->GetPinPoint( m_sel_part, m_sel_id.i, m_sel_part->side, m_sel_part->angle );
	int pMerge = m_sel_part->m_merge;

	// force to layer of pad if SMT
	int PL = m_Doc->m_plist->GetPinLayer( m_sel_part, m_sel_id.i );
	if( PL != LAY_PAD_THRU )
	{
		m_active_layer = PL;
		ShowActiveLayer(m_Doc->m_num_copper_layers);
	}

	// find starting pin in net
	int p1 = m_Doc->m_nlist->GetNetPinIndex( net, &m_sel_part->ref_des,  &pin_name );
	if( p1 == -1 )
		ASSERT(0);		// starting pin not found in net

	// add connection for stub trace
	SaveUndoInfoForNetAndConnections( net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
	m_sel_net = net;
	m_sel_id.Set( ID_NET, ID_CONNECT, 0, ID_SEG, 0 );
	m_sel_id.i = m_Doc->m_nlist->AddNetStub( net, p1, p.x, p.y );
	m_sel_net->connect[m_sel_id.i].m_merge = pMerge;
	// start dragging line
	int w = abs(m_Doc->m_trace_w);
	if( net->def_w )
		w = net->def_w;
	int via_w = abs(m_Doc->m_via_w);
	if( net->def_via_w )
		via_w = net->def_via_w;
	int via_hole_w = abs(m_Doc->m_via_hole_w);
	if( net->def_via_hole_w )
		via_hole_w = net->def_via_hole_w;
	m_routing_width = w;
	m_dir = 0;
	m_Doc->m_nlist->StartDraggingStub( pDC, net, m_sel_id.i, m_sel_id.ii,
		pi.x, pi.y, 
		mod_active_layer, w, 
		mod_active_layer, via_w, via_hole_w,
		2, m_inflection_mode );
	m_snap_angle_ref = p;
	SetCursorMode( CUR_DRAG_STUB );
	m_dlist->CancelHighLight();
	ShowSelectStatus();
	m_Doc->ProjectModified( TRUE );
	ReleaseDC( pDC );
	// Highlight
	if ( en_branch == BRANCH_TO_VERTEX ) 
	{	
		m_Doc->m_nlist->HighlightNetVertices( net, FALSE );
	}
	else if( en_branch == BRANCH_TO_LINE )
	{
		m_Doc->m_nlist->HighlightNetConnections( net );
	}
	else 
	{
		m_Doc->m_nlist->HighlightNet( net, TRANSPARENT_HILITE );
		m_Doc->m_plist->HighlightAllPadsOnNet( net, 1, m_active_layer );
	}
}
//===============================================================================================
//-------------------------------- attach this pad to a net	-------------------------------------
//===============================================================================================
void CFreePcbView::OnPadAddToNet()
{
	if( m_Doc->m_netlist_completed == 0 )
	{
		DlgAssignNet assign_net_dlg;
		assign_net_dlg.m_map = &m_Doc->m_nlist->m_map;
		assign_net_dlg.m_nLOCK = m_Doc->m_netlist_completed;
		int ret = assign_net_dlg.DoModal();
		if( ret == IDOK )
		{
			CString name = assign_net_dlg.m_net_str;
			void * ptr;
			cnet * new_net = 0;
			int test = m_Doc->m_nlist->m_map.Lookup( name, ptr );
			if( !test )
			{
				// create new net if legal string
				name.Trim();
				if( name.GetLength() )
				{
					new_net = m_Doc->m_nlist->AddNet( (char*)(LPCTSTR)name, 0, 0, 0 );
					SaveUndoInfoForNetAndConnections( new_net, CNetList::UNDO_NET_ADD, TRUE, m_Doc->m_undo_list );
				}
				else
				{
					// blank net name
					AfxMessageBox( "Illegal net name" );
					return;
				}
			}
			else
			{
				// use selected net
				new_net = (cnet*)ptr;
				SaveUndoInfoForNetAndConnections( new_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
			}
			// assign pin to net
			if( new_net )
			{
				SaveUndoInfoForPart( m_sel_part, CPartList::UNDO_PART_MODIFY, NULL, FALSE, m_Doc->m_undo_list );
				CString pin_name = m_sel_part->shape->GetPinNameByIndex( m_sel_id.i );
				m_Doc->m_nlist->AddNetPin( new_net,
					&m_sel_part->ref_des,
					&pin_name );
				m_Doc->m_nlist->OptimizeConnections(  new_net, -1, m_Doc->m_auto_ratline_disable,
															m_Doc->m_auto_ratline_min_pins, TRUE  );
				SetFKText( m_cursor_mode );
			}
			m_Doc->ProjectModified( TRUE );
		}
	}
	else if( g_bShow_nl_lock_Warning )
	{
		CString str = "Sorry, netlist is protected. You can remove protection through the menu Project-->Nets-->Netlist Protected";
		CDlgMyMessageBox dlg;
		dlg.Initialize( str );
		dlg.DoModal();
		g_bShow_nl_lock_Warning = !dlg.bDontShowBoxState;
	}
}
//===============================================================================================
// remove this pad from net
//
void CFreePcbView::OnPadDetachFromNet()
{
	if( m_Doc->m_netlist_completed == 0 || m_sel_part->shape->GetNumPins() == 1 )
	{
		int ret = AfxMessageBox( "Detach this pin from net?", MB_OKCANCEL );
		if( ret == IDOK )
		{
			cnet * pin_net = (cnet*)m_sel_part->pin[m_sel_id.i].net;
			SaveUndoInfoForPartAndNets( m_sel_part,
				CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
			CString pin_name = m_sel_part->shape->GetPinNameByIndex(m_sel_id.i);
			m_Doc->m_nlist->RemoveNetPin( pin_net, &m_sel_part->ref_des, &pin_name );
			SetFKText( m_cursor_mode );
			m_Doc->ProjectModified( TRUE );
			m_draw_layer = LAY_HILITE;//OnPadDetachFromNet
			Invalidate(FALSE);
		}
	}
	else if( g_bShow_nl_lock_Warning )
	{
		CString str = "Sorry, netlist is protected. You can remove protection through the menu Project-->Nets-->Netlist Protected";
		CDlgMyMessageBox dlg;
		dlg.Initialize( str );
		dlg.DoModal();
		g_bShow_nl_lock_Warning = !dlg.bDontShowBoxState;
	}
}
//===============================================================================================
// connect this pad to another pad
//
void CFreePcbView::OnPadConnectToPin()
{
	OnInfoBoxSendMess( "part_list ");
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CString pin_name = m_sel_part->shape->GetPinNameByIndex( m_sel_id.i );
	CPoint p = m_Doc->m_plist->GetPinPoint( m_sel_part, m_sel_id.i, m_sel_part->side, m_sel_part->angle );
	m_dragging_new_item = 0;
	m_dlist->StartDraggingRatLine( pDC, 0, 0, p.x, p.y, LAY_RAT_LINE, 1, 1 );	
	SetCursorMode( CUR_DRAG_CONNECT );
	if (m_sel_part->pin[m_sel_id.i].net)
		m_Doc->m_plist->HighlightAllPadsOnNet(	m_sel_part->pin[m_sel_id.i].net, 1, m_active_layer, TRANSPARENT_LAYER );
	ReleaseDC( pDC );
}
//===============================================================================================
// connect this vertex to another pad with a tee connection
//
void CFreePcbView::OnVertexConnectToPin()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	m_dragging_new_item = 0;
	m_dlist->StartDraggingRatLine( pDC, 0, 0, m_sel_vtx.x, m_sel_vtx.y, LAY_RAT_LINE, 1, 1 );
	SetCursorMode( CUR_DRAG_CONNECT );
	m_Doc->m_plist->HighlightAllPadsOnNet( m_sel_net, 1, m_active_layer, TRANSPARENT_LAYER );
	ReleaseDC( pDC );
}
//===============================================================================================
// set width for this segment (not a ratline)
//
void CFreePcbView::OnSegmentSetWidth()
{
	SetWidth( 0 );
	m_dlist->CancelHighLight();
	m_Doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, m_sel_is );
}
//===============================================================================================
// unroute this segment, convert to a ratline
//
void CFreePcbView::OnSegmentUnroute()
{
	// save undo info for connection
	if( m_cursor_mode == CUR_GROUP_SELECTED )
		SaveUndoInfoForGroup( UNDO_GROUP_MODIFY, m_Doc->m_undo_list );
	else
		SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );

	// see if segments to pin also need to be unrouted
	// see if start vertex of this segment is in start pad of connection
	for( cnet * n = m_Doc->m_nlist->GetFirstNet(); n; n = m_Doc->m_nlist->GetNextNet(/*LABEL*/) )
		for( int icon=0; icon<n->nconnects; icon++ )
		{
			cconnect * c = &n->connect[icon];
			if( c->m_selected )
			{
				for( int iseg=c->nsegs-1; iseg>=0; iseg-- )
				{
					if( c->seg[iseg].selected )
					{
						int x = c->vtx[iseg].x;
						int y = c->vtx[iseg].y;
						int layer = m_sel_seg.layer;
						c->seg[iseg].width = 0;
						c->seg[iseg].layer = LAY_RAT_LINE;
						if( iseg )
							if( c->seg[iseg-1].selected )
							{
								id sid(ID_NET,ID_CONNECT,icon,ID_SEG,iseg);
								UnSelect(n,&sid);
							}
						BOOL test1 = m_Doc->m_nlist->TestHitOnConnectionEndPad( x, y, n, icon, layer, 1 );
						if( test1 )
						{
							// unroute preceding segments
							for( int is=iseg-1; is>=0; is-- )
							{	
								c->seg[is].width = 0;
								c->seg[is].layer = LAY_RAT_LINE;
								if( is )
									if( c->seg[is-1].selected )
									{
										id sid(ID_NET,ID_CONNECT,icon,ID_SEG,is);
										UnSelect(n,&sid);
									}

							}
						}
						// see if end vertex of this segment is in end pad of connection
						x = c->vtx[iseg+1].x;
						y = c->vtx[iseg+1].y;
						BOOL test2 = m_Doc->m_nlist->TestHitOnConnectionEndPad( x, y, n, icon, layer, 0 );
						if( test2 )
						{
							// unroute following segments
							for( int is=c->nsegs-1; is>iseg; is-- )
							{
								c->seg[is].width = 0;
								c->seg[is].layer = LAY_RAT_LINE;
								if( is )
									if( c->seg[is-1].selected )
									{
										id sid(ID_NET,ID_CONNECT,icon,ID_SEG,is);
										UnSelect(n,&sid);
									}
							}
						}
					}
				}
				m_Doc->m_nlist->MergeUnroutedSegments( n, icon );
			}
		}
	
	if( m_sel_count == 1 )
	{
		cnet * n = m_sel_net;
		id sid = m_sel_id;
		sid.ii = min( m_sel_net->connect[m_sel_ic].nsegs-1, sid.ii );
		CancelSelection();
		while( n->connect[sid.i].seg[sid.ii].width && sid.ii > 0 )
			sid.ii--;
		NewSelect( n, &sid, 1, 0 );
	}
	m_Doc->ProjectModified( TRUE );
}
//===============================================================================================
// delete this segment, only used for the last segment of a stub trace
//
void CFreePcbView::OnSegmentDelete()
{
	if( m_sel_con.locked && m_sel_con.nsegs == 1 )
	{
		int ret = AfxMessageBox( "You are trying to delete a locked connection.\nAre you sure ? ",
			MB_YESNO );
		if( ret == IDNO )
			return;
	}
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
	int ic = m_Doc->m_nlist->RemoveSegment( m_sel_net, m_sel_ic, m_sel_is );
	if( m_Doc->m_vis[LAY_RAT_LINE] )
	{
		ic = m_Doc->m_nlist->OptimizeConnections(  m_sel_net, ic, m_Doc->m_auto_ratline_disable,
														 m_Doc->m_auto_ratline_min_pins, TRUE  );
	}
	if( ic == -1 )
		CancelSelection();
	else
	{
		m_sel_is = m_sel_net->connect[ic].nsegs;
		if( m_sel_is > 0 && ic >= 0 && m_sel_net->connect[ic].end_pin == cconnect::NO_END )
		{
			m_sel_ic = ic;
			m_sel_id.sst = ID_VERTEX;
			SetCursorMode( CUR_END_VTX_SELECTED );
			m_Doc->m_dlist->CancelHighLight();
			m_Doc->m_nlist->HighlightVertex( m_sel_net, m_sel_ic, m_sel_is );
		}
		else
			CancelSelection();
	}
	m_Doc->ProjectModified( TRUE );
}
//===============================================================================================
//--------------------------------- route this ratline ------------------------------------------
//===============================================================================================
void CFreePcbView::OnRatlineRoute()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	int last_seg_layer = 0;
	int n_segs = m_sel_con.nsegs;
	int w = m_sel_net->def_w;
	int via_w = m_Doc->m_via_w;
	int via_hole_w = m_Doc->m_via_hole_w;
	// get direction for routing, based on closest end of selected segment to cursor
	double d1x = p.x - m_sel_vtx.x;
	double d1y = p.y - m_sel_vtx.y;
	double d2x = p.x - m_sel_next_vtx.x;
	double d2y = p.y - m_sel_next_vtx.y;
	double d1 = d1x*d1x + d1y*d1y;
	double d2 = d2x*d2x + d2y*d2y;
	if( d1<d2 )
	{
		// route forward
		m_dir = 0;
		if ( m_sel_is )
			w=m_sel_last_seg.width;
		GetWidthsForSegment( &w, &via_w, &via_hole_w );
		if( m_sel_id.ii > 0 )
			last_seg_layer = m_sel_con.seg[m_sel_id.ii-1].layer;
		m_snap_angle_ref.x = m_sel_vtx.x;
		m_snap_angle_ref.y = m_sel_vtx.y;
		if( m_sel_id.ii > 0 )
		{
			if (m_sel_con.seg[m_sel_id.ii-1].width)
				w = m_sel_con.seg[m_sel_id.ii-1].width;
			else if (m_sel_vtx.via_hole_w)
				w = m_sel_vtx.via_hole_w;
		}
	}
	else
	{
		// route backward
		m_dir = 1;
		if ( m_sel_is<(m_sel_con.nsegs-1) )
			w=m_sel_next_seg.width;
		GetWidthsForSegment( &w, &via_w, &via_hole_w );
		if( m_sel_id.ii < (m_sel_con.nsegs-1) )
			last_seg_layer = m_sel_con.seg[m_sel_id.ii+1].layer;
		else if (m_sel_next_vtx.via_hole_w)
			w = m_sel_next_vtx.via_hole_w;
		m_snap_angle_ref.x = m_sel_next_vtx.x;
		m_snap_angle_ref.y = m_sel_next_vtx.y;
		if( m_sel_id.ii < (m_sel_con.nsegs-1))
		{
			if (m_sel_con.seg[m_sel_id.ii+1].width)
				w = m_sel_con.seg[m_sel_id.ii+1].width;
			else if (m_sel_next_vtx.via_hole_w)
				w = m_sel_next_vtx.via_hole_w;
		}
	}
	//
	//
	if( m_sel_id.ii == 0 && m_dir == 0)
	{
		// first segment, force to layer of starting pad if SMT
		cpart * p = m_sel_start_pin.part;
		CString pin_name = m_sel_start_pin.pin_name;
		int pin_index = m_Doc->m_nlist->GetPinIndexByNameForPart( p, pin_name, m_sel_con.vtx[0].x, m_sel_con.vtx[0].y );
		int L = m_Doc->m_plist->GetPinLayer( p, pin_index );
		if( L != LAY_PAD_THRU && L != m_active_layer )
		{
			m_active_layer = L;
			ShowActiveLayer(m_Doc->m_num_copper_layers);
			m_Doc->m_vis[m_active_layer] = 1;
			m_Doc->m_dlist->m_vis[m_active_layer] = 1;
		}
	}
	else if( m_sel_id.ii == (n_segs-1) && m_dir == 1 )
	{
		// last segment, force to layer of starting pad if SMT
		if( m_sel_con.end_pin != cconnect::NO_END )
		{
			cpart * p = m_sel_end_pin.part;
			CString pin_name = m_sel_end_pin.pin_name;
			int pin_index = m_Doc->m_nlist->GetPinIndexByNameForPart( p, pin_name, m_sel_con.vtx[m_sel_con.nsegs].x, m_sel_con.vtx[m_sel_con.nsegs].y );
			int L = m_Doc->m_plist->GetPinLayer( p, pin_index );
			if( L != LAY_PAD_THRU && L != m_active_layer  )
			{
				m_active_layer = L;
				ShowActiveLayer(m_Doc->m_num_copper_layers);
				m_Doc->m_vis[m_active_layer] = 1;
				m_Doc->m_dlist->m_vis[m_active_layer] = 1;
			}
		}
	}

	// now start dragging new segment
    m_routing_width = w;
	m_dragging_new_item = 0;
	m_Doc->m_nlist->StartDraggingSegment( pDC, m_sel_net, m_sel_ic, m_sel_is,
		p.x, p.y, m_active_layer,
		LAY_SELECTION, w,
		last_seg_layer, via_w, via_hole_w, m_dir, 2 );
	m_Doc->m_dlist->CancelHighLight();
	m_Doc->m_nlist->HighlightNet( m_sel_net, TRANSPARENT_LAYER );
	m_Doc->m_nlist->HighlightNetConnections( m_sel_net, TRANSPARENT_LAYER, 0, m_sel_ic, m_sel_is );
	m_Doc->m_plist->HighlightAllPadsOnNet( m_sel_net, 1, mod_active_layer, TRANSPARENT_LAYER );
	SetCursorMode( CUR_DRAG_RAT );
	ReleaseDC( pDC );
}
//===============================================================================================
//------------------------------- optimize this connection --------------------------------------
//===============================================================================================
void CFreePcbView::OnRatlineOptimize()
{
	int new_ic = m_Doc->m_nlist->OptimizeConnections( m_sel_net, m_sel_ic, m_Doc->m_auto_ratline_disable,
														m_Doc->m_auto_ratline_min_pins, TRUE  );
	ReselectNetItemIfConnectionsChanged( new_ic );
	m_Doc->ProjectModified( TRUE );
}
//===============================================================================================
//------------------------------- optimize this connection	-------------------------------------
//===============================================================================================
void CFreePcbView::OnRatlineChangeEndPin()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	m_dlist->CancelHighLight();
	cconnect * c = &m_sel_con;
	c->seg[m_sel_id.ii].dl_el->visible = FALSE;
	int x, y;
	if( m_sel_id.ii == 0 )
	{
		// ratline is first segment of connection
		x = c->vtx[1].x;
		y = c->vtx[1].y;
	}
	else
	{
		// ratline is last segment of connection
		x = c->vtx[m_sel_id.ii].x;
		y = c->vtx[m_sel_id.ii].y;
	}
	m_dlist->StartDraggingRatLine( pDC, 0, 0, x, y, LAY_RAT_LINE, 1, 1 );
	SetCursorMode( CUR_DRAG_RAT_PIN );
	ReleaseDC( pDC );
}
//===============================================================================================
//---------------------------- change vertex size vertex ----------------------------------------
//===============================================================================================
void CFreePcbView::OnVertexSize()
{
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
	CDlgVia dlg = new CDlgVia;//ok
	dlg.Initialize( m_Doc->m_via_w, m_Doc->m_via_hole_w, &m_Doc->m_v_w, &m_Doc->m_v_h_w, m_sel_vtx.via_w, m_sel_vtx.via_hole_w, m_Doc->m_units );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		m_sel_vtx.via_w = dlg.m_via_w;
		m_sel_vtx.via_hole_w = dlg.m_via_hole_w;
		m_dlist->CancelHighLight();
		m_Doc->m_nlist->DrawVia( m_sel_net, m_sel_ic, m_sel_is );
		m_Doc->m_nlist->HighlightVertex( m_sel_net, m_sel_ic, m_sel_is );
	}
	delete dlg;
}
//===============================================================================================
//---------------------------------- move this vertex -------------------------------------------
//===============================================================================================
void CFreePcbView::OnVertexMove()
{
//	m_Doc->m_nlist->SetNetVisibility( m_sel_net, TRUE );
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	id id = m_sel_id;
	int ic = m_sel_id.i;
	int ivtx = m_sel_id.ii;
	m_dragging_new_item = 0;
	m_from_pt.x = m_sel_vtx.x;
	m_from_pt.y = m_sel_vtx.y;
	m_Doc->m_nlist->StartDraggingVertex( pDC, m_sel_net, ic, ivtx, p.x, p.y, 2 );
	SetCursorMode( CUR_DRAG_VTX );
	ReleaseDC( pDC );
}
//===============================================================================================
// delete this vertex
// i.e. unroute adjacent segments and reroute if on same layer
// stub trace needs some special handling
//===============================================================================================
void CFreePcbView::OnVertexDelete()
{
	if( m_sel_is == 0 )
		ASSERT(0);
	if( m_sel_is == m_sel_con.nsegs && m_sel_con.end_pin >= 0 )
		ASSERT(0);
	if( m_sel_is == m_sel_con.nsegs )
	{
		m_sel_is--;
		OnSegmentDelete();
	}
	else
	{
		SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
		int tee = m_sel_vtx.tee_ID;
		int lay_0 = m_sel_con.seg[0].layer;
		int pad_lay_0 = m_sel_con.vtx[0].pad_layer;
		int seg_wid = m_sel_seg.width;
		m_Doc->m_nlist->UndrawConnection( m_sel_net,m_sel_ic );
		m_sel_con.seg.RemoveAt( m_sel_is-1 );
		m_sel_con.vtx.RemoveAt( m_sel_is );
		m_sel_con.nsegs--;
		m_Doc->m_nlist->DrawConnection( m_sel_net,m_sel_ic );
		if( m_sel_is == 1 )
		{
			m_sel_con.seg[0].layer = lay_0;
			if( pad_lay_0 != LAY_PAD_THRU )
				if( m_sel_con.seg[0].layer != pad_lay_0 )
					m_sel_con.seg[0].layer = LAY_RAT_LINE;
			if( m_sel_con.nsegs == 1 && m_sel_con.end_pin >= 0 )
			{
				if( m_sel_con.seg[0].layer != m_sel_con.vtx[m_sel_con.nsegs].pad_layer )
					m_sel_con.seg[0].layer = LAY_RAT_LINE;
			}
			if( m_sel_con.seg[0].layer == LAY_RAT_LINE )
				m_sel_con.seg[0].width = 0;
			if( m_sel_con.seg[0].width == 0 )
				m_sel_con.seg[0].layer = LAY_RAT_LINE;
		}
		int w = m_sel_last_seg.width;
		if( m_sel_is < m_sel_con.nsegs )
			w = max( m_sel_seg.width,w );
		int vw=m_sel_last_vtx.via_w, vh=m_sel_last_vtx.via_hole_w;
		if (vw == 0)
			GetWidthsForSegment( &w, &vw, &vh );
		int vwn=m_sel_vtx.via_w, vhn=m_sel_vtx.via_hole_w;
		if (vwn == 0)
			GetWidthsForSegment( &w, &vwn, &vhn );
		if( tee )
		{
			int rem = m_Doc->m_nlist->RemoveOrphanBranches( m_sel_net, tee, 1, vw, vh );
			if( rem == m_sel_ic )
			{
				m_Doc->ProjectModified( TRUE );
				return;
			}
			else if( rem < m_sel_ic && rem >= 0 )
				m_sel_ic--;
		}
		m_Doc->m_nlist->ReconcileVia( m_sel_net, m_sel_ic, m_sel_is-1, 0, vw, vh );
		m_Doc->m_nlist->ReconcileVia( m_sel_net, m_sel_ic, m_sel_is, 0, vwn, vhn );
		m_Doc->m_nlist->DrawConnection( m_sel_net, m_sel_ic );
		//m_Doc->m_dlist->CancelHighLight();
		//HighlightGroup();
		m_Doc->ProjectModified( TRUE );
	}
	Invalidate( FALSE );
}
//===============================================================================================
//------------------------- move the end vertex of a stub trace ---------------------------------
//===============================================================================================
void CFreePcbView::OnEndVertexMove()
{
	CDC * pDC = GetDC();
	SetDCToWorldCoords( pDC );
	pDC->SelectClipRgn( &m_pcb_rgn );
	CPoint p;
	p = m_last_cursor_point;
	SetCursorMode( CUR_DRAG_END_VTX );
	m_Doc->m_nlist->StartDraggingEndVertex( pDC, m_sel_net, m_sel_ic, m_sel_is, 2 );
	m_Doc->m_nlist->HighlightNetVertices( m_sel_net, FALSE, FALSE );
	ReleaseDC( pDC );
}
//===============================================================================================
//------------------------------- force a via on end vertex -------------------------------------
//===============================================================================================
void CFreePcbView::OnEndVertexAddVia()
{
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
	m_Doc->m_nlist->ForceVia( m_sel_net, m_sel_ic, m_sel_is,FALSE );
	if( m_Doc->m_vis[LAY_RAT_LINE] ) 
	{
		OnRatlineOptimize();
	}
	SetFKText( m_cursor_mode );
	m_Doc->ProjectModified( TRUE );
}
//===============================================================================================
//------------------------- remove forced via on end vertex -------------------------------------
//===============================================================================================
void CFreePcbView::OnEndVertexRemoveVia()
{
//	m_Doc->m_nlist->SetNetVisibility( m_sel_net, TRUE );
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
	m_Doc->m_nlist->UnforceVia( m_sel_net, m_sel_ic, m_sel_is, FALSE );
	int sel_ic = m_sel_ic;
	if( m_sel_con.seg[m_sel_is-1].layer == LAY_RAT_LINE )
	{
		sel_ic = m_Doc->m_nlist->RemoveSegment( m_sel_net, m_sel_ic, m_sel_is-1 );
		m_sel_is--;
	}
	if( m_Doc->m_vis[LAY_RAT_LINE] )
		OnRatlineOptimize();
	if( sel_ic == -1 )
		CancelSelection();
	else
	{
		m_sel_ic = sel_ic;
		SetFKText( m_cursor_mode );
	}
	m_Doc->ProjectModified( TRUE );
}
//===============================================================================================
//------------------------ append more segments to this stub trace ------------------------------
//===============================================================================================
void CFreePcbView::OnEndVertexAddSegments()
{
	if( !m_sel_net )
		return;
	if( m_sel_ic >= m_sel_net->nconnects )
		return;
	if( m_sel_iv != m_sel_con.nsegs )
		return;
	if( m_sel_con.end_pin != cconnect::NO_END )
		return;
	CDC * pDC = GetDC();
	SetDCToWorldCoords( pDC );
	pDC->SelectClipRgn( &m_pcb_rgn );
	CPoint p;
	p = m_last_cursor_point;
	m_sel_id.sst = ID_SEG;
	int w=m_sel_net->def_w, via_w=0, via_hole_w=0;
	m_snap_angle_ref.x = m_sel_vtx.x;
	m_snap_angle_ref.y = m_sel_vtx.y;
	if (m_sel_is)
		w=m_sel_last_seg.width;
	GetWidthsForSegment( &w, &via_w, &via_hole_w );
	if( m_sel_id.ii > 0 )
			w = m_sel_con.seg[m_sel_id.ii-1].width;
	m_routing_width = w;
	m_Doc->m_nlist->StartDraggingStub( pDC, m_sel_net, m_sel_ic, m_sel_is,
		p.x, p.y, 
		mod_active_layer, w, 
		mod_active_layer, via_w, via_hole_w,
		2, m_inflection_mode );
	SetCursorMode( CUR_DRAG_STUB );

	// hilite
	if( en_branch == DISABLE_BRANCH )
		m_Doc->m_plist->HighlightAllPadsOnNet(	m_sel_net, 1, m_active_layer );
	else if( en_branch == BRANCH_TO_VERTEX )
		m_Doc->m_nlist->HighlightNetVertices( m_sel_net, FALSE, FALSE );
	else
		m_Doc->m_nlist->HighlightNetConnections( m_sel_net );
	ReleaseDC( pDC );
}
//===============================================================================================
//----------- convert stub trace to regular connection by adding ratline to a pad ---------------
//===============================================================================================
void CFreePcbView::OnEndVertexAddConnection()
{
	OnVertexConnectToPin();
}
//===============================================================================================
//--------------- end vertex selected, delete it and the adjacent segment -----------------------
//===============================================================================================
void CFreePcbView::OnEndVertexDelete()
{
	OnVertexDelete();
}
//===============================================================================================
//------------------------- edit the position of an end vertex ----------------------------------
//===============================================================================================
void CFreePcbView::OnEndVertexEdit()
{
	DlgEditBoardCorner dlg;
	CString str = "Edit End Vertex Position";
	int x = m_sel_vtx.x;
	int y = m_sel_vtx.y;
	dlg.Init( &str, m_Doc->m_units, x, y );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
		m_Doc->m_nlist->MoveEndVertex( m_sel_net, m_sel_ic, m_sel_is,
			dlg.GetX(), dlg.GetY() );
		m_Doc->ProjectModified( TRUE );
		m_Doc->m_nlist->HighlightVertex( m_sel_net, m_sel_ic, m_sel_is );
	}
}
//===============================================================================================
//----------- finish routing a connection by making a segment to the destination pad ------------
//===============================================================================================
void CFreePcbView::OnRatlineComplete()
{
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
	int w=m_routing_width, v_w, v_h_w, vw_next, vh_next;
	//
	v_w=m_sel_vtx.via_w; 
	v_h_w=m_sel_vtx.via_hole_w;
	vw_next=m_sel_next_vtx.via_w; 
	vh_next=m_sel_next_vtx.via_hole_w;
	//
	if( v_w == 0 || v_h_w == 0 )
		GetWidthsForSegment( &w, &v_w, &v_h_w );
	if( vw_next == 0 || vh_next == 0 )
		GetWidthsForSegment( &w, &vw_next, &vh_next );
	int test = m_Doc->m_nlist->RouteSegment( m_sel_net, m_sel_ic, m_sel_is, 
											mod_active_layer, 
											w, v_w, v_h_w, vw_next, vh_next );
	if( !test )
	{		
		m_Doc->m_nlist->CancelDraggingSegment( m_sel_net, m_sel_ic, m_sel_is );
		m_sel_ic = m_Doc->m_nlist->OptimizeConnections( m_sel_net, m_sel_ic, m_Doc->m_auto_ratline_disable,
														m_Doc->m_auto_ratline_min_pins, TRUE  );
		if( m_dir )
			m_sel_is++;
		id cpy_id( ID_NET, ID_CONNECT, m_sel_ic, ID_VERTEX, m_sel_is );
		cnet * cpy_net = m_sel_net;
		CancelSelection();
		if( cpy_id.ii > 0 && cpy_id.ii < cpy_net->connect[cpy_id.i].nsegs ) 
			NewSelect( cpy_net, &cpy_id, 1, 0 );
	}
	m_Doc->ProjectModified( TRUE );
}
//===============================================================================================
//------------------------------ set width of a connection --------------------------------------
//===============================================================================================
void CFreePcbView::OnRatlineSetWidth()
{
	if( m_sel_con.nsegs == 1 )
		SetWidth( 2 );
	else
		SetWidth( 1 );
}
//===============================================================================================
//--------------------------------- delete a connection	-----------------------------------------
//===============================================================================================
void CFreePcbView::OnRatlineDeleteConnection()
{
	if( m_sel_con.locked )
	{
		int ret = AfxMessageBox( "You are trying to delete a locked connection.\nAre you sure ? ",
			MB_YESNO );
		if( ret == IDNO )
			return;
	}
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
	m_Doc->m_nlist->RemoveNetConnect( m_sel_net, m_sel_ic, FALSE );
	m_Doc->m_nlist->SetAreaConnections( m_sel_net );
	CancelSelection();
	m_Doc->ProjectModified( TRUE );
	Invalidate(FALSE);
}
//===============================================================================================
//--------------------------------- lock a connection -------------------------------------------
//===============================================================================================
void CFreePcbView::OnRatlineLockConnection()
{
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
	m_sel_con.locked = 1;
	ShowSelectStatus();
	SetFKText( m_cursor_mode );
	m_sel_ic = m_Doc->m_nlist->OptimizeConnections(  m_sel_net, m_sel_ic, m_Doc->m_auto_ratline_disable, m_Doc->m_auto_ratline_min_pins, FALSE );
	m_Doc->ProjectModified( TRUE );
}
//===============================================================================================
//-------------------------------- unlock a connection ------------------------------------------
//===============================================================================================
void CFreePcbView::OnRatlineUnlockConnection()
{
	if( AfxMessageBox( " Unlock connect?", MB_YESNO ) == IDYES )
	{
		SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
		m_sel_con.locked = 0;
		ShowSelectStatus();
		SetFKText( m_cursor_mode );
		m_Doc->ProjectModified( TRUE );
	}
}
//===============================================================================================
//--------------------------------- edit a text string ------------------------------------------
//===============================================================================================
void CFreePcbView::OnTextEdit()
{
	// create dialog and pass parameters
	CDlgAddText add_text_dlg;
	add_text_dlg.Initialize( 0, m_Doc->m_num_layers, 0, &m_sel_text->m_str,
		m_Doc->m_units, m_sel_text->m_layer, m_sel_text->m_mirror,
			m_sel_text->m_bNegative, m_sel_text->m_font_size,
			m_sel_text->m_stroke_width, m_sel_text->m_x, m_sel_text->m_y );
	int ret = add_text_dlg.DoModal();
	if( ret == IDCANCEL )
		return;

	// now replace old text with new one
	SaveUndoInfoForText( m_sel_text, CTextList::UNDO_TEXT_MODIFY, TRUE, m_Doc->m_undo_list );
	int x = add_text_dlg.m_x;
	int y = add_text_dlg.m_y;
	int mirror = add_text_dlg.m_bMirror;
	BOOL bNegative = add_text_dlg.m_bNegative;
	int font_size = add_text_dlg.m_height;
	int stroke_width = add_text_dlg.m_width;
	int layer = add_text_dlg.m_layer;
	CString test_str = add_text_dlg.m_str;
	m_dlist->CancelHighLight();
	CText * new_text = m_Doc->m_tlist->AddText( x, y, m_sel_text->m_angle, mirror, bNegative,
		layer, font_size, stroke_width, &test_str );
	new_text->m_guid = m_sel_text->m_guid;
	new_text->m_merge = m_sel_text->m_merge;
	m_Doc->m_tlist->RemoveText( m_sel_text );
	m_sel_text = new_text;
	m_Doc->m_tlist->HighlightText( m_sel_text );

	// start dragging if requested in dialog
	if( add_text_dlg.m_bDrag )
		OnTextMove();
	m_Doc->ProjectModified( TRUE );
}

//===============================================================================================
// move a outline corner
//
void CFreePcbView::OnOPCornerMove()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	m_from_pt.x = m_Doc->m_outline_poly[m_sel_id.i].GetX( m_sel_id.ii );
	m_from_pt.y = m_Doc->m_outline_poly[m_sel_id.i].GetY( m_sel_id.ii );
	m_Doc->m_outline_poly[m_sel_id.i].StartDraggingToMoveCorner( pDC, m_sel_id.ii, p.x, p.y, 2 );
	SetCursorMode( CUR_DRAG_OP_MOVE );
	ReleaseDC( pDC );
}
//===============================================================================================
// edit a outline corner
//
void CFreePcbView::OnOPCornerEdit()
{
	DlgEditBoardCorner dlg;
	CString str = "Corner Position";
	int x = m_Doc->m_outline_poly[m_sel_id.i].GetX(m_sel_id.ii);
	int y = m_Doc->m_outline_poly[m_sel_id.i].GetY(m_sel_id.ii);
	dlg.Init( &str, m_Doc->m_units, x, y );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		int gm = m_Doc->m_outline_poly[m_sel_id.i].GetMerge();
		if( gm >= 0 )
		{
			CString ps;
			ps.Format("This polyline is connected to other objects through the \"MERGE\" property. Move all objects of the group %s?", m_Doc->m_mlist->GetMerge( gm ) );
			if( AfxMessageBox( ps, MB_YESNO ) == IDNO )
				gm = -1;
		}
		if( gm == -1 )
		{
			CString ps;
			ps.Format("Move the entire line?");
			if( AfxMessageBox( ps, MB_YESNO ) == IDYES )
			{
				m_sel_id.sst = ID_SIDE;
				m_sel_id.ii = 0;
				NewSelect( NULL, &m_sel_id, 0, 0 );
				SelectContour();
			}
		}
		else
			NewSelectM( NULL, gm );	
		if( m_sel_count == 1 )
		{
			SaveUndoInfoForOutlinePoly( UNDO_OP, TRUE, m_Doc->m_undo_list );
			m_Doc->m_outline_poly[m_sel_id.i].MoveCorner( m_sel_id.ii,
				dlg.GetX(), dlg.GetY() );
			//CancelSelection();
		}
		else
		{
			SaveUndoInfoForGroup( UNDO_GROUP_MODIFY, m_Doc->m_undo_list );
			int dx = dlg.GetX() - x;
			int dy = dlg.GetY() - y;
			MoveGroup( dx, dy, 0 );
			SetCursorMode( CUR_GROUP_SELECTED );
			m_Doc->m_dlist->CancelHighLight();
			HighlightGroup();
		}
		m_Doc->ProjectModified( TRUE );
	}
}
//===============================================================================================
// delete a corner
//
void CFreePcbView::OnOPCornerDelete()
{
	int cc = 4;
	if( m_Doc->m_outline_poly[m_sel_id.i].GetClosed() == 0 )
		cc--;
	if( m_Doc->m_outline_poly[m_sel_id.i].GetNumCorners() < cc )
	{
		AfxMessageBox( "Outline has too few corners" );
		return;
	}
	SaveUndoInfoForOutlinePoly( UNDO_OP, TRUE, m_Doc->m_undo_list );
	m_Doc->m_outline_poly[m_sel_id.i].DeleteCorner( m_sel_id.ii );
	CancelSelection();
	m_Doc->ProjectModified( TRUE );
}
//===============================================================================================
// insert a new corner in a side of a outline
//
void CFreePcbView::OnOPSideAddCorner()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	m_Doc->m_outline_poly[m_sel_id.i].StartDraggingToInsertCorner( pDC, m_sel_id.ii, p.x, p.y, 2 );
	SetCursorMode( CUR_DRAG_OP_INSERT );
	ReleaseDC( pDC );
}
//===============================================================================================
// delete entire outline
//
void CFreePcbView::OnOPDeleteOutline()
{
	SaveUndoInfoForOutlinePoly( UNDO_OP, TRUE, m_Doc->m_undo_list );
	m_Doc->m_outline_poly[m_sel_id.i].Undraw();
	m_Doc->m_outline_poly.RemoveAt( m_sel_id.i );
	m_Doc->OPRefreshID();
	m_Doc->ProjectModified( TRUE );
	CancelSelection();
	Invalidate(FALSE);
}
//===============================================================================================
// move a copper area corner
//
void CFreePcbView::OnAreaCornerMove()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	m_from_pt.x = m_sel_net->area[m_sel_id.i].poly->GetX( m_sel_id.ii );
	m_from_pt.y = m_sel_net->area[m_sel_id.i].poly->GetY( m_sel_id.ii );
	m_Doc->m_nlist->StartDraggingAreaCorner( pDC, m_sel_net, m_sel_ia, m_sel_is, p.x, p.y, 2 );
	SetCursorMode( CUR_DRAG_AREA_MOVE );
	ReleaseDC( pDC );
}
//===============================================================================================
// delete a copper area corner
//
void CFreePcbView::OnAreaCornerDelete()
{
	carea * area;
	area = &m_sel_net->area[m_sel_id.i];
	if( area->poly->GetNumCorners() > 3 )
	{
		SaveUndoInfoForArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_MODIFY, TRUE, m_Doc->m_undo_list );
		area->poly->DeleteCorner( m_sel_id.ii );
		m_Doc->m_nlist->SetAreaConnections( m_sel_net, m_sel_ia );
		m_Doc->m_nlist->OptimizeConnections(  m_sel_net, -1, m_Doc->m_auto_ratline_disable,
														m_Doc->m_auto_ratline_min_pins, TRUE  );
		m_Doc->ProjectModified( TRUE );
		CancelSelection();
	}
	else
		OnAreaCornerDeleteArea();
}
//===============================================================================================
// delete entire area
//
void CFreePcbView::OnAreaCornerDeleteArea()
{
	OnAreaSideDeleteArea();
	m_Doc->ProjectModified( TRUE );
}
//===============================================================================================
//insert a new corner in a side of a copper area
//
void CFreePcbView::OnAreaSideAddCorner()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	m_Doc->m_nlist->StartDraggingInsertedAreaCorner( pDC, m_sel_net, m_sel_ia, m_sel_is, p.x, p.y, 2 );
	SetCursorMode( CUR_DRAG_AREA_INSERT );
	ReleaseDC( pDC );
}
//===============================================================================================
// delete entire area
//
void CFreePcbView::AreaSideDeleteArea()
{
	SaveUndoInfoForArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_DELETE, TRUE, m_Doc->m_undo_list );
	m_Doc->m_nlist->RemoveArea( m_sel_net, m_sel_ia );
	m_Doc->m_nlist->OptimizeConnections(  m_sel_net, -1, m_Doc->m_auto_ratline_disable,
														m_Doc->m_auto_ratline_min_pins, TRUE  );
	CancelSelection();
	m_Doc->ProjectModified( TRUE );
	
}
void CFreePcbView::OnAreaSideDeleteArea()
{
	AreaSideDeleteArea();
	m_draw_layer = -1;
	Invalidate(FALSE);
}
//===============================================================================================
// detect state where nothing is selected or being dragged
//
BOOL CFreePcbView::CurNone()
{
	return( m_cursor_mode == CUR_NONE_SELECTED );
}
//===============================================================================================
// detect any selected state
//
BOOL CFreePcbView::CurSelected()
{
	return( m_cursor_mode > CUR_NONE_SELECTED && m_cursor_mode < CUR_NUM_SELECTED_MODES );
}
//===============================================================================================
// detect any dragging state
//
BOOL CFreePcbView::CurDragging()
{
	return( m_cursor_mode > CUR_NUM_SELECTED_MODES && m_cursor_mode < CUR_NUM_MODES );
}
//===============================================================================================
// detect states using routing grid
//
BOOL CFreePcbView::CurDraggingRouting()
{
	return( m_cursor_mode == CUR_DRAG_RAT
		|| m_cursor_mode == CUR_DRAG_VTX
		|| m_cursor_mode == CUR_DRAG_VTX_INSERT
		|| m_cursor_mode == CUR_DRAG_END_VTX
		|| m_cursor_mode == CUR_ADD_AREA
		|| m_cursor_mode == CUR_DRAG_AREA_1
		|| m_cursor_mode == CUR_DRAG_AREA
		|| m_cursor_mode == CUR_DRAG_AREA_INSERT
		|| m_cursor_mode == CUR_DRAG_AREA_MOVE
		|| m_cursor_mode == CUR_ADD_AREA_CUTOUT
		|| m_cursor_mode == CUR_DRAG_AREA_CUTOUT_1
		|| m_cursor_mode == CUR_DRAG_AREA_CUTOUT
		|| m_cursor_mode == CUR_DRAG_STUB
		|| m_cursor_mode == CUR_MOVE_SEGMENT
		|| m_cursor_mode == CUR_DRAG_VTX_INSERT
		);
}
//===============================================================================================
// detect states using placement grid
//
BOOL CFreePcbView::CurDraggingPlacement()
{
	return( m_cursor_mode == CUR_ADD_OP
		|| m_cursor_mode == CUR_DRAG_OP_1
		|| m_cursor_mode == CUR_DRAG_OP
		|| m_cursor_mode == CUR_DRAG_OP_INSERT
		|| m_cursor_mode == CUR_DRAG_OP_MOVE
		|| m_cursor_mode == CUR_DRAG_PART
		|| m_cursor_mode == CUR_DRAG_REF
		|| m_cursor_mode == CUR_DRAG_TEXT
		|| m_cursor_mode == CUR_DRAG_GROUP
		|| m_cursor_mode == CUR_DRAG_GROUP_ADD
		|| m_cursor_mode == CUR_MOVE_ORIGIN );
}
//===============================================================================================
// snap cursor if required and set m_last_cursor_point
//
void CFreePcbView::SnapCursorPoint( CPoint wp, UINT nFlags )
{
	static int mem_scale_factor = 1;
	static int mem_1 = 1;
	static int mem_2 = 2;

	// see if we need to snap at all
	if( CurDragging() )
	{
		// yes, set snap modes based on cursor mode and SHIFT and CTRL keys
		if( m_cursor_mode == CUR_DRAG_RAT || m_cursor_mode == CUR_DRAG_STUB )
		{
			// routing a trace segment, set modes
			if( nFlags != -1 )
			{
				if( nFlags & MK_CONTROL )
				{
					// control key held down
					m_snap_mode = SM_GRID_LINES;
					m_inflection_mode = IM_NONE;
				}
				else
				{
					m_snap_mode = SM_GRID_POINTS;
					if( m_Doc->m_snap_angle == 45 )
					{
						if( nFlags & MK_SHIFT )
							m_inflection_mode = IM_45_90;
						else
							m_inflection_mode = IM_90_45;
					}
					else if( m_Doc->m_snap_angle == 90 )
						m_inflection_mode = IM_90;

				}
				m_dlist->SetInflectionMode( m_inflection_mode );
			}
		}
		else
		{
			// for other dragging modes, always use grid points with no inflection
			m_snap_mode = SM_GRID_POINTS;
			m_inflection_mode = IM_NONE;
			m_dlist->SetInflectionMode( m_inflection_mode );
		}
		// set grid spacing
		int grid_spacing;
		if( CurDraggingPlacement() )
		{
			grid_spacing = m_Doc->m_part_grid_spacing;
		}
		else if( CurDraggingRouting() )
		{
			grid_spacing = m_Doc->m_routing_grid_spacing;
		}
		else if( m_Doc->m_units == MIL )
		{
			grid_spacing = m_pcbu_per_wu;
		}
		else if( m_Doc->m_units == MM )
		{
			grid_spacing = m_pcbu_per_wu;               
		}
		else 
			ASSERT(0);
		// see if we need to snap to angle
		if( m_Doc->m_snap_angle && (wp != m_snap_angle_ref)
			&& ( m_cursor_mode == CUR_DRAG_RAT
			|| m_cursor_mode == CUR_DRAG_STUB
			|| m_cursor_mode == CUR_DRAG_AREA_1
			|| m_cursor_mode == CUR_DRAG_AREA
			|| m_cursor_mode == CUR_DRAG_AREA_CUTOUT_1
			|| m_cursor_mode == CUR_DRAG_AREA_CUTOUT
			|| m_cursor_mode == CUR_DRAG_OP_1
			|| m_cursor_mode == CUR_DRAG_OP ) )
		{
			// yes, check snap mode
			if( m_snap_mode == SM_GRID_LINES )
			{
				// patch to snap to grid lines, contributed by ???
				// modified by AMW to work when cursor x,y are < 0
				// offset cursor and ref positions by integral number of grid spaces
				// to make all values positive
				double offset_grid_spaces;
				modf( (double)INT_MAX/grid_spacing, &offset_grid_spaces );
				double offset = offset_grid_spaces*grid_spacing;
				double off_x = wp.x + offset;
				double off_y = wp.y + offset;
				double ref_x = m_snap_angle_ref.x + offset;
				double ref_y = m_snap_angle_ref.y + offset;
				//find nearest snap angle to an integer division of 90
				int snap_angle = m_Doc->m_snap_angle;
				if(90 % snap_angle != 0)
				{
					int snap_pos = snap_angle;
					int snap_neg = snap_angle;
					while(90 % snap_angle != 0)
					{
						snap_pos++;
						snap_neg--;
						if(snap_pos >= 90)
							snap_pos = 90;
						if(snap_neg <= 1)
							snap_neg = 1;
						if(90 % snap_pos == 0)
							snap_angle = snap_pos;
						if(90 % snap_neg == 0)
							snap_angle = snap_neg;
					}
				}

				//snap the x and y coordinates to the appropriate angle
				double angle_grid = snap_angle*M_PI/180.0;
				double dx = off_x - ref_x;
				double dy = off_y - ref_y;
				double angle = atan2(dy,dx) + 2*M_PI; //make it a positive angle
				if(angle > 2*M_PI)
					angle -= 2*M_PI;
				double angle_angle_grid = angle/angle_grid;
				int sel_snap_angle = (int)angle_angle_grid + ((angle_angle_grid - (double)((int)angle_angle_grid)) > 0.5 ? 1 : 0);
				double point_angle = angle_grid*sel_snap_angle; //result of angle calculation
				CString test, test_grid;
				test.Format("point_angle: %f\r\n",point_angle);
				test_grid.Format("grid_spacing: %d\r\n",grid_spacing);


				//find the distance along that angle
				//match the distance the actual point is from the start point
				//double dist = sqrt(dx*dx + dy*dy);
				double dist = dx*cos(point_angle)+dy*sin(point_angle);

				double distx = dist*cos(point_angle);
				double disty = dist*sin(point_angle);

				double xpos = ref_x + distx;
				double ypos = ref_y + disty;


				//special case horizontal lines and vertical lines
				// just to make sure floating point error doesn't cause any problems
				if(APPROX(point_angle,0) || APPROX(point_angle,2*M_PI) || APPROX(point_angle,M_PI))
				{
					//horizontal line
					//snap x component to nearest grid
					off_y = (int)(ypos + 0.5);
					double modval = fmod(xpos,(double)grid_spacing);
					if(modval > grid_spacing/2.0)
					{
						//round up to nearest grid space
						off_x = xpos + ((double)grid_spacing - modval);
					}
					else
					{
						//round down to nearest grid space
						off_x = xpos - modval;
					}
				}
				else if(APPROX(point_angle,M_PI/2) || APPROX(point_angle,3*M_PI/2))
				{
					//vertical line
					//snap y component to nearest grid
					off_x = (int)(xpos + 0.5);
					double modval = fabs(fmod(ypos,(double)grid_spacing));
					int test = modval * grid_spacing - offset;
					if(modval > grid_spacing/2.0)
					{
						off_y = ypos + ((double)grid_spacing - modval);
					}
					else
					{
						off_y = ypos - modval;
					}
				}
				else
				{
					//normal case
					//snap x and y components to nearest grid line along the same angle
					//calculate grid lines surrounding point
					int minx = ((int)(xpos/(double)grid_spacing))*grid_spacing - (xpos < 0);
					//int minx = (int)fmod(xpos,(double)grid_spacing);
					int maxx = minx + grid_spacing;
					int miny = ((int)(ypos/(double)grid_spacing))*grid_spacing - (ypos < 0);
					//int miny = (int)fmod(ypos,(double)grid_spacing);
					int maxy = miny + grid_spacing;

					//calculate the relative distances to each of those grid lines from the ref point
					int rminx = minx - ref_x;
					int rmaxx = maxx - ref_x;
					int rminy = miny - ref_y;
					int rmaxy = maxy - ref_y;

					//calculate the length of the hypotenuse of the triangle
					double maxxh = dist*(double)rmaxx/distx;
					double minxh = dist*(double)rminx/distx;
					double maxyh = dist*(double)rmaxy/disty;
					double minyh = dist*(double)rminy/disty;

					//some stupidly large value.  One of the results MUST be smaller than this unless the grid is this large (unlikely)
					double mindist = 1e100;
					int select = 0;

					if(fabs(maxxh - dist) < mindist)
					{
						select = 1;
						mindist = fabs(maxxh - dist);
					}
					if(fabs(minxh - dist) < mindist)
					{
						select = 2;
						mindist = fabs(minxh - dist);
					}
					if(fabs(maxyh - dist) < mindist)
					{
						select = 3;
						mindist = fabs(maxyh - dist);
					}
					if(fabs(minyh - dist) < mindist)
					{
						select = 4;
						mindist = fabs(minyh - dist);
					}

					switch(select)
					{
					case 1:
						//snap to line right of point
						off_x = maxx;
						off_y = (int)(disty*(double)rmaxx/distx + (double)(ref_y) + 0.5);
						break;
					case 2:
						//snap to line left of point
						off_x = minx;
						off_y = (int)(disty*(double)rminx/distx + (double)(ref_y) + 0.5);
						break;
					case 3:
						//snap to line above point
						off_x = (int)(distx*(double)rmaxy/disty + (double)(ref_x) + 0.5);
						off_y = maxy;
						break;
					case 4:
						//snap to line below point
						off_x = (int)(distx*(double)rminy/disty + (double)(ref_x) + 0.5);
						off_y = miny;
						break;
					default:
						ASSERT(0);//error
					}

				}
				// remove offset and correct for round-off
				double ttest = off_x - offset;
				if( ttest >= 0.0 )
					wp.x = ttest + 0.5;
				else
					wp.x = ttest - 0.5;
				ttest = off_y - offset;
				if( ttest >= 0.0 )
					wp.y = ttest + 0.5;
				else
					wp.y = ttest - 0.5;
			}
			else
			{
				// old code
				// snap to angle only if the starting point is on-grid
				double ddx = fmod( (double)(m_snap_angle_ref.x), grid_spacing );
				double ddy = fmod( (double)(m_snap_angle_ref.y), grid_spacing );
				if( fabs(ddx) < 0.5 && fabs(ddy) < 0.5 )
				{
					// starting point is on-grid, snap to angle
					// snap to n*45 degree angle
					const double pi = 3.14159265359;
					double dx = wp.x - m_snap_angle_ref.x;
					double dy = wp.y - m_snap_angle_ref.y;
					double dist = sqrt( dx*dx + dy*dy );
					double dist45 = dist/sqrt(2.0);
					{
						int d;
						d = (int)(dist/grid_spacing+0.5);
						dist = d*grid_spacing;
						d = (int)(dist45/grid_spacing+0.5);
						dist45 = d*grid_spacing;
					}
					if( m_Doc->m_snap_angle == 45 )
					{
						// snap angle = 45 degrees, divide circle into 8 octants
						double angle = atan2( dy, dx );
						if( angle < 0.0 )
							angle = 2.0*pi + angle;
						angle += pi/8.0;
						double d_quad = angle/(pi/4.0);
						int oct = d_quad;
						switch( oct )
						{
						case 0:
							wp.x = m_snap_angle_ref.x + dist;
							wp.y = m_snap_angle_ref.y;
							break;
						case 1:
							wp.x = m_snap_angle_ref.x + dist45;
							wp.y = m_snap_angle_ref.y + dist45;
							break;
						case 2:
							wp.x = m_snap_angle_ref.x;
							wp.y = m_snap_angle_ref.y + dist;
							break;
						case 3:
							wp.x = m_snap_angle_ref.x - dist45;
							wp.y = m_snap_angle_ref.y + dist45;
							break;
						case 4:
							wp.x = m_snap_angle_ref.x - dist;
							wp.y = m_snap_angle_ref.y;
							break;
						case 5:
							wp.x = m_snap_angle_ref.x - dist45;
							wp.y = m_snap_angle_ref.y - dist45;
							break;
						case 6:
							wp.x = m_snap_angle_ref.x;
							wp.y = m_snap_angle_ref.y - dist;
							break;
						case 7:
							wp.x = m_snap_angle_ref.x + dist45;
							wp.y = m_snap_angle_ref.y - dist45;
							break;
						case 8:
							wp.x = m_snap_angle_ref.x + dist;
							wp.y = m_snap_angle_ref.y;
							break;
						default:
							ASSERT(0);
							break;
						}
					}
					else
					{
						// snap angle is 90 degrees, divide into 4 quadrants
						double angle = atan2( dy, dx );
						if( angle < 0.0 )
							angle = 2.0*pi + angle;
						angle += pi/4.0;
						double d_quad = angle/(pi/2.0);
						int quad = d_quad;
						switch( quad )
						{
						case 0:
							wp.x = m_snap_angle_ref.x + dist;
							wp.y = m_snap_angle_ref.y;
							break;
						case 1:
							wp.x = m_snap_angle_ref.x;
							wp.y = m_snap_angle_ref.y + dist;
							break;
						case 2:
							wp.x = m_snap_angle_ref.x - dist;
							wp.y = m_snap_angle_ref.y;
							break;
						case 3:
							wp.x = m_snap_angle_ref.x;
							wp.y = m_snap_angle_ref.y - dist;
							break;
						case 4:
							wp.x = m_snap_angle_ref.x + dist;
							wp.y = m_snap_angle_ref.y;
							break;
						default:
							ASSERT(0);
							break;
						}
					}
				}
			}
		}
		else
			m_snap_mode = SM_GRID_POINTS;

		// snap to grid points if needed
		if( m_snap_mode == SM_GRID_POINTS )
		{
			// snap to grid
			// get position in integral units of grid_spacing
			if( wp.x > 0 )
				wp.x = (wp.x + grid_spacing/2)/grid_spacing;
			else
				wp.x = (wp.x - grid_spacing/2)/grid_spacing;
			if( wp.y > 0 )
				wp.y = (wp.y + grid_spacing/2)/grid_spacing;
			else
				wp.y = (wp.y - grid_spacing/2)/grid_spacing;
			// multiply by grid spacing, adding or subracting 0.5 to prevent round-off
			// when using a fractional grid
			double test = wp.x * grid_spacing;
			if( test > 0.0 )
				test += 0.5;
			else
				test -= 0.5;
			wp.x = test;
			test = wp.y * grid_spacing;
			if( test > 0.0 )
				test += 0.5;
			else
				test -= 0.5;
			wp.y = test;
		}

		// update drag operation
		if( wp != m_last_cursor_point )
		{
			CDC *pDC = GetDC();
			pDC->SelectClipRgn( &m_pcb_rgn );
			SetDCToWorldCoords( pDC );
			if( (int)m_pcbu_per_pixel != mem_scale_factor || mem_1 != (int)m_org_x || mem_2 != (int)m_org_y )
			{
				m_dlist->Draw( pDC, NULL );
				mem_scale_factor = (int)m_pcbu_per_pixel;
				mem_1 = (int)m_org_x;
				mem_2 = (int)m_org_y;
				return;
			}
			else
				m_dlist->Drag( pDC, wp.x, wp.y );
			ReleaseDC( pDC );
			// show relative distance
			if( m_cursor_mode == CUR_DRAG_GROUP
			|| m_cursor_mode == CUR_DRAG_GROUP_ADD
			|| m_cursor_mode == CUR_DRAG_PART
			|| m_cursor_mode == CUR_DRAG_VTX
			|| m_cursor_mode ==  CUR_DRAG_OP_MOVE
			|| m_cursor_mode == CUR_DRAG_AREA_MOVE
			|| m_cursor_mode ==  CUR_DRAG_MEASURE_2
			|| m_cursor_mode == CUR_MOVE_SEGMENT
				)
			{
				ShowRelativeDistance( wp.x - m_from_pt.x, wp.y - m_from_pt.y );
			}
		}
	}
	mem_scale_factor = (int)m_pcbu_per_pixel;
	mem_1 = (int)m_org_x;
	mem_2 = (int)m_org_y;

	// update cursor position
	m_last_cursor_point = wp;
	ShowCursor();
}
//===============================================================================================
LONG CFreePcbView::OnChangeVisibleGrid( UINT wp, LONG lp )
{
	m_Doc->m_visual_grid_spacing = max(0,lp);
	m_dlist->SetVisibleGrid( TRUE, m_Doc->m_visual_grid_spacing );	
	m_Doc->ProjectModified( TRUE );
	Invalidate(FALSE);
	return 0;
}
//===============================================================================================
LONG CFreePcbView::OnChangePlacementGrid( UINT wp, LONG lp )
{
	m_Doc->m_part_grid_spacing = max(0,lp);
	m_Doc->ProjectModified( TRUE );
	Invalidate(FALSE);
	return 0;
}
//===============================================================================================
LONG CFreePcbView::OnChangeRoutingGrid( UINT wp, LONG lp )
{
	if( wp == WM_BY_INDEX )
		m_Doc->m_routing_grid_spacing = fabs( m_Doc->m_routing_grid[lp] );
	else
		ASSERT(0);
	SetFocus();
	m_Doc->ProjectModified( TRUE );
	return 0;
}
//===============================================================================================
LONG CFreePcbView::OnChangeSnapAngle( UINT wp, LONG lp )
{
	if( wp == WM_BY_INDEX )
	{
		if( lp == 0 )
		{
			m_Doc->m_snap_angle = 45;
			m_inflection_mode = IM_90_45;
		}
		else if( lp == 1 )
		{
			m_Doc->m_snap_angle = 90;
			m_inflection_mode = IM_90;
		}
		else
		{
			m_Doc->m_snap_angle = 0;
			m_inflection_mode = IM_NONE;
		}
	}
	else
		ASSERT(0);
	m_Doc->ProjectModified( TRUE );
	SetFocus();
	return 0;
}
//===============================================================================================
LONG CFreePcbView::OnChangeUnits( UINT wp, LONG lp )
{
	if( wp == WM_BY_INDEX )
	{
		if( lp == 0 )
			m_Doc->m_units = MIL;
		else
			m_Doc->m_units = MM;
	}
	else
		ASSERT(0);
	m_Doc->ProjectModified( TRUE );
	SetFocus();
	ShowSelectStatus();
	return 0;
}
//===============================================================================================
void CFreePcbView::OnSegmentDeleteTrace()
{
	OnRatlineDeleteConnection();
}
//===============================================================================================
void CFreePcbView::OnAreaCornerProperties()
{
	// reuse board corner dialog
	DlgEditBoardCorner dlg;
	CString str = "Corner Position";
	CPoint pt = m_Doc->m_nlist->GetAreaCorner( m_sel_net, m_sel_ia, m_sel_is );
	dlg.Init( &str, m_Doc->m_units, pt.x, pt.y );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		SaveUndoInfoForArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_MODIFY, TRUE, m_Doc->m_undo_list );
//		SaveUndoInfoForNetAndConnectionsAndArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_MODIFY, TRUE, m_Doc->m_undo_list );
		m_Doc->m_nlist->MoveAreaCorner( m_sel_net, m_sel_ia, m_sel_is,
			dlg.GetX(), dlg.GetY() );
		m_Doc->m_nlist->SetAreaConnections( m_sel_net, m_sel_ia );
		m_Doc->m_nlist->OptimizeConnections(  m_sel_net, -1, m_Doc->m_auto_ratline_disable,
														m_Doc->m_auto_ratline_min_pins, TRUE  );
		m_Doc->m_nlist->HighlightAreaCorner( m_sel_net, m_sel_ia, m_sel_is );
		SetCursorMode( CUR_AREA_CORNER_SELECTED );
		m_Doc->ProjectModified( TRUE );
	}
}
//===============================================================================================
void CFreePcbView::OnRefProperties()
{
	RefProperties();
	Invalidate( FALSE );
}
void CFreePcbView::RefProperties()
{
	CDlgRefText dlg;
	
	if (m_sel_part)
		dlg.Initialize( m_Doc->m_plist, m_sel_part, &m_Doc->m_footprint_cache_map );
	else
		return;
	id pid( ID_PART_DEF );
	NewSelect( m_sel_part, &pid, 0, 0 );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		// edit this parts
		if( getbit(m_sel_flags, FLAG_SEL_PART) )
			for(cpart* p=m_Doc->m_plist->GetFirstPart(); p; p=m_Doc->m_plist->GetNextPart(p))
				if( p->selected )
				{
					if( dlg.m_ref_not_use && p->m_ref_size == 0 )
						continue;
					m_Doc->m_plist->ResizeRefText(	p, 
													dlg.m_height, 
													dlg.m_width, 
													dlg.m_vis );
					RECT rTotal, rRef;
					if (dlg.m_def_pos)
					{
						if (m_Doc->m_plist->GetPartBoundingRect(p, &rTotal))
							if (m_Doc->m_plist->GetRefBoundingRect(p, &rRef))
							{
								int x, y, a;
								if ( (rTotal.right - rTotal.left) > (rTotal.top - rTotal.bottom) )
								{
									if ( p->side )
									{
										x = (rTotal.right + rTotal.left)/2 + max((rRef.right - rRef.left),(rRef.top - rRef.bottom))/2;
										a = p->angle;
									}
									else
									{
										x = (rTotal.right + rTotal.left)/2 - max((rRef.right - rRef.left),(rRef.top - rRef.bottom))/2;
										a = -p->angle;
									}
									y = (rTotal.top + rTotal.bottom)/2 - p->m_ref_size/2;
									if( p->m_value_size && p->m_value_vis )
										y += p->m_value_size/2 + p->m_ref_size/2 + p->m_value_w + p->m_ref_w;
								}
								else
								{
									x = (rTotal.right + rTotal.left)/2 + p->m_ref_size/2;
									if( p->m_value_size && p->m_value_vis )
										x -= (p->m_value_size/2 + p->m_ref_size/2 + p->m_value_w + p->m_ref_w);
									if ( p->side )
									{
										y = (rTotal.top + rTotal.bottom)/2 + max((rRef.right - rRef.left),(rRef.top - rRef.bottom))/2;
										a = (90+p->angle);
									}
									else
									{
										y = (rTotal.top + rTotal.bottom)/2 - max((rRef.right - rRef.left),(rRef.top - rRef.bottom))/2;
										a = (-90-p->angle);
									}
								}
								m_Doc->m_plist->MoveRefText(p,x,y,a,dlg.m_height,dlg.m_width);
							}
					}
				}
		m_Doc->ProjectModified( TRUE );
		m_dlist->CancelHighLight();
		if( m_cursor_mode == CUR_PART_SELECTED )
			m_Doc->m_plist->HighlightPart( m_sel_part );
		else if( m_cursor_mode == CUR_REF_SELECTED 
			&& m_sel_part->m_ref_size && m_sel_part->m_ref_vis )
			m_Doc->m_plist->SelectRefText( m_sel_part );
		else if( m_cursor_mode == CUR_GROUP_SELECTED )
		{
			if( m_sel_count == 1 && m_sel_part->dl_ref_el )
			{
				cpart * m_part = m_sel_part;
				CancelSelection();
				id ID( ID_REF_DEF );
				NewSelect( m_part, &ID, TRUE, FALSE );
			}
			else 
			{
				m_page = 1;
				SetFKText( m_cursor_mode );
			}
			HighlightGroup();
		}
		else
			CancelSelection();
	}
}
//===============================================================================================
void CFreePcbView::OnVertexProperties()
{
	DlgEditBoardCorner dlg;
	CString str = "Vertex Position";
	int x = m_sel_vtx.x;
	int y = m_sel_vtx.y;
	dlg.Init( &str, m_Doc->m_units, x, y );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY,
			TRUE, m_Doc->m_undo_list );
		m_dlist->CancelHighLight();
		m_Doc->m_nlist->MoveVertex( m_sel_net, m_sel_ic, m_sel_is,
			dlg.GetX(), dlg.GetY() );
		m_Doc->ProjectModified( TRUE );
		m_Doc->m_nlist->HighlightVertex( m_sel_net, m_sel_ic, m_sel_is );
	}
}
//===============================================================================================
BOOL CFreePcbView::OnEraseBkgnd(CDC* pDC)
{
	// Erase the left and bottom panes, the PCB area is always redrawn
	m_left_pane_invalid = TRUE;
	return FALSE;
}
//===============================================================================================
void CFreePcbView::OnOPSideConvertToStraightLine()
{
	m_Doc->m_dlist->CancelHighLight();
	m_Doc->m_outline_poly[m_sel_id.i].SetSideStyle( m_sel_id.ii, CPolyLine::STRAIGHT );
	m_Doc->m_outline_poly[m_sel_id.i].HighlightSide( m_sel_id.ii );
	ShowSelectStatus();
	SetFKText( m_cursor_mode );
	m_Doc->ProjectModified( TRUE );
}
//===============================================================================================
void CFreePcbView::OnOPSideConvertToArcCw()
{
	m_Doc->m_dlist->CancelHighLight();
	m_Doc->m_outline_poly[m_sel_id.i].SetSideStyle( m_sel_id.ii, CPolyLine::ARC_CW );
	m_Doc->m_outline_poly[m_sel_id.i].HighlightSide( m_sel_id.ii );
	ShowSelectStatus();
	SetFKText( m_cursor_mode );
	m_Doc->ProjectModified( TRUE );
}
//===============================================================================================
void CFreePcbView::OnOPSideConvertToArcCcw()
{
	m_Doc->m_dlist->CancelHighLight();
	m_Doc->m_outline_poly[m_sel_id.i].SetSideStyle( m_sel_id.ii, CPolyLine::ARC_CCW );
	m_Doc->m_outline_poly[m_sel_id.i].HighlightSide( m_sel_id.ii );
	ShowSelectStatus();
	SetFKText( m_cursor_mode );
	m_Doc->ProjectModified( TRUE );
}
//===============================================================================================
// unroute entire connection
//
void CFreePcbView::OnUnrouteTrace()
{
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
	cconnect * c = &m_sel_con;
	for( int is=0; is<c->nsegs; is++ )
	{
		m_sel_net->connect[m_sel_ic].seg[is].width = 0;
		m_sel_net->connect[m_sel_ic].seg[is].layer = LAY_RAT_LINE;
	}
	m_Doc->m_nlist->MergeUnroutedSegments( m_sel_net, m_sel_ic );
	m_Doc->m_nlist->SetAreaConnections( m_sel_net );
	m_dlist->CancelHighLight();
	m_Doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, 0, m_seg_clearance );
	m_sel_id.st = ID_SEG;
	m_sel_id.ii = 0;
	SetCursorMode( CUR_RAT_SELECTED );
	m_Doc->ProjectModified( TRUE );
}
//===============================================================================================
// save undo info for a group, for UNDO_GROUP_MODIFY or UNDO_GROUP_DELETE
// creates
//
int CFreePcbView::SaveUndoInfoForGroup( int type, CUndoList * list, BOOL wMerge, BOOL bMessageBox )
{
	if( !(type == UNDO_GROUP_MODIFY || type == UNDO_GROUP_DELETE || type == UNDO_GROUP_ADD ))
		return IDCANCEL;

	list->NewEvent();
	undo_group_descriptor * undo = new undo_group_descriptor;//ok
	undo->view = this;
	undo->list = list;
	undo->type = type;
	undo->str.SetSize( m_sel_count );
	undo->m_id.SetSize( m_sel_count );

	// Save Undo Info For all Connections
	if( type == UNDO_GROUP_ADD )
	{	
		int iu = 0;
		SaveUndoInfoForOutlinePoly( UNDO_OP_ADD, FALSE, list );
		if( getbit( m_sel_flags, FLAG_SEL_OP ) )
			for( int item=m_Doc->m_outline_poly.GetSize()-1; item>=0; item-- )
			{
				//set size
				int ncorn = m_Doc->m_outline_poly[item].GetNumCorners();
				if( iu+ncorn >= undo->str.GetSize() )
				{
					undo->str.SetSize( iu+ncorn+10 );
					undo->m_id.SetSize( iu+ncorn+10 );
				}
				int nsides=0, ncorners=0;
				int gnc = ncorn-1;
				int cl = m_Doc->m_outline_poly[item].GetClosed();
				for( int i=gnc; i>=0; i-- )
				{
					int gs, gss;
					gs = m_Doc->m_outline_poly[item].GetSel(i);
					if( i == gnc && cl == 0)
						gss = 0;
					else
						gss = m_Doc->m_outline_poly[item].GetSideSel(i);
					if( gss )
					{
						undo->m_id[iu] = m_Doc->m_outline_poly[item].GetId();
						undo->m_id[iu].sst = ID_SIDE;
						undo->m_id[iu].ii = i;
						undo->str[iu] = "";
						iu++;
					}
					if( gs )
					{
						undo->m_id[iu] = m_Doc->m_outline_poly[item].GetId();
						undo->m_id[iu].sst = ID_CORNER;
						undo->m_id[iu].ii = i;
						undo->str[iu] = "";
						iu++;
					}
				}
			}
		if( getbit(m_sel_flags, FLAG_SEL_PART) )
		{
			//set size
			int n_parts = m_Doc->m_plist->GetNumParts();
			if( iu+n_parts >= undo->str.GetSize() )
			{
				undo->str.SetSize( iu+n_parts+10 );
				undo->m_id.SetSize( iu+n_parts+10 );
			}
			for( cpart* p=m_Doc->m_plist->GetFirstPart(); p; p=m_Doc->m_plist->GetNextPart(p) ) 
				if( p->selected )
				{
					undo->m_id[iu].Set( ID_PART_DEF );
					undo->str[iu] = p->ref_des;
					iu++;
					SaveUndoInfoForPart( p, CPartList::UNDO_PART_ADD, &p->ref_des, FALSE, list );
				}
		}
		if( getbit(m_sel_flags, FLAG_SEL_NET) )
			for( cnet* n=m_Doc->m_nlist->GetFirstNet(); n; n=m_Doc->m_nlist->GetNextNet(/*LABEL*/) )
				if( n->selected )
				{
					for( int ia=n->nareas-1; ia>=0; ia-- )
					{
						if( n->area[ia].selected )
						{
							//set size
							int n_a_corners = 2*n->area[ia].poly->GetNumCorners();
							if( iu+n_a_corners >= undo->str.GetSize() )
							{
								undo->str.SetSize( iu+n_a_corners+10 );
								undo->m_id.SetSize( iu+n_a_corners+10 );
							}
							for( int cont=n->area[ia].poly->GetNumContours()-1; cont>=0; cont-- )
							{
								int start_c = n->area[ia].poly->GetContourStart(cont);
								int end_c = n->area[ia].poly->GetContourEnd(cont);
								for( int cor=start_c; cor<=end_c; cor++ )
								{
									if( n->area[ia].poly->GetSideSel(cor) )
									{
										undo->m_id[iu].Set( ID_NET, ID_AREA, ia, ID_SIDE, cor );
										undo->str[iu] = n->name;
										iu++;
									}
									if( n->area[ia].poly->GetSel(cor) )
									{
										undo->m_id[iu].Set( ID_NET, ID_AREA, ia, ID_CORNER, cor );
										undo->str[iu] = n->name;
										iu++;
									}
								}
							}
						}
					}
					for( int con=n->nconnects-1; con>=0; con-- )
					{
						if( n->connect[con].m_selected )
						{
							//set size
							int n_obj = 2*n->connect[con].nsegs+1;
							if( iu+n_obj >= undo->str.GetSize() )
							{
								undo->str.SetSize( iu+n_obj+10 );
								undo->m_id.SetSize( iu+n_obj+10 );
							}
							for( int seg=n->connect[con].nsegs-1; seg>=0; seg-- )
							{
								if( n->connect[con].seg[seg].selected )
								{
									undo->m_id[iu].Set( ID_NET, ID_CONNECT, con, ID_SEG, seg );
									undo->str[iu] = n->name;
									iu++;
								}
								if( n->connect[con].vtx[seg+1].selected )
								{
									undo->m_id[iu].Set( ID_NET, ID_CONNECT, con, ID_VERTEX, seg+1 );
									undo->str[iu] = n->name;
									iu++;
								}
							}
						}
					}
				}
		if( getbit( m_sel_flags, FLAG_SEL_TEXT ) )
		{
			//set size
			int ntexts = m_Doc->m_tlist->GetNumTexts();
			if( iu+ntexts >= undo->str.GetSize() )
			{
				undo->str.SetSize( iu+ntexts+10 );
				undo->m_id.SetSize( iu+ntexts+10 );
			}
			int it = 0;
			for( CText* t=m_Doc->m_tlist->GetFirstText(); t; t=m_Doc->m_tlist->GetNextText(&it) )
				if( t->m_selected )
				{
					undo->m_id[iu].Set(ID_TEXT_DEF);
					::GetStringFromGuid( &t->m_guid, &undo->str[iu] );
					iu++;
					SaveUndoInfoForText( t, CTextList::UNDO_TEXT_ADD, FALSE, list );
				}
		}
		undo->str.SetSize( iu );
		undo->m_id.SetSize( iu );
		// now save undo descriptor 
		list->Push( UNDO_GROUP_ADD, undo, &UndoGroupCallback, iu );
	}
	else
	{
		int n_sel_count = m_Doc->m_plist->GetSelCount();
		if( n_sel_count )
			if( type == UNDO_GROUP_DELETE )
			{
				// delete part		
				CString mess;
				mess.Format( "Delete selected parts? (%d)", n_sel_count );
				int ret = IDOK;
				if( bMessageBox )
					ret = AfxMessageBox( mess, MB_YESNOCANCEL );
				if( ret == IDCANCEL )
				{
					delete undo;
					return IDCANCEL;
				}
				if( ret == IDNO )
				{
					id pid( ID_PART_DEF );
					for( cpart * p=m_Doc->m_plist->GetFirstPart(); p; p=m_Doc->m_plist->GetNextPart(p) )
					{
						if( p->selected )
							UnSelect( p, &pid );
					}
				}
				else if( m_Doc->m_netlist_completed && bMessageBox )
				{
					BOOL f = 0;
					id pid( ID_PART_DEF );
					for( cnet* n=m_Doc->m_nlist->GetFirstNet(); n; n=m_Doc->m_nlist->GetNextNet(/*LABEL*/) )
					{
						for( int ip=0; ip<n->npins; ip++ )
							if( n->pin[ip].part )
								if( n->pin[ip].part->selected && n->pin[ip].part->shape)
									if( n->pin[ip].part->shape->GetNumPins() > 1 )
							      	{
										UnSelect( n->pin[ip].part, &pid );
										f = 1;
									}
					}
					if( f && g_bShow_nl_lock_Warning )
					{
						CString str = "Sorry, netlist is protected. Can not delete part used in the netlist";
						CDlgMyMessageBox dlg;
						dlg.Initialize( str );
						dlg.DoModal();
						g_bShow_nl_lock_Warning = !dlg.bDontShowBoxState;
					}
				}
			}
		int iu = 0;
		for( cnet* n=m_Doc->m_nlist->GetFirstNet(); n; n=m_Doc->m_nlist->GetNextNet(/*LABEL*/) )
		{
			int num_save = -1;
			for( int con=0; con<n->nconnects; con++ )
			{
				cconnect * c = &n->connect[con];
				if( c->m_selected )
				{
					num_save = con;
					break;
				}
				cpart * p1 = n->pin[c->start_pin].part;
				cpart * p2 = 0;
				if( c->end_pin >= 0 )
					p2 = n->pin[c->end_pin].part;
				if( p1->selected )
				{
					num_save = con;
					break;
				}
				if( p2 )
					if( p2->selected )
					{
						num_save = con;
						break;
					}
			}
			if( num_save >= 0 )
				SaveUndoInfoForNetAndConnections( n, CNetList::UNDO_NET_MODIFY, FALSE, list );
		}
		if( getbit( m_sel_flags, FLAG_SEL_NET ) )
		{
			for( cnet* n=m_Doc->m_nlist->GetFirstNet(); n; n=m_Doc->m_nlist->GetNextNet(/*LABEL*/) )
			{
				if( n->selected )
				{
					// areas
					if( getbit(m_sel_flags, FLAG_SEL_AREA) )
					{
						//
						for( int ia=n->nareas-1; ia>=0; ia-- )
						{
							if( n->area[ia].selected )
							{
								n->area[ia].poly->Undraw();
								//set size
								int n_a_corners = 2*n->area[ia].poly->GetNumCorners();
								if( iu+n_a_corners >= undo->str.GetSize() )
								{
									undo->str.SetSize( iu+n_a_corners+10 );
									undo->m_id.SetSize( iu+n_a_corners+10 );
								}
								int area_saved = 0;
								int removing = 0;
								for( int cont=n->area[ia].poly->GetNumContours()-1; cont>=0; cont-- )
								{
									int nsides=0, ncorners=0;
									int start_c = n->area[ia].poly->GetContourStart(cont);
									int end_c = n->area[ia].poly->GetContourEnd(cont);
									for( int cor=start_c; cor<=end_c; cor++ )
									{
										if( n->area[ia].poly->GetSideSel(cor) )
										{
											undo->m_id[iu].Set( ID_NET, ID_AREA, ia, ID_SIDE, cor );
											undo->str[iu] = n->name;
											iu++;
											nsides++;
										}
										if( n->area[ia].poly->GetSel(cor) )
										{
											undo->m_id[iu].Set( ID_NET, ID_AREA, ia, ID_CORNER, cor );
											undo->str[iu] = n->name;
											iu++;
											ncorners++;
										}
									}
									if( nsides < (end_c-start_c) || type == UNDO_GROUP_MODIFY )
									{
										// will be modified
										if( !area_saved )
											SaveUndoInfoForArea( n, ia, CNetList::UNDO_AREA_MODIFY, FALSE, list );
										area_saved++;
										if( type == UNDO_GROUP_DELETE )
											DeleteWithRecalculationPoint( n->area[ia].poly, 0, wMerge );
									}
									else if( cont )
									{
										// will be modified
										if( !area_saved )
											SaveUndoInfoForArea( n, ia, CNetList::UNDO_AREA_MODIFY, FALSE, list );
										area_saved = 1;
										n->area[ia].poly->RemoveContour(cont, 0);
									}
									else
									{
										// will be deleted
										SaveUndoInfoForArea( n, ia, CNetList::UNDO_AREA_DELETE, FALSE, list );
										m_Doc->m_nlist->RemoveArea(n,ia);
										removing++;
										continue;
									}
								}	
								if( removing == 0 )
								{
									for( int cont=n->area[ia].poly->GetNumContours()-1; cont>=0; cont-- )
										n->area[ia].poly->RecalcRectC( cont );
									n->area[ia].poly->Draw();
								}
							}
						}
					}
				}
			}
			for( cnet* n=m_Doc->m_nlist->GetFirstNet(); n; n=m_Doc->m_nlist->GetNextNet(/*LABEL*/) )
			{
				if( n->selected )
				{
					// connects
					if( getbit(m_sel_flags, FLAG_SEL_CONNECT) )
					{
						int removing = 0;
						for( int con=n->nconnects-1; con>=0; con-- )
						{
							if( n->connect[con].m_selected )
							{
								//set size
								int n_obj = 2*n->connect[con].nsegs+1;
								if( iu+n_obj >= undo->str.GetSize() )
								{
									undo->str.SetSize( iu+n_obj+10 );
									undo->m_id.SetSize( iu+n_obj+10 );
								}
								int nsides = 0, nvertices=0;
								for( int seg=n->connect[con].nsegs-1; seg>=0; seg-- )
								{
									if( n->connect[con].seg[seg].selected )
									{
										if( type == UNDO_GROUP_MODIFY && iu >= m_sel_count )
										{
											AfxMessageBox( "Error function SaveUndoInfoForGroup.cpp ( iu > m_sel_count )" );
											m_Doc->m_nlist->CancelNextNet();
											delete undo;
											return IDCANCEL;
										}
										undo->m_id[iu].Set( ID_NET, ID_CONNECT, con, ID_SEG, seg );
										undo->str[iu] = n->name;
										iu++;
										nsides++;
									}
									if( n->connect[con].vtx[seg+1].selected )
									{
										if( type == UNDO_GROUP_MODIFY && iu >= m_sel_count )
										{
											AfxMessageBox( "Error function SaveUndoInfoForGroup.cpp ( iu > m_sel_count )" );
											m_Doc->m_nlist->CancelNextNet();
											delete undo;
											return IDCANCEL;
										}
										undo->m_id[iu].Set( ID_NET, ID_CONNECT, con, ID_VERTEX, seg+1 );
										undo->str[iu] = n->name;
										iu++;
										nvertices++;
									}
								}
								//
								if( nsides < n->connect[con].nsegs || type == UNDO_GROUP_MODIFY )
								{
									// will be modified
									m_Doc->m_nlist->UndrawConnection(n,con);
									if( type == UNDO_GROUP_DELETE )
									{
										int i_highlight = DeleteWithRecalculationPoint( 0, &n->connect[con], wMerge );
										if( i_highlight >= 0 )
										{
											id Id( ID_NET, ID_CONNECT, con, ID_VERTEX, i_highlight );
											NewSelect( n, &Id, 0, 0 );
											m_sel_net = n;
											m_sel_id = Id;
										}
										int sw,vw,hw;
										for( int vtx=0; vtx<n->connect[con].nsegs; vtx++ )
										{
											if( n->connect[con].vtx[vtx+1].via_w == 0 )
											{
												sw = n->connect[con].seg[vtx].width;
												vw = hw = 0;
												GetWidthsForSegment(&sw,&vw,&hw);
												m_Doc->m_nlist->ReconcileVia( n, con, vtx+1, TRUE, vw, hw );
											}
										}
									}
									m_Doc->m_nlist->DrawConnection(n,con);
								}
								else
								{
									// will be deleted
									if( n->connect[con].vtx[n->connect[con].nsegs].tee_ID )
										m_Doc->m_nlist->DisconnectBranch( n, con );
									m_Doc->m_nlist->RemoveNetConnect( n, con, FALSE );
									removing++;
								}	
							}
						}
					}			
				}
			}
			m_Doc->m_nlist->RepairAllBranches( TRUE );
		}
		if( getbit(m_sel_flags, FLAG_SEL_PART) )
		{
			//set size
			int n_parts = m_Doc->m_plist->GetNumParts();
			if( iu+n_parts >= undo->str.GetSize() )
			{
				undo->str.SetSize( iu+n_parts+10 );
				undo->m_id.SetSize( iu+n_parts+10 );
			}
			int pRemove = 0;
			for( cpart* p=m_Doc->m_plist->GetFirstPart(); p; p=m_Doc->m_plist->GetNextPart(p) )
			{
				if( pRemove )
				{
					cpart * prevp = m_Doc->m_plist->GetPrevPart(p);
					if( prevp )
					{
						m_Doc->m_nlist->PartDeleted(prevp);
						m_Doc->m_plist->Remove(prevp);
					}
				}
				pRemove = 0;
				if( p->selected )
				{
					undo->m_id[iu].Set( ID_PART_DEF );
					undo->str[iu] = p->ref_des;
					iu++;
					if( type == UNDO_GROUP_MODIFY )
					{
						// will be modified
						SaveUndoInfoForPart( p, CPartList::UNDO_PART_MODIFY, NULL, FALSE, list );
					}
					else
					{
						// will be deleted
						SaveUndoInfoForPart( p, CPartList::UNDO_PART_DELETE, NULL, FALSE, list );
						if( p->shape )
						{
							for( int pin=p->shape->GetNumPins()-1; pin>=0; pin-- )
							{
								if( p->pin[pin].net )
								{
									cnet * pnn = p->pin[pin].net;
									for( int pc=pnn->nconnects-1; pc>=0; pc-- )
									{
										cconnect * pcon = &pnn->connect[pc];
										cpart * part_con = pnn->pin[pcon->start_pin].part;									
										if( p == part_con && pcon->nsegs >= 1 )
										{
											if( pcon->end_pin == cconnect::NO_END ||
												( pcon->nsegs == 1 && pcon->seg[0].layer == LAY_RAT_LINE ))
											{
												if( pnn->connect[pc].vtx[pnn->connect[pc].nsegs].tee_ID )
													m_Doc->m_nlist->DisconnectBranch( pnn, pc );
												m_Doc->m_nlist->RemoveNetConnect( pnn, pc, 0 );
											}
											else 
												m_Doc->m_nlist->ReverseNetConnect(pnn, pc, TRUE);
										}
										else if( pcon->end_pin != cconnect::NO_END )
										{
											part_con = pnn->pin[pcon->end_pin].part;
											if( p == part_con )
											{
												if( pcon->nsegs == 1 && pcon->seg[0].layer == LAY_RAT_LINE )
													m_Doc->m_nlist->RemoveNetConnect( pnn, pc, 0 );
												else
													m_Doc->m_nlist->ReverseNetConnect(pnn, pc, FALSE);
											}
										}
									}
								}
							}
							pRemove = 1;
						}
					}
				}
			}
			if( pRemove )
			{
				cpart * endp = m_Doc->m_plist->GetEndPart();
				if( endp )
				{
					m_Doc->m_nlist->PartDeleted(endp);
					m_Doc->m_plist->Remove(endp);
				}
			}
		}
		// for texts
		if( getbit( m_sel_flags, FLAG_SEL_TEXT ) )
		{
			//set size
			int ntexts = m_Doc->m_tlist->GetNumTexts();
			if( iu+ntexts >= undo->str.GetSize() )
			{
				undo->str.SetSize( iu+ntexts+10 );
				undo->m_id.SetSize( iu+ntexts+10 );
			}
			int it = 0;
			for( CText* t=m_Doc->m_tlist->GetFirstText(); t; t=m_Doc->m_tlist->GetNextText(&it) )
			{
				if( t->m_selected )
				{
					undo->m_id[iu].Set(ID_TEXT_DEF);
					::GetStringFromGuid( &t->m_guid, &undo->str[iu] );
					iu++;
					if( type == UNDO_GROUP_MODIFY )
					{
						// will be modified
						SaveUndoInfoForText( t, CTextList::UNDO_TEXT_MODIFY, FALSE, list );
					}
					else
					{
						// will be deleted
						SaveUndoInfoForText( t, CTextList::UNDO_TEXT_DELETE, FALSE, list );
						m_Doc->m_tlist->RemoveText( t );
						it--;
					}
				}
			}
		}

		// save undo info for all outlines
		SaveUndoInfoForOutlinePoly( UNDO_OP, FALSE, list );

		// for outline
		if( getbit( m_sel_flags, FLAG_SEL_OP ) )
		{
			BOOL bDel = FALSE;
			for( int item=m_Doc->m_outline_poly.GetSize()-1; item>=0; item-- )
			{
				//set size
				int n_corners = m_Doc->m_outline_poly[item].GetNumCorners();
				if( iu+n_corners >= undo->str.GetSize() )
				{
					undo->str.SetSize( iu+n_corners+10 );
					undo->m_id.SetSize( iu+n_corners+10 );
				}
				int nsides=0, ncorners=0;
				int gnc = m_Doc->m_outline_poly[item].GetNumCorners()-1;
				int cl = m_Doc->m_outline_poly[item].GetClosed();
				for( int i=gnc; i>=0; i-- )
				{
					int gs, gss;
					gs = m_Doc->m_outline_poly[item].GetSel(i);
					if( i == gnc && cl == 0 )
						gss = 0;
					else
						gss = m_Doc->m_outline_poly[item].GetSideSel(i);
					if( gss )
					{
						undo->m_id[iu] = m_Doc->m_outline_poly[item].GetId();
						undo->m_id[iu].sst = ID_SIDE;
						undo->m_id[iu].ii = i;
						undo->str[iu] = "";
						iu++;
						nsides++;
					}
					if( gs )
					{
						undo->m_id[iu] = m_Doc->m_outline_poly[item].GetId();
						undo->m_id[iu].sst = ID_CORNER;
						undo->m_id[iu].ii = i;
						undo->str[iu] = "";
						iu++;
						ncorners++;
					}
				}
				if( nsides < gnc || type == UNDO_GROUP_MODIFY )
				{
					// will be modified
					if( type == UNDO_GROUP_DELETE )
					{
						m_Doc->m_outline_poly[item].Undraw();
						DeleteWithRecalculationPoint( &m_Doc->m_outline_poly[item], 0, wMerge );
						for( int i_cont=m_Doc->m_outline_poly[item].GetNumContours()-1; i_cont>=0; i_cont-- )
							m_Doc->m_outline_poly[item].RecalcRectC(i_cont);
						m_Doc->m_outline_poly[item].Draw();
					}
				}
				else
				{
					// will be deleted
					m_Doc->m_outline_poly[item].Undraw();
					m_Doc->m_outline_poly.RemoveAt(item);
					bDel = 1;
				}
			}
			if( bDel )
				m_Doc->OPRefreshID();
		}
		undo->str.SetSize( iu );
		undo->m_id.SetSize( iu );
		// now save undo descriptor 
		list->Push( UNDO_GROUP, undo, &UndoGroupCallback, iu );
	}
	//
	if( type != UNDO_GROUP_MODIFY )
	{
		m_sel_count = GetSelCount();
		if( GetSelectedItem() == 0 )
			CancelSelection();
	}
	prev_sel_count = m_sel_count;
	return IDOK;
}
//===============================================================================================
// save undo info for two existing parts and all nets connected to them,
// assuming that the parts will be modified (not deleted or added)
//
void CFreePcbView::SaveUndoInfoFor2PartsAndNets( cpart * part1, cpart * part2, BOOL new_event, CUndoList * list )
{
	void * ptr;
	cpart * part;

	if( new_event )
		list->NewEvent();
	for( int i=0; i<2; i++ )
	{
		if( i==0 )
			part = part1;
		else
			part = part2;
		for( int ip=0; ip<part->pin.GetSize(); ip++ )
		{
			cnet * net = (cnet*)part->pin[ip].net;
			if( net )
				net->utility = 0;
		}
		for( int ip=0; ip<part->pin.GetSize(); ip++ )
		{
			cnet * net = (cnet*)part->pin[ip].net;
			if( net )
			{
				if( net->utility == 0 )
				{
					for( int ic=0; ic<net->nconnects; ic++ )
					{
						undo_con * u_con = m_Doc->m_nlist->CreateConnectUndoRecord( net, ic );
						list->Push( CNetList::UNDO_CONNECT_MODIFY, u_con,
							&m_Doc->m_nlist->ConnectUndoCallback, u_con->size );
					}
					undo_net * u_net = m_Doc->m_nlist->CreateNetUndoRecord( net );
					list->Push( CNetList::UNDO_NET_MODIFY, u_net,
						&m_Doc->m_nlist->NetUndoCallback, u_net->size );
					net->utility = 1;
				}
			}
		}
	}
	// now save undo info for parts
	undo_part * u_part1 = m_Doc->m_plist->CreatePartUndoRecord( part1, NULL );
	list->Push( CPartList::UNDO_PART_MODIFY, u_part1,
		&m_Doc->m_plist->PartUndoCallback, u_part1->size );
	undo_part * u_part2 = m_Doc->m_plist->CreatePartUndoRecord( part2, NULL );
	list->Push( CPartList::UNDO_PART_MODIFY, u_part2,
		&m_Doc->m_plist->PartUndoCallback, u_part2->size );
	// now save undo descriptor
	if( new_event )
	{
		ptr = CreateUndoDescriptor( list, 0, &part1->ref_des, &part2->ref_des, 0, 0, NULL, NULL );
		list->Push( UNDO_2_PARTS_AND_NETS, ptr, &UndoCallback );
	}
	return;
}
//===============================================================================================
// save undo info for net, all connections and all areas
//
void CFreePcbView::SaveUndoInfoForNetAndConnectionsAndAreas( cnet * net, CUndoList * list, int UNDO_AREA )
{
	SaveUndoInfoForNetAndConnections( net,
		CNetList::UNDO_NET_MODIFY, FALSE, list );
	for( int ia=0; ia<net->nareas; ia++ )
		SaveUndoInfoForArea( net, ia, UNDO_AREA, FALSE, list );
}
//===============================================================================================
// save undo info for net, all connections and
// a single copper area
// type may be:
//	CNetList::UNDO_AREA_MODIFY	if area will be modified
//	CNetList::UNDO_AREA_DELETE	if area will be deleted
//
void CFreePcbView::SaveUndoInfoForNetAndConnectionsAndArea( cnet * net, int iarea,
														   int type, BOOL new_event, CUndoList * list )
{
	if( new_event )
	{
		list->NewEvent();
	}
	SaveUndoInfoForArea( net, iarea, type, FALSE, m_Doc->m_undo_list );
	SaveUndoInfoForNetAndConnections( net,
		CNetList::UNDO_NET_MODIFY, FALSE, m_Doc->m_undo_list );
	// now save undo descriptor
	if( new_event )
	{
		void * ptr = CreateUndoDescriptor( list, type, &net->name, NULL, iarea, 0, NULL, NULL );
		list->Push( UNDO_NET_AND_CONNECTIONS_AND_AREA, ptr, &UndoCallback );
	}
}
//===============================================================================================
// save undo info for a copper area to be modified or deleted
// type may be:
//	CNetList::UNDO_AREA_ADD	if area will be added
//	CNetList::UNDO_AREA_MODIFY	if area will be modified
//	CNetList::UNDO_AREA_DELETE	if area will be deleted
//
void CFreePcbView::SaveUndoInfoForArea( cnet * net, int iarea, int type, BOOL new_event, CUndoList * list )
{
	void *ptr;
	if( new_event )
	{
		list->NewEvent();
		SaveUndoInfoForNet( net, CNetList::UNDO_NET_OPTIMIZE, FALSE, list );
	}
	int nc = 1;
	if( type != CNetList::UNDO_AREA_ADD )
	{
		nc = net->area[iarea].poly->GetNumContours();
		if( !net->area[iarea].poly->GetClosed() )
			nc--;
	}
	if( nc > 0 )
	{
		undo_area * undo = m_Doc->m_nlist->CreateAreaUndoRecord( net, iarea, type );
		list->Push( type, (void*)undo, &m_Doc->m_nlist->AreaUndoCallback, undo->size );
	}
	// now save undo descriptor
	if( new_event )
	{
		ptr = CreateUndoDescriptor( list, type, &net->name, NULL, iarea, 0, NULL, NULL );
		list->Push( UNDO_AREA, ptr, &UndoCallback );
	}
}
//===============================================================================================
// save undo info for all of the areas in a net
//
void CFreePcbView::SaveUndoInfoForAllAreasInNet( cnet * net, BOOL new_event, CUndoList * list )
{
	if( new_event )
	{
		list->NewEvent();		// flag new undo event
		SaveUndoInfoForNet( net, CNetList::UNDO_NET_OPTIMIZE, FALSE, list );
	}
	for( int ia=net->area.GetSize()-1; ia>=0; ia-- )
		SaveUndoInfoForArea( net, ia, CNetList::UNDO_AREA_DELETE, FALSE, list );
	undo_area * u_area = m_Doc->m_nlist->CreateAreaUndoRecord( net, 0, CNetList::UNDO_AREA_CLEAR_ALL );
	list->Push( CNetList::UNDO_AREA_CLEAR_ALL, u_area, &m_Doc->m_nlist->AreaUndoCallback, u_area->size );
	// now save undo descriptor
	if( new_event )
	{
		void * ptr = CreateUndoDescriptor( list, 0, &net->name, NULL, 0, 0, NULL, NULL );
		list->Push( UNDO_ALL_AREAS_IN_NET, ptr, &UndoCallback );
	}
}
//===============================================================================================
// save undo info for all of the areas in two nets
//
void CFreePcbView::SaveUndoInfoForAllAreasIn2Nets( cnet * net1, cnet * net2, BOOL new_event, CUndoList * list )
{
	if( new_event )
	{
		list->NewEvent();		// flag new undo event
		SaveUndoInfoForNet( net1, CNetList::UNDO_NET_OPTIMIZE, FALSE, list );
		SaveUndoInfoForNet( net2, CNetList::UNDO_NET_OPTIMIZE, FALSE, list );
	}
	SaveUndoInfoForAllAreasInNet( net1, FALSE, list );
	SaveUndoInfoForAllAreasInNet( net2, FALSE, list );
	// now save undo descriptor
	if( new_event )
	{
		void * ptr = CreateUndoDescriptor( list, 0, &net1->name, &net2->name, 0, 0, NULL, NULL );
		list->Push( UNDO_ALL_AREAS_IN_2_NETS, ptr, &UndoCallback );
	}
}
//===============================================================================================
// save undo info for all nets (but not areas)
//
void CFreePcbView::SaveUndoInfoForAllNets( BOOL new_event, CUndoList * list )
{
	POSITION pos;
	CString name;
	CMapStringToPtr * m_map = &m_Doc->m_nlist->m_map;
	void * net_ptr;
	if( new_event )
		list->NewEvent();		// flag new undo event
	// traverse map of nets
	for( pos = m_map->GetStartPosition(); pos != NULL; )
	{
		// next net
		m_map->GetNextAssoc( pos, name, net_ptr );
		cnet * net = (cnet*)net_ptr;
		void * ptr;
		// loop through all connections in net
		for( int ic=0; ic<net->nconnects; ic++ )
		{
			undo_con * u_con = m_Doc->m_nlist->CreateConnectUndoRecord( net, ic );
			list->Push( CNetList::UNDO_CONNECT_MODIFY, u_con,
				&m_Doc->m_nlist->ConnectUndoCallback, u_con->size );
		}
		undo_net * u_net = m_Doc->m_nlist->CreateNetUndoRecord( net );
		list->Push( CNetList::UNDO_NET_MODIFY, u_net,
			&m_Doc->m_nlist->NetUndoCallback, u_net->size );
	}
}

//===============================================================================================
void CFreePcbView::SaveUndoInfoForMoveOrigin( int x_off, int y_off, CUndoList * list )
{
	// now push onto undo list
	undo_move_origin * undo = m_Doc->CreateMoveOriginUndoRecord( x_off, y_off );
	list->NewEvent();
	list->Push( 0, (void*)undo, &m_Doc->MoveOriginUndoCallback );
	// save top-level descriptor
	void * ptr = CreateUndoDescriptor( list, 0, NULL, NULL, x_off, y_off, NULL, NULL );
	list->Push( UNDO_MOVE_ORIGIN, ptr, &UndoCallback );
}
//===============================================================================================
void CFreePcbView::SaveUndoInfoForOutlinePoly( int type, BOOL new_event, CUndoList * list )
{
	// push undo info onto list
	if( new_event )
		list->NewEvent();		// flag new undo event

	// push all closed cutouts onto undo list
	for( int i=0; i<m_Doc->m_outline_poly.GetSize(); i++ )
	{
		CPolyLine * poly = &m_Doc->m_outline_poly[i];
		if( type == UNDO_OP_ADD )
		{
			// check sel
			BOOL add = TRUE;
			int gnc = poly->GetNumCorners()-1;
			int cl = poly->GetClosed();
			for( int sm=gnc; sm>=0; sm-- )
				if( poly->GetSel(sm) )
					add = FALSE;
				else if( cl || sm < gnc )
				{
					if( poly->GetSideSel(sm) )
						add = FALSE;
				}
			if( !add )
				continue;
		}
		undo_outline_poly * undo = m_Doc->CreateOutlinePolyUndoRecord( poly, i );
		list->Push( UNDO_OP, (void*)undo, &m_Doc->OutlinePolyUndoCallback );
	}
	// create UNDO_SM_CUTOUT_CLEAR_ALL record and push it
	list->Push( UNDO_OP_CLEAR_ALL, NULL, &m_Doc->OutlinePolyUndoCallback );
	// now push top-level callback for redoing
	if( new_event )
	{
		void * ptr = CreateUndoDescriptor( list, 0, NULL, NULL, 0, 0, NULL, NULL );
		list->Push( UNDO_ALL_OP, ptr, &UndoCallback );
	}
}
//===============================================================================================
void CFreePcbView::SaveUndoInfoForText( CText * text, int type, BOOL new_event, CUndoList * list )
{
	// create new undo record and push onto undo list
	undo_text * undo = m_Doc->m_tlist->CreateUndoRecord( text );
	if( new_event )
		list->NewEvent();		// flag new undo event
	list->Push( type, (void*)undo, &m_Doc->m_tlist->TextUndoCallback );
	if( new_event )
	{
		void * ptr = CreateUndoDescriptor( list, type, NULL, NULL, 0, 0, NULL, (void *)undo );
		list->Push( UNDO_TEXT, ptr, &UndoCallback );
	}
}
//===============================================================================================
void CFreePcbView::SaveUndoInfoForText( undo_text * u_text, int type, BOOL new_event, CUndoList * list )
{
	// copy undo record and push onto undo list
	undo_text * undo = new undo_text;//ok
	*undo = *u_text;
	if( new_event )
		list->NewEvent();		// flag new undo event
	list->Push( type, (void*)undo, &m_Doc->m_tlist->TextUndoCallback );
	if( new_event )
	{
		void * ptr = CreateUndoDescriptor( list, type, NULL, NULL, 0, 0, NULL, (void*)undo );
		list->Push( UNDO_TEXT, ptr, &UndoCallback );
	}
}
//===============================================================================================
void CFreePcbView::OnViewEntireBoard()
{
	ViewEntireBoard();
	Invalidate( FALSE );//OnViewEntireBoard
}
void CFreePcbView::ViewEntireBoard()
{
	if( m_Doc->m_outline_poly.GetSize() )
	{
		// get boundaries of board outline
		int max_x = INT_MIN;
		int min_x = INT_MAX;
		int max_y = INT_MIN;
		int min_y = INT_MAX;
		for( int ib=0; ib<m_Doc->m_outline_poly.GetSize(); ib++ )
		{
			for( int ic=0; ic<m_Doc->m_outline_poly[ib].GetNumCorners(); ic++ )
			{
				id bid = m_Doc->m_outline_poly[ib].GetId();
				if( bid.st == ID_BOARD )
				{
					if( m_Doc->m_outline_poly[ib].GetX( ic ) > max_x )
						max_x = m_Doc->m_outline_poly[ib].GetX( ic );
					if( m_Doc->m_outline_poly[ib].GetX( ic ) < min_x )
						min_x = m_Doc->m_outline_poly[ib].GetX( ic );
					if( m_Doc->m_outline_poly[ib].GetY( ic ) > max_y )
						max_y = m_Doc->m_outline_poly[ib].GetY( ic );
					if( m_Doc->m_outline_poly[ib].GetY( ic ) < min_y )
						min_y = m_Doc->m_outline_poly[ib].GetY( ic );
				}
			}
		}
		// reset window to enclose board outline
//		m_org_x = min_x - (max_x - min_x)/20;	// in pcbu
//		m_org_y = min_y - (max_y - min_y)/20;	// in pcbu
		double x_pcbu_per_pixel = 1.1 * (double)(max_x - min_x)/(m_client_r.right - m_left_pane_w);
		double y_pcbu_per_pixel = 1.1 * (double)(max_y - min_y)/(m_client_r.bottom - m_bottom_pane_h);
		m_pcbu_per_pixel = max( x_pcbu_per_pixel, y_pcbu_per_pixel );
		m_org_x = (max_x + min_x)/2 - (m_client_r.right - m_left_pane_w)/2 * m_pcbu_per_pixel;
		m_org_y = (max_y + min_y)/2 - (m_client_r.bottom - m_bottom_pane_h)/2 * m_pcbu_per_pixel;
		CRect screen_r;
		GetWindowRect( &screen_r );		// in pixels
		m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel,
			m_org_x, m_org_y );
		// extent board outlines
		CMainFrame * pMain = (CMainFrame*) AfxGetApp()->m_pMainWnd;
		if( !pMain ) return;
		CString str, x_str, y_str;
		::MakeCStringFromDimension( &x_str, max_x-min_x, m_Doc->m_units, FALSE, FALSE, FALSE, m_Doc->m_units==MIL?1:3 );
		::MakeCStringFromDimension( &y_str, max_y-min_y, m_Doc->m_units, FALSE, FALSE, FALSE, m_Doc->m_units==MIL?1:3 );
		str.Format( "Extent: x=%s y=%s", x_str, y_str );
		pMain->DrawStatus( 3, &str );
	}
	else
	{
		AfxMessageBox( "Board outline does not exist" );
	}
}
//===============================================================================================
RECT CFreePcbView::ViewAllElements()
{
	// reset window to enclose all elements
	BOOL bOK = FALSE;
	/*RECT r;
	r.left = r.bottom = INT_MAX;
	r.right = r.top = INT_MIN;
	// parts
	int test = m_Doc->m_plist->GetPartBoundaries( &r );
	if( test != 0 )
		bOK = TRUE;
	int max_x = r.right;
	int min_x = r.left;
	int max_y = r.top;
	int min_y = r.bottom;
	// polylines
	for( int ib=0; ib<m_Doc->m_outline_poly.GetSize(); ib++ )
	{
		id bid = m_Doc->m_outline_poly[ib].GetId();
		r = m_Doc->m_outline_poly[ib].GetBounds();
		max_x = max( max_x, r.right );
		min_x = min( min_x, r.left );
		max_y = max( max_y, r.top );
		min_y = min( min_y, r.bottom );
		bOK = TRUE;
	}
	// nets
	if( m_Doc->m_nlist->GetNetBoundaries( &r, 0 ) )
	{
		max_x = max( max_x, r.right );
		min_x = min( min_x, r.left );
		max_y = max( max_y, r.top );
		min_y = min( min_y, r.bottom );
		bOK = TRUE;
	}
	// texts
	if( m_Doc->m_tlist->GetTextBoundaries( &r ) )
	{
		max_x = max( max_x, r.right );
		min_x = min( min_x, r.left );
		max_y = max( max_y, r.top );
		min_y = min( min_y, r.bottom );
		bOK = TRUE;
	}*/
	//
	int map = 0;
	m_Doc->m_dlist->HighlightAll();
	RECT rct = m_Doc->m_dlist->GetHighlightedBounds(&map);
	m_Doc->m_dlist->CancelHighLight();
	if( rct.right - rct.left > 0 )
	{
		int min_x = max( -DEFAULT/2, rct.left );
		int min_y = max( -DEFAULT/2, rct.bottom );
		int max_x = min( DEFAULT/2, rct.right );
		int max_y = min( DEFAULT/2, rct.top );
		if( max_x < min_x || max_y < min_y )
		{
			min_x = -DEFAULT/2;
			min_y = -DEFAULT/2;
			max_x =  DEFAULT/2;
			max_y =  DEFAULT/2;
		}
		min_x -= 99999;
		min_y -= 99999;
		max_x += 99999;
		max_y += 99999;
		rct = rect(min_x,min_y,max_x,max_y);
		double x_pcbu_per_pixel = 1.1 * (double)(max_x - min_x)/(m_client_r.right - m_left_pane_w);
		double y_pcbu_per_pixel = 1.1 * (double)(max_y - min_y)/(m_client_r.bottom - m_bottom_pane_h);
		m_pcbu_per_pixel = max( x_pcbu_per_pixel, y_pcbu_per_pixel );
		m_org_x = (max_x + min_x)/2 - (m_client_r.right - m_left_pane_w)/2 * m_pcbu_per_pixel;
		m_org_y = (max_y + min_y)/2 - (m_client_r.bottom - m_bottom_pane_h)/2 * m_pcbu_per_pixel;
		CRect screen_r;
		GetWindowRect( &screen_r );
		m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel,
			m_org_x, m_org_y );
	}
	return rct;
}
void CFreePcbView::OnViewAllElements()
{
	ViewAllElements();
	Invalidate( FALSE );//OnViewAllElements
}
//===============================================================================================
void CFreePcbView::OnAreaEdgeHatchStyle()
{
	CDlgSetAreaHatch dlg;
	dlg.Init( m_sel_net->area[m_sel_id.i].poly->GetHatch() );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		int hatch = dlg.GetHatch();
		m_sel_net->area[m_sel_id.i].poly->SetHatch( hatch );
		m_Doc->ProjectModified( TRUE );
	}
}
//===============================================================================================
void CFreePcbView::OnPartEditFootprint()
{
	if( m_sel_part )
		if( m_sel_part->shape )
		{
			m_sel_part->shape->origin_moved_X = 0;
			m_sel_part->shape->origin_moved_Y = 0;
		}
	theApp.OnViewFootprint();
}
//===============================================================================================
void CFreePcbView::OnPartEditThisFootprint()
{
	m_Doc->m_edit_footprint = TRUE;
	if( m_sel_part )
		if( m_sel_part->shape )
		{
			m_sel_part->shape->origin_moved_X = 0;
			m_sel_part->shape->origin_moved_Y = 0;
		}
	theApp.OnViewFootprint();
}
//===============================================================================================
// Offer new footprint from the Footprint Editor
//
void CFreePcbView::OnExternalChangeFootprint( CShape * fp )
{
	CString str;
	str.Format( "Do you wish to replace the footprint of part \"%s\"\nwith the new footprint \"%s\" ?",
		m_sel_part->ref_des, fp->m_name );
	int ret = AfxMessageBox( str, MB_YESNO );
	if( ret == IDYES )
	{
		// OK, see if a footprint of the same name is already in the cache
		void * ptr;
		BOOL found = m_Doc->m_footprint_cache_map.Lookup( fp->m_name, ptr );
		if( found )
		{
			// see how many parts are using it, not counting the current one
			CShape * old_fp = (CShape*)ptr;
			int num = m_Doc->m_plist->GetNumFootprintInstances( old_fp );
			if( m_sel_part->shape == old_fp )
				num--;
			if( num <= 0 )
			{
				// go ahead and replace it
				m_Doc->m_plist->UndrawPart( m_sel_part );
				old_fp->Copy( fp );
				m_Doc->m_plist->PartFootprintChanged( m_sel_part, old_fp );
				m_Doc->ResetUndoState();
			}
			else
			{
				// offer to overwrite or rename it
				CDlgDupFootprintName dlg;
				CString mess;
				mess.Format( "Warning: A footprint named \"%s\"\r\nis already in use by other parts.\r\n", fp->m_name );
				mess += "Loading this new footprint will overwrite the old one\r\nunless you change its name\r\n";
				dlg.Initialize( &mess, &m_Doc->m_footprint_cache_map );
				int ret = dlg.DoModal();
				if( ret == IDOK )
				{
					// clicked "OK"
					if( dlg.m_replace_all_flag )
					{
						// replace all instances of footprint
						old_fp->Copy( fp );
						m_Doc->m_plist->FootprintChanged( old_fp );
						m_Doc->ResetUndoState();
					}
					else
					{
						// assign new name to footprint and put in cache
						CShape * shape = new CShape;//ok?
						shape->Copy( fp );
						shape->m_name = *dlg.GetNewName();
						m_Doc->m_footprint_cache_map.SetAt( shape->m_name, shape );
						m_Doc->m_plist->PartFootprintChanged( m_sel_part, shape );
						m_Doc->ResetUndoState();
					}
				}
			}
		}
		else
		{
			// footprint name not found in cache, add the new footprint
			CShape * shape = new CShape;//ok?
			shape->Copy( fp );
			m_Doc->m_footprint_cache_map.SetAt( shape->m_name, shape );
			m_Doc->m_plist->PartFootprintChanged( m_sel_part, shape );
			m_Doc->ResetUndoState();
		}
		m_Doc->ProjectModified( TRUE );
	}	
	m_dlist->CancelHighLight();
	SelectPart( m_sel_part );
	Invalidate( FALSE );//OnExternalChangeFootprint
}
//===============================================================================================
// find a part in the layout, center window on it and select it
//
void CFreePcbView::OnViewFindpart()
{
	CDlgFindPart dlg;
	dlg.Initialize( m_Doc->m_plist );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		CString * ref_des = &dlg.sel_ref_des;
		cpart * part = m_Doc->m_plist->GetPart( *ref_des );
		if( part )
		{
			if( part->shape )
			{
				dl_element * dl_sel = part->dl_sel;
				RECT Get;
				m_dlist->Get_Rect( dl_sel, &Get );
				int xc = ( Get.right + Get.left )/2;
				int yc = ( Get.top +   Get.bottom )/2;
				m_org_x = xc - ((m_client_r.right-m_left_pane_w)*m_pcbu_per_pixel)/2;
				m_org_y = yc - ((m_client_r.bottom-m_bottom_pane_h)*m_pcbu_per_pixel)/2;
				CRect screen_r;
				GetWindowRect( &screen_r );
				m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel,
					m_org_x, m_org_y );
				CPoint p(xc, yc);
				p = m_dlist->PCBToScreen( p );
				SetCursorPos( p.x, p.y - 4 );
				CancelSelection();
				m_sel_part = part;
				m_sel_id = part->m_id;
				m_sel_id.st = ID_SEL_RECT;
				SelectPart( m_sel_part );
				gLastKeyWasArrow = FALSE;
				SetCursorMode( CUR_PART_SELECTED );
			}
			else
			{
				AfxMessageBox( "Sorry, this part doesn't have a footprint" );
			}
		}
		else
		{
			AfxMessageBox( "Sorry, this part doesn't exist" );
		}
	}
	Invalidate( FALSE );//OnViewFindpart
}
//===============================================================================================
void CFreePcbView::OnFootprintWizard()
{
	m_Doc->OnToolsFootprintwizard();
}
//===============================================================================================
void CFreePcbView::OnFootprintEditor()
{
	theApp.OnViewFootprint();
}
//===============================================================================================
void CFreePcbView::OnCheckPartsAndNets()
{
	m_Doc->OnToolsCheckPartsAndNets();
}
//===============================================================================================
void CFreePcbView::OnDrc()
{
	m_Doc->OnToolsDrc();
}
//===============================================================================================
void CFreePcbView::OnClearDRC()
{
	m_Doc->OnToolsClearDrc();
}

//===============================================================================================
void CFreePcbView::AddOutlinePoly( BOOL bREPEAT, BOOL bEDIT )
{
	BOOL NEW=TRUE;
	SaveUndoInfoForOutlinePoly( UNDO_OP, TRUE, m_Doc->m_undo_list );
	//
	if( m_sel_id.type == ID_POLYLINE )
		if( m_sel_id.i < m_Doc->m_outline_poly.GetSize() )
			if( m_sel_id.ii < m_Doc->m_outline_poly[m_sel_id.i].GetNumCorners() )
			{
				m_polyline_layer = m_Doc->m_outline_poly[m_sel_id.i].GetLayer();
				m_polyline_hatch = m_Doc->m_outline_poly[m_sel_id.i].GetHatch();				
				if( bEDIT == 0 )
				{
					m_sel_id.sst = ID_CORNER;
					m_sel_id.ii = m_Doc->m_outline_poly[m_sel_id.i].GetNumCorners();
				}
				NEW = FALSE;
			}

	CDlgAddMaskCutout dlg;
	dlg.Initialize( m_polyline_layer, m_polyline_hatch );
	int ret;
	if(( bREPEAT && m_polyline_layer ) || ( NEW == 0 && bEDIT == 0 ))
	{
		ret = IDOK;
		dlg.m_layer = m_polyline_layer;
		dlg.m_hatch = m_polyline_hatch;
	}
	else
		ret = dlg.DoModal(); 
	if( ret == IDOK )
	{
		m_polyline_layer = dlg.m_layer;
		m_polyline_hatch = dlg.m_hatch;
		m_polyline_style = CPolyLine::STRAIGHT;

		// closed
		if( m_polyline_layer == LAY_BOARD_OUTLINE ||
			m_polyline_layer == LAY_SM_TOP ||
			m_polyline_layer == LAY_SM_BOTTOM )
			m_polyline_closed = 1;
		
		// force layer visible
		m_Doc->m_vis[m_polyline_layer] = TRUE;
		m_dlist->SetLayerVisible( m_polyline_layer, TRUE );

		// new outline
		if( NEW )
		{
			int sz = m_Doc->m_outline_poly.GetSize();
			m_Doc->m_outline_poly.SetSize(sz+1);
			m_Doc->m_outline_poly[sz].SetDisplayList( m_Doc->m_dlist );
			if( m_polyline_layer == LAY_BOARD_OUTLINE )
				m_sel_id.Set( ID_POLYLINE, ID_BOARD, sz, ID_CORNER, 0 );
			else if( m_polyline_layer == LAY_SM_TOP || 
					 m_polyline_layer == LAY_SM_BOTTOM )
				m_sel_id.Set( ID_POLYLINE, ID_SM_CUTOUT, sz, ID_CORNER, 0 );
			else
			{
				m_sel_id.Set( ID_POLYLINE, ID_GRAPHIC, sz, ID_CORNER, 0 );
				if( !bREPEAT || !m_polyline_width )
					if( OPSetWidth() != IDOK )
						return;
			}
		}
		if( NEW || bEDIT == 0 )
		{
			// start dragging rectangle
			CDC *pDC = GetDC();
			pDC->SelectClipRgn( &m_pcb_rgn );
			SetDCToWorldCoords( pDC );
			m_dlist->CancelHighLight();
			SetCursorMode( CUR_ADD_OP );
			m_dlist->StartDraggingArray( pDC, m_last_cursor_point.x,
				m_last_cursor_point.y, 2 );
			ReleaseDC( pDC );
		}
		else
		{
			m_Doc->m_outline_poly[m_sel_id.i].SetLayer( m_polyline_layer );
			m_Doc->m_outline_poly[m_sel_id.i].SetHatch( m_polyline_hatch );
			if( m_polyline_closed )
			{
				if( m_polyline_closed != m_Doc->m_outline_poly[m_sel_id.i].GetClosed() )
				{
					if( m_Doc->m_outline_poly[m_sel_id.i].GetNumCorners() < 3 )
						m_Doc->m_outline_poly[m_sel_id.i].AppendCorner( m_Doc->m_outline_poly[m_sel_id.i].GetX(0),
																		m_Doc->m_outline_poly[m_sel_id.i].GetY(0), 0 );
					m_Doc->m_outline_poly[m_sel_id.i].Close(0);
				}
			}
		}
	}
}
void CFreePcbView::OnAddOutlinePoly()
{
	AddOutlinePoly(0);
	Invalidate( FALSE );//OnAddOutlinePoly
}
//===============================================================================================
int CFreePcbView::OPSetWidth()
{
	int lay = m_Doc->m_outline_poly[m_sel_id.i].GetLayer();
	BOOL clOnly = ( lay == LAY_BOARD_OUTLINE || lay == LAY_SM_TOP || lay == LAY_SM_BOTTOM );
	CDlgAddPoly dlg;
	dlg.Initialize( m_Doc->m_units, 
					m_Doc->m_outline_poly[m_sel_id.i].GetW(), 
					m_Doc->m_outline_poly[m_sel_id.i].GetClosed(),
					0,clOnly);
	int ret = dlg.DoModal(); 
	if( ret == IDOK )
	{
		m_polyline_closed = dlg.GetClosedFlag();
		if( m_sel_id.type == ID_POLYLINE )
		{
			m_polyline_width = dlg.GetWidth();
			m_Doc->m_outline_poly[m_sel_id.i].SetW( m_polyline_width );
			BOOL cl = m_Doc->m_outline_poly[m_sel_id.i].GetClosed();
			if( cl != m_polyline_closed )
			{
				int ncor = m_Doc->m_outline_poly[m_sel_id.i].GetNumCorners();
				if( ncor )
					if( cl )
					{
						m_Doc->m_outline_poly[m_sel_id.i].UnClose();
						if( m_sel_id.ii >= ncor-1 )
						{
							m_sel_id.ii--;
							m_Doc->m_dlist->CancelHighLight();
							m_Doc->m_outline_poly[m_sel_id.i].HighlightSide( m_sel_id.ii, m_Doc->m_outline_poly[m_sel_id.i].GetW() );
						}
					}
					else
						m_Doc->m_outline_poly[m_sel_id.i].Close();
			}
			else
				m_Doc->m_outline_poly[m_sel_id.i].Draw( m_Doc->m_dlist );
		}
	}
	return ret;
}
void CFreePcbView::OnOPSetWidth()
{
	OPSetWidth();
	Invalidate( FALSE );
}
//===============================================================================================
// change hatch style for outline-polyline
void CFreePcbView::OPHatchStyle()
{
	AddOutlinePoly( FALSE, TRUE );
}

void CFreePcbView::OnOPHatchStyle()
{
	AddOutlinePoly( FALSE, TRUE );
	Invalidate( FALSE );
}
//===============================================================================================
// change side of part
void CFreePcbView::OnPartChangeSide()
{
	SaveUndoInfoForPartAndNets( m_sel_part,
		CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
	m_Doc->m_dlist->CancelHighLight();
	m_Doc->m_plist->UndrawPart( m_sel_part );
	m_sel_part->side = 1 - m_sel_part->side;
	m_Doc->m_plist->DrawPart( m_sel_part );
	m_Doc->m_nlist->PartMoved( m_sel_part , TRUE );
	m_Doc->m_nlist->OptimizeConnections( m_sel_part, m_Doc->m_auto_ratline_disable, 
										m_Doc->m_auto_ratline_min_pins );
	m_Doc->m_plist->HighlightPart( m_sel_part );
	ShowSelectStatus();
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );//OnPartChangeSide
}
//===============================================================================================
// rotate part clockwise 90 degrees clockwise
//
void CFreePcbView::OnPartRotate()
{
	SaveUndoInfoForPartAndNets( m_sel_part,
		CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
	m_Doc->m_dlist->CancelHighLight();
	m_Doc->m_plist->UndrawPart( m_sel_part );
	m_sel_part->angle = (m_sel_part->angle + 90)%360;
	m_Doc->m_plist->DrawPart( m_sel_part );
	m_Doc->m_nlist->PartMoved( m_sel_part , TRUE );
	m_Doc->m_nlist->OptimizeConnections( m_sel_part, m_Doc->m_auto_ratline_disable, 
										m_Doc->m_auto_ratline_min_pins );
	m_Doc->m_plist->HighlightPart( m_sel_part );
	ShowSelectStatus();
	m_Doc->ProjectModified( TRUE );
}
//===============================================================================================
void CFreePcbView::OnPartRotateCCW()
{
	SaveUndoInfoForPartAndNets( m_sel_part,
		CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
	m_Doc->m_dlist->CancelHighLight();
	m_Doc->m_plist->UndrawPart( m_sel_part );
	m_sel_part->angle = (m_sel_part->angle + 270)%360;
	m_Doc->m_plist->DrawPart( m_sel_part );
	m_Doc->m_nlist->PartMoved( m_sel_part , TRUE );
	m_Doc->m_nlist->OptimizeConnections( m_sel_part, m_Doc->m_auto_ratline_disable, 
										m_Doc->m_auto_ratline_min_pins );
	m_Doc->m_plist->HighlightPart( m_sel_part );
	ShowSelectStatus();
	m_Doc->ProjectModified( TRUE );
}
//===============================================================================================
void CFreePcbView::OnNetSetWidth()
{
	SetWidth( 2 );
	m_Doc->m_dlist->CancelHighLight();
	m_Doc->m_nlist->HighlightNetConnections( m_sel_net );
}
//===============================================================================================
void CFreePcbView::OnConnectSetWidth()
{
	SetWidth( 1 );
	m_Doc->m_dlist->CancelHighLight();
	m_Doc->m_nlist->HighlightConnection( m_sel_net, m_sel_ic );
}
//===============================================================================================
void CFreePcbView::OnSegmentAddVertex()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	SetCursorMode( CUR_DRAG_VTX_INSERT );
	m_Doc->m_nlist->StartDraggingSegmentNewVertex( pDC, m_sel_net, m_sel_ic, m_sel_is,
		p.x, p.y, m_sel_net->connect[m_sel_ic].seg[m_sel_is].layer,
		m_sel_net->connect[m_sel_ic].seg[m_sel_is].width, 2 );
}
//===============================================================================================
void CFreePcbView::OnConnectUnroutetrace()
{
	OnUnrouteTrace();
}
//===============================================================================================
void CFreePcbView::OnConnectDeletetrace()
{
	OnSegmentDeleteTrace();
}
//===============================================================================================
void CFreePcbView::OnSegmentChangeLayer()
{
	ChangeTraceLayer( 0, m_sel_seg.layer );
}
//===============================================================================================
void CFreePcbView::OnConnectChangeLayer()
{
	ChangeTraceLayer( 1 );
}
//===============================================================================================
void CFreePcbView::OnNetChangeLayer()
{
	ChangeTraceLayer( 2 );
}
//===============================================================================================
// change layer of routed trace segments
// if mode = 0, current segment
// if mode = 1, current connection
// if mode = 2, current net
//
void CFreePcbView::ChangeTraceLayer( int mode, int old_layer )
{
	CDlgChangeLayer dlg;
	dlg.Initialize( mode, old_layer, m_Doc->m_num_copper_layers );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		int err = 0;
		SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
		cconnect * c = &m_sel_con;
		int w=m_sel_seg.width;
		int vw=0, vh=0;
		if( dlg.m_apply_to == 0 )
		{
			GetWidthsForSegment( &w, &vw, &vh );
			err = m_Doc->m_nlist->ChangeSegmentLayer( m_sel_net,
						m_sel_id.i, m_sel_id.ii, dlg.m_new_layer,vw, vh );
			if( err )
			{
				AfxMessageBox( "Unable to change layer for this segment" );
			}
		}
		else if( dlg.m_apply_to == 1 )
		{
			for( int is=0; is<c->nsegs; is++ )
			{
				if( c->seg[is].layer >= LAY_TOP_COPPER )
				{
					w = c->seg[is].width;
					GetWidthsForSegment( &w, &vw, &vh );
					err += m_Doc->m_nlist->ChangeSegmentLayer( m_sel_net,
						m_sel_id.i, is, dlg.m_new_layer, vw, vh );
				}
			}
			if( err )
			{
				AfxMessageBox( "Unable to change layer for all segments" );
			}
		}
		else if( dlg.m_apply_to == 2 )
		{
			for( int ic=0; ic<m_sel_net->nconnects; ic++ )
			{
				cconnect * c = &m_sel_net->connect[ic];
				for( int is=0; is<c->nsegs; is++ )
				{
					if( c->seg[is].layer >= LAY_TOP_COPPER )
					{
						w = c->seg[is].width;
						GetWidthsForSegment( &w, &vw, &vh );
						err += m_Doc->m_nlist->ChangeSegmentLayer( m_sel_net,
							ic, is, dlg.m_new_layer, vw, vh );
					}
				}
			}
			if( err )
			{
				AfxMessageBox( "Unable to change layer for all segments" );
			}
		}
		m_Doc->ProjectModified( TRUE );
	}
}
//===============================================================================================
void CFreePcbView::OnNetEditnet()
{
	Editnet();
	Invalidate( FALSE );
}
//===============================================================================================
void CFreePcbView::Editnet( int i_pin )
{
	//SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
	CDlgEditNet dlg;
	netlist_info nl;
	m_Doc->m_nlist->ExportNetListInfo( &nl );
	int inet = -1;
	for( int i=0; i<nl.GetSize(); i++ )
	{
		if( nl[i].net == m_sel_net )
		{
			inet = i;
			break;
		}
	}
	if( inet == -1 )
		ASSERT(0);
	CString Spin = "";
	if( i_pin >= 0 )
		Spin = m_sel_net->pin[i_pin].part->ref_des + "." + m_sel_net->pin[i_pin].pin_name;
	dlg.Initialize( &nl, inet, m_Doc->m_plist, FALSE, m_sel_net->visible, m_Doc->m_units, m_Doc->m_netlist_completed,
		&m_Doc->m_w, &m_Doc->m_v_w, &m_Doc->m_v_h_w, &Spin );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		m_Doc->ResetUndoState();
		CancelSelection();
		m_Doc->m_nlist->ImportNetListInfo( &nl, 0, NULL,
			m_Doc->m_trace_w, m_Doc->m_via_w, m_Doc->m_via_hole_w );
	}
}
//===============================================================================================
void CFreePcbView::OnToolsMoveOrigin()
{
	CDlgMoveOrigin dlg;
	dlg.Initialize( m_Doc->m_units );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		if( dlg.m_drag )
		{
			CDC *pDC = GetDC();
			pDC->SelectClipRgn( &m_pcb_rgn );
			SetDCToWorldCoords( pDC );
			m_dlist->CancelHighLight();
			SetCursorMode( CUR_MOVE_ORIGIN );
			m_dlist->StartDraggingArray( pDC, m_last_cursor_point.x,
				m_last_cursor_point.y, 2 );
			ReleaseDC( pDC );
			Invalidate( FALSE );
		}
		else
		{
			SaveUndoInfoForMoveOrigin( -dlg.m_x, -dlg.m_x, m_Doc->m_undo_list );
			MoveOrigin( -dlg.m_x, -dlg.m_y );
			OnViewAllElements();
		}
	}
}
//===============================================================================================
// move origin of coord system by moving everything
// by (x_off, y_off)
//
void CFreePcbView::MoveOrigin( int x_off, int y_off )
{
	for( int ib=0; ib<m_Doc->m_outline_poly.GetSize(); ib++ )
		m_Doc->m_outline_poly[ib].MoveOrigin( x_off, y_off );
	m_Doc->m_plist->MoveOrigin( x_off, y_off );
	m_Doc->m_nlist->MoveOrigin( x_off, y_off );
	m_Doc->m_tlist->MoveOrigin( x_off, y_off );
}
//===============================================================================================
void CFreePcbView::OnLButtonDown(UINT nFlags, CPoint point)
{
	// save starting position in pixels
	m_bLButtonDown = TRUE;
	m_bDraggingRect = FALSE;
	m_start_pt = point;
	CView::OnLButtonDown(nFlags, point);
}
//===============================================================================================
// Select all parts and trace segments in rectangle
// Fill flag selected
// Set utility flags for selected parts and segments
//
void CFreePcbView::SelectItemsInRect( CRect r, UINT nFlags )
{
#define SET_M 0 //set cursor mode (dis)
	int EN_INV = 0, UNSEL = 0;
	//
	// reset page for group_selected
	m_page = 1;
	//
	if( m_last_mouse_point.x == r.left )
	{
		if( m_sel_count )
			UNSEL = 1;
		else
			return;
	}

	r.NormalizeRect();

	// mark all merges
	m_Doc->m_mlist->mark0();

	// find parts in rect
	if( m_sel_mask & (1<<SEL_MASK_PARTS ) )
	{
		cpart * part = m_Doc->m_plist->GetFirstPart();
		while( part )
		{
			if( !part->selected || UNSEL )
			{
				CRect p_r;
				if( m_Doc->m_plist->GetPartBoundingRect( part, &p_r ) )
				{
					p_r.NormalizeRect();
					if( (part->side == 0 && m_Doc->m_vis[LAY_TOP_COPPER] == 0) || (part->side == 1 && m_Doc->m_vis[LAY_BOTTOM_COPPER] == 0))
					{
						//isEmpty
					}
					else if( InRange( p_r.top, r.top, r.bottom )
						&& InRange( p_r.bottom, r.top, r.bottom )
						&& InRange( p_r.left, r.left, r.right )
						&& InRange( p_r.right, r.left, r.right ) )
					{
						// add part to selection list and highlight it
						id pid( ID_PART_DEF );
						int mrg = -1;
						if( getbit( m_sel_mask, SEL_MASK_MERGES ) )
							mrg = part->m_merge;
						//
						if( UNSEL )
							UnSelect( part, &pid, SET_M );
						//else if ( mrg >= 0 && !EN_INV )
						//	NewSelectM( NULL, mrg );
						else
							NewSelect( part, &pid, SET_M, EN_INV );
					}
				}
			}
			part = m_Doc->m_plist->GetNextPart( part );
		}
	}
	// find trace segments and vertices contained in rect
	// first via and then segment!!!
	if( m_sel_mask & (1<<SEL_MASK_VIA ))
	{
		cnet * net = m_Doc->m_nlist->GetFirstNet();
		while( net )
		{
			for( int ic=0; ic<net->nconnects; ic++ )
			{
				cconnect * c = &net->connect[ic];
				BOOL bPREV_SEG_SELECTED = TRUE;
				for( int is=0; is<c->nsegs; is++ )
				{
					cvertex * post_v = &c->vtx[is+1];
					cseg * s1 = &c->seg[is];
					cseg * s2 = &c->seg[min(c->nsegs-1,is+1)];
					BOOL bPostV =( InRange( post_v->x, r.left, r.right ) && InRange( post_v->y, r.top, r.bottom ));
					if( bPostV )
						if( (c->end_pin == cconnect::NO_END && post_v->tee_ID == 0) || is+1 < c->nsegs )
							if( m_Doc->m_vis[s1->layer] || m_Doc->m_vis[s2->layer] )
							{
								// add vertex to selection list
								id vid( ID_NET, ID_CONNECT, ic, ID_VERTEX, is+1 );
								if( UNSEL )
									UnSelect( net, &vid, SET_M );
								else 
									NewSelect( net, &vid, SET_M, EN_INV );
					}			
				}
			}
			net = m_Doc->m_nlist->GetNextNet(/*LABEL*/);
		}
	}
	/// and then segment..
	if( m_sel_mask & (1<<SEL_MASK_CON ) )
	{
		cnet * net = m_Doc->m_nlist->GetFirstNet();
		while( net )
		{
			for( int ic=0; ic<net->nconnects; ic++ )
			{
				cconnect * c = &net->connect[ic];
				BOOL bPREV_SEG_SELECTED = TRUE;
				for( int is=0; is<c->nsegs; is++ )
				{
					cvertex * pre_v = &c->vtx[is];
					cvertex * post_v = &c->vtx[is+1];
					cseg * s = &c->seg[is];
					BOOL bPreV = InRange( pre_v->x, r.left, r.right )
						&& InRange( pre_v->y, r.top, r.bottom );
					BOOL bPostV = InRange( post_v->x, r.left, r.right )
						&& InRange( post_v->y, r.top, r.bottom );
					if( bPreV && bPostV )
					{
						// add segment to selection list
						if( m_Doc->m_vis[s->layer] )
						{
							id sid( ID_NET, ID_CONNECT, ic, ID_SEG, is );
							if( UNSEL )
								UnSelect( net, &sid, SET_M );
							else
							{
								NewSelect( net, &sid, SET_M, EN_INV );
								sid.sst = ID_VERTEX;
								UnSelect( net, &sid, 0 );
								sid.ii++;
								UnSelect( net, &sid, 0 );
							}
						}
					}			
				}
			}
			net = m_Doc->m_nlist->GetNextNet(/*LABEL*/);
		}
	}

	// find texts in rect
	if( m_sel_mask & (1<<SEL_MASK_TEXT ) )
	{
		CText * t = m_Doc->m_tlist->GetFirstText();
		int it = 0;
		while( t )
		{
			if( !t->m_selected || UNSEL )
			{
				RECT Get;
				m_dlist->Get_Rect( t->dl_sel, &Get );
				if( m_Doc->m_vis[t->m_layer] )
					if( InRange( Get.left, r.left, r.right ) && 
						InRange( Get.right, r.left, r.right ) && 
						InRange( Get.top, r.top, r.bottom ) && 
						InRange( Get.bottom, r.top, r.bottom ) )
					{
						// add text to selection list and highlight it
						id sid(ID_TEXT_DEF);
						if( UNSEL )
							UnSelect( t, &sid, SET_M );
						else
							NewSelect( t, &sid, SET_M, EN_INV );
					}
			}
			t = m_Doc->m_tlist->GetNextText(&it);
		}
	}

	// find copper area sides in rect
	if( m_sel_mask & (1<<SEL_MASK_AREAS ) )
	{
		cnet * net = m_Doc->m_nlist->GetFirstNet();
		while( net )
		{
			if( net->nareas )
			{
				for( int ia=0; ia<net->nareas; ia++ )
				{
					carea * a = &net->area[ia];
					CPolyLine * poly = a->poly;
					RECT arect;
					arect = a->poly->GetBounds();
					if (arect.left > r.right)
						continue;
					if (r.left > arect.right)
						continue;
					if (arect.bottom > r.bottom)
						continue;
					if (r.top > arect.top)
						continue;
					for( int ic=0; ic<poly->GetNumContours(); ic++ )
					{
						int istart = poly->GetContourStart(ic);
						int iend = poly->GetContourEnd(ic);
						BOOL M = FALSE;
						for( int is=istart; is<=iend; is++ )
						{
							if( poly->GetSideSel( is ) == 0 || UNSEL )
							{
								int ic1, ic2;
								ic1 = is;
								if( is < iend )
									ic2 = is+1;
								else
									ic2 = istart;
								int x1 = poly->GetX(ic1);
								int y1 = poly->GetY(ic1);
								int x2 = poly->GetX(ic2);
								int y2 = poly->GetY(ic2);
								if( m_Doc->m_vis[poly->GetLayer()] )
								{
									if( InRange( x1, r.left, r.right )
										&& InRange( x2, r.left, r.right )
										&& InRange( y1, r.top, r.bottom )
										&& InRange( y2, r.top, r.bottom ) )
									{
										id aid( ID_NET, ID_AREA, ia, ID_SIDE, is );
										if( UNSEL )
											UnSelect( net, &aid, SET_M );
										else
											NewSelect( net, &aid, SET_M, EN_INV );
									}
									if( UNSEL )
									{
										if( InRange( x1, r.left, r.right )
											&& InRange( y1, r.top, r.bottom ) )
										{
											id aid( ID_NET, ID_AREA, ia, ID_CORNER, ic1 );
											UnSelect( net, &aid, SET_M );
										}
										if( InRange( x2, r.left, r.right )
											&& InRange( y2, r.top, r.bottom ) )
										{
											id aid( ID_NET, ID_AREA, ia, ID_CORNER, ic2 );
											UnSelect( net, &aid, SET_M );
										}
									}
								}
							}
						}
						if (M)
							break;
					}
				}
			}
			net = m_Doc->m_nlist->GetNextNet(/*LABEL*/);
		}
	}

	if( m_sel_mask & (1<<SEL_MASK_OP ) )
	{
	// find outline sides in rect
		for( int im=0; im<m_Doc->m_outline_poly.GetSize(); im++ )
		{
			CPolyLine * poly = &m_Doc->m_outline_poly[im];
			RECT grp =poly->GetBounds();
			if (grp.left > r.right)
				continue;
			if (r.left > grp.right)
				continue;
			if (grp.bottom > r.bottom)
				continue;
			if (r.top > grp.top)
				continue;
			id smid = poly->GetId();
			int gnc = poly->GetNumContours()-1;
			int ncor = poly->GetNumCorners();
			for( int ic=0; ic<=gnc; ic++ )
			{
				int istart = poly->GetContourStart(ic);
				int iend = poly->GetContourEnd(ic);	
				for( int is=istart; is<=iend; is++ )
				{
					if( is == ncor-1 )
						if( poly->GetClosed() == 0 )
							break;
					if( poly->GetSideSel( is ) == 0 || UNSEL )
					{
						int ic1, ic2;
						ic1 = is;
						ic2 = poly->GetIndexCornerNext(ic1);
						int x1 = poly->GetX(ic1);
						int y1 = poly->GetY(ic1);
						int x2 = poly->GetX(ic2);
						int y2 = poly->GetY(ic2);
						if( InRange( x1, r.left, r.right )
							&& InRange( x2, r.left, r.right )
							&& InRange( y1, r.top, r.bottom )
							&& InRange( y2, r.top, r.bottom )
							&& m_Doc->m_vis[poly->GetLayer()] )
						{ 								
							smid.sst = ID_SIDE;
							smid.ii = is;
							if( UNSEL )
								UnSelect( NULL, &smid, SET_M );
							else
								NewSelect( NULL, &smid, SET_M, EN_INV );
						}
						if( UNSEL )
						{
							if( InRange( x1, r.left, r.right )
								&& InRange( y1, r.top, r.bottom ) )
							{
								smid.sst = ID_CORNER;
								smid.ii = ic1;
								UnSelect( NULL, &smid, SET_M );
							}
							if( InRange( x2, r.left, r.right )
								&& InRange( y2, r.top, r.bottom ) )
							{
								smid.sst = ID_CORNER;
								smid.ii = ic2;
								UnSelect( NULL, &smid, SET_M );
							}
						}
					}
				}
			}
		}
	}

	// now highlight selected items
	if( m_sel_count <= 0 )
		CancelSelection();
	else if(m_sel_count == 1 &&
			m_sel_id.type == ID_NET &&
			m_sel_id.st == ID_CONNECT &&
			m_sel_id.sst == ID_VERTEX )
	{
		if( m_sel_iv == m_sel_con.nsegs )
			SetCursorMode( CUR_END_VTX_SELECTED );
		else
			SetCursorMode( CUR_VTX_SELECTED );
		HighlightGroup();
	}
	else
	{
		m_page = 1;
		SetFKText(CUR_GROUP_SELECTED);
		m_dlist->CancelHighLight();
		HighlightGroup();
		SetCursorMode( CUR_GROUP_SELECTED );
		CString AllRef="";
		m_Doc->m_plist->GetSelParts( &AllRef );
		OnInfoBoxSendMess( "part_list " + AllRef );
	}
	gLastKeyWasArrow = FALSE;
}


//===============================================================================================
// Start dragging group being moved or added
// If group is being added (i.e. pasted):
//	bAdd = TRUE;
//	x, y = coords for cursor point for dragging
//
void CFreePcbView::StartDraggingGroup( BOOL bAdd, int x, int y )
{
	if( !bAdd )
	{
		SetCursorMode( CUR_DRAG_GROUP );
	}
	else
	{
		SetCursorMode( CUR_DRAG_GROUP_ADD );
		m_last_mouse_point.x = x;
		m_last_mouse_point.y = y;
	}
	// snap dragging point to placement grid
	SnapCursorPoint( m_last_mouse_point, -1 );
	m_from_pt = m_last_cursor_point;

	// make texts, parts and segments invisible
	//m_dlist->SetLayerVisible( LAY_HILITE, FALSE );

	int n_lines = 0, n_rat_lines = 0;
	if( getbit(m_sel_flags, FLAG_SEL_PART) )
		for(cpart* part=m_Doc->m_plist->GetFirstPart(); part; part=m_Doc->m_plist->GetNextPart(part))
			if ( part->selected && part->dl_sel )
				n_lines +=4;
	if( getbit(m_sel_flags, FLAG_SEL_TEXT) )
	{
		int it = 0;
		for(CText* text=m_Doc->m_tlist->GetFirstText(); text; text=m_Doc->m_tlist->GetNextText(&it))
			if( text->m_selected && text->dl_sel )
				n_lines +=4;
	}
	if( getbit(m_sel_flags, FLAG_SEL_NET) )
		for(cnet* n=m_Doc->m_nlist->GetFirstNet(); n; n=m_Doc->m_nlist->GetNextNet(/*LABEL*/))
			if( n->selected )
			{
				if( getbit(m_sel_flags, FLAG_SEL_CONNECT) )
					for (int i=0; i<n->nconnects; i++)
						if( n->connect[i].m_selected )
						{
							for(int ii=0; ii<n->connect[i].nsegs; ii++)
								if( n->connect[i].seg[ii].selected )
									n_lines++;
							int imax = n->connect[i].nsegs;
							if( n->connect[i].end_pin >= 0 )
								imax--;
							for(int ii=1; ii<=imax; ii++)
								if( n->connect[i].vtx[ii].selected )
								{
									n_lines++;
									n_rat_lines+=2;
								}
						}
				if( getbit(m_sel_flags, FLAG_SEL_AREA) )
					for (int i=0; i<n->nareas; i++)
						if( n->area[i].selected )
							for( int ii=n->area[i].poly->GetNumCorners()-1; ii>=0; ii-- )		
								if( n->area[i].poly->GetSideSel(ii) )
									n_lines++;
			}
	if( getbit(m_sel_flags, FLAG_SEL_OP) )
	{
		for( int item=m_Doc->m_outline_poly.GetSize()-1; item>=0; item-- )
		{
			int iMax = m_Doc->m_outline_poly[item].GetNumCorners()-1;
			if( m_Doc->m_outline_poly[item].GetClosed() == 0 )
				iMax--;
			for(int ii=iMax; ii>=0; ii--)
				if( m_Doc->m_outline_poly[item].GetSideSel( ii ) )
					n_lines++;
		}
	}
	// set up dragline array
	m_dlist->MakeDragLineArray( n_lines );
	m_dlist->MakeDragRatlineArray( n_rat_lines, 1 );
	if( getbit(m_sel_flags, FLAG_SEL_PART) )
		for(cpart* part=m_Doc->m_plist->GetFirstPart(); part; part=m_Doc->m_plist->GetNextPart(part))
			if ( part->selected && part->dl_sel )
			{
				if (!bAdd)
					m_Doc->m_plist->MakePartVisible( part, FALSE );
				CPoint P[4];
				int np = 4;
				m_Doc->m_dlist->Get_Points( part->dl_sel, P, &np ); 
				if( np == 4 )
				{		
					for( int ii=0; ii<4; ii++ )
					{
						P[ii].x -= m_from_pt.x;
						P[ii].y -= m_from_pt.y;
					}
					m_dlist->AddDragLine( P[0], P[1] );
					m_dlist->AddDragLine( P[1], P[2] );
					m_dlist->AddDragLine( P[2], P[3] );
		    		m_dlist->AddDragLine( P[3], P[0] );	
				}
				else
				{
					RECT Get;
					m_Doc->m_dlist->Get_Rect( part->dl_sel, &Get );
					CPoint pt[4];
					pt[0].x = Get.left	- m_from_pt.x;
					pt[0].y = Get.bottom- m_from_pt.y;
					pt[1].x = Get.left	- m_from_pt.x;
					pt[1].y = Get.top	- m_from_pt.y;
					pt[2].x = Get.right - m_from_pt.x;
					pt[2].y = Get.top	- m_from_pt.y;
					pt[3].x = Get.right	- m_from_pt.x;
					pt[3].y = Get.bottom- m_from_pt.y;
					m_dlist->AddDragLine( pt[0], pt[1] );
					m_dlist->AddDragLine( pt[1], pt[2] );
					m_dlist->AddDragLine( pt[2], pt[3] );
					m_dlist->AddDragLine( pt[3], pt[0] );
				}
			}
	if( getbit(m_sel_flags, FLAG_SEL_NET) )
		for(cnet* n=m_Doc->m_nlist->GetFirstNet(); n; n=m_Doc->m_nlist->GetNextNet(/*LABEL*/))
			if( n->selected )
			{
				if( getbit(m_sel_flags, FLAG_SEL_CONNECT) )
					for (int i=0; i<n->nconnects; i++)
						if( n->connect[i].m_selected )
							for(int ii=0; ii<n->connect[i].nsegs; ii++)
							{
								cseg * s = &n->connect[i].seg[ii];
								cvertex * v = &n->connect[i].vtx[ii+1];
								if( s->selected )
								{
									dl_element * dl = s->dl_el;
									if (!bAdd)
									{
										dl->visible = FALSE;
										m_Doc->m_nlist->SetViaVisible( n, i, ii, FALSE );
										m_Doc->m_nlist->SetViaVisible( n, i, ii+1, FALSE );
									}
									cvertex * v1 = &n->connect[i].vtx[ii];
									cvertex * v2 = &n->connect[i].vtx[ii+1];
									CPoint p1( v1->x - m_from_pt.x, v1->y - m_from_pt.y );
									CPoint p2( v2->x - m_from_pt.x, v2->y - m_from_pt.y );
									m_dlist->AddDragLine( p1, p2 );
								}
								else if( v->selected )
								{
									if (!bAdd)
										m_Doc->m_nlist->SetViaVisible( n, i, ii+1, FALSE );
									cvertex * v_prev = &n->connect[i].vtx[max(0,ii)];
									cvertex * v_next = &n->connect[i].vtx[min(n->connect[i].nsegs,ii+2)];
									if( v_prev->selected == 0 )
									{
										dl_element * dl = s->dl_el;
										if (!bAdd)
											dl->visible = FALSE;
										CPoint p1( v_prev->x, v_prev->y );
										CPoint p2( v->x - m_from_pt.x, v->y - m_from_pt.y );
										m_dlist->AddDragRatline( p1, p2 );
									}
									if( v_next->selected == 0 )
									{
										cseg * sn = &n->connect[i].seg[min(n->connect[i].nsegs,ii+1)];
										dl_element * dl = sn->dl_el;
										if (!bAdd)
											dl->visible = FALSE;
										CPoint p1( v_next->x, v_next->y );
										CPoint p2( v->x - m_from_pt.x, v->y - m_from_pt.y );
										m_dlist->AddDragRatline( p1, p2 );
									}
								}
							}
				if( getbit(m_sel_flags, FLAG_SEL_AREA) )
					for (int i=0; i<n->nareas; i++)
						if( n->area[i].selected )
						{
							carea * a = &n->area[i];
							CPolyLine * poly = a->poly;
							if (!bAdd)
								a->poly->MakeVisible( FALSE );
							for( int ii=poly->GetNumCorners()-1; ii>=0; ii-- )
							{		
								if( poly->GetSideSel(ii) )
								{
									int ic1 = ii;
									int ic2 = poly->GetIndexCornerNext( ic1 );
									CPoint p1( poly->GetX(ic1) - m_from_pt.x, poly->GetY(ic1) - m_from_pt.y );
									CPoint p2( poly->GetX(ic2) - m_from_pt.x, poly->GetY(ic2) - m_from_pt.y );
									m_dlist->AddDragLine( p1, p2 );
								}
							}
						}
			}
	if( getbit(m_sel_flags, FLAG_SEL_OP) )
	{
		for( int item=m_Doc->m_outline_poly.GetSize()-1; item>=0; item-- )
		{
			int n_sel = 0;
			CPolyLine * poly = &m_Doc->m_outline_poly[item];
			int iMax = m_Doc->m_outline_poly[item].GetNumCorners()-1;
			if( poly->GetClosed() == 0 )
				iMax--;
			for(int ii=iMax; ii>=0; ii--)
			{
				BOOL s = m_Doc->m_outline_poly[item].GetSideSel( ii );
				if(s)
				{
					n_sel++;
					if (!bAdd)
						poly->MakeVisible( FALSE );
					int ic1 = ii;
					int ic2 = poly->GetIndexCornerNext( ic1 );
					CPoint p1( poly->GetX(ic1) - m_from_pt.x, poly->GetY(ic1) - m_from_pt.y );
					CPoint p2( poly->GetX(ic2) - m_from_pt.x, poly->GetY(ic2) - m_from_pt.y );
					m_dlist->AddDragLine( p1, p2 );
				}
			}
			if( n_sel )
				if (!bAdd)
						poly->MakeVisible( FALSE );
		}
	}
	if( getbit(m_sel_flags, FLAG_SEL_TEXT) )
	{
		int it = 0;
		for(CText* text=m_Doc->m_tlist->GetFirstText(); text; text=m_Doc->m_tlist->GetNextText(&it))
			if( text->m_selected && text->dl_sel )
			{
				if (!bAdd) 
					text->dl_el->visible = 0;
				CPoint P[4];
				int np = 4;
				m_Doc->m_dlist->Get_Points( text->dl_sel, P, &np ); 
				if( np == 4 )
				{
					for( int ii=0; ii<4; ii++ )
					{
						P[ii].x -= m_from_pt.x;
						P[ii].y -= m_from_pt.y;
					}
					m_dlist->AddDragLine( P[0], P[1] );
					m_dlist->AddDragLine( P[1], P[2] );
					m_dlist->AddDragLine( P[2], P[3] );
					m_dlist->AddDragLine( P[3], P[0] );
				}
				else
				{
					RECT Get;
					m_Doc->m_dlist->Get_Rect( text->dl_sel, &Get );
					CPoint Pt[4];
					Pt[0].x = Get.left		- m_from_pt.x;
					Pt[0].y = Get.bottom	- m_from_pt.y;
					Pt[1].x = Get.left		- m_from_pt.x;
					Pt[1].y = Get.top		- m_from_pt.y;
					Pt[2].x = Get.right		- m_from_pt.x;
					Pt[2].y = Get.top		- m_from_pt.y;
					Pt[3].x = Get.right		- m_from_pt.x;
					Pt[3].y = Get.bottom	- m_from_pt.y;
					m_dlist->AddDragLine( P[0], P[1] );
					m_dlist->AddDragLine( P[1], P[2] );
					m_dlist->AddDragLine( P[2], P[3] );
					m_dlist->AddDragLine( P[3], P[0] );
				}						
			}
	}
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p;
	p.x  = m_from_pt.x;
	p.y  = m_from_pt.y;
	CPoint cur_p = m_dlist->PCBToScreen( p );
	SetCursorPos( cur_p.x, cur_p.y );
	m_dlist->StartDraggingArray( pDC, m_from_pt.x, m_from_pt.y, TRUE );
	ReleaseDC( pDC );
}
//===============================================================================================
void CFreePcbView::CancelDraggingGroup()
{
	m_dlist->StopDragging();
	m_draw_layer = mod_active_layer; // CancelDraggingGroup
	// make elements visible again
	if( getbit(m_sel_flags, FLAG_SEL_PART) )
		for(cpart* p=m_Doc->m_plist->GetFirstPart(); p; p=m_Doc->m_plist->GetNextPart(p))
			if( p->selected )
			{
				m_Doc->m_plist->MakePartVisible( p, TRUE );
				if( p->side+LAY_TOP_COPPER != m_draw_layer )
					m_draw_layer = DISABLE_CHANGE_DRAW_LAYER;// CancelDraggingGroup
			}
	if( getbit(m_sel_flags, FLAG_SEL_NET) )
		for(cnet* n=m_Doc->m_nlist->GetFirstNet(); n; n=m_Doc->m_nlist->GetNextNet(/*LABEL*/))
			if( n->selected )
			{
				if( getbit(m_sel_flags, FLAG_SEL_CONNECT) )
					for (int i=0; i<n->nconnects; i++)
						if( n->connect[i].m_selected )
							for(int ii=0; ii<n->connect[i].nsegs; ii++)
								if( n->connect[i].seg[ii].selected || n->connect[i].vtx[ii+1].selected )
								{
									if( m_draw_layer != n->connect[i].seg[ii].layer)
										m_draw_layer = DISABLE_CHANGE_DRAW_LAYER;// CancelDraggingGroup
									dl_element * dl = n->connect[i].seg[ii].dl_el;
									if( dl->visible == 0 )
										if (n->visible || n->connect[i].seg[ii].layer >= LAY_TOP_COPPER ) 
											dl->visible = TRUE;
									if( ii+1<n->connect[i].nsegs )
									{
										dl_element * dl = n->connect[i].seg[ii+1].dl_el;
										if( dl->visible == 0 )
											if (n->visible || n->connect[i].seg[ii].layer >= LAY_TOP_COPPER ) 
												dl->visible = TRUE;
									}
									if( ii+1 < n->connect[i].nsegs || n->connect[i].end_pin == cconnect::NO_END )
										m_Doc->m_nlist->SetViaVisible( n, i, ii+1, TRUE );
									if( n->connect[i].seg[ii].layer == LAY_RAT_LINE && n->connect[i].vtx[ii].via_w == 0 )
										m_Doc->m_nlist->SetViaVisible( n, i, ii, FALSE );
									else
										m_Doc->m_nlist->SetViaVisible( n, i, ii, TRUE );
								}
				if( getbit(m_sel_flags, FLAG_SEL_AREA) )
					for (int i=0; i<n->nareas; i++)
						if( n->area[i].selected )
						{
							n->area[i].poly->MakeVisible( TRUE );
							if( n->area[i].poly->GetLayer() != m_draw_layer )
								m_draw_layer = DISABLE_CHANGE_DRAW_LAYER;// CancelDraggingGroup
						}
			}
	if( getbit(m_sel_flags, FLAG_SEL_OP) )
	{
		for( int item=m_Doc->m_outline_poly.GetSize()-1; item>=0; item-- )
		{
			int iMax = m_Doc->m_outline_poly[item].GetNumCorners()-1;
			if( m_Doc->m_outline_poly[item].GetClosed() == 0 )
				iMax--;
			for(int ii=iMax; ii>=0; ii--)
				if( m_Doc->m_outline_poly[item].GetSideSel( ii ) )
				{
					m_Doc->m_outline_poly[item].MakeVisible( TRUE );
					if( m_Doc->m_outline_poly[item].GetLayer() != m_draw_layer )
						m_draw_layer = DISABLE_CHANGE_DRAW_LAYER;// CancelDraggingGroup
					break;
				}
		}
	}
	if( getbit(m_sel_flags, FLAG_SEL_TEXT) )
	{
		int it = 0;
		for(CText* text=m_Doc->m_tlist->GetFirstText(); text; text=m_Doc->m_tlist->GetNextText(&it))
			if( text->m_selected )
			{
				if( m_draw_layer != text->m_layer )
					m_draw_layer = DISABLE_CHANGE_DRAW_LAYER;// CancelDraggingGroup
				text->dl_el->visible = TRUE;
			}
	}
	if( m_Doc->m_vis[LAY_HILITE] )
		m_dlist->SetLayerVisible( LAY_HILITE, TRUE );
	SetCursorMode( CUR_GROUP_SELECTED );
}
//===============================================================================================
//---------------------------------------- Mirror PCB -------------------------------------------
//===============================================================================================
void CFreePcbView::TurnGroup ()
{
	if( !ThisGroupContainsGluedParts() )
		return;
	if( prev_sel_count != m_sel_count )
		SaveUndoInfoForGroup( UNDO_GROUP_MODIFY, m_Doc->m_undo_list );
	CMainFrame * pMain = (CMainFrame*) AfxGetApp()->m_pMainWnd;
	if( !pMain )
		return;
	CString str = "Mirror Parts...";
	pMain->DrawStatus( 3, &str );
	Invalidate( FALSE );
	// parts
	if( getbit( m_sel_flags, FLAG_SEL_PART ) )
		for ( cpart * p = m_Doc->m_plist->GetFirstPart(); p; p=m_Doc->m_plist->GetNextPart(p) )
			if ( p->shape )
				if( p->selected )
					m_Doc->m_plist->Move(p, -p->x, p->y, (p->angle==0?0:(360-p->angle)), p->side==0?1:0 );
		// texts
	str = "Mirror Texts...";
	pMain->DrawStatus( 3, &str );
	//UpdateWindow();
	Invalidate( FALSE );
	if( getbit( m_sel_flags, FLAG_SEL_TEXT ) )
	{
		int it = 0;
		for ( CText * t = m_Doc->m_tlist->GetFirstText(); t; t=m_Doc->m_tlist->GetNextText(&it) )				
		{
			if( t->m_selected )
			{
				int Tlay = t->m_layer;
				if ( Tlay == LAY_SILK_TOP )
					Tlay = LAY_SILK_BOTTOM;
				else if ( Tlay == LAY_SILK_BOTTOM )
					Tlay = LAY_SILK_TOP;
				else if ( Tlay == LAY_TOP_COPPER )
					Tlay = LAY_BOTTOM_COPPER;
				else if ( Tlay == LAY_BOTTOM_COPPER )
					Tlay = LAY_TOP_COPPER;
				//
				else if ( Tlay == LAY_REFINE_TOP )
					Tlay = LAY_REFINE_BOT;
				else if ( Tlay == LAY_REFINE_BOT )
					Tlay = LAY_REFINE_TOP;
				//
				else if ( Tlay > LAY_BOTTOM_COPPER )
				{
					Tlay -= LAY_BOTTOM_COPPER;
					Tlay = m_Doc->m_num_copper_layers - Tlay - 1;
					Tlay += LAY_BOTTOM_COPPER;
				}
				m_Doc->m_tlist->MoveText(t,-t->m_x,t->m_y,(t->m_angle==0?0:(360-t->m_angle)),(t->m_mirror?0:1),t->m_bNegative,Tlay);
			}
		}
	}
		// outlines
	str = "Mirror Lines...";
	pMain->DrawStatus( 3, &str );
	//UpdateWindow();
	Invalidate( FALSE );
	if( getbit(m_sel_flags, FLAG_SEL_OP) )
	{
		for( int item=m_Doc->m_outline_poly.GetSize()-1; item>=0; item--)
		{
			if ( m_Doc->m_outline_poly[item].GetNumCorners() < 2 )
			{
				m_Doc->m_outline_poly[item].Undraw();
				m_Doc->m_outline_poly.RemoveAt(item);
				m_Doc->OPRefreshID();
				continue;
			}
			if ( m_Doc->m_outline_poly[item].GetSideSel(0) || m_Doc->m_outline_poly[item].GetSel(0) ) 
			{
				int lay = m_Doc->m_outline_poly[item].GetLayer(); 
				if ( lay == LAY_SM_TOP )
					lay = LAY_SM_BOTTOM;
				else if ( lay == LAY_SM_BOTTOM )
					lay = LAY_SM_TOP;
				//
				else if ( lay == LAY_SILK_TOP )
					lay = LAY_SILK_BOTTOM;
				else if ( lay == LAY_SILK_BOTTOM )
					lay = LAY_SILK_TOP;
				//
				else if ( lay == LAY_REFINE_TOP )
					lay = LAY_REFINE_BOT;
				else if ( lay == LAY_REFINE_BOT )
					lay = LAY_REFINE_TOP;
				//
				m_Doc->m_outline_poly[item].SetLayer(lay);
				int iMax = m_Doc->m_outline_poly[item].GetNumCorners()-1;
				int cl = m_Doc->m_outline_poly[item].GetClosed();
				for(int im=iMax; im>=0; im--)
				{
					int bx = m_Doc->m_outline_poly[item].GetX(im);
					int by = m_Doc->m_outline_poly[item].GetY(im);
					m_Doc->m_outline_poly[item].MoveCorner(im,-bx,by,FALSE);
					if( cl || im < iMax )
					{
						int style = m_Doc->m_outline_poly[item].GetSideStyle(im);
						m_Doc->m_outline_poly[item].SetSideStyle(im, (style==1?2:(style==2?1:0)), FALSE);
					}
				}
				m_Doc->m_outline_poly[item].Undraw();
				m_Doc->m_outline_poly[item].Draw();
			}
		}
	}
	// 
	str = "Mirror Nets...";
	pMain->DrawStatus( 3, &str );
	//UpdateWindow();
	Invalidate( FALSE );
	for( cnet * n=m_Doc->m_nlist->GetFirstNet(); n; n=m_Doc->m_nlist->GetNextNet(/*LABEL*/) )
	{
		BOOL bDrw;
		for (int i=0; i<n->nconnects; i++)
		{
			cpart * p1 = n->pin[n->connect[i].start_pin].part;
			cpart * p2 = 0;
			bDrw = 0;
			if( n->connect[i].end_pin >= 0 )
				p2 = n->pin[n->connect[i].end_pin].part;
			if ( n->connect[i].m_selected )
			{
				bDrw = 1;
				m_Doc->m_nlist->MirrorNetConnect(n,i,m_Doc->m_num_copper_layers,FALSE);
			}
			if( (p1->selected && !n->connect[i].m_selected) ||
				(!p1->selected && n->connect[i].m_selected) )
			{
				bDrw = 1;
				n->connect[i].seg[0].width = 0;
				n->connect[i].seg[0].layer = LAY_RAT_LINE;
				n->connect[i].vtx[0].x = -n->connect[i].vtx[0].x;
			}
			if( p2 )
				if( (p2->selected && !n->connect[i].m_selected) ||
					(!p2->selected && n->connect[i].m_selected) )
				{
					bDrw = 1;
					n->connect[i].seg[n->connect[i].nsegs-1].width = 0;
					n->connect[i].seg[n->connect[i].nsegs-1].layer = LAY_RAT_LINE;
					n->connect[i].vtx[n->connect[i].nsegs].x = -n->connect[i].vtx[n->connect[i].nsegs].x;
				}
			if( bDrw )
				m_Doc->m_nlist->MergeUnroutedSegments( n, i );
		}
	}
	str = "Netlist...";
	pMain->DrawStatus( 3, &str );
	//UpdateWindow();
	Invalidate( FALSE );
	if( getbit( m_sel_flags, FLAG_SEL_PART ) )
		for ( cpart * p = m_Doc->m_plist->GetFirstPart(); p; p=m_Doc->m_plist->GetNextPart(p) )
			if ( p->shape )
				if( p->selected )
					m_Doc->m_nlist->PartMoved(p,0);
	if( getbit( m_sel_flags, FLAG_SEL_NET ) )
		for( cnet * n=m_Doc->m_nlist->GetFirstNet(); n; n=m_Doc->m_nlist->GetNextNet(/*LABEL*/) )
		{
			if( n->selected )
			{
				// areas
				if( getbit( m_sel_flags, FLAG_SEL_AREA ) )
					for (int i=0; i<n->nareas; i++)
					{
						if ( n->area[i].selected )
						{
							int lay = n->area[i].poly->GetLayer(); 
							if ( lay == LAY_TOP_COPPER )
								lay = LAY_BOTTOM_COPPER;
							else if ( lay == LAY_BOTTOM_COPPER )
								lay = LAY_TOP_COPPER;
							else
							{
								lay -= LAY_BOTTOM_COPPER;
								lay = m_Doc->m_num_copper_layers - lay - 1;
								lay += LAY_BOTTOM_COPPER;
							}
							n->area[i].poly->SetLayer(lay);
							for (int cor=0; cor<n->area[i].poly->GetNumCorners(); cor++)
							{
								n->area[i].poly->SetX(cor,-n->area[i].poly->GetX(cor));
								n->area[i].poly->SetY(cor, n->area[i].poly->GetY(cor));
								int style = n->area[i].poly->GetSideStyle(cor);
								n->area[i].poly->SetSideStyle(cor, (style==1?2:(style==2?1:0)), FALSE);
							}
							for (int cont=0; cont<n->area[i].poly->GetNumContours(); cont++)
								n->area[i].poly->RecalcRectC(cont);
							n->area[i].poly->Undraw();
							n->area[i].poly->Draw();
							m_Doc->m_nlist->SetAreaConnections(n,i);
						}
					}
			}
		}
	// repair traces
	m_Doc->m_nlist->RepairAllBranches(FALSE);

	m_dlist->CancelHighLight();
	HighlightGroup();
	m_Doc->ProjectModified( TRUE );
}

//===============================================================================================
//---------------------------------------- Move Group -------------------------------------------
//===============================================================================================
void CFreePcbView::OnGroupMove()
{
	if( ThisGroupContainsGluedParts() )
	{
		m_dlist->SetLayerVisible( LAY_RAT_LINE, FALSE );
		StartDraggingGroup(fCopyTraces,m_last_mouse_point.x,m_last_mouse_point.y);
	}
}

BOOL CFreePcbView::ThisGroupContainsGluedParts()
{
	if( GluedPartsInGroup() )
	{
		int ret = AfxMessageBox( "This group contains glued parts, so in order \
not to disturb the location this parts \
relative to other glued parts of the Pcb, \
we suggest to select all glued objects and continue?  ", MB_YESNOCANCEL );
		if( ret == IDCANCEL )
			return FALSE;
		else if( ret == IDYES )
		{
			SelectGluedParts();
			for( int item=m_Doc->m_outline_poly.GetSize()-1; item>=0; item--)
				if( m_Doc->m_outline_poly[item].GetLayer() == LAY_BOARD_OUTLINE &&
					m_Doc->m_outline_poly[item].GetNumCorners() > 2 )
				{
					id ID( ID_POLYLINE, ID_BOARD, item, ID_SIDE, 0 );
					NewSelect( NULL, &ID, 0, 0 ); 
				}
			SelectContour();
		}
		else if( ret == IDNO )
			UnselectGluedPartsInGroup();
	}
	return TRUE;
}

//===============================================================================================
//---------------------- Move group of parts and trace segments ---------------------------------
//===============================================================================================
void CFreePcbView::MoveGroup( int dx, int dy, BOOL unroute )
{
	if( !ThisGroupContainsGluedParts() )
		return;
	if( prev_sel_count != m_sel_count )
		SaveUndoInfoForGroup( UNDO_GROUP_MODIFY, m_Doc->m_undo_list );
	// Set DRAW layer
	m_draw_layer = DISABLE_CHANGE_DRAW_LAYER; // MoveGroup
	// parts
	if( getbit(m_sel_flags, FLAG_SEL_PART) )
		for(cpart* p=m_Doc->m_plist->GetFirstPart(); p; p=m_Doc->m_plist->GetNextPart(p))
			if( p->selected )
			{
				m_Doc->m_plist->Move(p,p->x+dx,p->y+dy,p->angle,p->side);
			}
	
	// nets
	for(cnet* n=m_Doc->m_nlist->GetFirstNet(); n; n=m_Doc->m_nlist->GetNextNet(/*LABEL*/))
		for (int icon=0; icon<n->nconnects; icon++)
		{
			cconnect * c = &n->connect[icon];
			BOOL RedrawFlag = FALSE;
			for(int iseg=c->nsegs; iseg>=0; iseg--)
			{
				cseg * s = &c->seg[min(c->nsegs-1,iseg)];
				cseg * prev_s = &c->seg[max(0,iseg-1)];
				cvertex * v = &c->vtx[iseg];
				if( iseg == 0 )
				{
					cpart * pp = n->pin[c->start_pin].part;
					if( pp->selected )
					{
						RedrawFlag = TRUE;
						v->x += dx;
						v->y += dy;
					}
					if( unroute )
					{
						if( (pp->selected && !(s->selected || v->selected)) ||
							(!pp->selected && (s->selected || v->selected)) )
						{
							RedrawFlag = TRUE;
							s->width = 0;
							s->layer = LAY_RAT_LINE;
						}
					}
				}
				else if( iseg == c->nsegs && c->end_pin >= 0 )
				{
					cpart * pp = n->pin[c->end_pin].part;
					if( pp->selected )
					{
						RedrawFlag = TRUE;
						v->x += dx;
						v->y += dy;
					}
					if( unroute )
					{
						if( ( pp->selected && !(s->selected || v->selected)) ||
							( !pp->selected && (s->selected || v->selected)) )
						{
							RedrawFlag = TRUE;
							s->width = 0;
							s->layer = LAY_RAT_LINE;
						}
					}
				}
				else if( s->selected || v->selected || prev_s->selected )
				{
					RedrawFlag = TRUE;
					v->x += dx;
					v->y += dy;
					if( unroute )
					{
						if( s->selected && !prev_s->selected ) 
						{
							prev_s->width = 0;
							prev_s->layer = LAY_RAT_LINE;
						}
						if( !s->selected && prev_s->selected ) 
						{
							s->width = 0;
							s->layer = LAY_RAT_LINE;
						}
					}
				}
			}
			if( RedrawFlag )
				m_Doc->m_nlist->MergeUnroutedSegments(n,icon);
		}
	if( getbit(m_sel_flags, FLAG_SEL_NET) )
	{
		for(cnet* n=m_Doc->m_nlist->GetFirstNet(); n; n=m_Doc->m_nlist->GetNextNet(/*LABEL*/))
		{
			if( n->selected )
			{
				if( getbit(m_sel_flags, FLAG_SEL_AREA) )
				{
					for (int i=0; i<n->nareas; i++)
					{
						if( n->area[i].selected )
						{
							CPolyLine * p = n->area[i].poly;
							BOOL RedrawFlag = FALSE;
							for( int ic=p->GetNumCorners()-1; ic>=0; ic-- )
							{
								int ib = p->GetIndexCornerBack(ic);
								int prev_side_sel = p->GetSideSel(ib);
								int side_sel = p->GetSideSel(ic);
								int corner_sel = p->GetSel(ic);
								if( prev_side_sel || side_sel || corner_sel )
								{
									RedrawFlag = TRUE;
									p->SetX(ic,p->GetX(ic)+dx);
									p->SetY(ic,p->GetY(ic)+dy);
								}
							}
							if( RedrawFlag )
							{
								p->Draw();
							}
						}
					}
				}
			}
		}
	}
	// texts
	if( getbit(m_sel_flags, FLAG_SEL_TEXT) )
	{
		int it = 0;
		for(CText* t=m_Doc->m_tlist->GetFirstText(); t; t=m_Doc->m_tlist->GetNextText(&it))
		{
			if ( t->m_selected )
			{
				m_Doc->m_tlist->MoveText(t,t->m_x+dx,t->m_y+dy,t->m_angle,t->m_mirror,t->m_bNegative,t->m_layer);
			}
		}
	}
	// outlines
	if( getbit(m_sel_flags, FLAG_SEL_OP) )
	{
		for( int item=m_Doc->m_outline_poly.GetSize()-1; item>=0; item-- )
		{
			BOOL RedrawFlag = FALSE;
			int gns = m_Doc->m_outline_poly[item].GetNumCorners()-1;
			int cl = m_Doc->m_outline_poly[item].GetClosed();
			for( int ic=gns; ic>=0; ic-- )
			{
				int prev = m_Doc->m_outline_poly[item].GetIndexCornerBack( ic );
				BOOL sel, prev_sel;
				if( cl == 0 && ic == gns )
					sel = 0;
				else
					sel = m_Doc->m_outline_poly[item].GetSideSel( ic );
				if( cl == 0 && prev == gns )
					prev_sel = 0;
				else
					prev_sel = m_Doc->m_outline_poly[item].GetSideSel( prev );
				BOOL cor_sel = m_Doc->m_outline_poly[item].GetSel( ic );
				if( sel || prev_sel || cor_sel )
				{
					RedrawFlag = TRUE;
					m_Doc->m_outline_poly[item].SetX(ic,m_Doc->m_outline_poly[item].GetX(ic)+dx);
					m_Doc->m_outline_poly[item].SetY(ic,m_Doc->m_outline_poly[item].GetY(ic)+dy);
				}	
			}
			if( RedrawFlag )
			{
				m_Doc->m_outline_poly[item].Draw( m_Doc->m_dlist );
			}
		}
	}
	// repair
	m_Doc->m_nlist->RepairAllBranches(FALSE);
	groupAverageX+=dx;
	groupAverageY+=dy;
}


void CFreePcbView::OnInsidePolyline()
{
	// sel parts
	id ID( ID_PART_DEF );
	int i = m_sel_id.i;
	for( cpart * p=m_Doc->m_plist->GetFirstPart(); p; p=m_Doc->m_plist->GetNextPart(p) )
		if( m_Doc->m_outline_poly[i].TestPointInsideContour( p->x, p->y ) )
			NewSelect( p, &ID, 0, 0 );
	// sel texts
	ID.Set(ID_TEXT_DEF);
	int it = 0;
	for( CText * t=m_Doc->m_tlist->GetFirstText(); t; t=m_Doc->m_tlist->GetNextText(&it) )
		if( m_Doc->m_outline_poly[i].TestPointInsideContour( t->m_x, t->m_y ) )
			NewSelect( t, &ID, 0, 0 );
	// sel nets
	for( cnet * n=m_Doc->m_nlist->GetFirstNet(); n; n=m_Doc->m_nlist->GetNextNet(/*LABEL*/) )
	{
		for( int ia=0; ia<n->nareas; ia++ )
		{
			CPolyLine * p = n->area[ia].poly;
			// test
			int max_cor = p->GetNumCorners();
			int cnt_cor = 0;
			for( int icor=max_cor-1; icor>=0; icor-- )
			{
				if( m_Doc->m_outline_poly[i].TestPointInsideContour( p->GetX(icor), p->GetY(icor) ) )
				{	
					cnt_cor++;
					if( cnt_cor > max_cor/2 )
						break;
				}
			}
			if( cnt_cor )
			{
				ID.Set( ID_NET, ID_AREA, ia, ID_CORNER, 0 );
				for( int icor=max_cor-1; icor>=0; icor-- )
				{
					ID.ii = icor;
					NewSelect( n, &ID, 0, 0 );
				}
			}
			else
			{
				ID.Set( ID_NET, ID_AREA, ia, ID_CORNER, 0 );
				for( int icor=max_cor-1; icor>=0; icor-- )
				{
					if( m_Doc->m_outline_poly[i].TestPointInsideContour( p->GetX(icor), p->GetY(icor) ) )
					{
						ID.ii = icor;
						NewSelect( n, &ID, 0, 0 );
					}
				}
			}
		}
		for( int ic=0; ic<n->nconnects; ic++ )
		{
			cconnect * c = &n->connect[ic];
			ID.Set( ID_NET, ID_CONNECT, ic, ID_VERTEX, 0 );
			for( int icor=c->nsegs; icor>=0; icor-- )
			{
				cvertex * v = &c->vtx[icor];
				if( m_Doc->m_outline_poly[i].TestPointInsideContour( v->x, v->y ) )
				{
					ID.ii = icor;
					NewSelect( n, &ID, 0, 0 );
				}
			}
		}
	}
	// sel lines
	for( int ip=m_Doc->m_outline_poly.GetSize()-1; ip>=0; ip-- )
	{
		CPolyLine * p = &m_Doc->m_outline_poly[ip];
		// test all corners
		int ok = p->GetNumCorners();
		int nc = ok/2;
		for( int icor=p->GetNumCorners()-1; icor>=0; icor-- )
		{
			if( m_Doc->m_outline_poly[i].TestPointInsideContour( p->GetX(icor), p->GetY(icor) ) == 0 )
				ok--;
		}
		if( ok > nc )
		{
			ID = p->GetId();		
			ID.i = ip;
			ID.sst = ID_CORNER;
			for( int icor=p->GetNumCorners()-1; icor>=0; icor-- )
			{
				ID.ii = icor;
				NewSelect( NULL, &ID, 0, 0 );
			}
		}
	}
	if( m_sel_count )
	{
		SetCursorMode( CUR_GROUP_SELECTED );
		m_Doc->m_dlist->CancelHighLight();
		HighlightGroup();
	}
}


//===============================================================================================
// Highlight group selection
// the only legal members are parts, texts, trace segments and
// copper area, solder mask cutout and board outline sides
//
void CFreePcbView::HighlightGroup(BOOL bPins)
{
	if( m_sel_count )
	{
		// parts
		if( getbit(m_sel_flags, FLAG_SEL_PART) )
			for(cpart* p=m_Doc->m_plist->GetFirstPart(); p; p=m_Doc->m_plist->GetNextPart(p))
				if( p->selected )
				{
					SelectPart(p,bPins);
				}
		
		// nets
		if( getbit(m_sel_flags, FLAG_SEL_NET) )
			for(cnet* n=m_Doc->m_nlist->GetFirstNet(); n; n=m_Doc->m_nlist->GetNextNet(/*LABEL*/))
			{
				if( n->selected )
				{
					if( getbit(m_sel_flags, FLAG_SEL_CONNECT) )
						for (int icon=0; icon<n->nconnects; icon++)
							if( n->connect[icon].m_selected )
								for(int iseg=n->connect[icon].nsegs; iseg>=0; iseg--)
								{
									cseg * s = &n->connect[icon].seg[min(n->connect[icon].nsegs-1,iseg)];
									cvertex * v = &n->connect[icon].vtx[iseg];
									if( s->selected )
										m_Doc->m_nlist->HighlightSegment(n,icon,min(n->connect[icon].nsegs-1,iseg));
									if( v->selected )
										m_Doc->m_nlist->HighlightVertex(n,icon,iseg);
								}
					if( getbit(m_sel_flags, FLAG_SEL_AREA) )
						for (int ia=0; ia<n->nareas; ia++)
							if( n->area[ia].selected )
							{
								int hw = n->area[ia].poly->GetW();
								for ( int cor=n->area[ia].poly->GetNumCorners()-1; cor>=0; cor-- )
								{
									if( n->area[ia].poly->GetSel(cor) )
										m_Doc->m_nlist->HighlightAreaCorner(n,ia,cor);
									if( n->area[ia].poly->GetSideSel(cor) )
										n->area[ia].poly->HighlightSide(cor,abs(hw));
								}
							}
				}
			}
		// texts
		if( getbit(m_sel_flags, FLAG_SEL_TEXT) )
		{
			int it = 0;
			for(CText* t=m_Doc->m_tlist->GetFirstText(); t; t=m_Doc->m_tlist->GetNextText(&it))
				if ( t->m_selected )
					m_Doc->m_tlist->HighlightText(t);
		}
		// outlines
		if( getbit(m_sel_flags, FLAG_SEL_OP) )
		{
			for( int item=m_Doc->m_outline_poly.GetSize()-1; item>=0; item-- )
			{
				int gns = m_Doc->m_outline_poly[item].GetNumCorners()-1;
				int cl = m_Doc->m_outline_poly[item].GetClosed();
				for( int ic=gns; ic>=0; ic-- )
				{
					if( m_Doc->m_outline_poly[item].GetSel(ic) )
						m_Doc->m_outline_poly[item].HighlightCorner(ic);
					if( cl == 0 && ic == gns )
						continue;
					if( m_Doc->m_outline_poly[item].GetSideSel(ic) )
						m_Doc->m_outline_poly[item].HighlightSide(ic,m_Doc->m_outline_poly[item].GetW()+NM_PER_MIL);
				}
			}
		}
	}
	if( m_sel_count > 1 )
	{
		int map_orig_layer = 0;
		RECT rct = m_Doc->m_dlist->GetHighlightedBounds( &map_orig_layer );
		if( rct.right > rct.left )
		{
			id ID(0,0,0,0,0);
			dl_element * el_bounds = m_Doc->m_dlist->Add( ID, NULL, 0, DL_HOLLOW_RECT, TRUE, &rct, 0, NULL, 0 );
			m_Doc->m_dlist->HighLight( el_bounds );
			el_bounds->map_orig_layer = map_orig_layer;
		}
	}
}

//===============================================================================================
// Test for glued parts in group
//
BOOL CFreePcbView::GluedPartsInGroup()
{
	BOOL b1=0, b2=0;
	if( m_Doc->m_plist->GetSelCount() )
		for(cpart* p=m_Doc->m_plist->GetFirstPart(); p; p=m_Doc->m_plist->GetNextPart(p))
			if( p->glued )
				if( p->selected )
					b1 = TRUE;
				else 
					b2 = TRUE;
	if( b1 && b2 )
		return TRUE;
	else
		return FALSE;
}
//===============================================================================================
// Unglue parts in group
// returns index of item if found, otherwise -1
//
void CFreePcbView::UnselectGluedPartsInGroup()
{
	id iD( ID_PART_DEF );
	if( getbit(m_sel_flags, FLAG_SEL_PART) )
		for(cpart* p=m_Doc->m_plist->GetFirstPart(); p; p=m_Doc->m_plist->GetNextPart(p))
			if( p->glued && p->selected )
				UnSelect( p, &iD, 0 );
}
//===============================================================================================
void CFreePcbView::SelectGluedParts()
{
	id iD( ID_PART_DEF );
	for(cpart* p=m_Doc->m_plist->GetFirstPart(); p; p=m_Doc->m_plist->GetNextPart(p))
		if( p->glued )
			NewSelect( p, &iD, 0, 0 );
}
//===============================================================================================
// Set array of selection mask ids
//
void CFreePcbView::SetSelMaskArray( int mask )
{
	for( int i=0; i<NUM_SEL_MASKS; i++ )
	{
		if( mask & (1<<i) )
			m_mask_id[i].ii = 0;
		else
			m_mask_id[i].ii = 0xfffe;	// guaranteed not to exist
	}
}
//===============================================================================================
void CFreePcbView::OnAddSimilarArea()
{
	AddSimilarArea();
	Invalidate( FALSE );
}
void CFreePcbView::AddSimilarArea()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	m_dlist->CancelHighLight();
	SetCursorMode( CUR_ADD_AREA );
	m_active_layer = m_sel_net->area[m_sel_ia].poly->GetLayer();
	m_Doc->m_vis[m_active_layer] = TRUE;
	m_dlist->SetLayerVisible( m_active_layer, TRUE );
	ShowActiveLayer(m_Doc->m_num_copper_layers);
	for(int ii=0; ii<m_sel_net->nareas; ii++ )
		m_Doc->m_nlist->HighlightAreaSides( m_sel_net, ii, 3*m_pcbu_per_wu );
	m_Doc->m_plist->HighlightAllPadsOnNet( m_sel_net, 2, 0 );
	m_dlist->StartDraggingArray( pDC, m_last_cursor_point.x,
		m_last_cursor_point.y, 2 );
	m_polyline_style = CPolyLine::STRAIGHT;
	m_polyline_hatch = m_sel_net->area[m_sel_ia].poly->GetHatch();
	m_polyline_width = m_sel_net->area[m_sel_ia].poly->GetW();
	ReleaseDC( pDC );
}
//===============================================================================================
void CFreePcbView::OnAreaEdit()
{
	CDlgAddArea dlg;
	int layer = m_sel_net->area[m_sel_id.i].poly->GetLayer();
	int hatch = m_sel_net->area[m_sel_id.i].poly->GetHatch();
	int w = m_sel_net->area[m_sel_id.i].poly->GetW();
	while (1)
	{
		dlg.Initialize( m_Doc->m_nlist, 
					m_Doc->m_num_layers, 
					m_sel_net, 
					layer, 
					hatch, 
					w, 
					m_Doc->m_units, TRUE );
		int ret = dlg.DoModal();
		if( ret == IDOK )
		{
			cnet * net = dlg.m_net;
			if( m_sel_net == net )
			{
				SaveUndoInfoForAllAreasInNet( m_sel_net, TRUE, m_Doc->m_undo_list );
			}
			else
			{
				// move area to new net
				SaveUndoInfoForAllAreasIn2Nets( m_sel_net, net, TRUE, m_Doc->m_undo_list );
				int ia = m_Doc->m_nlist->AddArea( net, dlg.m_layer, 0, 0, 0 );
				net->area[ia].poly->Copy( m_sel_net->area[m_sel_ia].poly );
				net->area[ia].poly->SetPtr( net );
				id new_id = net->area[ia].poly->GetId();
				new_id.i = ia;
				net->area[ia].poly->SetId( &new_id );
				m_Doc->m_nlist->RemoveArea( m_sel_net, m_sel_ia ); 
				m_Doc->m_nlist->SetAreaConnections( net, ia );
				m_Doc->m_nlist->OptimizeConnections(  net, -1, m_Doc->m_auto_ratline_disable,
															m_Doc->m_auto_ratline_min_pins, TRUE  );
				CancelSelection();
				m_sel_net = net;
				m_sel_ia = ia;
			}
			m_sel_net->area[m_sel_ia].poly->Undraw();
			m_sel_net->area[m_sel_ia].poly->SetLayer( dlg.m_layer );
			m_sel_net->area[m_sel_ia].poly->SetHatch( dlg.m_hatch );
			m_sel_net->area[m_sel_ia].poly->SetW( dlg.m_width );
			m_sel_net->area[m_sel_ia].poly->Draw();
			int ret = m_Doc->m_nlist->AreaPolygonModified( m_sel_net, m_sel_ia, FALSE, TRUE );
			m_Doc->m_nlist->OptimizeConnections(  m_sel_net, -1, m_Doc->m_auto_ratline_disable,
															m_Doc->m_auto_ratline_min_pins, TRUE  );
			CancelSelection();
			if( ret == -1 )
			{
				// error
				AfxMessageBox( "Error: Unable to clip polygon due to intersecting arc" );			
				m_Doc->OnEditUndo();
			}
			m_Doc->ProjectModified( TRUE );
		}
		break;
	}
}
//===============================================================================================
void CFreePcbView::OnAreaEdgeApplyClearances()
{
	//** this is for testing only
}
//===============================================================================================
void CFreePcbView::ReselectNetItemIfConnectionsChanged( int new_ic )
{
	if( m_sel_net )
	if( new_ic >= 0 && new_ic < m_sel_net->nconnects
		&& (m_cursor_mode == CUR_SEG_SELECTED
		|| m_cursor_mode == CUR_RAT_SELECTED
		|| m_cursor_mode == CUR_VTX_SELECTED
		|| m_cursor_mode == CUR_END_VTX_SELECTED
		|| m_cursor_mode == CUR_CONNECT_SELECTED
		|| m_cursor_mode == CUR_NET_SELECTED ) )
	{
		m_Doc->m_dlist->CancelHighLight();
		m_sel_ic = new_ic;
		if( m_cursor_mode == CUR_SEG_SELECTED )
			m_Doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, m_sel_is );
		else if( m_cursor_mode == CUR_RAT_SELECTED )
			m_Doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, m_sel_is );
		else if( m_cursor_mode == CUR_VTX_SELECTED )
			m_Doc->m_nlist->HighlightVertex( m_sel_net, m_sel_ic, m_sel_iv );
		else if( m_cursor_mode == CUR_END_VTX_SELECTED )
			m_Doc->m_nlist->HighlightVertex( m_sel_net, m_sel_ic, m_sel_iv );
		else if( m_cursor_mode == CUR_CONNECT_SELECTED )
			m_Doc->m_nlist->HighlightConnection( m_sel_net, m_sel_ic );
		else if( m_cursor_mode == CUR_NET_SELECTED )
			m_Doc->m_nlist->HighlightNetConnections( m_sel_net );
	}
}
//===============================================================================================
void CFreePcbView::OnGroupCopy()
{
	// clear clipboard
	m_Doc->clip_outline_poly.SetSize(0);
	m_Doc->clip_tlist->RemoveAllTexts();
	m_Doc->clip_nlist->RemoveAllNets();
	m_Doc->clip_plist->RemoveAllParts();
	m_Doc->clip_mlist->CopyFrom( m_Doc->m_mlist );

	// set pointers
	CArray<CPolyLine> * g_op = &m_Doc->clip_outline_poly;
	CPartList * g_pl = m_Doc->clip_plist;
	CNetList * g_nl = m_Doc->clip_nlist;
	CTextList * g_tl = m_Doc->clip_tlist;

	// add all parts and text from group
	if( getbit(m_sel_flags, FLAG_SEL_PART) )
		for(cpart* part=m_Doc->m_plist->GetFirstPart(); part; part=m_Doc->m_plist->GetNextPart(part))
			if ( part->selected )
			{
				// add part to group partlist
				CShape * shape = part->shape;
				cpart * g_part = g_pl->Add( part->shape, &part->ref_des, part->x, part->y,
					part->side, part->angle, 1, 0 );
				// set ref text parameters
				g_part->m_ref_angle = part->m_ref_angle;
				g_part->m_ref_size = part->m_ref_size;
				g_part->m_ref_w = part->m_ref_w;
				g_part->m_ref_xi = part->m_ref_xi;
				g_part->m_ref_yi = part->m_ref_yi;
				g_part->m_value_angle = part->m_value_angle;
				g_part->m_value_size = part->m_value_size;
				g_part->m_value_w = part->m_value_w;
				g_part->m_value_xi = part->m_value_xi;
				g_part->m_value_yi = part->m_value_yi;
				g_part->value = part->value;
				g_part->m_merge = part->m_merge;

				// vis/invis
				g_part->m_ref_vis = part->m_ref_vis;
				g_part->m_value_vis = part->m_value_vis;

				// add pin nets to group netlist
				for( int ip=0; ip<part->shape->GetNumPins(); ip++ )
				{
					part_pin * pin = &part->pin[ip];
					CShape * shape = part->shape;
					cnet * net = pin->net;
					if( net )
					{
						// add net to group netlist if not already added
						cnet * g_net = g_nl->GetNetPtrByName( &net->name );
						if( g_net == NULL )
						{
							g_net = g_nl->AddNet( net->name, net->def_w, net->def_via_w, net->def_via_hole_w );
						}
						// add pin to net
						CString pin_name = shape->GetPinNameByIndex( ip );
						if( g_nl->FindPin( &part->ref_des, &pin_name ) == NULL )
							g_nl->AddNetPin( g_net, &part->ref_des, &pin_name, FALSE );
					}
				}
			}
	if( getbit(m_sel_flags, FLAG_SEL_TEXT) )
	{
		int it = 0;
		for(CText* t=m_Doc->m_tlist->GetFirstText(); t; t=m_Doc->m_tlist->GetNextText(&it))
			if( t->m_selected )
			{
				CText* nt = g_tl->AddText(	t->m_x, t->m_y, t->m_angle, t->m_mirror,  t->m_bNegative,
											t->m_layer, t->m_font_size, t->m_stroke_width, &t->m_str, FALSE );
				nt->m_merge = t->m_merge;
			}
	}

	// check all selected areas and connections
	g_nl->ClearTeeIDs();
	if( getbit(m_sel_flags, FLAG_SEL_NET) )
		for(cnet* net=m_Doc->m_nlist->GetFirstNet(); net; net=m_Doc->m_nlist->GetNextNet(/*LABEL*/))
			if( net->selected )
			{
				if( getbit(m_sel_flags, FLAG_SEL_CONNECT) )
					for (int i=0; i<net->nconnects; i++)
						if( net->connect[i].m_selected )
						{
							// connection, only add if between parts in group
							cconnect * c = &net->connect[i];
							cnet * g_net = g_nl->GetNetPtrByName( &net->name );
							if( g_net == NULL )
							{
								g_net = g_nl->AddNet( net->name, net->def_w, net->def_via_w, net->def_via_hole_w );
							}
							// test start and end pins
							BOOL bStartPinInGroup = FALSE;
							BOOL bEndPinInGroup = FALSE;
							BOOL bStubTrace = FALSE;
							if( c->end_pin == cconnect::NO_END )
								bStubTrace = TRUE;
							cpin * pin1 = &net->pin[c->start_pin];
							cpart * part1 = pin1->part;
							cpin * pin2 = NULL;
							cpart * part2 = NULL;
							if( !bStubTrace )
							{
								pin2 = &net->pin[c->end_pin];
								part2 = pin2->part;
							}
							//
							if( part1->selected )
							{
								if( part2 )
									if( !part2->selected )
										bStubTrace = TRUE;
								// add connection to group net, and copy all segments and vertices
								int p1 = g_nl->GetNetPinIndex( g_net, &pin1->ref_des, &pin1->pin_name );
								int g_ic;
								int ic;
								if( !bStubTrace )
								{
									int p2 = g_nl->GetNetPinIndex( g_net, &pin2->ref_des, &pin2->pin_name );
									g_ic = g_nl->AddNetConnect( g_net, p1, p2 );
								}
								else
								{
									g_ic = g_nl->AddNetStub( g_net, p1 );
								}
								cconnect * g_c = &g_net->connect[g_ic];
								g_c->m_merge = c->m_merge;
								g_c->nsegs = c->nsegs;
								g_c->seg.SetSize( c->nsegs );
								g_c->vtx.SetSize( c->nsegs + 1 );
								for( int is=0; is<c->nsegs; is++ )
								{
									g_c->seg[is] = c->seg[is];
									g_c->seg[is].m_dlist = NULL;
									g_c->seg[is].dl_el = NULL;
									g_c->vtx[is] = c->vtx[is];	// this zeros graphics elements
									c->vtx[is] = g_c->vtx[is];	// this restores them
									g_c->vtx[is].m_dlist = NULL;
									g_nl->AddTeeID( g_c->vtx[is].tee_ID );
								}
								g_c->vtx[c->nsegs] = c->vtx[c->nsegs];
								c->vtx[c->nsegs] = g_c->vtx[c->nsegs];
								g_c->vtx[c->nsegs].m_dlist = NULL;
								g_nl->AddTeeID( g_c->vtx[c->nsegs].tee_ID );
								// remove any routed segments that are not in group
								for( int is=0; is<c->nsegs; is++ )
								{
									if( c->seg[is].layer != LAY_RAT_LINE )
									{
										// routed segment, is this in group ?
										if( !c->seg[is].selected && !c->vtx[is].selected && !c->vtx[is+1].selected )
										{
											// not in group, unroute it
											g_net->connect[g_ic].seg[is].width = 0;
											g_net->connect[g_ic].seg[is].layer = LAY_RAT_LINE;
										}
									}
								}
								// merge unrouted segments
								g_nl->MergeUnroutedSegments( g_net, g_ic );
							}
						}
				if( getbit(m_sel_flags, FLAG_SEL_AREA) )
					for (int i=0; i<net->nareas; i++)
						if( net->area[i].selected )
						{
							BOOL bAllSides = TRUE;
							CPolyLine * p = net->area[i].poly;
							for( int is=0; is<p->GetNumSides(); is++ )
							{
								if( p->GetSideSel( is ) == 0 && p->GetSel( is ) == 0 )
								{
									bAllSides = FALSE;
									break;
								}
							}
							if( bAllSides )
							{
								// add area to group
								cnet * g_net = g_nl->GetNetPtrByName( &net->name );
								if( g_net == NULL )
								{
									g_net = g_nl->AddNet( net->name, net->def_w, net->def_via_w, net->def_via_hole_w );
								}
								int g_ia = g_nl->AddArea( g_net, p->GetLayer(), p->GetX(0), p->GetY(0),
									p->GetHatch() );
								CPolyLine * g_p = g_net->area[g_ia].poly;
								g_p->Copy( p );
								id g_id;
								g_id = g_p->GetId();
								g_id.i = g_ia;
								g_p->SetId( &g_id );
							}
						}
			}

	g_nl->CleanUpAllConnections();

	// now remove any nets with zero pins, connections and areas
	cnet * net = g_nl->GetFirstNet();
	while( net )
	{
		cnet * next_net = g_nl->GetNextNet(/*LABEL*/);
		if( net->npins == 0 && net->nconnects == 0 && net->nareas == 0 )
			g_nl->RemoveNet( net );
		net = next_net;
	}

	// copy to group
	// outlines
	if( getbit(m_sel_flags, FLAG_SEL_OP) )
	{
		for( int item=m_Doc->m_outline_poly.GetSize()-1; item>=0; item-- )
		{
			int num_sel = 0;
			int NumC = m_Doc->m_outline_poly[item].GetNumCorners();
			for(int ii=NumC-1; ii>=0; ii--)
				if( m_Doc->m_outline_poly[item].GetSel( ii ) )
					num_sel++;
			if( m_Doc->m_outline_poly[item].GetClosed() == 0 )
				NumC--;
			for(int ii=NumC-1; ii>=0; ii--)
				if( m_Doc->m_outline_poly[item].GetSideSel( ii ) )
					num_sel++;
			if( num_sel )
			{
				// add to group
				CPolyLine * p = &m_Doc->m_outline_poly[item];
				int g_ism = g_op->GetSize();
				g_op->SetSize(g_ism+1);
				CPolyLine * g_p = &g_op->GetAt(g_ism);
				g_p->Copy( p );
				id sid = p->GetId();
				sid.i = g_ism;
				g_p->SetId( &sid );
			}	
		}
	}
	// see if anything copied
	cnet * fn = g_nl->GetFirstNet();
	if( !fn && !g_pl->GetFirstPart() 
		&& !g_op->GetSize() && !g_tl->GetNumTexts() )
	{
		AfxMessageBox( "Nothing copied !\nRemember that traces must be connected\nto a part in the group to be copied" );
		CWnd* pMain = AfxGetMainWnd();
		if (pMain != NULL)
		{
			CMenu* pMenu = pMain->GetMenu();
			CMenu* submenu = pMenu->GetSubMenu(1);	// "Edit" submenu
			submenu->EnableMenuItem( ID_EDIT_PASTE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );
			pMain->DrawMenuBar();
		}
	}
	else
	{
		if( fn )
			g_nl->CancelNextNet();
		CWnd* pMain = AfxGetMainWnd();
		if (pMain != NULL)
		{
			CMenu* pMenu = pMain->GetMenu();
			CMenu* submenu = pMenu->GetSubMenu(1);	// "Edit" submenu
			submenu->EnableMenuItem( ID_EDIT_PASTE, MF_BYCOMMAND | MF_ENABLED );
			pMain->DrawMenuBar();
		}
	}
}
//===============================================================================================

//===============================================================================================
void CFreePcbView::OnGroupCut()
{
	OnGroupCopy();
	OnGroupDelete();
}
//===============================================================================================
// Remove all elements in group from project
//
void CFreePcbView::OnGroupDelete()
{
	DeleteGroup(0);
	Invalidate( FALSE );//OnGroupDelete
}
//===============================================================================================
void CFreePcbView::DeleteGroup( BOOL wMerge )
{
	if( SaveUndoInfoForGroup( UNDO_GROUP_DELETE, m_Doc->m_undo_list, wMerge ) == IDCANCEL )
		return;
	if( m_sel_count <= 0 )
		CancelSelection();
	else
	{
		m_Doc->m_dlist->CancelHighLight();
		HighlightGroup();
		if( m_sel_count == 1 && m_sel_id.type == ID_NET )
			NewSelect( m_sel_net, &m_sel_id, 1, 0 );
		else
		{		
			SetFKText( m_cursor_mode );		
		}
	}
	m_Doc->ProjectModified( TRUE );
}
//===============================================================================================
void CFreePcbView::OnGroupPaste( BOOL bwDialog, BOOL bSaveMerges )
{
#define ELEMENT_SELECTED 1
	void * vp;
	// pointers to group lists
	CPartList * g_pl = m_Doc->clip_plist;
	CTextList * g_tl = m_Doc->clip_tlist;
	CNetList * g_nl = new CNetList( NULL, g_pl );	// make copy to modify
	g_nl->Copy( m_Doc->clip_nlist );
	g_nl->MarkAllNets( 0 );
	CArray<CPolyLine> * g_op = &m_Doc->clip_outline_poly;
	// pointers to project lists
	CPartList * pl = m_Doc->m_plist;
	CNetList * nl = m_Doc->m_nlist;
	CTextList * tl = m_Doc->m_tlist;
	CArray<CPolyLine> * op = &m_Doc->m_outline_poly;

	// get paste options
	int check_one_pin = 1;
	if( g_pl )
		for( cpart * p=g_pl->GetFirstPart(); p; p=g_pl->GetNextPart(p) )
		{
			if( p->shape )
				if( p->shape->GetNumPins() > 1 )
				{
					check_one_pin = 0;
					break;
				}
		}
	CDlgGroupPaste dlg;	
	if( !check_one_pin && m_Doc->m_netlist_completed ) // PROTECTED
	{
		dlg.Initialize( NULL );
		static int wwm = 0;
		if( !wwm )
		{
			AfxMessageBox("When a netlist is protected (Project->Nets->Netlist Protected), only parts, texts and polylines can be inserted into the project.");
			wwm = 1;
		}
	}
	else
		dlg.Initialize( g_nl );
	int ret;
	if (!bwDialog)
		ret = dlg.DoModal();
	else
	{
		dlg.m_net_name_option = 0;
		dlg.m_dx = 0;
		dlg.m_dy = 0;
		ret = IDOK;
	}
	if( ret == IDOK )
	{
		if( !bSaveMerges )
			m_Doc->clip_mlist->Clear();
		// start ( m_sel_count = 0 )
		CancelSelection();
		// if protected
		if( !check_one_pin && m_Doc->m_netlist_completed ) // PROTECTED
			dlg.m_net_name_option = 2;
		// Clear netlist
		if( dlg.m_net_name_option == 2 )
			g_nl->RemoveAllNets();
		BOOL ref_as_text = dlg.m_ref_as_text;
		BOOL value_as_text = dlg.m_value_as_text;
		m_sel_id.type = ID_MULTI;
		m_Doc->m_undo_list->NewEvent();
		BOOL bDragGroup = !dlg.m_position_option;
		double min_d = (double)INT_MAX*(double)INT_MAX;
		int min_x = INT_MAX;	// lowest-left point for dragging group
		int min_y = INT_MAX;
		// make a map of all reference designators in project, including
		// refs in the netlist that don't exist in the partlist
		CMapStringToPtr ref_des_map;
		cpart * part = pl->GetFirstPart();
		while( part )
		{
			// ref_des_map
			ref_des_map.SetAt( part->ref_des, NULL );
			part = pl->GetNextPart( part );
		}
		cpart * gpart = g_pl->GetFirstPart();
		while( gpart )
		{
			// Merges renumber
			CString pM = m_Doc->clip_mlist->GetMerge( gpart->m_merge );
			CString mstr = pM;
			if( m_Doc->m_mlist->GetIndex( mstr ) >= 0 )
			{
				ParseRef( &pM, &mstr );
				pM = mstr;
				int im = 0; 
				do{ im++;
					mstr.Format( "%s%d", pM, im );
					}while( m_Doc->m_mlist->GetIndex( mstr ) >= 0 );
			}
			m_Doc->clip_mlist->SetMerge( gpart->m_merge, mstr );
			gpart = g_pl->GetNextPart( gpart );
		}
		cnet * net = nl->GetFirstNet();
		while( net )
		{
			for( int ip=0; ip<net->npins; ip++ )
			{
				cpin * p = &net->pin[ip];
				if( !ref_des_map.Lookup( p->ref_des, vp ) )
				{
					ref_des_map.SetAt( p->ref_des, NULL );
				}
			}
			net = nl->GetNextNet(/*LABEL*/);
		}

		// add parts from group, renaming if necessary
		cpart * g_part = g_pl->GetFirstPart();
		while( g_part )
		{
			if( ref_as_text && g_part->m_ref_vis && g_part->m_ref_size )
			{
				BOOL mirror = FALSE;
				int angle = g_part->angle + g_part->m_ref_angle;
				if( g_part->side )
				{
					angle = g_part->angle - g_part->m_ref_angle;
					mirror = TRUE;
				}
				CPoint rp = g_pl->GetRefPoint(g_part);
				CText *nt = g_tl->AddText( rp.x, rp.y, angle,
					mirror, FALSE, g_part->side+LAY_SILK_TOP, g_part->m_ref_size, g_part->m_ref_w,
					&g_part->ref_des, FALSE );
				nt->m_merge = g_part->m_merge;
			}
			if( value_as_text && g_part->m_value_vis && g_part->m_value_size )
			{
				BOOL mirror = FALSE;
				int angle = g_part->angle + g_part->m_value_angle;
				if( g_part->side )
				{
					angle = g_part->angle - g_part->m_value_angle;
					mirror = TRUE;
				}
				CPoint vp = g_pl->GetValuePoint(g_part);
				CText *nt = g_tl->AddText( vp.x+dlg.m_dx, vp.y+dlg.m_dy, angle,
					mirror, FALSE, g_part->side+LAY_SILK_TOP, g_part->m_value_size, g_part->m_value_w,
					&g_part->value, FALSE );
				nt->m_merge = g_part->m_merge;
			}
			CString conflicted_ref;
			CString g_prefix;
			int g_num = ParseRef( &g_part->ref_des, &g_prefix );
			BOOL bConflict = FALSE;
			// make new ref
			CString new_ref = g_part->ref_des;
			if( dlg.m_ref_option == 2 )
			{
				// add offset to ref
				new_ref.Format( "%s%d", g_prefix, g_num + dlg.m_ref_offset );
			}
			if( dlg.m_ref_option != 1  )
			{		
				if( ref_des_map.Lookup( new_ref, vp ) )
				{
					CString nr = new_ref;
					for(int num_r=0; num_r<20; num_r++)
					{
						nr.Format("%s|%d",new_ref, num_r);
						if( ref_des_map.Lookup( nr, vp ) == 0 )
							break;
					}
					new_ref = nr;
				}
				//
				// new ref conflicts with existing ref in project
				if( ref_des_map.Lookup( new_ref, vp ) )
				{
					conflicted_ref = new_ref;
					bConflict = TRUE;
				}
			}
			if( dlg.m_ref_option == 1 || bConflict )
			{
				// use next available ref
				int x_num = 0;
				do
				{
					x_num++;
					new_ref.Format( "%s%d", g_prefix, x_num );
				}while( m_Doc->m_plist->GetPart(new_ref) );
			}
			if( bConflict )
			{
				// ref in group conflicts with ref in project
				CString mess = "Part \"";
				mess += conflicted_ref;
				mess += "\" already exists in project.\nIt will be renamed \" ";
				mess += new_ref;
				mess += " \"";
				AfxMessageBox( mess );
				bConflict = TRUE;
			}
			// now change part refs in group netlist
			net = g_nl->GetFirstNet();
			while( net )
			{
				for( int ip=0; ip<net->npins; ip++ )
				{
					cpin * pin = &net->pin[ip];
					if( pin->utility == 0 && pin->ref_des == g_part->ref_des )
					{
						pin->ref_des = new_ref;
						pin->part = NULL;
						pin->utility = 1;	// only change it once
					}
				}
				net = g_nl->GetNextNet(/*LABEL*/);
			}
			// add new part
			m_sel_part = pl->Add( g_part->shape, &new_ref,
				g_part->x + dlg.m_dx, g_part->y + dlg.m_dy,
				g_part->side, g_part->angle, 1, 0 );

			// set flags
			setbit( m_sel_flags, FLAG_SEL_PART );

			ref_des_map.SetAt( new_ref, NULL );
			// set ref text parameters
			pl->UndrawPart( m_sel_part );
			m_sel_part->m_ref_angle = g_part->m_ref_angle;
			m_sel_part->m_ref_size = g_part->m_ref_size;
			m_sel_part->m_ref_w = g_part->m_ref_w;
			m_sel_part->m_ref_xi = g_part->m_ref_xi;
			m_sel_part->m_ref_yi = g_part->m_ref_yi;
			m_sel_part->m_ref_vis = g_part->m_ref_vis;
			if( ref_as_text )
				m_sel_part->m_ref_vis = 0;
			m_sel_part->m_value_angle = g_part->m_value_angle;
			m_sel_part->m_value_size = g_part->m_value_size;
			m_sel_part->m_value_w = g_part->m_value_w;
			m_sel_part->m_value_xi = g_part->m_value_xi;
			m_sel_part->m_value_yi = g_part->m_value_yi;
			m_sel_part->m_value_vis = g_part->m_value_vis;
			if( value_as_text )
				m_sel_part->m_value_vis = 0;
			m_sel_part->value = g_part->value;
			m_sel_part->m_id = g_part->m_id;
			m_sel_id = m_sel_part->m_id;
			m_sel_id.st = ID_SEL_RECT;

			if( g_part->m_merge >= 0 )
			{
				CString pM = m_Doc->clip_mlist->GetMerge( g_part->m_merge );
				int clrn = m_Doc->clip_mlist->GetClearance( g_part->m_merge );
				m_sel_part->m_merge = m_Doc->m_mlist->AddNew( pM, clrn );
			}
			
			pl->DrawPart( m_sel_part );
			// find closest part to lower left corner
			double d = m_sel_part->x + m_sel_part->y;
			if( d < min_d )
			{
				min_d = d;
				min_x = m_sel_part->x;
				min_y = m_sel_part->y;
			}

			// select el
			m_sel_part->selected = ELEMENT_SELECTED;
			m_sel_count++;			
			
			// end of loop, get next group part
			g_part = g_pl->GetNextPart( g_part );
		}

		// add nets from group
		// rename net if necessary
		CString g_suffix;
		if( dlg.m_net_rename_option == 0 )
		{
			// get highest group suffix already in project
			int max_g_num = 0;
			cnet * net = nl->GetFirstNet();
			while( net )
			{
				int n = net->name.ReverseFind( '_' );
				if( n > 0 )
				{
					CString prefix;
					CString test_suffix = net->name.Right( net->name.GetLength() - n - 1 );
					int g_num = ParseRef( &test_suffix, &prefix );
					if( prefix.Left(2) == "$G" )
						max_g_num = max( g_num, max_g_num );
				}
				net = nl->GetNextNet(/*LABEL*/);
			}
			g_suffix.Format( "_$G%d", max_g_num + 1 );
		}
		// now loop through all nets in group and add or merge with project
		cnet * g_net = g_nl->GetFirstNet();	// group net
		while( g_net )
		{

			// see if there are routed segments in this net
			BOOL bRouted = FALSE;
			if( dlg.m_pin_net_option == 1 )
			{
				for( int ic=0; ic<g_net->nconnects; ic++ )
				{
					cconnect * c = &g_net->connect[ic];
					for( int is=0; is<c->nsegs; is++ )
					{
						if( c->seg[is].width > 0 )
						{
							bRouted = TRUE;
							break;
						}
					}
					if( bRouted )
						break;
				}
			}
			// only add if there are areas, or routed segments if requested
			if( (dlg.m_pin_net_option == 0 || bRouted) || g_net->nareas > 0 )
			{
				// OK, add this net to project
				// utility flag is set in the Group Paste dialog for nets which
				// should be merged (i.e. not renamed)
				if( dlg.m_net_name_option == 1 && g_net->utility == 0 )
				{
					// rename net
					CString new_name;
					if( dlg.m_net_rename_option == 1 )
					{
						// get next "Nnnnnn" net name
						cnet * net = nl->GetFirstNet();
						int max_num = 0;
						CString prefix;
						while( net )
						{
							int num = ParseRef( &net->name, &prefix );
							if( prefix == "N" && num > max_num )
								max_num = num;
							net = nl->GetNextNet(/*LABEL*/);
						}
						new_name.Format( "N%05d", max_num+1 );
					}
					else
					{
						// add group suffix
						new_name = g_net->name + g_suffix;
					}
					// add new net
					m_sel_net = nl->AddNet( new_name,
						g_net->def_w, g_net->def_via_w, g_net->def_via_hole_w );

					// set flags
					setbit( m_sel_flags, FLAG_SEL_NET );
					m_sel_net->selected = ELEMENT_SELECTED;
				}
				else
				{
					// merge group net with project net of same name
					m_sel_net = nl->GetNetPtrByName( &g_net->name );
					if( !m_sel_net )
					{
						// no project net with the same name
						m_sel_net = nl->AddNet( g_net->name,
							g_net->def_w, g_net->def_via_w, g_net->def_via_hole_w );
					}
					// set flags
					setbit( m_sel_flags, FLAG_SEL_NET );
					m_sel_net->selected = ELEMENT_SELECTED;
				}
				if( !m_sel_net )
					ASSERT(0);
				// now create map for renaming tees
				CMap<int,int,int,int> tee_map;
				// connect group part pins to project net
				for( int ip=0; ip<g_net->npins; ip++ )
				{
					cpin * pin = &g_net->pin[ip];
					BOOL bAdd = TRUE;
					if( dlg.m_pin_net_option == 1 )
					{
						// only add pin if connected to a routed trace
						bAdd = FALSE;
						for( int ic=0; ic<g_net->nconnects; ic++ )
						{
							cconnect * c = &g_net->connect[ic];
							if( c->start_pin == ip || c->end_pin == ip )
							{
								for( int is=0; is<c->nsegs; is++ )
								{
									if( c->seg[is].width > 0 )
									{
										bAdd = TRUE;
										break;
									}
								}
							}
							if( bAdd )
								break;
						}
					}
					if( bAdd && nl->FindPin( &pin->ref_des, &pin->pin_name ) == NULL )
						nl->AddNetPin( m_sel_net, &pin->ref_des, &pin->pin_name, FALSE );
				}
				// create new traces
				for( int g_ic=0; g_ic<g_net->nconnects; g_ic++ )
				{
					cconnect * g_c = &g_net->connect[g_ic];
					// get start pin of connection in new net
					CString g_start_ref_des = g_net->pin[g_c->start_pin].ref_des;
					CString g_start_pin_name = g_net->pin[g_c->start_pin].pin_name;
					int new_start_pin = nl->GetNetPinIndex( m_sel_net, &g_start_ref_des, &g_start_pin_name );
					// get end pin of connection in new net
					CString g_end_ref_des;
					CString g_end_pin_name;
					int new_end_pin = cconnect::NO_END;
					if( g_c->end_pin != cconnect::NO_END )
					{
						g_end_ref_des = g_net->pin[g_c->end_pin].ref_des;
						g_end_pin_name = g_net->pin[g_c->end_pin].pin_name;
						new_end_pin = nl->GetNetPinIndex( m_sel_net, &g_end_ref_des, &g_end_pin_name );
					}
					if( new_start_pin != -1 && (new_end_pin != -1 || g_c->end_pin == cconnect::NO_END) )
					{
						// add connection to new net
						int ic;
						if( new_end_pin != cconnect::NO_END )
							ic = nl->AddNetConnect( m_sel_net, new_start_pin, new_end_pin, g_c->vtx[0].x, g_c->vtx[0].y, g_c->vtx[g_c->nsegs].x, g_c->vtx[g_c->nsegs].y );
						else
							ic = nl->AddNetStub( m_sel_net, new_start_pin, g_c->vtx[0].x, g_c->vtx[0].y );
						nl->UndrawConnection( m_sel_net, ic );
						// copy it and draw it
						if( ic < 0 )
							ASSERT(0);
						else
						{
							// set flags
							setbit( m_sel_flags, FLAG_SEL_CONNECT );

							// set merge
							cconnect * c = &m_sel_net->connect[ic];
							if( g_c->m_merge >= 0 )
							{
								CString pM = m_Doc->clip_mlist->GetMerge( g_c->m_merge );
								int clrn = m_Doc->clip_mlist->GetClearance( g_c->m_merge );
								c->m_merge = m_Doc->m_mlist->AddNew( pM, clrn );
							}

							// copy connection
							c->m_selected = ELEMENT_SELECTED;
							c->nsegs = g_c->nsegs;
							c->seg.SetSize( g_c->nsegs );
							c->vtx.SetSize( g_c->nsegs + 1 );
							for( int is=0; is<c->nsegs; is++ )
							{
								c->seg[is] = g_c->seg[is];
								c->seg[is].m_dlist = m_dlist;
								c->seg[is].dl_el = NULL;
								c->vtx[is] = g_c->vtx[is];
								c->vtx[is].m_dlist = m_dlist;
								c->vtx[is].dl_el = NULL;
								c->vtx[is].dl_hole = NULL;	

								// select el
								m_sel_id.Set( ID_NET, ID_CONNECT, g_ic, ID_SEG, is );
								c->seg[is].selected = ELEMENT_SELECTED;
								m_sel_count++;
								
								// select el
								m_sel_id.Set( ID_NET, ID_CONNECT, g_ic, ID_VERTEX, is );
								c->vtx[is].selected = ELEMENT_SELECTED;
								m_sel_count++;
							}
							c->vtx[c->nsegs] = g_c->vtx[g_c->nsegs];
							c->vtx[c->nsegs].m_dlist = m_dlist;
							c->vtx[c->nsegs].dl_el = NULL;
							c->vtx[c->nsegs].dl_hole = NULL;	
							if( c->end_pin == cconnect::NO_END )
							{
								// select el
								m_sel_id.Set( ID_NET, ID_CONNECT, g_ic, ID_VERTEX, c->nsegs );
								c->vtx[c->nsegs].selected = ELEMENT_SELECTED;
								m_sel_count++;		
							}
							else
							{
								// unselect
								c->vtx[c->nsegs].selected = 0;
							}
							for( int iv=0; iv<c->vtx.GetSize(); iv++ )
							{
								c->vtx[iv].x += dlg.m_dx;
								c->vtx[iv].y += dlg.m_dy;
								if( int g_id = c->vtx[iv].tee_ID )
								{
									// assign new tee_ID
									int new_id;
									BOOL bFound = tee_map.Lookup( g_id, new_id );
									if( !bFound )
									{
										new_id = nl->GetNewTeeID();
										tee_map.SetAt( g_id, new_id );
									}
									c->vtx[iv].tee_ID = new_id;
								}
								// update lower-left corner
								double d = c->vtx[iv].x + c->vtx[iv].y;
								if( d < min_d )
								{
									min_d = d;
									min_x = c->vtx[iv].x;
									min_y = c->vtx[iv].y;
								}
							}
							nl->DrawConnection( m_sel_net, ic );
						}
					}
				}
				// add copper areas
				if( g_net->nareas )
				{
					// set flags
					setbit( m_sel_flags, FLAG_SEL_AREA );

					for( int g_ia=0; g_ia<g_net->nareas; g_ia++ )
					{
						carea * ga = &g_net->area[g_ia];
						CPolyLine * gp = ga->poly;
						int ia = nl->AddArea( m_sel_net, gp->GetLayer(),
							gp->GetX(0), gp->GetY(0), gp->GetHatch() );
						m_sel_net->area[ia].selected = ELEMENT_SELECTED;
						CPolyLine * p = m_sel_net->area[ia].poly;
						m_sel_id = p->GetId();
						m_sel_id.i = ia;
						p->Copy( gp );
						p->SetId( &m_sel_id );
						p->SetPtr( m_sel_net );

						// merge
						int mer = gp->GetMerge();
						if( mer >= 0 )
						{						
							CString pM = m_Doc->clip_mlist->GetMerge( mer );
							int clrn = m_Doc->clip_mlist->GetClearance( mer );
							p->SetMerge(m_Doc->m_mlist->AddNew( pM, clrn ));
						}			
						for( int is=0; is<p->GetNumSides(); is++ )
						{
							int x = p->GetX(is);
							int y = p->GetY(is);
							p->SetX( is, x + dlg.m_dx );
							p->SetY( is, y + dlg.m_dy );							
							m_sel_id.ii = is;

							// select el
							m_sel_id.sst = ID_SIDE;
							p->SetSideSel( is, ELEMENT_SELECTED );
							m_sel_count++;

							// select el
							m_sel_id.sst = ID_CORNER;
							p->SetSel( is, ELEMENT_SELECTED );
							m_sel_count++;

							// update lower-left corner
							double d = x + y;
							if( d < min_d )
							{
								min_d = d;
								min_x = x;
								min_y = y;
							}
						}
						p->Draw( m_sel_net->m_dlist );
					}
				}
			}
			g_net = g_nl->GetNextNet(/*LABEL*/);
		}

		// add poly
		int grp_size = g_op->GetSize();
		int old_size = op->GetSize();
		if( grp_size > 0 )
		{
			// set flags
			setbit( m_sel_flags, FLAG_SEL_OP );
			op->SetSize( old_size + grp_size );
			for( int g_ism=0; g_ism<grp_size; g_ism++ )
			{
				int ism = g_ism + old_size;
				CPolyLine * g_p = &(*g_op)[g_ism];
				CPolyLine * p = &(*op)[ism];
				p->Copy( g_p );
				p->SetDisplayList( m_Doc->m_dlist );
				///
				m_sel_id = p->GetId();
				m_sel_id.i = ism;
				p->SetId( &m_sel_id );
				
				// merge
				int mer = g_p->GetMerge();
				if( mer >= 0 )
				{
					CString pM = m_Doc->clip_mlist->GetMerge( mer );
					int clrn = m_Doc->clip_mlist->GetClearance( mer );
					p->SetMerge(m_Doc->m_mlist->AddNew( pM, clrn ) );
				}
				int lim = p->GetNumCorners();
				int cl = p->GetClosed();
				for( int is=0; is<lim; is++ )
				{
					
					int x = p->GetX(is);
					int y = p->GetY(is);
					p->SetX( is, x + dlg.m_dx );
					p->SetY( is, y + dlg.m_dy );
					m_sel_id.ii = is;

					// select el
					m_sel_id.sst = ID_CORNER;
					p->SetSel( is, ELEMENT_SELECTED );
					m_sel_count++;

					// update lower-left corner
					double d = x + y;
					if( d < min_d )
					{
						min_d = d;
						min_x = x;
						min_y = y;
					}

					if( cl == 0 && is == lim-1 )
						continue;

					// select el
					m_sel_id.sst = ID_SIDE;
					p->SetSideSel( is, ELEMENT_SELECTED );
					m_sel_count++;
				}
				p->Draw( p->GetDisplayList() );
			}
		}

		// add text
		CText * t = g_tl->GetFirstText();
		int it = 0;
		while( t )
		{
			m_sel_text = m_Doc->m_tlist->AddText( t->m_x+dlg.m_dx, t->m_y+dlg.m_dy, t->m_angle,
				t->m_mirror, t->m_bNegative, t->m_layer, t->m_font_size, t->m_stroke_width,
				&t->m_str, TRUE );

			// set flags
			setbit( m_sel_flags, FLAG_SEL_TEXT );

			// select el
			m_sel_id.Set(ID_TEXT_DEF);
			m_sel_text->m_selected = ELEMENT_SELECTED;
			m_sel_count++;

			// merge
			if( t->m_merge >= 0 )
			{
				CString pM = m_Doc->clip_mlist->GetMerge( t->m_merge );
				int clrn = m_Doc->clip_mlist->GetClearance( t->m_merge );
				m_sel_text->m_merge = m_Doc->m_mlist->AddNew( pM, clrn );
			}

			RECT text_bounds;
			m_Doc->m_tlist->GetTextRectOnPCB( m_sel_text, &text_bounds );
			double d = text_bounds.left + text_bounds.bottom;
			if( d < min_d )
			{
				min_d = d;
				min_x = text_bounds.left;
				min_y = text_bounds.bottom;
			}
			t = g_tl->GetNextText(&it);
		}	

		// save undo info
		SaveUndoInfoForGroup( UNDO_GROUP_ADD, m_Doc->m_undo_list );

		if( bDragGroup )
		{
			if( min_x == INT_MAX || min_y == INT_MAX )
				AfxMessageBox( "No items to drag" );
			else
			{
				SetFKText( CUR_GROUP_SELECTED );
				Invalidate( FALSE );
				StartDraggingGroup( TRUE, min_x, min_y );
			}
		}
		else
		{
			if( m_sel_count )
				SetCursorMode( CUR_GROUP_SELECTED );
			HighlightGroup();
			if( m_Doc->m_vis[LAY_RAT_LINE] )
			{
				for( net=m_Doc->m_nlist->GetFirstNet(); net; net=m_Doc->m_nlist->GetNextNet(/*LABEL*/) )
				{
					m_Doc->m_nlist->OptimizeConnections( net, -1, m_Doc->m_auto_ratline_disable,
						m_Doc->m_auto_ratline_min_pins, TRUE ); 
				}
			}
			Invalidate(FALSE);
		}
		m_Doc->ProjectModified( TRUE );
	}
	// now destroy modified g_nl and restore links in g_pl
	delete g_nl;
#undef ELEMENT_SELECTED
}
//===============================================================================================
void CFreePcbView::OnGroupStaticHighlight()
{
	m_Doc->m_dlist->SetStatic( EL_STATIC );
	CancelSelection();
	Invalidate( FALSE);
}
//===============================================================================================
void CFreePcbView::OnGroupCancelHighlight()
{
	GroupCancelHighlight();
	Invalidate( FALSE );
}
void CFreePcbView::GroupCancelHighlight()
{
	m_Doc->m_dlist->CancelHighLight( TRUE );
	SetCursorMode(CUR_NONE_SELECTED);
}
void CFreePcbView::OnApproximationArc()
{
	ApproximArc();
	m_Doc->m_dlist->CancelHighLight();
	SetCursorMode( CUR_GROUP_SELECTED );
	HighlightGroup();
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );//OnApproximationArc
}
void CFreePcbView::OnGroupAlignParts()
{
	if( m_sel_flags == PART_ONLY )
	{
		SaveUndoInfoForGroup( UNDO_GROUP_MODIFY, m_Doc->m_undo_list );
		int min_x= INT_MAX, min_y= INT_MAX,
			max_x=INT_MIN, max_y=INT_MIN, count=0;
		for(cpart * p=m_Doc->m_plist->GetFirstPart(); p; p=m_Doc->m_plist->GetNextPart(p))
		{
			if( p->selected )
			{
				p->utility = 1;
				min_x = min( min_x, p->x );
				max_x = max( max_x, p->x );
				min_y = min( min_y, p->y );
				max_y = max( max_y, p->y );
				count++;
			}
			else
				p->utility = 0;
		}
		if( count != m_sel_count || count < 2 )
			return;
		int pt = 0;
		cpart * fp;
		do
		{
			int min_d = DEFAULT;	
			fp = NULL;
			for(cpart* p=m_Doc->m_plist->GetFirstPart(); p; p=m_Doc->m_plist->GetNextPart(p))
			{
				if( p->utility )
				{
					int d = Distance( p->x, p->y, min_x, min_y );
					if( d < min_d )
					{
						min_d = d;
						fp = p;
					}
				}
			}
			if( fp )
			{		
				if( (max_x-min_x) > (max_y-min_y) )
					m_Doc->m_plist->Move(fp, min_x+(pt*(max_x-min_x)/(count-1)), min_y, fp->angle, fp->side );
				else
					m_Doc->m_plist->Move(fp, min_x, min_y+(pt*(max_y-min_y)/(count-1)), fp->angle, fp->side );
				m_Doc->m_nlist->PartMoved(fp,TRUE);
				m_Doc->m_nlist->SetAreaConnections(fp);
				fp->utility = 0;
				pt++;
			}
		}while( fp );
		m_Doc->m_dlist->CancelHighLight();
		HighlightGroup();
		m_Doc->ProjectModified( TRUE );
	}
	Invalidate( FALSE );//OnGroupAlignParts
}
//===============================================================================================
void CFreePcbView::OnGroupSaveToFile()
{
	// Copy group to pseudo-clipboard
	OnGroupCopy();
	
	// force old-style file dialog by setting size of OPENFILENAME struct
	//
	CString filename = m_Doc->RunFileDialog( 0, "fpc" ); 
	if ( filename.GetLength() )
	{
		if( filename.Right(3).MakeLower() != "fpc" )
		{
			CString S = filename;
			filename.Format( "%s.fpc", S );
		}
		CString pathname = filename;
		// write project file
		CStdioFile pcb_file;
		int err = pcb_file.Open( pathname, CFile::modeCreate | CFile::modeWrite, NULL );
		if( !err )
		{
			// error opening partlist file
			CString mess;
			mess.Format( "Unable to save file %s", pathname );
			AfxMessageBox( mess );
		}
		else
		{
			// write clipboard to file
			try
			{
				// make map of all footprints used by group
				CMapStringToPtr clip_cache_map;
				cpart * part = m_Doc->clip_plist->GetFirstPart();
				while( part )
				{
					void * vp;
					if( part->shape )
						if( !clip_cache_map.Lookup( part->shape->m_name, vp ) )
							clip_cache_map.SetAt( part->shape->m_name, part->shape );
					part = m_Doc->clip_plist->GetNextPart( part );
				}
				m_Doc->WriteOptions( &pcb_file, 0 );
				m_Doc->WriteFootprints( &pcb_file, &clip_cache_map );
				m_Doc->WriteOutlinesPoly( &pcb_file, &m_Doc->clip_outline_poly );
				m_Doc->clip_plist->WriteParts( &pcb_file );
				m_Doc->clip_nlist->WriteNets( &pcb_file );
				m_Doc->clip_tlist->WriteTexts( &pcb_file );
				m_Doc->WriteMerges( &pcb_file, m_Doc->m_mlist );
				pcb_file.WriteString( "[end]\n" );
				pcb_file.Close();
			}
			catch( CString * err_str )
			{
				// error
				AfxMessageBox( *err_str );
				delete err_str;
				CDC * pDC = GetDC();
				OnDraw( pDC );
				ReleaseDC( pDC );
				return;
			}
		}
	}
}
//===============================================================================================
void CFreePcbView::OnEditCopy()
{
	if( !m_Doc->m_project_open )
		return;
	OnGroupCopy();
}
//===============================================================================================
void CFreePcbView::OnEditPaste()
{
	if( !m_Doc->m_project_open )
		return;
	OnGroupPaste(FALSE);
}
//===============================================================================================
void CFreePcbView::OnEditCut()
{
	if( !m_Doc->m_project_open )
		return;
	OnGroupCopy();
	OnGroupDelete();
}
//===============================================================================================
void CFreePcbView::RotateGroup(int angle, BOOL unroute)
{
	if( prev_sel_count != m_sel_count )	
		SaveUndoInfoForGroup( UNDO_GROUP_MODIFY, m_Doc->m_undo_list );
	FindGroupCenter();
	CMainFrame * pMain = (CMainFrame*) AfxGetApp()->m_pMainWnd;
	if( !pMain )
		return;
	
	// parts
	CString str;
	str = "Rotation Parts...";
	pMain->DrawStatus( 3, &str );
	UpdateWindow();
	int COUNT = 0;
	if( getbit(m_sel_flags, FLAG_SEL_PART) )
	{
		for(cpart* p=(m_sel_part&&m_sel_count==1)?m_sel_part:m_Doc->m_plist->GetFirstPart(); p; p=m_Doc->m_plist->GetNextPart(p))
			if( p->selected )
			{
				int x = p->x;
				int y = p->y;
				Rotate_i_Vertex( &x,&y,angle,groupAverageX,groupAverageY );
				int newAng;
				newAng = p->angle - angle;
				if( newAng >= 360 )
					newAng -= 360;
				if( newAng < 0 )
					newAng += 360;
				m_Doc->m_plist->Move(p,x,y,newAng,p->side);
				m_Doc->m_nlist->PartMoved(p,unroute);
				COUNT++;
				if( COUNT >= m_sel_count )
					break;
			}
	}
	
	// connects
	str = "Rotation Connections...";
	pMain->DrawStatus( 3, &str );
	UpdateWindow();
	Invalidate( FALSE );//RotateGroup
	if( getbit(m_sel_flags, FLAG_SEL_CONNECT) )
	{
		for(cnet* n=m_Doc->m_nlist->GetFirstNet(); n; n=m_Doc->m_nlist->GetNextNet(/*LABEL*/))
		{
			for (int icon=0; icon<n->nconnects; icon++)
			{
				cconnect * c = &n->connect[icon];
				if( c->m_selected )
				{
					BOOL RedrawFlag = FALSE;
					for(int iseg=(c->end_pin==cconnect::NO_END?c->nsegs:c->nsegs-1); iseg>0; iseg--)
					{
						cseg * s = &c->seg[min(c->nsegs-1,iseg)];
						cseg * prev_s = &c->seg[max(0,iseg-1)];
						cvertex * v = &c->vtx[iseg];
						if( s->selected || v->selected || prev_s->selected )
						{
							RedrawFlag = TRUE;
							Rotate_i_Vertex( &v->x,&v->y,angle,groupAverageX,groupAverageY );
							COUNT++;
						}
					}
					if( RedrawFlag )
					{
						m_Doc->m_nlist->MergeUnroutedSegments(n,icon);	
						if( COUNT >= m_sel_count )
							break;
					}
				}
			}
			if( COUNT >= m_sel_count )
			{
				m_Doc->m_nlist->CancelNextNet();
				break;
			}
		}
	}
	str = "Rotation Areas...";
	pMain->DrawStatus( 3, &str );
	UpdateWindow();
	Invalidate( FALSE );//RotateGroup

	// convert arcs
	BOOL conv_ARC = FALSE;
	m_Doc->m_nlist->MarkAllNets(0);
	MarkAllOutlinePoly(0,-1);
	if( angle%90 && COUNT<m_sel_count )
	{
		// test on ARCs
		BOOL bTEST = FALSE;
		if( getbit(m_sel_flags, FLAG_SEL_AREA) )
			for(cnet* n=m_Doc->m_nlist->GetFirstNet(); n; n=m_Doc->m_nlist->GetNextNet(/*LABEL*/))
				if( n->selected )					
					for (int i=0; i<n->nareas; i++)
						if( n->area[i].selected )
							for( int ic=n->area[i].poly->GetNumCorners()-1; ic>=0; ic-- )
								if( n->area[i].poly->GetSideStyle(ic) != CPolyLine::STRAIGHT )
								{
									bTEST = TRUE;
									n->area[i].utility2 = 1;
									break;
								}
		if( getbit(m_sel_flags, FLAG_SEL_OP) )
			for( int item=m_Doc->m_outline_poly.GetSize()-1; item>=0; item-- )
			{
				CPolyLine * p = &m_Doc->m_outline_poly[item];
				int gss = p->GetNumCorners()-1;
				int cl = p->GetClosed();
				if( cl == 0 )
					gss--;
				for( int iss=gss; iss>=0; iss-- )
					if( p->GetSideSel(iss) || (p->GetSel(iss) && p->GetSel(p->GetIndexCornerNext(iss)) ) )
						if( p->GetSideStyle(iss) != CPolyLine::STRAIGHT )
							{
								bTEST = TRUE;
								p->SetUtility(1);
								break;
							}
			}
		if( bTEST )
		{
			//Google translate
			//Эта группа содержит в себе арк элементы. 
			//При повороте на  угол не кратный 90 градусов 
			//линия может отображаться некорректно. 
			//Конвертировать арк линии с помощью аппроксиматора?
			if( AfxMessageBox( "This group contains arc elements. When turning at an angle not a multiple of 90 degrees, the line may not be displayed correctly. Convert an arc line using an approximator?", MB_YESNO ) == IDYES )
				conv_ARC = TRUE;
		}
		
	}
	// area
	if( getbit(m_sel_flags, FLAG_SEL_AREA) )
	{
		for(cnet* n=m_Doc->m_nlist->GetFirstNet(); n; n=m_Doc->m_nlist->GetNextNet(/*LABEL*/))
		{
			if( n->selected )
			{					
				for (int i=0; i<n->nareas; i++)
				{
					CPolyLine * p = n->area[i].poly;
					if( conv_ARC && n->area[i].utility2 )
					{
						/*id _id( ID_NET, ID_AREA, i, ID_CORNER, 0 );
						for( int ic=p->GetNumCorners()-1; ic>=0; ic-- )
						{
							_id.ii = ic;
							UnSelect( n, &_id );
						}
						_id.sst = ID_SIDE;
						for( int ic=p->GetNumCorners()-1; ic>=0; ic-- )
						{
							_id.ii = ic;
							UnSelect( n, &_id );
						}*/
						//p->ApproximateAllArcSides();
						ArcApp( p, TRUE, n, i );
						/*for( int ic=p->GetNumCorners()-1; ic>=0; ic-- )
						{
							_id.ii = ic;
							NewSelect( n, &_id, FALSE, FALSE );
						}*/
					}
					BOOL RedrawFlag = FALSE;
					for( int ic=p->GetNumCorners()-1; ic>=0; ic-- )
					{
						int ib = p->GetIndexCornerBack(ic);
						int prev_side_sel = p->GetSideSel(ib);
						int side_sel = p->GetSideSel(ic);
						int corner_sel = p->GetSel(ic);
						if( prev_side_sel || side_sel || corner_sel )
						{
							RedrawFlag = TRUE;
							int x = p->GetX(ic);
							int y = p->GetY(ic);
							Rotate_i_Vertex( &x,&y,angle,groupAverageX,groupAverageY );
							p->SetX(ic,x);
							p->SetY(ic,y);
							COUNT++;
						}
					}
					if( RedrawFlag )
					{
						p->Draw();
						Invalidate( FALSE );//RotateGroup
						if( COUNT >= m_sel_count )
							break;
					}
				}
			}
			if( COUNT >= m_sel_count )
			{
				m_Doc->m_nlist->CancelNextNet();
				break;
			}
		}
	}
	// texts
	str = "Rotation Texts...";
	pMain->DrawStatus( 3, &str );
	UpdateWindow();
	Invalidate( FALSE );//RotateGroup
	if( getbit(m_sel_flags, FLAG_SEL_TEXT) )
	{
		int it = 0;
		for(CText* t=m_Doc->m_tlist->GetFirstText(); t; t=m_Doc->m_tlist->GetNextText(&it))
		{
			if ( t->m_selected )
			{
				int x = t->m_x;
				int y = t->m_y;
				Rotate_i_Vertex( &x,&y,angle,groupAverageX,groupAverageY );
				m_Doc->m_tlist->MoveText(t,x,y,t->m_angle-angle,t->m_mirror,t->m_bNegative,t->m_layer);
				COUNT++;
				if( COUNT >= m_sel_count )
					break;
			}
		}
	}
	// lines
	str = "Rotation Mechanical lines...";
	pMain->DrawStatus( 3, &str );
	UpdateWindow();
	Invalidate( FALSE );//RotateGroup
	if( getbit(m_sel_flags, FLAG_SEL_OP) )
	{
		for( int item=m_Doc->m_outline_poly.GetSize()-1; item>=0; item-- )
		{
			BOOL RedrawFlag = FALSE;
			int gss = m_Doc->m_outline_poly[item].GetNumCorners()-1;
			int cl = m_Doc->m_outline_poly[item].GetClosed();		
			if( conv_ARC && m_Doc->m_outline_poly[item].GetUtility() )
			{
				/*id _id = m_Doc->m_outline_poly[item].GetId();
				_id.sst = ID_CORNER;
				for( int ic=gss; ic>=0; ic-- )
				{
					_id.ii = ic;
					UnSelect( NULL, &_id );
				}
				_id.sst = ID_SIDE;
				for( int ic=(cl?gss-1:gss); ic>=0; ic-- )
				{
					_id.ii = ic;
					UnSelect( NULL, &_id );
				}*/
				//m_Doc->m_outline_poly[item].ApproximateAllArcSides();
				ArcApp( &m_Doc->m_outline_poly[item], TRUE, NULL, NULL );
				gss = m_Doc->m_outline_poly[item].GetNumCorners()-1;
				/*for( int ic=m_Doc->m_outline_poly[item].GetNumSides()-1; ic>=0; ic-- )
				{
					_id.ii = ic;
					NewSelect( NULL, &_id, FALSE, FALSE );
				}*/
			}			
			for( int ic=gss; ic>=0; ic-- )
			{
				CPolyLine * p = &m_Doc->m_outline_poly[item];
				int side_sel;
				if( cl == 0 && ic == gss )
					side_sel = 0;
				else
					side_sel = p->GetSideSel(ic);
				int prev_side_sel;
				int ib = p->GetIndexCornerBack(ic);
				if( cl == 0 && ib == gss )
					prev_side_sel = 0;
				else
					prev_side_sel = p->GetSideSel(ib);
				int corner_sel = p->GetSel(ic);
				if( prev_side_sel || side_sel || corner_sel )
				{
					RedrawFlag = TRUE;
					int x = p->GetX(ic);
					int y = p->GetY(ic);
					Rotate_i_Vertex( &x,&y,angle,groupAverageX,groupAverageY );
					p->SetX(ic,x);
					p->SetY(ic,y);
					COUNT++;
				}
			}
			if( RedrawFlag )
			{
				m_Doc->m_outline_poly[item].Draw();
				if( COUNT >= m_sel_count )
					break;
			}
		}
	}
	str = "Rotation Completed...";
	pMain->DrawStatus( 3, &str );
	if( m_sel_count )
		SetCursorMode( CUR_GROUP_SELECTED);
	UpdateWindow();				
	// repair
	m_Doc->m_nlist->RepairAllBranches(FALSE);
}


//===============================================================================================
void CFreePcbView::FindGroupCenter()
{
	int groupNumberItems=0;
	groupAverageX=groupAverageY=0;

	// parts
	if( getbit( m_sel_flags, FLAG_SEL_PART ) )  
	{
		cpart * part = m_Doc->m_plist->GetFirstPart();
		while( part )
		{
			if( part->selected )
			{
				groupAverageX+=part->x;
				groupAverageY+=part->y;
				groupNumberItems++;
			}
			part = m_Doc->m_plist->GetNextPart( part );
		}
	}

	// texts 
	if( getbit( m_sel_flags, FLAG_SEL_TEXT ) )
	{
		CText * t = m_Doc->m_tlist->GetFirstText();
		int it = 0;
		while( t )
		{
			if( m_Doc->m_vis[t->m_layer] )
			{
				if( t->m_selected )
				{
					groupAverageX += t->m_x;
					groupAverageY += t->m_y;
					groupNumberItems++;
				}
			}
			t = m_Doc->m_tlist->GetNextText(&it);
		}
	}

	// nets
	if( getbit( m_sel_flags, FLAG_SEL_NET ) )
	{	
		for( cnet * net = m_Doc->m_nlist->GetFirstNet(); net; net = m_Doc->m_nlist->GetNextNet(/*LABEL*/) )
		{
			if( getbit( m_sel_flags, FLAG_SEL_CONNECT ) )
			{
				for( int ic=0; ic<net->nconnects; ic++ )
				{
					cconnect * c = &net->connect[ic];
					for( int is=0; is<c->nsegs; is++ )
					{
						cvertex * pre_v = &c->vtx[is];
						cvertex * post_v = &c->vtx[is+1];
						cseg * s = &c->seg[is];
						if( s->selected )
						{
							groupAverageX+=pre_v->x+post_v->x;
							groupAverageY+=pre_v->y+post_v->y;
							groupNumberItems+=2;
						}
						else if( post_v->selected )
						{
							groupAverageX+=post_v->x;
							groupAverageY+=post_v->y;
							groupNumberItems+=1;
						}
					}
				}
			}
			if( getbit( m_sel_flags, FLAG_SEL_AREA ) )
			{
				for( int ia=0; ia<net->nareas; ia++ )
				{
					carea * a = &net->area[ia];
					CPolyLine * poly = a->poly;
					for( int is=poly->GetNumCorners()-1; is>=0; is-- )
					{
						int is2 = poly->GetIndexCornerNext(is);
						int x1 = poly->GetX(is);
						int y1 = poly->GetY(is);
						int x2 = poly->GetX(is2);
						int y2 = poly->GetY(is2);
						if( poly->GetSideSel(is) )
						{
							groupAverageX+=x1+x2;
							groupAverageY+=y1+y2;
							groupNumberItems+=2;
						}
					}
				}
			}
		}
	}

	// outlines
	if( getbit(m_sel_flags, FLAG_SEL_OP) )
	{
		for( int im=m_Doc->m_outline_poly.GetSize()-1; im>=0; im-- )
		{
			CPolyLine * poly = &m_Doc->m_outline_poly[im];
			int gns = poly->GetNumCorners()-1;
			int cl = poly->GetClosed();
			for( int is=gns; is>=0; is-- )
			{
				int is2 = poly->GetIndexCornerNext(is);
				int x1 = poly->GetX(is);
				int y1 = poly->GetY(is);
				int x2 = poly->GetX(is2);
				int y2 = poly->GetY(is2);
				if( cl == 0 && is == gns )
					continue;
				if( poly->GetSideSel(is) )
				{
					groupAverageX+=x1+x2;
					groupAverageY+=y1+y2;
					groupNumberItems+=2;
				}
			}
		}
	}

	if(groupNumberItems)
	{
		groupAverageX/=groupNumberItems;
		groupAverageY/=groupNumberItems;

		double x=floor(groupAverageX/m_Doc->m_part_grid_spacing +.5);
		groupAverageX=(long long)(m_Doc->m_part_grid_spacing*x);
		x=floor(groupAverageY/m_Doc->m_part_grid_spacing +.5);
		groupAverageY=(long long)(m_Doc->m_part_grid_spacing*x);
	}
}
//===============================================================================================
// save undo info for part, prior to editing operation
// type may be:
//	UNDO_PART_DELETE	if part will be deleted
//	UNDO_PART_MODIFY	if part will be modified (e.g. moved)
//	UNDO_PART_ADD		if part will be added
// for UNDO_PART_ADD, use reference designator to identify part, ignore cpart * part
// on callback, ref_des will be used to find part, then name will be changed to part->ref_des
//
void CFreePcbView::SaveUndoInfoForPart( cpart * part, int type, CString * ref_des, BOOL new_event, CUndoList * list )
{
	undo_part * u_part;
	if( new_event )
		list->NewEvent();
	if( type == CPartList::UNDO_PART_ADD )
		u_part = m_Doc->m_plist->CreatePartUndoRecord( NULL, ref_des );
	else if( ref_des )
		u_part = m_Doc->m_plist->CreatePartUndoRecord( part, ref_des );
	else
		u_part = m_Doc->m_plist->CreatePartUndoRecord( part, &part->ref_des );

	list->Push( type, u_part, &m_Doc->m_plist->PartUndoCallback, u_part->size );

	void * ptr;
	if( new_event )
	{
		if( type == CPartList::UNDO_PART_ADD )
			ptr = CreateUndoDescriptor( list, type, ref_des, NULL, 0, 0, ref_des, NULL );
		else if( ref_des )
			ptr = CreateUndoDescriptor( list, type, &part->ref_des, NULL, 0, 0, ref_des, NULL );
		else
			ptr = CreateUndoDescriptor( list, type, &part->ref_des, NULL, 0, 0, &part->ref_des, NULL );
		list->Push( UNDO_PART, ptr, &UndoCallback );
	}
}
//===============================================================================================
// save undo info for a part and all nets connected to it
// type may be:
//	UNDO_PART_DELETE	if part will be deleted
//	UNDO_PART_MODIFY	if part will be modified (e.g. moved or ref_des changed)
// note that the ref_des may be different than the part->ref_des
// on callback, ref_des will be used to find part, then name will be changed to part->ref_des
//
void CFreePcbView::SaveUndoInfoForPartAndNets( cpart * part, int type, CString * ref_des, BOOL new_event, CUndoList * list )
{
	if( part == NULL )
		return;
	void * ptr;
	if( new_event )
		list->NewEvent();
	// set utility = 0 for all nets affected
	for( int ip=0; ip<part->pin.GetSize(); ip++ )
	{
		cnet * net = (cnet*)part->pin[ip].net;
		if( net )
			net->utility = 0;
	}
	// save undo info for all nets affected
	for( int ip=0; ip<part->pin.GetSize(); ip++ )
	{
		cnet * net = (cnet*)part->pin[ip].net;
		if( net )
		{
			if( net->utility == 0 )
			{
				SaveUndoInfoForNetAndConnections( net, CNetList::UNDO_NET_MODIFY, FALSE, list );
				net->utility = 1;
			}
		}
	}
	// save undo info for part
	SaveUndoInfoForPart( part, type, ref_des, FALSE, list );

	// save top-level descriptor
	if( new_event )
	{
		if( ref_des )
			ptr = CreateUndoDescriptor( list, type, &part->ref_des, NULL, 0, 0, ref_des, NULL );
		else
			ptr = CreateUndoDescriptor( list, type, &part->ref_des, NULL, 0, 0, &part->ref_des, NULL );
		list->Push( UNDO_PART_AND_NETS, ptr, &UndoCallback );
	}
}
//===============================================================================================
// save undo info for a net (not connections or areas)
//
void CFreePcbView::SaveUndoInfoForNet( cnet * net, int type, BOOL new_event, CUndoList * list )
{
	void * ptr;
	if( new_event )
		list->NewEvent();
	undo_net * u_net = m_Doc->m_nlist->CreateNetUndoRecord( net );
	list->Push( type, u_net, &m_Doc->m_nlist->NetUndoCallback, u_net->size );
}
//===============================================================================================
// save undo info for a net and connections, not areas
//
void CFreePcbView::SaveUndoInfoForNetAndConnections( cnet * net, int type, BOOL new_event, CUndoList * list )
{
	void * ptr;
	if( new_event )
		list->NewEvent();
	if( type != CNetList::UNDO_NET_ADD )
		for( int ic=net->nconnects-1; ic>=0; ic-- )
		{
			//cconnect * c = &net->connect[ic];
			SaveUndoInfoForConnection( net, ic, FALSE, list );
		}
	SaveUndoInfoForNet( net, type, FALSE, list );
	if( new_event )
	{
		ptr = CreateUndoDescriptor( list, type, &net->name, NULL, 0, 0, NULL, NULL );
		list->Push( UNDO_NET_AND_CONNECTIONS, ptr, &UndoCallback );
	}
}
//===============================================================================================
// save undo info for a connection
// Note: this is now ONLY called from other Undo functions, it should never be used on its own
//
void CFreePcbView::SaveUndoInfoForConnection( cnet * net, int ic, BOOL new_event, CUndoList * list )
{
	if( new_event )
		list->NewEvent();
	undo_con * u_con = m_Doc->m_nlist->CreateConnectUndoRecord( net, ic );
	list->Push( CNetList::UNDO_CONNECT_MODIFY, u_con,
		&m_Doc->m_nlist->ConnectUndoCallback, u_con->size );
}
//===============================================================================================
// top-level description of undo operation
// list is the CUndoList that it will be pushed to
//
void * CFreePcbView::CreateUndoDescriptor( CUndoList * list, int type, CString * name1, CString * name2,
										  int int1, int int2, CString * str1, void * ptr )
{
	undo_descriptor * u_d = new undo_descriptor;//ok
	u_d->view = this;
	u_d->list = list;
	u_d->type = type;
	if( name1 )
		u_d->name1 = *name1;
	if( name2 )
		u_d->name2 = *name2;
	u_d->int1 = int1;
	u_d->int2 = int2;
	if( str1 )
		u_d->str1 = *str1;
	u_d->ptr = ptr;
	return (void*)u_d;
}
//===============================================================================================
// initial callback from undo/redo stack
// used to push redo/undo info onto the other stack
// note this is a static function (i.e. global)
//
void CFreePcbView::UndoCallback( int type, void * ptr, BOOL undo )
{
	undo_descriptor * u_d = (undo_descriptor*)ptr;
	if( undo )
	{
		CFreePcbView * view = u_d->view;
		view->m_Doc->m_dlist->CancelHighLight();
		// if callback was from undo_list, push info to redo list, and vice versa
		CUndoList * redo_list;
		if( u_d->list == view->m_Doc->m_undo_list )
			redo_list = view->m_Doc->m_redo_list;
		else
			redo_list = view->m_Doc->m_undo_list;
		undo_text * u_text = (undo_text *)u_d->ptr;
		// save undo/redo info
		if( type == UNDO_PART )
		{
			cpart * part = view->m_Doc->m_plist->GetPart( u_d->str1 );	//use new ref des
			if( part )
			{
				if( u_d->type == CPartList::UNDO_PART_ADD )
				{
					view->SaveUndoInfoForPartAndNets( part, CPartList::UNDO_PART_DELETE, &u_d->str1, TRUE, redo_list );
				}
				else if( u_d->type == CPartList::UNDO_PART_MODIFY )
				{
					if( part->ref_des.Compare( u_d->name1 ) == 0 )
						view->SaveUndoInfoForPart( part, CPartList::UNDO_PART_MODIFY, &u_d->name1, TRUE, redo_list );
					else
						redo_list->Clear();
				}
			}
		}
		else if( type == UNDO_PART_AND_NETS )
		{
			cpart * part = view->m_Doc->m_plist->GetPart( u_d->str1 );
			if(u_d->type == CPartList::UNDO_PART_DELETE )
				view->SaveUndoInfoForPart( NULL, CPartList::UNDO_PART_ADD, &u_d->name1, TRUE, redo_list );
			else if( part && u_d->type == CPartList::UNDO_PART_MODIFY )
				view->SaveUndoInfoForPartAndNets( part, CPartList::UNDO_PART_MODIFY, NULL, TRUE, redo_list );
		}
		else if( type == UNDO_2_PARTS_AND_NETS )
		{
			cpart * part = view->m_Doc->m_plist->GetPart( u_d->name1 );
			cpart * part2 = view->m_Doc->m_plist->GetPart( u_d->name2 );
			if( part && part2 )
				view->SaveUndoInfoFor2PartsAndNets( part, part2, TRUE, redo_list );
		}
		else if( type == UNDO_NET_AND_CONNECTIONS )
		{
			cnet * net = view->m_Doc->m_nlist->GetNetPtrByName( &u_d->name1 );
			if( net )
				view->SaveUndoInfoForNetAndConnections( net, CNetList::UNDO_NET_MODIFY, TRUE, redo_list );
		}
		else if( type == UNDO_AREA )
		{
			cnet * net = view->m_Doc->m_nlist->GetNetPtrByName( &u_d->name1 );
			if( net )
			{
				if( u_d->type == CNetList::UNDO_AREA_ADD )
					view->SaveUndoInfoForArea( net, u_d->int1, CNetList::UNDO_AREA_DELETE, TRUE, redo_list );
				else if( u_d->type == CNetList::UNDO_AREA_DELETE )
					view->SaveUndoInfoForArea( net, u_d->int1, CNetList::UNDO_AREA_ADD, TRUE, redo_list );
				else if( type == UNDO_AREA )
					view->SaveUndoInfoForArea( net, u_d->int1, CNetList::UNDO_AREA_MODIFY, TRUE, redo_list );
			}
		}
		else if( type == UNDO_ALL_AREAS_IN_NET )
		{
			cnet * net = view->m_Doc->m_nlist->GetNetPtrByName( &u_d->name1 );
			if( net )
				view->SaveUndoInfoForAllAreasInNet( net, TRUE, redo_list );
		}
		else if( type == UNDO_ALL_AREAS_IN_2_NETS )
		{
			cnet * net1 = view->m_Doc->m_nlist->GetNetPtrByName( &u_d->name1 );
			cnet * net2 = view->m_Doc->m_nlist->GetNetPtrByName( &u_d->name2 );
			if( net1 && net2 )
				view->SaveUndoInfoForAllAreasIn2Nets( net1, net2, TRUE, redo_list );
		}
		else if( type == UNDO_ALL_OP )
		{
			view->SaveUndoInfoForOutlinePoly( UNDO_OP, TRUE, redo_list );
		}
		else if( type == UNDO_TEXT )
		{
			if( u_d->type == CTextList::UNDO_TEXT_ADD )
				view->SaveUndoInfoForText( u_text, CTextList::UNDO_TEXT_DELETE, TRUE, redo_list );
			else if( u_d->type == CTextList::UNDO_TEXT_MODIFY )
			{
				GUID guid = u_text->m_guid;
				CText * text = view->m_Doc->m_tlist->GetFirstText();
				int it = 0;
				while( text && text->m_guid != guid )
				{
					text = view->m_Doc->m_tlist->GetNextText(&it);
				}
				if( !text )
					ASSERT(0);	// guid not found
				view->SaveUndoInfoForText( text, CTextList::UNDO_TEXT_MODIFY, TRUE, redo_list );
			}
			else if( u_d->type == CTextList::UNDO_TEXT_DELETE )
				view->SaveUndoInfoForText( u_text, CTextList::UNDO_TEXT_ADD, TRUE, redo_list );
		}
		else if( type == UNDO_MOVE_ORIGIN )
		{
			view->SaveUndoInfoForMoveOrigin( -u_d->int1, -u_d->int2, redo_list );
		}
		else
			ASSERT(0);
	}
	delete(u_d);	// delete the undo record
}
//===============================================================================================
// callback for undoing group operations
// note this is a static function (i.e. global)
//
void CFreePcbView::UndoGroupCallback( int type, void * ptr, BOOL undo )
{
	undo_group_descriptor * u_d = (undo_group_descriptor*)ptr;
	if( undo )
	{
		CFreePcbView * view = u_d->view;
		CFreePcbDoc * doc = view->m_Doc;
		// if callback was from undo_list, push info to redo list, and vice versa
		CUndoList * redo_list;
		if( u_d->list == view->m_Doc->m_undo_list )
			redo_list = view->m_Doc->m_redo_list;
		else
			redo_list = view->m_Doc->m_undo_list;
		
		if( u_d->type == UNDO_GROUP_DELETE )
		{
			// just copy the undo record with type UNDO_GROUP_ADD
			undo_group_descriptor * new_u_d = new undo_group_descriptor;//ok
			new_u_d->list = redo_list;
			new_u_d->type = UNDO_GROUP_ADD;
			new_u_d->view = u_d->view;
			int n_items = u_d->m_id.GetSize();
			new_u_d->str.SetSize( n_items );
			new_u_d->m_id.SetSize( n_items );
			for( int i=0; i<n_items; i++ )
			{
				new_u_d->m_id[i] = u_d->m_id[i];
				new_u_d->str[i] = u_d->str[i];
			}
			redo_list->NewEvent();
			redo_list->Push( UNDO_GROUP, (void*)new_u_d, &view->UndoGroupCallback );
		}
		else
		{
			// reconstruct pointers from names of items (since they may have changed)
			// and save the current status of the group
			view->CancelSelection();
			int n_items = u_d->m_id.GetSize();
			void * sel;
			for( int i=0; i<n_items; i++ )
			{
				CString * str_ptr = &u_d->str[i];
				id this_id = u_d->m_id[i];
				if( this_id.type == ID_PART )
				{
					cpart * part = doc->m_plist->GetPart( *str_ptr );
					sel = (void*)part;
					if( sel )
						view->NewSelect( part, &this_id, 0, 0 );
				}
				else if( this_id.type == ID_NET )
				{
					cnet * net = doc->m_nlist->GetNetPtrByName( str_ptr );
					sel = (void*)net;
					if( sel )
						view->NewSelect( sel, &this_id, 0, 0 );
				}
				else if( this_id.type == ID_TEXT )
				{
					GUID guid;
					::SetGuidFromString( str_ptr, &guid );
					CText * text = doc->m_tlist->GetText( &guid );
					sel = (void*)text;
					if( sel )
						view->NewSelect( sel, &this_id, 0, 0 );
				}
				else if( this_id.type == ID_POLYLINE )
					view->NewSelect( NULL, &this_id, 0, 0 );
			}
			if( u_d->type == UNDO_GROUP_MODIFY )
			{
				//save group info
				view->SaveUndoInfoForGroup( u_d->type, redo_list );
			}
			else if( u_d->type == UNDO_GROUP_ADD )
			{
				// delete group
				view->SaveUndoInfoForGroup( UNDO_GROUP_DELETE, redo_list, 0, FALSE );
			}
		}
	}
	delete(u_d);	// delete the undo record
}
//===============================================================================================

//===============================================================================================
void CFreePcbView::OnGroupRotate()
{
	RotateGroup(90);
	Invalidate( FALSE );//OnGroupRotate	
}

void CFreePcbView::RotateGroup( int angle )
{
	if( !ThisGroupContainsGluedParts() )
		return;
	m_dlist->CancelHighLight();
	RotateGroup(angle,TRUE);
	m_Doc->m_nlist->OptimizeConnections( m_Doc->m_auto_ratline_disable, 
										 m_Doc->m_auto_ratline_min_pins );
	m_Doc->m_dlist->CancelHighLight();
	HighlightGroup();
	m_Doc->ProjectModified( TRUE );
}


//===============================================================================================
// enable/disable the main menu
// used when dragging
//
void CFreePcbView::SetMainMenu( BOOL bAll )
{
	CFrameWnd * pMainWnd = (CFrameWnd*)AfxGetMainWnd();
	if( bAll )
	{
		pMainWnd->SetMenu(&theApp.m_main);
		CMenu* pMenu = pMainWnd->GetMenu();
		CMenu* submenu = pMenu->GetSubMenu(0);
		if( m_Doc->m_netlist_completed )
			submenu->EnableMenuItem( ID_FILE_IMPORTNETLIST, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );
		else
			submenu->EnableMenuItem( ID_FILE_IMPORTNETLIST, MF_BYCOMMAND | MF_ENABLED );
		if( m_Doc->m_project_modified )
			m_Doc->ProjectModified( TRUE, FALSE );
	}
	else
		pMainWnd->SetMenu(&theApp.m_main_drag);
	return;
}

void CFreePcbView::VertexMoved()
{
	if( m_sel_id.st == ID_CONNECT &&
		(m_sel_id.sst == ID_SEG || m_sel_id.sst == ID_VERTEX) )
	{
		if( m_draw_layer > 0 )
		{
				m_draw_layer = mod_active_layer; //VertexMoved
			if( m_sel_is < m_sel_con.nsegs )
				if( m_sel_seg.layer != mod_active_layer )
					m_draw_layer = DISABLE_CHANGE_DRAW_LAYER; //VertexMoved
			if( m_sel_is )
				if( m_sel_last_seg.layer != mod_active_layer )
					m_draw_layer = DISABLE_CHANGE_DRAW_LAYER; //VertexMoved
		}
	}
}

//===============================================================================================
void CFreePcbView::SelectMergeSegments( cpart * mP )
{
	id seg_id( ID_NET, ID_CONNECT, 0, ID_SEG, 0 );
	for( cnet * n = m_Doc->m_nlist->GetFirstNet(); n; n = m_Doc->m_nlist->GetNextNet(/*LABEL*/) ) 
	{
		for( int icn=0; icn<n->nconnects; icn++ )
		{
			cconnect * cc = &n->connect[icn];
			seg_id.i = icn;
			if( cc->end_pin == cconnect::NO_END )
			{
				if( n->pin[cc->start_pin].part == mP )
					if( mP->m_merge == cc->m_merge )
						for( seg_id.ii=0; seg_id.ii<cc->nsegs; seg_id.ii++ )
							if( mP->selected )
							{
								UnSelect( n, & seg_id );
								id v_id( ID_NET, ID_CONNECT, icn, ID_VERTEX, seg_id.ii+1 );
								UnSelect( n, & v_id );
							}
							else
								NewSelect( n, & seg_id, 0, 0 );
			}
			else
			{
				if( (n->pin[cc->start_pin].part == mP && n->pin[cc->end_pin].part->selected ) ||
					(n->pin[cc->end_pin].part == mP && n->pin[cc->start_pin].part->selected ) ||
					(n->pin[cc->end_pin].part == mP && n->pin[cc->start_pin].part == mP) )
					if( mP->m_merge == cc->m_merge )
						for( seg_id.ii=0; seg_id.ii<cc->nsegs; seg_id.ii++ )
							if( mP->selected )
							{
								UnSelect( n, & seg_id );
								id v_id( ID_NET, ID_CONNECT, icn, ID_VERTEX, seg_id.ii+1 );
								UnSelect( n, & v_id );
							}
							else
								NewSelect( n, & seg_id, 0, 0 );
			}
		}
	}
}

//===============================================================================================
void CFreePcbView::SelectSimilarParts( BOOL includeValues )
{
	CString p_p = m_sel_part->shape->m_package;
	CString p_f = m_sel_part->shape->m_name;
	CString p_v = m_sel_part->value;
	CancelSelection();
	id Id( ID_PART_DEF );
	for (cpart * p = m_Doc->m_plist->GetFirstPart(); p; p = m_Doc->m_plist->GetNextPart(p))
	{
		if( p->shape )
		{
			CString g_p = p->shape->m_package;
			CString g_f = p->shape->m_name;
			CString g_v = p->value;
			if ( p_p.Compare(g_p) == 0 || (p_p.GetLength() == 0 && g_p.GetLength() == 0) )
				if ( p_f.Compare(g_f) == 0 )
					if (( p_v.Compare(g_v) == 0 || (p_v.GetLength() == 0 && g_v.GetLength() == 0) ) || includeValues == 0 )
					{	
						NewSelect( p, &Id, 0, 0 );
						SelectPart(p);
					}
		}
	}
	if( m_sel_count )
		NewSelect( m_sel_part, &Id, 1, 0 );
}
//===============================================================================================
void CFreePcbView::OnRefShowPart( cpart * p, BOOL highlight )
{
	dl_element * dl_sel = p->dl_sel;
	if( p->dl_sel )
	{
		RECT Get;
		m_dlist->Get_Rect( dl_sel, &Get );
		int xc = (Get.right + Get.left)/2;
		int yc = (Get.top + Get.bottom)/2;
		m_org_x = xc - ((m_client_r.right-m_left_pane_w)*m_pcbu_per_pixel)/2;
		m_org_y = yc - ((m_client_r.bottom-m_bottom_pane_h)*m_pcbu_per_pixel)/2;
		CRect screen_r;
		GetWindowRect( &screen_r );
		m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel,
			m_org_x, m_org_y );
		if( m_cursor_mode == CUR_PART_SELECTED || 
			m_cursor_mode == CUR_GROUP_SELECTED || 
			m_cursor_mode == CUR_NONE_SELECTED)
		{
			m_sel_part = p;
			m_sel_id = p->m_id;
			m_sel_id.st = ID_SEL_RECT;
			if (highlight)
			{	
				NewSelect( m_sel_part, &m_sel_id, 1, 0 );
				gLastKeyWasArrow = FALSE;
			}
		}
		
		m_Doc->m_vis[LAY_TOP_COPPER+p->side] = 1;
		m_Doc->m_dlist->SetLayerVisible( LAY_TOP_COPPER+p->side, 1 );
		m_active_layer = LAY_TOP_COPPER+p->side;
		ShowActiveLayer( m_Doc->m_num_copper_layers );
	}
	Invalidate(FALSE);//OnRefShowPart
}
//===============================================================================================
void CFreePcbView::OnAreaSideStyle()
{
	CDlgSideStyle dlg;
	int style = m_sel_net->area[m_sel_ia].poly->GetSideStyle( m_sel_id.ii );
	dlg.Initialize( style );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		SaveUndoInfoForArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_MODIFY, TRUE, m_Doc->m_undo_list );
		m_dlist->CancelHighLight();
		m_sel_net->area[m_sel_ia].poly->SetSideStyle( m_sel_id.ii, dlg.m_style );
		m_Doc->m_nlist->SelectAreaSide( m_sel_net, m_sel_ia, m_sel_id.ii );
		m_Doc->m_nlist->SetAreaConnections( m_sel_net, m_sel_ia );
		m_Doc->m_nlist->OptimizeConnections(  m_sel_net, -1, m_Doc->m_auto_ratline_disable,
														m_Doc->m_auto_ratline_min_pins, TRUE );
	}
	m_Doc->ProjectModified( TRUE );
}
//===============================================================================================
// move text string for value
//
void CFreePcbView::OnValueMove()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	// move cursor to part origin
	CPoint cur_p = m_dlist->PCBToScreen( m_last_cursor_point );
	SetCursorPos( cur_p.x, cur_p.y );
	m_dragging_new_item = 0;
	m_Doc->m_plist->StartDraggingValue( pDC, m_sel_part );
	SetCursorMode( CUR_DRAG_VALUE );
	ReleaseDC( pDC );
}
//===============================================================================================
void CFreePcbView::OnValueProperties()
{
	CDlgValueText dlg;
	if (m_sel_part)
		dlg.Initialize( m_Doc->m_plist, m_sel_part );
	else
		return;
	m_dlist->CancelHighLight();
	id id( ID_PART_DEF );
	NewSelect( m_sel_part, &id, 0, 0 );
	int ret =  dlg.DoModal();
	if( ret == IDOK )
	{
		SaveUndoInfoForGroup( UNDO_GROUP_MODIFY, m_Doc->m_undo_list );
		// edit this parts	
		if( getbit(m_sel_flags, FLAG_SEL_PART) )
			for(cpart* p=m_Doc->m_plist->GetFirstPart(); p; p=m_Doc->m_plist->GetNextPart(p))
				if ( p->selected )
				{
					int x = p->m_value_xi,
						y =	p->m_value_yi,
						a =	p->m_value_angle;
					m_Doc->m_plist->SetValue(	p, 
												&p->value, 
												x,
												y,
												a, 
												dlg.m_height, 
												dlg.m_width, 
												dlg.m_vis );
					RECT rTotal, rValue;
					if (dlg.m_def_pos)
					{
						if (m_Doc->m_plist->GetPartBoundingRect(p, &rTotal))
							if (m_Doc->m_plist->GetValueBoundingRect(p, &rValue))
							{
								if ( (rTotal.right - rTotal.left) > (rTotal.top - rTotal.bottom) )
								{
									if ( p->side )
									{
										x = (rTotal.right + rTotal.left)/2 + max((rValue.right - rValue.left),(rValue.top - rValue.bottom))/2;
										a = p->angle;
									}
									else
									{
										x = (rTotal.right + rTotal.left)/2 - max((rValue.right - rValue.left),(rValue.top - rValue.bottom))/2;
										a = -p->angle;
									}
									y = (rTotal.top + rTotal.bottom)/2 - p->m_value_size/2;
											
								}
								else
								{
									x = (rTotal.right + rTotal.left)/2 + p->m_value_size/2;
									if ( p->side )
									{
										y = (rTotal.top + rTotal.bottom)/2 + max((rValue.right - rValue.left),(rValue.top - rValue.bottom))/2;
										a = (90+p->angle);
									}
									else
									{
										y = (rTotal.top + rTotal.bottom)/2 - max((rValue.right - rValue.left),(rValue.top - rValue.bottom))/2;
										a = (-90-p->angle);
									}
								}
								m_Doc->m_plist->MoveValueText(p,x,y,a,dlg.m_height,dlg.m_width);
							}
					}
				}
		m_Doc->ProjectModified( TRUE );
		if( m_cursor_mode == CUR_PART_SELECTED )
			m_Doc->m_plist->HighlightPart( m_sel_part );
		else if( m_cursor_mode == CUR_VALUE_SELECTED 
				&& m_sel_part->m_value_size && m_sel_part->m_value_vis )
			m_Doc->m_plist->SelectValueText( m_sel_part );
		else if( m_cursor_mode == CUR_GROUP_SELECTED )
			HighlightGroup();
		else
			CancelSelection();
		Invalidate( FALSE );//OnValueProperties
	}
}
//===============================================================================================
void CFreePcbView::OnRefShowPart()
{
	cpart * p = m_sel_part;
	CancelSelection();
	OnRefShowPart(p, TRUE);
}
//===============================================================================================
void CFreePcbView::OnValueShowPart()
{
	OnRefShowPart(m_sel_part, TRUE);
}
//===============================================================================================
void CFreePcbView::OnPartEditValue()
{
	OnValueProperties();
}
//===============================================================================================
void CFreePcbView::OnRefRotateCW()
{
	SaveUndoInfoForPart( m_sel_part, CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list ); 
	m_dlist->CancelHighLight();
	m_Doc->m_plist->UndrawPart( m_sel_part );
	m_sel_part->m_ref_angle = (m_sel_part->m_ref_angle + 5)%360;
	m_Doc->m_plist->DrawPart( m_sel_part );
	m_Doc->m_plist->SelectRefText( m_sel_part );
	m_Doc->ProjectModified( TRUE );
}
//===============================================================================================
void CFreePcbView::OnRefRotateCCW()
{
	SaveUndoInfoForPart( m_sel_part, CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list ); 
	m_dlist->CancelHighLight();
	m_Doc->m_plist->UndrawPart( m_sel_part );
	m_sel_part->m_ref_angle = (m_sel_part->m_ref_angle + 355)%360;
	m_Doc->m_plist->DrawPart( m_sel_part );
	m_Doc->m_plist->SelectRefText( m_sel_part );
	m_Doc->ProjectModified( TRUE );
}
//===============================================================================================
void CFreePcbView::OnValueRotateCW()
{
	SaveUndoInfoForPart( m_sel_part, CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list ); 
	m_dlist->CancelHighLight();
	m_Doc->m_plist->UndrawPart( m_sel_part );
	m_sel_part->m_value_angle = (m_sel_part->m_value_angle + 5)%360;
	m_Doc->m_plist->DrawPart( m_sel_part );
	m_Doc->m_plist->SelectValueText( m_sel_part );
	m_Doc->ProjectModified( TRUE );
}
//===============================================================================================
void CFreePcbView::OnValueRotateCCW()
{
	SaveUndoInfoForPart( m_sel_part, CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list ); 
	m_dlist->CancelHighLight();
	m_Doc->m_plist->UndrawPart( m_sel_part );
	m_sel_part->m_value_angle = (m_sel_part->m_value_angle + 355)%360;
	m_Doc->m_plist->DrawPart( m_sel_part );
	m_Doc->m_plist->SelectValueText( m_sel_part );
	m_Doc->ProjectModified( TRUE );
}
//===============================================================================================
void CFreePcbView::OnSegmentMove()
{

	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	id id = m_sel_id;
	int ic = m_sel_id.i;
	int ivtx = m_sel_id.ii;
	m_dragging_new_item = 0;
	
	m_last_pt.x = m_sel_last_vtx.x;
	m_last_pt.y = m_sel_last_vtx.y;

	m_from_pt.x = m_sel_vtx.x;
	m_from_pt.y = m_sel_vtx.y;

	m_to_pt.x = m_sel_next_vtx.x;
	m_to_pt.y = m_sel_next_vtx.y;

	int nsegs = m_sel_con.nsegs;
	int use_third_segment = ivtx < nsegs - 1;
	if(use_third_segment)
	{
		m_next_pt.x = m_sel_next_next_vtx.x;	// Shouldn't really do this if we're off the edge?
		m_next_pt.y = m_sel_next_next_vtx.y;
	}
	CPoint p;
	p.x = (m_to_pt.x - m_from_pt.x) / 2 + m_from_pt.x;
	p.y = (m_to_pt.y - m_from_pt.y) / 2 + m_from_pt.y;
	m_Doc->m_nlist->StartMovingSegment( pDC, m_sel_net, ic, ivtx, p.x, p.y, 2, use_third_segment );
	SetCursorMode( CUR_MOVE_SEGMENT );
	ReleaseDC( pDC );
}


//===============================================================================================

//===============================================================================================
void CFreePcbView::AlignSegments( cnet * n, int ic, int iv, BOOL mirror, float ang )
{
	if( iv == 0 || iv >= n->connect[ic].nsegs )
		return;
	CPoint p, pback, pnext;
	p.x = n->connect[ic].vtx[iv].x;
	p.y = n->connect[ic].vtx[iv].y;
	pback.x = n->connect[ic].vtx[iv-1].x;
	pback.y = n->connect[ic].vtx[iv-1].y;
	pnext.x = n->connect[ic].vtx[iv+1].x;
	pnext.y = n->connect[ic].vtx[iv+1].y;
	m_dlist->CancelHighLight();
	CPoint P = AlignPoints(p,pback,pnext,mirror, ang>BY_ZERO? ang:m_Doc->m_snap_angle);
	SaveUndoInfoForNetAndConnections( n, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
	m_Doc->m_nlist->MoveVertex( n, ic, iv, P.x, P.y );
	m_Doc->m_nlist->HighlightVertex( n, ic, iv );
}
//===============================================================================================
void CFreePcbView::AlignSides( int type, int ic, int iv)
{
	CPoint p, pback, pnext;
	int B,N;
	switch (type)
	{
	case ID_POLYLINE:
		B =			m_Doc->m_outline_poly[ic].GetIndexCornerBack(iv);
		N =			m_Doc->m_outline_poly[ic].GetIndexCornerNext(iv);
		p.x =		m_Doc->m_outline_poly[ic].GetX(iv);
		p.y =		m_Doc->m_outline_poly[ic].GetY(iv);
		pback.x =	m_Doc->m_outline_poly[ic].GetX(B);
		pback.y =	m_Doc->m_outline_poly[ic].GetY(B);
		pnext.x =	m_Doc->m_outline_poly[ic].GetX(N);
		pnext.y =	m_Doc->m_outline_poly[ic].GetY(N);
		break;
	case ID_NET:
		B =			m_sel_net->area[ic].poly->GetIndexCornerBack(iv);
		N =			m_sel_net->area[ic].poly->GetIndexCornerNext(iv);
		p.x =		m_sel_net->area[ic].poly->GetX(iv);
		p.y =		m_sel_net->area[ic].poly->GetY(iv);
		pback.x =	m_sel_net->area[ic].poly->GetX(B);
		pback.y =	m_sel_net->area[ic].poly->GetY(B);
		pnext.x =	m_sel_net->area[ic].poly->GetX(N);
		pnext.y =	m_sel_net->area[ic].poly->GetY(N);
		break;
	}
	m_dlist->CancelHighLight();
	CPoint P = AlignPoints(p,pback,pnext,TRUE,m_Doc->m_snap_angle);
	switch (type)
	{
	case ID_POLYLINE:
		SaveUndoInfoForOutlinePoly( UNDO_OP, TRUE, m_Doc->m_undo_list );
		m_Doc->m_outline_poly[ic].MoveCorner( iv, P.x, P.y );
		m_Doc->m_outline_poly[ic].HighlightCorner(iv);
		break;
	case ID_NET:
		SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
		m_Doc->m_nlist->MoveAreaCorner( m_sel_net, ic, iv, P.x, P.y );
		m_Doc->m_nlist->HighlightAreaCorner( m_sel_net, ic, iv );
		break;
	}
}
//===============================================================================================
void CFreePcbView::SelectBetween ()
{
	if( m_sel_count == 2 )
	{
		cnet * save_sel_net = m_sel_net;
		id save_sel_id = m_sel_id;
		cnet *n1=0, *n2=0;
		id id1(ID_NET,ID_CONNECT,0,ID_SEG,0), id2(ID_NET,ID_CONNECT,0,ID_SEG,0);
		BOOL completed = FALSE;
		int layer=0;
		for(cnet* n=m_Doc->m_nlist->GetFirstNet(); n; n=m_Doc->m_nlist->GetNextNet(/*LABEL*/))
		{
			if( n->selected )
			{
				for (int i=0; i<n->nconnects; i++)
				{
					if( n->connect[i].m_selected )
					{
						for( int ii=0; ii<n->connect[i].nsegs; ii++ )
							if( n->connect[i].seg[ii].selected )
							{
								if( !n1 )
								{
									n1 = n;
									id1.i = i;
									id1.ii = ii;
									layer = n->connect[i].seg[ii].layer;
								}
								else if( !n2 && layer == n->connect[i].seg[ii].layer )
								{
									n2 = n;
									id2.i = i;
									id2.ii = ii;
									completed = TRUE;
									break;
								}
							}
					}
					if( completed )
						break;
				}
			}
			if( completed )
			{
				m_Doc->m_nlist->CancelNextNet();
				break;
			}
		}

		if(n1&&n2)
		{
			CPoint pv, v, nv, nnv;
			pv.x = n1->connect[id1.i].vtx[id1.ii].x;
			pv.y = n1->connect[id1.i].vtx[id1.ii].y;
			v.x = n1->connect[id1.i].vtx[id1.ii+1].x;
			v.y = n1->connect[id1.i].vtx[id1.ii+1].y;
			nv.x = n2->connect[id2.i].vtx[id2.ii+1].x - n2->connect[id2.i].vtx[id2.ii].x + v.x;
			nv.y = n2->connect[id2.i].vtx[id2.ii+1].y - n2->connect[id2.i].vtx[id2.ii].y + v.y;
			float ang_between = Angle( pv, v, nv );
			if( (ang_between > -5.0 && ang_between < +5.0) || ang_between > 175.0 || ang_between < -175.0 )
			{
				float a1 = Angle( pv.x, pv.y, v.x, v.y );
				nv.x = n2->connect[id2.i].vtx[id2.ii].x;
				nv.y = n2->connect[id2.i].vtx[id2.ii].y;
				nnv.x = n2->connect[id2.i].vtx[id2.ii+1].x;
				nnv.y = n2->connect[id2.i].vtx[id2.ii+1].y;
				CPoint P[4];
				P[0] = pv;
				P[1] = v;
				P[2] = nv;
				P[3] = nnv;
				RotatePOINTS(P,4,-a1,zero);
				//
				BOOL SEL=0;
				int MINX, MAXX, MID, Y, v2;
				if( abs(P[0].x-P[1].x) <= abs(P[2].x-P[3].x) )
				{
					v2 = 1;
					Y = P[0].y;
					MINX = min(P[0].x,P[1].x);
					MAXX = max(P[0].x,P[1].x);
					MID = (P[0].x + P[1].x)/2;
					MINX = min(MINX,max(P[2].x,P[3].x))-_2540;
					MAXX = max(MAXX,min(P[2].x,P[3].x))+_2540;
				}
				else
				{
					v2 = 2;
					Y = P[2].y;
					MINX = min(P[2].x,P[3].x);
					MAXX = max(P[2].x,P[3].x);
					MID = (P[2].x + P[3].x)/2;
					MINX = min(MINX,max(P[0].x,P[1].x))-_2540;
					MAXX = max(MAXX,min(P[0].x,P[1].x))+_2540;
				}
				// scan
				int ii_prev, mid_prev, Y_prev2, CURMINX, CURMAXX, CURMID;
				CPoint PP[2];				    
				cnet * net_prev2;			   
				id id_prev2;
				BOOL b2;
				do	
				{
					b2 = 0;
					for(cnet* n=m_Doc->m_nlist->GetFirstNet(); n; n=m_Doc->m_nlist->GetNextNet(/*LABEL*/))
						for (int i=0; i<n->nconnects; i++)
						{
							ii_prev = -1;
							for( int ii=0; ii<n->connect[i].nsegs; ii++ )
							{
								if( n->connect[i].seg[ii].layer != layer )
									continue;
								if( n->connect[i].seg[ii].selected )
									continue;
								// test 
								if( v2 != 1 && v2 != 2 )
									ASSERT(0);
								//
								PP[0].x = n->connect[i].vtx[ii].x;
								PP[0].y = n->connect[i].vtx[ii].y;
								PP[1].x = n->connect[i].vtx[ii+1].x;
								PP[1].y = n->connect[i].vtx[ii+1].y;
								RotatePOINTS(PP,2,-a1,zero);
								if( min(PP[0].x,PP[1].x) < MAXX && max(PP[0].x,PP[1].x) > MINX )
									if( min(PP[0].y,PP[1].y) < max(Y,P[3-v2].y) && max(PP[0].y,PP[1].y) > min(Y,P[3-v2].y) )
									{
										float a = Angle( PP[0].x,PP[0].y,PP[1].x,PP[1].y );
										if( (a > 175 && a < 185) || a < 5.0 || a > 355.0 )
										{
											BOOL OK = TRUE;
											int mid = (PP[0].x+PP[1].x)/2;
											if( ii_prev >= 0 )
											{
												id sid(ID_NET,ID_CONNECT,i,ID_SEG,ii_prev);
												//if( abs( MID - mid ) < abs( MID - mid_prev ) )
												//	NewSelect(n,&sid,0,TRUE);// cancel selection
												//else
												//	OK = FALSE;
											}
											if( OK )
											{
												if( b2 )
												{
													if( abs( Y - PP[0].y ) < abs( Y - Y_prev2 ) )
													{
													//	if( net_prev2->connect[id_prev2.i].seg[id_prev2.ii].selected )
													//		NewSelect(net_prev2,&id_prev2,0,TRUE);// cancel selection
													}
													else
														OK = FALSE;
												}
												if( OK )
												{
													id sid(ID_NET,ID_CONNECT,i,ID_SEG,ii);
													NewSelect(n,&sid,0,0);
													ii_prev = ii;
													id_prev2 = sid;
													net_prev2 = n;
													Y_prev2 = PP[0].y;
													mid_prev = mid;
													SEL = TRUE;
													b2 = TRUE;
													//
													CURMINX = min( PP[0].x,PP[1].x);
													CURMAXX = max( PP[0].x,PP[1].x);
													CURMID = (PP[0].x + PP[1].x)/2;
													if( v2 == 1 ) //may be not use
													{
														CURMINX = min(CURMINX,max(P[2].x,P[3].x));
														CURMAXX = max(CURMAXX,min(P[2].x,P[3].x));
													}
													else
													{
														CURMINX = min(CURMINX,max(P[0].x,P[1].x));
														CURMAXX = max(CURMAXX,min(P[0].x,P[1].x));
													}//may be n.u.
												}
											}
										}
									}
							}
						}
					if( b2 )
					{
						MINX = CURMINX;
						MAXX = CURMAXX;
						MID = CURMID;
						Y = Y_prev2;
						UpdateWindow();
						Invalidate(FALSE);//SelectBetween
					}
				}while( b2 );
				if( SEL )
				{
					m_sel_net = save_sel_net;
					m_sel_id = save_sel_id;
					return;
				}
			}
		}
	}
	if( getbit(m_sel_flags, FLAG_SEL_NET) )
		for(cnet* n=m_Doc->m_nlist->GetFirstNet(); n; n=m_Doc->m_nlist->GetNextNet(/*LABEL*/))
			if( n->selected )
			{
				if( getbit(m_sel_flags, FLAG_SEL_CONNECT) )
					for (int i=0; i<n->nconnects; i++)
						if( n->connect[i].m_selected )
						{
							int m_start = 0;
							for( int ii=0; ii<n->connect[i].nsegs; ii++ )
								if( n->connect[i].seg[ii].selected )
								{
									m_start = ii;
									break;
								}
							int m_end = 0;
							for( int ii=n->connect[i].nsegs-1; ii>=0; ii-- )
								if( n->connect[i].seg[ii].selected )
								{
									m_end = ii;
									break;
								}
							if( m_end-m_start > 1 )
							{
								for( int ii=m_start+1; ii<m_end; ii++ )
								{
									id Sel( ID_NET, ID_CONNECT, i, ID_SEG, ii );
									NewSelect( n, &Sel, 0,0 );
								}
							}
						}
			}
}
//===============================================================================================
BOOL CFreePcbView::TestSelElements (int mode)
{
	if( getbit(m_sel_flags, FLAG_SEL_PART) || getbit(m_sel_flags, FLAG_SEL_NET) )
	{
		#define RET0 ret0=1;
		#define RET1 ret1=1;
		int SEG_SEL = 0;
		int VTX_SEL = 0;
		if( mode == FOR_FK_PARTS_AUTO_LOC && !getbit(m_sel_flags, FLAG_SEL_PART) )
			return 0;
		for(cnet* n=m_Doc->m_nlist->GetFirstNet(); n; n=m_Doc->m_nlist->GetNextNet(/*LABEL*/))
		{
			if( !n->selected && getbit(m_sel_flags, FLAG_SEL_NET))
				continue;
			for (int i=0; i<n->nconnects; i++)
			{
				if( mode == FOR_FK_PARTS_AUTO_LOC )
				{
					if( n->connect[i].nsegs > 1 )
					{	
						if( n->pin[n->connect[i].start_pin].part->selected )
						{
							if( n->connect[i].seg[0].layer != LAY_RAT_LINE )
							{
								m_Doc->m_nlist->CancelNextNet();
								return 0;
							}
							if( n->connect[i].end_pin >= 0 )
							{
								if( n->pin[n->connect[i].end_pin].part->selected )
								{
									m_Doc->m_nlist->CancelNextNet();
									return 0;
								}
								if( n->connect[i].seg[n->connect[i].nsegs-1].layer != LAY_RAT_LINE )
								{
									m_Doc->m_nlist->CancelNextNet();
									return 0;
								}
							}
						}
					}
				}
				else if( n->connect[i].m_selected )
				{
					int first_a = INT_MIN;// (FOR_FK_RADIUS_UP_DOWN)
					int first_da =INT_MIN;// (FOR_FK_RADIUS_UP_DOWN)
					if( mode == FOR_FK_RADIUS_UP_DOWN )
					{
						if( n->connect[i].seg[0].selected ||
							n->connect[i].seg[n->connect[i].nsegs-1].selected )
						{
							m_Doc->m_nlist->CancelNextNet();
							return 0;
						}
					}
					int post_none = n->connect[i].nsegs;
					for( int iseg=0; iseg<n->connect[i].nsegs; iseg++ )
					{
						cconnect * c =  &n->connect[i];
						cseg * prev_s = &c->seg[max(0,iseg-1)];
						cseg * s =		&c->seg[iseg];
						cseg * post_s = &c->seg[min(iseg+1,c->nsegs-1)];
						BOOL ret0 = 0;
						BOOL ret1 = 0;
						if( c->vtx[iseg].selected )
							VTX_SEL++;
						if( mode == FOR_FK_SET_CLEARANCE )
						{
							if( n->connect[i].seg[0].selected )
								RET0
						}
						if( s->selected )
						{
							SEG_SEL++;
							cvertex * vb = &c->vtx[max(0,iseg-1)];
							cvertex * v =  &c->vtx[iseg];
							cvertex * vn = &c->vtx[iseg+1];
							cvertex * vnn =&c->vtx[min(c->nsegs,iseg+2)];
							float ab = Angle( v->x, v->y, vb->x, vb->y );
							float an = Angle( vnn->x, vnn->y, vn->x, vn->y );
							float a =  Angle( vn->x, vn->y, v->x,  v->y );
							float da_prev = a - ab;
							if( da_prev < -180.0 )
								da_prev += 360.0;
							else if( da_prev > 180.0 )
								da_prev -= 360.0;
							float da_post = an - a;
							if( da_post < -180.0 )
								da_post += 360.0;
							else if( da_post > 180.0 )
								da_post -= 360.0;
							switch( mode )
							{
							case FOR_FK_SET_CLEARANCE:
								if( prev_s->selected && iseg )
									RET0
								if( post_s->selected && (iseg+1)<n->connect[i].nsegs )
									RET0
								break;
							case FOR_FK_APPROXIM_ARC:
								if( !InRange( abs(da_prev)-abs(da_post), -1,1 ) )
									RET0
								if( prev_s->selected )
									RET0
								if( post_s->selected )
									RET0
								/*if( InRange( abs(ab-an), 0,5 ) )
										RET0
								if( InRange( abs(ab-an), 175,180 ) )
										RET0*/
								break;
							case FOR_FK_CONVERT_TO_LINE:
								if( prev_s->selected && iseg )
									RET1
								if( post_s->selected && iseg < c->nsegs-1 )
									RET1
								break;
							case FOR_FK_RADIUS_UP_DOWN:
								if( !prev_s->selected )
								{
									first_a = ab;
									first_da = da_prev;
								}
								break;
							case FOR_FK_INSERT_SEGMENT:
								if( prev_s->selected )
									RET0
								if( post_s->selected )
									RET0
								break;
							case FOR_FK_SELECT_BETWEEN:	
								if( !prev_s->selected && iseg > post_none )
									RET1
								if( !post_s->selected )
									post_none = iseg;
								if( m_sel_count == 2 && s->selected && prev_s->selected && s != prev_s )
									RET0
								break;
							}
						}
						if( ret0 || ret1 )
						{
							m_Doc->m_nlist->CancelNextNet();
							if( ret0 )
								return 0;
							else
								return 1;
						}
					}
				}
			}
		}
		//if( VTX_SEL == 1 && m_sel_count == 1 && mode == FOR_FK_INSERT_SEGMENT )
		//	return TRUE;
		if( mode == FOR_FK_PARTS_AUTO_LOC )
			return TRUE;
		else if( mode == FOR_FK_ALIGN_SEGMENTS ) 
		{
			if( !VTX_SEL )
				return FALSE;
			if( SEG_SEL )
				return FALSE;
		}
		else
		{
			if( !SEG_SEL )
				return FALSE;
			if( VTX_SEL )
				return FALSE;
		}
		if( mode == FOR_FK_SET_CLEARANCE && SEG_SEL < 2 )
			return FALSE;
		if( mode == FOR_FK_CONVERT_TO_LINE ) 
			return FALSE;
		if( mode == FOR_FK_SELECT_BETWEEN )
		{
			if( m_sel_count == 2 )
				return TRUE;
			return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}
//===============================================================================================
void CFreePcbView::OnSetClearance ()
{
	if( !m_sel_count )
		return;
	if( prev_sel_count != m_sel_count )
		SaveUndoInfoForGroup( UNDO_GROUP_MODIFY, m_Doc->m_undo_list );
	m_dlist->CancelHighLight();
	int cl, MARK_U=1;
	RECT SELRECT;
	SELRECT.left = DEFAULT;
	SELRECT.right = -DEFAULT;
	SELRECT.bottom = DEFAULT;
	SELRECT.top = -DEFAULT;
	if (m_seg_clearance < 0)
		cl = 0;
	else
		cl = m_seg_clearance;
	float	x1, y1, x1n, y1n, 
		    x_1, y_1, x_1n, y_1n,
			x2, y2, x2n, y2n, 
			f_an, f_x, f_y, 
			newx, newy, newxn, newyn; 
	id _id, idb, f_id;
	cnet * ptr;
	cnet * ptrb;
	cnet * f_ptr;
	int w1, w2;
	m_Doc->m_nlist->MarkAllNets(0);
	MarkAllOutlinePoly(0,-1);
	if( m_sel_id.type == ID_NET && m_sel_id.st == ID_CONNECT )
	{
		f_ptr = ptr = ptrb = m_sel_net;
		f_id = _id = idb = m_sel_id;	
		x2 = f_x = m_sel_net->connect[_id.i].vtx[_id.ii].x;
		y2 = f_y = m_sel_net->connect[_id.i].vtx[_id.ii].y;
		x2n = m_sel_net->connect[_id.i].vtx[_id.ii+1].x;
		y2n = m_sel_net->connect[_id.i].vtx[_id.ii+1].y;
		m_sel_net->connect[_id.i].seg[_id.ii].utility = MARK_U;
		m_Doc->m_nlist->HighlightVertex(m_sel_net, _id.i, _id.ii);
		m_Doc->m_nlist->HighlightVertex(m_sel_net, _id.i, _id.ii+1);
		w2 = m_sel_net->connect[_id.i].seg[_id.ii].width;
	}
	else if( m_sel_id.type == ID_NET && m_sel_id.st == ID_AREA )
	{
		f_ptr = ptr = ptrb = m_sel_net;
		f_id = _id = idb = m_sel_id;	
		x2 = f_x = m_sel_net->area[_id.i].poly->GetX(_id.ii);
		y2 = f_y = m_sel_net->area[_id.i].poly->GetY(_id.ii);
		int inext = m_sel_net->area[_id.i].poly->GetIndexCornerNext( _id.ii );
		x2n = m_sel_net->area[_id.i].poly->GetX(inext);
		y2n = m_sel_net->area[_id.i].poly->GetY(inext);
		m_sel_net->area[_id.i].poly->SetUtility( _id.ii, MARK_U );
		w2 = abs( m_sel_net->area[_id.i].poly->GetW() );
		m_Doc->m_nlist->HighlightAreaCorner(m_sel_net, _id.i, _id.ii, w2);	
		m_Doc->m_nlist->HighlightAreaCorner( m_sel_net, _id.i, inext, w2);	
	}
	else if( m_sel_id.type == ID_POLYLINE )
	{
		f_ptr = ptr = ptrb = NULL;
		f_id = _id = idb = m_sel_id;	
		x2 = f_x = m_Doc->m_outline_poly[_id.i].GetX(_id.ii);
		y2 = f_y = m_Doc->m_outline_poly[_id.i].GetY(_id.ii);
		int inext = m_Doc->m_outline_poly[_id.i].GetIndexCornerNext( _id.ii );
		x2n = m_Doc->m_outline_poly[_id.i].GetX(inext);
		y2n = m_Doc->m_outline_poly[_id.i].GetY(inext);
		m_Doc->m_outline_poly[_id.i].SetUtility( _id.ii, MARK_U );
		w2 = abs( m_Doc->m_outline_poly[_id.i].GetW() );
		m_Doc->m_outline_poly[_id.i].HighlightCorner( _id.ii, w2 );
		m_Doc->m_outline_poly[_id.i].HighlightCorner( inext, w2 );	
	}
	else return;
	SwellRect( &SELRECT, x2, y2 );
	SwellRect( &SELRECT, x2n, y2n );
	SwellRect( &SELRECT, w2/2 );
	f_an = Angle(x2n,y2n,x2,y2);
	Rotate_Vertex( &f_x, &f_y, -f_an);
	int bMess = 0;
	while (1)
	{
		ptr = FindNEXTSegmentOfGroup ( f_an, &_id, ptr, &f_id, f_ptr );
		if( _id.ii == -1 )
			break;
		// ID CONNECT
		if( _id.type == ID_NET && _id.st == ID_CONNECT )
		{
			cnet * n = (cnet*)ptr;			
			x_1 =	n->connect[_id.i].vtx[_id.ii-1].x;
			y_1 =	n->connect[_id.i].vtx[_id.ii-1].y;
			x1 =	n->connect[_id.i].vtx[_id.ii].x;
			y1 =	n->connect[_id.i].vtx[_id.ii].y;
			x1n =	n->connect[_id.i].vtx[_id.ii+1].x;
			y1n =	n->connect[_id.i].vtx[_id.ii+1].y;
			x_1n = n->connect[_id.i].vtx[_id.ii+2].x;
			y_1n = n->connect[_id.i].vtx[_id.ii+2].y;
			w1 = n->connect[_id.i].seg[_id.ii].width;
			n->connect[_id.i].seg[_id.ii].utility = MARK_U;
		}
		//ID AREA or ID POLYLINE
		else if( (_id.type == ID_NET && _id.st == ID_AREA) || _id.type == ID_POLYLINE )
		{
			cnet * n = NULL;
			CPolyLine * p;
			if( _id.type == ID_POLYLINE )
				p = &m_Doc->m_outline_poly[_id.i];
			else
			{
				n = (cnet*)ptr;
				p = n->area[_id.i].poly;
			}
			int ib = p->GetIndexCornerBack(_id.ii);
			int in = p->GetIndexCornerNext(_id.ii);
			int inn = p->GetIndexCornerNext(in);
			x_1 =	p->GetX(ib);
			y_1 =	p->GetY(ib);
			x1 =	p->GetX(_id.ii);
			y1 =	p->GetY(_id.ii);
			x1n =	p->GetX(in);
			y1n =	p->GetY(in);
			x_1n =  p->GetX(inn);
			y_1n = 	p->GetY(inn);
			w1 =    abs(p->GetW());
			p->SetUtility(_id.ii, MARK_U);
		}
		else
			ASSERT(0);
		if( idb.type == ID_NET && idb.st == ID_CONNECT )
		{
			cnet * n = (cnet*)ptrb;			
			x2 =	n->connect[idb.i].vtx[idb.ii].x;
			y2 =	n->connect[idb.i].vtx[idb.ii].y;
			x2n =	n->connect[idb.i].vtx[idb.ii+1].x;
			y2n =	n->connect[idb.i].vtx[idb.ii+1].y;
			w2 = n->connect[idb.i].seg[idb.ii].width;
		}
		else if( (idb.type == ID_NET && idb.st == ID_AREA) || idb.type == ID_POLYLINE )
		{
			cnet * n = NULL;
			CPolyLine * p;
			if( idb.type == ID_POLYLINE )
				p = &m_Doc->m_outline_poly[idb.i];
			else
			{
				n = (cnet*)ptrb;
				p = n->area[idb.i].poly;
			}
			x2 =	p->GetX(idb.ii);
			y2 =	p->GetY(idb.ii);
			int in = p->GetIndexCornerNext(idb.ii);
			x2n =	p->GetX(in);
			y2n =	p->GetY(in);
			w2 = abs(p->GetW());
		}
		else
			ASSERT(0);
		Rotate_Vertex( &x_1, &y_1, -f_an);
		Rotate_Vertex( &x1, &y1, -f_an);
		Rotate_Vertex( &x1n, &y1n, -f_an);
		Rotate_Vertex( &x_1n, &y_1n, -f_an);
		Rotate_Vertex( &x2, &y2, -f_an);
		Rotate_Vertex( &x2n, &y2n, -f_an);
		if( y1-f_y > 0 )//&& f_an < 180.0 ) || ( y1-f_y < 0 && f_an > 180.0 ))
			newy = y2 + cl + w1/2 + w2/2;
		else
			newy = y2 - cl - w1/2 - w2/2;
		float an_b = Angle(x1,y1,x_1,y_1);
		float an_n = Angle(x_1n,y_1n,x1n,y1n);		
		float dy = y1 - newy;
		newx = x1 - dy/tan(an_b*M_PI/180.0);
		dy = y1n - newy;
		newxn = x1n - dy/tan(an_n*M_PI/180.0);
		newyn = newy;
		Rotate_Vertex(&newx,&newy,	f_an);
		Rotate_Vertex(&newxn,&newyn,f_an);
		if( abs(newx) < DEFAULT/2 && abs(newy) < DEFAULT/2 && abs(newxn) < DEFAULT/2 && abs(newyn) < DEFAULT/2 )
		{
			if( _id.type == ID_NET && _id.st == ID_CONNECT )
			{
				cnet * n = (cnet*)ptr;	
				m_Doc->m_nlist->MoveVertex(n, _id.i, _id.ii, newx, newy);
				m_Doc->m_nlist->MoveVertex(n, _id.i, _id.ii+1, newxn, newyn);
				m_Doc->m_nlist->HighlightVertex(n, _id.i, _id.ii);
				m_Doc->m_nlist->HighlightVertex(n, _id.i, _id.ii+1);
			}
			else if( _id.type == ID_NET && _id.st == ID_AREA )
			{
				cnet * n = (cnet*)ptr;	
				m_Doc->m_nlist->MoveAreaCorner( n, _id.i, _id.ii, newx, newy ); 
				int inext = n->area[_id.i].poly->GetIndexCornerNext(_id.ii);
				m_Doc->m_nlist->MoveAreaCorner( n, _id.i, inext, newxn, newyn );
				for( int icor=0; icor<n->area[_id.i].poly->GetNumCorners(); icor++ )
					if( n->area[_id.i].poly->GetUtility(icor) )
					{
						m_Doc->m_nlist->HighlightAreaCorner( n, _id.i, icor, w1 );
						inext = n->area[_id.i].poly->GetIndexCornerNext(icor);
						m_Doc->m_nlist->HighlightAreaCorner( n, _id.i, inext,  w1 );
					}
			}
			else if( _id.type == ID_POLYLINE )
			{
				CPolyLine * p = &m_Doc->m_outline_poly[_id.i];
				p->SetX( _id.ii, newx );
				p->SetY( _id.ii, newy );
				int inext = p->GetIndexCornerNext(_id.ii);
				p->SetX( inext, newxn );
				p->SetY( inext, newyn );
				p->Draw( m_dlist );
				for( int icor=0; icor<p->GetNumCorners(); icor++ )
					if( p->GetUtility(icor) )
					{
						p->HighlightCorner( icor, w1 );
						int inxt = p->GetIndexCornerNext( icor );
						p->HighlightCorner( inxt, w1 );
					}
			}
			else
				ASSERT(0);
			RECT r1 = rect( newx, newy, newx+10, newy+10 );
			SwellRect( &r1, w1/2 );
			RECT r2 = rect( newxn, newyn, newxn+10, newyn+10 );
			SwellRect( &r2, w1/2 );
			SwellRect( &SELRECT, r1 );
			SwellRect( &SELRECT, r2 );
		}
		else
		{
			bMess = 1;
		}
		ptrb = ptr;
		idb = _id;
	}
	if( SELRECT.left < DEFAULT )
	{
		int lay_hilite_m = 0;
		setbit( lay_hilite_m, LAY_HILITE );
		id _id(0,0,0,0,0);
		RECT r = SELRECT;
		dl_element * el = m_Doc->m_dlist->Add( _id, NULL, 0, DL_HOLLOW_RECT, 1, &r, 1, NULL, 0 );
		m_Doc->m_dlist->HighLight( el );
		setbit( el->map_orig_layer, m_active_layer );
	}
	if( bMess )
		AfxMessageBox("Unable to align colinear segments!");
	m_seg_clearance = cl;
	m_page = 2;
	SetFKText(m_cursor_mode);
	m_Doc->ProjectModified(TRUE);
}





//===============================================================================================
cnet * CFreePcbView::FindNEXTSegmentOfGroup ( float angle, id * _id, cnet * sel_net, id * f_id, cnet * f_net )
{
	float x, y;
	if( f_id->type == ID_NET )
	{
		if( f_id->st == ID_CONNECT )
		{		
			x = f_net->connect[f_id->i].vtx[f_id->ii].x;
			y = f_net->connect[f_id->i].vtx[f_id->ii].y;
		}
		else if( f_id->st == ID_AREA )
		{
			x = f_net->area[f_id->i].poly->GetX(f_id->ii);
			y = f_net->area[f_id->i].poly->GetY(f_id->ii);
		}
		else
			ASSERT(0);
	}
	else if( f_id->type == ID_POLYLINE )
	{
		x = m_Doc->m_outline_poly[f_id->i].GetX(f_id->ii);
		y = m_Doc->m_outline_poly[f_id->i].GetY(f_id->ii);
	}
	else
		ASSERT(0);
	Rotate_Vertex( &x, &y, -angle );
	cnet * ret_n=NULL;
	_id->ii = -1;
	int len, min_len = DEFAULT;
	if( getbit(m_sel_flags, FLAG_SEL_NET) )
		for(cnet* n=m_Doc->m_nlist->GetFirstNet(); n; n=m_Doc->m_nlist->GetNextNet(/*LABEL*/))
		{
			if( getbit(m_sel_flags, FLAG_SEL_CONNECT) )
				for (int i=0; i<n->nconnects; i++)
					if( n->connect[i].m_selected )
						for(int ii=1; ii<n->connect[i].nsegs-1; ii++)
						{
							if (n->connect[i].seg[ii].utility )
								continue;
							if (!n->connect[i].seg[ii].selected )
								continue;
							float vx =	n->connect[i].vtx[ii].x;
							float vy =	n->connect[i].vtx[ii].y;
							float vxn =	n->connect[i].vtx[ii+1].x;
							float vyn =	n->connect[i].vtx[ii+1].y;
							Rotate_Vertex(&vx,&vy,-angle);
							Rotate_Vertex(&vxn,&vyn,-angle);
							len	= min(abs(vy-y),abs(vyn-y));
							if ( len < min_len )
							{
								min_len = len;
								ret_n = n;
								_id->type = ID_NET;
								_id->st = ID_CONNECT;
								_id->sst = ID_SEG;
								_id->i = i;
								_id->ii = ii;
							}
						}
			if( getbit(m_sel_flags, FLAG_SEL_AREA) )
				for (int i=0; i<n->nareas; i++)
					if( n->area[i].selected )
						if( find_side_for_set_clearance( n->area[i].poly, _id, y, angle, min_len ) )
						{
							ret_n = n;
							_id->type = ID_NET;
							_id->st = ID_AREA;
							_id->sst = ID_SIDE;
							_id->i = i;
						}
		}
	if( getbit( m_sel_flags, FLAG_SEL_OP ) )
	{
		for (int i=0; i<m_Doc->m_outline_poly.GetSize(); i++ )
			if( find_side_for_set_clearance( &m_Doc->m_outline_poly[i], _id, y, angle, min_len ) )
			{
				ret_n = NULL;
				id ID = m_Doc->m_outline_poly[i].GetId();
				_id->type = ID.type;
				_id->st = ID.st;
				_id->sst = ID_SIDE;
				_id->i = i;
			}
	}
	return ret_n;
}



BOOL CFreePcbView::find_side_for_set_clearance( CPolyLine * pp, id * _id, int Y, float angle, int min_len )
{
	BOOL RET = NULL;
	int cl = pp->GetClosed();
	int nc = pp->GetNumCorners();
	if( cl == 0 )
		nc--;
	for(int ii=0; ii<nc; ii++)
	{
		if ( pp->GetUtility(ii) )
			continue;
		if (!pp->GetSideSel(ii) )
			continue;
		float vx =	pp->GetX(ii);
		float vy =	pp->GetY(ii);
		int inext = pp->GetIndexCornerNext(ii);
		float vxn =	pp->GetX(inext);
		float vyn =	pp->GetY(inext);
		Rotate_Vertex(&vx,&vy,-angle);
		Rotate_Vertex(&vxn,&vyn,-angle);
		float len	= min(abs(vy-Y),abs(vyn-Y));
		if ( len < min_len )
		{
			min_len = len;
			_id->ii = ii;
			RET = TRUE;
		}
	}
	return RET;
}





//===============================================================================================
int CFreePcbView::InsertSegment (cnet * sel_n, int con, int seg, BOOL FULL_LENGTH )
{
	if ( sel_n->connect[con].vtx[seg].via_w || sel_n->connect[con].vtx[seg].tee_ID )
		return 0;
	if (seg == 0 || seg >= sel_n->connect[con].nsegs)
		return 0;
	float a1 = Angle(	sel_n->connect[con].vtx[seg-1].x,
						sel_n->connect[con].vtx[seg-1].y,
						sel_n->connect[con].vtx[seg].x,
						sel_n->connect[con].vtx[seg].y );
	float a2 = Angle(	sel_n->connect[con].vtx[seg+1].x,
						sel_n->connect[con].vtx[seg+1].y,
						sel_n->connect[con].vtx[seg].x,
						sel_n->connect[con].vtx[seg].y );
	float dx1 = sel_n->connect[con].vtx[seg-1].x - sel_n->connect[con].vtx[seg].x;
	float dy1 = sel_n->connect[con].vtx[seg-1].y - sel_n->connect[con].vtx[seg].y;
	float dx2 = sel_n->connect[con].vtx[seg+1].x - sel_n->connect[con].vtx[seg].x;
	float dy2 = sel_n->connect[con].vtx[seg+1].y - sel_n->connect[con].vtx[seg].y;
	float len1 = sqrt((double)dx1*(double)dx1 + (double)dy1*(double)dy1);
	float len2 = sqrt((double)dx2*(double)dx2 + (double)dy2*(double)dy2);
	float r = min(len1,len2)/1.1;
	if (FULL_LENGTH == 0)
		r = r/2.0;
	m_insert_seg_len = abs(m_insert_seg_len);
	float insert = abs(a1-a2)/2;
	if (insert > 180.0)
		insert -= 360.0;
	else if (insert < -180.0)
		insert += 360.0;
	if (insert > 1.0)
	{
		insert = abs(m_insert_seg_len)/2.0/sin(insert*M_PI/180.0);
		r = min(insert,r);
	}
	float newX1;
	float newY1;
	float newX2;
	float newY2;
	newX1 = sel_n->connect[con].vtx[seg].x + r*cos(a1*M_PI/180.0);
	newY1 = sel_n->connect[con].vtx[seg].y + r*sin(a1*M_PI/180.0);
	newX2 = sel_n->connect[con].vtx[seg].x + r*cos(a2*M_PI/180.0);
	newY2 = sel_n->connect[con].vtx[seg].y + r*sin(a2*M_PI/180.0);
	m_Doc->m_nlist->MoveVertex( sel_n, con, seg, newX1, newY1 );
	int layer = sel_n->connect[con].seg[seg].layer;
	//int w = (sel_n->connect[con].seg[seg-1].width+sel_n->connect[con].seg[seg].width)/2;
	int w = min(sel_n->connect[con].seg[seg-1].width,sel_n->connect[con].seg[seg].width);
	int insert_flag = m_Doc->m_nlist->InsertSegment(sel_n, con, seg,
													newX2, newY2, layer, w, 0, 0, 0 );
	return insert_flag;
}

//===============================================================================================

//===============================================================================================
void CFreePcbView::ApproximArc ()
{
	SaveUndoInfoForGroup( UNDO_GROUP_MODIFY, m_Doc->m_undo_list );
	float p[2], pb[2], p1[2], p2[2], pn[2];
	enum {X = 0, Y};
	int ID = 0;
	m_Doc->m_nlist->MarkAllNets(0);
	MarkAllOutlinePoly(0,-1);
	for(cnet* n=m_Doc->m_nlist->GetFirstNet(); n; n=m_Doc->m_nlist->GetNextNet(/*LABEL*/))
		if( n->selected )
		{
			for (int i=0; i<n->nconnects; i++)
				if( n->connect[i].m_selected )
					for( int ii=n->connect[i].nsegs-2; ii>0; ii-- )
					{
						if( n->connect[i].seg[ii].selected )
						{
							pb[X] = n->connect[i].vtx[ii-1].x;
							pb[Y] = n->connect[i].vtx[ii-1].y;
							p1[X] = n->connect[i].vtx[ii].x;
							p1[Y] = n->connect[i].vtx[ii].y;
							p2[X] = n->connect[i].vtx[ii+1].x;
							p2[Y] = n->connect[i].vtx[ii+1].y;
							pn[X] = n->connect[i].vtx[ii+2].x;
							pn[Y] = n->connect[i].vtx[ii+2].y;
							int layer = n->connect[i].seg[ii].layer;
							int wb = n->connect[i].seg[ii-1].width;
							int wn = n->connect[i].seg[ii+1].width;
							int dw = wn - wb;
							int N_SIDES = N_SIDES_APPROX_ARC;
							if( Distance( p1[X],p1[Y],p2[X],p2[Y] ) > 20*NM_PER_MM )
								N_SIDES *= 3;
							float * data = new float[N_SIDES*4+2];//ok
							int cnt = Rnd_Func(pn[X], pn[Y], p2[X], p2[Y], p1[X], p1[Y], pb[X], pb[Y], &data[0], (N_SIDES_APPROX_ARC*4));
							if( cnt )
							{
								dw = dw/cnt*2;
								int w = wb;
								n->connect[i].seg[ii].width = wn;
								for (int c=2; c<cnt-2; c=c+2)
								{
									wn -= dw;
									int insert_flag;
									insert_flag = m_Doc->m_nlist->InsertSegment( n, i, ii,
										data[c], data[c+1],
										layer, wn, 0, 0, 0 );
									if ( insert_flag )
									{
										id sel( ID_NET, ID_CONNECT, i, ID_SEG, ii );
										NewSelect( n, & sel, 0,0 );
									}
									else
									{
										m_Doc->m_nlist->CancelNextNet();
										return;
									}
								}
							}
							delete data;
						}
					}
			for (int i=0; i<n->nareas; i++)
				if( n->area[i].selected )
				{
					if( ArcApp( n->area[i].poly, FALSE, n, i ) )
						n->area[i].poly->Draw();
				}
		}
		for( int ibo=m_Doc->m_outline_poly.GetSize()-1; ibo>=0; ibo-- )
		{
			if( ArcApp( &m_Doc->m_outline_poly[ibo], FALSE, NULL, NULL ) )
				m_Doc->m_outline_poly[ibo].Draw();
		}
}


int CFreePcbView::ArcApp( CPolyLine * po, int forArcOnly, cnet * m_net, int m_i )
{
	float p[2], pb[2], p1[2], p2[2], pn[2];
	enum {X = 0, Y};
	BOOL bdraw = 0;
	int nc = po->GetNumCorners()-1;
	if( po->GetClosed() == 0 )
		nc--;
	for( int ii=nc; ii>=0; ii-- )
	{
		BOOL bE = (po->GetSideSel(ii) && forArcOnly == 0) || (po->GetSideStyle(ii) && forArcOnly);
		if( bE && po->GetUtility(ii) == 0 )
		{
			bdraw = 1;
			int ss = po->GetSideStyle(ii);
			po->SetSideStyle(ii,CPolyLine::STRAIGHT,0);
			po->SetUtility( ii, 1 );
			int ncont = po->GetNumContour(ii);
			int start = po->GetContourStart(ncont);
			int end = po->GetContourEnd(ncont); 
			int ib = po->GetIndexCornerBack(ii);
			int in = po->GetIndexCornerNext(ii);
			int inn = po->GetIndexCornerNext(in);
			pb[X] = po->GetX(ib);
			pb[Y] = po->GetY(ib);
			p1[X] = po->GetX(ii);
			p1[Y] = po->GetY(ii);
			p2[X] = po->GetX(in);
			p2[Y] = po->GetY(in);
			pn[X] = po->GetX(inn);
			pn[Y] = po->GetY(inn);
			int N_SIDES = N_SIDES_APPROX_ARC;
			if( Distance( p1[X],p1[Y],p2[X],p2[Y] ) > 20*NM_PER_MM )
				N_SIDES *= 3;
			float * data = new float[N_SIDES*4+2];//ok
			int cnt;
			if(ss)
			{
				CPoint * PTS = new CPoint[(N_SIDES*2)+1];//ok
				cnt = Generate_Arc( p1[X], p1[Y], p2[X], p2[Y], ss, PTS, (N_SIDES*2) );
				for(int a=0;a<cnt;a++)
				{
					data[2*a] = PTS[cnt-a-1].x;
					data[2*a+1] = PTS[cnt-a-1].y;
				}
				cnt *= 2;
				delete PTS;
			}
			else
				cnt = Rnd_Func(pn[X], pn[Y], p2[X], p2[Y], p1[X], p1[Y], pb[X], pb[Y], &data[0], (N_SIDES_APPROX_ARC*4));
			if( cnt )
			{
				int cor = ii+1;
				if( ii+1 > end )
					cor = start;
				for (int c=2; c<cnt-2; c=c+2)
				{
					if( m_net )
					{
						m_Doc->m_nlist->InsertAreaCorner( m_net, m_i, cor, data[c], data[c+1], CPolyLine::STRAIGHT );
						id sel( ID_NET, ID_AREA, m_i, ID_SIDE, cor );
						NewSelect( m_net, & sel, 0,0 );
					}
					else
					{
						po->InsertCorner( cor, data[c], data[c+1], 0 );
						id sel = po->GetId();
						sel.sst = ID_SIDE;
						sel.ii = cor;
						NewSelect( NULL, &sel, 0,0 );

					}
					if( cor == start )
					{
						ii++;
						po->SetUtility( cor, 1 );
					}
				}
			}
			delete data;
		}
	}
	return bdraw;
}

//===============================================================================================
int CFreePcbView::GetSelCount()
{
	int cnt = m_Doc->m_plist->GetSelCount();
	for( cnet*n=m_Doc->m_nlist->GetFirstNet(); n; n=m_Doc->m_nlist->GetNextNet(/*LABEL*/) )
		cnt += m_Doc->m_nlist->GetSelCount(n);
	for( int i=m_Doc->m_outline_poly.GetSize()-1; i>=0; i-- )
	{
		int gns = m_Doc->m_outline_poly[i].GetNumCorners()-1;
		int cl = m_Doc->m_outline_poly[i].GetClosed();
		for( int ii=gns; ii>=0; ii-- )
		{
			if( m_Doc->m_outline_poly[i].GetSel(ii) )
				cnt++;
			if( cl == 0 && ii == gns )
				continue;
			if( m_Doc->m_outline_poly[i].GetSideSel(ii) )
				cnt++;
		}
	}
	cnt += m_Doc->m_tlist->GetSelCount();
	return cnt;
}

//===============================================================================================
int CFreePcbView::DeleteWithRecalculationPoint (CPolyLine * p, cconnect * c, BOOL WITH_MERGE)
{
	int mv=-1;
	if( c )
	{
		for(int is=c->nsegs-2; is>0; is--)
		{	
			// если текущий сегмент выделен, а следующий НЕ выделен
			// то запоминаем конец диапазона
			// и удаляем вершины внутри диапазона
			if ( c->seg[is].selected && (!c->seg[is-1].selected || is == 1)  )
			{
				float x1 = c->vtx[is].x;
				float y1 = c->vtx[is].y;
				float bx1 = c->vtx[is-1].x;
				float by1 = c->vtx[is-1].y;
				// delete vertices
				int delseg = is+1;
				while ( delseg<(c->nsegs-1) )
				{
					if ( c->seg[delseg].selected )
					{
						if( c->vtx[delseg].tee_ID || c->vtx[delseg-1].tee_ID )
						{
							delseg++;
							continue;
						}
						// RemoveAt
						c->seg.RemoveAt(delseg-1);		
						c->vtx.RemoveAt(delseg);
						c->nsegs--;	
						if( mv > 0 )
							mv--;
					}
					else
						break;
				}
				float x2 = c->vtx[delseg].x;
				float y2 = c->vtx[delseg].y;
				float nx2 = c->vtx[delseg+1].x;
				float ny2 = c->vtx[delseg+1].y;
				if (WITH_MERGE)
				{
					float a1 = Angle(	bx1,by1,x1,y1  );
					float a2 = Angle(	nx2,ny2,x2,y2  );
					if (InRange((int)abs(a1-a2)%180, 10, 170) )
					{
						float x = x1 - x2;
						float y =  y1 - y2;
						Rotate_Vertex( &x, &y, -a1 );
						float newY = y;
						float newX = -y*tan((90-a1+a2)*M_PI/180.0);
						Rotate_Vertex( &newX, &newY, a1 );
						newX += x2;
						newY += y2;
						delseg--;
						if( !c->vtx[delseg].tee_ID && !c->vtx[delseg+1].tee_ID )
							if( c->nsegs > 1 )
							{
								c->vtx[delseg].x = newX;
								c->vtx[delseg].y = newY;
								// RemoveAt
								c->seg.RemoveAt(delseg);
								c->vtx.RemoveAt(delseg+1);
								c->nsegs--;
								mv = delseg;
							}
					}
				}
				is = min( (c->nsegs-2), is );
			}
		}
	}
	else if( p )
	{
		int start = -1;
		int end = -1;
		int i_beg = -1;
		int cl = p->GetClosed();
		int ncrns = p->GetNumCorners()-1;
		for( int is=ncrns; is>=0; is-- )
		{
			if( is == ncrns || cl ==0 ) 
				continue;
			if( p->GetSideSel( is ))
			{
				if( p->GetSideSel( p->GetIndexCornerNext(is)) == 0 )
					start = is;
				if( p->GetSideSel( p->GetIndexCornerBack(is)) == 0 )
				{
					end = is;
					if ( start == -1 )
					{
						start = p->GetContourStart(p->GetContour(is));
						while ( p->GetSideSel(p->GetIndexCornerNext(start)) )
							start = p->GetIndexCornerNext(start);
					}
				}				
			}			
			if ( end >= 0 )
			{
				int numb_cont = p->GetContour(start);
				if( numb_cont == p->GetContour(end) )
				{
					int bx1 = p->GetX(p->GetIndexCornerBack(end));
					int by1 = p->GetY(p->GetIndexCornerBack(end));
					int x1 = p->GetX(end);
					int y1 = p->GetY(end);
					int s_n = p->GetIndexCornerNext(start);
					int x2 = p->GetX(s_n);
					int y2 = p->GetY(s_n);
					int nx2 = p->GetX(p->GetIndexCornerNext(s_n));
					int ny2 = p->GetY(p->GetIndexCornerNext(s_n));
					float a1 = Angle(	bx1,by1,x1,y1  );
					float a2 = Angle(	nx2,ny2,x2,y2  );
					int dc = start - end;
					if( dc < 0 )
					{
						int cstart = p->GetContourStart(numb_cont);
						int cend = p->GetContourEnd(numb_cont);
						dc = cend - end + start - cstart + 1;
					}
					if( p->GetNumCorners() > 3 )
						p->DeleteCorner(p->GetIndexCornerNext(end),dc,0,0);
					end = min(end,p->GetContourEnd(numb_cont));
					if (InRange((int)abs(a1-a2)%180, 10, 170) && WITH_MERGE )
					{
						float x = x1 - x2;
						float y =  y1 - y2;
						Rotate_Vertex( &x, &y, -a1 );
						float newY = y;
						float newX = -y*tan((90-a1+a2)*M_PI/180.0);
						Rotate_Vertex( &newX, &newY, a1 );
						newX += x2;
						newY += y2;
						mv = p->GetIndexCornerNext(end);
						p->SetX(mv,newX);
						p->SetY(mv,newY);
						if( p->GetNumCorners() > 3 )
							p->DeleteCorner(end,1,0,0);
					}
					is = min(is,(p->GetContourEnd(numb_cont)+1));
				}
				start = -1;
				end = -1;
			}
		}
	}
	return mv;
}
//===============================================================================================

//===============================================================================================
void CFreePcbView::TracesRadiusUpDown(BOOL UP)
{
	if( m_sel_flags != CONNECT_ONLY )
		return;
	m_dlist->CancelHighLight();
	enum
	{
		X=0,Y,
	};
	BOOL flag_ok = FALSE;
	int imin, imax;
	float increment = m_Doc->m_routing_grid_spacing;
	if( increment < 999 )
		increment = m_Doc->m_part_grid_spacing;
	float pb[2], p1[2], p2[2], pn[2], ab, an;

	// Highlight all vertices...
	RECT WinRect;
	WinRect = m_dlist->GetWindowRect();
	m_Doc->m_nlist->MarkAllNets(1);
	for(cnet* n=m_Doc->m_nlist->GetFirstNet(); n; n=m_Doc->m_nlist->GetNextNet(/*LABEL*/))
			if( n->selected )
			{
				if( getbit(m_sel_flags, FLAG_SEL_CONNECT) )
					for (int i=0; i<n->nconnects; i++)
						if( n->connect[i].m_selected )
							for(int ii=0; ii<n->connect[i].nsegs; ii++)
								if( n->connect[i].seg[ii].selected )
								{
									imin = ii;
									imax = ii;
									while ( n->connect[i].seg[imin].selected )
									{
										imin--;
										if ( imin < 0 )
										{
											m_Doc->m_nlist->CancelNextNet();
											return;
										}
									}
									while ( n->connect[i].seg[imax].selected )
									{
										imax++;
										if ( imax >= n->connect[i].nsegs )
										{
											m_Doc->m_nlist->CancelNextNet();
											return;
										}
									}
									pb[X] = n->connect[i].vtx[imin].x;
									pb[Y] = n->connect[i].vtx[imin].y;
									p1[X] = n->connect[i].vtx[imin+1].x;
									p1[Y] = n->connect[i].vtx[imin+1].y;
									p2[X] = n->connect[i].vtx[imax].x;
									p2[Y] = n->connect[i].vtx[imax].y;
									pn[X] = n->connect[i].vtx[imax+1].x;
									pn[Y] = n->connect[i].vtx[imax+1].y;
									ab = Angle (p1[X], p1[Y], pb[X], pb[Y]);
									an = Angle (p2[X], p2[Y], pn[X], pn[Y]);
									float dan = abs(ab-an);
									if (dan > 180.0)
										dan = 360.0 - dan;
									if ( !InRange(dan, 5, 175)  )
										continue;
									Rotate_Vertex (&pb[X], &pb[Y], -ab);
									Rotate_Vertex (&p1[X], &p1[Y], -ab);
									Rotate_Vertex (&p2[X], &p2[Y], -ab);
									Rotate_Vertex (&pn[X], &pn[Y], -ab);
									double ytop;
									ytop = ((pn[Y] - p2[Y]) * p2[X] - ((pn[X] - p2[X]) * (p2[Y] - p1[Y])) );
									float Xc;
									Xc = ytop/(pn[Y] - p2[Y]);
									float Yc;
									Yc = pb[Y];
									float m;
									if (n->connect[i].seg[ii].utility > 1)
										m = n->connect[i].seg[ii].utility;
									else
									{
										m = Distance(p1[X], p1[Y], Xc, Yc);
										for(int ss=imin+1; ss<imax; ss++)
											if( n->connect[i].seg[ss].selected )
												n->connect[i].seg[ss].utility = m;
									}
									Rotate_Vertex (&Xc, &Yc, ab); 
									p1[X] = n->connect[i].vtx[ii].x;
									p1[Y] = n->connect[i].vtx[ii].y;
									p2[X] = n->connect[i].vtx[ii+1].x;
									p2[Y] = n->connect[i].vtx[ii+1].y;
									p1[X] -= Xc;
									p1[Y] -= Yc;
									p2[X] -= Xc;
									p2[Y] -= Yc;
									if (UP)
									{
										float k = (float)(m + increment)/(float)m;
										p1[X] *= k;
										p1[Y] *= k;
										p2[X] *= k;
										p2[Y] *= k;
									}
									else if( m > increment*2 ) 
									{
										float k = (float)(m - increment)/(float)m;
										p1[X] *= k;
										p1[Y] *= k;
										p2[X] *= k;
										p2[Y] *= k;
									}
									else 
									{
										float k = (float)increment*1.8/(float)m;
										p1[X] *= k;
										p1[Y] *= k;
										p2[X] *= k;
										p2[Y] *= k;
									}
									p1[X] += Xc;
									p1[Y] += Yc;
									p2[X] += Xc;
									p2[Y] += Yc;
									if (n->connect[i].vtx[ii].utility == 1)
									{
										m_Doc->m_nlist->MoveVertex( n, i, ii, p1[X], p1[Y]); 
										m_Doc->m_nlist->HighlightVertex( n, i, ii );
										n->connect[i].vtx[ii].utility = 0;
										flag_ok = TRUE;
									}
									if (n->connect[i].vtx[ii+1].utility == 1)
									{
										m_Doc->m_nlist->MoveVertex( n, i, ii+1, p2[X], p2[Y]);
										m_Doc->m_nlist->HighlightVertex( n, i, ii+1 );
										n->connect[i].vtx[ii+1].utility = 0;
										flag_ok = TRUE;
									}	
								}
			}
	if (flag_ok)
	{
		int minx = DEFAULT,miny = DEFAULT,
			maxx =-DEFAULT,maxy =-DEFAULT;
		for(cnet* n=m_Doc->m_nlist->GetFirstNet(); n; n=m_Doc->m_nlist->GetNextNet(/*LABEL*/))
			if( n->selected )
				for (int c=0; c<n->nconnects; c++)
					if( n->connect[c].m_selected )
						for(int s=0; s<n->connect[c].nsegs; s++)
							if( n->connect[c].seg[s].selected )
							{
								//min
								if (n->connect[c].vtx[s].x < minx)
									minx = n->connect[c].vtx[s].x;
								if (n->connect[c].vtx[s+1].x < minx)
									minx = n->connect[c].vtx[s+1].x;
								if (n->connect[c].vtx[s].y < miny)
									miny = n->connect[c].vtx[s].y;
								if (n->connect[c].vtx[s+1].y < miny)
									miny = n->connect[c].vtx[s+1].y;
								//max
								if (n->connect[c].vtx[s].x > maxx)
									maxx = n->connect[c].vtx[s].x;
								if (n->connect[c].vtx[s+1].x > maxx)
									maxx = n->connect[c].vtx[s+1].x;
								if (n->connect[c].vtx[s].y > maxy)
									maxy = n->connect[c].vtx[s].y;
								if (n->connect[c].vtx[s+1].y > maxy)
									maxy = n->connect[c].vtx[s+1].y;
							}
		m_Doc->m_nlist->AddHighlightLines(minx,miny,3);
		m_Doc->m_nlist->AddHighlightLines(maxx,maxy,3);
	}
	else
	{	
		static BOOL dsFlag = FALSE;
		CString str;
		str += "It is impossible to calculate the coordinates of the vertices.\n";
		str += "Manual correction with arrows is recommended.\n";
		CDlgMyMessageBox dlg;
		dlg.Initialize( str );
		if (!dsFlag) 
			dlg.DoModal();
		dsFlag = dlg.bDontShowBoxState;
		HighlightGroup();
	}
}


//===============================================================================================
//------------------------ Select Contour for Board, sm cutouts or  areas -----------------------
//===============================================================================================
void CFreePcbView::SelectContour()
{
	
	int S;
	if( m_sel_count == 0 )
		return;
	id sid;
	sid.Set( ID_NET, ID_AREA, 0, ID_SIDE, 0 );
	if( getbit(m_sel_flags, FLAG_SEL_NET) )
		for(cnet* n=m_Doc->m_nlist->GetFirstNet(); n; n=m_Doc->m_nlist->GetNextNet(/*LABEL*/))
			if( n->selected )
			{
				if( getbit(m_sel_flags, FLAG_SEL_AREA) )
					for (int i=0; i<n->nareas; i++)
						if( n->area[i].selected )
						{
							sid.i = i;
							int w = n->area[i].poly->GetW();
							for( int ii=n->area[i].poly->GetNumCorners()-1; ii>=0; ii-- )
							{
								if( n->area[i].poly->GetSideSel(ii) )
								{
									int num_c = n->area[i].poly->GetNumContour(ii);
									int max_c = num_c;
									if( num_c == 0 )
										max_c = n->area[i].poly->GetNumContours()-1;
									for( int h=n->area[i].poly->GetContourStart(num_c); 
											 h<=n->area[i].poly->GetContourEnd(max_c); h++ )
									{
										sid.ii = h;
										NewSelect( n, &sid, 0, 0);
										n->area[i].poly->HighlightSide(h,abs(w));
									}
									ii = n->area[i].poly->GetContourStart(num_c);
								}
									
							}
						}
			}
	if( getbit(m_sel_flags, FLAG_SEL_OP) )
	{
		for( int item=m_Doc->m_outline_poly.GetSize()-1; item>=0; item-- )
		{
			int gss = m_Doc->m_outline_poly[item].GetNumCorners()-1;
			int cl = m_Doc->m_outline_poly[item].GetClosed();
			if( cl == 0 )
				gss--;
			for(int ii=gss; ii>=0; ii--)
				if( m_Doc->m_outline_poly[item].GetSideSel( ii ) )
				{	
					sid = m_Doc->m_outline_poly[item].GetId();
					int num_c = m_Doc->m_outline_poly[item].GetNumContour(ii);
					int sc = m_Doc->m_outline_poly[item].GetContourStart(num_c);
					int ec = m_Doc->m_outline_poly[item].GetContourEnd(num_c);
					if( cl == 0 && ec >= gss )
						ec = gss;
					for(int sel=sc; sel<=ec; sel++ )
					{		
						sid.sst = ID_SIDE;
						sid.ii = sel;
						NewSelect( NULL, &sid, 0, 0);
						m_Doc->m_outline_poly[item].HighlightSide(sel,m_Doc->m_outline_poly[item].GetW());
					}
					ii = m_Doc->m_outline_poly[item].GetContourStart(num_c);
				}
		}
	}
	SetCursorMode(CUR_GROUP_SELECTED);
}
//===============================================================================================
//----------------------------------- SetUpReferences -------------------------------------------
//===============================================================================================
void CFreePcbView::SetUpReferences()
{
	if( m_sel_flags != PART_ONLY )
		return;
	if( m_sel_count == 0 )
		return;
	SaveUndoInfoForGroup( UNDO_GROUP_MODIFY, m_Doc->m_undo_list );
	static BOOL arFlag = FALSE;
	CString str;
	str = " Operation References positioning may take several minutes \n";
	str += " if you choose too small a step. Select the scanning step\n";
	str += " in placement grid box(recommended 100 mils or more).\n";
	str += " If the location is not found, the item will remain highlighted.";
	CDlgMyMessageBox dlg;
	dlg.Initialize( str );
	if (!arFlag ) 
	{
		dlg.DoModal();
		arFlag = dlg.bDontShowBoxState;
	}
	// mark all parts
	m_Doc->m_plist->MarkAllParts(0);
	
	// Get selection rect
	id sid;
	RECT idsRect;
	idsRect.left =	 INT_MAX;
	idsRect.right = INT_MIN;
	idsRect.bottom = INT_MAX;
	idsRect.top =	INT_MIN;
	int max_ref_length = INT_MIN;
	for(cpart* part=m_Doc->m_plist->GetFirstPart(); part; part=m_Doc->m_plist->GetNextPart(part))
		if ( part->selected )
		{
			if( !part->shape )
				continue;
			RECT pr;
			if ( m_Doc->m_plist->GetPartBoundingRect(part,&pr) )
			{
				idsRect.left =	min (idsRect.left,	pr.left);
				idsRect.right = max (idsRect.right,	pr.right);
				idsRect.bottom = min (idsRect.bottom,pr.bottom);
				idsRect.top =	max (idsRect.top,	pr.top);
			}
			if (m_Doc->m_plist->GetValueBoundingRect(part,&pr) )
			{
				idsRect.left =	min (idsRect.left,	pr.left);
				idsRect.right = max (idsRect.right,	pr.right);
				idsRect.bottom = min (idsRect.bottom,pr.bottom);
				idsRect.top =	max (idsRect.top,	pr.top);
			}
			if (m_Doc->m_plist->GetRefBoundingRect(part,&pr) )
			{
				max_ref_length = max (max_ref_length,	(pr.top-pr.bottom));
				max_ref_length = max (max_ref_length,	(pr.right-pr.left));
			}		
		}	
	SwellRect(&idsRect,max_ref_length); 
	// scanning..
	int DEG_90, INC_H, Old_X, Old_Y, Old_Ang;
	for(cpart* p=m_Doc->m_plist->GetFirstPart(); p; p=m_Doc->m_plist->GetNextPart(p))
		if ( p->selected )
		{
			if( !p->shape )
				continue;
			// set ref visible
			m_Doc->m_plist->ResizeRefText(p,p->m_ref_size,p->m_ref_w,TRUE);	
			// save old position
			Old_X = p->m_ref_xi;
			Old_Y = p->m_ref_yi;
			Old_Ang = p->m_ref_angle;
			//
			DEG_90 = 0;
			INC_H = 1;
			while(1)
			{
				if (m_Doc->m_plist->RefAutoLocation(p, m_Doc->m_part_grid_spacing, DEG_90, idsRect, &m_Doc->m_outline_poly, m_Doc->m_tlist, INC_H, p->m_ref_w*2 ) )
				{
					if (m_Doc->m_plist->RefAutoLocation(p, m_Doc->m_part_grid_spacing, DEG_90, idsRect, &m_Doc->m_outline_poly, m_Doc->m_tlist, INC_H, p->m_ref_w/2 ) )
					{
						if( DEG_90 )
						{
							if( INC_H == 0 )
							{
								p->m_ref_xi = Old_X;
								p->m_ref_yi = Old_Y;
								p->m_ref_angle = Old_Ang;
								m_Doc->m_plist->UndrawPart(p);
								m_Doc->m_plist->DrawPart(p);
								break;
							}
							INC_H = 0;
							DEG_90 = 0;
							continue;
						}
						DEG_90 = 1;
					}
					else
					{
						id pid( ID_PART_DEF );
						UnSelect( p, &pid );
						break;
					}
				}
				else
				{
					id pid( ID_PART_DEF );
					UnSelect( p, &pid );
					break;
				}
			}
			UpdateWindow();
			Invalidate( FALSE );//SetUpReferences
		}
	if( m_sel_count )
	{
		m_page = 2;
		SetFKText( m_cursor_mode );
		m_Doc->m_dlist->CancelHighLight();
		HighlightGroup();
	}
	else
		CancelSelection();
	m_Doc->ProjectModified( TRUE );
	return;
}
//===============================================================================================
//------------------------------------ MoveSegment ----------------------------------------------
//===============================================================================================
void CFreePcbView::MoveSegment (cnet * sel_net, int sel_ic, int sel_is, int dx, int dy, BOOL b_45 )
{
#define		sel_con				sel_net->connect[sel_ic]
#define		sel_last_vtx		sel_net->connect[sel_ic].vtx[sel_is-1]
#define		sel_vtx				sel_net->connect[sel_ic].vtx[sel_is]
#define		sel_next_vtx		sel_net->connect[sel_ic].vtx[sel_is+1]
#define		sel_next_next_vtx	sel_net->connect[sel_ic].vtx[sel_is+2]

	// 0. Move the line defined by the segment
	if (sel_is)
	{
		m_last_pt.x = sel_last_vtx.x;
		m_last_pt.y = sel_last_vtx.y;
	}
	else
	{
		m_last_pt.x = 0;
		m_last_pt.y = 0;
	}

	m_from_pt.x = sel_vtx.x;
	m_from_pt.y = sel_vtx.y;

	m_to_pt.x = sel_next_vtx.x;
	m_to_pt.y = sel_next_vtx.y;

	int nsegs = sel_con.nsegs;
	int use_third_segment = sel_is < nsegs - 1;
	if(use_third_segment)
	{
		m_next_pt.x = sel_next_next_vtx.x;	// Shouldn't really do this if we're off the edge?
		m_next_pt.y = sel_next_next_vtx.y;
	} else {
		m_next_pt.x = 0;
		m_next_pt.y = 0;
	}

	// 1. Move the endpoints of (xi, yi), (xf, yf) of the line by the mouse movement. This
	//		is just temporary, since the final ending position is determined by the intercept
	//		points with the leading and trailing segments:
	int new_from_x = m_from_pt.x + dx;			
	int new_from_y = m_from_pt.y + dy;

	int new_to_x = m_to_pt.x + dx;			
	int new_to_y = m_to_pt.y + dy;

	int old_x0_dir = sign(m_from_pt.x - m_last_pt.x);
	int old_y0_dir = sign(m_from_pt.y - m_last_pt.y);

	int old_x1_dir = sign(m_to_pt.x - m_from_pt.x);
	int old_y1_dir = sign(m_to_pt.y - m_from_pt.y);

	int old_x2_dir = sign(m_next_pt.x - m_to_pt.x);
	int old_y2_dir = sign(m_next_pt.y - m_to_pt.y);

	// 2. Find the intercept between the extended segment in motion and the leading segment.
	int i_nudge_from_x, i_nudge_from_y;
	if (sel_is)
	{
		double d_new_from_x;
		double d_new_from_y;
		FindLineIntersection(m_last_pt.x, m_last_pt.y, m_from_pt.x, m_from_pt.y,
								new_from_x,    new_from_y,	   new_to_x,    new_to_y,
								&d_new_from_x, &d_new_from_y);
		i_nudge_from_x = floor(d_new_from_x + .5);
		i_nudge_from_y = floor(d_new_from_y + .5);
	}
	else
	{
		i_nudge_from_x = new_from_x;
		i_nudge_from_y = new_from_y;
	}
	// 3. Find the intercept between the extended segment in motion and the trailing segment:
	int i_nudge_to_x, i_nudge_to_y;
	if(use_third_segment)
	{
		double d_new_to_x;
		double d_new_to_y;
		FindLineIntersection(new_from_x,    new_from_y,	   new_to_x,    new_to_y,
								m_to_pt.x,		m_to_pt.y,	m_next_pt.x, m_next_pt.y,
								&d_new_to_x, &d_new_to_y);
		i_nudge_to_x = floor(d_new_to_x + .5);
		i_nudge_to_y = floor(d_new_to_y + .5);
	} 
	else 
	{
		i_nudge_to_x = new_to_x;
		i_nudge_to_y = new_to_y;
	}
	
	// If we drag too far, the line segment can reverse itself causing a little triangle to form.
	//   That's a bad thing.
	if(    (sign(i_nudge_to_x - i_nudge_from_x) == old_x1_dir && sign(i_nudge_to_y - i_nudge_from_y) == old_y1_dir) 
		&& (sign(i_nudge_from_x - m_last_pt.x) == old_x0_dir &&  sign(i_nudge_from_y - m_last_pt.y) == old_y0_dir) 
		&& ((sign(m_next_pt.x - i_nudge_to_x) == old_x2_dir && sign(m_next_pt.y - i_nudge_to_y) == old_y2_dir) || !use_third_segment))
	{
	//	Move both vetices to the new position:
		int d = Distance( sel_vtx.x, sel_vtx.y, i_nudge_from_x, i_nudge_from_y );
		BOOL EN = ( abs( d-abs(dx)*sqrt(2.0) )<_2540/10 || 
					abs( d-abs(dy)*sqrt(2.0) )<_2540/10 ||
					abs( d-abs(dx) )<10 ||
					abs( d-abs(dy) )<10 );
		int d2 = Distance( sel_next_vtx.x, sel_next_vtx.y, i_nudge_to_x, i_nudge_to_y );
		BOOL EN2 = (	abs( d-abs(dx)*sqrt(2.0) )<_2540/10 || 
				abs( d-abs(dy)*sqrt(2.0) )<_2540/10 ||
				abs( d-abs(dx) )<10 ||
				abs( d-abs(dy) )<10 );
		if( !b_45 || EN ) 
			if( InRange( i_nudge_from_x, INT_MIN/4, INT_MAX/4 ))
				if( InRange( i_nudge_from_y, INT_MIN/4, INT_MAX/4 ))
					m_Doc->m_nlist->MoveVertex( sel_net, sel_ic, sel_is, i_nudge_from_x, i_nudge_from_y );
		if( !b_45 || EN2 ) 
			if( InRange( i_nudge_to_x, INT_MIN/4, INT_MAX/4 ))
				if( InRange( i_nudge_to_y, INT_MIN/4, INT_MAX/4 ))
					m_Doc->m_nlist->MoveVertex( sel_net, sel_ic, sel_is+1, i_nudge_to_x, i_nudge_to_y );
	} 
	else 
		return;
	gTotalArrowMoveX += dx;
	gTotalArrowMoveY += dy;
}


//===============================================================================================
//---------------------------------------- MergeGroup -------------------------------------------
//===============================================================================================
void CFreePcbView::MergeGroup( int merge0 )
{
	// parts
	if( getbit(m_sel_flags, FLAG_SEL_PART) )
		for(cpart* p=m_Doc->m_plist->GetFirstPart(); p; p=m_Doc->m_plist->GetNextPart(p))
			if( p->selected )
				p->m_merge = merge0;
	
	// nets
	m_Doc->m_nlist->OptimizeConnections(0,0,0);
	for( cnet * n = m_Doc->m_nlist->GetFirstNet(); n; n = m_Doc->m_nlist->GetNextNet(/*LABEL*/) )
	{
		for( int ia=0; ia<n->nareas; ia++ )
			if( n->area[ia].selected )
				n->area[ia].poly->SetMerge(merge0);
	}
	m_Doc->m_nlist->SetAreaConnections();

	// texts
	if( getbit(m_sel_flags, FLAG_SEL_TEXT) )
	{
		int it = 0;
		for(CText* t=m_Doc->m_tlist->GetFirstText(); t; t=m_Doc->m_tlist->GetNextText(&it))
			if ( t->m_selected )
				t->m_merge = merge0;
	}
	// outline
	if( getbit(m_sel_flags, FLAG_SEL_OP) )
	{
		for( int item=m_Doc->m_outline_poly.GetSize()-1; item>=0; item-- )
		{
			int gns = m_Doc->m_outline_poly[item].GetNumCorners()-1;
			int cl = m_Doc->m_outline_poly[item].GetClosed();
			for(int ic=gns; ic>=0; ic--)
			{
				if( m_Doc->m_outline_poly[item].GetSel(ic) )
				{
					m_Doc->m_outline_poly[item].SetMerge(merge0);
					break;
				}
				if( cl == 0 && ic == gns )
					continue;
				if( m_Doc->m_outline_poly[item].GetSideSel(ic) )
				{
					m_Doc->m_outline_poly[item].SetMerge(merge0);
					break;
				}
			}
		}
	}
	m_Doc->ProjectModified( TRUE );
}

//==================================================================================
void CFreePcbView::MergeGroup()
{
	m_Doc->RemoveOrphanMerges();
	if( !m_sel_count )
		return;
	CString sel_merge_name = m_sel_merge_name;
	int merge0 = -1;
	if( m_sel_merge_name.GetLength() )
	{
		int ipos = m_sel_merge_name.Find("(");
		if( ipos > 0 )
			sel_merge_name = m_sel_merge_name.Left(ipos);
		sel_merge_name.Trim();
		merge0 = m_Doc->m_mlist->GetIndex(sel_merge_name);

	}
	else
	{
		if ( m_sel_id.type == ID_PART && m_sel_id.st == ID_SEL_RECT )
			if( m_sel_part )
				merge0 = m_sel_part->m_merge;					
		if ( m_sel_id.type == ID_NET )
		{
			if( m_sel_net )
			{
				if ( m_sel_id.st == ID_CONNECT )
					if ( m_sel_id.i < m_sel_net->nconnects )
						merge0 = m_sel_net->connect[m_sel_id.i].m_merge;
				if ( m_sel_id.st == ID_AREA )
					if ( m_sel_id.i < m_sel_net->nareas )
						merge0 = m_sel_net->area[m_sel_id.i].poly->GetMerge();
			}
		}
	}
	if ( m_sel_id.type == ID_POLYLINE )
		merge0 = m_Doc->m_outline_poly[m_sel_id.i].GetMerge();
	int clrnc = m_Doc->m_fill_clearance;
	if( merge0 >= 0 )
		clrnc = m_Doc->m_mlist->GetClearance( merge0 );
	CDlgAddMerge dlg;
	dlg.Doc = m_Doc;
	dlg.m_cl = clrnc;
	int ret;
	if ( merge0 == -1 )
	{	
		if( m_sel_merge_name.GetLength() )
		{
			AfxMessageBox("Some parts already have a Merge name.");
			int ipos = m_sel_merge_name.Find("(");
			if( ipos > 0 )
				dlg.m_merge_name = m_sel_merge_name.Left(ipos);
			else
				dlg.m_merge_name = m_sel_merge_name;
			dlg.m_merge_name.Trim();
		}
		else
			dlg.m_merge_name = "Empty"; 
		ret = dlg.DoModal();
	}
	else
	{
		dlg.m_merge_name = m_Doc->m_mlist->GetMerge(merge0); 
		AfxMessageBox("Some parts already have a Merge name.");
		ret = dlg.DoModal();
	}
	if (ret == IDOK)
	{
		if( dlg.m_merge_name.Left(4) == "None" )
		{
			ExplodeGroup();
			return;
		}
		merge0 = m_Doc->m_mlist->AddNew(dlg.m_merge_name, dlg.m_cl);
		MergeGroup(merge0);
	}
	CancelSelection();
	NewSelectM(NULL,merge0);
	m_sel_merge_name = dlg.m_merge_name;
	CString str;
	int clrn = dlg.m_cl;
	if( clrn == 0 )
		clrn = m_Doc->m_fill_clearance;
	::MakeCStringFromDimension( &str, clrn, m_Doc->m_units, TRUE, FALSE, FALSE, 2 );
	m_sel_merge_name += " (copper area fill clearance = " + str + ") ";
	SetCursorMode( CUR_GROUP_SELECTED );
	HighlightGroup();
}
//===============================================================================================
//-------------------------------------- ExplodeGroup -------------------------------------------
//===============================================================================================
void CFreePcbView::ExplodeGroup()
{
	if( AfxMessageBox(" Delete this merger?", MB_YESNO ) == IDYES )
	{
		MergeGroup(-1);
		m_Doc->RemoveOrphanMerges();
		m_sel_merge_name = "";
		SetFKText( m_cursor_mode );
	}
}


//===============================================================================================
//-------------------------------------- SelectMerge --------------------------------------------
//===============================================================================================
CString CFreePcbView::NewSelectM( cpart * ptr, int number )
{
	BOOL En_UnSel=0;
	int num = number;
	id ptr_id( ID_PART_DEF );
	if( ptr )
	{
		num = ptr->m_merge;
		if( ptr->selected )
			En_UnSel = 1;
		UnSelect( ptr, &ptr_id );
	}
	if ( num >= m_Doc->m_mlist->GetSize() || num < 0 )
		return "";
	if( m_Doc->m_mlist->GetMark( num ) && En_UnSel )
		m_Doc->m_mlist->mark1( num, 0 );
	else
		m_Doc->m_mlist->mark1( num, 1 );
	id sid;
	for(cnet* n=m_Doc->m_nlist->GetFirstNet(); n; n=m_Doc->m_nlist->GetNextNet(/*LABEL*/))
	{
		for (int i=0; i<n->nconnects; i++)
		{
			if( n->connect[i].end_pin == cconnect::NO_END && n->connect[i].seg[0].layer == LAY_RAT_LINE )
				continue;
			// SEL CONNECT
			id vtx ( ID_NET, ID_CONNECT, i, ID_VERTEX, 0 );
			sid.Set( ID_NET, ID_CONNECT, i, ID_SEG, 0 );
			if (n->connect[i].m_merge == num )
			{
				for(int ic=0; ic<n->connect[i].nsegs; ic++)
				{
					sid.ii = ic;
					if( En_UnSel )
						UnSelect(  n, &sid );
					else
						NewSelect( n, &sid, 0, 0 );
					if( n->connect[i].vtx[ic+1].via_w )
					{
						vtx.ii = ic+1;
						if( En_UnSel )
							UnSelect(  n, &vtx );
						else
							NewSelect( n, &vtx, 0, 0 );
					}
				}
			}
			else if( n->connect[i].end_pin >= 0 )
			{
				if( n->pin[n->connect[i].start_pin].part->m_merge == num && n->pin[n->connect[i].end_pin].part->selected ) 
				{
					for(int ic=0; ic<n->connect[i].nsegs; ic++)
					{
						sid.ii = ic;
						if( En_UnSel )
							UnSelect( n, &sid );
						else
							NewSelect( n, &sid, 0, 0 );
						if( n->connect[i].vtx[ic+1].via_w )
							break;
					}
				}
				else if( n->pin[n->connect[i].end_pin].part->m_merge == num && n->pin[n->connect[i].start_pin].part->selected )
				{
					for(int ic=n->connect[i].nsegs-1; ic>=0; ic--)
					{
						sid.ii = ic;
						if( En_UnSel )
							UnSelect( n, &sid );
						else
							NewSelect( n, &sid, 0, 0 );
						if( n->connect[i].vtx[ic+1].via_w )
							break;
					}
				}
			}
		}
		for (int i=0; i<n->nareas; i++)
		{
			// SEL AREA
			sid.Set( ID_NET, ID_AREA, i, ID_SIDE, 0 );
			if ( n->area[i].poly->GetMerge() == num )
			{
				for( int ic=n->area[i].poly->GetNumCorners()-1; ic>=0; ic-- )
				{
					sid.ii = ic;
					if( En_UnSel )
						UnSelect( n, &sid );
					else
						NewSelect( n, &sid, 0, 0 );
				}
			}
		}
	}
	// SEL TEXTS
	sid.Set(ID_TEXT_DEF);
	int it = 0;
	for(CText* t=m_Doc->m_tlist->GetFirstText(); t; t=m_Doc->m_tlist->GetNextText(&it))
		if ( t->m_merge == num )
			if( En_UnSel )
				UnSelect( t, &sid );
			else
				NewSelect( t, &sid, 0, 0 );
	// SEL OUTLINE POLY
	for( int item=m_Doc->m_outline_poly.GetSize()-1; item>=0; item-- )
	{
		if ( m_Doc->m_outline_poly[item].GetMerge() == num )
		{
			sid = m_Doc->m_outline_poly[item].GetId();
			sid.sst = ID_SIDE;
			int gss = m_Doc->m_outline_poly[item].GetNumCorners()-1;
			int cl = m_Doc->m_outline_poly[item].GetClosed();
			if( cl == 0 )
				gss--;
			for( int ic=gss; ic>=0; ic-- )
			{
				sid.ii = ic;
				if( En_UnSel )
					UnSelect( NULL, &sid );
				else
					NewSelect( NULL, &sid, 0, 0 );
			}
		}
	}
	// SEL PARTS
	sid.Set( ID_PART_DEF );
	for(cpart* p=m_Doc->m_plist->GetFirstPart(); p; p=m_Doc->m_plist->GetNextPart(p))
	{
		if( p == ptr )
			continue;
		if( p->m_merge == num )
			if( En_UnSel )
				UnSelect( p, &sid );
			else
				NewSelect( p, &sid, 0, 0 );
	}
	if( ptr )
		if( En_UnSel == 0 || m_sel_count == 0 )
		NewSelect( ptr, &ptr_id, m_sel_count?0:1, 0 );
	// END
	if( m_sel_count > 1 )
	{
		int clrn = m_Doc->m_mlist->GetClearance(num);
		if( clrn == 0 )
			clrn = m_Doc->m_fill_clearance;
		CString str="";
		::MakeCStringFromDimension( &str, clrn, m_Doc->m_units, TRUE, FALSE, FALSE, 2 );
		str = " (copper area fill clearance = " + str + ") ";	
		m_sel_merge_name = m_Doc->m_mlist->GetMerge(num) + str;
		m_Doc->m_dlist->CancelHighLight();
		HighlightGroup();
		SetCursorMode(CUR_GROUP_SELECTED);
			return m_sel_merge_name;
	}
	return "";
}


//===============================================================================================
//------------------------------------------ Info Box -------------------------------------------
//===============================================================================================
void CFreePcbView::OnInfoBoxMess( int command, int n_str, CArray<CString> *str )
{
	static int mem_sel_count = 0;
	static BOOL IB_TAB_INDEX = FALSE;
	if( n_str == 0 || str == NULL )
	{
		// reset mem_sel_count
		mem_sel_count = 0;
		return;
	}
	enum{C_JUMP=0, C_PARTS, C_PINS, C_MOVE, C_PASTE, C_ENQUARY, C_INVALIDATE};
	cpart * sel_p = 0;
	id sid( ID_PART_DEF );
	int pMerge = -1;
	int LOGLEN = 0;
	n_str = min( n_str, str->GetSize() );	

	// jump to point of selected part
	if ( command == C_JUMP )
	{
		mem_sel_count = 0;
		CString ref = str->GetAt(0);
		sel_p = m_Doc->m_plist->GetPart(ref);
		if( sel_p )
		{
			OnRefShowPart(sel_p, FALSE);
			if ( n_str == 1 )
			{
				if( !sel_p->selected )
					if( CurSelected() || CurNone() )
						NewSelect( sel_p, &sid, 0,0 );
			}
			else if ( n_str >= 4 )
			{
				CString pin = "";
				pin =  str->GetAt(1);
				CString s = "";
				s =  str->GetAt(2);
				double xx = my_atof(&s);
				s =  str->GetAt(3);
				double yy = my_atof(&s);
				if( m_Doc->m_units == MIL )
				{
					xx *= NM_PER_MIL;
					yy *= NM_PER_MIL;
				}
				else if( m_Doc->m_units == MM )
				{
					xx *= NM_PER_MM;
					yy *= NM_PER_MM;
				}
				int i = m_Doc->m_nlist->GetPinIndexByNameForPart( sel_p, pin, xx, yy );
				if(!m_Doc->m_plist->SelectPad(sel_p,i,1,m_active_layer))
					m_Doc->m_plist->SelectPad(sel_p,i,1,0);
				IB_TAB_INDEX = TRUE;
			}
		}
	}
	// highlight infobox partlist
	else if ( command == C_PARTS )
	{
		CString ref="";
		for (int ns=0; ns<n_str; ns++)
		{
			ref = str->GetAt(ns);
			sel_p = m_Doc->m_plist->GetPart(ref);
			if (sel_p)
			{	
				// return if no footprint
				if (!sel_p->shape)
					continue;
				// continue
				if( !sel_p->selected )
					if( CurSelected() || CurNone() )
						NewSelect( sel_p, &sid, 0,0 );
			}
		    else if( m_Doc->m_dlg_log )
			{
				if ( LOGLEN == 0 )
				{
					m_Doc->m_dlg_log->ShowWindow( SW_SHOW );
					m_Doc->m_dlg_log->BringWindowToTop();
					CString mess = "Warning! Some objects not found:   ";
					LOGLEN += mess.GetLength();
					m_Doc->m_dlg_log->AddLine( "\r\n" );
					m_Doc->m_dlg_log->AddLine(mess);
					m_Doc->m_dlg_log->AddLine( " " );
				}
				m_Doc->m_dlg_log->AddLine(ref);
				m_Doc->m_dlg_log->AddLine( " " );
				LOGLEN += ref.GetLength();
				if( LOGLEN > 80 )
				{
					m_Doc->m_dlg_log->AddLine( "\r\n" );
					LOGLEN = 0;
				}	
			}
		}
	}
	// highlight infobox pin-list
	else if( command == C_PINS )
	{
		mem_sel_count = 0;
		CString ref="";
		for (int ns=0; ns<n_str; ns++)
		{
			if( ref.Compare(str->GetAt(ns)) == 0 && ns )
			{
				int test = 0;
			}
			else
			{
				ref = str->GetAt(ns);
				sel_p = m_Doc->m_plist->GetPart(ref);
			}
			if( sel_p && ns%4 == 0 )
			{			
				IB_TAB_INDEX = TRUE;
				CString pin = "";
				ns++;
				pin =  str->GetAt(ns);
				ns++;
				CString s = "";
				s =  str->GetAt(ns);
				double xx = my_atof(&s);
				ns++;
				// return if no footprint
				if (!sel_p->shape)
					continue;
				// continue
				s =  str->GetAt(ns);
				double yy = my_atof(&s);
				if( m_Doc->m_units == MIL )
				{
					xx *= NM_PER_MIL;
					yy *= NM_PER_MIL;
				}
				else if( m_Doc->m_units == MM )
				{
					xx *= NM_PER_MM;
					yy *= NM_PER_MM;
				}
				int i = m_Doc->m_nlist->GetPinIndexByNameForPart( sel_p, pin, xx, yy );
				if(!m_Doc->m_plist->SelectPad(sel_p,i,1,m_active_layer))
					m_Doc->m_plist->SelectPad(sel_p,i,1,0);
			}
			// hilite via
			else if( ref.Left(9) == "*DL_CIRC#" && ns%4 == 0 )
			{
				CString s;
				ns++;
				s =  str->GetAt(ns);
				double xx = my_atof(&s);
				ns++;
				s =  str->GetAt(ns);
				double yy = my_atof(&s);
				ns++;
				s =  str->GetAt(ns);
				double W = my_atof(&s);
				if( m_Doc->m_units == MIL )
				{
					xx *= NM_PER_MIL;
					yy *= NM_PER_MIL;
					W  *= NM_PER_MIL;
				}
				else if( m_Doc->m_units == MM )
				{
					xx *= NM_PER_MM;
					yy *= NM_PER_MM;
					W  *= NM_PER_MM;
				}
				if( InRange( xx, -DEFAULT/2, DEFAULT/2 ) )
					if( InRange( yy, -DEFAULT/2, DEFAULT/2 ) )
						if( InRange( W, -DEFAULT/20, DEFAULT/20 ) )
						{
							id Id(0,0,0,0,0);
							RECT r = rect( xx,yy,xx,yy );
							SwellRect( &r, W/2 );
							dl_element * el = m_Doc->m_dlist->Add( Id, NULL, 0, DL_CIRC, 1, &r, 0, NULL, 0 );
							m_Doc->m_dlist->HighLight( el );
							setbit(el->map_orig_layer,LAY_PAD_THRU);
							IB_TAB_INDEX = TRUE;
						}
			}
			else if( m_Doc->m_dlg_log )
			{
				if( ns%4 < 2 )
				{
				 	if ( LOGLEN == 0 )
					{
						m_Doc->m_dlg_log->ShowWindow( SW_SHOW );
						m_Doc->m_dlg_log->BringWindowToTop();
						CString mess = "Warning! Some objects not found:   ";
						LOGLEN += mess.GetLength();
						m_Doc->m_dlg_log->AddLine( "\r\n" );
						m_Doc->m_dlg_log->AddLine(mess);
						if( m_sel_count == 0 )
							m_sel_count = 1; // when m_sel_count == 0 only
					}
					m_Doc->m_dlg_log->AddLine(ref);
					if( ns%2 == 0 )
						m_Doc->m_dlg_log->AddLine( "." );
					else
						m_Doc->m_dlg_log->AddLine( " " );
					LOGLEN += ref.GetLength();
					if( LOGLEN > 80 && ns%2 )
					{
						m_Doc->m_dlg_log->AddLine( "\r\n" );
						LOGLEN = 0;
					}
				}
			}
		}
	}
	// F4 from infobox
	else if( command == C_MOVE )
	{
		mem_sel_count = 0;
		CString ref = str->GetAt(0);
		sel_p = m_Doc->m_plist->GetPart(ref);
		if (sel_p)
			if (sel_p->shape)
			{
				if( CurSelected() || CurNone() )
				{
					CancelSelection();
					m_sel_part = sel_p;
					m_sel_id.Set( ID_PART_DEF );
					OnPartMove();
					SetCursorPos( m_client_origin.x+(m_client_r.right-m_client_r.left)/2, m_client_origin.y+(m_client_r.bottom-m_client_r.top)/2 );
				}
				return;
			}
	}
	// paste copper pour from areas.exe
	else if ( command == C_PASTE )
	{
		CString file;
		if (n_str >= 1)
		{
			file = str->GetAt(0);
			m_Doc->PasteFromFile( file, TRUE );
			m_Doc->m_nlist->SetAreaConnections();
		}
	}
	// message to infobox
	else if( command == C_ENQUARY )
	{
		OnInfoBoxSendMess(m_Doc->m_pcb_full_path);
		return;
	}
	// Invalidate(FALSE)
	else if( command == C_INVALIDATE )
	{
		if( !IB_TAB_INDEX )
		{
			m_dlist->CancelHighLight();
			if( m_sel_count != mem_sel_count )
			{
				HighlightGroup(TRUE);
				mem_sel_count = m_sel_count;
			}
			else
			{
				HighlightGroup(FALSE);
				mem_sel_count = 0;
			}
		}
		IB_TAB_INDEX = FALSE;
		m_draw_layer = LAY_HILITE;//OnInfoBoxMess
		Invalidate( FALSE );//OnInfoBoxMess
	}

	// set_cursor_mode = CUR_GROUP_SELECTED
	if( CurSelected() || CurNone() )
	{
		setbit(m_sel_flags,FLAG_SEL_PART);
		if( m_cursor_mode != CUR_GROUP_SELECTED )
			SetCursorMode( CUR_GROUP_SELECTED );
		else
			ShowSelectStatus();
	}
}


CWnd * CFreePcbView::OnInfoBoxSendMess( CString mess )
{
	COPYDATASTRUCT cd;
	int m_len = mess.GetLength();
    cd.cbData = m_len + 1;
	char * data;
	data = new char[m_len+1];//ok
    for (int cp=m_len; cp>0; cp--)
		data[cp-1] = mess[cp-1];
	data[m_len] = '\0';
	cd.lpData = data;
	cd.cbData = m_len+1;
	CWnd * Find = FindWindow(NULL, "Infobox");
	int RES = 0;
	if( Find )
	{
		HWND InfoBox; 
		InfoBox = Find->GetSafeHwnd();
		if( InfoBox )
			RES = ::SendMessage(InfoBox, WM_COPYDATA, 0, (LPARAM)&cd);
	}
	delete data;
	if( RES )
		return Find;
	else
		return NULL;
}

// new selected element
BOOL CFreePcbView::GetSelectedItem()
{
	// 1: nets
	if( getbit(m_sel_flags, FLAG_SEL_NET) )
	{		
		for(cnet* n=m_Doc->m_nlist->GetFirstNet(); n; n=m_Doc->m_nlist->GetNextNet(/*LABEL*/))
		{
			if( n->selected )
			{
				m_sel_net = n;
				if( getbit(m_sel_flags, FLAG_SEL_CONNECT) )
				{
					for (int icon=0; icon<n->nconnects; icon++)
						if( n->connect[icon].m_selected )
							for(int iseg=n->connect[icon].nsegs; iseg>0; iseg--)
							{
								cseg * s = &n->connect[icon].seg[iseg-1];
								cvertex * v = &n->connect[icon].vtx[iseg];
								if( s->selected )
								{
									m_sel_id.Set( ID_NET, ID_CONNECT, icon, ID_SEG, iseg-1 );
									m_Doc->m_nlist->CancelNextNet();
									return true;
								}
								if( v->selected )
								{
									m_sel_id.Set( ID_NET, ID_CONNECT, icon, ID_VERTEX, iseg );
									m_Doc->m_nlist->CancelNextNet();
									return true;
								}
							}
				}
				if( getbit(m_sel_flags, FLAG_SEL_AREA) )
				{
					for (int ia=0; ia<n->nareas; ia++)
						if( n->area[ia].selected )
							for ( int cor=n->area[ia].poly->GetNumCorners()-1; cor>=0; cor-- )
							{
								if( n->area[ia].poly->GetSel(cor) )
								{
									m_sel_id.Set( ID_NET, ID_AREA, ia, ID_CORNER, cor );
									m_Doc->m_nlist->CancelNextNet();
									return true;
								}
								if( n->area[ia].poly->GetSideSel(cor) )
								{
									m_sel_id.Set( ID_NET, ID_AREA, ia, ID_SIDE, cor );
									m_Doc->m_nlist->CancelNextNet();
									return true;
								}
							}
				}
			}
		}
	}
	// 2: outlines
	if( getbit(m_sel_flags, FLAG_SEL_OP) )
	{
		for( int item=m_Doc->m_outline_poly.GetSize()-1; item>=0; item-- )
		{
			int gns = m_Doc->m_outline_poly[item].GetNumCorners()-1;
			int cl = m_Doc->m_outline_poly[item].GetClosed();
			for(int ic=gns; ic>=0; ic--)
			{
				if( m_Doc->m_outline_poly[item].GetSel(ic) )
				{
					m_sel_id = m_Doc->m_outline_poly[item].GetId();
					m_sel_id.i = item;
					m_sel_id.sst = ID_CORNER;
					m_sel_id.ii = ic;
					return true;
				}
				if( cl == 0 && ic == gns )
					continue;
				if( m_Doc->m_outline_poly[item].GetSideSel(ic) )
				{
					m_sel_id = m_Doc->m_outline_poly[item].GetId();
					m_sel_id.i = item;
					m_sel_id.sst = ID_SIDE;
					m_sel_id.ii = ic;
					return true;
				}
			}
		}
	}
	// 3: texts
	if( getbit(m_sel_flags, FLAG_SEL_TEXT) )
	{
		m_sel_id.Set(ID_TEXT_DEF);
		int it = 0;
		for(CText* t=m_Doc->m_tlist->GetFirstText(); t; t=m_Doc->m_tlist->GetNextText(&it))
			if ( t->m_selected )
			{
				m_sel_text = t;
				return true;
			}
	}
	// 4: parts
	if( getbit(m_sel_flags, FLAG_SEL_PART) )
	{
		m_sel_id.Set( ID_PART_DEF );
		for(cpart* p=m_Doc->m_plist->GetFirstPart(); p; p=m_Doc->m_plist->GetNextPart(p))
			if( p->selected )
			{
				m_sel_part = p;
				return true;
			}
	}
	m_sel_part = NULL;
	m_sel_net = NULL;
	m_sel_text = NULL;
	m_sel_count = 0;
	m_sel_id.Set(0,0,0,0,0);
	return 0;
}


int  CFreePcbView::UnSelect( void * ptr, id * sid, BOOL bSET_CURSOR_MODE )
{
	// Unselect part
	int m = -1;
	if ( sid->type == ID_PART && sid->st == ID_SEL_RECT )
	{
		cpart * p = (cpart*)ptr;
		if(p)
		{
			if(p->shape)
			{
				if( p->selected )
				{
					p->selected = 0;
					m_sel_count--;
				}
			}
		}	
	}
	// Unselect net
	else if ( sid->type == ID_NET )
	{
		cnet* n = (cnet*)ptr;
		if(n)
		{
			if ( sid->st == ID_CONNECT )
			{	
				if ( sid->sst == ID_SEG )
				{
					if( n->connect[sid->i].seg[sid->ii].selected )
					{
						n->connect[sid->i].seg[sid->ii].selected = 0;
						m_sel_count--;					
					}
				}
				else if ( sid->sst == ID_VERTEX )
				{		
					if( n->connect[sid->i].vtx[sid->ii].selected )
					{
						n->connect[sid->i].vtx[sid->ii].selected = 0;
						m_sel_count--;
					}	
				}
			}
			else if ( sid->st == ID_AREA )
			{
				if ( sid->sst == ID_SIDE )
				{
					BOOL sel = n->area[sid->i].poly->GetSideSel( sid->ii );			
					if( sel )
					{
						n->area[sid->i].poly->SetSideSel( sid->ii, 0 );
						m_sel_count--;
					}
				}
				else if ( sid->sst == ID_CORNER )
				{
					BOOL sel = n->area[sid->i].poly->GetSel( sid->ii );
					if( sel )
					{
						n->area[sid->i].poly->SetSel( sid->ii, 0 );
						m_sel_count--;
					}
				}
			}
		}
	}
	// Unselect text
	else if ( sid->type == ID_TEXT )
	{
		CText * t = (CText*)ptr;
		if(t)
		{
			if( t->m_selected )
			{
				t->m_selected = 0;
				m_sel_count--;
			}
		}
	} 
	// Unselect outline
	else if ( sid->type == ID_POLYLINE )
	{
		if (sid->sst == ID_SIDE)
		{
			BOOL sel = m_Doc->m_outline_poly[sid->i].GetSideSel( sid->ii );
			if( sel )
			{
				m_Doc->m_outline_poly[sid->i].SetSideSel( sid->ii, 0 );
				m_sel_count--;
			}
		}
		else if (sid->sst == ID_CORNER)		
		{
			BOOL sel = m_Doc->m_outline_poly[sid->i].GetSel( sid->ii );
			if( sel )
			{
				m_Doc->m_outline_poly[sid->i].SetSel( sid->ii, 0 );
				m_sel_count--;
			}
		}
	}
	if( sid->type == m_sel_id.type && 
		sid->st == m_sel_id.st &&
		sid->sst == m_sel_id.sst &&
		sid->i == m_sel_id.i &&
		sid->ii == m_sel_id.ii )
		m_sel_id.Clear();
	if( m_sel_id.type == ID_NONE )
	{
		// select prev
		if( GetSelectedItem() == 0 )
			CancelSelection();
	}
	if( bSET_CURSOR_MODE && m_sel_count )
	{
		if( m_sel_id.type == ID_NET )
			NewSelect( m_sel_net, &m_sel_id, TRUE, FALSE );
		else if( m_sel_id.type == ID_PART )
			NewSelect( m_sel_part, &m_sel_id, TRUE, FALSE );
		else if( m_sel_id.type == ID_TEXT )
			NewSelect( m_sel_text, &m_sel_id, TRUE, FALSE );
		else
			NewSelect( NULL, &m_sel_id, TRUE, FALSE );
	}
	return 0;
}

// new selected element
int CFreePcbView::NewSelect( void * ptr, id * sid, BOOL bSet_cursor_mode, BOOL bInvert )
{	
	// select part
	int m = -1;
	if ( sid->type == ID_PART && sid->st == ID_SEL_RECT )
	{
		setbit( m_sel_flags, FLAG_SEL_PART );
		cpart * p = (cpart*)ptr;
		if(p)
		{
			if(p->shape)
			{
				m = p->m_merge;
				if( p->selected == 0 )
				{
					p->selected = 1;
					m_sel_count++;
				}
				else if( bInvert )
				{
					UnSelect( p, sid, bSet_cursor_mode );
					return m;
				}
				m_sel_part = p;
				m_sel_id = *sid;
				if( bSet_cursor_mode && m_sel_count == 1 )
				{
					m_Doc->m_dlist->CancelHighLight();
					SelectPart( m_sel_part );
					m_Doc->m_plist->SelectRefText( m_sel_part );
					m_Doc->m_plist->SelectValueText( m_sel_part );
					SetCursorMode( CUR_PART_SELECTED );
					OnInfoBoxSendMess( "part_list " + m_sel_part->ref_des + " " );
				}
			}
		}	
	}
	// select net
	else if ( sid->type == ID_NET )
	{
		setbit( m_sel_flags, FLAG_SEL_NET );
		cnet* n = (cnet*)ptr;
		if(n)
		{
			n->selected = TRUE;
			if ( sid->st == ID_CONNECT && sid->i < n->nconnects )
			{				
				if ( sid->sst == ID_SEG && sid->ii < n->connect[sid->i].nsegs )
				{					
					if( n->connect[sid->i].seg[sid->ii].layer != LAY_RAT_LINE )
						m = n->connect[sid->i].m_merge;
					if( n->connect[sid->i].seg[sid->ii].selected == 0 )
					{
						setbit( m_sel_flags, FLAG_SEL_CONNECT );
						clrbit( m_sel_flags, FLAG_SEL_VTX );
						n->connect[sid->i].m_selected = TRUE;
						n->connect[sid->i].seg[sid->ii].selected = 1;
						m_sel_count++;
					}
					else if( bInvert )
					{
						UnSelect( n, sid, bSet_cursor_mode );
						return m;
					}
					m_sel_net = n;
					m_sel_id = *sid;
					if( bSet_cursor_mode && m_sel_count == 1 )
					{	
						m_Doc->m_dlist->CancelHighLight();
						int seg_cl = 0;
						if( m_sel_seg.layer == LAY_RAT_LINE )
							seg_cl = m_seg_clearance;
						m_Doc->m_nlist->HighlightSegment( n, sid->i, sid->ii, seg_cl );
						if( n->connect[sid->i].seg[sid->ii].layer == LAY_RAT_LINE )
							SetCursorMode( CUR_RAT_SELECTED );
						else
							SetCursorMode( CUR_SEG_SELECTED );		
					}
				}
				else if ( sid->sst == ID_VERTEX && sid->ii <= n->connect[sid->i].nsegs )
				{						
					m = n->connect[sid->i].m_merge;
					if( n->connect[sid->i].vtx[sid->ii].selected == 0 )
					{
						setbit( m_sel_flags, FLAG_SEL_CONNECT );
						setbit( m_sel_flags, FLAG_SEL_VTX );
						n->connect[sid->i].m_selected = TRUE;
						n->connect[sid->i].vtx[sid->ii].selected = 1;
						m_sel_count++;
					}	
					else if( bInvert )
					{
						UnSelect( n, sid, bSet_cursor_mode );
						return m;
					}
					m_sel_net = n;
					m_sel_id = *sid;
					if( bSet_cursor_mode && m_sel_count == 1 )
					{
						m_Doc->m_dlist->CancelHighLight();
						m_Doc->m_nlist->HighlightVertex( n, sid->i, sid->ii );
						if( sid->ii < m_sel_net->connect[sid->i].nsegs )
							SetCursorMode( CUR_VTX_SELECTED );
						else
							SetCursorMode( CUR_END_VTX_SELECTED );
					}
				}
			}
			else if ( sid->st == ID_AREA && sid->i < n->nareas )
			{
				if( sid->ii < n->area[sid->i].poly->GetNumCorners() )
				{
					m = n->area[sid->i].poly->GetMerge();
					if ( sid->sst == ID_SIDE )
					{
						BOOL sel = n->area[sid->i].poly->GetSideSel( sid->ii );			
						if( !sel )
						{
							setbit( m_sel_flags, FLAG_SEL_AREA );
							n->area[sid->i].selected = TRUE;
							n->area[sid->i].poly->SetSideSel( sid->ii, 1 );
							m_sel_count++;
						}
						else if( bInvert )
						{
							UnSelect( n, sid, bSet_cursor_mode );
							return m;
						}
						m_sel_net = n;
						m_sel_id = *sid;
						if( bSet_cursor_mode && m_sel_count == 1 )
						{
							m_Doc->m_dlist->CancelHighLight();
							int w = m_sel_net->area[sid->i].poly->GetW();
							n->area[sid->i].poly->HighlightSide( sid->ii, w );
							if( w >= 0 && m_seg_clearance )
								n->area[sid->i].poly->HighlightSide( sid->ii, -w-2*m_seg_clearance, TRANSPARENT_BACKGND );
							if( abs(w) > m_pcbu_per_wu )
								m_Doc->m_nlist->HighlightAreaSides( m_sel_net, sid->i, 3*m_pcbu_per_wu );
							SetCursorMode( CUR_AREA_SIDE_SELECTED );
							m_Doc->ProjectModified( TRUE );
						}
					}
					else if ( sid->sst == ID_CORNER )
					{		
						BOOL sel = n->area[sid->i].poly->GetSel( sid->ii );
						if( !sel )
						{
							setbit( m_sel_flags, FLAG_SEL_AREA );
							n->area[sid->i].selected = TRUE;
							n->area[sid->i].poly->SetSel( sid->ii, 1 );
							m_sel_count++;
						}
						else if( bInvert )
						{
							UnSelect( n, sid, bSet_cursor_mode );
							return m;
						}
						m_sel_net = n;
						m_sel_id = *sid;
						if( bSet_cursor_mode && m_sel_count == 1 )
						{
							m_Doc->m_dlist->CancelHighLight();
							m_Doc->m_nlist->HighlightAreaCorner( n, sid->i, sid->ii );
							m_Doc->m_nlist->HighlightAreaSides( n, sid->i, 3*m_pcbu_per_wu ); 
							SetCursorMode( CUR_AREA_CORNER_SELECTED );	
						}
					}
				}
			}
		}
	}
	// select text
	else if ( sid->type == ID_TEXT )
	{
		CText * t = (CText*)ptr;
		if(t)
		{
			m = t->m_merge;
			if( t->m_selected == 0 )
			{
				setbit( m_sel_flags, FLAG_SEL_TEXT );
				t->m_selected = 1;
				m_sel_count++;
			}
			else if( bInvert )
			{
				UnSelect( t, sid, bSet_cursor_mode );
				return m;
			}
			m_sel_text = t;
			m_sel_id = *sid;
			if( bSet_cursor_mode && m_sel_count == 1 )
			{
				m_Doc->m_dlist->CancelHighLight();
				m_Doc->m_tlist->HighlightText(t);
				cnet * tn = m_Doc->m_nlist->GetNetPtrByName( &t->m_str );
				if( tn )
				{
					m_Doc->m_nlist->HighlightNet( tn, TRANSPARENT_HILITE );
					m_Doc->m_nlist->HighlightNetConnections( tn, TRANSPARENT_HILITE );
					m_Doc->m_plist->HighlightAllPadsOnNet( tn, 2, 0, TRANSPARENT_HILITE );
				}
				SetCursorMode( CUR_TEXT_SELECTED );
			}
		}
	} 
	// select poly
	else if ( sid->type == ID_POLYLINE && sid->i < m_Doc->m_outline_poly.GetSize() )
	{		
		if( sid->ii < m_Doc->m_outline_poly[sid->i].GetNumCorners() )
		{
			m = m_Doc->m_outline_poly[sid->i].GetMerge();
			if (sid->sst == ID_SIDE)
			{	
				BOOL sel = m_Doc->m_outline_poly[sid->i].GetSideSel( sid->ii );
				if( !sel )
				{
					setbit( m_sel_flags, FLAG_SEL_OP );
					m_Doc->m_outline_poly[sid->i].SetSideSel( sid->ii, 1 );
					m_sel_count++;
				}
				else if( bInvert )
				{
					UnSelect( NULL, sid, bSet_cursor_mode );
					return m;
				}
				m_sel_id = *sid;
				if( bSet_cursor_mode && m_sel_count == 1 )
				{		
					m_Doc->m_dlist->CancelHighLight();
					m_Doc->m_outline_poly[sid->i].HighlightSide( sid->ii, m_Doc->m_outline_poly[sid->i].GetW()+1 );
					SetCursorMode( CUR_OP_SIDE_SELECTED );
					m_polyline_layer = m_Doc->m_outline_poly[sid->i].GetLayer();
					m_polyline_hatch = m_Doc->m_outline_poly[sid->i].GetHatch();
					m_polyline_width = m_Doc->m_outline_poly[sid->i].GetW();
					m_polyline_closed = m_Doc->m_outline_poly[sid->i].GetClosed();
				}
			}
			else if (sid->sst == ID_CORNER)		
			{
				BOOL sel = m_Doc->m_outline_poly[sid->i].GetSel( sid->ii );
				if( !sel )
				{
					setbit( m_sel_flags, FLAG_SEL_OP );
					m_Doc->m_outline_poly[sid->i].SetSel( sid->ii, 1 );
					m_sel_count++;
				}
				else if( bInvert )
				{
					UnSelect( NULL, sid, bSet_cursor_mode );
					return m;
				}
				m_sel_id = *sid;
				if( bSet_cursor_mode && m_sel_count == 1 )
				{
					m_Doc->m_dlist->CancelHighLight();
					m_Doc->m_outline_poly[sid->i].HighlightCorner( sid->ii );
					SetCursorMode( CUR_OP_CORNER_SELECTED );
				}
			}
		}
	}
	// select dre
	if( sid->type == ID_DRC && sid->st == ID_DRE )
	{
		setbit( m_sel_flags, FLAG_SEL_DRE );
		m_sel_dre = (DRError*)ptr;
		if( m_sel_dre )
		{
			m_sel_id = *sid;
			m_sel_count++;
			if( bSet_cursor_mode && m_sel_count == 1 )
			{
				m_Doc->m_drelist->HighLight( m_sel_dre );
				SetCursorMode( CUR_DRE_SELECTED );
			}
		}
	}
	// select pad
	else if( (sid->type == ID_PART && sid->st != ID_SEL_RECT) || sid->type == ID_PART_LINES )
	{
		setbit( m_sel_flags, FLAG_SEL_PART );
		cpart * p = (cpart*)ptr;
		//
		if( p )
		{
			if( m_sel_id.i != (*sid).i || m_sel_part != p )
				m_sel_count++;
			m_sel_part = p;
			m_sel_id = *sid;
			m = m_sel_part->m_merge;
			if( bSet_cursor_mode && m_sel_count == 1 )
			{
				if( sid->st == ID_PAD && sid->i < m_sel_part->shape->GetNumPins() )
				{
					m_Doc->m_dlist->CancelHighLight();
					m_Doc->m_plist->SelectPad( m_sel_part, sid->i, 0, 0 );	
					ShowPinState( m_sel_part, sid->i );
					SetCursorMode( CUR_PAD_SELECTED );
					CString pin = m_sel_part->ref_des + "." + m_sel_part->shape->m_padstack[sid->i].name;
					CString net = "Unconnected";
					cnet * pn = m_sel_part->pin[sid->i].net;
					if( pn ) 
					{
						net = pn->name;
						if( pn->npins < m_Doc->m_auto_ratline_min_pins || m_Doc->m_auto_ratline_disable == 0 )
						{
							m_Doc->m_nlist->HighlightNet( pn, TRANSPARENT_LAYER );
							m_Doc->m_nlist->HighlightNetConnections( pn, TRANSPARENT_LAYER );
							m_Doc->m_plist->HighlightAllPadsOnNet( pn, 1, m_active_layer, m_active_layer, sid->i, m_sel_part );
						}
					}
					OnInfoBoxSendMess("part_list " + pin + " \"" + net + "\"");
				}
				else if( sid->st == ID_SEL_REF_TXT )
				{
					m_Doc->m_plist->SelectRefText( m_sel_part );
					m_Doc->m_plist->SelectPads( m_sel_part, 1, m_active_layer, TRANSPARENT_BACKGND );
					SetCursorMode( CUR_REF_SELECTED );
				}
				else if( sid->st == ID_SEL_VALUE_TXT )
				{
					m_Doc->m_plist->SelectValueText( m_sel_part );
					m_Doc->m_plist->SelectPads( m_sel_part, 1, m_active_layer, TRANSPARENT_BACKGND );
					SetCursorMode( CUR_VALUE_SELECTED );
				}
			}
		}
		else
		{
			int test = 0;
		}
	}
	
	// select group
	if( bSet_cursor_mode && m_sel_count > 1 )
	{
		m_Doc->m_dlist->CancelHighLight();
		SetCursorMode( CUR_GROUP_SELECTED );
		HighlightGroup();
		CString AllRef="";
		m_Doc->m_plist->GetSelParts( &AllRef );
		OnInfoBoxSendMess( "part_list " + AllRef );
	}
	return m;
}

// marking
void CFreePcbView::MarkAllOutlinePoly( int utility, int layer )
{
	// mark all corners of solder mask cutouts
	for( int im=0; im<m_Doc->m_outline_poly.GetSize(); im++ )
	{
		CPolyLine * poly = &m_Doc->m_outline_poly[im];
		if( poly->GetLayer() != layer && layer >= 0 )
			continue;
		poly->SetUtility(utility);
		for( int ic=poly->GetNumCorners()-1; ic>=0; ic-- )
			poly->SetUtility( ic, utility );	
	}
}



//===============================================================================================
//---------------------------------------- SetUpParts -------------------------------------------
//===============================================================================================
void CFreePcbView::SetUpParts ()
{
	if( !getbit(m_sel_flags, FLAG_SEL_PART) )
		return;
	m_Doc->m_dlist->CancelHighLight();
	if( getbit( m_sel_flags, FLAG_SEL_CONNECT ) )
		for( cnet * n = m_Doc->m_nlist->GetFirstNet(); n; n = m_Doc->m_nlist->GetNextNet(/*LABEL*/) )
			for( int i=0; i<n->nconnects; i++ )
			{
				cconnect * sc = &n->connect[i];
				if( sc->m_selected )
					for( int ii=sc->nsegs-1; ii>=0; ii-- )
						if( sc->seg[ii].selected )
						{
							id _id( ID_NET, ID_CONNECT, i, ID_SEG, ii );
							UnSelect(n,&_id);
						}
			}
	//**if( !m_sel_part )
	{
		int max_pins = 0;
		for(cpart* nextp=m_Doc->m_plist->GetFirstPart(); nextp; nextp=m_Doc->m_plist->GetNextPart(nextp))
			if( nextp->shape )
				if( nextp->selected )
				{
					int nps = nextp->shape->GetNumPins();
					if( nps > max_pins )
					{
						max_pins = nps;
						m_sel_part = nextp;
						m_sel_id.Set( ID_PART_DEF );
					}
				}
	}
	for(cpart* nextp=m_Doc->m_plist->GetFirstPart(); nextp; nextp=m_Doc->m_plist->GetNextPart(nextp))
		if( nextp->selected && nextp != m_sel_part )
		{
			m_Doc->m_plist->Move( nextp, 0, 0, 0, nextp->side );
			m_Doc->m_nlist->PartMoved( nextp, TRUE );
		}

	static int iMod=0;
	iMod = abs( iMod );
	iMod++;
	if( iMod > 2 )
		iMod = 0;
	int MARK = 1;
	m_Doc->m_plist->MarkAllParts(0);
	m_sel_part->utility = MARK;
	cpart * find_p;
	do
	{
		if( iMod == 2 )
			iMod = -2;
		else if( iMod == -2 )
			iMod = 2;
		find_p = NULL;
		double max_S=0, max_pins=0;
		for(cpart* nextp=m_Doc->m_plist->GetFirstPart(); nextp; nextp=m_Doc->m_plist->GetNextPart(nextp))
			if( nextp->selected && !nextp->utility )
			{
				CPoint pp, fpp;
				int npp /*= nextp->shape->GetNumPins()*/, n_pins_g;
				cpart * fp=0;
				m_Doc->m_plist->FindNetPointForPart( nextp, &pp, &npp, fp, &fpp, &n_pins_g, max( 0, iMod ) );
				if( npp > max_pins )
				{
					find_p = nextp;
					max_pins = npp;
				}
				else if( npp == max_pins )
				{
					RECT pr;
					m_Doc->m_plist->GetPartBoundingRect( nextp, &pr );
					double pS = ((double)pr.right-(double)pr.left)*((double)pr.top-(double)pr.bottom);
					if( pS > max_S )
					{
						find_p = nextp;
						max_S = pS;
					}
				}
			}
		if( find_p )
		{
			double sum_len[4];
			double xPosition[4];
			double yPosition[4];
			for( int p_angle=0; p_angle<360; p_angle+=90 )
			{
				m_Doc->m_plist->Move( find_p, find_p->x, find_p->y, p_angle, find_p->side );
				CPoint new_point;
				new_point = m_Doc->m_plist->PartAutoLocation( find_p, max( 100000,m_Doc->m_part_grid_spacing ), max( 0, iMod ), m_Doc->m_dr.pad_pad );
				xPosition[p_angle/90] = new_point.x;
				yPosition[p_angle/90] = new_point.y;
				m_Doc->m_plist->Move( find_p, new_point.x, new_point.y, p_angle, find_p->side, FALSE );
				m_Doc->m_nlist->PartMoved( find_p, TRUE, FALSE );
				m_Doc->m_nlist->OptimizeConnections( find_p, m_Doc->m_auto_ratline_disable, 
													 m_Doc->m_auto_ratline_min_pins, 0 );
				sum_len[p_angle/90] = 0;
				for( cnet * net=m_Doc->m_nlist->GetFirstNet(); net; net=m_Doc->m_nlist->GetNextNet(/*LABEL*/) )
				{
					for( int np=0; np<net->npins; np++ )
						net->pin[np].utility = 0;
					for( int icon=0; icon<net->nconnects; icon++ )
					{
						if( net->connect[icon].end_pin >= 0 )
							if( net->connect[icon].nsegs == 1 )
								if( net->connect[icon].seg[0].layer == LAY_RAT_LINE )
									if( net->pin[net->connect[icon].start_pin].part == find_p || net->pin[net->connect[icon].end_pin].part == find_p )
										if( net->pin[net->connect[icon].start_pin].part->utility || net->pin[net->connect[icon].end_pin].part->utility )
										{
											int x1 = net->connect[icon].vtx[0].x;
											int y1 = net->connect[icon].vtx[0].y;
											int x2 = net->connect[icon].vtx[1].x;
											int y2 = net->connect[icon].vtx[1].y;
											double len = Distance( x1,y1,x2,y2 );
											//
											int i_p = net->connect[icon].start_pin;
											if( net->pin[net->connect[icon].end_pin].part == find_p )
												i_p = net->connect[icon].end_pin;
											if( len < net->pin[i_p].utility*m_pcbu_per_wu || net->pin[i_p].utility == 0 )
											{
												net->pin[i_p].utility = (len/m_pcbu_per_wu);
											}
										}
					}
					for( int ip=net->npins-1 ; ip>=0; ip-- )
						sum_len[p_angle/90] += net->pin[ip].utility;
				}
			}
			double min = (double)DEFAULT*(double)INT_MAX;
			int best = 0;
			for( int i=0; i<4; i++ )
			{
				if( sum_len[i] < min )
				{
					best = i;
					min = sum_len[i];
				}
			}
			m_Doc->m_plist->Move( find_p, xPosition[best], yPosition[best], best*90, find_p->side );
			m_Doc->m_nlist->PartMoved( find_p, TRUE );
			UpdateWindow();
			Invalidate( FALSE );//partsAutoPos
			find_p->utility = MARK;
		}
	}while( find_p );
	m_Doc->m_nlist->OptimizeConnections();
	m_Doc->m_dlist->CancelHighLight();
	HighlightGroup();
}

void CFreePcbView::ShowPinState (cpart * p, int pin)
{
	//pin name
	CRect pr,pr2;
	m_Doc->m_dlist->Get_Rect( p->pin[pin].dl_sel, &pr );
	//
	long long TH = (long long)NM_PER_MIL*2.0*m_pcbu_per_pixel/(long long)m_pcbu_per_wu;
	//TH = min( TH, NM_PER_MM*2 );
	TH = max( TH, NM_PER_MM/10 );
	CString pnm = p->shape->GetPinNameByIndex( pin );
	CString text;
	text.Format("%s.%s", p->ref_des, pnm );
	CText * tpin = m_Doc->m_tlist->AddText(pr.left+(TH/3),pr.top+(TH/2),0,0,0,LAY_PAD_THRU,TH/2,TH/20,&text);
	m_Doc->m_dlist->HighLight( tpin->dl_el );
	clrbit(tpin->dl_el->layers_bitmap,LAY_PAD_THRU);
	dl_element * cel = m_Doc->m_dlist->Cpy( tpin->dl_el );
	cel->transparent = TRANSPARENT_BLACK_GROUND;
	m_Doc->m_dlist->Get_Rect( tpin->dl_el, &pr );
	m_Doc->m_tlist->RemoveText( tpin );
	pr2 = pr;

	//pin net name
	if( p->pin[pin].net )
	{
		text = p->pin[pin].net->name;
		CText * tnet = m_Doc->m_tlist->AddText(pr.left,pr.top+(TH/2),0,0,0,LAY_PAD_THRU,TH/2,TH/20,&text);
		m_Doc->m_dlist->HighLight( tnet->dl_el );
		clrbit(tnet->dl_el->layers_bitmap,LAY_PAD_THRU);
		cel = m_Doc->m_dlist->Cpy( tnet->dl_el );
		cel->transparent = TRANSPARENT_BLACK_GROUND;
		m_Doc->m_dlist->Get_Rect( tnet->dl_el, &pr2 );
		m_Doc->m_tlist->RemoveText( tnet );
	}

	// Rectangle
	//SwellRect( &pr, pr2 );
	//SwellRect( &pr, TH/5 );
	//int lmap = 0;
	//setbit(lmap,LAY_PAD_THRU);
	//dl_element * el = m_Doc->m_dlist->Add(NULL,NULL,lmap,DL_RRECT,1,&pr,TH/4,NULL,0);
	//el->transparent = TRANSPARENT_BACKGND;
	//m_Doc->m_dlist->HighLight( el );
	//clrbit(el->layers_bitmap,LAY_PAD_THRU);
}


