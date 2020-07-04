///
/// @file	QAFDebug.cpp "../Src/QAFDebug.cpp"
/// @brief	Implementation of functions for reporting critical errors.
///         
///			This is the implementation of the "QAFDebug.h" macros.  
///

#include "stdafx.h"

#include <malloc.h>
#include <shlobj.h>

#include "QAFDebug.h"

// Only if reporting is not disabled
#ifndef QAF_DISABLED

//** AMW
CString QAF_LOG_DIR = "";

////////////////////////////////////////////////////////////////////////////////////
// Defines and constants
////////////////////////////////////////////////////////////////////////////////////

/// Size of the shared flags stored in the memory-mapped-file 
const DWORD QDEBUG_SHMEMSIZE = sizeof(DWORD);

/// Format string for HRESULT and the buffer length for formatting
const DWORD QAFDEBUG_FMT_HRESULT_LEN = 20;
const LPCTSTR QAFDEBUG_FMT_HRESULT = _T("FAILED(0x%08X)");

// DebugOutString() for Visual Studio IDE
// 3 parameters
// "filename(line) : error message" 
#define QAFDEBUG_FMT_POS_VS_IDE _T("%s(%d) : %s")

// DebugOutString() for saving to file
// QAFDBG01 [2002-11-11 13:57:49:0976] [process=0x00000608, thread=0x00000644, module=C:\Works\QAF\test\bin\QRunTest.exe] [file=C:\Works\QAF\core\QAFCore2\TestCase.cpp, function=, line=167, expression=Q_FAILED(0x800401F3)] Invalid class string

// 7 parameters - formatting date and time
// "QAFDBG01 [2002-11-11 13:57:49:0976] "          
#define QAFDEBUG_FMT_DATE QAFDEBUG_STD_PREFIX _T("[%04d-%02d-%02d %02d:%02d:%02d:%03d] ")

// 4 parameters - formatting process info
// "... [process=0x00000608, thread=0x00000644, module=C:\Works\QAF\test\bin\QRunTest.exe, GetLastError=21] " 
#define QAFDEBUG_FMT_PROCESS QAFDEBUG_FMT_DATE _T("[process=0x%08X, thread=0x%08X, module=%s, GetLastError=%d] ")

// 3 parameters - formatting position info
// "... [file=C:\Works\QAF\core\QAFCore2\TestCase.cpp, line=167, expression=Q_FAILED(0x800401F3)] " 
#define QAFDEBUG_FMT_POS QAFDEBUG_FMT_PROCESS _T("[file=%s, line=%d, expression=\"%s\"] ")

// 1 parameter - the error message
// "... error message" 
#define QAFDEBUG_FMT_FULL QAFDEBUG_FMT_POS _T("%s")

// The resulting format strings
// 3 parameters
#define QAFDEBUG_IDE_FORMAT QAFDEBUG_FMT_POS_VS_IDE
// 16 parameters
#define QAFDEBUG_STD_FORMAT QAFDEBUG_FMT_FULL

// Fixed error messages for faults in the debug engine
#define QAFDEBUG_ERROR_MUTEX_CREATE QAFDEBUG_ERROR_PREFIX _T("Mutex is not created\r\n")
#define QAFDEBUG_ERROR_MAP_FILE_CREATE QAFDEBUG_ERROR_PREFIX _T("Mapped-memory file is not created\r\n")
#define QAFDEBUG_ERROR_LOG_FILE_PATH QAFDEBUG_ERROR_PREFIX _T("Path to the error log file cannot be retrieved\r\n")
#define QAFDEBUG_ERROR_POINTER_NOT_MAPPED QAFDEBUG_ERROR_PREFIX _T("Memory pointer is not mapped to the Mapped-memory file\r\n")
#define QAFDEBUG_ERROR_NULL QAFDEBUG_ERROR_PREFIX _T("qafOutputDebugString() got an empty error message.\r\n")
#define QAFDEBUG_ERROR_STD_NULL QAFDEBUG_ERROR_PREFIX _T("QAFDebug::std pointer is not initialized.\r\n")
#define QAFDEBUG_ERROR_OPEN_LOG_FILE QAFDEBUG_ERROR_PREFIX _T("\""QAFDEBUG_LOG_FILE_NAME"\"log file cannot be opened.\r\n")
#define QAFDEBUG_ERROR_ALLOCATE_BUFFER QAFDEBUG_ERROR_PREFIX _T("Cannot allocate memory for formatting the error message.\r\n")
#define QAFDEBUG_ERROR_NULL_ARGS QAFDEBUG_ERROR_PREFIX _T("NULL input parameters for formatting the error message.\r\n")
#define QAFDEBUG_ERROR_FORMAT QAFDEBUG_ERROR_PREFIX _T("Cannot format the error message.\r\n")

