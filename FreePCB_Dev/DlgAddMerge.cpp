// DlgAddMerge.cpp : implementation file
//
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *		Author:       Duxah (ver. 2.0 - 2.033)             *
 *		email: duxah@yahoo.com							   *
 *		URL: www.freepcb.dev							   *
 *		Copyright: (C) Duxah 2014 - 2020.				   *
 *		This software is free for non-commercial use.	   *
 *		It may be copied, modified, and redistributed	   *
 *		provided that this copyright notice is 			   *
 *		preserved on all copies. You may not use this	   *
 *		software, in whole or in part, in support of	   *
 *		any commercial product without the express 		   *
 *		consent of the authors.							   *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#include "stdafx.h"
#include "FreePcb.h"
#include "DlgAddMerge.h"


// CDlgAddMergeName dialog

IMPLEMENT_DYNAMIC(CDlgAddMerge, CDialog)
CDlgAddMerge::CDlgAddMerge(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgAddMerge::IDD, pParent)
{
	Doc = NULL;
	m_merge_name = "";
	m_cl = 0;
}  

CDlgAddMerge::~CDlgAddMerge()
{
}

void CDlgAddMerge::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MERGE_NAME, m_edit);
	DDX_Control(pDX, IDC_MERGE_AREA_CLEARANCE, m_edit_cl);
	DDX_Control(pDX, IDC_DLG_MERGE_UNITS, m_edit_u);
	if( pDX->m_bSaveAndValidate )
	{
		m_edit.GetWindowText(m_merge_name);
		if (m_merge_name.Find("\"",0) >= 0)
		{
			AfxMessageBox( "Illegal merge name. The name can not contain the quote character!" );
			pDX->Fail();
		}
		if (m_merge_name.Find(" ",0) >= 0)
		{
			AfxMessageBox( "Illegal merge name. The name can not contain a space character!" );
			pDX->Fail();
		}
		m_merge_name = m_merge_name.Left(CShape::MAX_NAME_SIZE);
		//
		CString str="";
		m_edit_cl.GetWindowText(str);
		if( Doc )
		{
			if( Doc->m_units == MM )
				str += "MM";
			else if( Doc->m_units == MIL )
				str += "MIL";
		}
		m_cl = my_atof(&str);
	}
	else
	{
		CString s;
		int num=0;	
		if( m_merge_name.Left(5) == "Empty" || m_merge_name.GetLength() == 0 )
		{
			m_merge_name = "Merge-1";
			while( Doc->m_mlist->GetIndex(m_merge_name) >= 0 )
			{
				num++;
				::MakeCStringFromDimension( &s, num, NM, FALSE, FALSE, FALSE, 0 );
				m_merge_name.Format( "Merge-%s", s );
			}
		}
		m_edit.AddString("None");
		for( int im=0; im<Doc->m_mlist->GetSize(); im++ )
		{
			m_edit.AddString( Doc->m_mlist->GetMerge(im) );
		}
		m_edit.SetWindowText(m_merge_name);
		if( m_cl )
		{
			::MakeCStringFromDimension( &s, (float)m_cl, Doc->m_units, FALSE, FALSE, FALSE );
			m_edit_cl.SetWindowText(s);
		}
		else
			m_edit_cl.SetWindowText("0.0");
		if( Doc )
		{
			if( Doc->m_units == MM )
				m_edit_u.SetWindowText("Units MM");
			else if( Doc->m_units == MIL )
				m_edit_u.SetWindowText("Units MIL");
		}
		else
			m_edit_u.SetWindowText("Units NM");
	}
}


BEGIN_MESSAGE_MAP(CDlgAddMerge, CDialog)
END_MESSAGE_MAP()


// CDlgAddWidth message handlers
