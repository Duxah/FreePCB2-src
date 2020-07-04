// DlgGroupPaste.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgGroupPaste.h"

// columns for list
enum {
	COL_VIS = 0,
	COL_NAME,
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

// global so that it is available to ListCompare() for sorting list control items
netlist_info gnl;

// global callback function for sorting
// lp1, lp2 are indexes to global arrays above
//		
int CALLBACK ListCompare( LPARAM lp1, LPARAM lp2, LPARAM type )
{
	int ret = 0;
	switch( type )
	{
		case SORT_UP_NAME:
		case SORT_DOWN_NAME:
			ret = (strcmp( ::gnl[lp1].name, ::gnl[lp2].name ));
			break;

		case SORT_UP_WIDTH:
		case SORT_DOWN_WIDTH:
			if( ::gnl[lp1].w > ::gnl[lp2].w )
				ret = 1;
			else if( ::gnl[lp1].w < ::gnl[lp2].w )
				ret = -1;
			break;

		case SORT_UP_VIA_W:
		case SORT_DOWN_VIA_W:
			if( ::gnl[lp1].v_w > ::gnl[lp2].v_w )
				ret = 1;
			else if( ::gnl[lp1].v_w < ::gnl[lp2].v_w )
				ret = -1;
			break;

		case SORT_UP_HOLE_W:
		case SORT_DOWN_HOLE_W:
			if( ::gnl[lp1].v_h_w > ::gnl[lp2].v_h_w )
				ret = 1;
			else if( ::gnl[lp1].v_h_w < ::gnl[lp2].v_h_w )
				ret = -1;
			break;

		case SORT_UP_PINS:
		case SORT_DOWN_PINS:
			if( ::gnl[lp1].ref_des.GetSize() > ::gnl[lp2].ref_des.GetSize() )
				ret = 1;
			else if( ::gnl[lp1].ref_des.GetSize() < ::gnl[lp2].ref_des.GetSize() )
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

// CDlgGroupPaste dialog

IMPLEMENT_DYNAMIC(CDlgGroupPaste, CDialog)
CDlgGroupPaste::CDlgGroupPaste(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgGroupPaste::IDD, pParent)
{
}

CDlgGroupPaste::~CDlgGroupPaste()
{
}

void CDlgGroupPaste::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_RADIO_USE_GROUP_REF, m_radio_use_group_ref);
	DDX_Control(pDX, IDC_RADIO_USE_NEXT_REF, m_radio_use_next_ref);
	DDX_Control(pDX, IDC_RADIO_ADD_REF_OFFSET, m_radio_add_ref_offset);
	DDX_Control(pDX, IDC_EDIT_REF_OFFSET, m_edit_offset);
	DDX_Control(pDX, IDC_RADIO_USE_GROUP_NETS, m_radio_use_group_nets);
	DDX_Control(pDX, IDC_RADIO_DEL_GROUP_NETS, m_radio_del_group_nets);
	DDX_Control(pDX, IDC_RADIO_USE_SELECTED_NETS, m_radio_use_selected_nets);
	DDX_Control(pDX, IDC_LIST_SELECT_GROUP_NETS, m_list_ctrl);
	DDX_Control(pDX, IDC_RADIO_USE_SUFFIX, m_radio_use_suffix);
	DDX_Control(pDX, IDC_RADIO_MAKE_NEW_NAMES, m_radio_make_new_names);
	DDX_Control(pDX, IDC_RADIO_DRAG, m_radio_drag);
	DDX_Control(pDX, IDC_RADIO_OFFSET, m_radio_offset);
	DDX_Control(pDX, IDC_EDIT_GROUP_X, m_edit_x);
	DDX_Control(pDX, IDC_EDIT_GROUP_Y, m_edit_y);
	DDX_Control(pDX, IDC_COMBO_GROUP_UNITS, m_combo_units);
	DDX_Control(pDX, IDC_RADIO_RETAIN_ALL, m_radio_retain_all_nets);
	DDX_Control(pDX, IDC_RADIO_RETAIN_TRACES, m_radio_retain_traces);
	DDX_Control(pDX, IDC_REF_TO_TEXT, m_ref_to_text);
	DDX_Control(pDX, IDC_VALUE_TO_TEXT, m_value_to_text);
	DDX_Text( pDX, IDC_EDIT_GROUP_X, m_dx );
	DDX_Text( pDX, IDC_EDIT_GROUP_Y, m_dy );
	DDX_Text( pDX, IDC_EDIT_REF_OFFSET, m_ref_offset );
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		m_sort_type = 0;
		m_radio_use_selected_nets.SetCheck(1);
		m_radio_use_next_ref.SetCheck(1); 
		m_radio_use_suffix.SetCheck(1);
		m_radio_drag.SetCheck(1);
		m_radio_retain_all_nets.SetCheck(1);
		m_combo_units.AddString( "MM" );
		m_combo_units.AddString( "MIL" );
		m_combo_units.SelectString( 0, "MIL" );
		SetFields();
		// now set up listview control
		int nItem;
		LVITEM lvitem;
		CString str;
		DWORD old_style = m_list_ctrl.GetExtendedStyle();
		m_list_ctrl.SetExtendedStyle( LVS_EX_FULLROWSELECT | LVS_EX_FLATSB | LVS_EX_CHECKBOXES | old_style );
		m_list_ctrl.InsertColumn( COL_VIS, "Sel", LVCFMT_LEFT, 30 ); 
		m_list_ctrl.InsertColumn( COL_NAME, "Name", LVCFMT_LEFT, 140 );
		m_list_ctrl.InsertColumn( COL_PINS, "Pins", LVCFMT_LEFT, 40 );
		m_list_ctrl.InsertColumn( COL_WIDTH, "Width", LVCFMT_LEFT, 55 );
		m_list_ctrl.InsertColumn( COL_VIA_W, "Via W", LVCFMT_LEFT, 55 );   
		m_list_ctrl.InsertColumn( COL_HOLE_W, "Hole", LVCFMT_LEFT, 55 );
		if( m_grp_nlist )
		{
			for( int i=0; i<gnl.GetSize(); i++ )
			{
				lvitem.mask = LVIF_TEXT | LVIF_PARAM;
				lvitem.pszText = "";
				lvitem.lParam = i;
				nItem = m_list_ctrl.InsertItem( i, "" );
				m_list_ctrl.SetItemData( i, (LPARAM)i );
				m_list_ctrl.SetItem( i, COL_NAME, LVIF_TEXT, ::gnl[i].name, 0, 0, 0, 0 );
				str.Format( "%d", ::gnl[i].ref_des.GetSize() );
				m_list_ctrl.SetItem( i, COL_PINS, LVIF_TEXT, str, 0, 0, 0, 0 );
				str.Format( "%d", ::gnl[i].w/NM_PER_MIL );
				m_list_ctrl.SetItem( i, COL_WIDTH, LVIF_TEXT, str, 0, 0, 0, 0 );
				str.Format( "%d", ::gnl[i].v_w/NM_PER_MIL );
				m_list_ctrl.SetItem( i, COL_VIA_W, LVIF_TEXT, str, 0, 0, 0, 0 );
				str.Format( "%d", ::gnl[i].v_h_w/NM_PER_MIL );
				m_list_ctrl.SetItem( i, COL_HOLE_W, LVIF_TEXT, str, 0, 0, 0, 0 );
				ListView_SetCheckState( m_list_ctrl, nItem, 0 );
			}
		}
		CRect rc;
		GetWindowRect(&rc); // getting dialog coordinates
		if( m_grp_nlist == NULL )
		{
			MoveWindow(rc.left, rc.top, rc.Width()/2+12, rc.Height() );
			m_radio_del_group_nets.SetCheck(1);
			m_radio_retain_all_nets.SetCheck(0);
			m_radio_retain_traces.SetCheck(0);
			m_radio_retain_all_nets.EnableWindow(0);
			m_radio_retain_traces.EnableWindow(0);
			m_radio_del_group_nets.EnableWindow(0);
		}
	}
	else
	{
		// outgoing
		SetFields();
		CString str;
		m_combo_units.GetWindowText( str );
		if( str.Compare("MIL") == 0 )
		{
			m_dx *= NM_PER_MIL;
			m_dy *= NM_PER_MIL;
		}
		else
		{
			m_dx *= 1000000;
			m_dy *= 1000000;
		}
		// export data into netlist
		if( m_grp_nlist )
		{
			for( int iItem=0; iItem<m_list_ctrl.GetItemCount(); iItem++ )
			{
				int i = m_list_ctrl.GetItemData( iItem ); 
				CString * net_name = &::gnl[i].name;
				cnet * grp_net = m_grp_nlist->GetNetPtrByName( net_name );
				if( grp_net == NULL )
					ASSERT(0);
				grp_net->utility = ListView_GetCheckState( m_list_ctrl, iItem );
			}
		}
		::gnl.SetSize(0);
	}
}



