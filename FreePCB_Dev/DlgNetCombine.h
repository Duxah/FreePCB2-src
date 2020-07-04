// CDlgNetCombine dialogC

class CDlgNetCombine : public CDialog
{
	DECLARE_DYNAMIC(CDlgNetCombine)

public:
	CDlgNetCombine(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgNetCombine();

// Dialog Data
	enum { IDD = IDD_NET_COMBINE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	int m_visible_state;
	int m_item_selected;
	CArray<int> *m_w;		// array of default widths
	CArray<int> *m_v_w;		// array of via widths (matching m_w[] entries)
	CArray<int> *m_v_h_w;	// array of via hole widths
	CNetList * m_nlist;
	CPartList * m_plist;
	netlist_info * m_nl;
	CArray<CString> m_names;	// array of net names to combine
	CString m_new_name;			// name for combined net

	void Initialize( CNetList * nlist, CPartList * plist ); 
private:
	int m_sort_type;
	CListCtrl m_list_ctrl;
	virtual BOOL OnInitDialog();
	void CDlgNetCombine::DrawListCtrl();
	afx_msg void OnLvnColumnclickListNet(NMHDR *pNMHDR, LRESULT *pResult);
	CButton m_OK;
	CButton m_cancel;
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
};
