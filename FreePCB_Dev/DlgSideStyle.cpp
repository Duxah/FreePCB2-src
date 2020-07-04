#include "stdafx.h"
#include "FreePcb.h"
#include "DlgSideStyle.h"


// CDlgSideStyle dialog

IMPLEMENT_DYNAMIC(CDlgSideStyle, CDialog)
CDlgSideStyle::CDlgSideStyle(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgSideStyle::IDD, pParent)
{
}

CDlgSideStyle::~CDlgSideStyle()
{
}

void CDlgSideStyle::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Radio( pDX, IDC_RADIO_STRAIGHT, m_style );
	return;
}


BEGIN_MESSAGE_MAP(CDlgSideStyle, CDialog)
END_MESSAGE_MAP()


// CDlgSideStyle message handlers
void CDlgSideStyle::Initialize( int style )
{
	m_style = style;
}