// Fixed error message for unknown error
#define QAFDEBUG_ERROR_NO_MESSAGE _T("[Could not find any description for the error]\r\n")

////////////////////////////////////////////////////////////////////////////////////
// Service Functions
////////////////////////////////////////////////////////////////////////////////////

///
/// @fn ODS
/// @brief Replacement for OutputDebugString() with checking that it is enabled.
///
inline void ODS( LPCTSTR szMessage )
{
	#ifndef QAF_OUTPUTDEBUGSTRING_DISABLED
		OutputDebugString( szMessage );
	#endif
}

////////////////////////////////////////////////////////////////////////////////////
// Classes
////////////////////////////////////////////////////////////////////////////////////

/// A simple synchronization class, designed to be used as an automatic variable.
/// Once its instance is instanciated, it locks its visibility scope. 
class CSync
{
public:
	
	/// Flag that the critical section object must be constructed or destroyed
	enum CSOP { CS_NOP = 0, CS_CREATE = 1, CS_DESTROY = 2 };
	
	/// Constructor creates and enters the critical section
    CSync( CRITICAL_SECTION * pcs, const CSOP csop = CS_NOP )
	{
		m_pcs = pcs;
		m_csop = csop;
		if( CS_CREATE == m_csop )
			InitializeCriticalSection( m_pcs ); 
		EnterCriticalSection( m_pcs ); 
	}
	
	/// Destructor leaves and destroys the critical section
    ~CSync() 
	{
		LeaveCriticalSection( m_pcs ); 
		if( CS_DESTROY == m_csop )
			DeleteCriticalSection( m_pcs ); 
	}

private:
	
	/// Critical section is used for synchronization
	CRITICAL_SECTION * m_pcs;
	
	/// Flag that the critical section object must be constructed or destroyed
	CSOP m_csop;
	
};

///
/// @class CQAFDebug
/// @brief Class to generate debug log
///
class CQAFDebug
{
private:

	#ifndef QAF_UNITTEST_DISABLED
	/// This is a named mutex that regulates access to the memory-mapped-file.
	HANDLE m_hMutex;

	/// Pointer to the shared memory that contains the shared flags.
	LPVOID m_lpvMem; 

	/// Handle of the shared memory-mapped-file.
	HANDLE m_hMapObject;  
	#endif

	/// Name of the log file, it is initialized by the constructor
	#ifndef QAF_LOGFILE_DISABLED
	TCHAR m_lpszLogFileName[MAX_PATH];
	#endif

	/// Critical section is used for synchronization the class methods
	CRITICAL_SECTION m_cs;

protected:
	
	#ifndef QAF_UNITTEST_DISABLED
	/// Set the shared flag in the memory mapped file
	bool SetSharedFlag( const DWORD dwFlag ) 
	{ 
		if( ! tryLock() )
			return false;
		
		memcpy( m_lpvMem, &dwFlag, sizeof(dwFlag) );
		
		unLock();
		
		return true;
	} 
	
	/// Get the shared flag from the memory mapped file
	void CQAFDebug::GetSharedFlag( const LPDWORD lpdwFlag ) 
	{ 
		if( (NULL != lpdwFlag) && tryLock() )
		{
			memcpy( lpdwFlag, m_lpvMem, sizeof(*lpdwFlag) );
			unLock();
		}
	}
	
	/// Check if the debug log is enabled in the memory mapped file
	bool isEnabled( void )
	{
		DWORD dwTemp = FALSE;
		
		GetSharedFlag( &dwTemp );
		
		return (dwTemp == TRUE);
	}
	
