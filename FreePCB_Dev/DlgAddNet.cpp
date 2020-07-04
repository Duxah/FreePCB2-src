// DlgAddNet.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgAddNet.h"


// CDlgAddNet dialog

IMPLEMENT_DYNAMIC(CDlgAddNet, CDialog)
CDlgAddNet::CDlgAddNet(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgAddNet::IDD, pParent)
{
}

CDlgAddNet::~CDlgAddNet()
{
}

void CDlgAddNet::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDlgAddNet, CDialog)
END_MESSAGE_MAP()


// CDlgAddNet message handlers
