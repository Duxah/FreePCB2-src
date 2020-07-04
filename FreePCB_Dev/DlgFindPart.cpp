// DlgFindPart.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgFindPart.h"
#include ".\dlgfindpart.h"


// CDlgFindPart dialog

IMPLEMENT_DYNAMIC(CDlgFindPart, CDialog)
CDlgFindPart::CDlgFindPart(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgFindPart::IDD, pParent)
{
}

CDlgFindPart::~CDlgFindPart()
{
}

void CDlgFindPart::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_combo_ref_des);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		cpart * part = m_pl->GetFirstPart();
		while( part )
		{
			m_combo_ref_des.AddString( part->ref_des );
			part = m_pl->GetNextPart( part );
		}
	}
	else
	{
		// outgoing
		m_combo_ref_des.GetWindowText( sel_ref_des );
	}
}


BEGIN_MESSAGE_MAP(CDlgFindPart, CDialog)
END_MESSAGE_MAP()


// CDlgFindPart message handlers

void CDlgFindPart::Initialize( CPartList * pl )
{
	m_pl = pl;
}
