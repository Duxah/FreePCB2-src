#pragma once
#include "afxwin.h"


// CDlgAddMaskCutout dialog

class CDlgAddMaskCutout : public CDialog
{
	DECLARE_DYNAMIC(CDlgAddMaskCutout)

public:
	CDlgAddMaskCutout(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgAddMaskCutout();
	void Initialize( int l, int h );
// Dialog Data
	enum { IDD = IDD_ADD_MASK_CUTOUT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CComboBox m_combo_layer;
	CButton m_radio_none;
	CButton m_radio_edge;
	CButton m_radio_full;
	int m_layer;
	int m_hatch;
};
