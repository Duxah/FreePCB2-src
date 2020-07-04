// DlgEditNet.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgEditNet.h"
#include ".\dlgeditnet.h"


// CDlgEditNet dialog

IMPLEMENT_DYNAMIC(CDlgEditNet, CDialog)
CDlgEditNet::CDlgEditNet(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgEditNet::IDD, pParent)
{
	m_pins_edited = FALSE;
	m_units = MIL;
}

CDlgEditNet::~CDlgEditNet()
{
}

void CDlgEditNet::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_RADIO_DEF_WIDTH, m_button_def_width);
	DDX_Control(pDX, IDC_RADIO_SET_WIDTH, m_button_set_width);
	DDX_Control(pDX, IDC_EDIT_VIA_PAD_W, m_edit_pad_w);
	DDX_Control(pDX, IDC_EDIT_VIA_HOLE_W, m_edit_hole_w);
	DDX_Control(pDX, IDC_LIST_PIN, m_list_pins);
	DDX_Control(pDX, IDC_EDIT_NAME, m_edit_name);
	DDX_Control(pDX, IDC_CHECK_VISIBLE, m_check_visible);
	DDX_Control(pDX, IDC_COMBO_WIDTH, m_combo_width);
	DDX_Control(pDX, IDC_EDIT_ADD_PIN, m_edit_addpin);
	DDX_Control(pDX, IDC_CHECK1, m_check_apply);
	DDX_Control(pDX, IDC_BUTTON_ADD, m_button_add_pin);
	DDX_Control(pDX, ID_MY_OK, m_button_OK);
	m_button_add_pin.EnableWindow( !m_nlist_lock );
	if( pDX->m_bSaveAndValidate )
	{
		// now implement edits into netlist_info
		m_edit_name.GetWindowText( m_name );
		if( m_name.GetLength() > MAX_NET_NAME_SIZE )
		{
			CString mess;
			mess.Format( "Max length of net name is %d characters", MAX_NET_NAME_SIZE ); 
			AfxMessageBox( mess );
			pDX->Fail();
		}
		CString s = "";
		DDX_Text(pDX,IDC_COMBO_WIDTH,s);
		if( s.Left(1) == "-" )
		{
			AfxMessageBox( "illegal trace width" );
			pDX->Fail();
		}
		m_def_w = my_atof(&s);
		s = "";
		DDX_Text( pDX, IDC_EDIT_VIA_PAD_W, s );
		if( s.Left(1) == "-" )
		{
			AfxMessageBox( "illegal via width" );
			pDX->Fail();
		}
		m_def_v_w = my_atof(&s);
		s = "";
		DDX_Text( pDX, IDC_EDIT_VIA_HOLE_W, s );
		if( s.Left(1) == "-" )
		{
			AfxMessageBox( "illegal hole width" );
			pDX->Fail();
		}
		m_def_v_h_w = my_atof(&s); 
		m_visible = m_check_visible.GetState();
		for( int in=0; in<m_nl->GetSize(); in++ )
		{
			if( m_name == (*m_nl)[in].name && m_in != in && !(*m_nl)[in].deleted )
			{
				AfxMessageBox( "duplicate net name" );
				pDX->Fail();
			}
		}
	    if (m_name.Find("\"",0) >= 0)
		{
			AfxMessageBox( "Illegal net name" );
			pDX->Fail();
		}

		// now update netlist_info
		if( m_new_net )
		{
			// add entry to netlist_info for new net, set index m_in
			int num_nets = m_nl->GetSize();
			m_nl->SetSize( num_nets + 1 );
			m_in = num_nets;
			(*m_nl)[m_in].net = NULL;
		}
		(*m_nl)[m_in].apply_trace_width = m_check_apply.GetCheck();
		(*m_nl)[m_in].apply_via_width = m_check_apply.GetCheck();
		(*m_nl)[m_in].modified = TRUE;
		(*m_nl)[m_in].name = m_name;
		(*m_nl)[m_in].visible = m_visible;
		(*m_nl)[m_in].w = abs(m_def_w);
		(*m_nl)[m_in].v_w = abs(m_def_v_w);
		(*m_nl)[m_in].v_h_w = abs(m_def_v_h_w);
		if( m_pins_edited )
		{
			int npins = m_list_pins.GetCount();
			(*m_nl)[m_in].ref_des.SetSize( npins );
			(*m_nl)[m_in].pin_name.SetSize( npins );
			for( int i=0; i<npins; i++ )
			{
				// now parse pin string from listbox
				CString str;
				m_list_pins.GetText( i, str );
				str.Trim();
				int len = str.GetLength();
				if( len < 3 )
					ASSERT(0);
				int dot_pos = str.FindOneOf( "." );
				if( dot_pos == 0 || dot_pos >= (len-1) )
					ASSERT(0);
				CString refstr = str.Left(dot_pos);
				CString pinstr = str.Right( len - dot_pos - 1 );
				int pin = atoi( pinstr );
				(*m_nl)[m_in].ref_des[i] = refstr;
				(*m_nl)[m_in].pin_name[i] = pinstr;
				// now remove pin from other net, if necessary
				for( int in=0; in<m_nl->GetSize(); in++ )
				{
					if( (*m_nl)[in].deleted == FALSE && m_in != in )
					{
						for( int ip=0; ip<(*m_nl)[in].ref_des.GetSize(); ip++ )
						{
							if( refstr == (*m_nl)[in].ref_des[ip]
							&& pinstr == (*m_nl)[in].pin_name[ip] )
							{
								(*m_nl)[in].modified = TRUE;
								(*m_nl)[in].ref_des.RemoveAt( ip );
								(*m_nl)[in].pin_name.RemoveAt( ip);
							}
						}
					}
				}
			}
		}
	}
	else
	{
		// default is to apply new trace width
		m_check_apply.SetCheck(0);
	}
}

