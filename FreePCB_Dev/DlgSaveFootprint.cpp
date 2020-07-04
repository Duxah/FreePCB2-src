// DlgSaveFootprint.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgSaveFootprint.h"
#include "PathDialog.h"

// globals
CString gLastFolderName;	// folder of last footprint imported or saved
CString gLastFileName;		// file name of last footprint imported or saved

// CDlgSaveFootprint dialog

IMPLEMENT_DYNAMIC(CDlgSaveFootprint, CDialog)
CDlgSaveFootprint::CDlgSaveFootprint(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgSaveFootprint::IDD, pParent)
{
}

CDlgSaveFootprint::~CDlgSaveFootprint()
{
}

void CDlgSaveFootprint::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PREVIEW2, m_preview);
	DDX_Control(pDX, IDC_EDIT_SAVE_NAME, m_edit_name);
	DDX_Control(pDX, IDC_COMBO_LIBS, m_combo_lib);
	DDX_Control(pDX, IDC_EDIT_AUTHOR, m_edit_author);
	DDX_Control(pDX, IDC_EDIT_SOURCE, m_edit_source);
	DDX_Control(pDX, IDC_EDIT_DESC, m_edit_desc);
	DDX_Control(pDX, IDC_EDIT_FP_SAVE_FOLDER, m_edit_folder);
	DDX_Control(pDX, IDC_STATIC_UNITS, m_static_units);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		// draw preview of footprint
		CMetaFileDC m_mfDC;
		CDC * pDC = this->GetDC();
		RECT rw;
		m_preview.GetClientRect( &rw );
		int x_size = rw.right - rw.left;
		int y_size = rw.bottom - rw.top;
		HENHMETAFILE hMF = m_footprint->CreateMetafile( &m_mfDC, pDC, x_size, y_size );
		m_preview.SetEnhMetaFile( hMF );
		ReleaseDC( pDC );
		// initialize other fields
		if( m_units == MM )  
			m_static_units.SetWindowText( " mm" );
		else
			m_static_units.SetWindowText( " mil" );
		m_edit_name.SetWindowText( m_name );
		m_edit_author.SetWindowText( m_footprint->m_author );
		m_edit_source.SetWindowText( m_footprint->m_package );
		m_edit_desc.SetWindowText( m_footprint->m_desc );
		// insert all library file names
		CString * def_lib = m_foldermap->GetLastFolder();
		if( (*def_lib).GetLength() == 0 )
			def_lib = m_foldermap->GetDefaultFolder();
		m_folder = m_foldermap->GetFolder( def_lib, m_dlg_log );
		m_edit_folder.SetWindowText( *def_lib );
		InitFileList();
	}
}

void CDlgSaveFootprint::Initialize( CString * name, 
								    CShape * footprint,							 
								    int units,	
									LPCTSTR default_file_name,
									CMapStringToPtr * shape_cache_map,
									CFootLibFolderMap * footlibfoldermap,
									CDlgLog * log )

{
	m_name = *name;
	m_units = units;
	if( ((CString)default_file_name).GetLength() )
		m_default_filename = default_file_name;
	else
		m_default_filename = gLastFileName;
	m_footprint = footprint;
	m_footprint_cache_map = shape_cache_map;
	m_foldermap = footlibfoldermap;
	CString * last_folder_str = footlibfoldermap->GetLastFolder();
	m_dlg_log = log;
	m_folder = footlibfoldermap->GetFolder( last_folder_str, m_dlg_log );
}

BEGIN_MESSAGE_MAP(CDlgSaveFootprint, CDialog)
	ON_CBN_SELCHANGE(IDC_COMBO_LIBS, OnCbnSelchangeComboLibs)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON_SAVE_FP_BROWSE, OnBnClickedButtonBrowse)
END_MESSAGE_MAP()


// CDlgSaveFootprint message handlers

