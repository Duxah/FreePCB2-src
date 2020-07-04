// DlgShortcuts.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgShortcuts.h"


// CDlgShortcuts dialog

IMPLEMENT_DYNAMIC(CDlgShortcuts, CDialog)
CDlgShortcuts::CDlgShortcuts(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgShortcuts::IDD, pParent)
{
}

CDlgShortcuts::~CDlgShortcuts()
{
}

void CDlgShortcuts::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDlgShortcuts, CDialog)
END_MESSAGE_MAP()


// CDlgShortcuts message handlers