void CDlgEditNet::Initialize( netlist_info * nl,
							 int i,
							 CPartList * plist,
							 BOOL new_net,
							 BOOL visible,
							 int units,
							 int nlist_lock,
							 CArray<int> * w,
							 CArray<int> * v_w,
							 CArray<int> * v_h_w,
							 CString * pin_str )
{
	m_nlist_lock = nlist_lock;
	m_units = units;
	if( pin_str )
		m_s_pin = *pin_str;
	else
		m_s_pin = "";
	m_nl = nl;
	m_in = i;
	m_plist = plist;
	m_visible = visible;
	m_new_net = new_net;
	if( new_net )
	{
		m_name = "";
		m_def_w = 0;
		m_def_v_w = 0;
		m_def_v_h_w = 0;
	}
	else
	{
		m_name = (*nl)[i].name;
		m_def_w = (*nl)[i].w;
		m_def_v_w = (*nl)[i].v_w;
		m_def_v_h_w = (*nl)[i].v_h_w;
	}
	m_w = w;
	m_v_w = v_w;
	m_v_h_w = v_h_w;
}

BEGIN_MESSAGE_MAP(CDlgEditNet, CDialog)
	ON_BN_CLICKED(IDC_RADIO_DEF_WIDTH, OnBnClickedRadioDefWidth)
	ON_BN_CLICKED(IDC_RADIO_SET_WIDTH, OnBnClickedRadioSetWidth)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, OnBnClickedButtonDelete)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnBnClickedButtonAdd)
	ON_CBN_SELCHANGE(IDC_COMBO_WIDTH, OnCbnSelchangeComboWidth)
	ON_CBN_EDITCHANGE(IDC_COMBO_WIDTH, OnCbnEditchangeComboWidth)
	ON_EN_UPDATE(IDC_EDIT_ADD_PIN, OnEnUpdateEditAddPin)
	ON_BN_CLICKED(ID_MY_OK, OnBnClickedOk)
END_MESSAGE_MAP()


// CDlgEditNet message handlers

void CDlgEditNet::OnBnClickedRadioDefWidth()
{
	CString test;
	m_combo_width.GetWindowTextA(test);
	ChangeTraceWidth( test );
	m_edit_pad_w.EnableWindow( 0 );
	m_edit_hole_w.EnableWindow( 0 );
}

void CDlgEditNet::OnBnClickedRadioSetWidth()
{
	m_edit_pad_w.EnableWindow( 1 );
	m_edit_hole_w.EnableWindow( 1 );
}

