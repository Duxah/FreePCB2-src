// DlgPartlist.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgPartlist.h"
#include "DlgAddPart.h"

//global partlist_info so that sorting callbacks will work
partlist_info pl;

// columns for list
enum {
	COL_VIS = 0,
	COL_NAME,
	COL_PACKAGE,
	COL_FOOTPRINT,
	COL_VALUE
};

// sort types
enum {
	SORT_UP_NAME = 0,
	SORT_DOWN_NAME,
	SORT_UP_PACKAGE,
	SORT_DOWN_PACKAGE,
	SORT_UP_FOOTPRINT,
	SORT_DOWN_FOOTPRINT,
	SORT_UP_VALUE,
	SORT_DOWN_VALUE
};

// global callback function for sorting
// lp1, lp2 are indexes to global arrays above
//===============================================================================================	
int CALLBACK ComparePartlist( LPARAM lp1, LPARAM lp2, LPARAM type )
{
	int ret = 0;
	switch( type )
	{
		case SORT_UP_NAME:
		case SORT_DOWN_NAME:
			ret = (strcmp( ::pl[lp1].ref_des, ::pl[lp2].ref_des ));
			break;

		case SORT_UP_PACKAGE:
		case SORT_DOWN_PACKAGE:
			if( ::pl[lp1].shape && ::pl[lp2].shape )
				ret = (strcmp( ::pl[lp1].shape->m_package, ::pl[lp2].shape->m_package ));
			else
				ret = 0;
			break;

		case SORT_UP_FOOTPRINT:
		case SORT_DOWN_FOOTPRINT:
			if( ::pl[lp1].shape && ::pl[lp2].shape )
				ret = (strcmp( ::pl[lp1].shape->m_name, ::pl[lp2].shape->m_name ));
			else
				ret = 0;
			break;

		case SORT_UP_VALUE:
		case SORT_DOWN_VALUE:
			ret = (strcmp( ::pl[lp1].value, ::pl[lp2].value ));
			break;

	}
	switch( type )
	{
		case SORT_DOWN_NAME:
		case SORT_DOWN_PACKAGE:
		case SORT_DOWN_FOOTPRINT:
		case SORT_DOWN_VALUE:
			ret = -ret;
			break;
	}
	return ret;
}

// CDlgPartlist dialog
//===============================================================================================
IMPLEMENT_DYNAMIC(CDlgPartlist, CDialog)
CDlgPartlist::CDlgPartlist(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgPartlist::IDD, pParent)
{
}

CDlgPartlist::~CDlgPartlist()
{
}
//===============================================================================================
void CDlgPartlist::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_list_ctrl);
	DDX_Control(pDX, IDC_BUTTON_ADD, m_button_add);
	DDX_Control(pDX, IDC_BUTTON_EDIT, m_button_edit);
	DDX_Control(pDX, IDC_BUTTON_DELETE, m_button_delete);
	if( pDX->m_bSaveAndValidate )
	{
		// leaving, get value_vis checkbox states
		for (int iItem=0; iItem<m_list_ctrl.GetItemCount(); iItem++ )
		{
			int ip = m_list_ctrl.GetItemData( iItem );
			BOOL iTest = ListView_GetCheckState( m_list_ctrl, iItem );
			::pl[ip].value_vis = ListView_GetCheckState( m_list_ctrl, iItem );
		}
		m_plist->ImportPartListInfo( &::pl, 0 );
	}
	DDX_Control(pDX, IDC_CHECK1, m_check_footprint);
	DDX_Control(pDX, IDC_CHECK3, m_check_value);
}

//===============================================================================================
BEGIN_MESSAGE_MAP(CDlgPartlist, CDialog)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnBnClickedButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_EDIT, OnBnClickedButtonEdit)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, OnBnClickedButtonDelete)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST1, OnLvnColumnClickList1)
	ON_BN_CLICKED(IDC_BUTTON_VAL_VIS, OnBnClickedValueVisible)
	ON_BN_CLICKED(IDC_BUTTON_VAL_INVIS, OnBnClickedValueInvisible)
	ON_NOTIFY(NM_CLICK, IDC_LIST1, OnNMClickList1)
