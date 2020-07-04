///
/// @file	QAFDebug.h "../Include/QAFDebug.h"
/// @brief	Macros for reporting critical errors.
///         
///			This file defines a set of macros that replaces some of 
///			standard ATL and C++ macros. It is strongly recommended to use
///			these macros in all functions. <P>
///				
///			The general guideline of using these macros: if you write code 
///			that should never fail under normal conditions, use these macros to report
///			the cases when something extra-ordinary happens. <P>
///				
///			For the moment these macros are defined in both RELEASE and DEBUG builds.
///			They use OutputDebugString() and log file to report about errors. <P>
///				
///			You must add QAFDebug.cpp to your project in order to use this header file.
///

#ifndef _QAFDEBUG_H_
#define _QAFDEBUG_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


//** AMW
#define QAF_LOGDIR(str) (QAFDebug::SetLogDir( str ))

///
/// @def QAF_DISABLED
/// @brief Disable all error log staff, you may define it for release builds if you want.
///
/// I ENABLE reporting even in RELEASE builds, it makes the program a bit larger (not affecting performance)
/// but instead I get the full diagnostics for all problems. To disable the error reporting in release builds
/// either uncomment these lines or define the QAF_DISABLED conditional define for the release builds.
/// 
#ifndef QAF_DISABLED
	#define QAF_DISABLED
	// If you want to disable the error log in release builds, uncomment this test of _DEBUG
	//#ifndef _DEBUG
		#undef QAF_DISABLED
	//#endif
#endif

///
/// @def QAF_LOGFILE_DISABLED
/// @brief Disable writing to the error log file, you may define it for release builds if you want.
///
/// I ENABLE writting to the error log file even in RELEASE builds , it makes the program a bit larger 
/// (not affecting performance) but instead I get the full diagnostics for all problems. 
/// To disable writting to the error log file in release builds either uncomment these lines or define the 
/// QAF_LOGFILE_DISABLED conditional define for the release builds.
/// 
#ifndef QAF_LOGFILE_DISABLED
	#define QAF_LOGFILE_DISABLED
	// If you want to disable the error log in release builds, uncomment this test of _DEBUG
	//#ifndef _DEBUG
		#undef QAF_LOGFILE_DISABLED
	//#endif
#endif

///
/// @def QAF_OUTPUTDEBUGSTRING_DISABLED
/// @brief Disable calling OutputDebugString(), I recommend you to define it for release builds.
///
/// I DISABLE reporting using OutputDebugString() in RELEASE builds (since usually nobody will trace it).
///
#ifndef QAF_OUTPUTDEBUGSTRING_DISABLED
	#define QAF_OUTPUTDEBUGSTRING_DISABLED
	// This will enable the reporting back for DEBUG build
	#ifdef _DEBUG
		#undef QAF_OUTPUTDEBUGSTRING_DISABLED
	#endif
#endif

/// @def QAF_UNITTEST_DISABLED
/// @brief Disable unit tests related staff, I recommend you to define it for release builds 
/// and disable unit tests too.
///
/// I DISABLE part of the functionality related to the unit tests in RELEASE build to optimize the code.
/// Defining this directive removes all the synchronization and memory mapped file staff from the code.
///
#ifndef _DEBUG
	#define QAF_UNITTEST_DISABLED
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Includes
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <winerror.h>
#include <tchar.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Macros
///////////////////////////////////////////////////////////////////////////////////////////////////////////

///
/// @def	Q_ASSERT
/// @brief	This macro reports about critical error if @c bCondition evaluates to @c false.
/// @param	bCondition	Any expression that evaluates to @c bool or @c int
///	@return	bool (same as bCondition)
/// @author	Andrew Schetinin
/// @date	September 26, 2002
///
///			This macro output a error report string to the error log (file and/or debug console). 
///			The string looks like that (word-wrapped here): 
/// @code 
///	QAFDBG01 [2003-01-23 13:57:16:213] [process=0x00000670, thread=0x00000374, 
///	module=C:\java\sem\test.exe, GetLastError=0] [file=C:\Projects\TAP\QTAP.cpp, line=65, 
///	expression="false"] Assertion raised
/// @endcode 
///			It is recommended to use this macro in all places where any error 
///			means program crash or non-predictable behavior of dependent code.
///			It will help to detect where first the error appeared.
///         Recommended for IFs without ELSE or testing return values of functions.
/// @code 
///	if( Q_ASSERT( TRUE == bVarShouldAlwaysBeTRUE ) ) // here it will write to the error log
///		do_something();
/// @endcode 
///