void CDlgEditNet::OnBnClickedButtonDelete()
{
	BOOL Flag = FALSE;
	int sc = m_list_pins.GetSelCount();
	if( sc == 1 )
	{
		CString str = "";
		sc = -1;
		for( int ii=m_list_pins.GetCount()-1; ii>=0; ii-- )
		{
			int sel = m_list_pins.GetSel(ii);
			if(sel)
			{
				sc = ii;
				break;
			}
		}
		if( sc >= 0 )
		{
			m_list_pins.GetText( sc, str );
			sc = str.Find(".");
			if( sc > 0 )
			{
				str = str.Left(sc);
				cpart * part = m_plist->GetPart( str );
				if( part )
					if( part->shape )
						if( part->shape->GetNumPins() == 1 ) 
							Flag = TRUE;
			}
		}
	}
	if( m_nlist_lock == 0 || Flag )
	{
		int ret = IDOK;//AfxMessageBox( "Delete pin(s) from net?", MB_OKCANCEL );
		if( ret == IDOK )
		{
			// Get the indexes of all the selected pins and delete them
			int nCount = m_list_pins.GetSelCount();
			CArray<int,int> aryListBoxSel;
			aryListBoxSel.SetSize(nCount);
			m_list_pins.GetSelItems(nCount, aryListBoxSel.GetData()); 
			for( int j=(nCount-1); j>=0; j-- )
			{
				int iItem = aryListBoxSel[j];
				m_list_pins.DeleteString( iItem );
			}
			m_pins_edited = TRUE;
		}
	}
}

void CDlgEditNet::OnBnClickedButtonAdd()
{
	if( m_nlist_lock )
		return;
	CString str;

	m_edit_addpin.GetWindowText( str );
	str.Trim();
	int len = str.GetLength();
	if( len < 3 )
	{
		AfxMessageBox( "Illegal pin" );
		return;
	}
	else
	{
		int test = m_list_pins.FindStringExact( 0, str );
		if( test != -1 )
		{
			AfxMessageBox( "Pin already in this net" );
			return;
		}
		int dot_pos = str.FindOneOf( "." );
		if( dot_pos >= 2 && dot_pos < (len-1) )
		{
			CString refstr = str.Left( dot_pos );
			cpart * part = m_plist->GetPart( refstr );
			if( !part )
			{
				str.Format( "Part \"%s\" does not exist", refstr );
				AfxMessageBox( str );
				return;
			}
			CString pinstr = str.Right( len - dot_pos - 1 );
			if( !CheckLegalPinName( &pinstr ) )
			{
				str = "Pin name must consist of zero or more letters\n";
				str	+= "Followed by zero or more numbers\n";
				str	+= "For example: 1, 23, A12, SOURCE are legal\n";
				str	+= "while 1A, A2B3 are not\n";
				AfxMessageBox( str );
				return;
			}
			if( !part->shape )
			{
				str.Format( "Part \"%s\" does not have a footprint", refstr );
				int ret = AfxMessageBox( str, MB_OKCANCEL );
				if( ret != IDOK )
					return;
			}
			else
			{
				int pin_index = part->shape->GetPinIndexByName( pinstr, -1 );
				if( pin_index == -1 )
				{
					str.Format( "Pin \"%s\" not found in footprint \"%s\"", pinstr, 
						part->shape->m_name );
					AfxMessageBox( str );
					return;
				}
			}
			// now search for pin in m_nl
			int i_found = -1;
			for( int i=0; i<m_nl->GetSize(); i++ )
			{
				if( (*m_nl)[i].deleted == FALSE )
				{
					for( int ip=0; ip<(*m_nl)[i].ref_des.GetSize(); ip++ )
					{
						if( refstr.Compare((*m_nl)[i].ref_des[ip]) == 0
						 && pinstr.Compare((*m_nl)[i].pin_name[ip]) == 0 )
						{
							i_found = i;
							break;
						}
					}
				}
			}
			if( i_found != -1 )
			{
				if( i_found == m_in )
				{
					AfxMessageBox( "Pin already in this net" );
					return;
				}
				else
				{
					CString mess;
					mess.Format( "Pin %s.%s is assigned to net \"%s\"\nRemove it from \"%s\"? ",
						refstr, pinstr, (*m_nl)[i_found].name, (*m_nl)[i_found].name );
					int ret = AfxMessageBox( mess, MB_OKCANCEL );
					if( ret != IDOK )
						return;
				}
			}
			m_list_pins.AddString( refstr + "." + pinstr );
			m_pins_edited = TRUE;
			m_button_OK.SetButtonStyle( BS_DEFPUSHBUTTON );
			m_button_add_pin.SetButtonStyle( BS_PUSHBUTTON );
			m_edit_addpin.SetWindowText( "" );
		}
	}

}

