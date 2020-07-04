// DlgEditBoardCorner.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgEditBoardCorner.h"


// DlgEditBoardCorner dialog

IMPLEMENT_DYNAMIC(DlgEditBoardCorner, CDialog)
DlgEditBoardCorner::DlgEditBoardCorner(CWnd* pParent /*=NULL*/)
	: CDialog(DlgEditBoardCorner::IDD, pParent)
{
	m_title_str = NULL;
	m_units = MIL;
	m_x = m_y = 0;
}

DlgEditBoardCorner::~DlgEditBoardCorner()
{
}

void DlgEditBoardCorner::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_CORNER_UNITS, m_combo_units);
	DDX_Control(pDX, IDC_EDIT_X, m_edit_x);
	DDX_Control(pDX, IDC_EDIT_Y, m_edit_y);
	if( !pDX->m_bSaveAndValidate && m_title_str )
	{
		// incoming
		SetWindowText( *m_title_str );
		m_combo_units.InsertString( 0, "MIL" );
		m_combo_units.InsertString( 1, "MM" );
		if( m_units == MIL )
			m_combo_units.SetCurSel(0);
		else
			m_combo_units.SetCurSel(1);
		SetFields();
	}
	else
	{
		// outgoing
		GetFields();
	}
}

void DlgEditBoardCorner::Init( CString * str, int units, int x, int y )
{
	m_title_str = str;
	m_x = x;
	m_y = y;
	m_units = units;
}

BEGIN_MESSAGE_MAP(DlgEditBoardCorner, CDialog)
	ON_CBN_SELCHANGE(IDC_COMBO_CORNER_UNITS, OnCbnSelchangeComboCornerUnits)
END_MESSAGE_MAP()


// DlgEditBoardCorner message handlers

void DlgEditBoardCorner::OnCbnSelchangeComboCornerUnits()
{
	GetFields();
	if( m_combo_units.GetCurSel() == 0 )
		m_units = MIL;
	else
		m_units = MM;
	SetFields();
}

void DlgEditBoardCorner::GetFields()
{
	// get x and y values
	CString xstr;
	m_edit_x.GetWindowText( xstr );
	CString ystr;
	m_edit_y.GetWindowText( ystr );
	if( m_units == MIL )
	{
		m_x = my_atof( &xstr )*NM_PER_MIL;
		m_y = my_atof( &ystr )*NM_PER_MIL;
	}
	else
	{
		m_x = my_atof( &xstr )*1000000.0;
		m_y = my_atof( &ystr )*1000000.0;
	}
}

void DlgEditBoardCorner::SetFields()
{
	CString str;
	if( m_units == MIL )
	{
		::MakeCStringFromDouble( &str, (double)m_x/NM_PER_MIL );
		m_edit_x.SetWindowText( str );
		::MakeCStringFromDouble( &str, (double)m_y/NM_PER_MIL );
		m_edit_y.SetWindowText( str );
	}
	else
	{
		::MakeCStringFromDouble( &str, (double)m_x/1000000.0 );
		m_edit_x.SetWindowText( str );
		::MakeCStringFromDouble( &str, (double)m_y/1000000.0 );
		m_edit_y.SetWindowText( str );
	}

}
