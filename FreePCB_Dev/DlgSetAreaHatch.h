#pragma once
#include "afxwin.h"


// CDlgSetAreaHatch dialog

class CDlgSetAreaHatch : public CDialog
{
	DECLARE_DYNAMIC(CDlgSetAreaHatch)

public:
	CDlgSetAreaHatch(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgSetAreaHatch();
	void Init( int hatch );
	int GetHatch(){ return m_hatch; };

// Dialog Data
	enum { IDD = IDD_SET_AREA_HATCH };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	int m_hatch;
	CButton m_radio_none;
	CButton m_radio_edge;
	CButton m_radio_full;
};