	/// Try to lock the memory mapped file.
	bool tryLock()
	{
		return (WaitForSingleObject( m_hMutex, 50 ) == WAIT_OBJECT_0);
	}
	
	/// Unlock the locked memory mapped file.
	void unLock()
	{
		ReleaseMutex( m_hMutex );
	}
	#endif
	
	/// Generate the file name and directory (check that the file is here).
	/// Get the buffer and the buffer length in TCHAR characters including tailing 0x00(00).
	/// Returns the length of the written string or 0 if the file name cannot be generated.
	/// The file name is constructed from the:
	/// 1. Try get the folder path from the environment variable QAFDEBUG_LOG_ENV_VAR
	/// 2. Try CSIDL_APPDATA (C:\Documents and Settings\username\Application Data) + QAFDEBUG_LOG_SUBFOLDER
	/// 3. Try CSIDL_COMMON_APPDATA (C:\Documents and Settings\All Users\Application Data) + QAFDEBUG_LOG_SUBFOLDER
	/// 4. Return 0
	/// If the folders are missing on the disk, they are created.
	static DWORD qafGetLogFileName( LPTSTR lpszFilenameBuf, const DWORD dwMaxLen );
	
	/// Get the shared instance of the log class
	static CQAFDebug & instance( void )
	{
		// Initialize the static instance (it will be a global variable)
 		static CQAFDebug std_err;
		// Return the instance
		return std_err;
	}
	
public:

	/// Constructor. It does some complex initialization of memory shared files and instance pointers.
	CQAFDebug()
		#ifndef QAF_UNITTEST_DISABLED
		: m_hMutex(NULL), m_hMapObject(NULL), m_lpvMem(NULL)
		#endif
	{
		CSync sync( &m_cs, CSync::CS_CREATE );
		
		#ifndef QAF_LOGFILE_DISABLED
		// Initialize the file name
		if( 0 == qafGetLogFileName( m_lpszLogFileName, MAX_PATH ) )
			m_lpszLogFileName[0] = 0;
		#endif

		#ifndef QAF_UNITTEST_DISABLED
		m_hMutex = CreateMutex( NULL, false, QAFDEBUG_SILENCE_MUTEX );
		if( NULL == m_hMutex )
			ODS( QAFDEBUG_ERROR_MUTEX_CREATE );
		
		m_hMapObject = CreateFileMapping( 
			INVALID_HANDLE_VALUE, // use paging file
			NULL,                 // default security attributes
			PAGE_READWRITE,       // read/write access
			0,                    // size: high 32-bits
			QDEBUG_SHMEMSIZE,     // size: low 32-bits
			QDEBUG_SHMEMFILE );   // name of map object
		if( NULL == m_hMapObject ) 
			ODS( QAFDEBUG_ERROR_MAP_FILE_CREATE ); 
		
		// The first process to attach initializes memory.
		bool fInit = (GetLastError() != ERROR_ALREADY_EXISTS); 
		
		// Get a pointer to the file-mapped shared memory.
		m_lpvMem = MapViewOfFile( 
			m_hMapObject,     // object to map view of
			FILE_MAP_WRITE, // read/write access
			0,              // high offset:  map from
			0,              // low offset:   beginning
			0 );            // default: map entire file
		if( NULL == m_lpvMem ) 
			ODS( QAFDEBUG_ERROR_POINTER_NOT_MAPPED ); 
		
		if( fInit )
			SetSharedFlag( TRUE );
		#endif
	}
	
	/// Destructor. It deinitializes the shared flags.
	~CQAFDebug()
	{
		CSync sync( &m_cs, CSync::CS_DESTROY );
		
		#ifndef QAF_UNITTEST_DISABLED
		unLock();
		#endif

		#ifndef QAF_LOGFILE_DISABLED
		m_lpszLogFileName[0] = 0;
		#endif
		
		#ifndef QAF_UNITTEST_DISABLED
		if( NULL != m_lpvMem )
		{
			UnmapViewOfFile( m_lpvMem );
			m_lpvMem = NULL;
		}
		
		if( NULL != m_hMapObject )
		{
			CloseHandle( m_hMapObject );
			m_hMapObject = NULL;
		}
		
		if( NULL != m_hMutex )
		{
			CloseHandle( m_hMutex );
			m_hMutex = NULL;
		}
		#endif
	}
	
