// DlgSlot.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgSlot.h"

// defines
enum {
	POS_DEFAULT,
	POS_DEFINED,
	POS_DRAG
};

// CDlgSlot dialog

IMPLEMENT_DYNAMIC(CDlgSlot, CDialog)
CDlgSlot::CDlgSlot(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgSlot::IDD, pParent)
{
}

CDlgSlot::~CDlgSlot()
{
}

void CDlgSlot::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_RADIO2, m_radio_set);
	DDX_Control(pDX, IDC_RADIO1, m_radio_drag);
	DDX_Control(pDX, IDC_RADIO5, m_radio_plated);
	DDX_Control(pDX, IDC_RADIO6, m_radio_unplated);
	DDX_Control(pDX, IDC_COMBO1, m_combo_units);
	DDX_Control(pDX, IDC_EDIT1, m_edit_x);
	DDX_Control(pDX, IDC_EDIT2, m_edit_y);
	DDX_Control(pDX, IDC_EDIT_DIAMETER, m_edit_w);
	DDX_Control(pDX, IDC_EDIT_TRAVEL, m_edit_travel);
	DDX_Control(pDX, IDC_COMBO_DIRECTION, m_combo_direction);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		m_combo_units.InsertString( 0, "MIL" );
		m_combo_units.InsertString( 1, "MM" );
		m_combo_direction.InsertString( 0, "RIGHT" );
		m_combo_direction.InsertString( 1, "DOWN" );
		m_combo_direction.InsertString( 2, "LEFT" );
		m_combo_direction.InsertString( 3, "UP" );
		SetFields();
	}
	else
	{
		// outgoing
		GetFields();
	}
}


BEGIN_MESSAGE_MAP(CDlgSlot, CDialog)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnCbnSelChangeCombo1)
	ON_BN_CLICKED(IDC_RADIO2, OnBnClickedSet)
	ON_BN_CLICKED(IDC_RADIO1, OnBnClickedDrag)
	ON_BN_CLICKED(IDC_RADIO5, OnBnClickedPlated)
	ON_BN_CLICKED(IDC_RADIO6, OnBnClickedUnplated)
END_MESSAGE_MAP()

void CDlgSlot::Initialize( int units, int w, int travel, int dir,
						  int x, int y, BOOL bPlated, BOOL bDrag )
{
	m_bDrag =  bDrag;
	m_bPlated =  bPlated;
	m_units = units;
	m_w = w;
	m_travel = travel;
	m_dir = dir;
	m_x = x;
	m_y = y;
}

void CDlgSlot::SetFields()
{
	CString str;
	m_radio_plated.SetCheck( m_bPlated );
	m_radio_unplated.SetCheck( !m_bPlated );
	m_radio_set.SetCheck( !m_bDrag );
	m_radio_drag.SetCheck( m_bDrag );
	m_edit_x.EnableWindow( !m_bDrag );
	m_edit_y.EnableWindow( !m_bDrag );
	m_combo_direction.SetCurSel( m_dir/90 );
	if( m_units == MIL ) 
		m_combo_units.SetCurSel( 0 );
	else
		m_combo_units.SetCurSel( 1 );
	::MakeCStringFromDimension( &str, m_w, m_units, FALSE, FALSE, FALSE, 3 );
	m_edit_w.SetWindowText( str );
	::MakeCStringFromDimension( &str, m_travel, m_units, FALSE, FALSE, FALSE, 3 );
	m_edit_travel.SetWindowText( str );
	::MakeCStringFromDimension( &str, m_x, m_units, FALSE, FALSE, FALSE, 3 );
	m_edit_x.SetWindowText( str );
	::MakeCStringFromDimension( &str, m_y, m_units, FALSE, FALSE, FALSE, 3 );
	m_edit_y.SetWindowText( str );
}

void CDlgSlot::GetFields()
{
	CString str;
	m_bDrag = m_radio_drag.GetCheck();
	m_bPlated = m_radio_plated.GetCheck();
	m_dir = m_combo_direction.GetCurSel() * 90;
	m_edit_w.GetWindowText( str );
	m_w = ::GetDimensionFromString( &str, m_units );
	m_edit_travel.GetWindowText( str );
	m_travel = ::GetDimensionFromString( &str, m_units );
	m_edit_x.GetWindowText( str );
	m_x = ::GetDimensionFromString( &str, m_units );
	m_edit_y.GetWindowText( str );
	m_y = ::GetDimensionFromString( &str, m_units );
}


// CDlgSlot message handlers

void CDlgSlot::OnCbnSelChangeCombo1()
{
	GetFields();
	int m_sel_units = m_combo_units.GetCurSel();
	if( m_sel_units == 0 )
		m_units = MIL;
	else
		m_units = MM;
	SetFields();
}

void CDlgSlot::OnBnClickedSet()
{
	GetFields();
	SetFields();
}

void CDlgSlot::OnBnClickedDrag()
{
	GetFields();
	SetFields();
}

void CDlgSlot::OnBnClickedPlated()
{
	GetFields();
	SetFields();
}

void CDlgSlot::OnBnClickedUnplated()
{
	GetFields();
	SetFields();
}

