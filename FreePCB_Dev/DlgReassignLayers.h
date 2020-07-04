#pragma once
#include "afxwin.h"


// CDlgReassignLayers dialog

class CDlgReassignLayers : public CDialog
{
	DECLARE_DYNAMIC(CDlgReassignLayers)

public:
	CDlgReassignLayers(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgReassignLayers();
	void Initialize( int num_old_layers, int num_new_layers );

// Dialog Data
	enum { IDD = IDD_REASSIGN_LAYERS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CComboBox m_combo[16];
	int m_old_layers;
	int m_new_layers;
	int new_layer[16];	// array to hold new layer assignments
};