	#ifndef QAF_LOGFILE_DISABLED
	/// try open the log file, testing its size and moving it to the same log file if needed.
	static HANDLE tryOpenLogFile( void )
	{
		CSync sync( &instance().m_cs );
		LPTSTR lpszFilename = instance().m_lpszLogFileName;
		bool bAlreadyTried = false;
		HANDLE h = INVALID_HANDLE_VALUE;
		while( true )
		{
			h = CreateFile( lpszFilename, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_ARCHIVE, NULL );
			if( INVALID_HANDLE_VALUE == h )
			{
				//OutputDebugString( "Exited tryOpenLogFile 1 !\n" );
				return INVALID_HANDLE_VALUE;
			}
			if( bAlreadyTried || (QAFDEBUG_LOG_FILE_MAX_SIZE > GetFileSize( h, NULL )) )
				break;
			if( ! CloseHandle(h) )
			{
				//OutputDebugString( "Exited tryOpenLogFile 2 !\n" );
				return INVALID_HANDLE_VALUE;
			}
			TCHAR lpszCopyFilename[MAX_PATH] = { 0 };
			_tcscpy( lpszCopyFilename, lpszFilename );
			LPTSTR lpszPos = _tcsrchr( lpszCopyFilename, _T('\\') );
			lpszPos[1] = 0;
			_tcscat( lpszPos, QAFDEBUG_LOG_OLD_FILE_NAME );
			MoveFileEx( lpszFilename, lpszCopyFilename, MOVEFILE_REPLACE_EXISTING );
			//OutputDebugString( "File is moved!\n" );
			bAlreadyTried = true;
		}
		return h;
	}
	#endif

	/// try to enable the log
	static bool tryEnable( void )
	{
		#ifdef QAF_UNITTEST_DISABLED
		return true;
		#else
		CSync sync( &instance().m_cs );
		return instance().SetSharedFlag( TRUE );
		#endif
	}
	
	/// try to disable the log
	static bool tryDisable( void )
	{
		#ifdef QAF_UNITTEST_DISABLED
		return true;
		#else
		CSync sync( &instance().m_cs );
		return instance().SetSharedFlag( FALSE );
		#endif
	}
	
};

////////////////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////////////////

// This is taken from MSDN, there is no such constant in VC++ 6.0 header files
//**const DWORD INVALID_FILE_ATTRIBUTES = 0xFFFFFFFF;

