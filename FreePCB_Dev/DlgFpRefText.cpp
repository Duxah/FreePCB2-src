// DlgFpRefText.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgFpRefText.h"
#include ".\dlgfpreftext.h"


// CDlgFpRefText dialog

IMPLEMENT_DYNAMIC(CDlgFpRefText, CDialog)
CDlgFpRefText::CDlgFpRefText(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgFpRefText::IDD, pParent)
{
}

CDlgFpRefText::~CDlgFpRefText()
{
}

void CDlgFpRefText::Initialize( int height, int width, int units )
{
	m_height = height;
	m_width = width;
	m_units = units;
}

void CDlgFpRefText::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_FP_REF_TEXT_UNITS, m_combo_units);
	DDX_Control(pDX, IDC_EDIT_FP_REF_CHAR_HEIGHT, m_edit_height);
	DDX_Control(pDX, IDC_RADIO_FP_REF_SET, m_radio_set);
	DDX_Control(pDX, IDC_EDIT_FP_REF_WIDTH, m_edit_width);
	DDX_Control(pDX, IDC_RADIO_FP_REF_DEF, m_radio_def);
	DDX_Control(pDX, IDC_EDIT_FP_REF_DEF_WIDTH, m_edit_def_width);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		m_combo_units.InsertString( 0, "MIL" );
		m_combo_units.InsertString( 1, "MM" );
		if( m_units == MIL )
			m_combo_units.SetCurSel( 0 );
		else
			m_combo_units.SetCurSel( 1 );
		SetFields();
		m_radio_set.SetCheck( 1 );
		m_edit_def_width.EnableWindow( FALSE );
		m_edit_width.EnableWindow( TRUE );
	}
	else
	{
		// outgoing
		GetFields();
		if( m_radio_def.GetCheck() )
			m_width = m_def_width;
	}
}


BEGIN_MESSAGE_MAP(CDlgFpRefText, CDialog)
	ON_BN_CLICKED(IDC_RADIO_FP_REF_SET, OnBnClickedRadioSet)
	ON_BN_CLICKED(IDC_RADIO_FP_REF_DEF, OnBnClickedRadioDef)
	ON_CBN_SELCHANGE(IDC_COMBO_FP_REF_TEXT_UNITS, OnCbnSelchangeComboRefTextUnits)
	ON_EN_CHANGE(IDC_EDIT_FP_REF_CHAR_HEIGHT, OnEnChangeEditCharHeight)
END_MESSAGE_MAP()


// CDlgFpRefText message handlers

void CDlgFpRefText::OnBnClickedRadioSet()
{
	m_edit_width.EnableWindow( TRUE );
}

void CDlgFpRefText::OnBnClickedRadioDef()
{
	m_edit_width.EnableWindow( FALSE );
}

void CDlgFpRefText::OnCbnSelchangeComboRefTextUnits()
{
	GetFields();
	if( m_combo_units.GetCurSel() == 0 )
		m_units = MIL;
	else
		m_units = MM;
	SetFields();
}

void CDlgFpRefText::OnEnChangeEditCharHeight()
{
	CString str;
	m_edit_height.GetWindowText( str );
	if( m_units == MIL )
		m_height = atof( str ) * NM_PER_MIL;
	else
		m_height = atof( str ) * 1000000.0;
	m_def_width = m_height/10;
	if( m_def_width < 1*NM_PER_MIL )
		m_def_width = 1*NM_PER_MIL;
	if( m_units == MIL )
	{
		MakeCStringFromDouble( &str, m_def_width/NM_PER_MIL );
		m_edit_def_width.SetWindowText( str );
	}
	else
	{
		MakeCStringFromDouble( &str, m_def_width/1000000.0 );
		m_edit_def_width.SetWindowText( str );
	}
}

void CDlgFpRefText::GetFields()
{
	CString str;
	if( m_units == MIL )
	{
		m_edit_height.GetWindowText( str );
		m_height = atof( str ) * NM_PER_MIL;
		m_edit_width.GetWindowText( str );
		m_width = atof( str ) * NM_PER_MIL;
	}
	else
	{
		m_edit_height.GetWindowText( str );
		m_height = atof( str ) * 1000000.0;
		m_edit_width.GetWindowText( str );
		m_width = atof( str ) * 1000000.0;
	}
	m_def_width = m_height/10;
}

void CDlgFpRefText::SetFields()
{
	CString str;
	m_def_width = m_height/10;
	if( m_def_width < 1*NM_PER_MIL )
		m_def_width = 1*NM_PER_MIL;
	if( m_units == MIL )
	{
		MakeCStringFromDouble( &str, m_height/NM_PER_MIL );
		m_edit_height.SetWindowText( str );
		MakeCStringFromDouble( &str, m_width/NM_PER_MIL );
		m_edit_width.SetWindowText( str );
		MakeCStringFromDouble( &str, m_def_width/NM_PER_MIL );
		m_edit_def_width.SetWindowText( str );
	}
	else
	{
		MakeCStringFromDouble( &str, m_height/1000000.0 );
		m_edit_height.SetWindowText( str );
		MakeCStringFromDouble( &str, m_width/1000000.0 );
		m_edit_width.SetWindowText( str );
		MakeCStringFromDouble( &str, m_def_width/1000000.0 );
		m_edit_def_width.SetWindowText( str );
	}
}

