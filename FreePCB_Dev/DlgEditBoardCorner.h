#pragma once
#include "afxwin.h"


// DlgEditBoardCorner dialog

class DlgEditBoardCorner : public CDialog
{
	DECLARE_DYNAMIC(DlgEditBoardCorner)

public:
	DlgEditBoardCorner(CWnd* pParent = NULL);   // standard constructor
	virtual ~DlgEditBoardCorner();
	void Init( CString * str, int units, int x, int y );
	int GetX(){ return m_x; };
	int GetY(){ return m_y; };

// Dialog Data
	enum { IDD = IDD_EDIT_BOARD_CORNER };
	
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CString * m_title_str;
	CComboBox m_combo_units;
	CEdit m_edit_x;
	CEdit m_edit_y;
	afx_msg void OnCbnSelchangeComboCornerUnits();
private:
	int m_x;
	int m_y;
	int m_units;
	void GetFields();
	void SetFields();
};
