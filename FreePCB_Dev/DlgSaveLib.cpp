// DlgSaveLib.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgSaveLib.h"


// CDlgSaveLib dialog

IMPLEMENT_DYNAMIC(CDlgSaveLib, CDialog)
CDlgSaveLib::CDlgSaveLib(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgSaveLib::IDD, pParent)
{
}

CDlgSaveLib::~CDlgSaveLib()
{
	//delete m_i_sel;
	//delete m_sel_data;
}

void CDlgSaveLib::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_FP2, m_list_box);

	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		for( int i=0; i<m_n_fp; i++ )
		{
			m_list_box.InsertString( i, m_names[i] );
			m_list_box.SetItemData( i, i );
		}
	}
	else
	{
		// outgoing
		delete m_i_sel;
		delete m_sel_data;
	}
}

BEGIN_MESSAGE_MAP(CDlgSaveLib, CDialog)
	ON_BN_CLICKED(IDC_BUTTON8, OnBnClickedDelete)
	ON_BN_CLICKED(IDC_BUTTON4, OnBnClickedButtonTop)
	ON_BN_CLICKED(IDC_BUTTON9, OnBnClickedButtonUp)
	ON_BN_CLICKED(IDC_BUTTON5, OnBnClickedButtonDown)
	ON_BN_CLICKED(IDC_BUTTON10, OnBnClickedButtonBottom)
END_MESSAGE_MAP()

// initialize data (from calling function)
void CDlgSaveLib::Initialize( CArray<CString> * names )
{
	m_n_fp = names->GetSize();
	m_names.SetSize(m_n_fp);
	m_i_sel = new int[m_n_fp];	  // ok (dodataex)
	m_sel_data = new int[m_n_fp]; // ok (dodataex)
	for( int i=0; i<m_n_fp; i++ )
		m_names[i] = (*names)[i];
}

void CDlgSaveLib::OnBnClickedDelete()
{
	// get selected indexes
	GetSelected();
	// delete string
	for( int i=m_n_sel-1; i>=0; i-- )
	{
		int i_del = m_i_sel[i];
		m_list_box.DeleteString( i_del );
	}
}

void CDlgSaveLib::OnBnClickedButtonTop()
{
	GetSelected();
	int i_top = 0;
	for( int i=0; i<m_n_sel; i++ )
	{
		int i_del = m_i_sel[i];
		CString name;
		m_list_box.GetText( i_del, name );
		int index = m_list_box.GetItemData( i_del );
		m_list_box.DeleteString( i_del );
		m_list_box.InsertString( i_top, name ); 
		m_list_box.SetItemData( i_top, index ); 
		i_top++;
	}
	ResetSelected();
}

void CDlgSaveLib::OnBnClickedButtonUp()
{
	GetSelected();
	int sel_at_top = 0;
	for( int i=0; i<m_n_sel; i++ )
	{
		int i_del = m_i_sel[i];
		if( i_del != sel_at_top )
		{
			// move selected string and data up
			CString name;
			m_list_box.GetText( i_del, name );
			int index = m_list_box.GetItemData( i_del );
			m_list_box.DeleteString( i_del );
			m_list_box.InsertString( i_del-1, name ); 
			m_list_box.SetItemData( i_del-1, index ); 
		}
		else
			sel_at_top++;	// still no room to move
	}
	ResetSelected();
}

void CDlgSaveLib::OnBnClickedButtonDown()
{
	int n_box = m_list_box.GetCount();
	GetSelected();
	int sel_at_bottom = n_box-1;
	for( int i=m_n_sel-1; i>=0; i-- )
	{
		int i_del = m_i_sel[i];
		if( i_del != sel_at_bottom )
		{
			// move selected string and data down
			CString name;
			m_list_box.GetText( i_del, name );
			int index = m_list_box.GetItemData( i_del );
			m_list_box.DeleteString( i_del );
			m_list_box.InsertString( i_del+1, name ); 
			m_list_box.SetItemData( i_del+1, index ); 
		}
		else
			sel_at_bottom--;	// still no room to move
	}
	ResetSelected();
}

void CDlgSaveLib::OnBnClickedButtonBottom()
{
	// get selected indexes
	int n_box = m_list_box.GetCount();
	GetSelected();
	int i_bottom = n_box-1;
	for( int i=m_n_sel-1; i>=0; i-- )
	{
		int i_del = m_i_sel[i];
		CString name;
		m_list_box.GetText( i_del, name );
		int index = m_list_box.GetItemData( i_del );
		m_list_box.DeleteString( i_del );
		m_list_box.InsertString( i_bottom, name ); 
		m_list_box.SetItemData( i_bottom, index ); 
		i_bottom--;
	}
	ResetSelected();
}

// get selections from list box and save data
void CDlgSaveLib::GetSelected()
{
	m_n_sel = m_list_box.GetSelCount();
	m_list_box.GetSelItems( m_n_sel, m_i_sel );
	for( int i_sel=0; i_sel<m_n_sel; i_sel++ )
		m_sel_data[i_sel] = m_list_box.GetItemData( m_i_sel[i_sel] );
}

// set selections from saved data
void CDlgSaveLib::ResetSelected()
{
	for( int i=0; i<m_n_sel; i++ )
	{
		int i_data = m_sel_data[i];
		for( int i=0; i<m_list_box.GetCount(); i++ )
		{
			int idata = m_list_box.GetItemData( i );
			if( idata == i_data )
				m_list_box.SetSel( i );
		}
	}
}
