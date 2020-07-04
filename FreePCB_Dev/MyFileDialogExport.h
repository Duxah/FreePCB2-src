#pragma once


// CMyFileDialogExport

class CMyFileDialogExport : public CFileDialog
{
	DECLARE_DYNAMIC(CMyFileDialogExport)

	enum { PARTS_ONLY, NETS_ONLY, PARTS_AND_NETS }; 
	enum { FREEPCB, PADSPCB }; 

public:
	CMyFileDialogExport(BOOL bOpenFileDialog, // TRUE for FileOpen, FALSE for FileSaveAs
		LPCTSTR lpszDefExt = NULL,
		LPCTSTR lpszFileName = NULL,
		DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		LPCTSTR lpszFilter = NULL,
		CWnd* pParentWnd = NULL,
		DWORD dsize = sizeof(OPENFILENAME) );
	virtual ~CMyFileDialogExport();
	void Initialize( int select );
	virtual BOOL OnFileNameOK();
	int m_select;
	int m_format;

protected:
//	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);
	CButton m_radio_parts;
	CButton m_radio_nets;
	CButton m_radio_parts_and_nets;
	CButton m_radio_padspcb;
	CButton m_radio_freepcb;
	CButton m_check_values;
	DECLARE_MESSAGE_MAP()
};


