// DlgNetlist.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgNetlist.h"
#include "DlgEditNet.h"
#include "DlgSetTraceWidths.h"
#include "DlgNetCombine.h"
#include "DlgChooseNetName.h"

extern CFreePcbApp theApp;

// columns for list
enum {
	COL_NAME = 0,
	COL_PINS,
	COL_WIDTH,
	COL_VIA_W,
	COL_HOLE_W
};

// sort types
enum {
	SORT_UP_NAME = 0,
	SORT_DOWN_NAME,
	SORT_UP_PINS,
	SORT_DOWN_PINS,
	SORT_UP_WIDTH,
	SORT_DOWN_WIDTH,
	SORT_UP_VIA_W,
	SORT_DOWN_VIA_W,
	SORT_UP_HOLE_W,
	SORT_DOWN_HOLE_W
};

// global so that it is available to Compare() for sorting list control items
netlist_info nl_combine;

// global callback function for sorting
// lp1, lp2 are indexes to global arrays above
//		
int CALLBACK CompareNetlistCombine( LPARAM lp1, LPARAM lp2, LPARAM type )
{
	int ret = 0;
	switch( type )
	{
		case SORT_UP_NAME:
		case SORT_DOWN_NAME:
			ret = (strcmp( ::nl_combine[lp1].name, ::nl_combine[lp2].name ));
			break;

		case SORT_UP_WIDTH:
		case SORT_DOWN_WIDTH:
			if( ::nl_combine[lp1].w > ::nl_combine[lp2].w )
				ret = 1;
			else if( ::nl_combine[lp1].w < ::nl_combine[lp2].w )
				ret = -1;
			break;

		case SORT_UP_VIA_W:
		case SORT_DOWN_VIA_W:
			if( ::nl_combine[lp1].v_w > ::nl_combine[lp2].v_w )
				ret = 1;
			else if( ::nl_combine[lp1].v_w < ::nl_combine[lp2].v_w )
				ret = -1;
			break;

		case SORT_UP_HOLE_W:
		case SORT_DOWN_HOLE_W:
			if( ::nl_combine[lp1].v_h_w > ::nl_combine[lp2].v_h_w )
				ret = 1;
			else if( ::nl_combine[lp1].v_h_w < ::nl_combine[lp2].v_h_w )
				ret = -1;
			break;

		case SORT_UP_PINS:
		case SORT_DOWN_PINS:
			if( ::nl_combine[lp1].ref_des.GetSize() > ::nl_combine[lp2].ref_des.GetSize() )
				ret = 1;
			else if( ::nl_combine[lp1].ref_des.GetSize() < ::nl_combine[lp2].ref_des.GetSize() )
				ret = -1;
			break;
	}
	switch( type )
	{
		case SORT_DOWN_NAME:
		case SORT_DOWN_WIDTH:
		case SORT_DOWN_VIA_W:
		case SORT_DOWN_HOLE_W:
		case SORT_DOWN_PINS:
			ret = -ret;
			break;
	}

	return ret;
}

// CDlgNetCombine dialog

IMPLEMENT_DYNAMIC(CDlgNetCombine, CDialog)
CDlgNetCombine::CDlgNetCombine(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgNetCombine::IDD, pParent)
{
	m_w = 0;
	m_v_w = 0;
	m_v_h_w = 0;
	m_nlist = 0;
}

CDlgNetCombine::~CDlgNetCombine()
{
}

void CDlgNetCombine::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_NET, m_list_ctrl);
	if( pDX->m_bSaveAndValidate )
	{
		
	}
	else
	{
		
	}
}


BEGIN_MESSAGE_MAP(CDlgNetCombine, CDialog)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST_NET, OnLvnColumnclickListNet)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
END_MESSAGE_MAP()


void CDlgNetCombine::Initialize( CNetList * nlist, CPartList * plist )
{
	m_nlist = nlist;
	m_plist = plist;
}

// CDlgNetCombine message handlers

BOOL CDlgNetCombine::OnInitDialog()
{
	CDialog::OnInitDialog();

	// make copy of netlist data so that it can be edited but user can still cancel
	// use global netlist_info so it can be sorted in the list control
	m_nl = &::nl_combine;
	m_nlist->ExportNetListInfo( m_nl );

	// initialize netlist control
	m_item_selected = -1;
	m_sort_type = 0;
	m_visible_state = 1;
	DrawListCtrl();
	return TRUE;  
}

