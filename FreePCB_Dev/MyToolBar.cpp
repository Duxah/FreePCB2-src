// MyToolBar.cpp : implementation file
//

#include "stdafx.h"
#include "MyToolBar.h"
#include "math.h"

//#define IDC_COMBO_VISIBLE_GRID 2000
#define COMBO_W 100
#define COMBO_H 100

#define POS_COMBO_VISIBLE_GRID 11

extern CFreePcbApp theApp;

// CMyToolBar

IMPLEMENT_DYNAMIC(CMyToolBar, CToolBar)
CMyToolBar::CMyToolBar()
{
	m_last_vg = -1;
}

CMyToolBar::~CMyToolBar()
{
}


BEGIN_MESSAGE_MAP(CMyToolBar, CToolBar)
	ON_WM_CREATE()
	ON_CBN_EDITCHANGE( IDC_COMBO_VISIBLE_GRID, OnEditVisibleGrid )
	ON_CBN_EDITCHANGE( IDC_COMBO_PLACEMENT_GRID, OnEditPlacementGrid )
	ON_CBN_SELENDOK( IDC_COMBO_VISIBLE_GRID, OnSelectVisibleGrid )
	ON_CBN_SELENDOK( IDC_COMBO_PLACEMENT_GRID, OnSelectPlacementGrid )
	ON_CBN_SELENDOK( IDC_COMBO_ROUTING_GRID, OnSelectRoutingGrid )
	ON_CBN_SELENDOK( IDC_COMBO_SNAP_ANGLE, OnSelectSnapAngle )
	ON_CBN_SELENDOK( IDC_COMBO_UNITS, OnSelectUnits )
END_MESSAGE_MAP()



// CMyToolBar message handlers

int CMyToolBar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CToolBar::OnCreate(lpCreateStruct) == -1)
		return -1;

	if( !LoadToolBar( IDR_MYTOOLBAR ) )
		return -1;

	SetButtonInfo( 12, IDC_STATIC_VISIBLE_GRID, TBBS_SEPARATOR, 40 );
	SetButtonInfo( 13, IDC_STATIC_VISIBLE_GRID, TBBS_SEPARATOR, 50 );
	SetButtonInfo( 14, IDC_STATIC_VISIBLE_GRID, TBBS_SEPARATOR, 80 );
	SetButtonInfo( 15, IDC_COMBO_VISIBLE_GRID, TBBS_SEPARATOR, 90 );
	SetButtonInfo( 16, IDC_STATIC_PLACEMENT_GRID, TBBS_SEPARATOR, 64 );
	SetButtonInfo( 17, IDC_COMBO_PLACEMENT_GRID, TBBS_SEPARATOR, 90 );
	SetButtonInfo( 18, IDC_STATIC_ROUTING_GRID, TBBS_SEPARATOR, 50 );
	SetButtonInfo( 19, IDC_COMBO_ROUTING_GRID, TBBS_SEPARATOR, 90 );
	SetButtonInfo( 20, IDC_STATIC_SNAP_ANGLE, TBBS_SEPARATOR, 50 );
	SetButtonInfo( 21, IDC_COMBO_SNAP_ANGLE, TBBS_SEPARATOR, 50 );

	m_font.CreateFont( 14, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
		OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY, 
		DEFAULT_PITCH | FF_DONTCARE, "Arial" );
	SetFont( &m_font );

	RECT rect;
	GetItemRect( 12, &rect );
	rect.top += 5;
	rect.left += 5;
	m_ctlStaticUnits.Create( "   Units", WS_VISIBLE, rect, this, IDC_STATIC_VISIBLE_GRID );
	m_ctlStaticUnits.SetFont( &m_font );

	GetItemRect( 13, &rect );
	rect.bottom = rect.top + 300;
	m_ctlComboUnits.Create( WS_CHILD | WS_VISIBLE | WS_VSCROLL |
		CBS_DROPDOWNLIST, rect, this, IDC_COMBO_UNITS );
	m_ctlComboUnits.SetFont( &m_font );

	m_ctlComboUnits.InsertString( 0, "mil" );
	m_ctlComboUnits.InsertString( 1, "mm" );
	m_ctlComboUnits.SetCurSel( 0 );

	GetItemRect( 14, &rect );
	rect.top += 5;
	rect.left += 5;
	m_ctlStaticVisibleGrid.Create( "   Grids: Visible", WS_VISIBLE, rect, this, IDC_STATIC_VISIBLE_GRID );
	m_ctlStaticVisibleGrid.SetFont( &m_font );

	GetItemRect( 15, &rect );
	rect.bottom = rect.top + 300;
	m_ctlComboVisibleGrid.Create( WS_CHILD | WS_VISIBLE | WS_VSCROLL |
		CBS_DROPDOWN, rect, this, IDC_COMBO_VISIBLE_GRID );
	m_ctlComboVisibleGrid.SetFont( &m_font );

	GetItemRect( 16, &rect );
	rect.top += 5;
	rect.left += 5;
	m_ctlStaticPlacementGrid.Create( "  Placement", WS_VISIBLE, rect, this, IDC_STATIC_PLACEMENT_GRID );
	m_ctlStaticPlacementGrid.SetFont( &m_font );

	GetItemRect( 17, &rect );
	rect.bottom = rect.top + 300;
	m_ctlComboPlacementGrid.Create( WS_CHILD | WS_VISIBLE | WS_VSCROLL |
		CBS_DROPDOWN, rect, this, IDC_COMBO_PLACEMENT_GRID );
	m_ctlComboPlacementGrid.SetFont( &m_font );

	GetItemRect( 18, &rect );
	rect.top += 5;
	rect.left += 5;
	m_ctlStaticRoutingGrid.Create( "  Routing", WS_VISIBLE, rect, this, IDC_STATIC_ROUTING_GRID );
	m_ctlStaticRoutingGrid.SetFont( &m_font );

	GetItemRect( 19, &rect );
	rect.bottom = rect.top + 300;
	m_ctlComboRoutingGrid.Create( WS_CHILD | WS_VISIBLE | WS_VSCROLL |
		CBS_DROPDOWNLIST, rect, this, IDC_COMBO_ROUTING_GRID );
	m_ctlComboRoutingGrid.SetFont( &m_font );

	GetItemRect( 20, &rect );
	rect.top += 5;
	rect.left += 5;
	m_ctlStaticSnapAngle.Create( "  Angle", WS_VISIBLE, rect, this, IDC_STATIC_SNAP_ANGLE );
	m_ctlStaticSnapAngle.SetFont( &m_font );

	GetItemRect( 21, &rect );
	rect.bottom = rect.top + 300;
	m_ctlComboSnapAngle.Create( WS_CHILD | WS_VISIBLE | WS_VSCROLL |
		CBS_DROPDOWNLIST, rect, this, IDC_COMBO_SNAP_ANGLE );
	m_ctlComboSnapAngle.SetFont( &m_font );

	return 0;
}