END_MESSAGE_MAP()
//===============================================================================================
BOOL CDlgPartlist::OnInitDialog()
{
	CDialog::OnInitDialog();
	m_plist->ExportPartListInfo( &::pl, NULL );
	m_sort_type = SORT_UP_NAME;
	m_check_footprint.EnableWindow(0);
	m_check_value.EnableWindow(0);
	DrawListCtrl();
	return TRUE;
}
//===============================================================================================
void CDlgPartlist::DrawListCtrl()
{
	// now set up listview control
	int nItem;
	LVITEM lvitem;
	CString str;
	DWORD old_style = m_list_ctrl.GetExtendedStyle();
	m_list_ctrl.SetExtendedStyle( LVS_EX_FULLROWSELECT | LVS_EX_FLATSB | LVS_EX_CHECKBOXES | old_style );
	m_list_ctrl.DeleteAllItems();
	m_list_ctrl.InsertColumn( COL_VIS, "Value Vis", LVCFMT_LEFT, 60 );
	m_list_ctrl.InsertColumn( COL_NAME, "Reference", LVCFMT_LEFT, 70 );
	m_list_ctrl.InsertColumn( COL_PACKAGE, "Package", LVCFMT_LEFT, 150 );
	m_list_ctrl.InsertColumn( COL_FOOTPRINT, "Footprint", LVCFMT_LEFT, 150 );
	m_list_ctrl.InsertColumn( COL_VALUE, "Value", LVCFMT_LEFT, 200 );
	m_n_sel = 0;
	for( int i=0; i<::pl.GetSize(); i++ )
	{
		lvitem.mask = LVIF_TEXT | LVIF_PARAM;
		lvitem.pszText = "";
		lvitem.lParam = i;
		nItem = m_list_ctrl.InsertItem( i, "" );
		m_list_ctrl.SetItemData( i, (LPARAM)i );
		ListView_SetCheckState( m_list_ctrl, nItem, ::pl[i].value_vis );
		m_list_ctrl.SetItem( i, COL_NAME, LVIF_TEXT, ::pl[i].ref_des, 0, 0, 0, 0 );
		if( ::pl[i].shape )
		{
			m_list_ctrl.SetItem( i, COL_FOOTPRINT, LVIF_TEXT, ::pl[i].shape->m_name, 0, 0, 0, 0 );
			m_list_ctrl.SetItem( i, COL_PACKAGE, LVIF_TEXT, pl[i].shape->m_package, 0, 0, 0, 0 );
		}
		else
		{
			m_list_ctrl.SetItem( i, COL_FOOTPRINT, LVIF_TEXT, "", 0, 0, 0, 0 );
			m_list_ctrl.SetItem( i, COL_PACKAGE, LVIF_TEXT, "", 0, 0, 0, 0 );
		}
		m_list_ctrl.SetItem( i, COL_VALUE, LVIF_TEXT, ::pl[i].value, 0, 0, 0, 0 );
		if (::pl[i].selected)
		{
			m_list_ctrl.SetItemState( i, LVIS_SELECTED, LVIS_SELECTED );
			m_n_sel++;
		}
	}
	m_list_ctrl.SortItems( ::ComparePartlist, m_sort_type );	// resort 
	RestoreSelections();
	if (m_n_sel > 1)
	{
		m_check_footprint.EnableWindow();
		m_check_value.EnableWindow();
	}
	if( m_n_sel > 1 || !m_lock_netlist )
		m_button_edit.EnableWindow(TRUE);
	else
		m_button_edit.EnableWindow(FALSE);
	m_button_delete.EnableWindow(!m_lock_netlist);
}
//===============================================================================================
void CDlgPartlist::Initialize( CPartList * plist,
			CMapStringToPtr * shape_cache_map,
			CFootLibFolderMap * footlibfoldermap,
			int units, CDlgLog * log, int lock_netlist )
{
	m_n_sel = 0;
	m_units = units;
	m_plist = plist;
	m_footprint_cache_map = shape_cache_map;
	m_footlibfoldermap = footlibfoldermap;
	m_sort_type = SORT_UP_NAME;
	m_dlg_log = log;
	m_lock_netlist = lock_netlist;
}

