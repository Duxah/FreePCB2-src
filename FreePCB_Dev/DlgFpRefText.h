#pragma once
#include "afxwin.h"


// CDlgFpRefText dialog

class CDlgFpRefText : public CDialog
{
	DECLARE_DYNAMIC(CDlgFpRefText)

public:
	CDlgFpRefText(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgFpRefText();
	void Initialize( int height, int width, int units );
	int GetHeight(){ return m_height; };
	int GetWidth(){ return m_width; };

// Dialog Data
	enum { IDD = IDD_FP_REF_TEXT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
private:
	void SetFields();
	void GetFields();
	int m_width;
	int m_height;
	int m_def_width;
	int m_units;
	CComboBox m_combo_units;
	CEdit m_edit_height;
	CButton m_radio_set;
	CEdit m_edit_width;
	CButton m_radio_def;
	CEdit m_edit_def_width;
	afx_msg void OnBnClickedRadioSet();
	afx_msg void OnBnClickedRadioDef();
	afx_msg void OnCbnSelchangeComboRefTextUnits();
	afx_msg void OnEnChangeEditCharHeight();
};
