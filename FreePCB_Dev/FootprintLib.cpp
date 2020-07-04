// imlementation of classes to manage footprint libraries
//
#include "stdafx.h"
#include "FootprintLib.h"
#include "DlgLog.h"
#include "DlgAddPart.h"
#include "DlgMyMessageBox.h"

BOOL bDisableMessageIfFileNotFound = FALSE;

// index one library file
// file_name is just the file name, not the complete path
//
void CFootLibFolder::IndexLib( CString * file_name, CDlgLog * dlog )
{
	// see if file_name already exists in index
	int n_libs = m_footlib.GetSize();
	CString full_path = m_full_path_to_folder + "\\" + *file_name;
	int nlib = n_libs;
	for( int ilib=0; ilib<n_libs; ilib++ )
	{
		if( m_footlib[ilib].m_full_path == full_path )
		{
			// found it
			nlib = ilib;
			break;
		}
	}
	if( nlib < n_libs )
	{
		// file has been indexed previously, we are re-indexing it
		// remove all previous entries
		for( int i=0; i<GetNumFootprints( nlib ); i++ )
			m_lib_map.RemoveKey( *GetFootprintName( nlib, i ) );
	}
	else
	{
		// new file to be added to index
		m_footlib.SetSize( nlib+1 );	// add one to array
		m_footlib[nlib].m_file_name = *file_name;
		m_footlib[nlib].m_full_path = full_path;
	}
	// now index the file
	int n_footprints = 0;

	CStdioFile file;
	int OK = file.Open( m_footlib[nlib].m_full_path, CFile::modeRead );
	if( !OK && !bDisableMessageIfFileNotFound )     
	{
		CDlgMyMessageBox dlg;
		CString mess = "Unable to open library file\n\n \"" + m_footlib[nlib].m_full_path + "\"";
		dlg.Initialize( mess );
		dlg.DoModal();
		bDisableMessageIfFileNotFound = dlg.bDontShowBoxState;
	}

	CString instr;
	int pos = 0;
	int last_ih = 0;
	int last_if = -1;
	if( OK ) 
	{
		while( file.ReadString( instr ) )
		{
			if( instr.Left(5) == "name:" )
			{
				// found a footprint
				// if there was a previous footprint, save offset to next one
				if( last_if != -1 )
					m_footlib[nlib].m_foot[last_if].m_next_offset = file.GetPosition();
				CString shape_name = instr.Right( instr.GetLength()-5 );
				shape_name.Trim();
				if( shape_name.Right(1) == "\"" )
					shape_name = shape_name.Left( shape_name.GetLength() -1 );
				if( shape_name.Left(1) == "\"" )
					shape_name = shape_name.Right( shape_name.GetLength() -1 );
				shape_name.Trim();
				if( n_footprints >= (m_footlib[nlib].m_foot.GetSize()-1) )
					m_footlib[nlib].m_foot.SetSize(n_footprints + 100 );
				m_footlib[nlib].m_foot[n_footprints].m_name = shape_name;
				m_footlib[nlib].m_foot[n_footprints].m_offset = pos;
				unsigned int i;
				i = (nlib<<24) + pos;
				m_lib_map.SetAt( shape_name, (void*)i );
				// save indices to this footprint
				last_if = n_footprints;
				// next
				n_footprints++;
			}
			if( instr.Left(8) == "package:" && n_footprints>0 )
			{
				CString package_name = instr.Right( instr.GetLength()-8 );
				package_name.Trim();
				if( package_name.Right(1) == "\"" )
					package_name = package_name.Left( package_name.GetLength() -1 );
				if( package_name.Left(1) == "\"" )
					package_name = package_name.Right( package_name.GetLength() -1 );
				package_name.Trim();
				m_footlib[nlib].m_foot[n_footprints-1].m_package = package_name;
			}
			pos = file.GetPosition();
		}
		// set next_offset of last footprint to -1
		if( last_if != -1 )
			m_footlib[nlib].m_foot[last_if].m_next_offset = -1;
		// set array sizes
		m_footlib[nlib].m_foot.SetSize( n_footprints );
		SetExpanded( nlib, FALSE );
		file.Close();
		m_footlib[nlib].m_indexed = TRUE;
	}
}

