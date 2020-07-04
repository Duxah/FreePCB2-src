// DlgAddText.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgFpText.h"
#include "layers.h"

int gFpLastHeight = 100*NM_PER_MIL;
int gFpLastWidth = 10*NM_PER_MIL;
int gFpUseDefaultWidth = TRUE;

// CDlgFpText dialog

IMPLEMENT_DYNAMIC(CDlgFpText, CDialog)
CDlgFpText::CDlgFpText(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgFpText::IDD, pParent)
{
	m_x = 0;
	m_y = 0;
}

CDlgFpText::~CDlgFpText()
{
}

void CDlgFpText::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_HEIGHT, m_edit_height);
	DDX_Control(pDX, IDC_SET_WIDTH, m_button_set_width);
	DDX_Control(pDX, IDC_DEF_WIDTH, m_button_def_width);
	DDX_Control(pDX, IDC_EDIT_WIDTH, m_edit_width);
	DDX_Control(pDX, IDC_EDIT_TEXT, m_text);
	DDX_Control(pDX, IDC_Y, m_edit_y);
	DDX_Control(pDX, IDC_X, m_edit_x);
	DDX_Text( pDX, IDC_EDIT_TEXT, m_str );
	DDX_Control(pDX, IDC_LIST_ANGLE, m_list_angle);
	DDX_Control(pDX, IDC_RADIO1, m_button_drag);
	DDX_Control(pDX, IDC_RADIO2, m_button_set_position );
	DDX_Control(pDX, IDC_COMBO_ADD_TEXT_UNITS, m_combo_units);
	if( pDX->m_bSaveAndValidate )
	{
		// leaving the dialog
		if( m_str.GetLength() == 0 )
		{
			AfxMessageBox( "Invalid text string" );
			pDX->Fail();
		}
		if( m_str.Find( '\"' ) != -1 )
		{
			AfxMessageBox( "Text string can't contain \"" );
			pDX->Fail();
		}
		GetFields();
		int ia = m_list_angle.GetCurSel();
		CString str_a;
		m_list_angle.GetText(ia,str_a);
		m_angle = my_atoi(&str_a);
		if( m_bNewText )
		{
			gFpLastHeight = m_height;
			gFpLastWidth = m_width;
			gFpUseDefaultWidth = m_button_def_width.GetCheck();
		}
	}
}


BEGIN_MESSAGE_MAP(CDlgFpText, CDialog)
	ON_BN_CLICKED(IDC_SET_WIDTH, OnBnClickedSetWidth)
	ON_BN_CLICKED(IDC_DEF_WIDTH, OnBnClickedDefWidth)
	ON_EN_CHANGE(IDC_EDIT_HEIGHT, OnEnChangeEditHeight)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDC_RADIO2, OnBnClickedSetPosition)
	ON_BN_CLICKED(IDC_RADIO1, OnBnClickedDrag)
	ON_CBN_SELCHANGE(IDC_COMBO_ADD_TEXT_UNITS, OnCbnSelchangeComboAddTextUnits)
END_MESSAGE_MAP()

// Initialize dialog
//
void CDlgFpText::Initialize( BOOL bDrag, BOOL bFixedString, 
		CString * str, int units, 
		int angle, int height, int width, int x, int y )

{
	m_bDrag = bDrag;
	m_bFixedString = bFixedString;
	m_bNewText = (bDrag && !bFixedString && !str);
	if( str )
		m_str = *str;
	else
		m_str = "";
	m_units = units;
	m_angle = angle;
	m_height = height;
	m_width = width;
	m_x = x;
	m_y = y;
}

// CDlgFpText message handlers