#ifndef QAF_DISABLED
	#define Q_ASSERT( bCondition ) ((bCondition) ? true : (QAFDebug::OutputDebugStringEx( \
		_T(__FILE__), __LINE__, _T(#bCondition), QAFDEBUG_ERROR_ASSERTION ), false) )
#else 
	#define Q_ASSERT( bCondition ) (bCondition)
#endif

///
/// @def	Q_INVALID
/// @brief	This macro reports about critical error if @c bCondition evaluates to @c true.
/// @param	bCondition	Any expression that evaluates to @c bool or @c int
///	@return	bool (same as bCondition)
/// @author	Andrew Schetinin
/// @date	December 11, 2002
///
///			This macro output a error report string to the error log (file and/or debug console). 
///			The string looks like that (word-wrapped here): 
/// @code 
///	QAFDBG01 [2003-01-23 13:57:16:213] [process=0x00000670, thread=0x00000374, 
///	module=C:\java\sem\test.exe, GetLastError=0] [file=C:\Projects\TAP\QTAP.cpp, line=65, 
///	expression="false"] Assumption failed
/// @endcode 
///			It is recommended to use this macro in all places where any error 
///			means program crash or non-predictable behavior of dependent code.
///			It will help to detect where first the error appeared.
///         Recommended for testing input parameters or error conditions.
/// @code 
///	if( Q_INVALID( NULL == lpszStringParam ) ) // here it will write to the error log
///		return E_INVALIDARG;
/// @endcode 
///

#ifndef QAF_DISABLED
	#define Q_INVALID( bCondition ) ((bCondition) ? (QAFDebug::OutputDebugStringEx( \
		_T(__FILE__), __LINE__, _T(#bCondition), QAFDEBUG_ERROR_INVALID_ASSUMPTION ), true) : false )
#else 
	#define Q_INVALID( bCondition ) (bCondition)
#endif

///
/// @def	Q_SUCCEEDED
/// @brief	Add reporting critical errors to the standard SUCCEEDED macro. 
/// @param	Status	HRESULT result code
///	@return	bool. @c true means "no error"
/// @author	Andrew Schetinin
/// @date	September 26, 2002
///
///			Generic test for success on any status value (non-negative numbers
///			indicate success). <P>
///
///			This macro output a error report string to the error log (file and/or debug console). 
///			The string looks like that (word-wrapped here): 
/// @code 
///	QAFDBG01 [2003-01-23 13:57:16:213] [process=0x00000670, thread=0x00000374, 
///	module=C:\java\sem\test.exe, GetLastError=0] [file=C:\Projects\TAP\QTAP.cpp, line=65, 
///	expression="Q_FAILED(0x800401F3)"] The component's CLSID is missing or corrupt.
/// @endcode 
///			It is recommended to use this macro in all places where any error 
///			means program crash or non-predictable behavior of dependent code.
///			It will help to detect where first the error appeared.
///         Recommended for testing return HRESULTs that usually always should return S_OK or S_FALSE.
///			Another use for this macro is to report about a critical error in your function 
///			that is returned to the calling function.  
/// @code 
///	if( Q_SUCCEEDED(hr) ) // here it will write to the error log
///		do_something();
/// @endcode 
///

#ifndef QAF_DISABLED
	#define Q_SUCCEEDED(hResultExpr) SUCCEEDED(Q_ERROR(hResultExpr))
#else 
	#define Q_SUCCEEDED(hResultExpr) SUCCEEDED(hResultExpr)
#endif

///
/// @def	Q_FAILED
/// @brief	Add reporting critical errors to the standard FAILED macro. 
/// @param	Status	HRESULT result code
///	@return	bool. @c true means "there is an error!"
/// @author	Andrew Schetinin
/// @date	September 26, 2002
///
///			Generic test for failure on any status value. <P>
///
///			This macro output a error report string to the error log (file and/or debug console). 
///			The string looks like that (word-wrapped here): 
/// @code 
///	QAFDBG01 [2003-01-23 13:57:16:213] [process=0x00000670, thread=0x00000374, 
///	module=C:\java\sem\test.exe, GetLastError=0] [file=C:\Projects\TAP\QTAP.cpp, line=65, 
///	expression="Q_FAILED(0x800401F3)"] The component's CLSID is missing or corrupt.
/// @endcode 
///			It is recommended to use this macro in all places where any error 
///			means program crash or non-predictable behavior of dependent code.
///			It will help to detect where first the error appeared.
///         Recommended for testing return HRESULTs that usually always should return S_OK or S_FALSE.
///			Another use for this macro is to report about the error before returning from your function.
/// @code 
///	if( Q_FAILED(hr) ) // here it will write to the error log
///		return hr;
/// @endcode 
///

#ifndef QAF_DISABLED
	#define Q_FAILED(hResultExpr) FAILED(Q_ERROR(hResultExpr))
#else 
	#define Q_FAILED(hResultExpr) FAILED(hResultExpr)
#endif

///
/// @def	Q_ERROR
/// @brief	Reports critical errors returned from your function before returning. 
/// @param	Status	HRESULT result code
///	@return	HRESULT (same as Status)
/// @author	Andrew Schetinin
/// @date	September 26, 2002
///
///			Generic test for failure on any status value. <P>
///
///			This macro output a error report string to the error log (file and/or debug console). 
///			The string looks like that (word-wrapped here): 
/// @code 
///	QAFDBG01 [2003-01-23 13:57:16:213] [process=0x00000670, thread=0x00000374, 
///	module=C:\java\sem\test.exe, GetLastError=0] [file=C:\Projects\TAP\QTAP.cpp, line=65, 
///	expression="Q_FAILED(0x800401F3)"] The component's CLSID is missing or corrupt.
/// @endcode 
///			It is recommended to use this macro in all places where any error 
///			means program crash or non-predictable behavior of dependent code.
///			It will help to detect where first the error appeared.
///         Recommended for testing return HRESULTs that usually always should return S_OK or S_FALSE.
///			Another use for this macro is to test return values of generic functions that 
///         usually never fail.
/// @code 
///	// This will report about the error in the exact place where it first happened.
///	return Q_ERROR( CoCreateInstance( clsid, NULL, dwCtx, IID_IDispatch, (void**)(&this->p) ) );
/// @endcode 
///

#ifndef QAF_DISABLED
	#define Q_ERROR(hResultExpr) (QAFDebug::ReportComError( hResultExpr, _T(__FILE__), __LINE__ ))
#else 
	#define Q_ERROR(hResultExpr) (hResultExpr)
#endif

///
/// @def	Q_LOG
/// @brief	Reports critical errors with a string message. 
/// @param	lpszMessage	LPCTSTR message string
///	@return	void
/// @author	Andrew Schetinin
/// @date	January 28, 2003
///
///			This is a macro for reporting about critical errors in a user-understandable format. 
///			Generally it is preferable to Q_ASSERT(false). <P>
///
///			This macro output a error report string to the error log (file and/or debug console). 
///			The string looks like that (word-wrapped here): 
/// @code 
///	QAFDBG01 [2003-01-23 13:57:16:213] [process=0x00000670, thread=0x00000374, 
///	module=C:\java\sem\test.exe, GetLastError=0] [file=C:\Projects\TAP\QTAP.cpp, line=65, 
///	expression="My Error Message"] Assertion raised
/// @endcode 
///			It is recommended to use this macro in all places where any error 
///			means program crash or non-predictable behavior of dependent code.
///			It will help to detect where first the error appeared.
///         Recommended for IFs without ELSE or testing return values of functions.
/// @code 
///	catch( ... )
///	{
///		Q_LOG( _T("Unknown exception catched") ); // here it will report about all exceptions
///	}
/// @endcode 
///

#ifndef QAF_DISABLED
	#define Q_LOG(lpszMessage) (QAFDebug::OutputDebugStringEx( _T(__FILE__), __LINE__, QAFDEBUG_ERROR_LOG, lpszMessage ))
#else 
	#define Q_LOG(lpszMessage) (lpszMessage)
#endif

///
/// @def	Q_EXCEPTION
/// @brief	Reports critical exceptions. 
/// @param	e	CException object instance (MFC-style)
///	@return	void
/// @author	Andrew Schetinin
/// @date	November 18, 2002
///
///			This is a special macro to report about an MFC-style exception. <P>
///
///			This macro output a error report string to the error log (file and/or debug console). 
///			The string looks like that (word-wrapped here): 
/// @code 
///	QAFDBG01 [2003-01-23 13:57:16:213] [process=0x00000670, thread=0x00000374, 
///	module=C:\java\sem\test.exe, GetLastError=0] [file=C:\Projects\TAP\QTAP.cpp, line=65, 
///	expression="e"] Cannot open file
/// @endcode 
///			It is recommended to use this macro in all places where any error 
///			means program crash or non-predictable behavior of dependent code.
///			It will help to detect where first the error appeared.
///         Recommended for reporting about exceptions in CATCH clauses.
///			You may define your own exception classes (without MFC support), just define 
///			the GetErrorMessage() method and this macro will work.
/// @code 
///	catch( CMyException & e ) 
///	{
///		Q_EXCEPTION(e); // here it will write to the error log with the error message
///	}
///	catch( ... )
///	{
///		Q_LOG( _T("Unknown exception catched") ); // here it will report about any other exception
///	}
/// @endcode 
///

#ifndef QAF_DISABLED
	#define Q_EXCEPTION(e) \
	{ \
		TCHAR szBuf[250]; \
		if( (NULL == e) || (! e->GetErrorMessage( szBuf, 250, NULL )) ) \
			strcpy( szBuf, QAFDEBUG_ERROR_NO_MESSAGE ); \
		QAFDebug::OutputDebugStringEx( _T(__FILE__), __LINE__, _T(#e), szBuf ); \
	} 
#else 
	#define Q_EXCEPTION(e)
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Defines related to the unit tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////

///
/// @def	Q_ENABLE_DEBUG_LOG
/// @brief	Enable the error log in case if it was disabled because of the previous unit test failure. 
///	@return	void
/// @author	Andrew Schetinin
/// @date	November 18, 2002
///
///			This is a special macro for unit test functions. This macro ensures that the 
///			reporting is switched on. It is recommended to put it at the beginning of a single 
///			tests case (at the beginning of the function).
/// @code 
///	void CUnitTest::testCase01( void ) 
///	{
///		Q_ENABLE_DEBUG_LOG; // switch the error log on
///		CPPUNIT_ASSERT( Q_SUCCEEDED( QAFGetRegKey( HKCU_C_END, &str ) ) ); // write to the log if it fails
///		...
///	}
/// @endcode 
///

#define Q_ENABLE_DEBUG_LOG ;
#ifndef QAF_DISABLED
	#ifndef QAF_UNITTEST_DISABLED
		#undef Q_ENABLE_DEBUG_LOG
		#define Q_ENABLE_DEBUG_LOG QAFDebug::tryEnable();
	#endif 
#endif

///
/// @def	Q_SILENT
/// @brief	Temporary disable the error log and evaluate the expression. 
/// @param	expr	Any expression
///	@return	void
/// @author	Andrew Schetinin
/// @date	November 18, 2002
///
///			This is a special macro for unit test functions. It is useful for testing the wrong cases 
///			(for example, passing wrong parameters and checking that the function fails). 
///			For wrong test cases we do not want to report about errors because we want them to happen.
/// @code 
///	void CUnitTest::testCase01( void ) 
///	{
///		Q_ENABLE_DEBUG_LOG; // switch the error log on
///		CPPUNIT_ASSERT( Q_SUCCEEDED( QAFGetRegKey( HKCU_C_END, &str ) ) ); // write to the log if it fails
///		Q_SILENT( CPPUNIT_ASSERT( Q_FAILED( QAFGetRegKey( NULL, NULL ) ) ) ); // do not write to log 
///		...
///	}
/// @endcode 
///

#define Q_SILENT(expr) expr;
#ifndef QAF_DISABLED
	#ifndef QAF_UNITTEST_DISABLED
		#undef Q_SILENT
		#define Q_SILENT(expr) \
		{ \
			bool bLogDisabled = QAFDebug::tryDisable(); \
			expr; \
			if( bLogDisabled ) \
				QAFDebug::tryEnable(); \
		}
	#endif 
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Conditional defines
///////////////////////////////////////////////////////////////////////////////////////////////////////////

// Only if reporting is not disabled
#ifndef QAF_DISABLED
	
/// The name of the mutex for synchronizing the unit test support staff.
const LPCTSTR QAFDEBUG_SILENCE_MUTEX = _T("QAFDebugMutex001A");

/// Subfolder in the application data folder
const LPCTSTR QAFDEBUG_LOG_SUBFOLDER = _T("FreePCB\\Log\\");

/// Name of the environment variable that may set the output debug log folder.
const LPCTSTR QAFDEBUG_LOG_ENV_VAR = _T("QAFERRORLOGPATH");

/// @brief Maximum log file size
/// 
/// Maximum log file size (there are two log files - one current and second previous).
/// When the current file exceeds this limit, it is renamed to the second name. 
/// The size is in TCHARs (it is translated to bytes). Generally 1 record takes about 
/// 300 TCHARs, so I reserve space for about 800-1500 records with 250,000 TCHARs limit.
/// This should be enough and it guaranties that the both log files together will not be 
/// larger than 1 Mb for UNICODE and 500 Kb for REGULAR build.
const DWORD QAFDEBUG_LOG_FILE_MAX_SIZE = (250 * 1024 * sizeof(TCHAR));

/// The current error log file name
const LPCTSTR QAFDEBUG_LOG_FILE_NAME = _T("error.log");

/// The previous error log file name
const LPCTSTR QAFDEBUG_LOG_OLD_FILE_NAME = _T("error.old.log");

/// The name of the memory-mapped-file that stores the shared flags
const LPCTSTR QDEBUG_SHMEMFILE = _T("QAFDbgMemFile01");

/// This is a prefix for the log file and debug out, for filtering our messages
#define QAFDEBUG_STD_PREFIX _T("QAFD_FreePCB ")

/// Errors in the reporting engine
#define QAFDEBUG_ERROR_PREFIX QAFDEBUG_STD_PREFIX _T("Debug System Error --> ")

/// Fixed error message for assertion raised
#define QAFDEBUG_ERROR_ASSERTION _T("Assertion raised\r\n")

/// Fixed error message for invalid assumption raised
#define QAFDEBUG_ERROR_INVALID_ASSUMPTION _T("Invalid assumption is raised\r\n")

/// Fixed expression for the custom error message
#define QAFDEBUG_ERROR_LOG _T("Error Message")

#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// class CQAFDebug - this is a service class - do not use it directly!
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef QAF_DISABLED

///
/// @namespace QAFDebug
/// @brief Namespace QAFDebug hides the debug functions from global scope.
///
namespace QAFDebug
{
	///
	/// @brief Try to enable the error log 
	/// @return bool true if the log is enabled successfully
	/// @author	Andrew Schetinin
	/// @date	November 20, 2002
	///
	/// Try to enable the error log. This function is used only in unit tests.  
	///	Do not use it in your code!
	///
	bool tryEnable( void );
	
	///
	/// @brief Try to disable the error log 
	/// @return bool true if the log is disabled successfully
	/// @author	Andrew Schetinin
	/// @date	November 20, 2002
	///
	/// Try to disable the error log. This function is used only in unit tests.  
	///	Do not use it in your code!
	///
	bool tryDisable( void );
	
	///
	/// @brief Reports about critical errors  
	/// @param	szFilename	LPCTSTR - the file name returned by _FILE_
	/// @param	iLine		const int - the line number returned by _LINE_
	/// @param	szExpression	LPCTSTR - the expression where the error was detected
	/// @param	szErrorMessage	LPCTSTR - the error message generated
	/// @author	Andrew Schetinin
	/// @date	November 20, 2002
	///
	/// Reports critical errors.  
	///	This function is used by debug macros. Do not use it in your code!
	///
	void OutputDebugStringEx( LPCTSTR szFilename, const int iLine, LPCTSTR szExpression, LPCTSTR szErrorMessage );
	
	///
	/// @brief Reports about critical errors if HRESULT is failed  
	/// @param	hrStatus	HRESULT - tested on failure
	/// @param	szFile		LPCTSTR - the file name returned by _FILE_
	/// @param	iLine		const int - the line number returned by _LINE_
	///	@return	HRESULT		the same hrStatus that it received
	/// @author	Andrew Schetinin
	/// @date	November 20, 2002
	///
	/// Reports critical errors if HRESULT is failed.  
	///	This function is used by debug macros. Do not use it in your code!
	///
	HRESULT qafReportComError( const HRESULT hrStatus, LPCTSTR szFile, const int iLine );
	
	///
	/// @brief Reports about critical errors if HRESULT is failed  
	/// @param	hrStatus	HRESULT - tested on failure
	/// @param	szFile		LPCTSTR - the file name returned by _FILE_
	/// @param	iLine		const int - the line number returned by _LINE_
	///	@return	HRESULT		the same hrStatus that it received
	/// @author	Andrew Schetinin
	/// @date	November 20, 2002
	///
	/// Reports critical errors if HRESULT is failed.  
	///	This function is used by debug macros. Do not use it in your code!
	///
	inline HRESULT ReportComError( const HRESULT hrStatus, LPCTSTR szFile, const int iLine )
	{
		if( FAILED(hrStatus) )
			qafReportComError( hrStatus, szFile, iLine );
		return hrStatus;
	}

	///
	/// @brief Return an accessible directory name for all log files.  
	/// @param	lpszDirBuf	LPTSTR buffer for the directory name 
	/// @param	dwMaxLen	DWORD size of the buffer in characters (including the trailing zero)
	///	@return	DWORD		length of the returned string or 0 in case of error
	/// @author	Andrew Schetinin
	/// @date	February 6, 2003
	///
	/// Get the buffer and the buffer length in TCHAR characters including tailing 0x00(00).
	/// Returns the length of the written string or 0 if the directory name cannot be generated.
	/// The directory name is constructed from the: <p>
	/// 0. Folder path set by SetLogDir()
	/// 1. Try get the folder path from the environment variable QAFDEBUG_LOG_ENV_VAR <p>
	/// 2. Try CSIDL_APPDATA (C:\Documents and Settings\username\Application Data) + QAFDEBUG_LOG_SUBFOLDER <p>
	/// 3. Try CSIDL_COMMON_APPDATA (C:\Documents and Settings\All Users\Application Data) 
	///    + QAFDEBUG_LOG_SUBFOLDER <p>
	/// 4. Return 0 <p>
	/// If the folders are missing on the disk, they are created.
	///
	DWORD GetLogDir( LPTSTR lpszDirBuf, const DWORD dwMaxLen );
	
	//** AMW set logfile path directly
	void SetLogDir( CString * str );
}

#endif


///////////////////////////////////////////////
// END OF FILE
///////////////////////////////////////////////
#endif