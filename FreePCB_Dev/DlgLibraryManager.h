#pragma once
#include "FootprintLib.h"
#include "afxwin.h"


// CDlgLibraryManager dialog

// globals
static int m_page_sel = 0;
static int m_units_sel = 0;

class CDlgLibraryManager : public CDialog
{
	DECLARE_DYNAMIC(CDlgLibraryManager)

public:
	enum { PG_LETTER, PG_A4 };
	enum { U_NATIVE, U_MM, U_MIL, U_MM_MIL };
	CDlgLibraryManager(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgLibraryManager();
	void Initialize( CFootLibFolderMap * foldermap, CDlgLog * log );

// Dialog Data
	enum { IDD = IDD_LIB_MANAGER };
	CFootLibFolder * m_footlib;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	CFootLibFolderMap * m_foldermap;

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonMakePdf();
	CEdit m_edit_footlib;
	CComboBox m_combo_libfile;
	afx_msg void OnBnClickedButtonMgrBrowse();
	CComboBox m_combo_page_size;
	CComboBox m_combo_units;
	CDlgLog * m_dlg_log;
};
