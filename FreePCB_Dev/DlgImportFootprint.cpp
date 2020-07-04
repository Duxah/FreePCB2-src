// DlgImportFootprint.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgImportFootprint.h"
#include "PathDialog.h"

//globals
extern CString gLastFileName;		// last file name imported
extern CString gLastFolderName;		// last folder name imported
extern BOOL gLocalCacheExpanded;

// CDlgImportFootprint dialog

IMPLEMENT_DYNAMIC(CDlgImportFootprint, CDialog)
CDlgImportFootprint::CDlgImportFootprint(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgImportFootprint::IDD, pParent)
{
	m_footlibfolder = NULL;
	m_footprint_name = "";
	m_footprint_filename = "";
	m_footprint_folder = "";
	m_shape.m_name = "";
}

CDlgImportFootprint::~CDlgImportFootprint()
{
}

void CDlgImportFootprint::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BUTTON_BROWSE_LIB_FOLDER, m_button_browse);
	DDX_Control(pDX, IDC_EDIT_LIB_FOLDER, m_edit_library_folder);
	DDX_Control(pDX, IDC_PART_LIB_TREE, part_tree);
	DDX_Control(pDX, IDC_PREVIEW, m_preview);
	DDX_Control(pDX, IDC_EDIT_IMPORT_AUTHOR, m_edit_author);
	DDX_Control(pDX, IDC_EDIT_IMPORT_SOURCE, m_edit_source);
	DDX_Control(pDX, IDC_EDIT_IMPORT_DESC, m_edit_desc);
}


BEGIN_MESSAGE_MAP(CDlgImportFootprint, CDialog)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_LIB_FOLDER, OnBnClickedButtonBrowseLibFolder)
	ON_NOTIFY(TVN_SELCHANGED, IDC_PART_LIB_TREE, OnTvnSelchangedPartLibTree)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
END_MESSAGE_MAP()


// CDlgImportFootprint message handlers


void CDlgImportFootprint::InitInstance( CMapStringToPtr * shape_cache_map,
							 CFootLibFolderMap * foldermap, CDlgLog * log )
{
	m_footprint_cache_map = shape_cache_map;
	m_foldermap = foldermap;
	CString * path_str = foldermap->GetLastFolder();
	m_footlibfolder = foldermap->GetFolder( path_str, log );
	m_dlg_log = log;
}

BOOL CDlgImportFootprint::OnInitDialog()
{
	CDialog::OnInitDialog();
	InitPartLibTree();
	return TRUE;  // return TRUE unless you set the focus to a control
}



void CDlgImportFootprint::OnBnClickedButtonBrowseLibFolder()
{
	static CString path = "";
	CPathDialog dlg( "Open Folder", "Select footprint library folder", *m_footlibfolder->GetFullPath() );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		CString path_str = dlg.GetPathName();
		m_edit_library_folder.SetWindowText( path_str );
		m_footlibfolder = m_foldermap->GetFolder( &path_str, m_dlg_log );
		if( !m_footlibfolder )
		{
			ASSERT(0);
		}
		InitPartLibTree();
		m_foldermap->SetLastFolder( &path_str );
	}
}

void CDlgImportFootprint::OnTvnSelchangedPartLibTree(NMHDR *pNMHDR, LRESULT *pResult)
{
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
		CString str = "";
		if( m_ilib == 0 )
		{
			m_in_cache = TRUE;
			POSITION pos;
			CString key;
			void * ptr;
			pos = m_footprint_cache_map->GetStartPosition();
			for( int i=0; i<=m_ifoot; i++ )
			{
				m_footprint_cache_map->GetNextAssoc( pos, key, ptr );
			}
			str = key;
		}
		else
		{
			m_ilib--;
			m_in_cache = FALSE;
			str = *m_footlibfolder->GetFootprintName( m_ilib, m_ifoot );
		}
		m_footprint_name = str;

		// draw footprint preview in control
		void * ptr;
		// lookup shape in cache
		BOOL bInCache = m_footprint_cache_map->Lookup( m_footprint_name, ptr );
		if( bInCache )
		{
			// found it, make shape
			m_shape.Copy( (CShape*)ptr );
			m_footprint_filename = "";
			m_footprint_folder = "";
		}
		else
		{
			// not in cache, get from library file
			CString * lib_file_name = m_footlibfolder->GetLibraryFullPath( m_ilib );
			int offset = m_footlibfolder->GetFootprintOffset( m_ilib, m_ifoot );
			// make shape from library file
			int err = m_shape.MakeFromFile( NULL, m_footprint_name, *lib_file_name, offset ); 
			if( err )
			{
				// unable to make shape
				ASSERT(0);
			}
			BOOL bOK = ::SplitString( lib_file_name, 
				&m_footprint_folder, &m_footprint_filename, '\\', TRUE );
		}
		// now draw preview of footprint
		CMetaFileDC m_mfDC;
		CDC * pDC = this->GetDC();
		RECT rw;
		m_preview.GetClientRect( &rw );
		int x_size = rw.right - rw.left;
		int y_size = rw.bottom - rw.top;
		HENHMETAFILE hMF;
		hMF = m_shape.CreateMetafile( &m_mfDC, pDC, x_size, y_size );
		m_preview.SetEnhMetaFile( hMF );
		ReleaseDC( pDC );
		// update text strings
		m_edit_author.SetWindowText( m_shape.m_author );
		m_edit_source.SetWindowText( m_shape.m_package );
		m_edit_desc.SetWindowText( m_shape.m_desc );
	}
	*pResult = 0;
}