// enter with the following variables set up by calling function:
//	m_new_net = TRUE to add net, FALSE to edit net
//	m_name = pointer to CString with net name (or "" if adding new net)
//  m_use_nl_not_nlist = TRUE to use m_nl, FALSE to use m_nlist for net info
//	m_nl = pointer to net_info array 
//	m_in = index into m_nl for this net
//	m_plist = pointer to partlist
//	m_w = pointer to CArray of trace widths
//	m_v_w = pointer to CArray of via pad widths
//	m_v_h_w = pointer to CArray of via hole widths
//	m_visible = visibility flag
//
BOOL CDlgEditNet::OnInitDialog()
{
	CDialog::OnInitDialog();
	CString str;
	m_edit_name.SetWindowText( m_name );
	::MakeCStringFromDimension( &str, abs(m_def_w), m_units, TRUE, TRUE, FALSE, (m_units==MIL?0:3) );
	m_combo_width.SetWindowText( str );
	::MakeCStringFromDimension( &str, abs(m_def_v_w), m_units, TRUE, TRUE, FALSE, (m_units==MIL?0:3) );
	m_edit_pad_w.SetWindowText( str );
	::MakeCStringFromDimension( &str, abs(m_def_v_h_w), m_units, TRUE, TRUE, FALSE, (m_units==MIL?0:3) );
	m_edit_hole_w.SetWindowText( str );
	m_edit_pad_w.EnableWindow( 1 );
	m_edit_hole_w.EnableWindow( 1 );
	m_button_def_width.SetCheck( 0 );
	m_button_set_width.SetCheck( 1 );
	m_check_visible.SetCheck( m_visible );
	// set up combo box for trace widths
	int n = m_w->GetSize();
	for( int i=0; i<n; i++ )
	{
		::MakeCStringFromDimension( &str, abs((*m_w)[i]), m_units, TRUE, TRUE, FALSE, (m_units==MIL?0:3) );
		m_combo_width.InsertString( i, str );
	}
	// set up list box for pins
	if( !m_new_net )
	{
		int npins = (*m_nl)[m_in].ref_des.GetSize();
		for( int i=0; i<npins; i++ )
		{
			str.Format( "%s.%s", (*m_nl)[m_in].ref_des[i], (*m_nl)[m_in].pin_name[i] ); 
			m_list_pins.AddString( str );
		}
		if( m_s_pin.GetLength() )
		{
			int i_pin = m_list_pins.FindString( 0, m_s_pin );
			if( i_pin >= 0 )
				m_list_pins.SetSel( i_pin );
		}
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgEditNet::OnCbnSelchangeComboWidth()
{
	CString test;
	int i = m_combo_width.GetCurSel();
	m_combo_width.GetLBText( i, test );
	ChangeTraceWidth( test );
}

void CDlgEditNet::OnCbnEditchangeComboWidth()
{
	CString test;
	m_combo_width.GetWindowText( test );
	ChangeTraceWidth( test );
}


void CDlgEditNet::ChangeTraceWidth( CString test )
{
	int n = m_w->GetSize();
	if( m_button_def_width.GetCheck() )
	{
		int new_w = my_atof( &test );
		int new_v_w = 0;
		int new_v_h_w = 0;
		if( new_w > 0 )
		{
			for( int i=0; i<n; i++ )
			{
				if( new_w == abs((*m_w)[i]) ) 
				{
					new_v_w = (*m_v_w)[i];
					new_v_h_w = (*m_v_h_w)[i];
					break;
				}
			}
		}
		CString s;
		if ( test.Right(2) == "MM" || test.Right(2) == "mm" )
			::MakeCStringFromDimension( &s, abs(new_v_w), MM, TRUE, TRUE, FALSE, 2 );
		else
			::MakeCStringFromDimension( &s, abs(new_v_w), MIL, TRUE, TRUE, FALSE, 0 );
		m_edit_pad_w.SetWindowText( s );
		if ( test.Right(2) == "MM" || test.Right(2) == "mm" )
			::MakeCStringFromDimension( &s, abs(new_v_h_w), MM, TRUE, TRUE, FALSE, 2 );
		else
			::MakeCStringFromDimension( &s, abs(new_v_h_w), MIL, TRUE, TRUE, FALSE, 0 );
		m_edit_hole_w.SetWindowText( s );
	}
}

void CDlgEditNet::SetFields()
{
}

void CDlgEditNet::GetFields()
{
}

void CDlgEditNet::OnEnUpdateEditAddPin()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_UPDATE flag ORed into the lParam mask.
	m_button_OK.SetButtonStyle( BS_PUSHBUTTON );
	m_button_add_pin.SetButtonStyle( BS_DEFPUSHBUTTON );
}

void CDlgEditNet::OnBnClickedOk()
{
	// if we are adding pins, trap "Enter" key
	void * focus_ptr = this->GetFocus();
	void * addpin_ptr = &m_edit_addpin;
	CString str;
	m_edit_addpin.GetWindowText( str );
	if( focus_ptr == addpin_ptr && str.GetLength() > 0 )
		OnBnClickedButtonAdd();
	else
		OnOK();
}
