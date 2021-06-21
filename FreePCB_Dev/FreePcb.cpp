// FreePcb.cpp : Defines the class behaviors for the application. 
//

#include "stdafx.h"
#include "freepcb.h"
#include "resource.h"
#include "DlgShortcuts.h"
#include "afxwin.h"

#include ".\freepcb.h"
//#include "QAFDebug.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;  
#endif

/////////////////////////////////////////////////////////////////////////////
// CFreePcbApp

BEGIN_MESSAGE_MAP(CFreePcbApp, CWinApp)
	//{{AFX_MSG_MAP(CFreePcbApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
	ON_COMMAND(ID_VIEW_FOOTPRINT, OnViewFootprint)
	ON_COMMAND(ID_VIEW_PCB_EDITOR, OnViewPcbEditor)
	ON_COMMAND(ID_FILE_OPENFOOTPRINTEDITOR, OnViewFootprint)
	ON_COMMAND(ID_HELP_GOTOWEBSITE, OnHelpGotoWebsite)
	ON_COMMAND(ID_FILE_MRU_FILE1, OnFileMruFile1)
	ON_COMMAND(ID_FILE_MRU_FILE2, OnFileMruFile2)
	ON_COMMAND(ID_FILE_MRU_FILE3, OnFileMruFile3)
	ON_COMMAND(ID_FILE_MRU_FILE4, OnFileMruFile4)
	ON_COMMAND(ID_FILE_MRU_FILE5, OnFileMruFile5)
	ON_COMMAND(ID_FILE_MRU_FILE6, OnFileMruFile6)
	ON_COMMAND(ID_FILE_MRU_FILE7, OnFileMruFile7)
	ON_COMMAND(ID_HELP_KEYBOARDSHORTCUTS, OnHelpKeyboardshortcuts)
	ON_COMMAND(ID_TOOLS_OPENONLINEAUTOROUTER, OnToolsOpenOnlineAutorouter)
	ON_COMMAND(ID_HELP_FREEROUTINGWEBSITE, OnHelpFreeRoutingWebsite)
	ON_COMMAND(ID_HELP_USERGUIDE_PDF, OnHelpUserGuidePdf)
	ON_COMMAND(ID_HELP_FAQ, OnHelpFAQ)
	ON_COMMAND(ID_HELP_FPCROUTE, OnHelpFpcRoute)
	ON_COMMAND(ID_HELP_USERGUIDESUPPLEMENT_PDF, OnHelpUserGuideSupplementPdf)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFreePcbApp construction

CFreePcbApp::CFreePcbApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CFreePcbApp object

CFreePcbApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CFreePcbApp initialization

BOOL CFreePcbApp::InitInstance()
{
	AfxEnableControlContainer();

	// Standard initialization

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	// Change the registry key under which our settings are stored.
	SetRegistryKey(_T("eebit2"));

	CWinApp::LoadStdProfileSettings();  // Load standard INI file options (including MRU)
	if( CWinApp::m_pRecentFileList == NULL)
	{
		AfxMessageBox( "NOTE: The recent file list is disabled on your system.\nUse the system policy editor to re-enable." );
	}
	//else
	//	CWinApp::m_pRecentFileList->m_nSize = _AFX_MRU_COUNT;
	EnableShellOpen();

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.
	m_pDocTemplate = new CSingleDocTemplate(//the app
		IDR_MAINFRAME,
		RUNTIME_CLASS(CFreePcbDoc),
		RUNTIME_CLASS(CMainFrame),    
		RUNTIME_CLASS(CFreePcbView));
	AddDocTemplate(m_pDocTemplate);

	// load menus
	VERIFY( m_main.LoadMenu( IDR_MAINFRAME ) );
	VERIFY( m_main_drag.LoadMenu( IDR_MAINFRAME_DRAG ) );
	VERIFY( m_foot.LoadMenu( IDR_FOOTPRINT ) );
	VERIFY( m_foot_drag.LoadMenu( IDR_FOOTPRINT_DRAG ) );

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// The window has been initialized, so show and update it.
	UINT iShowCmd = GetProfileInt(_T("Settings"),_T("ShowCmd"),SW_SHOW);
	m_pMainWnd->ShowWindow(iShowCmd);
	m_pMainWnd->UpdateWindow();

	// set pointers to document and view
	CMainFrame * pMainWnd = (CMainFrame*)AfxGetMainWnd();
	m_Doc = (CFreePcbDoc*)pMainWnd->GetActiveDocument();
	m_View = (CFreePcbView*)pMainWnd->GetActiveView();
	m_View->InitInstance();
	//
	// set initial view mode
	m_view_mode = PCB;
	//
	m_Doc->InitializeNewProject();
	//
	if( cmdInfo.m_nShellCommand == CCommandLineInfo::FileOpen )
	{
		CString fn = cmdInfo.m_strFileName;
		m_Doc->OnFileAutoOpen( fn );
	}

	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	CEdit m_edit_build;
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_EDIT1, m_edit_build);
	if( !pDX->m_bSaveAndValidate )
	{// VERSION
		// incoming
#ifdef _DEBUG
		m_edit_build.SetWindowText( "2 Debug: 032k"/* "$WCREV$ Debug: ($WCDATE$)" */ );
#else
		m_edit_build.SetWindowText( "2 Release: 032k"/* "$WCREV$ Debug: ($WCDATE$)" */ );
#endif
	}
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CFreePcbApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CFreePcbApp message handlers


