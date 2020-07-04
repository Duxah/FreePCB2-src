#pragma once
#include "stdafx.h"
#include "afxwin.h"


// CDlgSaveFootprint dialog

class CDlgSaveFootprint : public CDialog
{
	DECLARE_DYNAMIC(CDlgSaveFootprint)

public:
	CDlgSaveFootprint(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgSaveFootprint();
	void Initialize( CString * name, CShape * footprint, int units,							 
					LPCTSTR default_file_name,
					CMapStringToPtr * shape_cache_map,
					CFootLibFolderMap * footlibfoldermap,
					CDlgLog * log );
	void InitFileList();
// Dialog Data
	enum { IDD = IDD_SAVE_FOOTPRINT };
	int m_lib_user;		// index to "user_created.flb"
	int m_lib_last;		// index to last file saved

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CMapStringToPtr * m_footprint_cache_map;
	CFootLibFolderMap * m_foldermap;
	CFootLibFolder * m_folder;
	CStatic m_preview;
	CShape * m_footprint;
	int m_units;
	CString m_default_filename;
	CString m_folder_name;
	CString m_name;
	CString m_author;
	CString m_package;
	CString m_desc;
	CEdit m_edit_name;
	CComboBox m_combo_lib;
	afx_msg void OnCbnSelchangeComboLibs();
	afx_msg void OnBnClickedOk();
	CEdit m_edit_source;
	CEdit m_edit_author;
	CEdit m_edit_desc;
	CEdit m_edit_folder;
	CDlgLog * m_dlg_log;
	afx_msg void OnBnClickedButtonBrowse();
	CStatic m_static_units;
};