// Initialize the tree control representing the footprint library and cache
//
void CDlgImportFootprint::InitPartLibTree()
{
	CString str;
	LPCSTR p;

	// initialize folder name
	m_edit_library_folder.SetWindowText( *m_footlibfolder->GetFullPath() );
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

	if( gLocalCacheExpanded )
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
		tvInsert.hInsertAfter = 0;
		tvInsert.hParent = hLocal;
		tvInsert.item.pszText = (LPSTR)p;
		tvInsert.item.lParam = (LPARAM)i;
		pCtrl->InsertItem(&tvInsert);
		i++;
	}

	// insert all library names
	HTREEITEM hLib;
	HTREEITEM hLib_last;
	HTREEITEM hHead;
	HTREEITEM hHead_last;
	// loop through libraries
	for( int ilib=0; ilib<m_footlibfolder->GetNumLibs(); ilib++ )
	{
		// put library filename into Tree
		str = *m_footlibfolder->GetLibraryFileName( ilib );
		p = (LPCSTR)str;
		tvInsert.hParent = NULL;
		tvInsert.item.pszText = (LPSTR)p;
		if( ilib == 0 )
			tvInsert.hInsertAfter = hLocal;
		else
			tvInsert.hInsertAfter = hLib_last;
		tvInsert.item.lParam = -1;
		hLib = pCtrl->InsertItem(&tvInsert);	// insert library name

		if( m_footlibfolder->GetExpanded( ilib ) )
			part_tree.SetItemState( hLib, TVIS_EXPANDED, TVIS_EXPANDED );

		hLib_last = hLib;

		// loop through footprints in heading
		for( int i=0; i<m_footlibfolder->GetNumFootprints(ilib); i++ )
		{
			// put footprint into tree
			str = *m_footlibfolder->GetFootprintName( ilib, i );
			p = (LPCSTR)str;
			tvInsert.hParent = hLib;
			tvInsert.item.pszText = (LPSTR)p;
			UINT32 lp = (ilib+1)*0x1000000 + i;
			tvInsert.item.lParam = (LPARAM)lp;
			tvInsert.hInsertAfter = 0;
			pCtrl->InsertItem(&tvInsert);
		}
	}
}


void CDlgImportFootprint::OnBnClickedOk()
{
	// get state of tree control so we can reproduce it next time
	// get next top-level item
	HTREEITEM hItem = part_tree.GetNextItem( NULL, TVGN_CHILD );
	// get all items
	int ilib = -1;
	while( hItem )
	{
		// top-level item
		BOOL expanded = TVIS_EXPANDED & part_tree.GetItemState( hItem, TVIS_EXPANDED );
		CString str;
		if( ilib == -1 )
			gLocalCacheExpanded = expanded & TVIS_EXPANDED;
		else
			m_footlibfolder->SetExpanded( ilib, expanded & TVIS_EXPANDED );
		// get next top-level item
		hItem = part_tree.GetNextItem( hItem, TVGN_NEXT );
		ilib++;
	}
	// save filename and folder of footprint to be imported
	gLastFileName = m_footprint_filename;
	gLastFolderName = m_footprint_folder;
	OnOK();
}

void CDlgImportFootprint::OnBnClickedCancel()
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
			m_footlibfolder->SetExpanded( ilib, expanded & TVIS_EXPANDED );
		// get next top-level item
		item = part_tree.GetNextItem( item, TVGN_NEXT );
		ilib++;
	}
	OnCancel();
}
