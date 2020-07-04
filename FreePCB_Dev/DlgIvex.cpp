// ivexDlg.cpp : implementation file
//

#include "stdafx.h"
//#include "ivex.h"
#include "DlgIvex.h"
#include "ivex_mod_file.h"
#include "file_io.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
extern CFreePcbApp theApp;

// CivexDlg dialog
CivexDlg::CivexDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CivexDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CivexDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_INFILE, m_edit_infile);
	DDX_Control(pDX, IDC_EDIT_OUTFILE, m_edit_outfile);
	DDX_Control(pDX, IDC_EDIT2, m_edit_messages);
}

BEGIN_MESSAGE_MAP(CivexDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON_INFILE, OnBnClickedButtonInfile)
	ON_BN_CLICKED(IDC_BUTTON_OUTFILE, OnBnClickedButtonOutfile)
	ON_EN_CHANGE(IDC_EDIT_INFILE, OnEnChangeEditInfile)
	ON_EN_CHANGE(IDC_EDIT_OUTFILE, OnEnChangeEditOutfile)
	ON_BN_CLICKED(ID_CONVERT, OnBnClickedConvert)
END_MESSAGE_MAP()


// CivexDlg message handlers

BOOL CivexDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CivexDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	CDialog::OnSysCommand(nID, lParam);
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CivexDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CivexDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CivexDlg::OnBnClickedButtonInfile()
{
	// get project file name
	CString filename = theApp.m_Doc->RunFileDialog( 1, "mod" ); 
	if ( filename.GetLength() )
	{
		in_pathname = filename;
		m_edit_infile.SetWindowText( in_pathname );
		if( in_pathname.Right(4) == ".mod" )
			m_edit_outfile.SetWindowText( in_pathname.Left( in_pathname.GetLength()-4 ) + ".fpl" );
	}
}

void CivexDlg::OnBnClickedButtonOutfile()
{
	// TODO: Add your control notification handler code here
}

void CivexDlg::OnEnChangeEditInfile()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}

void CivexDlg::OnEnChangeEditOutfile()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}

void CivexDlg::OnBnClickedConvert()
{
	// test
	CString instr, keystr;
	CArray<CString> p;

	CStdioFile mod_file, lib_file;
	m_edit_infile.GetWindowText( in_pathname );
	m_edit_outfile.GetWindowText( out_pathname );
	int err = mod_file.Open( in_pathname, CFile::modeRead, NULL );
	if( !err )
	{
		// error opening file
		CString mess;
		mess.Format( "Unable to open file %s", in_pathname );
		AfxMessageBox( mess );
		return;
	}
	err = lib_file.Open( out_pathname, CFile::modeWrite | CFile::modeCreate, NULL );
	if( !err )
	{
		// error opening file
		CString mess;
		mess.Format( "Unable to open file %s", out_pathname );
		AfxMessageBox( mess );
		return;
	}
	CString str;
	str.Format( "Converting Ivex file %s...\r\n", in_pathname );
	m_edit_messages.ReplaceSel( str );
	err = ConvertIvex( &mod_file, &lib_file, &m_edit_messages );
	m_edit_messages.ReplaceSel( "done\r\n" );
	mod_file.Close();
	lib_file.Close();
}