void CMyToolBar::SetLists( CArray<double> * visible, 
			  CArray<double> * placement, 
			  CArray<double> * routing,
			  double vg, double pg, double rg, int ag,
			  int units )
{
	if( units == NM || units == MM )
		m_ctlComboUnits.SetCurSel(1);
	else
		m_ctlComboUnits.SetCurSel(0);
	m_v = visible;
	m_p = placement;
	m_r = routing;
	m_ctlComboVisibleGrid.ResetContent();
	m_ctlComboPlacementGrid.ResetContent();
	m_ctlComboRoutingGrid.ResetContent();
	m_ctlComboSnapAngle.ResetContent();
	m_ctlComboVisibleGrid.SetCurSel( 0 );
	m_ctlComboPlacementGrid.SetCurSel( 0 );
	m_ctlComboRoutingGrid.SetCurSel( 0 );
	m_ctlComboSnapAngle.SetCurSel( 0 );
	for( int i=0; i<visible->GetSize(); i++ )
	{
		CString str;
		double val = (*m_v)[i];
		BOOL is_mm = ( val < 0 );
		val = fabs( val );
		if( is_mm )
			str.Format( "%9.3f", val/1000000.0 );
		else
			str.Format( "%9.3f", val/NM_PER_MIL );
		str.Trim();
		if( str[str.GetLength()-1] == '0' )
			str = str.Left( str.GetLength()-1 );
		if( str[str.GetLength()-1] == '0' )
			str = str.Left( str.GetLength()-1 );
		if( str[str.GetLength()-1] == '0' )
			str = str.Left( str.GetLength()-1 );
		if( str[str.GetLength()-1] == '.' )
			str = str.Left( str.GetLength()-1 );
		if( is_mm )
			m_ctlComboVisibleGrid.AddString( str + " mm" );
		else
			m_ctlComboVisibleGrid.AddString( str + " mil" );
		if( val == vg )
			m_ctlComboVisibleGrid.SetCurSel( i );
	}
	for( int i=0; i<placement->GetSize(); i++ )
	{
		CString str;
		double val = (*m_p)[i];
		BOOL is_mm = ( val < 0 );
		val = fabs( val );
		if( is_mm )
			str.Format( "%9.3f", val/1000000.0 );
		else
			str.Format( "%9.3f", val/NM_PER_MIL );
		str.Trim();
		if( str[str.GetLength()-1] == '0' )
			str = str.Left( str.GetLength()-1 );
		if( str[str.GetLength()-1] == '0' )
			str = str.Left( str.GetLength()-1 );
		if( str[str.GetLength()-1] == '0' )
			str = str.Left( str.GetLength()-1 );
		if( str[str.GetLength()-1] == '.' )
			str = str.Left( str.GetLength()-1 );
		if( is_mm )
			m_ctlComboPlacementGrid.AddString( str + " mm" );
		else
			m_ctlComboPlacementGrid.AddString( str + " mil" );
		if( val == pg )
			m_ctlComboPlacementGrid.SetCurSel( i );
	}
	if( routing != NULL )
	{
		for( int i=0; i<routing->GetSize(); i++ )
		{
			CString str;
			double val = (*m_r)[i];
			BOOL is_mm = ( val < 0 );
			val = fabs( val );
			if( is_mm )
				str.Format( "%9.3f", val/1000000.0 );
			else
				str.Format( "%9.3f", val/NM_PER_MIL );
			str.Trim();
			if( str[str.GetLength()-1] == '0' )
				str = str.Left( str.GetLength()-1 );
			if( str[str.GetLength()-1] == '0' )
				str = str.Left( str.GetLength()-1 );
			if( str[str.GetLength()-1] == '0' )
				str = str.Left( str.GetLength()-1 );
			if( str[str.GetLength()-1] == '.' )
				str = str.Left( str.GetLength()-1 );
			if( is_mm )
				m_ctlComboRoutingGrid.AddString( str + " mm" );
			else
				m_ctlComboRoutingGrid.AddString( str + " mil" );
			if( val == rg )
				m_ctlComboRoutingGrid.SetCurSel( i );
		}
	}
	else
	{
		m_ctlComboRoutingGrid.AddString( "---" );
		m_ctlComboRoutingGrid.SetCurSel(0);
	}
	m_ctlComboSnapAngle.AddString( "45" );
	m_ctlComboSnapAngle.AddString( "90" );
	m_ctlComboSnapAngle.AddString( "Off" );
	m_ctlComboSnapAngle.SetCurSel( 0 );
}

