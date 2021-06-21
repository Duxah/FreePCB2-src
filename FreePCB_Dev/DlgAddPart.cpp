// DlgAddPart.cpp : implementation file
//
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *		Author:    Allan Wright (ver. 1.0 - 1.359)		   *
 *		URL: www.freepcb.com							   *
 *		Maintained:    Duxah (ver. 2.0 - 2.033)			   *
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
#include "DlgAddPart.h"
#include "resource.h"
#include "DlgDupFootprintName.h"
#include "PathDialog.h"


// save expanded state of local cache
BOOL gLocalCacheExpanded = FALSE;

// global for last ref des
CString last_ref_des = "";
CString last_footprint = "";
CString last_package = "";

// CDlgAddPart dialog

IMPLEMENT_DYNAMIC(CDlgAddPart, CDialog)
CDlgAddPart::CDlgAddPart(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgAddPart::IDD, pParent)
{
	CONVERT_PART_ANGLE = 1;
	m_footprint_cache_map = 0;
	m_units = MIL;
}

CDlgAddPart::~CDlgAddPart()
{
}

void CDlgAddPart::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PART_LIB_TREE, part_tree);
	DDX_Control(pDX, IDC_FOOTPRINT, m_edit_footprint);
	DDX_Control(pDX, IDC_FOOT_SEARCH, m_edit_search);
	DDX_Control(pDX, IDC_RADIO_DRAG, m_radio_drag);
	DDX_Control(pDX, IDC_RADIO_SET, m_radio_set);
	DDX_Control(pDX, IDC_RADIO_OFFBOARD, m_radio_offboard);
	DDX_Control(pDX, IDC_X, m_edit_x);
	DDX_Control(pDX, IDC_Y, m_edit_y);
	DDX_Control(pDX, IDC_LIST_SIDE, m_list_side);
	DDX_Control(pDX, IDC_PART_REF, m_edit_ref_des);
	DDX_Control(pDX, IDC_PART_PACKAGE, m_edit_package);
	DDX_Control(pDX, IDC_PREVIEW, m_preview);
	DDX_Control(pDX, IDC_COMBO_ADD_PART_UNITS, m_combo_units);
	DDX_Control(pDX, IDC_EDIT_ADD_AUTHOR, m_edit_author);
	DDX_Control(pDX, IDC_EDIT_ADD_DESC, m_edit_desc);
	DDX_Control(pDX, IDC_BUTTON_ADD_PART_BROWSE, m_button_browse);
	DDX_Control(pDX, IDC_EDIT_ADD_PART_LIB, m_edit_lib);
	DDX_Control(pDX, IDC_EDIT_VALUE, m_edit_value);
	DDX_Control(pDX, IDC_CHECK1, m_check_value_visible);
	DDX_Control(pDX, IDC_COMBO2, m_combo_angle);
	DDX_Control(pDX, IDC_STATIC2, m_warning);
	if( m_n_list_locked )
		m_warning.EnableWindow();
	else
		m_warning.EnableWindow(0);
	//DDX_CBString
	if( pDX->m_bSaveAndValidate )
	{
		// get package
		m_edit_package.GetWindowText( m_package );

		// outgoing
		// test for valid ref des
		CString ref_des_str;
		m_edit_ref_des.GetWindowText( ref_des_str );
		ref_des_str.Trim();
		if( !m_multiple )
		{
			if( ref_des_str.GetLength() == 0 )
			{
				CString mess;
				mess.Format( "Illegal reference designator" );
				AfxMessageBox( mess );
				pDX->PrepareEditCtrl( IDC_PART_REF );
				pDX->Fail();
			}
			if( ref_des_str.FindOneOf( ",. " ) != -1 )
			{
				CString mess;
				mess.Format( "Illegal reference designator \"%s\"", ref_des_str );
				AfxMessageBox( mess );
				pDX->PrepareEditCtrl( IDC_PART_REF );
				pDX->Fail();
			}
			CString CPY_ref_des_str = ref_des_str;
			int l1 = ref_des_str.GetLength();
			CString label = "?";
			if( m_ip >= 0 )
				if( (*m_pl)[m_ip].ref_des.Right(1) == "?" )
					if( l1 < (*m_pl)[m_ip].ref_des.GetLength() )
						if( ref_des_str.Compare( (*m_pl)[m_ip].ref_des.Left(l1) ) == 0 )
							label = "?*";
			int CPY_m_ip = m_ip;
			int FLAG = 0;
			int n_parts = 0;
			for( int i=0; i<m_pl->GetSize(); i++ )
			{
				if( (*m_pl)[i].deleted )
					continue;
				if( i == CPY_m_ip )
					continue;
				if( CPY_ref_des_str.Compare( (*m_pl)[i].ref_des ) == 0 )
				{
					CString mess;
					if( FLAG == 0 )
					{
						FLAG = 1;
						mess.Format( " Duplicate reference! %s already used. Determine free index value and rename REF automatically?", (*m_pl)[i].ref_des);//, (*m_pl)[i].ref_des, label );
						int ret = AfxMessageBox( mess, MB_YESNOCANCEL );
						if( ret == IDNO )
						{
							CPY_ref_des_str = (*m_pl)[i].ref_des + label;
							(*m_pl)[i].ref_des = (*m_pl)[i].ref_des + label;
							CPY_m_ip = i;
							i = -1;
							continue;
						}
						else if( ret == IDYES )
						{
							// use next available ref
							CString g_prefix, new_ref;
							ParseRef( &ref_des_str, &g_prefix );
							int x_num = 0;
							BOOL bREP = TRUE;
							for( int x_num=1; bREP; x_num++ )
							{
								bREP = FALSE;
								new_ref.Format( "%s%d", g_prefix, x_num );					
								for( int inum=0; inum<m_pl->GetSize(); inum++ )
									if( new_ref.Compare( (*m_pl)[inum].ref_des ) == 0 )
									{
										bREP = TRUE;
										break;
									}
							}
							ref_des_str = new_ref;
							break;
						}
						else
						{
							pDX->PrepareEditCtrl( IDC_PART_REF );
							pDX->Fail();
						}
					}
					else
					{		
						mess.Format( "Part %s already exists and will also be renamed to \"%s%s\"!", (*m_pl)[i].ref_des, (*m_pl)[i].ref_des, label );
						AfxMessageBox( mess, MB_OK );
						CPY_ref_des_str = (*m_pl)[i].ref_des + label;
						(*m_pl)[i].ref_des = (*m_pl)[i].ref_des + label;
						CPY_m_ip = i;
						i = -1;		
						continue;
					}
				}
			}
		}

		//test for valid value string
		CString value_str;
		m_edit_value.GetWindowText( value_str );
		if( value_str.Find( "@" ) != -1 )
		{
			CString mess;
			mess.Format( "Value cannot contain \"@\"" );
			AfxMessageBox( mess );
			pDX->PrepareEditCtrl( IDC_EDIT_VALUE );
			pDX->Fail();
		}
		CShape * new_shape = NULL;
		void * ptr = 0;
		int m_pl_sz = m_pl->GetSize();
		if( !m_multiple || (m_multiple_mask & MSK_FOOTPRINT) )
		{
			// check if footprint valid and load into cache if necessary
			CString foot_str,pack_str;
			m_edit_package.GetWindowText( pack_str );
			m_shape.m_package = pack_str;
			m_edit_footprint.GetWindowText( foot_str );
			BOOL bInCache = m_footprint_cache_map->Lookup( foot_str, ptr );
			int n_old_pins=0, origin_moved_X=0, origin_moved_Y=0;
			CShape * old_sh = NULL;
			if( m_ip >= 0 )
				old_sh = (*m_pl)[m_ip].shape;
			if (old_sh )
			{
				CShape * new_sh = &m_shape;
				if( m_shape.m_name.Compare("EMPTY_SHAPE") == 0 && bInCache )
					new_sh = (CShape*)ptr;
				n_old_pins = old_sh->m_padstack.GetSize();
				// Pin1 align
				int i1 = old_sh->GetPinIndexByName( "1", -1 );
				int i2 = new_sh->GetPinIndexByName( "1", -1 );
				if( i1 == -1 || i2 == -1 )
				{
					i1 = old_sh->GetPinIndexByName( "A1", -1 );
					i2 = new_sh->GetPinIndexByName( "A1", -1 );
					if( i1 == -1 || i2 == -1 )
					{
						i1 = old_sh->GetPinIndexByName( "A", -1 );
						i2 = new_sh->GetPinIndexByName( "A", -1 );
						if( i1 == -1 || i2 == -1 )
						{
							i1 = old_sh->GetPinIndexByName( "a", -1 );
							i2 = new_sh->GetPinIndexByName( "a", -1 );
						}
					}
				}
				if( i1 >= 0 && i2 >= 0 )
				{
					int xp1 = old_sh->m_padstack[i1].x_rel;
					int yp1 = old_sh->m_padstack[i1].y_rel;
					int xp2 = new_sh->m_padstack[i2].x_rel;
					int yp2 = new_sh->m_padstack[i2].y_rel;
					origin_moved_X = xp2 - xp1;
					origin_moved_Y = yp2 - yp1;
				}
			}


			// check number of pins, unless new part or no old shape
			if( !m_new_part && (*m_pl)[m_ip].shape )
			{
				int n_new_pins = m_shape.m_padstack.GetSize();
				if( m_shape.m_name.Compare( "EMPTY_SHAPE" ) == 0 )
					n_new_pins = INT_MAX;
				if( n_new_pins < n_old_pins )
				{
					CString mess;
					int ret;
					if( !m_n_list_locked )
					{
						mess.Format( "Warning: %s has fewer pins than %s\nDo you really want to replace it ? ",
							foot_str, (*m_pl)[m_ip].shape->m_name );
						ret = AfxMessageBox( mess, MB_YESNO );
					}
					else
					{
						mess.Format( "Warning: %s has fewer pins than %s,and since the netlist is protected, no continuation is possible. \n(Unlock netlist: Project -> Nets -> Netlist_Protected)",
							foot_str, (*m_pl)[m_ip].shape->m_name );
						ret = AfxMessageBox( mess, MB_OK );
					}
					if( ret != IDYES)
					{
						//m_edit_footprint.SetWindowText( (*m_pl)[m_ip].shape->m_name ); bag
						//m_edit_package.SetWindowText( (*m_pl)[m_ip].shape->m_package ); bag
						pDX->Fail();
					}
				}
			}


			if( bInCache )
			{
				// footprint with the same name is already in the local cache
				// however, it is possible that the footprint was selected from a library
				// file and might not match the footprint in the cache
				CShape * cache_shape = (CShape*)ptr;	// footprint from cache
				BOOL bFootprintUnchanged = cache_shape->Compare( &m_shape );
				if( m_in_cache || bFootprintUnchanged || m_shape.m_name.Compare("EMPTY_SHAPE") == 0 )
				{
					// the new footprint is already in the cache, because:
					//	it was selected from the cache, 
					//	or the new footprint is identical to the cached version,
					//	or a new footprint was never selected from the library tree
					// therefore, do nothing
				}
				else
				{
					// load new footprint into cache, 
					// replacing the previous one with the same name
					int num_other_instances = 0;
					for( int i=0; i<m_pl_sz; i++ )
					{
						part_info * pi = &(*m_pl)[i];
						if( pi->shape )
						{
							if( pi->shape->m_name == foot_str && ref_des_str != pi->ref_des )
							{
								num_other_instances++;
							}
						}
					}
					if( num_other_instances )
					{
						// display dialog to warn user about overwriting the old footprint
						CDlgDupFootprintName dlg;
						CString mess;
						mess.Format( "Warning: A footprint named \"%s\"\r\nis already in use by other parts.\r\n", foot_str );
						mess += "Loading this new footprint will overwrite the old one\r\nunless you change its name\r\n";
						dlg.Initialize( &mess, m_footprint_cache_map );
						int ret = dlg.DoModal();
						if( ret == IDOK )
						{
							// clicked "OK"
							if( dlg.m_replace_all_flag )
							{
								// replace all instances of footprint
								cache_shape->Copy( &m_shape );
								for( int i=0; i<m_pl_sz; i++ )
								{
									part_info * pi = &(*m_pl)[i];
									if( pi->shape == cache_shape )
									{
										pi->bShapeChanged = TRUE;
									}
								}
							}
							else
							{
								// assign new name to footprint and put in cache
								CShape * shape = new CShape;// ok?
								shape->Copy( &m_shape );
								shape->m_name = *dlg.GetNewName();	
								m_footprint_cache_map->SetAt( shape->m_name, shape );
								ptr = shape;
							}
						}
						else
						{
							// clicked "Cancel"
							pDX->Fail();
						}
					}
					else
					{
						// replace the footprint in the cache
						cache_shape->Copy( &m_shape );
						if( !m_new_part && m_ip != -1 )
							(*m_pl)[m_ip].bShapeChanged = TRUE;
					}
				}
			}
			else   
			{
				// shape not in cache
				if( m_shape.m_name.Compare("EMPTY_SHAPE") == 0 )
				{
					AfxMessageBox( "Error: No footprint selected" );
					pDX->Fail();
				}
				CShape * shape = new CShape;// ok?
				shape->Copy( &m_shape );
				shape->m_name = foot_str;	// in case it was renamed
				m_footprint_cache_map->SetAt( foot_str, shape );
				ptr = shape;
			}

			// OK, now we can assume that the footprint is in the cache
			// and ptr is a valid pointer to it
			if( ptr == NULL )
				ASSERT(0);
			new_shape = (CShape*)ptr;
			new_shape->origin_moved_X = origin_moved_X;
			new_shape->origin_moved_Y = origin_moved_Y;

			if( m_new_part )
			{
				// if new part, add entry to partlist_info
				m_ip = m_pl->GetSize();
				m_pl->SetSize( m_ip + 1 );
				(*m_pl)[m_ip].part = 0;
				// set globals for next time
				last_ref_des = ref_des_str;
				last_footprint = new_shape->m_name;
				last_package = new_shape->m_package;
			}

			// OK, now update partlist_info with new shape
			BOOL bFootprintChanged = TRUE;
			if( (*m_pl)[m_ip].shape == new_shape)
				bFootprintChanged = FALSE;
			(*m_pl)[m_ip].shape = new_shape;
			(*m_pl)[m_ip].ref_size = new_shape->m_ref_size;
			(*m_pl)[m_ip].ref_width = new_shape->m_ref_w;
		}

		if( !m_multiple || (m_multiple_mask & MSK_VALUE) )
		{
			// update value
			CString value_str;
			m_edit_value.GetWindowText( value_str );
			(*m_pl)[m_ip].value = value_str;
			if( !m_multiple )
				(*m_pl)[m_ip].value_vis = m_check_value_visible.GetCheck();
		}

		// update partlist_info
		if( !m_multiple ) 
		{
			// update all fields for part
			GetFields();
			(*m_pl)[m_ip].ref_des = ref_des_str;
			(*m_pl)[m_ip].x = m_x;
			(*m_pl)[m_ip].y = m_y;
			int side = m_list_side.GetCurSel();
			(*m_pl)[m_ip].side = side;
			int cent_angle = 0;
			if( (*m_pl)[m_ip].shape )
				cent_angle = (*m_pl)[m_ip].shape->m_centroid_angle;
			(*m_pl)[m_ip].angle = ::GetPartAngleForReportedAngle( m_combo_angle.GetCurSel()*CONVERT_PART_ANGLE, 
				cent_angle, side );
			(*m_pl)[m_ip].deleted = FALSE;
			if( m_radio_offboard.GetCheck() )
				(*m_pl)[m_ip].bOffBoard = TRUE;
			else
				(*m_pl)[m_ip].bOffBoard = FALSE;
		}
	}
	else
	{
		// incoming
		m_combo_units.InsertString( 0, "MIL" );
		m_combo_units.InsertString( 1, "MM" );
		m_edit_lib.SetWindowText( *m_folder->GetFullPath() );
	}
}