void CFreePcbApp::OnViewFootprint()
{
	SwitchToView( RUNTIME_CLASS( CFootprintView ) );
	m_view_mode = FOOTPRINT;
	return;
}

void CFreePcbApp::OnViewPcbEditor()
{	
	SwitchToView( RUNTIME_CLASS( CFreePcbView ) );
	m_view_mode = PCB;
	return;
}

// switch to a new CView
//
BOOL CFreePcbApp::SwitchToView( CRuntimeClass * pNewViewClass )
{
	// I have no idea what most of this code actually does, but it seems to work
	CFrameWnd * pMainWnd = (CFrameWnd*)AfxGetMainWnd();
	CView * pOldActiveView = pMainWnd->GetActiveView();
	if( pOldActiveView->IsKindOf( pNewViewClass ) )
		return TRUE;
	CView * pNewView;
	if( pNewViewClass != RUNTIME_CLASS( CFreePcbView ) )
	{
		// switch to footprint view
		CCreateContext context;
		context.m_pNewViewClass = pNewViewClass;
		context.m_pCurrentDoc = m_Doc;
		pNewView = STATIC_DOWNCAST(CView, pMainWnd->CreateView(&context));
		m_View_fp = (CFootprintView*)pNewView;
	}
	else
	{
		// switch to pcb view
		pNewView = m_View;
	}
	if( pNewView )
	{
#if 0
		CMenu m_NewMenu;
		if( pNewViewClass == RUNTIME_CLASS( CFreePcbView ) )
			m_NewMenu.LoadMenu(IDR_MAINFRAME);
		else
			m_NewMenu.LoadMenu(IDR_FOOTPRINT);
		ASSERT(m_NewMenu);
		// Add the new menu
		pMainWnd->SetMenu(&m_NewMenu);
		m_NewMenu.Detach();
#endif
		if( pNewViewClass == RUNTIME_CLASS( CFreePcbView ) )
			pMainWnd->SetMenu(&m_main);
		else
			pMainWnd->SetMenu(&m_foot);

		// Exchange view window IDs so RecalcLayout() works.
		UINT temp = ::GetWindowLong(pOldActiveView->m_hWnd, GWL_ID);
		::SetWindowLong(pOldActiveView->m_hWnd, GWL_ID, ::GetWindowLong(pNewView->m_hWnd, GWL_ID));
		::SetWindowLong(pNewView->m_hWnd, GWL_ID, temp);
		// SetActiveView
		pMainWnd->SetActiveView( pNewView );
		pOldActiveView->ShowWindow( SW_HIDE );
		pNewView->ShowWindow( SW_SHOW );	
		pNewView->OnInitialUpdate();
		pMainWnd->RecalcLayout();
		if( pNewViewClass == RUNTIME_CLASS( CFreePcbView ) )
		{
			// switch to pcb view
			// first, see if we were editing the footprint of the selected part
			CShape * temp_footprint;
			if(	m_View->m_cursor_mode == CUR_PART_SELECTED 
				&& m_Doc->m_edit_footprint
				&& (m_Doc->m_footprint_modified || m_Doc->m_footprint_name_changed) )
			{
				// yes, make a copy of the footprint from the editor
				temp_footprint = new CShape;//ok
				temp_footprint->Copy( &m_View_fp->m_fp );
			}
			// destroy old footprint view
			pOldActiveView->DestroyWindow();
			if( !m_Doc->m_project_open )
			{
				m_Doc->m_project_modified = FALSE;
				m_Doc->m_project_modified_since_autosave = FALSE;
				m_Doc->OnFileClose();	
			}
			// restore toolbar stuff
			CMainFrame * frm = (CMainFrame*)AfxGetMainWnd();
			frm->m_wndMyToolBar.SetLists( &m_Doc->m_visible_grid, &m_Doc->m_part_grid, &m_Doc->m_routing_grid,
				m_Doc->m_visual_grid_spacing, m_Doc->m_part_grid_spacing, m_Doc->m_routing_grid_spacing, 
				m_Doc->m_snap_angle, m_Doc->m_units );
			m_View->m_dlist->SetVisibleGrid( 1, m_Doc->m_visual_grid_spacing );
			frm->SetWindowText( m_Doc->m_window_title ); 
			m_View->ShowSelectStatus();
			m_View->ShowActiveLayer(m_Doc->m_num_copper_layers);
			if(	m_View->m_cursor_mode == CUR_PART_SELECTED 
				&& m_Doc->m_edit_footprint
				&& (m_Doc->m_footprint_modified || m_Doc->m_footprint_name_changed) )
			{
				// now offer to replace the footprint of the selected part 
				m_View->OnExternalChangeFootprint( temp_footprint );
				delete temp_footprint;
			}
			m_Doc->m_edit_footprint = FALSE;	// clear this flag for next time
		}
		else
		{
			// switching to footprint view, create it
			int units = MIL;
			m_View_fp = (CFootprintView*)pNewView;
			if( m_View->m_cursor_mode == CUR_PART_SELECTED && m_Doc->m_edit_footprint )
			{
				m_View_fp->InitInstance( m_View->m_sel_part->shape );
				units = m_View->m_sel_part->shape->m_units;
			}
			else
			{
				m_View_fp->InitInstance( NULL );
			}
			// restore toolbar stuff
			CMainFrame * frm = (CMainFrame*)AfxGetMainWnd();
			frm->m_wndMyToolBar.SetLists( &m_Doc->m_fp_visible_grid, &m_Doc->m_fp_part_grid, NULL,
				m_Doc->m_fp_visual_grid_spacing, m_Doc->m_fp_part_grid_spacing, 0, m_Doc->m_fp_snap_angle, units );
			m_View_fp->m_dlist->SetVisibleGrid( 1, m_Doc->m_fp_visual_grid_spacing );
		}
		// resize window in case it changed
		CRect client_rect;
		pMainWnd->GetClientRect( client_rect );
		// TODO: replace these constants
		client_rect.top += 24;		// leave room for toolbar
		client_rect.bottom -= 18;	// leave room for status bar
		pNewView->MoveWindow( client_rect, 1 );
		//
		return TRUE;
	}
	return FALSE;
}