BOOL CDlgFpText::OnInitDialog()
{
	CDialog::OnInitDialog();

	// units
	m_combo_units.InsertString(	0, "MIL" );
	m_combo_units.InsertString(	1, "MM"	);
	if(	m_units	== MIL )
	{
		m_combo_units.SetCurSel(0);
		m_unit_mult = NM_PER_MIL;
	}
	else
	{
		m_combo_units.SetCurSel(1);
		m_unit_mult = 1000000;
	}

	// height and width
	if( m_bNewText )
	{
		m_height = gFpLastHeight;
		m_width = gFpLastWidth;
	}

	// angles
	m_list_angle.InsertString( -1, "0" );
	m_list_angle.InsertString( -1, "90" );
	m_list_angle.InsertString( -1, "180" );
	m_list_angle.InsertString( -1, "270" );
	if( m_angle == 0 )
		m_list_angle.SetCurSel( 0 );
	else if( m_angle == 90 )
		m_list_angle.SetCurSel( 1 );
	else if( m_angle == 180 )
		m_list_angle.SetCurSel( 2 );
	else 
		m_list_angle.SetCurSel( 3 );
	if( m_angle%90 )
	{
		CString str_a;
		::MakeCStringFromDimension( &str_a, m_angle, NM, 0,0,0 );
		m_list_angle.InsertString( -1, str_a );
		m_list_angle.SetCurSel( 4 );
	}	
	if( !m_bNewText )
	{
		// editing, so set from variables
		m_button_set_position.SetCheck( 1 );
		m_button_drag.SetCheck( 0 );
		m_button_set_width.SetCheck( 1 );
		m_button_def_width.SetCheck( 0 );
		m_edit_x.EnableWindow( 1 );
		m_edit_y.EnableWindow( 1 );
		m_edit_width.EnableWindow( 1 );
	}
	else
	{
		// adding new text
		m_button_drag.SetCheck( 1 );
		m_list_angle.SetCurSel( 0 );
		m_button_def_width.SetCheck( gFpUseDefaultWidth );
		m_button_set_width.SetCheck( !gFpUseDefaultWidth );
		m_edit_x.EnableWindow( 0 );
		m_edit_y.EnableWindow( 0 );
		m_list_angle.EnableWindow( 0 );
		m_edit_width.EnableWindow( !gFpUseDefaultWidth );
	}
	if( m_bFixedString )
		m_text.EnableWindow( FALSE );
	SetFields();
	return TRUE;  // return TRUE unless you set the focus to a control
}

void CDlgFpText::OnBnClickedSetWidth()
{
	m_button_set_width.SetCheck( 1 );
	m_button_def_width.SetCheck( 0 );
	m_edit_width.EnableWindow( 1 );
}

void CDlgFpText::OnBnClickedDefWidth()
{
	OnEnChangeEditHeight();
	m_button_set_width.SetCheck( 0 );
	m_button_def_width.SetCheck( 1 );
	m_edit_width.EnableWindow( 0 );
}

void CDlgFpText::OnEnChangeEditHeight()
{
	if( m_button_def_width.GetCheck() )
	{
		CString str;
		m_edit_height.GetWindowText( str );
		m_height = atof(str)*m_unit_mult;
		m_width = m_height/10;
		m_width = max( m_width, 1*NM_PER_MIL );
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
}

void CDlgFpText::OnBnClickedOk()
{
	OnOK();
}

void CDlgFpText::OnBnClickedSetPosition()
{
	m_edit_x.EnableWindow( 1 );
	m_edit_y.EnableWindow( 1 );
	m_list_angle.EnableWindow( 1 );
	m_bDrag = 0;
}

void CDlgFpText::OnBnClickedDrag()
{
	m_edit_x.EnableWindow( 0 );
	m_edit_y.EnableWindow( 0 );
	m_list_angle.EnableWindow( 0 );
	m_bDrag = 1;
}

void CDlgFpText::OnCbnSelchangeComboAddTextUnits()
{
	GetFields();
	if( m_combo_units.GetCurSel() == 0 )
	{
		m_units = MIL;
		m_unit_mult = NM_PER_MIL;
	}
	else
	{
		m_units = MM;
		m_unit_mult = 1000000;
	}
	SetFields();
}

void CDlgFpText::GetFields()
{
	CString str;
	double mult;
	if( m_units == MIL )
		mult = NM_PER_MIL;
	else
		mult = 1000000.0;
	m_edit_height.GetWindowText( str );
	m_height = atof( str ) * mult;
	m_edit_width.GetWindowText( str );
	m_width = atof( str ) * mult;
	m_edit_x.GetWindowText( str );
	m_x = atof( str ) * mult;
	m_edit_y.GetWindowText( str );
	m_y = atof( str ) * mult;
}

void CDlgFpText::SetFields()
{
	CString str;
	double mult;
	if( m_units == MIL )
		mult = NM_PER_MIL;
	else
		mult = 1000000.0;
	MakeCStringFromDouble( &str, m_height/mult );
	m_edit_height.SetWindowText( str );
	MakeCStringFromDouble( &str, m_width/mult );
	m_edit_width.SetWindowText( str );
	MakeCStringFromDouble( &str, m_x/mult );
	m_edit_x.SetWindowText( str );
	MakeCStringFromDouble( &str, m_y/mult );
	m_edit_y.SetWindowText( str );
}