BEGIN_MESSAGE_MAP(CDlgAddPart, CDialog)
	ON_NOTIFY(TVN_SELCHANGED, IDC_PART_LIB_TREE, OnTvnSelchangedPartLibTree)
	ON_BN_CLICKED(IDC_RADIO_DRAG, OnBnClickedRadioDrag)
	ON_BN_CLICKED(IDC_RADIO_SET, OnBnClickedRadioSet)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_EN_CHANGE(IDC_FOOT_SEARCH, OnEditFootChange)
	ON_CBN_SELCHANGE(IDC_COMBO_ADD_PART_UNITS, OnCbnSelchangeComboAddPartUnits)
	ON_BN_CLICKED(IDC_BUTTON_ADD_PART_BROWSE, OnBnClickedButtonBrowse)
	ON_BN_CLICKED(IDC_RADIO_OFFBOARD, OnBnClickedRadioOffBoard)
	ON_STN_CLICKED(IDC_STATIC2, &CDlgAddPart::OnStnClickedStatic2)
END_MESSAGE_MAP()


//**************** public methods ********************

// initialize dialog
//	
void CDlgAddPart::Initialize(partlist_info * pl,
							 int i, 
							 BOOL standalone,
							 BOOL new_part,
							 BOOL multiple,
							 int multiple_mask,
							 int n_list_locked,
							 CMapStringToPtr * shape_cache_map,
							 CFootLibFolderMap * footlibfoldermap,
							 int units,
							 CDlgLog * log )
{
	m_units = units;  
	m_pl = pl;
	m_ip = i;
	m_standalone = standalone;
	m_new_part = new_part;
	m_multiple = multiple;
	m_multiple_mask = multiple_mask;
	m_n_list_locked = n_list_locked;
	m_footprint_cache_map = shape_cache_map;
	m_footlibfoldermap = footlibfoldermap;
	CString * last_folder_path = m_footlibfoldermap->GetLastFolder();
	m_folder = m_footlibfoldermap->GetFolder( last_folder_path, log );
	if(!m_folder)
	{
		last_folder_path = m_footlibfoldermap->GetDefaultFolder();
		m_folder = m_footlibfoldermap->GetFolder( last_folder_path, log );
	}
	m_in_cache = FALSE;
	m_dlg_log = log;
}