int CFreePcbApp::ExitInstance()
{
	return( CWinApp::ExitInstance() );
}

void CFreePcbApp::OnHelpGotoWebsite()
{
    SHELLEXECUTEINFO ShExecInfo;
	CString fn = "https://freepcb.dev";

	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = NULL;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = NULL;
	ShExecInfo.lpFile = fn;
	ShExecInfo.lpParameters = NULL;
	ShExecInfo.lpDirectory = NULL;
	ShExecInfo.nShow = SW_MAXIMIZE;
	ShExecInfo.hInstApp = NULL;

	ShellExecuteEx(&ShExecInfo);
}

void CFreePcbApp::OnFileMruFile1() 
{
	ASSERT( CWinApp::m_pRecentFileList );
	CString str = (*CWinApp::m_pRecentFileList)[0];
	m_Doc->OnFileAutoOpen( str );
	return;
}

void CFreePcbApp::OnFileMruFile2()
{
	ASSERT( CWinApp::m_pRecentFileList );
	CString str = (*CWinApp::m_pRecentFileList)[1];
	m_Doc->OnFileAutoOpen( str );
	return;
}

void CFreePcbApp::OnFileMruFile3()
{
	ASSERT( CWinApp::m_pRecentFileList );
	CString str = (*CWinApp::m_pRecentFileList)[2];
	m_Doc->OnFileAutoOpen( str );
	return;
}

void CFreePcbApp::OnFileMruFile4()
{
	ASSERT( CWinApp::m_pRecentFileList );
	CString str = (*CWinApp::m_pRecentFileList)[3];
	m_Doc->OnFileAutoOpen( str );
	return;
}

void CFreePcbApp::OnFileMruFile5()
{
	ASSERT( CWinApp::m_pRecentFileList );
	CString str = (*CWinApp::m_pRecentFileList)[4];
	m_Doc->OnFileAutoOpen( str );
	return;
}

void CFreePcbApp::OnFileMruFile6()
{
	ASSERT( CWinApp::m_pRecentFileList );
	CString str = (*CWinApp::m_pRecentFileList)[5];
	m_Doc->OnFileAutoOpen( str );
	return;
}

void CFreePcbApp::OnFileMruFile7()
{
	ASSERT( CWinApp::m_pRecentFileList );
	CString str = (*CWinApp::m_pRecentFileList)[6];
	m_Doc->OnFileAutoOpen( str );
	return;
}

BOOL CFreePcbApp::OnOpenRecentFile(UINT nID)
{
	return( CWinApp::OnOpenRecentFile( nID ) );
}

