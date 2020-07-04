// DlgLog.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgLog.h"
#include ".\dlglog.h"


// CDlgLog dialog

IMPLEMENT_DYNAMIC(CDlgLog, CDialog)
CDlgLog::CDlgLog(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgLog::IDD, pParent)
{
	m_running = TRUE;
}

CDlgLog::~CDlgLog()
{
}

void CDlgLog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_LOG, m_edit_log);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		m_edit_log.LimitText( 1000000 );
	}
}


BEGIN_MESSAGE_MAP(CDlgLog, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_WM_SETCURSOR()
	ON_BN_CLICKED(ID_HIDE_ME, OnBnClickedHideMe)
END_MESSAGE_MAP()

void CDlgLog::Move( int x, int y ) 
{
   CRect myRect;
   GetWindowRect(&myRect);
   myRect.OffsetRect( x, y );
   MoveWindow(myRect);
}

void CDlgLog::Clear()
{
	// clear edit control
	m_edit_log.SetSel( 0, 999999 );
	m_edit_log.ReplaceSel( "" );
}

void CDlgLog::AddLine( LPCTSTR str )
{
	m_edit_log.ReplaceSel( str );
}

// CDlgLog message handlers

void CDlgLog::OnBnClickedOk()
{
	m_running = FALSE;
	CWnd * frm = ::AfxGetMainWnd();
	frm->BringWindowToTop();
	OnOK();
}

BOOL CDlgLog::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	CMainFrame * frm = (CMainFrame*)AfxGetMainWnd();
	if( frm->m_bCursorHidden )
	{
		::ShowCursor( TRUE );
		frm->m_bCursorHidden = FALSE;
	}
//	return CDialog::OnSetCursor(pWnd, nHitTest, message);
	return TRUE;
}

void CDlgLog::OnBnClickedHideMe()
{
	this->ShowWindow( SW_HIDE );
}