BEGIN_MESSAGE_MAP(CDlgGroupPaste, CDialog)
	ON_BN_CLICKED(IDC_RADIO_USE_GROUP_REF, OnBnClickedRadioUseGroupRef)
	ON_BN_CLICKED(IDC_RADIO_USE_NEXT_REF, OnBnClickedRadioUseNextRef)
	ON_BN_CLICKED(IDC_RADIO_ADD_REF_OFFSET, OnBnClickedRadioAddRefOffset)
	ON_BN_CLICKED(IDC_RADIO_USE_GROUP_NETS, OnBnClickedRadioUseGroupNets)
	ON_BN_CLICKED(IDC_RADIO_USE_SELECTED_NETS, OnBnClickedRadioUseSelectedNets)
	ON_BN_CLICKED(IDC_RADIO_USE_SUFFIX, OnBnClickedRadioUseSuffix)
	ON_BN_CLICKED(IDC_RADIO_MAKE_NEW_NAMES, OnBnClickedRadioMakeNewNames)
	ON_BN_CLICKED(IDC_RADIO_DRAG, OnBnClickedRadioDrag)
	ON_BN_CLICKED(IDC_RADIO_OFFSET, OnBnClickedRadioOffset)
	ON_CBN_SELCHANGE(IDC_COMBO_GROUP_UNITS, OnCbnSelchangeComboGroupUnits)
	ON_BN_CLICKED(IDC_RADIO_RETAIN_ALL, OnBnClickedRadioRetainAll)
	ON_BN_CLICKED(IDC_RADIO_RETAIN_TRACES, OnBnClickedRadioRetainTraces)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST_SELECT_GROUP_NETS, OnLvnColumnClickListSelectGroupNets)
