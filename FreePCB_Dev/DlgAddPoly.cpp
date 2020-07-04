// DlgAddPoly.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgAddPoly.h"
#include ".\dlgaddpoly.h"


// CDlgAddPoly dialog

IMPLEMENT_DYNAMIC(CDlgAddPoly, CDialog)
CDlgAddPoly::CDlgAddPoly(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgAddPoly::IDD, pParent)
{
	m_width = 10*NM_PER_MIL;	// default
}

CDlgAddPoly::~CDlgAddPoly()
{
}

void CDlgAddPoly::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_RADIO_OPEN, m_radio_open);
	DDX_Control(pDX, IDC_RADIO_CLOSED, m_radio_closed);
	DDX_Control(pDX, IDC_COMBO_ADD_POLY_UNITS, m_combo_units);
	DDX_Control( pDX, IDC_EDIT_WIDTH, m_edit_width );
	if( !pDX->m_bSaveAndValidate )
	{
		// entry
		m_radio_open.SetCheck( 1 );
		m_combo_units.InsertString( 0, "MIL" );
		m_combo_units.InsertString( 1, "MM" );
		if( m_units == MIL )
			m_combo_units.SetCurSel( 0 );
		else
			m_combo_units.SetCurSel( 1 );
		SetFields();
		if( m_Close )
		{
			m_radio_closed.SetCheck(1);
			m_radio_open.SetCheck(0);
		}
		else
		{
			m_radio_open.SetCheck(1);
			m_radio_closed.SetCheck(0);
		}
		if( m_bOpenedOnly )
		{
			m_radio_open.SetCheck(1);
			m_radio_closed.EnableWindow(0);
		}
		else if( m_bClosedOnly )
		{
			m_radio_closed.SetCheck(1);
			m_radio_open.EnableWindow(0);
		}
	}
	else
	{
		// exit
		GetFields();
		m_closed_flag = m_radio_closed.GetCheck();
		if( m_width < 0 || m_width > 999*NM_PER_MIL )
		{
			pDX->PrepareEditCtrl( IDC_EDIT_WIDTH );
			AfxMessageBox( "Width out of range (1 to 999 mils)" );
			pDX->Fail();
		}
	}
}

void CDlgAddPoly::Initialize( int units, int w, BOOL bCl, BOOL bOpenedOnly, BOOL bClosedOnly )
{
	m_units = units;
	m_width = max(NM_PER_MIL,w);
	m_Close = bCl;
	m_bOpenedOnly = bOpenedOnly;
	m_bClosedOnly = bClosedOnly;
}


BEGIN_MESSAGE_MAP(CDlgAddPoly, CDialog)
	ON_CBN_SELCHANGE(IDC_COMBO_ADD_POLY_UNITS, OnCbnSelchangeComboAddPolyUnits)
END_MESSAGE_MAP()


void CDlgAddPoly::GetFields()
{
	CString str;
	if( m_units == MIL )
	{
		m_edit_width.GetWindowText( str );
		m_width = atof( str ) * NM_PER_MIL;
	}
	else
	{
		m_edit_width.GetWindowText( str );
		m_width = atof( str ) * 1000000.0;
	}
}

void CDlgAddPoly::SetFields()
{
	CString str;
	if( m_units == MIL )
	{
		MakeCStringFromDouble( &str, m_width/NM_PER_MIL );
		m_edit_width.SetWindowText( str );
	}
	else
	{
		MakeCStringFromDouble( &str, m_width/1000000.0 );
		m_edit_width.SetWindowText( str );
	}
}
void CDlgAddPoly::OnCbnSelchangeComboAddPolyUnits()
{
	GetFields();
	if( m_combo_units.GetCurSel() == 0 )
		m_units = MIL;
	else
		m_units = MM;
	SetFields();
}

