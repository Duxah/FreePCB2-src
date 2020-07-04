// DlgSetSegmentWidth.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgSetSegmentWidth.h"
#include ".\dlgsetsegmentwidth.h"

// DlgSetSegmentWidth dialog

IMPLEMENT_DYNAMIC(DlgSetSegmentWidth, CDialog)
DlgSetSegmentWidth::DlgSetSegmentWidth(CWnd* pParent /*=NULL*/)
	: CDialog(DlgSetSegmentWidth::IDD, pParent)
{
	m_w = 0;
	m_v_w = 0;
	m_v_h_w = 0;
	m_tv = 1;
	m_mode = 0;
	m_def = 0;
	m_apply = 0;
	m_init_w = 0;
	m_init_via_w = 0;
	m_init_via_hole_w = 0;
	m_units = MIL;
}

DlgSetSegmentWidth::~DlgSetSegmentWidth()
{
}

void DlgSetSegmentWidth::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_CBString( pDX, IDC_COMBO_WIDTH, m_width_str );
	DDX_CBString( pDX, IDC_EDIT_VIA_PAD, m_via_w_str );
	DDX_CBString( pDX, IDC_EDIT_VIA_HOLE, m_via_hole_w_str );
	DDX_Control(pDX, IDC_DEF_NET, m_def_net);
	DDX_Control(pDX, IDC_APPLY_NET, m_apply_net);
	DDX_Control(pDX, IDC_APPLY_CON, m_apply_con);
	DDX_Control(pDX, IDC_APPLY_SEG, m_apply_seg);
	DDX_Control(pDX, IDC_COMBO_WIDTH, m_width_box);
	DDX_Control(pDX, IDC_EDIT_VIA_PAD, m_via_w);
	DDX_Control(pDX, IDC_EDIT_VIA_HOLE, m_via_hole_w);
	DDX_Control(pDX, IDC_RADIO_SET_VIA, rb_set_via);
	DDX_Control(pDX, IDC_RADIO_DEF_VIA, rb_def_via);
	DDX_Control(pDX, IDC_RADIO_MOD_TV, m_radio_mod_tv);
	DDX_Control(pDX, IDC_RADIO_MOD_T, m_radio_mod_t);
	DDX_Control(pDX, IDC_RADIO_MOD_V, m_radio_mod_v);
	if( !pDX->m_bSaveAndValidate )
	{
		// entering dialog
	}
	else
	{
		// exiting dialog
		m_width = my_atof( &m_width_str );
		m_via_width = my_atof( &m_via_w_str );
		m_hole_width = my_atof( &m_via_hole_w_str );
		if( !(m_tv == 3 || m_width > 0) )
		{
			AfxMessageBox( "illegal trace width" );
			pDX->Fail();
		}
		if( m_tv != 2 && rb_set_via.GetCheck() != 0 )
		{
			if( m_via_width <= 0 )
			{
				AfxMessageBox( "illegal via width" );
				pDX->Fail();
			}
			if( m_hole_width <= 0 )
			{
				AfxMessageBox( "illegal hole width" );
				pDX->Fail();
			}
		}
		// decode buttons
		if( m_def_net.GetCheck() )
			m_def = 2;
		else
			m_def = 0;
		if( m_apply_net.GetCheck() )
			m_apply = 3;
		else if( m_apply_con.GetCheck() )
			m_apply = 2;
		else if( m_apply_seg.GetCheck() )
			m_apply = 1;
		else
			m_apply = 0;
	}
}


BEGIN_MESSAGE_MAP(DlgSetSegmentWidth, CDialog)
	ON_BN_CLICKED(IDC_DEF_NET, OnBnClickedDefNet)
	ON_BN_CLICKED(IDC_APPLY_NET, OnBnClickedApplyNet)
	ON_BN_CLICKED(IDC_APPLY_CON, OnBnClickedApplyCon)
	ON_BN_CLICKED(IDC_APPLY_SEG, OnBnClickedApplySeg)
	ON_BN_CLICKED(IDC_RADIO_DEF_VIA, OnBnClickedRadioDefVia)
	ON_BN_CLICKED(IDC_RADIO_SET_VIA, OnBnClickedRadioSetVia)
	ON_CBN_SELCHANGE(IDC_COMBO_WIDTH, OnCbnSelchangeComboWidth)
	ON_CBN_EDITCHANGE(IDC_COMBO_WIDTH, OnCbnEditchangeComboWidth)
	ON_BN_CLICKED(IDC_RADIO_MOD_TV, OnBnClickedRadioModTv)
	ON_BN_CLICKED(IDC_RADIO_MOD_T, OnBnClickedRadioModT)
	ON_BN_CLICKED(IDC_RADIO_MOD_V, OnBnClickedRadioModV)
