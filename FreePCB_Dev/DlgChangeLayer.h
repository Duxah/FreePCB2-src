#pragma once
#include "afxwin.h"


// CDlgChangeLayer dialog

class CDlgChangeLayer : public CDialog
{
	DECLARE_DYNAMIC(CDlgChangeLayer)

public:
	CDlgChangeLayer(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgChangeLayer();

// Dialog Data
	enum { IDD = IDD_CHANGE_LAYER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CComboBox m_combo_layer;
	int m_mode;
	int m_num_copper_layers;
	int m_old_layer;
	int m_new_layer;
	int m_apply_to;
	void Initialize( int mode, int old_layer, int num_copper_layers );
	CButton m_radio_segment;
	CButton m_radio_trace;
	CButton m_radio_net;
};
