// DlgSetTraceWidths.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgSetTraceWidths.h"
#include ".\dlgsettracewidths.h"


// CDlgSetTraceWidths dialog

IMPLEMENT_DYNAMIC(CDlgSetTraceWidths, CDialog)
CDlgSetTraceWidths::CDlgSetTraceWidths(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgSetTraceWidths::IDD, pParent)
{
	m_width = 0;
	m_via_width = 0;
	m_hole_width = 0;
	m_apply_trace	= FALSE;
	m_apply_via		= FALSE;
	bTraces			= FALSE;     
	bRevertTraces	= FALSE; 
	bVias			= FALSE;
	bDefaultVias	= FALSE;
	bRevertVias		= FALSE;
	m_w = 0;
	m_v_w = 0;
	m_v_h_w = 0;
	m_units = MIL;
}

CDlgSetTraceWidths::~CDlgSetTraceWidths()
{
}

void CDlgSetTraceWidths::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_WIDTH, m_combo_width);
	DDX_Control(pDX, IDC_RADIO_DEF, m_radio_default_via_for_trace);
	DDX_Control(pDX, IDC_RADIO_SET, m_radio_set_via_width);
	DDX_Control(pDX, IDC_EDIT_VIA_W, m_edit_via_pad);
	DDX_Control(pDX, IDC_EDIT_HOLE_W, m_edit_via_hole);
	DDX_Control(pDX, IDC_CHECK1, m_check_apply);
	DDX_Control(pDX, IDC_CHECK2, m_check_trace);
	DDX_Control(pDX, IDC_CHECK3, m_check_vias);
	DDX_Control(pDX, IDC_RADIO_REVERT_TRACES, m_radio_revert_traces);
	DDX_Control(pDX, IDC_RADIO_REVERT_VIAS, m_radio_revert_vias);
	DDX_Control(pDX, IDC_RADIO_SET_TRACE_WIDTH, m_radio_set_trace_width);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		CString str;
		for( int i=0; i<m_w->GetSize(); i++ )  
		{
			::MakeCStringFromDimension( &str, abs((*m_w)[i]), m_units, TRUE, TRUE, FALSE, (m_units==MIL?0:3) );
			m_combo_width.InsertString( i, str );
		}
		if( m_width > 0 )
		{
			::MakeCStringFromDimension( &str, abs(m_width), m_units, TRUE, TRUE, FALSE, (m_units==MIL?0:3) );
			m_combo_width.SetWindowText( str );
		}
		if( m_via_width > 0 && m_hole_width > 0 )
		{
			::MakeCStringFromDimension( &str, abs(m_via_width), m_units, TRUE, TRUE, FALSE, (m_units==MIL?0:3) );
			m_edit_via_pad.SetWindowText( str );
			::MakeCStringFromDimension( &str, abs(m_hole_width), m_units, TRUE, TRUE, FALSE, (m_units==MIL?0:3) );
			m_edit_via_hole.SetWindowText( str );
		}
		m_check_trace.SetCheck(1);
		m_check_vias.SetCheck(1);
		m_radio_revert_vias.SetCheck(1);
		m_radio_default_via_for_trace.SetCheck(0);
		if( m_width > 0 )
		{
			m_radio_set_trace_width.SetCheck( 1 );
			m_radio_revert_traces.SetCheck( 0 );
		}
		else
		{
			m_radio_revert_traces.SetCheck( 1 );
			m_radio_set_trace_width.SetCheck( 0 );
		}
		m_check_apply.SetCheck(0);
		SetFields();
	}
	else
	{
		// outgoing
		SetFields();
		if( bTraces && bRevertTraces )
		{
			m_width = 0;
		}
		else if( bTraces )
		{
			CString s = "";
			DDX_Text(pDX,IDC_COMBO_WIDTH,s);
			m_width = my_atof(&s);
		}
		else
			m_width = -1;
		if( bVias && bRevertVias )
		{
			m_via_width = 0;
			m_hole_width = 0;
		}
		else if( bVias )
		{
			CString s = "";
			DDX_Text(pDX,IDC_EDIT_VIA_W,s);
			m_via_width = my_atof(&s);
			s = "";
			DDX_Text(pDX,IDC_EDIT_HOLE_W,s);
			m_hole_width = my_atof(&s);
		}
		else
			m_via_width = -1;
	}
}


BEGIN_MESSAGE_MAP(CDlgSetTraceWidths, CDialog)
	ON_BN_CLICKED(IDC_RADIO_DEF, OnBnClickedRadioDef)
	ON_BN_CLICKED(IDC_RADIO_SET, OnBnClickedRadioSet)
	ON_CBN_SELCHANGE(IDC_COMBO_WIDTH, OnCbnSelchangeComboWidth)
	ON_CBN_EDITCHANGE(IDC_COMBO_WIDTH, OnCbnEditchangeComboWidth)
	ON_BN_CLICKED(IDC_CHECK2, OnBnClickedSetTrace)
	ON_BN_CLICKED(IDC_CHECK3, OnBnClickedSetVias)
	ON_BN_CLICKED(IDC_RADIO_REVERT_TRACES, OnBnClickedRadioRevertTraces)
	ON_BN_CLICKED(IDC_RADIO_REVERT_VIAS, OnBnClickedRadioRevertVias)
	ON_BN_CLICKED(IDC_RADIO_SET_TRACE_WIDTH, OnBnClickedRadioSetTraceWidth)
	ON_BN_CLICKED(IDC_RADIO_SET_SIZE, &CDlgSetTraceWidths::OnBnClickedRadioSetSize)