// get flag indicating that dragging was requested
//
BOOL CDlgAddPart::GetDragFlag()
{
	return m_drag_flag;
}

//***************** message handlers *******************

BOOL CDlgAddPart::OnInitDialog()
{
	CString str;
	LPCSTR p;

	CDialog::OnInitDialog();
	CTreeCtrl * pCtrl = &part_tree;
	int i_exp = 0;

	InitPartLibTree("");

	// list control for angle
	for(int a=0; a<360; a=a+CONVERT_PART_ANGLE )
	{
		CString s;
		MakeCStringFromDouble( &s, a );
		m_combo_angle.InsertString( a/CONVERT_PART_ANGLE, s );
	}
	m_list_side.InsertString( 0, "top" );
	m_list_side.InsertString( 1, "bottom" );

	// set up for new part or edit existing part
	if( m_new_part )
	{
		m_edit_ref_des.SetWindowText( "" );
		m_edit_footprint.SetWindowText( "" );
		m_edit_package.SetWindowText( "" );
		if( last_ref_des.GetLength() )
		{
			int num_pos = -1;
			for( int i=last_ref_des.GetLength()-1; i>=0; i-- )
			{
				if( last_ref_des[i] > '9' || last_ref_des[i] < '0' )
				{
					num_pos = i+1;
					break;
				}
			}
			if( num_pos > 0 && num_pos < last_ref_des.GetLength() )
			{
				CString num_part = last_ref_des.Right( last_ref_des.GetLength()-num_pos );
				int num = atoi( num_part );
				if( num > 0 )
				{
					num = 0;
					BOOL done = FALSE;
					while( !done )
					{
						CString ref_des;
						ref_des.Format( "%s%d", last_ref_des.Left(num_pos), num+1 );
						done = TRUE;
						for( int i=0; i<m_pl->GetSize(); i++ )
						{
							if( m_pl->GetAt(i).ref_des == ref_des )
							{
								num++;
								done = FALSE;
								break;
							}
						}
						if( done )
						{
							m_edit_ref_des.SetWindowText( ref_des );
							m_edit_footprint.SetWindowText( last_footprint );
							m_edit_package.SetWindowText( last_package );
						}
					}
				}
			}
		}
		m_units = MIL;
		m_combo_units.SetCurSel(0);
		m_x = 0;
		m_y = 0;
		m_edit_x.SetWindowText( "0" );
		m_edit_y.SetWindowText( "0" );
		m_combo_angle.SetCurSel( 0 );
		m_list_side.SetCurSel( 0 );
		if( m_standalone )
		{
			m_radio_drag.SetCheck( 1 );
			m_radio_offboard.SetCheck( 0 );
			m_radio_set.SetCheck( 0 );
			m_drag_flag = TRUE;
			m_combo_units.EnableWindow( FALSE );
			m_edit_x.EnableWindow( FALSE );
			m_edit_y.EnableWindow( FALSE );
			m_combo_angle.EnableWindow( FALSE );
			m_list_side.EnableWindow( FALSE );
		}
		else if( !m_standalone )
		{
			m_radio_drag.SetCheck( 0 );
			m_radio_offboard.SetCheck( 1 );
			m_radio_set.SetCheck( 0 );
			m_radio_drag.EnableWindow( 0 );
			m_drag_flag = FALSE;
			m_combo_units.EnableWindow( FALSE );
			m_edit_x.EnableWindow( FALSE );
			m_edit_y.EnableWindow( FALSE );
			m_combo_angle.EnableWindow( FALSE );
			m_list_side.EnableWindow( FALSE );
		}
	}
	else if( m_multiple )
	{
		part_info * pi = &(*m_pl)[m_ip];
		m_edit_ref_des.SetWindowText( "multiple" );
		m_edit_ref_des.EnableWindow( FALSE );
		//m_edit_search.EnableWindow( FALSE );
		m_edit_package.EnableWindow( FALSE );
		m_edit_footprint.EnableWindow( m_multiple_mask & MSK_FOOTPRINT );
		part_tree.EnableWindow( m_multiple_mask & MSK_FOOTPRINT );
		m_edit_lib.EnableWindow( m_multiple_mask & MSK_FOOTPRINT );
		m_button_browse.EnableWindow( m_multiple_mask & MSK_FOOTPRINT );
		m_edit_value.EnableWindow( m_multiple_mask & MSK_VALUE );
		m_check_value_visible.EnableWindow( FALSE );
		m_edit_package.SetWindowText( "" );
		m_edit_footprint.SetWindowText( "" );
		m_edit_value.SetWindowText( "" );
		m_check_value_visible.SetCheck(0);

		if( m_units == MIL )
			m_combo_units.SetCurSel(0);
		else
			m_combo_units.SetCurSel(1);
		m_x = 0;
		m_y = 0;
		SetFields();
		m_radio_set.EnableWindow( 0 );
		m_radio_drag.EnableWindow( 0 );
		m_radio_offboard.EnableWindow( 0 );
		m_combo_units.EnableWindow( FALSE );
		m_edit_x.EnableWindow( FALSE );
		m_edit_y.EnableWindow( FALSE );
		m_combo_angle.EnableWindow( FALSE );
		m_list_side.EnableWindow( FALSE );
	}
	else
	{
		part_info * pi = &(*m_pl)[m_ip]; 
		m_edit_ref_des.SetWindowText( pi->ref_des ); 
		if( m_n_list_locked )
			m_edit_ref_des.EnableWindow( FALSE );
		m_edit_value.SetWindowText( pi->value );
		m_check_value_visible.SetCheck( pi->value_vis );
		if( pi->shape )
		{
			m_edit_footprint.SetWindowText( pi->shape->m_name );
			m_edit_package.SetWindowText( pi->shape->m_package );
		}
		if( m_units == MIL )
			m_combo_units.SetCurSel(0);
		else
			m_combo_units.SetCurSel(1);
		m_x = (*m_pl)[m_ip].x;
		m_y = (*m_pl)[m_ip].y;
		SetFields();
		m_list_side.SetCurSel( pi->side );
		int cent_angle = 0;
		if( pi->shape )
			cent_angle = pi->shape->m_centroid_angle;
		int angle = GetReportedAngleForPart( (*m_pl)[m_ip].angle, 
			cent_angle, (*m_pl)[m_ip].side );
		if(angle/CONVERT_PART_ANGLE < m_combo_angle.GetCount())
			m_combo_angle.SetCurSel( angle/CONVERT_PART_ANGLE );
		m_radio_drag.SetCheck( 0 );
		m_radio_offboard.SetCheck( 0 );
		m_radio_set.SetCheck( 1 );
		m_radio_drag.EnableWindow( 0 );
		m_radio_offboard.EnableWindow( 0 );
		m_drag_flag = FALSE;
		m_combo_units.EnableWindow( TRUE );
		m_edit_x.EnableWindow( TRUE );
		m_edit_y.EnableWindow( TRUE );
		m_combo_angle.EnableWindow( TRUE );
		m_list_side.EnableWindow( TRUE );
	}
	return TRUE;  // return TRUE unless you set the focus to a control
}

