// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__A67CF07C_BB64_4AAC_A6B3_A53183E1282F__INCLUDED_)
#define AFX_MAINFRM_H__A67CF07C_BB64_4AAC_A6B3_A53183E1282F__INCLUDED_

#pragma once
#include "MyToolBar.h"
//#include "winuser.h"
#include "FreePcbDoc.h"
#include "FreePcbView.h"


#define TIMER_PERIOD 15		// seconds between timer events

class CMainFrame : public CFrameWnd
{
	
protected: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

// Attributes
public:
 
// Operations
public:
	virtual BOOL DestroyWindow();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	CMyToolBar  m_wndMyToolBar;
	BOOL m_bCursorHidden;
	BOOL m_bHideCursor;		// flag to hide cursor if in m_hide_cursor_rect
	CRect m_hide_cursor_rect;	// rect to hide cursor in screen coords
	CRect m_client_rect;	// client rect in screen coords
	        
public:
	int DrawStatus( int pane, CString * str );
	void SetHideCursor( BOOL bHide, CRect * screen_rect );
	afx_msg LONG OnChangeVisibleGrid( UINT wp, LONG lp );
	afx_msg LONG OnChangePlacementGrid( UINT wp, LONG lp );
	afx_msg LONG OnChangeRoutingGrid( UINT wp, LONG lp );
	afx_msg LONG OnChangeSnapAngle( UINT wp, LONG lp );
	afx_msg LONG OnChangeUnits( UINT wp, LONG lp );
	afx_msg int OnCopyData(CWnd* pWnd, COPYDATASTRUCT* Msg); 
	afx_msg LRESULT Wm__DropFiles( WPARAM Message, LPARAM qwerty );
protected:  // control bar embedded members
	CStatusBar  m_wndStatusBar;
	CView * m_view;
//	CFreePcbDoc * m_doc;
//	CStatic m_ctlStaticVisibleGrid;
//	CComboBox m_ctlComboVisibleGrid;
//	CStatic m_ctlStaticPlacementGrid;
//	CComboBox m_ctlComboPlacementGrid;
//	CStatic m_ctlStaticRoutingGrid;
//	CComboBox m_ctlComboRoutingGrid;

// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnEditUndo();
	UINT_PTR m_timer;
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnMove(int x, int y);
	afx_msg void OnSize(UINT nType, int cx, int cy);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__A67CF07C_BB64_4AAC_A6B3_A53183E1282F__INCLUDED_)
