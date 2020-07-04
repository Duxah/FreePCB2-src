// DlgVia.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgVia.h"


// CDlgVia dialog

IMPLEMENT_DYNAMIC(CDlgVia, CDialog)
CDlgVia::CDlgVia(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgVia::IDD, pParent)
{
}

CDlgVia::~CDlgVia()
{
}

void CDlgVia::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_VIA_WIDTH, m_edit_via_w);
	DDX_Control(pDX, IDC_VIA_HOLE_WIDTH, m_edit_hole_w);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		CString str_v,str_h,str,str2;
		::MakeCStringFromDimension( &str_v, abs(m_def_w), m_units, TRUE, TRUE, FALSE, (m_units==MIL?0:3) );
		m_edit_via_w.AddString( str_v );
		::MakeCStringFromDimension( &str_h, abs(m_def_hole_w), m_units, TRUE, TRUE, FALSE, (m_units==MIL?0:3) );
		m_edit_hole_w.AddString( str_h );
		if( m_v->GetSize() == m_h->GetSize() )
			for( int i=0; i<m_v->GetSize(); i++ )
			{
				int v = m_v->GetAt(i);
				int h = m_h->GetAt(i);
				if( v || h )
				{	
					if(!v)
						str = str_v;
					else
						::MakeCStringFromDimension( &str, abs(v), m_units, TRUE, TRUE, FALSE, (m_units==MIL?0:3) );
					if(!h)
						str2 = str_h;
					else
						::MakeCStringFromDimension( &str2, abs(h), m_units, TRUE, TRUE, FALSE, (m_units==MIL?0:3) );
					if( m_edit_via_w.FindString( 0,str ) == -1 )
						m_edit_via_w.AddString( str );
					if( m_edit_hole_w.FindString( 0,str2 ) == -1 )
						m_edit_hole_w.AddString( str2 );
				}
			}
		::MakeCStringFromDimension( &str, abs(m_via_w), m_units, TRUE, TRUE, FALSE, (m_units==MIL?0:3) );
		int i = m_edit_via_w.FindString( 0,str );
		if( i != -1 )
		{
			//m_edit_via_w.SetCurSel(i);
			//m_edit_hole_w.SetCurSel(i);
			m_edit_via_w.SetWindowTextA(str);
			m_edit_via_w.SetCurSel(i);
			OnChange();
		}
	}
	else
	{
		// outgoing
		CString str;
		m_edit_via_w.GetWindowText( str );
		m_via_w = my_atof( &str );
		if( m_via_w <= 0 )
		{
			AfxMessageBox( "Illegal via width" );
			pDX->Fail();
		}
		m_edit_hole_w.GetWindowText( str );
		m_via_hole_w = my_atof( &str );
		if( m_via_hole_w <= 0 )
		{
			AfxMessageBox( "Illegal via hole width" );
			pDX->Fail();
		}
	}
}

void CDlgVia::Initialize( int def_vw, int def_vh, CArray<int> * m_v_w, CArray<int> * m_v_h_w, int via_w, int via_hole_w, int units )
{
	m_v = m_v_w;
	m_h = m_v_h_w;
	m_def_w = def_vw;
	m_def_hole_w = def_vh;
	m_via_w = via_w;
	m_via_hole_w = via_hole_w;
	m_units = units;
}

BEGIN_MESSAGE_MAP(CDlgVia, CDialog)
	ON_CBN_SELCHANGE(IDC_VIA_WIDTH, OnChange)
END_MESSAGE_MAP()


// CDlgVia message handlers
void CDlgVia::OnChange()
{
	int i = m_edit_via_w.GetCurSel();
	if( i >= 0 )
	{
		CString str;
		m_edit_via_w.GetWindowText( str );
		m_via_w = my_atof( &str );
		m_via_hole_w = 0;
		if( m_via_w == abs(m_def_w) )
			m_via_hole_w = abs(m_def_hole_w);
		else for (int ii=0; ii<m_v->GetSize(); ii++)
			if( m_via_w == abs(m_v->GetAt(ii)) )
				m_via_hole_w = abs(m_h->GetAt(ii));
		if( m_via_hole_w == 0 )
			m_via_hole_w = abs(m_def_hole_w);
		::MakeCStringFromDimension( &str, m_via_hole_w, m_units, TRUE, TRUE, FALSE, (m_units==MIL?0:3) );
		int fs = m_edit_hole_w.FindString(0,str);
		if( fs >= 0 )
			m_edit_hole_w.SetCurSel(fs);
	}
}