/////////////////////////////////////////////////////////////////////////////////////////////////
// DirExists
/////////////////////////////////////////////////////////////////////////////////////////////////
bool DirExists0( LPCTSTR dirName )
{
	DWORD dwAttr = GetFileAttributes( dirName );
	
	if( dwAttr == INVALID_FILE_ATTRIBUTES )
		return false;
	
	return ((dwAttr & FILE_ATTRIBUTE_DIRECTORY) != 0);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// CreateSubFolders
// Must get the folder full path without tailing '\'
/////////////////////////////////////////////////////////////////////////////////////////////////
bool CreateSubFolders0( LPCTSTR lpszFolderName )
{
	// Prepare variables
	DWORD dwLen = _tcslen(lpszFolderName); 
	if( dwLen <= 0 )
		return false;
	// Copy the input string with removing the trailing '\'
	LPTSTR lpszTemp = (LPTSTR) malloc( dwLen + 1 );
	if( NULL == lpszTemp )
		return false;
	_tcscpy( lpszTemp, lpszFolderName );
	if( lpszTemp[dwLen - 1] == _T('\\') )
		dwLen--;
	lpszTemp[dwLen] = _T('\0');
	// If the directory exists, that's it!
	bool bRet = DirExists0( lpszTemp );
	// If the directory does not exists, check its parent
	if( ! bRet )
	{
		LPTSTR lpszPos = _tcsrchr( lpszTemp, _T('\\') );
		if( NULL != lpszPos )
		{
			(*lpszPos) = _T('\0');
			// Call the function recursively
			if( CreateSubFolders0( lpszTemp ) )
			{
				// Now try to create the subfolder and exit
				(*lpszPos) = _T('\\');
				bRet = (TRUE == CreateDirectory( lpszTemp, NULL ));
			}
		} 
	}
	free( lpszTemp );
	return bRet;
}

// Taken from the platform SDK
//**const DWORD CSIDL_COMMON_APPDATA = 0x0023;      // All Users\Application Data
//**const DWORD CSIDL_FLAG_CREATE    = 0x8000;      // new for Win2K, or this in to force creation of folder

/// Count of the libraries with the SHGetFolderPath function
const int LIBRARY_COUNT = 2;

/// The first DLL that is checked for the SHGetFolderPathA(W) function
const LPCTSTR LIBRARY_DLL[LIBRARY_COUNT] = { _T("shell32.dll"), _T("shfolder.dll") };

/// The name of the function (it differs for ASCII and UNICODE builds) 
#ifdef _UNICODE
	const LPCSTR SHGetFolderPathStr = "SHGetFolderPathW";
#else
	const LPCSTR SHGetFolderPathStr = "SHGetFolderPathA";
#endif

/// The folder search function definition (taken from MSDN)
typedef HRESULT (__stdcall* PSHGetFolderPath)  
	( HWND hwndOwner, int nFolder, HANDLE hToken, DWORD dwFlags, LPTSTR pszPath );

/// My wrapper function that tries to load first shell32.dll and then shfolder.dll
HRESULT SHGetSpecialFolderPathCustom( HWND hwndOwner, int nFolder, HANDLE hToken, DWORD dwFlags, LPTSTR pszPath ) 
{
	HRESULT hr = E_FAIL;
	for( int i = 0; i < LIBRARY_COUNT; i++ )
	{
		HMODULE hModule = LoadLibrary( LIBRARY_DLL[i] );
		if( NULL != hModule )
		{
			PSHGetFolderPath pFunc = (PSHGetFolderPath) GetProcAddress( hModule, SHGetFolderPathStr );
			if( NULL != pFunc )
				hr = pFunc( hwndOwner, nFolder, hToken, dwFlags, pszPath );
			FreeLibrary( hModule );
		}
		if( S_OK == hr ) // I cannot test for SUCCEEDED because it may return S_FALSE on error (look in MSDN)
			break;
	}
	return hr;
}

/// This is an enumeration from MSDN for newer versions of SHxxx functions 
#ifndef SHGFP_TYPE_CURRENT
#define SHGFP_TYPE_CURRENT 0
#endif 

/// Generate the file name and directory (check that the file is here).
/// Get the buffer and the buffer length in TCHAR characters including tailing 0x00(00).
/// Returns the length of the written string or 0 if the file name cannot be generated.
/// The file name is constructed from the:
/// 1. Try get the folder path from the environment variable QAFDEBUG_LOG_ENV_VAR
/// 2. Try CSIDL_APPDATA (C:\Documents and Settings\username\Application Data) + QAFDEBUG_LOG_SUBFOLDER
/// 3. Try CSIDL_COMMON_APPDATA (C:\Documents and Settings\All Users\Application Data) + QAFDEBUG_LOG_SUBFOLDER
/// 4. Return 0
/// If the folders are missing on the disk, they are created.
DWORD CQAFDebug::qafGetLogFileName( LPTSTR lpszFilenameBuf, const DWORD dwMaxLen )
{
	DWORD dwRet = QAFDebug::GetLogDir( lpszFilenameBuf, dwMaxLen );
	if( 0 == dwRet )
		return 0;
	// If the directory is created, it is o'key and we can continue with the file name
	_tcscat( lpszFilenameBuf, QAFDEBUG_LOG_FILE_NAME );
	return _tcslen(lpszFilenameBuf);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// QAFDebug namespace
/////////////////////////////////////////////////////////////////////////////////////////////////

bool QAFDebug::tryEnable(void)
{
	return CQAFDebug::tryEnable();
}

bool QAFDebug::tryDisable(void)
{
	return CQAFDebug::tryDisable();
}

void QAFDebug::OutputDebugStringEx( LPCTSTR szFilename, const int iLine, 
	LPCTSTR szExpression, LPCTSTR szErrorMessage )
{
	// test input parameters
	if( NULL == szFilename )
		szFilename = _T("");
	if( NULL == szExpression )
		szExpression = _T("");
	if( NULL == szErrorMessage )
		szErrorMessage = _T("");

	int iRes = 0;
	const DWORD dwBufSize = 1024; // In TCHAR characters!
	LPTSTR lpszBuffer = (LPTSTR) malloc( dwBufSize * sizeof(TCHAR) );
	if( NULL == lpszBuffer ) // if the memory cannot be allocated, the function fails
	{
		ODS( QAFDEBUG_ERROR_ALLOCATE_BUFFER );
		return; 
	}
	
	// Format and output the error message for positioning in the Visual Studio IDE
	#ifndef QAF_OUTPUTDEBUGSTRING_DISABLED
		#ifdef _DEBUG
			iRes = wsprintf( lpszBuffer, QAFDEBUG_IDE_FORMAT, szFilename, iLine, szErrorMessage );
			if( 0 < iRes )
			{
				// Add the last \r\n
				DWORD dwBufferSize = _tcslen( lpszBuffer );
				if( (lpszBuffer[dwBufferSize - 1] != _T('\r')) && (lpszBuffer[dwBufferSize - 1] != _T('\n')) )
					_tcscat( lpszBuffer, _T("\r\n") );
				::ODS( lpszBuffer );
			}
			else
				::ODS( QAFDEBUG_ERROR_FORMAT );
		#endif
	#endif

	// Prepare parameters for the complete error message
	SYSTEMTIME st;
	GetLocalTime( &st );
	DWORD dwProcess = GetCurrentProcessId(), dwThread = GetCurrentThreadId();
	TCHAR lpszModule[MAX_PATH];
	if( 0 == GetModuleFileName( NULL, lpszModule, MAX_PATH ) )
		lpszModule[0] = 0;
	DWORD dwLastError = GetLastError();

	// Format the complete error message 
	LPTSTR lpszFmtStr = QAFDEBUG_STD_FORMAT;
	iRes = wsprintf( lpszBuffer, lpszFmtStr, 
		st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
		dwProcess, dwThread, lpszModule, dwLastError,
		szFilename, iLine, szExpression,
		szErrorMessage );
	DWORD dw = GetLastError();
	if( 0 > iRes )
	{
		ODS( QAFDEBUG_ERROR_FORMAT );
		free( lpszBuffer );
		return;
	}

	// Add the last \r\n
	DWORD dwBufferSize = _tcslen( lpszBuffer );
	if( (lpszBuffer[dwBufferSize - 1] != _T('\r')) && (lpszBuffer[dwBufferSize - 1] != _T('\n')) )
	{
		_tcscat( lpszBuffer, _T("\r\n") );
		dwBufferSize += 2;
	}

	// Output the complete error message to the Visual Studio IDE
	ODS( lpszBuffer );
	
	// If the log file is enabled
	#ifndef QAF_LOGFILE_DISABLED

	// Add another \r\n for an additional empty line (that simplifies reading)
	_tcscat( lpszBuffer, _T("\r\n") );
	dwBufferSize += 2;
	
	// Output the complete error message to file
	// Try to write to the file during 200 msec (5 times with sleeps within)
	HANDLE h = INVALID_HANDLE_VALUE;
	for( int i = 0; i < 5; i++ )
	{
		h = CQAFDebug::tryOpenLogFile();
		#ifdef _DEBUG 
			DWORD dw = GetLastError(); // for my debug purposes
		#endif
		if( h != INVALID_HANDLE_VALUE )
		{
			SetFilePointer( h, 0, NULL, FILE_END ); // I cannot check the return value
			#ifdef _DEBUG 
				dw = GetLastError(); // for my debug purposes
			#endif
			dwBufferSize *= sizeof(TCHAR); // I must recalculate here the buffer size because it was in characters 
			if( ! WriteFile( h, lpszBuffer, dwBufferSize, &dwBufferSize, NULL ) ) // and I need it in bytes
				#ifdef _DEBUG 
				dw = GetLastError(); // for my debug purposes
				#else
				dw = 0; // to disable the compiler warning
				#endif
			
			CloseHandle( h );

			break;
		}
		Sleep( 40 );
	}

	// If the log file is enabled
	#endif

	free( lpszBuffer );
}

HRESULT QAFDebug::qafReportComError( HRESULT hrStatus, LPCTSTR szFile, const int iLine )
{
	if( SUCCEEDED(hrStatus) )
		return hrStatus;
	
    if( FACILITY_WINDOWS == HRESULT_FACILITY(hrStatus) )
        hrStatus = HRESULT_CODE(hrStatus);
	
	LPTSTR szErrMsg = NULL;
	int iFreeErrMsg = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 
		NULL, hrStatus, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), (LPTSTR)&szErrMsg, 0, NULL );
	if( (0 == iFreeErrMsg) || (NULL == szErrMsg) )
		szErrMsg = QAFDEBUG_ERROR_NO_MESSAGE;
	
	TCHAR lpszBuf[QAFDEBUG_FMT_HRESULT_LEN];
	if( 0 >= wsprintf( lpszBuf, QAFDEBUG_FMT_HRESULT, hrStatus ) )
		lpszBuf[0] = 0;
	
	QAFDebug::OutputDebugStringEx( szFile, iLine, lpszBuf, szErrMsg );
	
	if( iFreeErrMsg != 0 )
        LocalFree( szErrMsg );
	
	return hrStatus;
}