void CDlgSaveFootprint::OnCbnSelchangeComboLibs()
{
	int ilib = m_combo_lib.GetCurSel();
}

void CDlgSaveFootprint::OnBnClickedOk()
{
	// update name and other strings
	// check folder name
	m_edit_folder.GetWindowText( m_folder_name );
	CString Slowercase1 = m_folder_name;
	CString Slowercase2 = *m_folder->GetFullPath();
	if( Slowercase1.CompareNoCase(Slowercase2) != 0 )
	{
		// folder was changed without browsing, regenerate library
		int ret = _chdir( m_folder_name );
		if( ret == -1 )
		{
			// folder doesn't exist
			CString mess = "Folder \"" + m_folder_name + "\" doesn't exist\n";
			mess += "Create it ?";
			ret = AfxMessageBox( mess, MB_YESNO );
			if( ret == IDNO )
				return;
			// create it
			ret = _mkdir( m_folder_name );
			if( ret == -1 )
			{
				// can't create folder
				CString mess = "Unable to create folder \"";
				mess += m_folder_name;
				AfxMessageBox( mess );
				return;
			}
		}
		// index folder
		CFootLibFolder * new_folder = new CFootLibFolder;// ok
		CString mess;
		mess.Format( "Indexing library folder \"%s\"\r\n", m_folder_name );
		m_dlg_log->AddLine( mess );
		new_folder->IndexAllLibs( &m_folder_name, m_dlg_log );
		m_dlg_log->AddLine( "\r\n" );
		if( m_folder_name.GetLength() )
		{
			m_foldermap->AddFolder( &m_folder_name, new_folder );
			m_folder = m_foldermap->GetFolder( &m_folder_name, m_dlg_log );
			m_foldermap->SetLastFolder( &m_folder_name );
		}
		InitFileList();
		delete new_folder;
	}
	m_edit_name.GetWindowText( m_name );
	m_name.Replace( '\"', '\'' );			// replace any " with '
	if( m_name.GetLength() > CShape::MAX_NAME_SIZE )
	{
		CString mess;
		mess.Format( "Name too long, can't exceed %d characters",
			CShape::MAX_NAME_SIZE );
		AfxMessageBox( mess );
		return;
	}
	m_footprint->m_units = m_units;
	m_footprint->m_name = m_name;
	m_edit_author.GetWindowText( m_author );
	m_author.Replace( '\"', '\'' );
	m_footprint->m_author = m_author.Trim();
	m_edit_source.GetWindowText( m_package );
	m_package.Replace( '\"', '\'' );
	m_footprint->m_package = m_package.Trim();
	m_edit_desc.GetWindowText( m_desc );
	m_desc.Replace( '\"', '\'' );
	m_footprint->m_desc = m_desc.Trim();

	// get footprint name, file name
	CString file_name;
	m_combo_lib.GetWindowText( file_name );
	CString file_path = *m_folder->GetFullPath() + "\\" + file_name;

	// save file name and folder
	gLastFileName = file_name;
	gLastFolderName = *m_folder->GetFullPath();

	// now check for duplication of existing footprint
	int ilib;
	int offset;
	int ifootprint;
	int next_offset;
	CString fn;
	BOOL footprint_exists = m_folder->GetFootprintInfo( &m_name, &file_path,
		&ilib, &ifootprint, &fn, &offset, &next_offset );
	BOOL same_file = ( fn == file_path );
	BOOL replace = FALSE;
	if( footprint_exists && same_file )
	{
		// footprint name already exists in this file
		CString mess;
		mess.Format( "Footprint \"%s\" already exists in file \"%s\"\nOverwrite ?",
			m_name, fn );
		int ret = AfxMessageBox( mess, MB_OKCANCEL );
		if( ret == IDOK )
		{
			replace = TRUE;
		}
		else
			return;
	}
	else if( footprint_exists && !same_file )
	{
		// footprint name already exists in another file
		CString mess;
		mess.Format( "Footprint \"%s\" already exists in another library file \"%s\"\nDo you want to change the name?",
			m_name, fn );
		int ret = AfxMessageBox( mess, MB_YESNO );
		if( ret == IDYES )
		{
			return;
		}
	}
	// OK, save it

	BOOL file_exists = ( m_folder->SearchFileName( &file_path ) != -1 );
	if( !file_exists )
	{
		// new library file, create it and write footprint
		CStdioFile f;
		BOOL ok = f.Open( file_path, CFile::modeCreate | CFile::modeWrite );
		if( !ok )
		{
			AfxMessageBox( "Unable to open file " + file_path );
			return;
		}
		m_footprint->WriteFootprint( &f );
		f.Close();
	}
	else if( file_exists && !replace )
	{
		// existing library file, insert new footprint
		int offset;
		// no heading, insert ahead of first footprint or heading in file
		CStdioFile * fin = new CStdioFile( file_path, CFile::modeRead );// ok
		CStdioFile * fout = new CStdioFile( "temp.txt", CFile::modeCreate | CFile::modeWrite );// ok
		CString in_str;
		BOOL bInserted = FALSE;
		// now loop through all lines in file
		while( fin->ReadString( in_str ) )
		{
			in_str.Trim();
			if( !bInserted && (in_str.Left(1) == "[" || in_str.Left(5) == "name:") )
			{
				// insert footprint
				m_footprint->WriteFootprint( fout );
				// now copy last line read from source
				fout->WriteString( in_str + "\n" );
				bInserted = TRUE;
			}
			else
			{
				// write line
				fout->WriteString( in_str + "\n" );
			}
		}
		delete fin;
		delete fout;
		int err = remove( file_path );
		if( err )
		{
			CString str = "File system error: Unable to modify library file\n";
			str += "File could be read-only or you don't have permission to modify it\n";
			str += "The modified file has been saved as \"temp.txt\"";
			AfxMessageBox( str );
			return;
		}
		else
		{
			err = rename( "temp.txt", file_path );
			if( err )
			{
				CString str = "File system error: Unable to modify library file\n";
				str += "The original file has been deleted but can't be rewritten\n";
				str += "The modified file has been saved as \"temp.txt\"";
				AfxMessageBox( str );
				return;
			}
		}
	}
	else
	{
		// replace existing footprint
		CStdioFile * fin = new CStdioFile( file_path, CFile::modeRead );// ok
		CStdioFile * fout = new CStdioFile( "temp.txt", CFile::modeCreate | CFile::modeWrite );// ok
		CString in_str;
		BOOL bDeleted = FALSE;
		BOOL bInserted = FALSE;
		// now loop through all lines in file
		while( fin->ReadString( in_str ) )
		{
			in_str.Trim();
			if( !bDeleted && fin->GetPosition() <= offset )
			{
				// haven't reached footprint, write line
				fout->WriteString( in_str + "\n" );
			}
			else if( !bDeleted && next_offset<0 )
			{
				// reached footprint, this is the last footprint in the file
				bDeleted = TRUE;
			}
			else if( !bDeleted && fin->GetPosition() < next_offset )
			{
				// reached footprint, skip lines to next footprint
			}
			else if( !bInserted )
			{
				// we have deleted the original footprint, insert replacement
				bDeleted = TRUE;
				m_footprint->WriteFootprint( fout );
				// if this was the last footprint in the file, we are done
				if( next_offset < 0 )
					break;
				// otherwise, copy last line read from source
				fout->WriteString( in_str + "\n" );
				bInserted = TRUE;
			}
			else
			{
				// finished inserting, this was no the last footprint
				// copy subsequent lines
				fout->WriteString( in_str + "\n" );
			}
		}
		delete fin;
		delete fout;
		int err = remove( file_path );
		if( err )
		{
			CString str = "File system error: Unable to modify library file\n";
			str += "File could be read-only or you don't have permission to modify it\n";
			str += "The modified file has been saved as \"temp.txt\"";
			AfxMessageBox( str );
			return;
		}
		else
		{
			err = rename( "temp.txt", file_path );
			if( err )
			{
				CString str = "File system error: Unable to modify library file\n";
				str += "The original file has been deleted but can't be rewritten\n";
				str += "The modified file has been saved as \"temp.txt\"";
				AfxMessageBox( str );
				return;
			}
		}
	}
	// now index the file
	m_folder->IndexLib( &file_name );
	OnOK();
}

