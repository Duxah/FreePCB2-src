// DlgProjectOptions.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgProjectOptions.h"
#include "DlgAddWidth.h"
#include "PathDialog.h"
#include ".\dlgprojectoptions.h"

// global callback function for sorting
//		
int CALLBACK WidthCompare( LPARAM lp1, LPARAM lp2, LPARAM type )
{
	if( lp1 == lp2 )
		return 0;
	else if( lp1 > lp2 )
		return 1;
	else
		return -1;
}

// CDlgProjectOptions dialog

IMPLEMENT_DYNAMIC(CDlgProjectOptions, CDialog)
CDlgProjectOptions::CDlgProjectOptions(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgProjectOptions::IDD, pParent)
	, m_trace_w(0)
	, m_via_w(0)
	, m_hole_w(0)
	, m_layers(0)
{
	m_folder_changed = FALSE;
	m_folder_has_focus = FALSE;
}

CDlgProjectOptions::~CDlgProjectOptions()
{
}

void CDlgProjectOptions::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	if( !pDX->m_bSaveAndValidate )
	{
		// convert NM to MILS
		m_glue_w = m_glue_w/NM_PER_MIL;
		// convert seconds to minutes
		m_auto_interval = m_auto_interval/60;
	}
	DDX_Control(pDX, IDC_EDIT_NAME, m_edit_name);
	DDX_Text(pDX, IDC_EDIT_NAME, m_name );
	DDX_Control(pDX, IDC_EDIT_FOLDER, m_edit_folder);
	DDX_Text(pDX, IDC_EDIT_FOLDER, m_path_to_folder );
	DDX_Control(pDX, IDC_EDIT_LIBRARY_FOLDER, m_edit_lib_folder);
	DDX_Text(pDX, IDC_EDIT_LIBRARY_FOLDER, m_lib_folder );
	DDX_Control(pDX, IDC_EDIT_NUM_LAYERS, m_edit_layers );
	DDX_Text(pDX, IDC_EDIT_NUM_LAYERS, m_layers );
	DDV_MinMaxInt(pDX, m_layers, 1, 16 );
	DDX_Text(pDX, IDC_EDIT_GLUE_W, m_glue_w ); 
	DDV_MinMaxInt(pDX, m_glue_w, 1, 1000 );
	DDX_Control(pDX, IDC_LIST_WIDTH_MENU, m_list_menu);
	DDX_Control(pDX, IDC_CHECK_AUTOSAVE, m_check_autosave);
	DDX_Control(pDX, IDC_EDIT_AUTO_INTERVAL, m_edit_auto_interval);
	DDX_Text(pDX, IDC_EDIT_AUTO_INTERVAL, m_auto_interval );
	DDX_Control(pDX, IDC_CHECK1, m_check_SMT_connect);
	DDX_Control(pDX, IDC_CHECK_AUTORAT_DISABLE, m_check_disable_auto_rats);
	DDX_Control(pDX, IDC_EDIT_MIN_PINS, m_edit_min_pins);
	DDX_Text(pDX, IDC_EDIT_MIN_PINS, m_auto_ratline_min_pins );
	DDV_MinMaxInt(pDX, m_auto_ratline_min_pins, 0, 10000 );

	if( pDX->m_bSaveAndValidate )
	{
		// leaving dialog
		if( m_name.GetLength() == 0 )
		{
			pDX->PrepareEditCtrl( IDC_EDIT_NAME );
			AfxMessageBox( "Please enter name for project" );
			pDX->Fail();
		}
		else if( m_path_to_folder.GetLength() == 0 )
		{
			pDX->PrepareEditCtrl( IDC_EDIT_FOLDER );
			AfxMessageBox( "Please enter project folder" );
			pDX->Fail();
		}
		else if( m_lib_folder.GetLength() == 0 )
		{
			pDX->PrepareEditCtrl( IDC_EDIT_LIBRARY_FOLDER );
			AfxMessageBox( "Please enter library folder" );
			pDX->Fail();
		}
		else
		{
			// save options
			m_bSMT_connect_copper = m_check_SMT_connect.GetCheck();
			m_bAuto_Ratline_Disable = m_check_disable_auto_rats.GetCheck();

			// convert minutes to seconds
			m_auto_interval *= 60;

			// convert NM to MILS
			m_glue_w = m_glue_w*NM_PER_MIL;

			// update trace width menu
			int n = m_list_menu.GetItemCount();
			m_w->SetSize( n );
			m_v_w->SetSize( n );
			m_v_h_w->SetSize( n );
			char str[10];
			CString s;
			for( int i=0; i<n; i++ )
			{
				m_list_menu.GetItemText( i, 0, str, sizeof(str) );
				s = str;
				if( s.Right(2) == "MM" || s.Right(2) == "mm" )
					m_w->SetAt(i, -my_atof(&s));
				else
					m_w->SetAt(i, my_atof(&s));
				m_list_menu.GetItemText( i, 1, str, sizeof(str) );
				s = str;
				if( s.Left(3) == "def" )
					s = "0mm";
				if( s.Right(2) == "MM" || s.Right(2) == "mm" )
					m_v_w->SetAt(i, -my_atof(&s));
				else
					m_v_w->SetAt(i, my_atof(&s));
				m_list_menu.GetItemText( i, 2, str, sizeof(str) );
				s = str;
				if( s.Left(3) == "def" )
					s = "0mm";
				if( s.Right(2) == "MM" || s.Right(2) == "mm" )
					m_v_h_w->SetAt(i, -my_atof(&s));
				else
					m_v_h_w->SetAt(i, my_atof(&s));
			}
			s = "";
			DDX_Text(pDX,IDC_EDIT_DEF_TRACE_W,s);
			if( s.Right(2) == "MM" || s.Right(2) == "mm" )
				m_trace_w = -my_atof(&s);
			else
				m_trace_w = my_atof(&s);
			s = "";
			DDX_Text(pDX,IDC_EDIT_DEF_VIA_W,s);
			if( s.Right(2) == "MM" || s.Right(2) == "mm" )
				m_via_w = -my_atof(&s);
			else
				m_via_w = my_atof(&s);
			s = "";
			DDX_Text(pDX,IDC_EDIT_DEF_VIA_HOLE,s);
			if( s.Right(2) == "MM" || s.Right(2) == "mm" )
				m_hole_w = -my_atof(&s);
			else
				m_hole_w = my_atof(&s);
		}
	}
	else
	{
		CString s;
		::MakeCStringFromDimension( &s, abs(m_trace_w), (m_trace_w>=0?MIL:MM), TRUE, TRUE, FALSE, (m_trace_w>=0?0:3) );
		DDX_Text(pDX,IDC_EDIT_DEF_TRACE_W,s);
		::MakeCStringFromDimension( &s, abs(m_via_w), (m_via_w>=0?MIL:MM), TRUE, TRUE, FALSE, (m_via_w>=0?0:3) );
		DDX_Text(pDX, IDC_EDIT_DEF_VIA_W, s );
		::MakeCStringFromDimension( &s, abs(m_hole_w), (m_hole_w>=0?MIL:MM), TRUE, TRUE, FALSE, (m_hole_w>=0?0:3) );
		DDX_Text(pDX, IDC_EDIT_DEF_VIA_HOLE, s );
	}
}


