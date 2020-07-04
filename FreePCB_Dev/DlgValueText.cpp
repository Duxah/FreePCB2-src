// DlgValueText.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "PartList.h"
#include "DlgValueText.h"


// CDlgValueText dialog

IMPLEMENT_DYNAMIC(CDlgValueText, CDialog)
CDlgValueText::CDlgValueText(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgValueText::IDD, pParent)
{
}

CDlgValueText::~CDlgValueText()
{
}

void CDlgValueText::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_REF_ID, m_edit_ref_des);
	DDX_Control(pDX, IDC_EDIT_FOOTPRINT, m_edit_value);
	DDX_Control(pDX, IDC_EDIT_CHAR_HEIGHT, m_edit_height);
	DDX_Control(pDX, IDC_COMBO_REF_TEXT_UNITS, m_combo_units);
	DDX_Control(pDX, IDC_RADIO_SET, m_radio_set);
	DDX_Control(pDX, IDC_RADIO_DEF, m_radio_def);
	DDX_Control(pDX, IDC_EDIT_WIDTH, m_edit_width);
	DDX_Control(pDX, IDC_EDIT_DEF_WIDTH, m_edit_def_width);
	DDX_Control(pDX, IDC_CHECK_VISIBLE, m_check_visible);
	DDX_Control(pDX, IDC_CHECK_DEF_POS, m_check_def_pos);
	if( !pDX->m_bSaveAndValidate )
	{
		// entry
		m_edit_ref_des.EnableWindow( FALSE );
		m_edit_ref_des.SetWindowText( m_part->ref_des );
		m_edit_value.EnableWindow( FALSE );
		m_edit_value.SetWindowText( m_part->value );
		m_combo_units.InsertString( 0, "MIL" );
		m_combo_units.InsertString( 1, "MM" );
		if( m_units == MIL )
			m_combo_units.SetCurSel( 0 );
		else
			m_combo_units.SetCurSel( 1 );
		if( m_width == m_height/10 )
		{
			m_radio_set.SetCheck( FALSE );
			m_radio_def.SetCheck( TRUE );
			m_edit_def_width.EnableWindow( FALSE );
			m_edit_width.EnableWindow( FALSE );
		}
		else
		{
			m_radio_set.SetCheck( TRUE );
			m_radio_def.SetCheck( FALSE );
			m_edit_def_width.EnableWindow( FALSE );
			m_edit_width.EnableWindow( TRUE );
		}
		SetFields();
	}
	else
	{
		// exit
		GetFields();
		if( m_radio_def.GetCheck() )
			m_width = m_def_width;
	}
}

// the calling program should call this to set up dialog
// and provide pointers to variables which will be modified
//
void CDlgValueText::Initialize( CPartList * plist, cpart * part )
{
	m_plist = plist;
	m_part = part;
	m_vis = part->m_value_vis;
	m_def_pos = 0;
	m_units = part->shape->m_units;
	m_width = m_part->m_value_w;
	m_height = m_part->m_value_size;
	m_def_width = m_height/10;
}

BEGIN_MESSAGE_MAP(CDlgValueText, CDialog)
	ON_BN_CLICKED(IDC_RADIO_SET, OnBnClickedRadioSet)
	ON_EN_CHANGE(IDC_EDIT_CHAR_HEIGHT, OnEnChangeEditCharHeight)
	ON_BN_CLICKED(IDC_RADIO_DEF, OnBnClickedRadioDef)
	ON_CBN_SELCHANGE(IDC_COMBO_REF_TEXT_UNITS, OnCbnSelchangeComboRefTextUnits)
END_MESSAGE_MAP()


// CDlgValueText message handlers

void CDlgValueText::OnBnClickedRadioSet()
{
	m_edit_width.EnableWindow( TRUE );
}

void CDlgValueText::OnBnClickedRadioDef()
{
	m_edit_width.EnableWindow( FALSE );

}

void CDlgValueText::OnEnChangeEditCharHeight()
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

void CDlgValueText::GetFields()
{
	CString str;
	m_vis = m_check_visible.GetCheck();
	m_def_pos = m_check_def_pos.GetCheck();
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

void CDlgValueText::SetFields()
{
	CString str;
	m_check_visible.SetCheck( m_vis );
	m_check_def_pos.SetCheck(m_def_pos);
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

void CDlgValueText::OnCbnSelchangeComboRefTextUnits()
{
	GetFields();
	if( m_combo_units.GetCurSel() == 0 )
		m_units = MIL;
	else
		m_units = MM;
	SetFields();
}