void CDlgSaveFootprint::OnBnClickedButtonBrowse()
{
	CPathDialog dlg( "Open Folder", "Select footprint library folder", *m_folder->GetFullPath() );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		CString path_str = dlg.GetPathName();
		m_edit_folder.SetWindowText( path_str );
		m_folder = m_foldermap->GetFolder( &path_str, m_dlg_log );
		if( !m_folder )
		{
			CFootLibFolder * new_folder = new CFootLibFolder;// ok
			CString mess;
			mess.Format( "Indexing library folder \"%s\"\r\n", path_str );
			m_dlg_log->AddLine( mess );
			new_folder->IndexAllLibs( &path_str, m_dlg_log );
			m_dlg_log->AddLine( "\r\n" );
			if( path_str.GetLength() )
			{
				m_foldermap->AddFolder( &path_str, new_folder );
				m_folder = new_folder;
				m_foldermap->SetLastFolder( &path_str );
			}
			delete new_folder;
		}
		InitFileList();
	}
}

// initialize file list, selecting the last file name used
// add "user_created.fpl" and last file name if necessary
// uses global gLastFileName
//
void CDlgSaveFootprint::InitFileList()
{
	// insert all library names, checking for last file name and "user_created.fpl"
	m_lib_user = -1;
	m_lib_last = -1;
	int cur_last = -1;
	CString lib_str,gLast="";
	char file_str[_MAX_FNAME];
	m_combo_lib.GetWindowText(gLast);
	// clear combo box
	while( m_combo_lib.GetCount() != 0 )
		m_combo_lib.DeleteString(0);

	// now add library files
	int nlibs = m_folder->GetNumLibs();
	for( int ilib=0; ilib<nlibs; ilib++ )
	{
		lib_str = *m_folder->GetLibraryFullPath( ilib );
		_splitpath( lib_str, NULL, NULL, file_str, NULL );
		strcat( file_str, ".fpl" );
		m_combo_lib.InsertString( -1, file_str );
		CString Sfile_str = file_str;
		if( file_str == gLastFileName )
			m_lib_last = ilib;
		if( Sfile_str.Compare("user_created.fpl") == 0 )
			m_lib_user = ilib;
		if( Sfile_str.Compare(gLast) == 0 )
			cur_last = ilib;
	}
	// see if the last file name was found
	if( m_lib_last < 0 && gLastFileName.GetLength() )
	{
		// no, add last file name to end of list
		m_combo_lib.InsertString( -1, gLastFileName );
		m_lib_last = m_folder->GetNumLibs();
		// and select it
		m_combo_lib.SetCurSel( m_lib_last );
	}
	else if( m_lib_last >= 0 ) 
	{
		// yes, select last file name
		m_combo_lib.SetCurSel( m_lib_last );
	}
	// see if "user_created.fpl" was found
	if( m_lib_user < 0 )
	{
		// file "user_created.fpl" doesn't exist, add name to end of list
		m_combo_lib.InsertString( -1, "user_created.fpl" );
		m_lib_user = m_folder->GetNumLibs();
	}
	// if no last file name, select user_created.fpl
	if( m_lib_last < 0 )
		m_combo_lib.SetCurSel( m_lib_user );
	if( cur_last >= 0 )
		m_combo_lib.SetCurSel( cur_last );
}
