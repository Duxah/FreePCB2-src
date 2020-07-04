#pragma once
#include "afxwin.h"


// CIndexing dialog

class CDlgIndexing : public CDialog
{
	DECLARE_DYNAMIC(CDlgIndexing)

public:
	CDlgIndexing(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgIndexing();

// Dialog Data
	enum { IDD = IDD_INDEXING };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
};
