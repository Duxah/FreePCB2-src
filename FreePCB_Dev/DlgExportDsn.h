#pragma once
#include "afxwin.h"


// CDlgExportDsn dialog

class CDlgExportDsn : public CDialog
{
	DECLARE_DYNAMIC(CDlgExportDsn)

public:
	CDlgExportDsn(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgExportDsn();
	void Initialize( CString * dsn_filepath,
					 int num_board_outline_polys,
					 int bounds_poly, int signals_poly,
					 int flags );


// Dialog Data
	enum { IDD = IDD_EXPORT_DSN };

	// flags for options
	enum {
		DSN_VERBOSE = 0x1,
		DSN_INFO_ONLY = 0x2,
		DSN_FROM_TO_MASK = 0xC,
		DSN_FROM_TO_NONE = 0,
		DSN_FROM_TO_ALL = 0x4,
		DSN_FROM_TO_LOCKED = 0x8,
		DSN_FROM_TO_NET_LOCKED = 0xC
	};

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	int m_num_polys;
	int m_bounds_poly;
	int m_signals_poly;
	int m_flags;
	CComboBox m_combo_bounds;
	CComboBox m_combo_signals;
	CString m_dsn_filepath;
	CButton m_check_verbose;
	CButton m_check_info;
	CButton m_check_from_to;
	CButton m_radio_all;
	CButton m_radio_locked;
	CButton m_radio_net_locked;
	afx_msg void OnBnClickedCheckFromTo();
	afx_msg void OnBnClickedRadioAll();
	afx_msg void OnBnClickedRadioLocked();
	afx_msg void OnBnClickedRadioNetLocked();
};
