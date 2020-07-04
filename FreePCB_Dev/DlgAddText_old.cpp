// DlgAddText.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgAddText.h"
#include "layers.h"


// CDlgAddText dialog

IMPLEMENT_DYNAMIC(CDlgAddText, CDialog)
CDlgAddText::CDlgAddText(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgAddText::IDD, pParent)
{
}

CDlgAddText::~CDlgAddText()
{
}

void CDlgAddText::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_LAYER, m_layer_list);
	DDX_Control(pDX, IDC_EDIT_HEIGHT, m_height);
	DDX_Control(pDX, IDC_SET_WIDTH, m_button_set_width);
	DDX_Control(pDX, IDC_DEF_WIDTH, m_button_def_width);
	DDX_Control(pDX, IDC_EDIT_WIDTH, m_width);
	DDX_Control(pDX, IDC_EDIT_TEXT, m_text);
	DDX_Control(pDX, IDC_Y, m_edit_y);
	DDX_Control(pDX, IDC_X, m_edit_x);
	DDX_Control(pDX, IDC_LIST_ANGLE, m_list_angle);
}


BEGIN_MESSAGE_MAP(CDlgAddText, CDialog)
	ON_BN_CLICKED(IDC_SET_WIDTH, OnBnClickedSetWidth)
	ON_BN_CLICKED(IDC_DEF_WIDTH, OnBnClickedDefWidth)
	ON_EN_CHANGE(IDC_EDIT_HEIGHT, OnEnChangeEditHeight)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()


// CDlgAddText message handlers

BOOL CDlgAddText::OnInitDialog()
{
	CDialog::OnInitDialog();
	m_layer_list.InsertString( -1, layer_str[LAY_SILK_TOP] );
	m_layer_list.InsertString( -1, layer_str[LAY_SILK_BOTTOM] );
	for( int i=LAY_TOP_COPPER; i<m_num_layers; i++ )
		m_layer_list.InsertString( -1, layer_str[i] );
	m_layer_list.SetCurSel( 0 );
	m_height.SetWindowText( "100" );
	m_width.SetWindowText( "10" );
	m_button_set_width.SetCheck( 0 );
	m_button_def_width.SetCheck( 1 );
	m_width.EnableWindow( 0 );
	m_list_angle.InsertString( -1, "0" );
	m_list_angle.InsertString( -1, "90" );
	m_list_angle.InsertString( -1, "180" );
	m_list_angle.InsertString( -1, "270" );
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgAddText::OnBnClickedSetWidth()
{
	m_button_set_width.SetCheck( 1 );
	m_button_def_width.SetCheck( 0 );
	m_width.EnableWindow( 1 );
}

void CDlgAddText::OnBnClickedDefWidth()
{
	m_button_set_width.SetCheck( 0 );
	m_button_def_width.SetCheck( 1 );
	m_width.EnableWindow( 0 );
}

void CDlgAddText::OnEnChangeEditHeight()
{
	if( m_button_def_width.GetCheck() )
	{
		char str[80];
		m_height.GetWindowText( str, 20 );
		double h = atof( str  );
		if( h != 0.0 )
		{
			CString w_str;
			int w = (int)h/10;
			if( w < 5 )
				w = 5;
			w_str.Format( "%d", w );
			m_width.SetWindowText( (LPCTSTR)w_str );
		}
	}
}

void CDlgAddText::OnBnClickedOk()
{
	// check and set output variables
	char str[255];
	m_height.GetWindowText( str, 80 );
	m_output_height = atoi( str );
	if( m_output_height <=0 || m_output_height > 1000 )
	{
		CString mess;
		mess.Format( "Illegal height: %s", str );
		AfxMessageBox( mess );
		return;
	}
	m_width.GetWindowText( str, 80 );
	m_output_width = atoi( str );
	if( m_output_width <=0 || m_output_width > 1000 )
	{
		CString mess;
		mess.Format( "Illegal width: %s", str );
		AfxMessageBox( mess );
		return;
	}
	int layer = m_layer_list.GetCurSel();
	if( layer == 0 )
		m_output_layer = LAY_SILK_TOP;
	else if( layer == 1 )
		m_output_layer = LAY_SILK_BOTTOM;
	else
		m_output_layer = layer - 2 + LAY_TOP_COPPER;
	m_text.GetWindowText( str, 255 );
	m_output_str = str;
	OnOK();
}