// draw listview control and sort according to m_sort_type
//
void CDlgNetCombine::DrawListCtrl()
{
	int nItem;
	CString str;
	DWORD old_style = m_list_ctrl.GetExtendedStyle(); 
	m_list_ctrl.SetExtendedStyle( LVS_EX_FULLROWSELECT | LVS_EX_FLATSB | old_style );
	m_list_ctrl.DeleteAllItems();
	m_list_ctrl.InsertColumn( COL_NAME, "Name", LVCFMT_LEFT, 140 );
	m_list_ctrl.InsertColumn( COL_PINS, "Pins", LVCFMT_LEFT, 40 );
	m_list_ctrl.InsertColumn( COL_WIDTH, "Width", LVCFMT_LEFT, 40 );
	m_list_ctrl.InsertColumn( COL_VIA_W, "Via W", LVCFMT_LEFT, 40 );   
	m_list_ctrl.InsertColumn( COL_HOLE_W, "Hole", LVCFMT_LEFT, 40 );
	int iItem = 0;
	for( int i=0; i<::nl_combine.GetSize(); i++ )
	{
		nItem = m_list_ctrl.InsertItem( iItem, "" );
		m_list_ctrl.SetItemData( iItem, (LPARAM)i );
		m_list_ctrl.SetItem( iItem, COL_NAME, LVIF_TEXT, ::nl_combine[i].name, 0, 0, 0, 0 );
		str.Format( "%d", ::nl_combine[i].ref_des.GetSize() );
		m_list_ctrl.SetItem( iItem, COL_PINS, LVIF_TEXT, str, 0, 0, 0, 0 );
		str.Format( "%d", ::nl_combine[i].w/NM_PER_MIL );
		m_list_ctrl.SetItem( iItem, COL_WIDTH, LVIF_TEXT, str, 0, 0, 0, 0 );
		str.Format( "%d", ::nl_combine[i].v_w/NM_PER_MIL );
		m_list_ctrl.SetItem( iItem, COL_VIA_W, LVIF_TEXT, str, 0, 0, 0, 0 );
		str.Format( "%d", ::nl_combine[i].v_h_w/NM_PER_MIL );
		m_list_ctrl.SetItem( iItem, COL_HOLE_W, LVIF_TEXT, str, 0, 0, 0, 0 );
		ListView_SetCheckState( m_list_ctrl, nItem, ::nl_combine[i].visible );
	}
	m_list_ctrl.SortItems( ::CompareNetlistCombine, m_sort_type );
	if( ::nl_combine.GetSize() == 2 )
	{
		m_list_ctrl.SetItemState( 0, LVIS_SELECTED, LVIS_SELECTED );
		m_list_ctrl.SetItemState( 1, LVIS_SELECTED, LVIS_SELECTED );
		::ShowCursor( TRUE );	// force cursor
		OnBnClickedOk();
	}
}

void CDlgNetCombine::OnLvnColumnclickListNet(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	int column = pNMLV->iSubItem;
	if( column == COL_NAME )
	{
		if( m_sort_type == SORT_UP_NAME )
			m_sort_type = SORT_DOWN_NAME;
		else
			m_sort_type = SORT_UP_NAME;
		m_list_ctrl.SortItems( ::CompareNetlistCombine, m_sort_type );
	}
	else if( column == COL_WIDTH )
	{
		if( m_sort_type == SORT_UP_WIDTH )
			m_sort_type = SORT_DOWN_WIDTH;
		else
			m_sort_type = SORT_UP_WIDTH;
		m_list_ctrl.SortItems( ::CompareNetlistCombine, m_sort_type );
	}
	else if( column == COL_VIA_W )
	{
		if( m_sort_type == SORT_UP_VIA_W )
			m_sort_type = SORT_DOWN_VIA_W;
		else
			m_sort_type = SORT_UP_VIA_W;
		m_list_ctrl.SortItems( ::CompareNetlistCombine, m_sort_type );
	}
	else if( column == COL_HOLE_W )
	{
		if( m_sort_type == SORT_UP_HOLE_W )
			m_sort_type = SORT_DOWN_HOLE_W;
		else
			m_sort_type = SORT_UP_HOLE_W;
		m_list_ctrl.SortItems( ::CompareNetlistCombine, m_sort_type );
	}
	else if( column == COL_PINS )
	{
		if( m_sort_type == SORT_UP_PINS )
			m_sort_type = SORT_DOWN_PINS;
		else
			m_sort_type = SORT_UP_PINS;
		m_list_ctrl.SortItems( ::CompareNetlistCombine, m_sort_type );
	}
	*pResult = 0;
}


void CDlgNetCombine::OnBnClickedOk()
{
	char ch[41];

	int n = m_list_ctrl.GetSelectedCount();
	if( n == 0 )
	{
		AfxMessageBox( "No nets selected" );
		return;
	}
	m_names.SetSize(n);
	int i = 0;
	POSITION pos = m_list_ctrl.GetFirstSelectedItemPosition();
	while( pos )
	{
		int nItem = m_list_ctrl.GetNextSelectedItem(pos);
		m_list_ctrl.GetItemText( nItem, 0, ch, 40 );
		m_names[i++] = ch;
	}
	CDlgChooseNetName dlg;
	dlg.Initialize( &m_names );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		m_new_name = dlg.m_sel_str;
		OnOK();
	}
	else
	{
		OnCancel();
		//return;
	}
}

void CDlgNetCombine::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	OnCancel();
}