BEGIN_MESSAGE_MAP(CDlgProjectOptions, CDialog)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnBnClickedButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_EDIT, OnBnClickedButtonEdit)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, OnBnClickedButtonDelete)
	ON_EN_CHANGE(IDC_EDIT_NAME, OnEnChangeEditName)
	ON_EN_CHANGE(IDC_EDIT_FOLDER, OnEnChangeEditFolder)
	ON_EN_SETFOCUS(IDC_EDIT_FOLDER, OnEnSetfocusEditFolder)
	ON_EN_KILLFOCUS(IDC_EDIT_FOLDER, OnEnKillfocusEditFolder)
	ON_BN_CLICKED(IDC_CHECK_AUTOSAVE, OnBnClickedCheckAutosave)
	ON_BN_CLICKED(IDC_BUTTON_LIB, OnBnClickedButtonLib)
	ON_BN_CLICKED(IDC_BUTTON_PRF, OnBnClickedButtonProjectFolder)
	ON_BN_CLICKED(IDC_CHECK_AUTORAT_DISABLE, OnBnClickedCheckAutoRatDisable)
END_MESSAGE_MAP()

// initialize data
//
void CDlgProjectOptions::Init( BOOL new_project,
							  CString * name,
							  CString * path_to_folder,
							  CString * lib_folder,
							  CString * app_dir,
							  int num_layers,
							  BOOL bSMT_connect_copper,
							  int glue_w,
							  int trace_w,
							  int via_w,
							  int hole_w,
							  int auto_interval,
							  BOOL bAuto_Ratline_Disable,
							  int auto_ratline_min_pins,
							  CArray<int> * w,
							  CArray<int> * v_w,
							  CArray<int> * v_h_w )
{
	m_new_project = new_project;
	if(!m_new_project)
		m_name = *name;
	else
		m_name = "";
	//
	int isep = app_dir->ReverseFind( '\\' );
	if( isep == -1 )
		isep = app_dir->ReverseFind( ':' );
	if( isep == -1 )
		ASSERT(0);		// unable to parse filename
	CString app = app_dir->Left(isep+1);
	struct _stat buf;
	m_path_to_folder = *path_to_folder;
	int err = _stat( m_path_to_folder, &buf );
	if( err )
	{
		m_path_to_folder = app;
		//*path_to_folder = m_path_to_folder;
	}
	//
	m_lib_folder = *lib_folder;
	err = _stat( m_lib_folder, &buf );
	if( err )
	{
		m_lib_folder = app + "fp_lib\\lib";
		//*lib_folder = m_lib_folder;
	}
	//
	m_layers = num_layers;
	m_bSMT_connect_copper = bSMT_connect_copper;
	m_glue_w = glue_w;
	m_trace_w = trace_w;
	m_via_w = via_w;
	m_hole_w = hole_w;
	m_auto_interval = auto_interval;
	m_bAuto_Ratline_Disable = bAuto_Ratline_Disable;
	m_auto_ratline_min_pins = auto_ratline_min_pins;
	m_w = w;
	m_v_w = v_w;
	m_v_h_w = v_h_w;
}