END_MESSAGE_MAP()

// DlgSetSegmentWidth message handlers

void DlgSetSegmentWidth::OnBnClickedDefNet()
{
}

void DlgSetSegmentWidth::OnBnClickedApplyNet()
{
	if( m_apply_net.GetCheck() )
	{
		m_apply_con.SetCheck( 0 );
		m_apply_seg.SetCheck( 0 );
	}
}

void DlgSetSegmentWidth::OnBnClickedApplyCon()
{
	if( m_apply_con.GetCheck() )
	{
		m_apply_net.SetCheck( 0 );
		m_apply_seg.SetCheck( 0 );
	}
}

void DlgSetSegmentWidth::OnBnClickedApplySeg()
{
	if( m_apply_seg.GetCheck() )
	{
		m_apply_net.SetCheck( 0 );
		m_apply_con.SetCheck( 0 );
	}
}

BOOL DlgSetSegmentWidth::OnInitDialog()
{
	CString w_str;

	CDialog::OnInitDialog();
	if( m_w )
	{
		for( int iw=0; iw<m_w->GetSize(); iw++ )
		{
			::MakeCStringFromDimension( &w_str, abs((*m_w)[iw]), m_units, TRUE, TRUE, FALSE, (m_units==MIL?0:3) );
			m_width_box.InsertString( iw, w_str );
		}
	}
	if( m_mode == 0 ) 
	{
		// called from segment
		m_apply_seg.SetCheck( 1 );
	}
	else if( m_mode == 1 )
	{
		// called from trace, or ratline trace segment
		m_apply_seg.EnableWindow( 0 );
		m_apply_con.SetCheck( 1 );
		m_def_net.SetCheck( 0 );
	}
	else if( m_mode == 2 )
	{
		// called from net, or ratline connection
		m_apply_seg.EnableWindow( 0 );
		m_apply_con.EnableWindow( 0 );
		m_apply_net.SetCheck( 1 );
		m_def_net.SetCheck( 1 );
	}
	rb_def_via.SetCheck( 1 );
	m_via_w.EnableWindow( 0 );
	m_via_hole_w.EnableWindow( 0 );
	if( m_init_w )
	{
		::MakeCStringFromDimension( &w_str, abs(m_init_w), m_units, TRUE, TRUE, FALSE, (m_units==MIL?0:3) );
		m_width_box.SetWindowText( w_str );
	}
	if( m_init_via_w )
	{
		::MakeCStringFromDimension( &w_str, abs(m_init_via_w), m_units, TRUE, TRUE, FALSE, (m_units==MIL?0:3) );
		m_via_w.SetWindowText( w_str );
	}
	if( m_init_via_hole_w )
	{
		::MakeCStringFromDimension( &w_str, abs(m_init_via_hole_w), m_units, TRUE, TRUE, FALSE, (m_units==MIL?0:3) );
		m_via_hole_w.SetWindowText( w_str );
	}
	m_radio_mod_tv.SetCheck(1);
	OnBnClickedRadioDefVia();
	return TRUE;
}

void DlgSetSegmentWidth::OnBnClickedRadioDefVia()
{
	rb_set_via.SetCheck( 1-rb_def_via.GetCheck() );
	m_via_w.EnableWindow( 0 );
	m_via_hole_w.EnableWindow( 0 );
	if( rb_def_via.GetCheck() )
	{
		CString test;
		m_width_box.GetWindowTextA( test );
		int i = m_width_box.FindString( 0, test );
		if( i >= 0 )
		{
			int v_w = (*m_v_w)[i];
			int v_h_w = (*m_v_h_w)[i];
			::MakeCStringFromDimension( &test, abs(v_w), m_units, TRUE, TRUE, FALSE, (m_units==MIL?0:3) );
			m_via_w.SetWindowText( test );
			::MakeCStringFromDimension( &test, abs(v_h_w), m_units, TRUE, TRUE, FALSE, (m_units==MIL?0:3) );
			m_via_hole_w.SetWindowText( test );
		}
	}
}

