// DlgExportDsn.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgExportDsn.h"
#include ".\dlgexportdsn.h"

// globals to save options
BOOL g_bVerbose = FALSE;
BOOL g_bInfo = FALSE;
int g_FromToType = 0;

// CDlgExportDsn dialog

IMPLEMENT_DYNAMIC(CDlgExportDsn, CDialog)
CDlgExportDsn::CDlgExportDsn(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgExportDsn::IDD, pParent)
	, m_bounds_poly(0)
	, m_signals_poly(0)
	, m_dsn_filepath(_T(""))
{
}

CDlgExportDsn::~CDlgExportDsn()
{
}

void CDlgExportDsn::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHECK_VERBOSE, m_check_verbose);
	DDX_Control(pDX, IDC_CHECK_INFO, m_check_info);
	DDX_CBIndex(pDX, IDC_COMBO_BOUNDS, m_bounds_poly);
	DDX_CBIndex(pDX, IDC_COMBO_SIGNALS, m_signals_poly);
	DDX_Control(pDX, IDC_COMBO_BOUNDS, m_combo_bounds);
	DDX_Control(pDX, IDC_COMBO_SIGNALS, m_combo_signals);
	DDX_Text(pDX, IDC_EDIT_FILE, m_dsn_filepath);
	DDX_Control(pDX, IDC_CHECK_FROM_TO, m_check_from_to);
	DDX_Control(pDX, IDC_RADIO_ALL, m_radio_all);
	DDX_Control(pDX, IDC_RADIO_LOCKED, m_radio_locked);
	DDX_Control(pDX, IDC_RADIO_NET_LOCKED, m_radio_net_locked);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		if( m_num_polys < 1 )
			m_num_polys = 1;
		for( int i=1; i<=m_num_polys; i++ )
		{
			CString str;
			str.Format( "%d", i );
			m_combo_bounds.InsertString( i-1, str );
			m_combo_signals.InsertString( i-1, str );
		}
		m_combo_bounds.SetCurSel(m_bounds_poly);
		m_combo_signals.SetCurSel(m_signals_poly);
		m_check_verbose.SetCheck( m_flags & DSN_VERBOSE );
		m_check_info.SetCheck( m_flags & DSN_INFO_ONLY );
		if( m_flags & DSN_FROM_TO_MASK )
		{
			m_check_from_to.SetCheck( 1 );
		}
		else
		{
			m_check_from_to.SetCheck( 0 );
			m_radio_all.EnableWindow( FALSE );
			m_radio_locked.EnableWindow( FALSE );
			m_radio_net_locked.EnableWindow( FALSE );
		}
		m_radio_all.SetCheck( (m_flags & DSN_FROM_TO_MASK) == DSN_FROM_TO_ALL );
		m_radio_locked.SetCheck( (m_flags & DSN_FROM_TO_MASK) == DSN_FROM_TO_LOCKED );
		m_radio_net_locked.SetCheck( (m_flags & DSN_FROM_TO_MASK) == DSN_FROM_TO_NET_LOCKED );
	}
	else 
	{
		// outgoing 
		m_flags = 0;
		if( m_check_verbose.GetCheck() )
			m_flags |= DSN_VERBOSE;
		if( m_check_info.GetCheck() )
			m_flags |= DSN_INFO_ONLY;
		if( m_check_from_to.GetCheck() )
		{
			if( m_radio_all.GetCheck() )
				m_flags |= DSN_FROM_TO_ALL;
			else if( m_radio_locked.GetCheck() )
				m_flags |= DSN_FROM_TO_LOCKED;
			else
				m_flags |= DSN_FROM_TO_NET_LOCKED;
		}
	}
}


BEGIN_MESSAGE_MAP(CDlgExportDsn, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDC_CHECK_FROM_TO, OnBnClickedCheckFromTo)
	ON_BN_CLICKED(IDC_RADIO_ALL, OnBnClickedRadioAll)
	ON_BN_CLICKED(IDC_RADIO_LOCKED, OnBnClickedRadioLocked)
	ON_BN_CLICKED(IDC_RADIO_NET_LOCKED, OnBnClickedRadioNetLocked)
END_MESSAGE_MAP()


void CDlgExportDsn::Initialize( CString * dsn_filepath,
							    int num_board_outline_polys,
								int bounds_poly, int signals_poly,
								int flags )
{
	m_dsn_filepath = *dsn_filepath;
	m_bounds_poly = bounds_poly;
	m_signals_poly = signals_poly;
	m_num_polys = num_board_outline_polys;
	m_flags = flags;
}
			
// CDlgExportDsn message handlers

void CDlgExportDsn::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	OnOK();
}



void CDlgExportDsn::OnBnClickedCheckFromTo()
{
	BOOL bFromTo = m_check_from_to.GetCheck();
	m_radio_all.EnableWindow( bFromTo );
	m_radio_locked.EnableWindow( bFromTo );
	m_radio_net_locked.EnableWindow( bFromTo );
	if( bFromTo )
	{
		m_radio_all.SetCheck( TRUE );
		m_radio_locked.SetCheck( FALSE );
		m_radio_net_locked.SetCheck( FALSE );
	}
}

void CDlgExportDsn::OnBnClickedRadioAll()
{
	// TODO: Add your control notification handler code here
}

void CDlgExportDsn::OnBnClickedRadioLocked()
{
	// TODO: Add your control notification handler code here
}

void CDlgExportDsn::OnBnClickedRadioNetLocked()
{
	// TODO: Add your control notification handler code here
}
