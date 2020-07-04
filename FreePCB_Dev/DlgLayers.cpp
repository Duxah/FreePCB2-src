// DlgLayers.cpp : implementation file
//
// uses global arrays in layers.h
//
#include "stdafx.h"
#include "FreePcb.h"
#include "DlgLayers.h"
#include "layers.h"
#include ".\dlglayers.h"


// CDlgLayers dialog

IMPLEMENT_DYNAMIC(CDlgLayers, CDialog)
CDlgLayers::CDlgLayers(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgLayers::IDD, pParent)
{
}

CDlgLayers::~CDlgLayers()
{
}

void CDlgLayers::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	if( !pDX->m_bSaveAndValidate )
	{
		// on entry
		for( int i=m_nlayers; i<NUM_DLG_LAYERS; i++ )
			GetDlgItem( IDC_CHECK_LAYER_3+i-2 )->EnableWindow( 0 );
	}
	for( int i=0; i<NUM_DLG_LAYERS; i++ )
	{
		if( i>1 )
			DDX_Check( pDX, IDC_CHECK_LAYER_3+i-2, m_vis[i] );
	}
	DDX_Check( pDX, IDC_CHECK_LAYER_SYS_CLRS, m_sys_clrs );
	if( pDX->m_bSaveAndValidate )
	{
		// on OK
		for( int i=0; i<NUM_DLG_LAYERS; i++ )
		{
			m_rgb_ptr[i*3] = m_rgb[i][0];
			m_rgb_ptr[i*3+1] = m_rgb[i][1];
			m_rgb_ptr[i*3+2] = m_rgb[i][2];
		}
	}
}

BEGIN_MESSAGE_MAP(CDlgLayers, CDialog)
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_BUTTON_LAYER_1, OnBnClickedButtonLayer1)
	ON_BN_CLICKED(IDC_BUTTON_LAYER_2, OnBnClickedButtonLayer2)
	ON_BN_CLICKED(IDC_BUTTON_LAYER_3, OnBnClickedButtonLayer3)
	ON_BN_CLICKED(IDC_BUTTON_LAYER_4, OnBnClickedButtonLayer4)
	ON_BN_CLICKED(IDC_BUTTON_LAYER_5, OnBnClickedButtonLayer5)
	ON_BN_CLICKED(IDC_BUTTON_LAYER_6, OnBnClickedButtonLayer6)
	ON_BN_CLICKED(IDC_BUTTON_LAYER_7, OnBnClickedButtonLayer7)
	ON_BN_CLICKED(IDC_BUTTON_LAYER_8, OnBnClickedButtonLayer8)
	ON_BN_CLICKED(IDC_BUTTON_LAYER_9, OnBnClickedButtonLayer9)
	ON_BN_CLICKED(IDC_BUTTON_LAYER_10, OnBnClickedButtonLayer10)
	ON_BN_CLICKED(IDC_BUTTON_LAYER_11, OnBnClickedButtonLayer11)
	ON_BN_CLICKED(IDC_BUTTON_LAYER_12, OnBnClickedButtonLayer12)
	ON_BN_CLICKED(IDC_BUTTON_LAYER_13, OnBnClickedButtonLayer13)
	ON_BN_CLICKED(IDC_BUTTON_LAYER_14, OnBnClickedButtonLayer14)
	ON_BN_CLICKED(IDC_BUTTON_LAYER_15, OnBnClickedButtonLayer15)
	ON_BN_CLICKED(IDC_BUTTON_LAYER_16, OnBnClickedButtonLayer16)
	ON_BN_CLICKED(IDC_BUTTON_LAYER_17, OnBnClickedButtonLayer17)
	ON_BN_CLICKED(IDC_BUTTON_LAYER_18, OnBnClickedButtonLayer18)
	ON_BN_CLICKED(IDC_BUTTON_LAYER_19, OnBnClickedButtonLayer19)
	ON_BN_CLICKED(IDC_BUTTON_LAYER_20, OnBnClickedButtonLayer20)
	ON_BN_CLICKED(IDC_BUTTON_LAYER_21, OnBnClickedButtonLayer21)
	ON_BN_CLICKED(IDC_BUTTON_LAYER_22, OnBnClickedButtonLayer22)
	ON_BN_CLICKED(IDC_BUTTON_LAYER_23, OnBnClickedButtonLayer23)
	ON_BN_CLICKED(IDC_BUTTON_LAYER_24, OnBnClickedButtonLayer24)
	ON_BN_CLICKED(IDC_BUTTON_LAYER_25, OnBnClickedButtonLayer25)
	ON_BN_CLICKED(IDC_BUTTON_LAYER_26, OnBnClickedButtonLayer26)
	ON_BN_CLICKED(IDC_BUTTON_LAYER_27, OnBnClickedButtonLayer27)
	ON_BN_CLICKED(IDC_BUTTON_LAYER_28, OnBnClickedButtonLayer28)
	ON_BN_CLICKED(IDC_BUTTON_LAYER_29, OnBnClickedButtonLayer29)
	ON_BN_CLICKED(IDC_BUTTON_LAYER_30, OnBnClickedButtonLayer30)
	ON_BN_CLICKED(IDC_BUTTON_LAYER_31, OnBnClickedButtonLayer31)
