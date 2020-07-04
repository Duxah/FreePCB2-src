#pragma once
#include "afxwin.h"


// CDlgVia dialog

class CDlgVia : public CDialog
{
	DECLARE_DYNAMIC(CDlgVia)

public:
	CDlgVia(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgVia();
	void Initialize( int def_vw, int def_vh, CArray<int> * m_v_w, CArray<int> * m_v_h_w, int via_w, int via_hole_w, int units );

// Dialog Data
	enum { IDD = IDD_VIA };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CComboBox m_edit_via_w;
	CComboBox m_edit_hole_w;
	int m_def_w;
	int m_def_hole_w;
	int m_via_w;
	int m_via_hole_w;
	int m_units;
	CArray<int> * m_v;
	CArray<int> * m_h;
	afx_msg void OnChange();
};
