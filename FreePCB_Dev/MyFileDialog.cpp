// MyFileDialog.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "MyFileDialog.h"


// CMyFileDialog

IMPLEMENT_DYNAMIC(CMyFileDialog, CFileDialog)
CMyFileDialog::CMyFileDialog(BOOL bOpenFileDialog, LPCTSTR lpszDefExt, LPCTSTR lpszFileName,
		DWORD dwFlags, LPCTSTR lpszFilter, CWnd* pParentWnd, DWORD dsize) :
		CFileDialog(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags, lpszFilter, pParentWnd, dsize)
{
}

CMyFileDialog::~CMyFileDialog()
{
}

void CMyFileDialog::DoDataExchange(CDataExchange* pDX)
{
	// note: for some reason, this is not called on exit, use OnFileNameOK() instead
	CFileDialog::DoDataExchange(pDX);
	DDX_Control( pDX, IDC_RADIO_PARTS, m_radio_parts );
	DDX_Control( pDX, IDC_RADIO_NETS, m_radio_nets );
	DDX_Control( pDX, IDC_RADIO_PARTSANDNETS, m_radio_parts_and_nets );
	DDX_Control( pDX, IDC_RADIO_PADSPCB, m_radio_padspcb );
	DDX_Control( pDX, IDC_RADIO_FREEPCB, m_radio_freepcb );
	if( !pDX->m_bSaveAndValidate )
	{
		// on entry
		if( (m_flags & IMPORT_PARTS) && (m_flags & IMPORT_NETS) )
			m_radio_parts_and_nets.SetCheck( TRUE );
		else if( m_flags & IMPORT_NETS )
			m_radio_nets.SetCheck( TRUE );
		else
			m_radio_parts.SetCheck( TRUE );
		m_radio_padspcb.SetCheck( TRUE );
	}
}

BOOL CMyFileDialog::OnInitDialog()
{
	CFileDialog::OnInitDialog(); //Call base class method first
	return TRUE;
}

void CMyFileDialog::Initialize( int flags )
{
	m_flags = flags;
}

BOOL CMyFileDialog::OnFileNameOK()
{
	// on exit
	/*m_flags &= ~IMPORT_PARTS;
	m_flags &= ~IMPORT_NETS;
	if( m_radio_parts.GetCheck() || m_radio_parts_and_nets.GetCheck() )
		m_flags |= IMPORT_PARTS;
	if( m_radio_nets.GetCheck() || m_radio_parts_and_nets.GetCheck() )
		m_flags |= IMPORT_NETS;

	if( m_radio_padspcb.GetCheck() )
		m_format = PADSPCB;*/

	return FALSE;
}

BEGIN_MESSAGE_MAP(CMyFileDialog, CFileDialog)
END_MESSAGE_MAP()

// CMyFileDialog message handlers

