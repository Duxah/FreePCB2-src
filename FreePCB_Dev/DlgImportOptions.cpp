// DlgImportOptions.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgImportOptions.h"
#include ".\dlgimportoptions.h"


// CDlgImportOptions dialog

IMPLEMENT_DYNAMIC(CDlgImportOptions, CDialog)
CDlgImportOptions::CDlgImportOptions(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgImportOptions::IDD, pParent)
{
}

CDlgImportOptions::~CDlgImportOptions()
{
}

void CDlgImportOptions::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_RADIO_REMOVE_PARTS, m_radio_remove_parts);
	DDX_Control(pDX, IDC_RADIO_KEEP_PARTS_NO_CONNECTIONS, m_radio_keep_parts_no_connections);
	DDX_Control(pDX, IDC_RADIO_KEEP_PARTS_AND_CONNECTIONS, m_radio_keep_parts_and_connections);
	DDX_Control(pDX, IDC_RADIO4, m_radio_change_fp);
	DDX_Control(pDX, IDC_RADIO3, m_radio_keep_fp);
	DDX_Control(pDX, IDC_RADIO6, m_radio_remove_nets);
	DDX_Control(pDX, IDC_RADIO5, m_radio_keep_nets);
	DDX_Control(pDX, IDC_CHECK_KEEP_TRACES, m_check_keep_traces);
	DDX_Control(pDX, IDC_CHECK_KEEP_STUBS, m_check_keep_stubs);
	DDX_Control(pDX, IDC_CHECK_KEEP_AREAS, m_check_keep_areas);
	DDX_Control(pDX, ID_SAVE_AND_IMPORT, m_button_save_and_import);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		if( m_flags & IMPORT_PARTS )
		{
			m_radio_remove_parts.EnableWindow( 1 );
			m_radio_keep_parts_no_connections.EnableWindow( 1 );
			m_radio_keep_parts_and_connections.EnableWindow( 1 );
			m_radio_keep_fp.EnableWindow( 1 );
			m_radio_change_fp.EnableWindow( 1 );
			if( m_flags & KEEP_PARTS_AND_CON )
				m_radio_keep_parts_and_connections.SetCheck( TRUE );
			else if( m_flags & KEEP_PARTS_NO_CON )
				m_radio_keep_parts_no_connections.SetCheck( TRUE );
			else
				m_radio_remove_parts.SetCheck( TRUE );
			if( m_flags & KEEP_FP )
				m_radio_keep_fp.SetCheck( TRUE );
			else
				m_radio_change_fp.SetCheck( TRUE );
		}
		else
		{
			m_radio_remove_parts.EnableWindow( 0 );
			m_radio_keep_parts_no_connections.EnableWindow( 0 );
			m_radio_keep_parts_and_connections.EnableWindow( 0 );
			m_radio_keep_fp.EnableWindow( 0 );
			m_radio_change_fp.EnableWindow( 0 );
		}
		if( m_flags & IMPORT_NETS )
		{
			m_radio_keep_nets.EnableWindow( 1 );
			m_radio_remove_nets.EnableWindow( 1 );
			m_check_keep_traces.EnableWindow( 1 );
			m_check_keep_stubs.EnableWindow( 1 );
			m_check_keep_areas.EnableWindow( 1 );
			if( m_flags & KEEP_NETS )
				m_radio_keep_nets.SetCheck( TRUE );
			else
				m_radio_remove_nets.SetCheck( TRUE );
			m_check_keep_traces.SetCheck( m_flags & KEEP_TRACES );
			m_check_keep_stubs.SetCheck( m_flags & KEEP_STUBS );
			m_check_keep_areas.SetCheck( m_flags & KEEP_AREAS );
		}
		else
		{
			m_radio_keep_nets.EnableWindow( 0 );
			m_radio_remove_nets.EnableWindow( 0 );
			m_check_keep_traces.EnableWindow( 0 );
			m_check_keep_stubs.EnableWindow( 0 );
			m_check_keep_areas.EnableWindow( 0 );
		}
	}
	else
	{
		// outgoing
		if( m_flags & IMPORT_PARTS )
		{
			m_flags &= ~(KEEP_PARTS_NO_CON | KEEP_PARTS_AND_CON | KEEP_FP);
			if( m_radio_keep_parts_no_connections.GetCheck() )
				m_flags |= KEEP_PARTS_NO_CON;
			else if( m_radio_keep_parts_and_connections.GetCheck() )
				m_flags |= KEEP_PARTS_AND_CON;
			if( m_radio_keep_fp.GetCheck() )
				m_flags |= KEEP_FP;
		}
		if( m_flags & IMPORT_NETS )
		{
			m_flags &= ~(KEEP_NETS | KEEP_TRACES | KEEP_STUBS | KEEP_AREAS);
			if( m_radio_keep_nets.GetCheck() )
				m_flags |= KEEP_NETS;
			if( m_check_keep_traces.GetCheck() )
				m_flags |= KEEP_TRACES;
			if( m_check_keep_stubs.GetCheck() )
				m_flags |= KEEP_STUBS;
			if( m_check_keep_areas.GetCheck() )
				m_flags |= KEEP_AREAS;
		}
	}
}


BEGIN_MESSAGE_MAP(CDlgImportOptions, CDialog)
	ON_BN_CLICKED(ID_SAVE_AND_IMPORT, OnBnClickedSaveAndImport)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()

void CDlgImportOptions::Initialize( int flags )
{
	m_flags = flags;
}

// CDlgImportOptions message handlers

void CDlgImportOptions::OnBnClickedSaveAndImport()
{
	m_flags |= SAVE_BEFORE_IMPORT;
	OnOK();
}

void CDlgImportOptions::OnBnClickedOk()
{
	m_flags &= ~SAVE_BEFORE_IMPORT;
	OnOK();
}