// CDlgPartlist message handlers
//===============================================================================================
void CDlgPartlist::OnBnClickedButtonEdit()
{
	SaveSelections();
	// save value_vis checkbox states
	for (int Item=0; Item<m_list_ctrl.GetItemCount(); Item++ )
	{
		int ip = m_list_ctrl.GetItemData( Item );
		::pl[ip].value_vis = ListView_GetCheckState( m_list_ctrl, Item );
	}
	// edit selected part(s)
	int n_sel = m_list_ctrl.GetSelectedCount();
	if( n_sel == 0 )
	{
		AfxMessageBox( "You have no part selected" );
		return;
	}
	BOOL bMultiple = FALSE;
	if( n_sel > 1 )
		bMultiple = TRUE;

	POSITION pos = m_list_ctrl.GetFirstSelectedItemPosition(); 
	if (pos == NULL)
		ASSERT(0);
	int iItem = m_list_ctrl.GetNextSelectedItem(pos);
	int i = m_list_ctrl.GetItemData( iItem );
	CDlgAddPart dlg;
	int multiple_mask = MSK_FOOTPRINT	* m_check_footprint.GetCheck()
					  + MSK_VALUE		* m_check_value.GetCheck(); 
	if( bMultiple && multiple_mask == 0 )
	{
		AfxMessageBox( "To edit multiple parts, please select Footprint or Value" );
		return;
	}
	dlg.Initialize( &::pl, i, FALSE, FALSE, bMultiple, multiple_mask, m_lock_netlist,
		m_footprint_cache_map, m_footlibfoldermap, m_units, m_dlg_log );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		CString str;
		if( bMultiple )
		{
			// update all selected parts with new package and footprint
			POSITION pos = m_list_ctrl.GetFirstSelectedItemPosition();
			while( pos )
			{
				int iItem = m_list_ctrl.GetNextSelectedItem(pos);
				int ip = m_list_ctrl.GetItemData( iItem );
				if( ip != i )
				{
					if( multiple_mask & MSK_FOOTPRINT )
					{
						::pl[ip].shape = ::pl[i].shape;
						::pl[ip].ref_size = ::pl[i].ref_size;
						::pl[ip].ref_width = ::pl[i].ref_width;
					}
					if( multiple_mask & MSK_VALUE )
						::pl[ip].value = ::pl[i].value;
				}
			}
		}
		DrawListCtrl();
	}
}
//===============================================================================================
void CDlgPartlist::OnBnClickedButtonAdd()
{
	SaveSelections();
	// save value_vis checkbox states
	for (int Item=0; Item<m_list_ctrl.GetItemCount(); Item++ )
	{
		int ip = m_list_ctrl.GetItemData( Item );
		::pl[ip].value_vis = ListView_GetCheckState( m_list_ctrl, Item );
	}
	// now add part
	CDlgAddPart dlg;
	dlg.Initialize( &::pl, -1, FALSE, TRUE, FALSE, 0, m_lock_netlist,
		m_footprint_cache_map, m_footlibfoldermap, m_units, m_dlg_log );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		DrawListCtrl();
	}
}
//===============================================================================================
void CDlgPartlist::OnBnClickedButtonDelete()
{
	int n_sel = m_list_ctrl.GetSelectedCount();
	if( n_sel == 0 )
		AfxMessageBox( "You have no part selected" );
	else
	{
		while( m_list_ctrl.GetSelectedCount() )
		{
			POSITION pos = m_list_ctrl.GetFirstSelectedItemPosition();
			if (pos == NULL)
				ASSERT(0);
			int iItem = m_list_ctrl.GetNextSelectedItem(pos);
			int ip = m_list_ctrl.GetItemData( iItem );
			::pl[ip].deleted = TRUE;
			m_list_ctrl.DeleteItem( iItem );
		}
	}
}

