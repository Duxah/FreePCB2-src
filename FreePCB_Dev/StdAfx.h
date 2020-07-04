// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
// 
#define WINVER							0x0502
#define _WIN32_IE						0x0502 
//  #define OPENFILENAME_SIZE_VERSION_400	0x50000000
//#define _AFX_MRU_COUNT 7
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1

#if !defined(AFX_STDAFX_H__C29FB9FF_0492_4F8B_9C69_871E6A2CDF93__INCLUDED_)
#define AFX_STDAFX_H__C29FB9FF_0492_4F8B_9C69_871E6A2CDF93__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
 
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
 
#include <afxwin.h>         // MFC core and standard components
#define new DEBUG_NEW
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT
   
#include <afxdhtml.h>
//#include <afxdlgs.h>
#include <direct.h>
#include <shlwapi.h>
#include <sys/types.h>
#include <sys/stat.h>
 
#include "MainFrm.h"
#include "FreePcb.h"
#include "PolyLine.h"
#include "NetList.h" 
#include "DisplayList.h"
#include "PartList.h"
#include "FreePcbView.h"
#include "FreePcbDoc.h"
#include "FootprintView.h"
#include "file_io.h"
#include "utility.h"
#include "ids.h"
#include "layers.h"
#include "Shape.h"
#include "PcbFont.h"
#include "smfontutil.h"
#include "TextList.h"
#include "resource.h"
#include "UndoList.h" 
#include "flags.h" 
#include "DlgLog.h" 

#define ASSERT(f) assert(f)	//changed ASSERT() to work in release versions if NDEBUG undefined

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__C29FB9FF_0492_4F8B_9C69_871E6A2CDF93__INCLUDED_)