// Initialize the tree control representing the footprint library and cache
//
void CDlgAddPart::InitPartLibTree( CString Filter )
{
	int len_f = Filter.GetLength();
	CString str;
	LPCSTR p;

	// initialize folder name
	m_edit_lib.SetWindowText( *m_folder->GetFullPath() );
	CTreeCtrl * pCtrl = &part_tree;
	pCtrl->DeleteAllItems();
	int i_exp = 0;
	
	// allow vertical scroll
	long style = ::GetWindowLong( part_tree, GWL_STYLE );
	style = style & ~TVS_NOSCROLL;
	::SetWindowLong( part_tree, GWL_STYLE, style | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS );

	// insert local cache name
	TVINSERTSTRUCT tvInsert;
	tvInsert.hParent = NULL;
	tvInsert.hInsertAfter = NULL;
	tvInsert.item.mask = TVIF_TEXT | TVIF_PARAM;
	tvInsert.item.pszText = _T("local cache");
	tvInsert.item.lParam = -1;
	HTREEITEM hLocal = pCtrl->InsertItem(&tvInsert);

	if( /*gLocalCacheExpanded ||*/ len_f )
		part_tree.SetItemState( hLocal, TVIS_EXPANDED, TVIS_EXPANDED );

	// insert cached footprints
	POSITION pos;
	CString key;
	void * ptr;
	int i = 0;
	for( pos = m_footprint_cache_map->GetStartPosition(); pos != NULL; )
	{
		m_footprint_cache_map->GetNextAssoc( pos, key, ptr );
		p = (LPCSTR)key;
		CString foot = p;
		int foot_len = foot.GetLength();
		if( foot_len >= len_f )
		{
			foot = foot.Left(len_f);
			if( len_f == 0 || foot.CompareNoCase( Filter ) == 0 )
			{
				tvInsert.hInsertAfter = 0;
				tvInsert.hParent = hLocal;
				tvInsert.item.pszText = (LPSTR)p;
				tvInsert.item.lParam = (LPARAM)i;
				pCtrl->InsertItem(&tvInsert);
				i++;
			}
		}	
	}

	// insert all library names
	HTREEITEM hLib;
	HTREEITEM hLib_last;
	HTREEITEM hHead;
	HTREEITEM hHead_last;
	// loop through libraries
	int n_added = 0;
	for( int ilib=0; ilib<m_folder->GetNumLibs(); ilib++ )
	{
		if( ilib && n_added == 0 )
			part_tree.DeleteItem(hLib_last);
		// put library filename into Tree
		str = *m_folder->GetLibraryFileName( ilib );
		p = (LPCSTR)str;
		tvInsert.hParent = NULL;
		tvInsert.item.pszText = (LPSTR)p;
		if( ilib == 0 )
			tvInsert.hInsertAfter = hLocal;
		else
			tvInsert.hInsertAfter = hLib_last;
		tvInsert.item.lParam = -1;
		hLib = pCtrl->InsertItem(&tvInsert);	// insert library name

		if( /*m_folder->GetExpanded( ilib ) ||*/ len_f )
			part_tree.SetItemState( hLib, TVIS_EXPANDED, TVIS_EXPANDED );

		hLib_last = hLib;
		n_added = 0;
		// loop through footprints in heading
		for( int i=0; i<m_folder->GetNumFootprints(ilib); i++ )
		{
			// put footprint into tree
			str = *m_folder->GetFootprintName( ilib, i );
			p = (LPCSTR)str;
			CString foot = p;
			int foot_len = foot.GetLength();
			if( foot_len >= len_f )
			{
				foot = foot.Left(len_f);
				if( len_f == 0 || foot.CompareNoCase( Filter ) == 0 )
				{
					tvInsert.hParent = hLib;
					tvInsert.item.pszText = (LPSTR)p;
					UINT32 lp = (ilib+1)*0x1000000 + i;
					tvInsert.item.lParam = (LPARAM)lp;
					tvInsert.hInsertAfter = 0;
					pCtrl->InsertItem(&tvInsert);
					n_added++;
				}
			}		
		}
	}
	if( n_added == 0 && m_folder->GetNumLibs() )
		part_tree.DeleteItem(hLib_last);
}