END_MESSAGE_MAP()


// CDlgSetTraceWidths message handlers

void CDlgSetTraceWidths::OnBnClickedRadioDef()
{
	OnCbnEditchangeComboWidth();
	SetFields();
}

void CDlgSetTraceWidths::OnBnClickedRadioSet()
{
	SetFields();
}

void CDlgSetTraceWidths::OnCbnSelchangeComboWidth()
{
	CString test;
	int i = m_combo_width.GetCurSel();
	m_combo_width.GetLBText( i, test );
	if( m_radio_default_via_for_trace.GetCheck() )
	{
		int new_w = my_atof( &test );
		int new_v_w = 0;
		int new_v_h_w = 0;
		if( new_w >= 0 )
		{
			for( int i=m_w->GetSize()-1; i>=0; i-- )
			{
				if( new_w == abs((*m_w)[i]) ) 
				{
					new_v_w = (*m_v_w)[i];
					new_v_h_w = (*m_v_h_w)[i];
					break;
				}
			}
		}
		CString s;
		if ( test.Right(2) == "MM" || test.Right(2) == "mm" )
			::MakeCStringFromDimension( &s, abs(new_v_w), MM, TRUE, TRUE, FALSE, 2 );
		else
			::MakeCStringFromDimension( &s, abs(new_v_w), MIL, TRUE, TRUE, FALSE, 0 );
		m_edit_via_pad.SetWindowText( s );
		if ( test.Right(2) == "MM" || test.Right(2) == "mm" )
			::MakeCStringFromDimension( &s, abs(new_v_h_w), MM, TRUE, TRUE, FALSE, 2 );
		else
			::MakeCStringFromDimension( &s, abs(new_v_h_w), MIL, TRUE, TRUE, FALSE, 0 );
		m_edit_via_hole.SetWindowText( s );
	}
}

void CDlgSetTraceWidths::OnCbnEditchangeComboWidth()
{
	CString test;
	int n = m_w->GetSize();
	m_combo_width.GetWindowText( test );
	int new_w = my_atof( &test );
	int new_v_w = 0;
	int new_v_h_w = 0;
	if( new_w >= 0 )
	{
		for( int i=m_w->GetSize()-1; i>=0; i-- )
		{
			if( new_w == abs((*m_w)[i]) ) 
			{
				new_v_w = (*m_v_w)[i];
				new_v_h_w = (*m_v_h_w)[i];
				break;
			}
		}
	}
	CString s;
	if ( test.Right(2) == "MM" || test.Right(2) == "mm" )
		::MakeCStringFromDimension( &s, abs(new_v_w), MM, TRUE, TRUE, FALSE, 2 );
	else
		::MakeCStringFromDimension( &s, abs(new_v_w), MIL, TRUE, TRUE, FALSE, 0 );
	m_edit_via_pad.SetWindowText( s );
	if ( test.Right(2) == "MM" || test.Right(2) == "mm" )
		::MakeCStringFromDimension( &s, abs(new_v_h_w), MM, TRUE, TRUE, FALSE, 2 );
	else
		::MakeCStringFromDimension( &s, abs(new_v_h_w), MIL, TRUE, TRUE, FALSE, 0 );
	m_edit_via_hole.SetWindowText( s );
}

void CDlgSetTraceWidths::OnBnClickedSetTrace()
{
	SetFields();
}

void CDlgSetTraceWidths::OnBnClickedSetVias() 
{
	SetFields();
}

void CDlgSetTraceWidths::SetFields()
{
	bTraces = m_check_trace.GetCheck();     
	bRevertTraces = m_radio_revert_traces.GetCheck(); 
	m_radio_revert_traces.EnableWindow( bTraces );
	m_radio_set_trace_width.EnableWindow( bTraces );
	m_combo_width.EnableWindow( bTraces && !bRevertTraces );

	bVias = m_check_vias.GetCheck();
	if( m_radio_default_via_for_trace.GetCheck() && !bTraces )
	{
		m_radio_revert_vias.SetCheck(1);
		m_radio_default_via_for_trace.SetCheck(0);
	}
	bDefaultVias = m_radio_default_via_for_trace.GetCheck();
	bRevertVias = m_radio_revert_vias.GetCheck();
	m_radio_default_via_for_trace.EnableWindow( bVias && bTraces );
	m_radio_set_via_width.EnableWindow( bVias );
	m_edit_via_pad.EnableWindow( bVias && !bDefaultVias && !bRevertVias );
	m_edit_via_hole.EnableWindow( bVias && !bDefaultVias && !bRevertVias  );

	m_apply_trace = m_check_apply.GetCheck() && bTraces;
	m_apply_via = m_check_apply.GetCheck() && bVias;
}

void CDlgSetTraceWidths::OnBnClickedRadioRevertTraces()
{
	SetFields();
}

void CDlgSetTraceWidths::OnBnClickedRadioRevertVias()
{
	SetFields();
}

void CDlgSetTraceWidths::OnBnClickedRadioSetTraceWidth()
{
	SetFields();
}


void CDlgSetTraceWidths::OnBnClickedRadioSetSize()
{
	// TODO: добавьте свой код обработчика уведомлений
}
