// DlgDupFootprintName.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgDupFootprintName.h"
#include ".\dlgdupfootprintname.h"


// CDlgDupFootprintName dialog

IMPLEMENT_DYNAMIC(CDlgDupFootprintName, CDialog)
CDlgDupFootprintName::CDlgDupFootprintName(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgDupFootprintName::IDD, pParent)
{
}

CDlgDupFootprintName::~CDlgDupFootprintName()
{
}

void CDlgDupFootprintName::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_DUP_MESSAGE, m_edit_message);
	DDX_Control(pDX, IDC_RADIO_REPLACE_ALL, m_radio_replace_all);
	DDX_Control(pDX, IDC_RADIO_REPLACE_THIS, m_radio_replace_this);
	DDX_Control(pDX, IDC_EDIT_NEW_FP_NAME, m_edit_new_name);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		m_edit_message.SetWindowText( *m_str );
		m_radio_replace_all.SetCheck( 1 );
		m_edit_new_name.EnableWindow( 0 );
	}
	else
	{
		// outgoing
		m_replace_all_flag = m_radio_replace_all.GetCheck();
		m_edit_new_name.GetWindowText( m_new_name_str );
		if( m_replace_all_flag == FALSE )
		{
			if( m_new_name_str.GetLength() == 0 )
			{
				// blank name
				AfxMessageBox( "You must enter a new footprint name" );
				pDX->Fail();
			}
			void * ptr;
			if( m_footprint_cache_map->Lookup( m_new_name_str, ptr ) )
			{
				// blank name
				AfxMessageBox( "New footprint name is already in use" );
				pDX->Fail();
			}
		}
	}
}


BEGIN_MESSAGE_MAP(CDlgDupFootprintName, CDialog)
	ON_BN_CLICKED(IDC_RADIO_REPLACE_ALL, OnBnClickedRadioReplaceAll)
	ON_BN_CLICKED(IDC_RADIO_REPLACE_THIS, OnBnClickedRadioReplaceThis)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()

// initialize the dialog
//
void CDlgDupFootprintName::Initialize( CString * message, 
									  CMapStringToPtr * shape_cache_map )
{
	m_str = message;
	m_footprint_cache_map = shape_cache_map;
}


// CDlgDupFootprintName message handlers

void CDlgDupFootprintName::OnBnClickedRadioReplaceAll()
{
	m_edit_new_name.EnableWindow( 0 );
}

void CDlgDupFootprintName::OnBnClickedRadioReplaceThis()
{
	m_edit_new_name.EnableWindow( 1 );
}

void CDlgDupFootprintName::OnBnClickedOk()
{
	m_replace_all_flag = m_radio_replace_all.GetCheck();
	OnOK();
}
