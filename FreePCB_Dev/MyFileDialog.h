#pragma once


// CMyFileDialog

class CMyFileDialog : public CFileDialog
{
	DECLARE_DYNAMIC(CMyFileDialog)

	enum { PARTS_ONLY, NETS_ONLY, PARTS_AND_NETS }; 

public:
	CMyFileDialog(BOOL bOpenFileDialog, // TRUE for FileOpen, FALSE for FileSaveAs
		LPCTSTR lpszDefExt = NULL,
		LPCTSTR lpszFileName = NULL,
		DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		LPCTSTR lpszFilter = NULL,
		CWnd* pParentWnd = NULL,
		DWORD dsize = NULL );//sizeof(OPENFILENAME) );
	virtual ~CMyFileDialog();
	virtual BOOL OnFileNameOK();
	void Initialize( int flags );
	int m_flags;
	int m_format;

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);
	CButton m_radio_parts;
	CButton m_radio_nets;
	CButton m_radio_parts_and_nets;
	CButton m_radio_padspcb;
	CButton m_radio_freepcb;
	DECLARE_MESSAGE_MAP()
};