END_MESSAGE_MAP()

void CDlgGroupPaste::Initialize( CNetList * grp_nlist )
{
	m_grp_nlist = grp_nlist;
	m_ref_offset = 0;
	m_ref_as_text = 0;
	m_value_as_text = 0;
	m_dx = 0;
	m_dy = 0;
	if( m_grp_nlist )
		grp_nlist->ExportNetListInfo( &gnl );
}

void CDlgGroupPaste::SetFields()
{
	if( m_ref_to_text.GetCheck() )
	{
		m_ref_as_text = 1;
	}
	if( m_value_to_text.GetCheck() )
	{
		m_value_as_text = 1;
	}
	// set values and enable/disable items based on selections
	if( m_radio_use_group_ref.GetCheck() )
	{
		m_ref_option = 0;
		m_edit_offset.EnableWindow(0);
	}
	else if( m_radio_use_next_ref.GetCheck() )
	{
		m_ref_option = 1;
		m_edit_offset.EnableWindow(0);
	}
	else if( m_radio_add_ref_offset.GetCheck() )
	{
		m_ref_option = 2;
		m_edit_offset.EnableWindow(1);
	}
	//
	if( m_radio_use_group_nets.GetCheck() )
	{
		m_net_name_option = 0;
		m_list_ctrl.EnableWindow(0);
		m_radio_use_suffix.EnableWindow(0);
		m_radio_make_new_names.EnableWindow(0);
	}
	else if( m_radio_use_selected_nets.GetCheck() )
	{
		m_net_name_option = 1;
		m_list_ctrl.EnableWindow(1);
		m_radio_use_suffix.EnableWindow(1);
		m_radio_make_new_names.EnableWindow(1);
	}
	//
	if( m_radio_del_group_nets.GetCheck() )
	{
		m_net_name_option = 2;
		m_list_ctrl.EnableWindow(0);
		m_radio_use_suffix.EnableWindow(0);
		m_radio_make_new_names.EnableWindow(0);
	}
	//
	if( m_radio_drag.GetCheck() )
	{
		m_position_option = 0;
		m_edit_x.EnableWindow(0);
		m_edit_y.EnableWindow(0);
		m_combo_units.EnableWindow(0);
	}
	else if( m_radio_offset.GetCheck() )
	{
		m_position_option = 1;
		m_edit_x.EnableWindow(1);
		m_edit_y.EnableWindow(1);
		m_combo_units.EnableWindow(1);
	}
	if( m_radio_use_suffix.GetCheck() )
	{
		m_net_rename_option = 0;
	}
	else if( m_radio_make_new_names.GetCheck() )
	{
		m_net_rename_option = 1;
	}
	if( m_radio_retain_all_nets.GetCheck() )
	{
		m_pin_net_option = 0;
	}
	else
	{
		m_pin_net_option = 1;
	}
}

