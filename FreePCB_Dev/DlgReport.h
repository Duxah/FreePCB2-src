#pragma once
#include "afxwin.h"
#include "FreePcbDoc.h"


// CDlgReport dialog

class CDlgReport : public CDialog
{
	DECLARE_DYNAMIC(CDlgReport)
public:
	enum{ NO_PCB_STATS = 0x1,
		  NO_DRILL_LIST = 0x2,
		  NO_PARTS_LIST = 0x4,
		  NO_CAM_PARAMS = 0x8,
		  NO_DRC_PARAMS = 0x10,
		  DRC_LIST = 0x20,
		  CONNECTIVITY_LIST = 0x40,
		  USE_MM = 0x1000,
		  CW = 0x2000,
		  TOP = 0x4000
	};
public:
	CDlgReport(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgReport();
	void Initialize( CFreePcbDoc * doc );

// Dialog Data
	enum { IDD = IDD_REPORT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CFreePcbDoc * m_doc;
	CPartList * m_pl;
	CNetList * m_nl;
	int m_flags;
	int m_units;
	int m_top;
	int m_ccw;
	CButton m_radio_top;
	CButton m_radio_ccw;
	CButton m_check_board;
	CButton m_check_drill;
	CButton m_check_parts;
	CButton m_check_cam;
	CButton m_check_drc_params;
	CButton m_check_drc;
	CButton m_check_connectivity;
	CButton m_radio_inch;
	CButton m_radio_mm;
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
};