void CDlgAddPart::OnTvnSelchangedPartLibTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	HTREEITEM SEL_Item = part_tree.GetSelectedItem();
	CString SEL_Text = part_tree.GetItemText(SEL_Item);
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	UINT32 lp = pNMTreeView->itemNew.lParam;
	m_ilib = -1;
	m_ihead = -1;
	m_ifoot = -1;
	if( lp != -1 )
	{
		m_ilib = (lp>>24) & 0xff;
		m_ihead = (lp>>16) & 0xff;
		m_ifoot = lp & 0xffff;
		CString str = "", str_p = "";
		if( m_ilib == 0 )
		{
			m_in_cache = TRUE;
			POSITION pos;
			CString key;
			void * ptr=NULL;
			pos = m_footprint_cache_map->GetStartPosition();
			for( int i=0; ; i++ )
			{
				m_footprint_cache_map->GetNextAssoc( pos, key, ptr );
				if( !ptr )
					break;
				CShape * sh = (CShape*)ptr;
				if( sh )
				{
					str = key;
					str_p = sh->m_package;
					CString sh_m_name = sh->m_name;
					if( sh_m_name.Compare( SEL_Text ) == 0 )
					{
						m_ifoot = i;
						break;
					}
				}
			}
		}
		else
		{
			m_ilib--;
			m_in_cache = FALSE;
			str = *m_folder->GetFootprintName( m_ilib, m_ifoot );
			str_p = *m_folder->GetFootprintPackage( m_ilib, m_ifoot );
		}
		m_footprint_name = str;	
		m_edit_footprint.SetWindowText( str );
		m_edit_package.SetWindowText( str_p );

		// draw footprint preview in control
		void * ptr;
		// lookup shape in cache
		if( m_in_cache )
		{
			// found it, make shape
			BOOL found = m_footprint_cache_map->Lookup( m_footprint_name, ptr );
			if( !found )
			{
				AfxMessageBox( "Warning: Library was changed!" );
				CString * last_folder_path = m_footlibfoldermap->GetLastFolder();
				m_folder->IndexAllLibs( last_folder_path, m_dlg_log );
				m_folder = m_footlibfoldermap->GetFolder( last_folder_path, m_dlg_log );
			}
			m_shape.Copy( (CShape*)ptr );
		}
		else
		{
			// not in cache, get from library file
			CString * lib_file_name = m_folder->GetLibraryFullPath( m_ilib );
			int offset = m_folder->GetFootprintOffset( m_ilib, m_ifoot );
			// make shape from library file
			int err = m_shape.MakeFromFile( NULL, m_footprint_name, *lib_file_name, offset ); 
			if( err )
			{
				AfxMessageBox( "Warning: Library was changed!" );
				// unable to make shape
				CString * last_folder_path = m_footlibfoldermap->GetLastFolder();
				m_folder->IndexAllLibs( last_folder_path, m_dlg_log );
				m_folder = m_footlibfoldermap->GetFolder( last_folder_path, m_dlg_log );
			}
		}
		// now draw preview of footprint
		CMetaFileDC m_mfDC;
		CDC * pDC = this->GetDC();
		RECT rw;
		m_preview.GetClientRect( &rw );
		int x_size = rw.right - rw.left;
		int y_size = rw.bottom - rw.top;
		HENHMETAFILE hMF = m_shape.CreateMetafile( &m_mfDC, pDC, x_size, y_size );
		m_preview.SetEnhMetaFile( hMF );
		ReleaseDC( pDC );
		// update text strings
		m_edit_author.SetWindowText( m_shape.m_author );
		m_edit_desc.SetWindowText( m_shape.m_desc );
	}
	*pResult = 0;
}

