// FreePcb.h : main header file for the FREEPCB application
//


#if !defined(AFX_FREEPCB_H__2D973E6F_7601_4C61_8528_D36001F51E5D__INCLUDED_)
#define AFX_FREEPCB_H__2D973E6F_7601_4C61_8528_D36001F51E5D__INCLUDED_

#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "stdafx.h"
#include "resource.h"       // main symbols
#include "FootprintView.h"

// units for length
enum
{
	NM,		// nanometers
	MM,		// millimeters
	MIL,	// mils (1/1000 inch)
	MM_MIL,	// both mm and mils (for text output)
	NATIVE	// native units (for text output )
};
// conversion factors
#define _2540		2540
// define world units for CDisplayList
#define m_pcbu_per_wu 1270

#define NM_PER_MIL	25400
#define NM_PER_MM	1000000
#define DEFAULT		999999999
#define BY_ZERO		0.001
#define N_SIDES_APPROX_ARC 12

// there are four coordinate systems:
//	WU = window coords
//	screen coords (pixels)
//	PCBU = PCB coords (nanometers)
//	DU = display coords (mils)
//
// conversion factors
#define PCBU_PER_MIL	NM_PER_MIL
#define PCBU_PER_MM		NM_PER_MM

#define PCB_BOUND	32000*PCBU_PER_MIL	// boundary

// custom messages
enum {
	WM_USER_VISIBLE_GRID = WM_USER +1,
	WM_USER_PLACEMENT_GRID,
	WM_USER_ROUTING_GRID,
	WM_USER_SNAP_ANGLE,
	WM_USER_UNITS
};
enum {
	WM_BY_VALUE,
	WM_BY_INDEX,
	WM_BY_STRING
};

/////////////////////////////////////////////////////////////////////////////
// CFreePcbApp:
// See FreePcb.cpp for the implementation of this class
//

class CFreePcbApp : public CWinApp
{
public:
	enum {
		PCB = 1,
		FOOTPRINT
	};
	CFreePcbApp();
	CFreePcbDoc * m_Doc;
	CFreePcbView * m_View;
	CFootprintView * m_View_fp;
	CSingleDocTemplate * m_pDocTemplate;
	BOOL SwitchToView( CRuntimeClass * pNewViewClass );
	CString GetMRUFile();
	void AddMRUFile( CString * str );
	int DoMessageBox(LPCTSTR lpszPrompt, UINT nType, UINT nIDPrompt);

	CMenu m_main;		// main menus
	CMenu m_main_drag;
	CMenu m_foot;
	CMenu m_foot_drag;

	int m_view_mode; // FOOTPRINT or PCB

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFreePcbApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(CFreePcbApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	BOOL OnOpenRecentFile(UINT nID);
	afx_msg void OnViewFootprint();
	afx_msg void OnViewPcbEditor();
	afx_msg void OnHelpGotoWebsite();
	afx_msg void OnFileMruFile1();
	afx_msg void OnFileMruFile2();
	afx_msg void OnFileMruFile3();
	afx_msg void OnFileMruFile4();
	afx_msg void OnFileMruFile5();
	afx_msg void OnFileMruFile6();
	afx_msg void OnFileMruFile7();
	afx_msg void OnHelpKeyboardshortcuts();
	afx_msg void OnToolsOpenOnlineAutorouter();
	afx_msg void OnHelpFreeRoutingWebsite();
	afx_msg void OnHelpFAQ();
	afx_msg void OnHelpUserGuidePdf();
	afx_msg void OnHelpUserGuideSupplementPdf();
	afx_msg void OnHelpFpcRoute();  
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FREEPCB_H__2D973E6F_7601_4C61_8528_D36001F51E5D__INCLUDED_)