// CDlgGroupPaste message handlers

void CDlgGroupPaste::OnBnClickedRadioUseGroupRef()
{
	SetFields();
}

void CDlgGroupPaste::OnBnClickedRadioUseNextRef()
{
	SetFields();
}

void CDlgGroupPaste::OnBnClickedRadioAddRefOffset()
{
	SetFields();
}

void CDlgGroupPaste::OnBnClickedRadioUseGroupNets()
{
	SetFields();
}

void CDlgGroupPaste::OnBnClickedRadioUseSelectedNets()
{
	SetFields();
}

void CDlgGroupPaste::OnBnClickedRadioUseSuffix()
{
	SetFields();
}

void CDlgGroupPaste::OnBnClickedRadioMakeNewNames()
{
	SetFields();
}

void CDlgGroupPaste::OnBnClickedRadioDrag()
{
	SetFields();
}

void CDlgGroupPaste::OnBnClickedRadioOffset()
{
	SetFields();
}

void CDlgGroupPaste::OnCbnSelchangeComboGroupUnits()
{
	// TODO: Add your control notification handler code here
}

void CDlgGroupPaste::OnBnClickedRadioRetainAll()
{
	// TODO: Add your control notification handler code here
}

void CDlgGroupPaste::OnBnClickedRadioRetainTraces()
{
	// TODO: Add your control notification handler code here
}

void CDlgGroupPaste::OnLvnColumnClickListSelectGroupNets(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	int column = pNMLV->iSubItem;
	if( column == COL_NAME )
	{
		if( m_sort_type == SORT_UP_NAME )
			m_sort_type = SORT_DOWN_NAME;
		else
			m_sort_type = SORT_UP_NAME;
		m_list_ctrl.SortItems( ::ListCompare, m_sort_type );
	}
	else if( column == COL_WIDTH )
	{
		if( m_sort_type == SORT_UP_WIDTH )
			m_sort_type = SORT_DOWN_WIDTH;
		else
			m_sort_type = SORT_UP_WIDTH;
		m_list_ctrl.SortItems( ::ListCompare, m_sort_type );
	}
	else if( column == COL_VIA_W )
	{
		if( m_sort_type == SORT_UP_VIA_W )
			m_sort_type = SORT_DOWN_VIA_W;
		else
			m_sort_type = SORT_UP_VIA_W;
		m_list_ctrl.SortItems( ::ListCompare, m_sort_type );
	}
	else if( column == COL_HOLE_W )
	{
		if( m_sort_type == SORT_UP_HOLE_W )
			m_sort_type = SORT_DOWN_HOLE_W;
		else
			m_sort_type = SORT_UP_HOLE_W;
		m_list_ctrl.SortItems( ::ListCompare, m_sort_type );
	}
	else if( column == COL_PINS )
	{
		if( m_sort_type == SORT_UP_PINS )
			m_sort_type = SORT_DOWN_PINS;
		else
			m_sort_type = SORT_UP_PINS;
		m_list_ctrl.SortItems( ::ListCompare, m_sort_type );
	}
	*pResult = 0;
}
