#pragma once
#include "afxwin.h"
#include "afxcmn.h"


// CDlgImportFootprint dialog

class CDlgImportFootprint : public CDialog
{
	DECLARE_DYNAMIC(CDlgImportFootprint)

public:
	CDlgImportFootprint(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgImportFootprint();
	void InitPartLibTree();
	void InitInstance( CMapStringToPtr * shape_cache_map,
					   CFootLibFolderMap * foldermap,
					   CDlgLog * log );

// Dialog Data
	enum { IDD = IDD_IMPORT_FOOTPRINT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CDlgLog * m_dlg_log;
	CMapStringToPtr * m_footprint_cache_map;
	CFootLibFolderMap * m_foldermap;
	CFootLibFolder * m_footlibfolder;
	CString m_footprint_name;
	CString m_footprint_filename;
	CString m_footprint_folder;
	CShape m_shape;
	BOOL m_in_cache;
	int m_ilib;		// indices into libraries
	int m_ihead;
	int m_ifoot;

	CButton m_button_browse;
	CEdit m_edit_library_folder;
	CTreeCtrl part_tree;
	CStatic m_preview;

private:
	virtual BOOL OnInitDialog();
	afx_msg void OnTvnSelchangedPartLibTree(NMHDR *pNMHDR, LRESULT *pResult);
public:
	afx_msg void OnBnClickedButtonBrowseLibFolder();
	CEdit m_edit_author;
	CEdit m_edit_source;
	CEdit m_edit_desc;
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
};