void DlgSetSegmentWidth::OnBnClickedRadioSetVia()
{
	rb_def_via.SetCheck( 1-rb_set_via.GetCheck() );
	m_via_w.EnableWindow( 1 );
	m_via_hole_w.EnableWindow( 1 );
}

void DlgSetSegmentWidth::OnCbnSelchangeComboWidth()
{	
	if( rb_def_via.GetCheck() )
	{
		CString test;
		int i = m_width_box.GetCurSel();
		int v_w = (*m_v_w)[i];
		int v_h_w = (*m_v_h_w)[i];
		::MakeCStringFromDimension( &test, abs(v_w), m_units, TRUE, TRUE, FALSE, (m_units==MIL?0:3) );
		m_via_w.SetWindowText( test );
		::MakeCStringFromDimension( &test, abs(v_h_w), m_units, TRUE, TRUE, FALSE, (m_units==MIL?0:3) );
		m_via_hole_w.SetWindowText( test );
	}
}

void DlgSetSegmentWidth::OnCbnEditchangeComboWidth()
{
	CString test;
	int n = m_w->GetSize();
	if( rb_def_via.GetCheck() )
	{
		m_width_box.GetWindowText( test );
		int new_w = atoi( (LPCSTR)test );
		int new_v_w = 0;
		int new_v_h_w = 0;
		if( new_w > 0 )
		{
			for( int i=0; i<n; i++ )
			{
				if( new_w == (*m_w)[i] ) 
				{
					new_v_w = (*m_v_w)[i];
					new_v_h_w = (*m_v_h_w)[i];
					break;
				}
			}
			if( new_v_w )
			{
				::MakeCStringFromDimension( &test, abs(new_v_w), m_units, TRUE, TRUE, FALSE, (m_units==MIL?0:3) );
				m_via_w.SetWindowText( test );
				::MakeCStringFromDimension( &test, abs(new_v_h_w), m_units, TRUE, TRUE, FALSE, (m_units==MIL?0:3) );
				m_via_hole_w.SetWindowText( test );
			}
		}
	}
}

void DlgSetSegmentWidth::OnBnClickedRadioModTv()
{
	OnBnClickedRadioModify();
}

void DlgSetSegmentWidth::OnBnClickedRadioModT()
{
	OnBnClickedRadioModify();
}

void DlgSetSegmentWidth::OnBnClickedRadioModV()
{
	OnBnClickedRadioModify();
}

void DlgSetSegmentWidth::OnBnClickedRadioModify()
{
	if( m_radio_mod_tv.GetCheck() ) 
	{
		// enable trace and via controls
		m_tv = 1;
		m_width_box.EnableWindow();
		rb_set_via.EnableWindow();
		rb_def_via.EnableWindow();
		if( rb_set_via.GetCheck() )
		{
			m_via_w.EnableWindow();
			m_via_hole_w.EnableWindow();
		}
		else
		{
			m_via_w.EnableWindow(0);
			m_via_hole_w.EnableWindow(0);
		}
	}
	else if( m_radio_mod_t.GetCheck() )
	{
		// disable via controls
		m_tv = 2;
		m_width_box.EnableWindow();
		rb_set_via.EnableWindow(0);
		rb_def_via.EnableWindow(0);
		m_via_w.EnableWindow(0);
		m_via_hole_w.EnableWindow(0);
	}
	else
	{
		// disable trace controls
		m_tv = 3;
		m_width_box.EnableWindow(0);
		rb_set_via.SetCheck(1);
		rb_def_via.SetCheck(0);
		rb_set_via.EnableWindow();
		rb_def_via.EnableWindow(0);
		m_via_w.EnableWindow();
		m_via_hole_w.EnableWindow();
	}
}

