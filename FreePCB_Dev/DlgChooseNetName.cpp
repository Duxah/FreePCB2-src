// DlgChooseNetName.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgChooseNetName.h"


// CDlgChooseNetName dialog

IMPLEMENT_DYNAMIC(CDlgChooseNetName, CDialog)
CDlgChooseNetName::CDlgChooseNetName(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgChooseNetName::IDD, pParent)
{
}

CDlgChooseNetName::~CDlgChooseNetName()
{
}

void CDlgChooseNetName::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_combo_names);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		for( int i=0; i<m_str->GetSize(); i++ )
		{
			CString s = (*m_str)[i];
			m_combo_names.InsertString( i, s );
		}
		m_combo_names.SetCurSel( 0 );
	}
	else
	{
		// outgoing
		m_combo_names.GetWindowText( m_sel_str );
	}
}

void CDlgChooseNetName::Initialize( CArray<CString> * str )
{
	m_str = str;
}


BEGIN_MESSAGE_MAP(CDlgChooseNetName, CDialog)
END_MESSAGE_MAP()


// CDlgChooseNetName message handlers
