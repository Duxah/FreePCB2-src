// DlgMoveOrigin.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgMoveOrigin.h"
#include ".\dlgmoveorigin.h"


// CDlgMoveOrigin dialog

IMPLEMENT_DYNAMIC(CDlgMoveOrigin, CDialog)
CDlgMoveOrigin::CDlgMoveOrigin(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgMoveOrigin::IDD, pParent)
{
}

CDlgMoveOrigin::~CDlgMoveOrigin()
{
}

void CDlgMoveOrigin::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_RADIO1, m_radio_drag);
	DDX_Control(pDX, IDC_RADIO2, m_radio_set);
	DDX_Control(pDX, IDC_COMBO1, m_combo_units);
	DDX_Control(pDX, IDC_EDIT1, m_edit_x);
	DDX_Control(pDX, IDC_EDIT2, m_edit_y);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		m_combo_units.AddString( "MIL" );
		m_combo_units.AddString( "MM" );
		m_combo_units.SetCurSel(0);
		m_radio_drag.SetCheck(1);
		SetFields();
	}
	else
	{
		// outgoing
		GetFields();
	}
}


BEGIN_MESSAGE_MAP(CDlgMoveOrigin, CDialog)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnCbnSelchangeUnits)
	ON_BN_CLICKED(IDC_RADIO1, OnBnClickedDrag)
	ON_BN_CLICKED(IDC_RADIO2, OnBnClickedSet)
END_MESSAGE_MAP()

void CDlgMoveOrigin::Initialize( int units )
{
	m_units = units;
	m_x = m_y = 0;
}

void CDlgMoveOrigin::GetFields()
{
	CString str;
	if( m_radio_drag.GetCheck() )
		m_drag = TRUE;
	else
		m_drag = FALSE;
	m_edit_x.GetWindowText( str );
	m_x = atof( str ) * m_mult;
	m_edit_y.GetWindowText( str );
	m_y = atof( str ) * m_mult;
}

void CDlgMoveOrigin::SetFields()
{
	CString str;
	if( m_combo_units.GetCurSel() == 0 )
	{
		m_units = MIL;
		m_mult = NM_PER_MIL;
	}
	else
	{
		m_units = MM;
		m_mult = 1000000.0;
	}
	if( m_radio_drag.GetCheck() )
	{
		m_combo_units.EnableWindow(0);
		m_edit_x.EnableWindow(0);
		m_edit_y.EnableWindow(0);
		m_x = m_y = 0;
		m_edit_x.SetWindowText( "" );
		m_edit_y.SetWindowText( "" );
	}
	else
	{
		m_combo_units.EnableWindow(1);
		m_edit_x.EnableWindow(1);
		m_edit_y.EnableWindow(1);
		MakeCStringFromDouble( &str, m_x/m_mult );
		m_edit_x.SetWindowText( str );
		MakeCStringFromDouble( &str, m_y/m_mult );
		m_edit_y.SetWindowText( str );
	}
}

// CDlgMoveOrigin message handlers

void CDlgMoveOrigin::OnCbnSelchangeUnits()
{
	GetFields();
	SetFields(); 
}

void CDlgMoveOrigin::OnBnClickedDrag()
{
	GetFields();
	SetFields(); 
}

void CDlgMoveOrigin::OnBnClickedSet()
{
	GetFields();
	SetFields(); 
}