// index all of the library files in a folder
//
void CFootLibFolder::IndexAllLibs( CString * full_path, CDlgLog * dlg_log )
{
	Clear();
	m_full_path_to_folder = *full_path;

	// start looking for library files
	CFileFind finder;
	if( _chdir( m_full_path_to_folder ) != 0 )
	{
		CString mess;
		mess.Format( "Unable to open library folder \"%s\"", m_full_path_to_folder );
		AfxMessageBox( mess );
		*full_path = "";
	}
	else
	{
		// pop up log window
		dlg_log->ShowWindow( SW_SHOW );
		dlg_log->UpdateWindow();
		dlg_log->BringWindowToTop();

		BOOL bWorking = finder.FindFile( "*.fpl" );
		while (bWorking)
		{
			bWorking = finder.FindNextFile();
			CString fn = finder.GetFileName();
			if( !finder.IsDirectory() ) 
			{
				// found a library file, index it
				CString log_message;
				log_message.Format( "Indexing library: \"%s\"\r\n", fn );
				dlg_log->AddLine( log_message );
				IndexLib( &fn );
			}
		}
	}
	finder.Close();
}

// clear all data
//
void CFootLibFolder::Clear()
{
	m_footlib.RemoveAll();
	m_lib_map.RemoveAll();
	m_full_path_to_folder = "";
}

// returns the number of libraries
//
int CFootLibFolder::GetNumLibs()
{ 
	return m_footlib.GetSize(); 
}

// returns the full path to the folder
//
CString * CFootLibFolder::GetFullPath()
{ 
	return &m_full_path_to_folder; 
}

// get info about a footprint
// enter with:
//	name = pointer to CString containing footprint name
//  prefer_file_path = pointer to preferred file to search
//	ilib = pointer to variable to receive library index (or NULL)
//	file_name = pointer to CString to receive lib file name (or NULL)
//	offset = pointer to variable to receive position of footprint in lib file (or NULL)
// returns FALSE if fails, TRUE if succeeds with:
//	*ilib = index into array of libraries
//	*file_name = full path to library file
//	*offset = offset into library file
//
BOOL CFootLibFolder::GetFootprintInfo( CString * name, CString * prefer_lib_file, 
									  int * ilib, int * ifootprint, CString * file_name, 
									  int * offset, int * next_offset ) 
{
	void * ptr;
	int m_ilib;
	int m_if = -1;
	int m_offset;

	BOOL bExists = FALSE;
	int prefer_lib_index = -1;
	if( prefer_lib_file )
	{
		// search for preferred folder
		for( int il=0; il<m_footlib.GetSize(); il++ )
		{
			if( m_footlib[il].m_full_path == *prefer_lib_file )
			{
				prefer_lib_index = il;
				// now search for footprint
				for( int in=0; in<m_footlib[il].m_foot.GetSize(); in++ )
				{
					if( m_footlib[il].m_foot[in].m_name == *name )
					{
						m_ilib = il;
						bExists = TRUE;
					}
				}
				if( bExists )
					break;
			}
			if( bExists )
				break;
		}
	}
	if( !bExists )
	{
		// look for any file path
		bExists = m_lib_map.Lookup( *name, ptr );
		UINT32 pos = (UINT32)ptr;
		m_ilib = pos>>24;
	}
	if( bExists )
	{
		CString m_file_name = m_footlib[m_ilib].m_full_path;
		// search arrays for file
		for( int i=0; i<m_footlib[m_ilib].m_foot.GetSize(); i++ )
		{
			if( m_footlib[m_ilib].m_foot[i].m_name == *name )
			{
				m_if = i;
				break;
			}
		}
		if( m_if == -1 )
			ASSERT(0);
		// OK
		if( ilib )
			*ilib = m_ilib;
		if( ifootprint )
			*ifootprint = m_if;
		if( file_name )
			*file_name = m_file_name;
		if( offset )
			*offset = m_footlib[m_ilib].m_foot[m_if].m_offset;
		if( next_offset )
			*next_offset = m_footlib[m_ilib].m_foot[m_if].m_next_offset;
		return TRUE;
	}
	else
		return FALSE;
}

