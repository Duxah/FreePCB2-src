#pragma once
#include "FreePcbView.h"

// CMyToolBar

class CMyToolBar : public CToolBar
{
	DECLARE_DYNAMIC(CMyToolBar)

public:
	CMyToolBar();
	virtual ~CMyToolBar();
	CFont m_font;
	int m_last_vg;
	int m_last_pg;
	int m_last_rg;
	CArray<double> * m_v;
	CArray<double> * m_p;
	CArray<double> * m_r;
	void SetLists( CArray<double> * visible, 
		CArray<double> * placement, 
		CArray<double> * routing,
		double vg, double pg, double rg, int ag, int units );
	void OnSelectVisibleGrid();
	void OnSelectPlacementGrid();
	void OnEditVisibleGrid();
	void OnEditPlacementGrid();
	void OnSelectRoutingGrid();
	void OnSelectSnapAngle();
	void OnSelectUnits();
	void SetUnits( int units );
	CStatic m_ctlStaticUnits;
	CComboBox m_ctlComboUnits;
	CStatic m_ctlStaticVisibleGrid;
	CComboBox m_ctlComboVisibleGrid;
	CStatic m_ctlStaticPlacementGrid;
	CComboBox m_ctlComboPlacementGrid;
	CStatic m_ctlStaticRoutingGrid;
	CComboBox m_ctlComboRoutingGrid;
	CStatic m_ctlStaticSnapAngle;
	CComboBox m_ctlComboSnapAngle;

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
};


