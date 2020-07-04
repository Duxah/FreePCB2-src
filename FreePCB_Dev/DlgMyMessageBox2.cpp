// DlgMyMessageBox.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgMyMessageBox2.h"


// CDlgMyMessageBox dialog

IMPLEMENT_DYNAMIC(CDlgMyMessageBox2, CDialog)
CDlgMyMessageBox2::CDlgMyMessageBox2(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgMyMessageBox2::IDD, pParent)
{
}

CDlgMyMessageBox2::~CDlgMyMessageBox2()
{
}

void CDlgMyMessageBox2::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_MYMESSAGE_2, m_message);
	DDX_Control(pDX, IDC_CHECK1, m_check_dont_show);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		m_message.SetWindowText( *m_mess );
		// show cursor
		::ShowCursor( TRUE );
	}
	else
	{
		// outgoing
		bDontShowBoxState = m_check_dont_show.GetCheck();
		::ShowCursor( FALSE );
	}
}

BEGIN_MESSAGE_MAP(CDlgMyMessageBox2, CDialog)
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()


void CDlgMyMessageBox2::Initialize( CString * mess )
{
	m_mess = mess;
}