END_MESSAGE_MAP()

void CDlgLayers::Initialize( int nlayers, int vis[], int rgb[][3], int system_colors )
{
	m_sys_clrs = system_colors;
	m_nlayers = nlayers;
	m_vis = vis;
	m_rgb_ptr = (int*)rgb;
	for( int i=0; i<NUM_DLG_LAYERS; i++ )
	{
		m_rgb[i][0] = m_rgb_ptr[i*3];
		m_rgb[i][1] = m_rgb_ptr[i*3+1];
		m_rgb[i][2] = m_rgb_ptr[i*3+2];
	}
}


// CDlgLayers message handlers
//

// set color boxes
//
HBRUSH CDlgLayers::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	// TODO:  Change any attributes of the DC here
	// TODO:  Return a different brush if the default is not desired
	if ( pWnd->GetDlgCtrlID() >= IDC_STATIC_LAYER_1 && pWnd->GetDlgCtrlID() <= (IDC_STATIC_LAYER_1+NUM_DLG_LAYERS-1) )
	{
		int i = pWnd->GetDlgCtrlID() - IDC_STATIC_LAYER_1;
		// Set layer color
		pDC->SetTextColor(RGB(m_rgb[i][0], m_rgb[i][1], m_rgb[i][2]));
		pDC->SetBkMode(TRANSPARENT);
		m_brush.DeleteObject();
		m_brush.CreateSolidBrush( RGB(m_rgb[i][0], m_rgb[i][1], m_rgb[i][2]) );
		hbr = m_brush;
	}
	return hbr;
}

// handle edit button clicks
//
void CDlgLayers::OnBnClickedButtonLayer1() { EditColor( 0 ); }
void CDlgLayers::OnBnClickedButtonLayer2() { EditColor( 1 ); }
void CDlgLayers::OnBnClickedButtonLayer3() { EditColor( 2 ); }
void CDlgLayers::OnBnClickedButtonLayer4() { EditColor( 3 ); }
void CDlgLayers::OnBnClickedButtonLayer5() { EditColor( 4 ); }
void CDlgLayers::OnBnClickedButtonLayer6() { EditColor( 5 ); }
void CDlgLayers::OnBnClickedButtonLayer7() { EditColor( 6 ); }
void CDlgLayers::OnBnClickedButtonLayer8() { EditColor( 7 ); }
void CDlgLayers::OnBnClickedButtonLayer9() { EditColor( 8 ); }
void CDlgLayers::OnBnClickedButtonLayer10() { EditColor( 9 ); }
void CDlgLayers::OnBnClickedButtonLayer11() { EditColor( 10 ); }
void CDlgLayers::OnBnClickedButtonLayer12() { EditColor( 11 ); }
void CDlgLayers::OnBnClickedButtonLayer13() { EditColor( 12 ); }
void CDlgLayers::OnBnClickedButtonLayer14() { EditColor( 13 ); }
void CDlgLayers::OnBnClickedButtonLayer15() { EditColor( 14 ); }
void CDlgLayers::OnBnClickedButtonLayer16() { EditColor( 15 ); }
void CDlgLayers::OnBnClickedButtonLayer17() { EditColor( 16 ); }
void CDlgLayers::OnBnClickedButtonLayer18() { EditColor( 17 ); }
void CDlgLayers::OnBnClickedButtonLayer19() { EditColor( 18 ); }
void CDlgLayers::OnBnClickedButtonLayer20() { EditColor( 19 ); }
void CDlgLayers::OnBnClickedButtonLayer21() { EditColor( 20 ); }
void CDlgLayers::OnBnClickedButtonLayer22() { EditColor( 21 ); }
void CDlgLayers::OnBnClickedButtonLayer23() { EditColor( 22 ); }
void CDlgLayers::OnBnClickedButtonLayer24() { EditColor( 23 ); }
void CDlgLayers::OnBnClickedButtonLayer25() { EditColor( 24 ); }
void CDlgLayers::OnBnClickedButtonLayer26() { EditColor( 25 ); }
void CDlgLayers::OnBnClickedButtonLayer27() { EditColor( 26 ); }
void CDlgLayers::OnBnClickedButtonLayer28() { EditColor( 27 ); }
void CDlgLayers::OnBnClickedButtonLayer29() { EditColor( 28 ); }
void CDlgLayers::OnBnClickedButtonLayer30() { EditColor( 29 ); }
void CDlgLayers::OnBnClickedButtonLayer31() { EditColor( 30 ); }

// edit color for selected layer
//
void CDlgLayers::EditColor( int layer )
{
	CColorDialog dlg( RGB(m_rgb[layer][0], m_rgb[layer][1], m_rgb[layer][2]), 
		CC_FULLOPEN | CC_ANYCOLOR );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		COLORREF color = dlg.GetColor();
		m_rgb[layer][0] = GetRValue( color );
		m_rgb[layer][1] = GetGValue( color );
		m_rgb[layer][2] = GetBValue( color );
		Invalidate( FALSE );
	}
}

