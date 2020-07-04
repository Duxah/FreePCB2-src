#pragma once
#include "afxwin.h"
#include "Netlist.h"


// CDlgAddArea dialog

class CDlgAddArea : public CDialog
{
	DECLARE_DYNAMIC(CDlgAddArea)

public:
	CDlgAddArea(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgAddArea();
	void Initialize( CNetList * nl, int nlayers, 
		cnet * net, int layer, int hatch, int width, int units, BOOL EN_W=TRUE );

// Dialog Data
	enum { IDD = IDD_ADD_AREA };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CString m_net_name;		// set to net name on return
	cnet * m_net;		// set to selected net, or NULL if net doesn't exist
	int m_layer;	// set to selected layer on return
	int m_num_layers;
	int m_hatch;
	int m_width;
	int m_units;
	int m_en_w;
	BOOL bNewArea;
	CNetList * m_nlist;
	CComboBox m_combo_net;
	CEdit m_area_width;
	CListBox m_list_layer;
	CButton m_radio_none;
	CButton m_radio_full;
	CButton m_radio_edge;
};