void CDlgAddPart::OnBnClickedRadioDrag()
{
	m_combo_units.EnableWindow( FALSE );
	m_edit_x.EnableWindow( FALSE );
	m_edit_y.EnableWindow( FALSE );
	m_combo_angle.EnableWindow( FALSE );
	m_list_side.EnableWindow( FALSE );
	m_drag_flag = TRUE;
}

void CDlgAddPart::OnBnClickedRadioSet()
{
	m_combo_units.EnableWindow( TRUE );
	m_edit_x.EnableWindow( TRUE );
	m_edit_y.EnableWindow( TRUE );
	m_combo_angle.EnableWindow( TRUE );
	m_list_side.EnableWindow( TRUE );
	m_drag_flag = FALSE;
}

void CDlgAddPart::OnBnClickedRadioOffBoard()
{
	m_combo_units.EnableWindow( FALSE );
	m_edit_x.EnableWindow( FALSE );
	m_edit_y.EnableWindow( FALSE );
	m_combo_angle.EnableWindow( FALSE );
	m_list_side.EnableWindow( FALSE );
	m_drag_flag = FALSE;
}

void CDlgAddPart::OnEditFootChange()
{
	CString m_edit_s;
	m_edit_search.GetWindowText( m_edit_s );
	InitPartLibTree( m_edit_s );
	part_tree.RedrawWindow();
}