// search for a file name in library
// returns index to file, or -1 if not found
//
int CFootLibFolder::SearchFileName( CString * fn )
{
	for( int i=0; i<m_footlib.GetSize(); i++ )
	{
		if( m_footlib[i].m_full_path == *fn )
			return i;
	}
	return -1;	// if file not found
}

// get library file full path
CString * CFootLibFolder::GetLibraryFullPath( int ilib )
{ 
	return &m_footlib[ilib].m_full_path; 
}

// get library file name
CString * CFootLibFolder::GetLibraryFileName( int ilib )
{ 
	return &m_footlib[ilib].m_file_name; 
}

// get number of footprints under a heading in library file
int CFootLibFolder::GetNumFootprints( int ilib )
{ 
	return m_footlib[ilib].m_foot.GetSize(); 
}

// get footprint name
CString * CFootLibFolder::GetFootprintName( int ilib, int ifoot )
{ 
	return &m_footlib[ilib].m_foot[ifoot].m_name; 
}

CString * CFootLibFolder::GetFootprintPackage( int ilib, int ifoot )
{
	return &m_footlib[ilib].m_foot[ifoot].m_package; 
}

// get footprint offset
int CFootLibFolder::GetFootprintOffset( int ilib, int ifoot )
{ 
	return m_footlib[ilib].m_foot[ifoot].m_offset; 
}

//********************* class CFootLibFolderMap *************************
//

// constructor
//
CFootLibFolderMap::CFootLibFolderMap()
{
}

// destructor, also destroys all CFootLibFolders in the map
//
CFootLibFolderMap::~CFootLibFolderMap()
{
	POSITION pos;
	CString name;
	void * ptr;
	for( pos = m_folder_map.GetStartPosition(); pos != NULL; )
	{
		m_folder_map.GetNextAssoc( pos, name, ptr );
		CFootLibFolder * folder = (CFootLibFolder*)ptr;
		if( folder )
		{
			folder->Clear();
			delete folder;
		}
	}
}

// add CFootLibFolder to map
// if folder == NULL, the folder will be created on the first call to GetFolder
//
void CFootLibFolderMap::AddFolder( CString * full_path, CFootLibFolder * folder  )
{
	CString str = full_path->MakeLower();
	void * ptr;
	if( !m_folder_map.Lookup( str, ptr ) )
		m_folder_map.SetAt( str, folder );
}

CFootLibFolder * CFootLibFolderMap::GetFolder( CString * full_path, CDlgLog * log )
{
	void * ptr;
	CFootLibFolder * folder;
	CString str = full_path->MakeLower();
	if( !m_folder_map.Lookup( str, ptr ) || ptr == NULL )
	{
		folder = new CFootLibFolder();// ok
		CString mess;
		mess.Format( "Indexing library folder \"%s\"\r\n", str );
		log->AddLine( mess );
		folder->IndexAllLibs( &str, log );
		if( str.GetLength() == 0 )
			str = *GetDefaultFolder();
		log->AddLine( "\r\n" );
		m_folder_map.SetAt( str, folder );
		log->AddLine( "\r\n" );
	}
	else
		folder = (CFootLibFolder*)ptr;
	return folder;
}

BOOL CFootLibFolderMap::FolderIndexed( CString * full_path )
{
	void * ptr;
	CString str = full_path->MakeLower();
	if( m_folder_map.Lookup( str, ptr ) )
		return TRUE;
	else
		return FALSE;
}

void CFootLibFolderMap::SetDefaultFolder( CString * def_full_path )
{
	m_default_folder = *def_full_path;
}

void CFootLibFolderMap::SetLastFolder( CString * last_full_path )
{
	m_last_folder = *last_full_path;
}

CString * CFootLibFolderMap::GetDefaultFolder()
{
	return( &m_default_folder );
}

CString * CFootLibFolderMap::GetLastFolder()
{
	if( m_last_folder.GetLength() )
		return( &m_last_folder );
	else
		return( &m_default_folder );
}