void QAFDebug::SetLogDir( CString * str )
{
	QAF_LOG_DIR = *str;
}

DWORD QAFDebug::GetLogDir( LPTSTR lpszDirBuf, const DWORD dwMaxLen )
{
	// Test three different folders, one set from outside, second for regular applications 
	// and third for service applications
	for( int i = 0; i <= 3; i++ )
	{
		if( 0 == i ) 
		{
			// 0. Try get the folder path from the global variable QAF_LOGDIR
			if( QAF_LOG_DIR.GetLength() )
				_tcscpy( lpszDirBuf, QAF_LOG_DIR );
			else
				continue; // try the next option
		}
		else if( 1 == i ) 
		{
			// 1. Try get the folder path from the environment variable QAFDEBUG_LOG_ENV_VAR 
			DWORD dwRet = GetEnvironmentVariable( QAFDEBUG_LOG_ENV_VAR, lpszDirBuf, dwMaxLen );
			if( (dwRet <= 0) || (dwRet >= dwMaxLen) )
				continue; // try the second
		}
		else
		{
			if( 2 == i )
			{
				// 2. Try CSIDL_APPDATA (C:\Documents and Settings\username\Application Data) + QAFDEBUG_LOG_SUBFOLDER
				if( S_OK != SHGetSpecialFolderPathCustom( NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, lpszDirBuf ) )
					continue; // try the third
			}
			else
			{
				// 3. Try CSIDL_COMMON_APPDATA (C:\Documents and Settings\All Users\Application Data) + QAFDEBUG_LOG_SUBFOLDER
				if( S_OK != SHGetSpecialFolderPathCustom( NULL, CSIDL_COMMON_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, lpszDirBuf ) )
					return 0; // FAILED!!!
			}
			// Add the relative path 
			DWORD dwSize = _tcslen(lpszDirBuf); 
			if( dwSize > 0 )
			{
				if( lpszDirBuf[dwSize - 1] != _T('\\') )
					_tcscat( lpszDirBuf, _T("\\") );
				_tcscat( lpszDirBuf, QAFDEBUG_LOG_SUBFOLDER );
			}	
		}
		// Test the resulting string 
		DWORD dwSize = _tcslen(lpszDirBuf);
		if( 0 == dwSize )
			continue; // This trial failed
		// Add final '\'
		if( lpszDirBuf[dwSize - 1] != _T('\\') )
			_tcscat( lpszDirBuf, _T("\\") );
		// Check that the directory exists and create missing subdirectories
		if( ! CreateSubFolders0(lpszDirBuf) )
			continue; // This trial failed
		// Return the result
		return _tcslen(lpszDirBuf);
	} 
	// FAILED !!!
	return 0;
}

///////////////////////////////////////////////
// END OF FILE
///////////////////////////////////////////////
#endif