void CDlgAddPart::OnBnClickedOk()
{
	// get state of tree control so we can reproduce it next time
	// get next top-level item
	HTREEITEM item = part_tree.GetNextItem( NULL, TVGN_CHILD );
	// get all items
	int ilib = -1;
	while( item )
	{
		// top-level item
		BOOL expanded = part_tree.GetItemState( item, TVIS_EXPANDED );
		CString str;
		if( ilib == -1 )
			gLocalCacheExpanded = expanded & TVIS_EXPANDED;
		else
			m_folder->SetExpanded( ilib, expanded & TVIS_EXPANDED );
		// get next top-level item
		item = part_tree.GetNextItem( item, TVGN_NEXT );
		ilib++;
	}
	OnOK();
}

void CDlgAddPart::OnBnClickedCancel()
{
	// get state of tree control so we can reproduce it next time
	// get next top-level item
	HTREEITEM item = part_tree.GetNextItem( NULL, TVGN_CHILD );
	// get all items
	int ilib = -1;
	while( item )
	{
		// top-level item
		BOOL expanded = TVIS_EXPANDED & part_tree.GetItemState( item, TVIS_EXPANDED );
		CString str;
		if( ilib == -1 )
			gLocalCacheExpanded = expanded & TVIS_EXPANDED;
		else
			m_folder->SetExpanded( ilib, expanded & TVIS_EXPANDED );
		// get next top-level item
		item = part_tree.GetNextItem( item, TVGN_NEXT );
		ilib++;
	}
	OnCancel();
}

