// DlgAddWidth.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgAddWidth.h"


// CDlgAddWidth dialog

IMPLEMENT_DYNAMIC(CDlgAddWidth, CDialog)
CDlgAddWidth::CDlgAddWidth(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgAddWidth::IDD, pParent)
{
}

CDlgAddWidth::~CDlgAddWidth()
{
}

void CDlgAddWidth::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	CString s;
	s = m_width;
	DDX_Text( pDX, IDC_EDIT_WIDTH, s );
	for( int i=0; i<min(sizeof(m_width)-1,s.GetLength()); i++ )
	{
		m_width[i] = s.GetAt(i);
		m_width[i+1] = '\0';
	}
	s = m_via_w;
	DDX_Text( pDX, IDC_EDIT_VIA_W, s );
	for( int i=0; i<min(sizeof(m_via_w)-1,s.GetLength()); i++ )
	{
		m_via_w[i] = s.GetAt(i);
		m_via_w[i+1] = '\0';
	}
	s = m_via_hole_w;
	DDX_Text( pDX, IDC_EDIT_HOLE_W, s );
	for( int i=0; i<min(sizeof(m_via_hole_w)-1,s.GetLength()); i++ )
	{
		m_via_hole_w[i] = s.GetAt(i);
		m_via_hole_w[i+1] = '\0';
	}
}


BEGIN_MESSAGE_MAP(CDlgAddWidth, CDialog)
END_MESSAGE_MAP()


// CDlgAddWidth message handlers
