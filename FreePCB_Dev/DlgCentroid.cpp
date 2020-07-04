// DlgCentroid.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgCentroid.h"


// CDlgCentroid dialog

IMPLEMENT_DYNAMIC(CDlgCentroid, CDialog)
CDlgCentroid::CDlgCentroid(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgCentroid::IDD, pParent)
{
}

CDlgCentroid::~CDlgCentroid()
{
}

void CDlgCentroid::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_RADIO3, m_button_default);
	DDX_Control(pDX, IDC_RADIO2, m_button_set);
	DDX_Control(pDX, IDC_COMBO1, m_combo_units);
	DDX_Control(pDX, IDC_EDIT1, m_edit_x);
	DDX_Control(pDX, IDC_EDIT2, m_edit_y);
	DDX_Control(pDX, IDC_COMBO_ANGLE, m_combo_angle);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		m_combo_units.InsertString( 0, "MIL" );
		m_combo_units.InsertString( 1, "MM" );
		m_combo_angle.InsertString( 0, "0" );
		m_combo_angle.InsertString( 1, "90" );
		m_combo_angle.InsertString( 2, "180" );
		m_combo_angle.InsertString( 3, "270" );
		SetFields();
	}
	else
	{
		// outgoing
		GetFields();
	}
}


BEGIN_MESSAGE_MAP(CDlgCentroid, CDialog)
	ON_BN_CLICKED(IDC_RADIO3, OnBnClickedDefault)
	ON_BN_CLICKED(IDC_RADIO2, OnBnClickedSet)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnCbnSelChangeCombo1)
END_MESSAGE_MAP()

void CDlgCentroid::Initialize( CENTROID_TYPE type, int units, int x, int y, int angle )
{
	m_type = type;
	m_units = units;
	m_x = x;
	m_y = y;
	m_angle = angle;
}

void CDlgCentroid::SetFields()
{
	CString str;
	if( m_type == CENTROID_DEFAULT )
	{
		m_button_default.SetCheck( 1 );
		m_button_set.SetCheck( 0 );
		m_combo_units.EnableWindow( FALSE );
		m_edit_x.EnableWindow( FALSE );
		m_edit_y.EnableWindow( FALSE );
	}
	else
	{
		m_button_default.SetCheck( 0 );
		m_button_set.SetCheck( 1 );
		if( m_units == MIL ) 
			m_combo_units.SetCurSel( 0 );
		else
			m_combo_units.SetCurSel( 1 );
		m_combo_units.EnableWindow( TRUE );
		m_edit_x.EnableWindow( TRUE );
		m_edit_y.EnableWindow( TRUE );
	}
	m_combo_angle.SetCurSel( m_angle/90 );
	::MakeCStringFromDimension( &str, m_x, m_units, FALSE, FALSE, FALSE, 3 );
	m_edit_x.SetWindowText( str );
	::MakeCStringFromDimension( &str, m_y, m_units, FALSE, FALSE, FALSE, 3 );
	m_edit_y.SetWindowText( str );
}

void CDlgCentroid::GetFields()
{
	if( m_button_default.GetCheck() )
		m_type = CENTROID_DEFAULT;
	else
		m_type = CENTROID_DEFINED;
	CString str;
	m_edit_x.GetWindowText( str );
	m_x = ::GetDimensionFromString( &str, m_units );
	m_edit_y.GetWindowText( str );
	m_y = ::GetDimensionFromString( &str, m_units );
	m_angle = 90 * m_combo_angle.GetCurSel();
}


// CDlgCentroid message handlers

void CDlgCentroid::OnBnClickedDefault()
{
	m_button_set.SetCheck(0);
	GetFields();
	SetFields();
}

void CDlgCentroid::OnBnClickedSet()
{
	m_button_default.SetCheck(0); 
	GetFields();
	SetFields();
}

void CDlgCentroid::OnCbnSelChangeCombo1()
{
	GetFields();
	int m_sel_units = m_combo_units.GetCurSel();
	if( m_sel_units == 0 )
		m_units = MIL;
	else
		m_units = MM;
	SetFields();
}