void CDlgAddPart::OnCbnSelchangeComboAddPartUnits()
{
	GetFields();
	if( m_combo_units.GetCurSel() == 0 )
		m_units = MIL;
	else
		m_units = MM;
	SetFields();
}

void CDlgAddPart::GetFields()
{
	CString str;
	if( m_units == MIL )
	{
		m_edit_x.GetWindowText( str );
		m_x = atof( str ) * NM_PER_MIL;
		m_edit_y.GetWindowText( str );
		m_y = atof( str ) * NM_PER_MIL;
	}
	else
	{
		m_edit_x.GetWindowText( str );
		m_x = atof( str ) * 1000000.0;
		m_edit_y.GetWindowText( str );
		m_y = atof( str ) * 1000000.0;
	}
}

void CDlgAddPart::SetFields()
{
	CString str;
	if( m_units == MIL )
	{
		MakeCStringFromDouble( &str, (double)m_x/NM_PER_MIL );
		m_edit_x.SetWindowText( str );
		MakeCStringFromDouble( &str, (double)m_y/NM_PER_MIL );
		m_edit_y.SetWindowText( str );
	}
	else
	{
		MakeCStringFromDouble( &str, (double)m_x/1000000.0 );
		m_edit_x.SetWindowText( str );
		MakeCStringFromDouble( &str, (double)m_y/1000000.0 );
		m_edit_y.SetWindowText( str );
	}
}

void CDlgAddPart::OnBnClickedButtonBrowse()
{
	CPathDialog dlg( "Open Folder", "Select footprint library folder", *m_folder->GetFullPath() );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		CString path_str = dlg.GetPathName();
		m_edit_lib.SetWindowText( path_str );
		if( !m_footlibfoldermap->FolderIndexed( &path_str ) )
		{
			// if library folder not indexed, pop up log
			m_dlg_log->ShowWindow( SW_SHOW );   
			m_dlg_log->UpdateWindow();
			m_dlg_log->BringWindowToTop();
			m_dlg_log->Clear();
			m_dlg_log->UpdateWindow(); 
		}
		m_folder = m_footlibfoldermap->GetFolder( &path_str, m_dlg_log );
		if( !m_folder )
		{
			ASSERT(0);
		}
		InitPartLibTree("");
		m_footlibfoldermap->SetLastFolder( &path_str );
		m_footlibfoldermap->SetDefaultFolder( &path_str );
		
	}
}



void CDlgAddPart::OnStnClickedStatic2()
{
	// TODO: добавьте свой код обработчика уведомлений
}