void CMyToolBar::OnEditVisibleGrid()
{
	CString str;
	m_ctlComboVisibleGrid.GetWindowTextA(str);
	AfxGetMainWnd()->SendMessage( WM_USER_VISIBLE_GRID, WM_BY_STRING, (int)my_atof(&str) );
	return;
}

void CMyToolBar::OnEditPlacementGrid()
{
	CString str;
    m_ctlComboPlacementGrid.GetWindowTextA(str);
	AfxGetMainWnd()->SendMessage( WM_USER_PLACEMENT_GRID, WM_BY_INDEX, (int)my_atof(&str) );
	return;
}

void CMyToolBar::OnSelectVisibleGrid()
{
	CString str;

	int cur_sel = m_ctlComboVisibleGrid.GetCurSel();
	m_ctlComboVisibleGrid.GetLBText(cur_sel,str);
	AfxGetMainWnd()->SendMessage( WM_USER_VISIBLE_GRID, WM_BY_STRING, (int)my_atof(&str) );
	return;
}

void CMyToolBar::OnSelectPlacementGrid()
{
	CString str;

	int cur_sel = m_ctlComboPlacementGrid.GetCurSel();
    m_ctlComboPlacementGrid.GetLBText(cur_sel,str);
	AfxGetMainWnd()->SendMessage( WM_USER_PLACEMENT_GRID, WM_BY_INDEX, (int)my_atof(&str) );
	return;
}

void CMyToolBar::OnSelectRoutingGrid()
{
	CString str;

	int cur_sel = m_ctlComboRoutingGrid.GetCurSel();
	AfxGetMainWnd()->SendMessage( WM_USER_ROUTING_GRID, WM_BY_INDEX, cur_sel );
	return;
}

void CMyToolBar::OnSelectSnapAngle()
{
	CString str;

	int cur_sel = m_ctlComboSnapAngle.GetCurSel();
	AfxGetMainWnd()->SendMessage( WM_USER_SNAP_ANGLE, WM_BY_INDEX, cur_sel );
	return;
}

void CMyToolBar::OnSelectUnits()
{
	CString str;

	int cur_sel = m_ctlComboUnits.GetCurSel();
	AfxGetMainWnd()->SendMessage( WM_USER_UNITS, WM_BY_INDEX, cur_sel );
	return;
}

void CMyToolBar::SetUnits( int units )
{
	CString str;
	int cur_sel;

	if( units == MIL )
		cur_sel = 0;
	else
		cur_sel = 1;
	m_ctlComboUnits.SetCurSel( cur_sel );	
	AfxGetMainWnd()->SendMessage( WM_USER_UNITS, WM_BY_INDEX, cur_sel );
	return;
}