CString CFreePcbApp::GetMRUFile()
{
	CRecentFileList * pRecentFileList = CWinApp::m_pRecentFileList;
	if( pRecentFileList == NULL )
		return "";
	if( pRecentFileList->GetSize() == 0 )
		return "";
	CString str = (*CWinApp::m_pRecentFileList)[0];
	return str;
}

void CFreePcbApp::AddMRUFile( CString * str )
{
	CRecentFileList * pRecentFileList = CWinApp::m_pRecentFileList;
	if( m_pRecentFileList == NULL )
		return;
	(*CWinApp::m_pRecentFileList).Add( *str );
}

void CFreePcbApp::OnHelpKeyboardshortcuts()
{
	CDlgShortcuts dlg = new CDlgShortcuts;//ok
	dlg.DoModal();
	delete dlg;
}

void CFreePcbApp::OnToolsOpenOnlineAutorouter()
{
    SHELLEXECUTEINFO ShExecInfo;
	CString fn = m_Doc->m_app_dir + "\\freeroute.jnlp";

	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = NULL;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = NULL;
	ShExecInfo.lpFile = fn;
	ShExecInfo.lpParameters = NULL;
	ShExecInfo.lpDirectory = NULL;
	ShExecInfo.nShow = SW_MAXIMIZE;
	ShExecInfo.hInstApp = NULL;

	ShellExecuteEx(&ShExecInfo);
}

void CFreePcbApp::OnHelpFreeRoutingWebsite()
{
    SHELLEXECUTEINFO ShExecInfo;
	CString fn = "https://freerouting.org";

	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = NULL;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = NULL;
	ShExecInfo.lpFile = fn;
	ShExecInfo.lpParameters = NULL;
	ShExecInfo.lpDirectory = NULL;
	ShExecInfo.nShow = SW_MAXIMIZE;
	ShExecInfo.hInstApp = NULL;

	ShellExecuteEx(&ShExecInfo);
}

void CFreePcbApp::OnHelpFAQ()
{
	SHELLEXECUTEINFO ShExecInfo;
	CString fn = "https://freepcb.dev/How_to.html";

	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = NULL;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = NULL;
	ShExecInfo.lpFile = fn;
	ShExecInfo.lpParameters = NULL;
	ShExecInfo.lpDirectory = NULL;
	ShExecInfo.nShow = SW_MAXIMIZE;
	ShExecInfo.hInstApp = NULL;

	ShellExecuteEx(&ShExecInfo);
}

void CFreePcbApp::OnHelpUserGuidePdf()
{
    SHELLEXECUTEINFO ShExecInfo;
	CString fn = m_Doc->m_app_dir + "\\doc\\freepcb_user_guide.pdf";

	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = NULL;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = NULL;
	ShExecInfo.lpFile = fn;
	ShExecInfo.lpParameters = NULL;
	ShExecInfo.lpDirectory = NULL;
	ShExecInfo.nShow = SW_MAXIMIZE;
	ShExecInfo.hInstApp = NULL;

	ShellExecuteEx(&ShExecInfo);
}

void CFreePcbApp::OnHelpUserGuideSupplementPdf()
{
    SHELLEXECUTEINFO ShExecInfo;
	CString fn = m_Doc->m_app_dir + "\\doc\\freepcb_user_guide_supplement.pdf";

	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = NULL;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = NULL;
	ShExecInfo.lpFile = fn;
	ShExecInfo.lpParameters = NULL;
	ShExecInfo.lpDirectory = NULL;
	ShExecInfo.nShow = SW_MAXIMIZE;
	ShExecInfo.hInstApp = NULL;

	ShellExecuteEx(&ShExecInfo);
}

void CFreePcbApp::OnHelpFpcRoute()
{
    SHELLEXECUTEINFO ShExecInfo;
	CString fn = m_Doc->m_app_dir + "\\doc\\fpcroute_user_guide.pdf";

	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = NULL;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = NULL;
	ShExecInfo.lpFile = fn;
	ShExecInfo.lpParameters = NULL;
	ShExecInfo.lpDirectory = NULL;
	ShExecInfo.nShow = SW_MAXIMIZE;
	ShExecInfo.hInstApp = NULL;

	ShellExecuteEx(&ShExecInfo);
}

int CFreePcbApp::DoMessageBox(LPCTSTR lpszPrompt, UINT nType, UINT nIDPrompt)
{
	// show cursor
	CMainFrame * frm = (CMainFrame*)AfxGetMainWnd();
	::ShowCursor( TRUE );
	int ret = CWinApp::DoMessageBox(lpszPrompt, nType, nIDPrompt);
	::ShowCursor( FALSE );
	return ret;
}

