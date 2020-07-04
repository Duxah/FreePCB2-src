#pragma once
#include "layers.h"

#define NUM_DLG_LAYERS (LAY_TOP_COPPER + 16)// copper layers

// CDlgLayers dialog

class CDlgLayers : public CDialog
{
	DECLARE_DYNAMIC(CDlgLayers)

public:
	CDlgLayers(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgLayers();

// Dialog Data
	enum { IDD = IDD_LAYERS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

private:
	int * m_vis;
	int m_rgb[MAX_LAYERS][3];
	int * m_rgb_ptr;
	int m_nlayers;
	CBrush m_brush;
	CColorDialog * m_cdlg;

	DECLARE_MESSAGE_MAP()
public:
	int m_check[NUM_DLG_LAYERS];
	int m_sys_clrs; //checkbox "use sys color"
	void Initialize( int nlayers, int vis[], int rgb[][3], int system_colors );
	void EditColor( int layer );
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnBnClickedButtonLayer1();
	afx_msg void OnBnClickedButtonLayer2();
	afx_msg void OnBnClickedButtonLayer3();
	afx_msg void OnBnClickedButtonLayer4();
	afx_msg void OnBnClickedButtonLayer5();
	afx_msg void OnBnClickedButtonLayer6();
	afx_msg void OnBnClickedButtonLayer7();
	afx_msg void OnBnClickedButtonLayer8();
	afx_msg void OnBnClickedButtonLayer9();
	afx_msg void OnBnClickedButtonLayer10();
	afx_msg void OnBnClickedButtonLayer11();
	afx_msg void OnBnClickedButtonLayer12();
	afx_msg void OnBnClickedButtonLayer13();
	afx_msg void OnBnClickedButtonLayer14();
	afx_msg void OnBnClickedButtonLayer15();
	afx_msg void OnBnClickedButtonLayer16();
	afx_msg void OnBnClickedButtonLayer17();
	afx_msg void OnBnClickedButtonLayer18();
	afx_msg void OnBnClickedButtonLayer19();
	afx_msg void OnBnClickedButtonLayer20();
	afx_msg void OnBnClickedButtonLayer21();
	afx_msg void OnBnClickedButtonLayer22();
	afx_msg void OnBnClickedButtonLayer23();
	afx_msg void OnBnClickedButtonLayer24();
	afx_msg void OnBnClickedButtonLayer25();
	afx_msg void OnBnClickedButtonLayer26();
	afx_msg void OnBnClickedButtonLayer27();
	afx_msg void OnBnClickedButtonLayer28();
	afx_msg void OnBnClickedButtonLayer29();
	afx_msg void OnBnClickedButtonLayer30();
	afx_msg void OnBnClickedButtonLayer31();
};