BOOL CDlgProjectOptions::OnInitDialog()
{
	CDialog::OnInitDialog();

	// initialize strings
	m_edit_folder.SetWindowText( m_path_to_folder );
	// set up list control
	DWORD old_style = m_list_menu.GetExtendedStyle();
	m_list_menu.SetExtendedStyle( LVS_EX_FULLROWSELECT | LVS_EX_FLATSB | old_style );
	m_list_menu.InsertColumn( 0, "Trace width", LVCFMT_LEFT, 77 );
	m_list_menu.InsertColumn( 1, "Via pad width", LVCFMT_LEFT, 77 );
	m_list_menu.InsertColumn( 2, "Via hole width", LVCFMT_LEFT, 78 );
	CString str;
	int n = m_w->GetSize();
	for( int i=0; i<n; i++ )
	{
		int nItem = m_list_menu.InsertItem( i, "" );
		m_list_menu.SetItemData( i, (LPARAM)m_w->GetAt(i) );
		float w = m_w->GetAt(i);
		if( w >= 0 )
			::MakeCStringFromDimension( &str, w, MIL, TRUE, TRUE, FALSE, 0 );
		else
			::MakeCStringFromDimension( &str, -w, MM, TRUE, TRUE, FALSE, 2 );
		m_list_menu.SetItem( i, 0, LVIF_TEXT, str, 0, 0, 0, 0 );
		w = m_v_w->GetAt(i);
		if( w > 0 )
			::MakeCStringFromDimension( &str, w, MIL, TRUE, TRUE, FALSE, 0 );
		else if( w < 0 )
			::MakeCStringFromDimension( &str, -w, MM, TRUE, TRUE, FALSE, 2 );
		else
			str = "default";
		m_list_menu.SetItem( i, 1, LVIF_TEXT, str, 0, 0, 0, 0 );
		w = m_v_h_w->GetAt(i);
		if( w > 0 )
			::MakeCStringFromDimension( &str, w, MIL, TRUE, TRUE, FALSE, 0 );
		else if( w < 0 )
			::MakeCStringFromDimension( &str, -w, MM, TRUE, TRUE, FALSE, 2 );
		else
			str = "default";
		m_list_menu.SetItem( i, 2, LVIF_TEXT, str, 0, 0, 0, 0 );
	}
	//if( !m_new_project )
	//{
		// disable some fields for existing project
		m_edit_folder.EnableWindow( FALSE );
		m_edit_lib_folder.EnableWindow( FALSE );
	//}
	m_check_autosave.SetCheck(1);
	m_check_autosave.EnableWindow(0);
	m_edit_auto_interval.SetWindowTextA("1");
	m_edit_auto_interval.EnableWindow(0);
	m_check_disable_auto_rats.SetCheck( m_bAuto_Ratline_Disable );
	m_edit_min_pins.EnableWindow( m_bAuto_Ratline_Disable );
	m_check_SMT_connect.SetCheck( m_bSMT_connect_copper );
	return TRUE;
}