// set m_sort_type based on column clicked and last sort, 
// then sort the list, then save m_last_sort_type = m_sort_type
//===============================================================================================
void CDlgPartlist::OnLvnColumnClickList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	int column = pNMLV->iSubItem;
	if( column == COL_NAME )
	{
		if( m_sort_type == SORT_UP_NAME )
			m_sort_type = SORT_DOWN_NAME;
		else
			m_sort_type = SORT_UP_NAME;
		m_list_ctrl.SortItems( ::ComparePartlist, m_sort_type );
	}
	else if( column == COL_PACKAGE )
	{
		if( m_sort_type == SORT_UP_PACKAGE )
			m_sort_type = SORT_DOWN_PACKAGE;
		else
			m_sort_type = SORT_UP_PACKAGE;
		m_list_ctrl.SortItems( ::ComparePartlist, m_sort_type );
	}
	else if( column == COL_FOOTPRINT )
	{
		if( m_sort_type == SORT_UP_FOOTPRINT )
			m_sort_type = SORT_DOWN_FOOTPRINT;
		else
			m_sort_type = SORT_UP_FOOTPRINT;
		m_list_ctrl.SortItems( ::ComparePartlist, m_sort_type );
	}
	else if( column == COL_VALUE )
	{
		if( m_sort_type == SORT_UP_VALUE )
			m_sort_type = SORT_DOWN_VALUE;
		else
			m_sort_type = SORT_UP_VALUE;
		m_list_ctrl.SortItems( ::ComparePartlist, m_sort_type );
	}
	*pResult = 0;
}

void CDlgPartlist::OnBnClickedValueVisible()
{
	SaveSelections();
	POSITION pos = m_list_ctrl.GetFirstSelectedItemPosition();
	while( pos )
	{
		int iItem = m_list_ctrl.GetNextSelectedItem(pos);
		int ip = m_list_ctrl.GetItemData( iItem );
		::pl[ip].value_vis = TRUE;
	}
	DrawListCtrl();
	RestoreSelections();
}
//===============================================================================================
void CDlgPartlist::OnBnClickedValueInvisible()
{
	SaveSelections();
	POSITION pos = m_list_ctrl.GetFirstSelectedItemPosition(); 
	while( pos )
	{
		int iItem = m_list_ctrl.GetNextSelectedItem(pos);
		int ip = m_list_ctrl.GetItemData( iItem );
		::pl[ip].value_vis = FALSE;
	}
	DrawListCtrl();
}

//===============================================================================================
void CDlgPartlist::OnNMClickList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	SaveSelections();
	static int last_n_sel = 0;
	m_n_sel = m_list_ctrl.GetSelectedCount();
	if( m_n_sel != last_n_sel )
	{
		m_check_footprint.EnableWindow( m_n_sel > 1 );
		m_check_value.EnableWindow( m_n_sel > 1 );
	}
	m_button_edit.EnableWindow( m_n_sel );
	m_button_delete.EnableWindow( !m_lock_netlist );
	last_n_sel = m_n_sel;
	*pResult = 0;
}
//===============================================================================================
void CDlgPartlist::SaveSelections()
{
	int nItems = m_list_ctrl.GetItemCount();
	bSelected.SetSize( ::pl.GetSize() );
	for( int iItem=0; iItem<nItems; iItem++ )
	{
		int ip = m_list_ctrl.GetItemData( iItem );
		if( m_list_ctrl.GetItemState( iItem, LVIS_SELECTED ) == LVIS_SELECTED )
		{
			bSelected[ip] = TRUE;
			::pl[ip].selected = TRUE;
		}
		else
		{
			bSelected[ip] = FALSE;
			::pl[ip].selected = FALSE;
		}
	}
}
//===============================================================================================
void CDlgPartlist::RestoreSelections()
{

	int nItems = m_list_ctrl.GetItemCount();
	for( int iItem=0; iItem<nItems; iItem++ )
	{
		int ip = m_list_ctrl.GetItemData( iItem );
		if( ip < bSelected.GetSize() )
			if( bSelected[ip] == TRUE )
				m_list_ctrl.SetItemState( iItem, LVIS_SELECTED, LVIS_SELECTED );
	}
	bSelected.SetSize(0);
}

