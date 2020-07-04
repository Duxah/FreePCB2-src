// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "mainfrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CFreePcbApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
	ON_MESSAGE( WM_USER_VISIBLE_GRID, OnChangeVisibleGrid )
	ON_MESSAGE( WM_USER_PLACEMENT_GRID, OnChangePlacementGrid )
	ON_MESSAGE( WM_USER_ROUTING_GRID, OnChangeRoutingGrid )
	ON_MESSAGE( WM_USER_SNAP_ANGLE, OnChangeSnapAngle )
	ON_MESSAGE( WM_USER_UNITS, OnChangeUnits )
	ON_WM_SYSCOMMAND()
	ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
	ON_WM_TIMER()
	ON_WM_SETCURSOR()
	ON_WM_MOVE()
	ON_WM_SIZE()
	ON_WM_COPYDATA()
	ON_MESSAGE(WM_DROPFILES, Wm__DropFiles)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	0,
	0,
	0,
	0,
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	m_bAutoMenuEnable = FALSE;
	m_bHideCursor = FALSE;
	m_bCursorHidden = FALSE;
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	if (!m_wndMyToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) )
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	// status bar stuff
	if (!m_wndStatusBar.Create(this ) ||
		!m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	
	// initialize pane 0 of status bar
	UINT uID, uStyle;
	int nWidth;
	m_wndStatusBar.GetPaneInfo( 0, uID, uStyle, nWidth );
	m_wndStatusBar.SetPaneInfo( 0, uID, uStyle | SBARS_SIZEGRIP, 200 );

	// initialize pane 1 of status bar
	m_wndStatusBar.GetPaneInfo( 1, uID, uStyle, nWidth );
	CDC * pDC = m_wndStatusBar.GetDC();
	pDC->SelectObject( m_wndStatusBar.GetFont() );
	CRect rectArea;
	pDC->DrawText( _T("X: -9999999"), 	-1, rectArea, DT_SINGLELINE | DT_CALCRECT );
	m_wndStatusBar.ReleaseDC( pDC );
	int pane_width = rectArea.Width();
	m_wndStatusBar.SetPaneInfo( 1, uID, uStyle, pane_width );
	CString test;
	test.Format( "X: 0" );
	m_wndStatusBar.SetPaneText( 1, test );

	// initialize pane 2 of status bar
	m_wndStatusBar.GetPaneInfo( 2, uID, uStyle, nWidth );
	pDC = m_wndStatusBar.GetDC();
	pDC->SelectObject( m_wndStatusBar.GetFont() );
	pDC->DrawText( _T("Y: -9999999"), -1, rectArea, DT_SINGLELINE | DT_CALCRECT );
	m_wndStatusBar.ReleaseDC( pDC );
	pane_width = rectArea.Width();
	m_wndStatusBar.SetPaneInfo( 2, uID, uStyle, pane_width );
	test.Format( "Y: 0" );
	m_wndStatusBar.SetPaneText( 2, test );

	// initialize pane 3 of status bar
	m_wndStatusBar.GetPaneInfo( 3, uID, uStyle, nWidth );
	pDC = m_wndStatusBar.GetDC();
	pDC->SelectObject( m_wndStatusBar.GetFont() );
	pDC->DrawText( _T("********************************************STATUS STRING**************************************************"), 
						-1, rectArea, DT_SINGLELINE | DT_CALCRECT );
	m_wndStatusBar.ReleaseDC( pDC );
	pane_width = rectArea.Width();
	m_wndStatusBar.SetPaneInfo( 3, uID, uStyle, pane_width );
	test.Format( "hello" );
	m_wndStatusBar.SetPaneText( 3, test );

	// initialize pane 4 of status bar
	m_wndStatusBar.GetPaneInfo( 4, uID, uStyle, nWidth );
	pDC = m_wndStatusBar.GetDC();
	pDC->SelectObject( m_wndStatusBar.GetFont() );
	pDC->DrawText( _T("BottomXXX"), -1, rectArea, DT_SINGLELINE | DT_CALCRECT );
	m_wndStatusBar.ReleaseDC( pDC );
	pane_width = rectArea.Width();
	m_wndStatusBar.SetPaneInfo( 4, uID, uStyle, pane_width );
	test.Format( "hello" );
	m_wndStatusBar.SetPaneText( 4, test );

	// menu stuff
	CMenu* pMenu = GetMenu();
	pMenu->EnableMenuItem( 1, MF_BYPOSITION | MF_DISABLED | MF_GRAYED ); 
	pMenu->EnableMenuItem( 2, MF_BYPOSITION | MF_DISABLED | MF_GRAYED ); 
	pMenu->EnableMenuItem( 3, MF_BYPOSITION | MF_DISABLED | MF_GRAYED ); 
	pMenu->EnableMenuItem( 4, MF_BYPOSITION | MF_DISABLED | MF_GRAYED ); 
	pMenu->EnableMenuItem( 5, MF_BYPOSITION | MF_DISABLED | MF_GRAYED ); 
	//pMenu->EnableMenuItem( 6, MF_BYPOSITION | MF_DISABLED | MF_GRAYED ); 
	pMenu->EnableMenuItem( 7, MF_BYPOSITION | MF_DISABLED | MF_GRAYED ); 
	CMenu* submenu = pMenu->GetSubMenu(0);	// "File" submenu
	submenu->EnableMenuItem( ID_FILE_SAVE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
	submenu->EnableMenuItem( ID_FILE_SAVE_AS, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
	submenu->EnableMenuItem( ID_FILE_CLOSE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
	submenu->EnableMenuItem( ID_FILE_IMPORTNETLIST, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
	submenu->EnableMenuItem( ID_FILE_EXPORTNETLIST, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
	submenu->EnableMenuItem( ID_FILE_GENERATECADFILES, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );
	submenu->EnableMenuItem( ID_FILE_GENERATEREPORTFILE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );
	submenu->EnableMenuItem( ID_DSN_FILE_EXPORT, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );
	submenu->EnableMenuItem( ID_SES_FILE_IMPORT, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );
	submenu->EnableMenuItem( ID_FILE_SAVEPROJECTASLIBRARY, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );
	submenu = pMenu->GetSubMenu(1);	// "Edit" submenu
	submenu->EnableMenuItem( ID_EDIT_UNDO, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
	submenu->EnableMenuItem( ID_EDIT_REDO, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
	// create timer event every TIMER_PERIOD seconds
	m_timer = SetTimer( 1, TIMER_PERIOD*1000, 0 );
	// separator
	// DecimalSeparator;
	// cursor
	SetCursor( LoadCursor( NULL, IDC_ARROW ) );
	DragAcceptFiles();
	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// Read Registry settings for window placement.
	cs.x = (int)theApp.GetProfileInt(_T("Settings"),_T("LeftWinCoord"),cs.x);
	cs.y = (int)theApp.GetProfileInt(_T("Settings"),_T("TopWinCoord"),cs.y);
	cs.cx = (int)theApp.GetProfileInt(_T("Settings"),_T("WinWidth"),cs.cx);
	cs.cy = (int)theApp.GetProfileInt(_T("Settings"),_T("WinHeight"),cs.cy);

	return CFrameWnd::PreCreateWindow(cs);
}

int CMainFrame::DrawStatus( int pane, CString * str )
{
	m_wndStatusBar.SetPaneText( pane, *str );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

LONG CMainFrame::OnChangeVisibleGrid( UINT wp, LONG lp )
{
	m_view = (CView*)GetActiveView();
	if( m_view->IsKindOf( RUNTIME_CLASS(CFreePcbView) ) )
		((CFreePcbView*)m_view)->OnChangeVisibleGrid( wp, lp );
	else if( m_view->IsKindOf( RUNTIME_CLASS(CFootprintView) ) )
		((CFootprintView*)m_view)->OnChangeVisibleGrid( wp, lp );
	return 0;
}

LONG CMainFrame::OnChangePlacementGrid( UINT wp, LONG lp )
{
	m_view = (CView*)GetActiveView();
	if( m_view->IsKindOf( RUNTIME_CLASS(CFreePcbView) ) )
		((CFreePcbView*)m_view)->OnChangePlacementGrid( wp, lp );
	else if( m_view->IsKindOf( RUNTIME_CLASS(CFootprintView) ) )
		((CFootprintView*)m_view)->OnChangePlacementGrid( wp, lp );
	return 0;
}

LONG CMainFrame::OnChangeRoutingGrid( UINT wp, LONG lp )
{
	m_view = (CView*)GetActiveView();
	if( m_view->IsKindOf( RUNTIME_CLASS(CFreePcbView) ) )
		((CFreePcbView*)m_view)->OnChangeRoutingGrid( wp, lp );
	return 0;
}

LONG CMainFrame::OnChangeSnapAngle( UINT wp, LONG lp )
{
	m_view = (CView*)GetActiveView();
	if( m_view->IsKindOf( RUNTIME_CLASS(CFreePcbView) ) )
		((CFreePcbView*)m_view)->OnChangeSnapAngle( wp, lp );
	else if( m_view->IsKindOf( RUNTIME_CLASS(CFootprintView) ) )
		((CFootprintView*)m_view)->OnChangeSnapAngle( wp, lp );
	return 0;
}

LONG CMainFrame::OnChangeUnits( UINT wp, LONG lp )
{
	m_view = (CView*)GetActiveView();
	if( m_view->IsKindOf( RUNTIME_CLASS(CFreePcbView) ) )
		((CFreePcbView*)m_view)->OnChangeUnits( wp, lp );
	else if( m_view->IsKindOf( RUNTIME_CLASS(CFootprintView) ) )
		((CFootprintView*)m_view)->OnChangeUnits( wp, lp );
	return 0;
}

LRESULT CMainFrame::Wm__DropFiles( WPARAM Message, LPARAM qwerty )
{
	CString FName ;
	char buff[MAX_PATH] ;
	HDROP hDrop = (HDROP)Message ;
	int nFiles = DragQueryFile(hDrop, -1, NULL, NULL) ;
	for(int i=0; i<nFiles; i++) 
	{ 
		DragQueryFile(hDrop, i, buff, sizeof(buff)) ; 
		FName = buff ; 
		m_view = (CView*)GetActiveView();
		if( (CFreePcbView*)m_view )
		{
			((CFreePcbView*)m_view)->m_Doc->OnFileAutoOpen( FName );
			break;
		}
	}
	DragFinish(hDrop) ;
	return 0;
}

int CMainFrame::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* Msg) 
{
	enum{C_JUMP=0, C_PARTS, C_PINS, C_MOVE, C_PASTE, C_ENQUARY, C_INVALIDATE};
	int np, mode=-1;
	CArray<CString> p;
	p.SetSize( 0 );
	CString Mess, sz;
	Mess = (char*)Msg->lpData;
	CString key_str;
	if (Mess.Left(4) == "jump")
	{
		key_str = "jump";
		mode = C_JUMP;
	}
	else if (Mess.Left(4) == "pins")
	{
		key_str = "pins";
		mode = C_PINS;
	}
	else if (Mess.Left(5) == "parts")
	{
		key_str = "parts";
		mode = C_PARTS;
	}
	else if (Mess.Left(4) == "move")
	{
		key_str = "move";
		mode = C_MOVE;
	}
	else if (Mess.Left(5) == "paste")
	{
		key_str = "paste";
		mode = C_PASTE;
	}
	else if (Mess.Left(7) == "enquary")
	{
		key_str = "enquary";
		mode = C_ENQUARY;
	}
	else if(Mess.Left(10) == "invalidate")
	{
		key_str = "invalidate";
		mode = C_INVALIDATE;
	}
	if (mode >= 0)
	{
		np = ParseKeyString( &Mess, &key_str, &p );
		m_view = (CView*)GetActiveView();
		if( (CFreePcbView*)m_view )
			((CFreePcbView*)m_view)->OnInfoBoxMess(mode,np,&p);
	}
	return 0;
}

void CMainFrame::OnSysCommand(UINT nID, LPARAM lParam)
{
	m_view = (CView*)GetActiveView();
	if( nID == SC_CLOSE )
	{
		CFreePcbDoc * doc = (CFreePcbDoc*)GetActiveDocument();
		if( m_view->IsKindOf( RUNTIME_CLASS(CFreePcbView) ) )
		{
			if( doc )
			{
				if( doc->FileClose() == IDCANCEL )
					return;
			}
		}
		else
		{
			((CFootprintView*)m_view)->OnFootprintFileClose();
			return;
		}
	}
	CFrameWnd::OnSysCommand(nID, lParam);
}

void CMainFrame::OnEditUndo()
{
	m_view = (CView*)GetActiveView();
	CFreePcbDoc * doc = (CFreePcbDoc*)GetActiveDocument();
	if( m_view->IsKindOf( RUNTIME_CLASS(CFreePcbView) ) )
		doc->OnEditUndo();
	else if( m_view->IsKindOf( RUNTIME_CLASS(CFootprintView) ) )
		((CFootprintView*)m_view)->OnEditUndo();
}

void CMainFrame::OnTimer(UINT nIDEvent)
{
	m_view = (CView*)GetActiveView();
	if( m_view->IsKindOf( RUNTIME_CLASS(CFreePcbView) ) )
	{
		CFreePcbDoc * doc = (CFreePcbDoc*)GetActiveDocument();
		if( doc->m_project_open )
			doc->OnTimer();
	}
	CFrameWnd::OnTimer(nIDEvent);
}

BOOL CMainFrame::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if( !m_bHideCursor )
	{
		// cursor hiding disabled
		if(!m_bCursorHidden )
		{
			return( CFrameWnd::OnSetCursor(pWnd, nHitTest, message) );
		}
		else
		{
			::ShowCursor( TRUE );
			m_bCursorHidden = FALSE;
		}
	}
	else  
	{
		// cursor hiding enabled
		CPoint cur_pt;
		GetCursorPos( &cur_pt );
		if( m_hide_cursor_rect.PtInRect(cur_pt) )
		{
			if( !m_bCursorHidden )
			{
				::ShowCursor( FALSE );
				m_bCursorHidden = TRUE;
				return TRUE;
			}
		}
		else
		{
			if( m_bCursorHidden )
			{
				::ShowCursor( TRUE );
				m_bCursorHidden = FALSE;
				return TRUE;
			}
		}
	}
	return TRUE;
}

// on moving window, get client area in screen coords
//
void CMainFrame::OnMove(int x, int y)
{
	CPoint new_client_topleft_pt(x,y+24);
	m_hide_cursor_rect.OffsetRect( new_client_topleft_pt - m_client_rect.TopLeft() );
	m_client_rect.MoveToXY( new_client_topleft_pt );
	CFrameWnd::OnMove(x, y);
}

// on resizing window, get client area in screen coords
//
void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
	int new_client_width = cx;
	int new_client_height = cy - 48;
	int dx = new_client_width - m_client_rect.Width();
	int dy = new_client_height - m_client_rect.Height();
	m_client_rect.right += dx;
	m_client_rect.bottom += dy;
	m_hide_cursor_rect.right += dx;
	m_hide_cursor_rect.bottom += dy;
	/*CRect wr;
	GetWindowRect(&wr);
	int pane_width = wr.right - wr.left - 150;
	UINT uID, uStyle;
	int nWidth;
	m_wndStatusBar.GetPaneInfo( 0, uID, uStyle, nWidth );
	m_wndStatusBar.SetPaneInfo( 3, uID, uStyle, pane_width );*/
	CFrameWnd::OnSize(nType, cx, cy);
}

// enable and set a screen rectangle for hiding the cursor, or disable
//
void CMainFrame::SetHideCursor( BOOL bHideEnable, CRect * screen_rect )
{
	if( bHideEnable && screen_rect )
		m_hide_cursor_rect = screen_rect;
	m_bHideCursor = bHideEnable;
}

BOOL CMainFrame::DestroyWindow()
{
	// Read current WINDOWPLACEMENT value
	WINDOWPLACEMENT wpl;
	GetWindowPlacement(&wpl);
	// write settings to registry
	theApp.WriteProfileInt(_T("Settings"),_T("LeftWinCoord"),(int)wpl.rcNormalPosition.left);
	theApp.WriteProfileInt(_T("Settings"),_T("TopWinCoord"),(int)wpl.rcNormalPosition.top);
	theApp.WriteProfileInt(_T("Settings"),_T("WinWidth"),((int)wpl.rcNormalPosition.right-(int)wpl.rcNormalPosition.left));
	theApp.WriteProfileInt(_T("Settings"),_T("WinHeight"),((int)wpl.rcNormalPosition.bottom-(int)wpl.rcNormalPosition.top));
	theApp.WriteProfileInt(_T("Settings"),_T("ShowCmd"),wpl.showCmd);

	return CWnd::DestroyWindow();
}