// CDlgProjectOptions message handlers

void CDlgProjectOptions::OnBnClickedButtonAdd()
{
	CDlgAddWidth dlg;
	dlg.m_width[0] = '\0';
	dlg.m_via_w[0] = '\0';
	dlg.m_via_hole_w[0] = '\0';
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		if( dlg.m_width[0] == '\0' )
		{
			AfxMessageBox( "Illegal trace width." );
			return;
		}
		CString str;
		str = dlg.m_width;
		if( my_atof(&str) > _2540 )
		{
			str = dlg.m_via_w;
			float dlg_m_via_w = my_atof(&str);
			if( dlg_m_via_w >= 0 )
			{
				str = dlg.m_via_hole_w;
				float dlg_m_via_hole_w = my_atof(&str);
				if( dlg_m_via_hole_w >= 0 )
				{
					int ic = m_list_menu.GetItemCount();
					m_list_menu.InsertItem( ic, "" );
					m_list_menu.SetItem( ic, 0, LVIF_TEXT, dlg.m_width, 0, 0, 0, 0 );
					if( dlg_m_via_w > BY_ZERO )
						m_list_menu.SetItem( ic, 1, LVIF_TEXT, dlg.m_via_w, 0, 0, 0, 0 );
					else
						m_list_menu.SetItem( ic, 1, LVIF_TEXT, "default", 0, 0, 0, 0 );
					if( dlg_m_via_hole_w > BY_ZERO )
						m_list_menu.SetItem( ic, 2, LVIF_TEXT, dlg.m_via_hole_w, 0, 0, 0, 0 );
					else
						m_list_menu.SetItem( ic, 2, LVIF_TEXT, "default", 0, 0, 0, 0 );
					m_list_menu.SortItems( ::WidthCompare, 0 );
					return;
				}
			}
		}
	}
	else
		return;
	AfxMessageBox( "Illegal values (After the numbers you need to write units of measurement. For example: 1.0mm or 40mil)" );
}

