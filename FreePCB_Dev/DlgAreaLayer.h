#pragma once
#include "afxwin.h"


// DlgAreaLayer dialog

class DlgAreaLayer : public CDialog
{
	DECLARE_DYNAMIC(DlgAreaLayer)

public:
	DlgAreaLayer(CWnd* pParent = NULL);   // standard constructor
	virtual ~DlgAreaLayer();
	void Initialize( int num_layers );

// Dialog Data
	enum { IDD = IDD_AREA_LAYER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CListBox m_list_layer;
	int m_num_layers;
	int m_layer;
};
