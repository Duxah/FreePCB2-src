#pragma once
#include "afxcmn.h"
#include "afxwin.h"


// CDlgSaveLib dialog

class CDlgSaveLib : public CDialog
{
	DECLARE_DYNAMIC(CDlgSaveLib)

public:
	CDlgSaveLib(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgSaveLib();
	void Initialize( CArray<CString> * names );
	void GetSelected();
	void ResetSelected();

// Dialog Data
	enum { IDD = IDD_SAVE_LIB };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CListBox m_list_box;
	int m_n_fp;			// number of footprints in project
	CArray<CString> m_names;	// footprint names
	int m_n_sel;		// number of items in list box selected
	int * m_i_sel;		// array of selected item indices
	int * m_sel_data;	// array of selected item data

	afx_msg void OnBnClickedDelete();
	afx_msg void OnBnClickedButtonTop();
	afx_msg void OnBnClickedButtonUp();
	afx_msg void OnBnClickedButtonDown();
	afx_msg void OnBnClickedButtonBottom();
};