void CDlgProjectOptions::OnBnClickedButtonEdit()
{
	POSITION pos = m_list_menu.GetFirstSelectedItemPosition();
	int i_sel = m_list_menu.GetNextSelectedItem( pos );
	if( i_sel < 0 )
	{
		AfxMessageBox( "no menu item selected" );
	}
	else
	{
		CDlgAddWidth dlg;
		char str[10];
		m_list_menu.GetItemText( i_sel, 0, str, sizeof(str) );
		for( int i=0; i<min(sizeof(dlg.m_width)-1,sizeof(str)); i++ )
		{
			dlg.m_width[i] = str[i];
			dlg.m_width[i+1] = '\0';
		}
		m_list_menu.GetItemText( i_sel, 1, str, sizeof(str) );
		if( str[0] == 'd' )
		{
			dlg.m_via_w[0] = '0';
			dlg.m_via_w[1] = '\0';
		}
		else for( int i=0; i<min(sizeof(dlg.m_via_w)-1,sizeof(str)); i++ )
		{
			dlg.m_via_w[i] = str[i];
			dlg.m_via_w[i+1] = '\0';
		}
		m_list_menu.GetItemText( i_sel, 2, str, sizeof(str) );
		if( str[0] == 'd' )
		{
			dlg.m_via_hole_w[0] = '0';
			dlg.m_via_hole_w[1] = '\0';
		}
		else for( int i=0; i<min(sizeof(dlg.m_via_hole_w)-1,sizeof(str)); i++ )
		{
			dlg.m_via_hole_w[i] = str[i];
			dlg.m_via_hole_w[i+1] = '\0';
		}
		int ret = dlg.DoModal();
		if( ret == IDOK )
		{
			m_list_menu.DeleteItem( i_sel );
			m_list_menu.InsertItem( i_sel, "" );
			m_list_menu.SetItem( i_sel, 0, LVIF_TEXT, dlg.m_width, 0, 0, 0, 0 );
			CString str = dlg.m_via_w;
			float via_w = my_atof(&str);
			if( via_w < BY_ZERO )
				m_list_menu.SetItem( i_sel, 1, LVIF_TEXT, "default", 0, 0, 0, 0 );
			else
				m_list_menu.SetItem( i_sel, 1, LVIF_TEXT, dlg.m_via_w, 0, 0, 0, 0 );
			str = dlg.m_via_hole_w;
			float hole_w = my_atof(&str);
			if( hole_w < BY_ZERO )
				m_list_menu.SetItem( i_sel, 2, LVIF_TEXT, "default", 0, 0, 0, 0 );
			else
				m_list_menu.SetItem( i_sel, 2, LVIF_TEXT, dlg.m_via_hole_w, 0, 0, 0, 0 );
			m_list_menu.SortItems( ::WidthCompare, 0 );
		}
	}
}

void CDlgProjectOptions::OnBnClickedButtonDelete()
{
	POSITION pos = m_list_menu.GetFirstSelectedItemPosition();
	int i_sel = m_list_menu.GetNextSelectedItem( pos );
	if( i_sel < 0 )
		AfxMessageBox( "no menu item selected" );
	else
		m_list_menu.DeleteItem( i_sel );
}

void CDlgProjectOptions::OnEnChangeEditName()
{
	CString str;
	m_edit_name.GetWindowText( str ); 
	if( m_new_project == TRUE && m_folder_changed == FALSE )
		m_edit_folder.SetWindowText( m_path_to_folder + str );
}

void CDlgProjectOptions::OnEnChangeEditFolder()
{
	if( m_folder_has_focus )
		m_folder_changed = TRUE;
}

void CDlgProjectOptions::OnEnSetfocusEditFolder()
{
	m_folder_has_focus = TRUE;
}

void CDlgProjectOptions::OnEnKillfocusEditFolder()
{
	m_folder_has_focus = FALSE;
}

void CDlgProjectOptions::OnBnClickedCheckAutosave()
{
	if( m_check_autosave.GetCheck() )
		m_edit_auto_interval.EnableWindow( TRUE );
	else
	{
		m_edit_auto_interval.EnableWindow( FALSE );
		m_edit_auto_interval.SetWindowText( "0" );
	}
}

void CDlgProjectOptions::OnBnClickedButtonLib()
{
	CPathDialog dlg( "Library Folder", "Select default library folder", m_lib_folder );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		m_lib_folder = dlg.GetPathName().MakeLower();
		m_edit_lib_folder.SetWindowText( m_lib_folder );
	}
}

void CDlgProjectOptions::OnBnClickedButtonProjectFolder()
{
	if( m_new_project )
	{
		CPathDialog dlg( "Project Folder", "Select project folder", m_path_to_folder );
		int ret = dlg.DoModal();
		if( ret == IDOK )
		{
			m_path_to_folder = dlg.GetPathName()+"\\";
			m_edit_folder.SetWindowText( m_path_to_folder );
		}
	}
}

void CDlgProjectOptions::OnBnClickedCheckAutoRatDisable()
{
	m_edit_min_pins.EnableWindow( m_check_disable_auto_rats.GetCheck() );
}
