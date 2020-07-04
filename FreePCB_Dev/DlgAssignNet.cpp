// DlgAssignNet.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgAssignNet.h"
#include ".\dlgassignnet.h"


// DlgAssignNet dialog

IMPLEMENT_DYNAMIC(DlgAssignNet, CDialog)
DlgAssignNet::DlgAssignNet(CWnd* pParent /*=NULL*/)
	: CDialog(DlgAssignNet::IDD, pParent)
{
	m_net_str = "";
	m_map = 0;
	created_name = "";
	m_nLOCK = 0;
}

DlgAssignNet::~DlgAssignNet()
{
}

void DlgAssignNet::DoDataExchange(CDataExchange* pDX)
{	
	CDialog::DoDataExchange(pDX);
	DDX_CBString( pDX, IDC_COMBO_NET, m_net_str );
	DDV_MaxChars( pDX, m_net_str, MAX_NET_NAME_SIZE+1 );
	DDX_Control(pDX, IDC_COMBO_NET, m_combo_net);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		if( m_map )
		{
			POSITION pos;
			CString key;
			void * ptr;

			// Iterate through the entire netlist
			for( pos = m_map->GetStartPosition(); pos != NULL; )
			{
				m_map->GetNextAssoc( pos, key, ptr );
				m_combo_net.AddString( key );
			}
		}
		m_combo_net.SetFocus();
		::ShowCursor( TRUE );	// force cursor
	}
	else
	{
		// outgoing
		pDX->PrepareCtrl( IDC_COMBO_NET );
		if( m_net_str.GetLength()  == 0 )
		{
			AfxMessageBox( "Illegal net name" );
			pDX->Fail();
		}
		else if (m_net_str.Find("\"",0) >= 0)
		{
			AfxMessageBox( "Illegal net name" );
			pDX->Fail();
		}
		void * ptr;
		if( m_net_str != created_name ) 
		{
			if( !m_map->Lookup( m_net_str, ptr ) )
			{
				POSITION pos;
				CString name;
				void * net_ptr;
				for( pos = m_map->GetStartPosition(); pos != NULL; )
				{
					// next net
					m_map->GetNextAssoc( pos, name, net_ptr );
					cnet * net = (cnet*)net_ptr;
					if( net )
						for( int ip=0; ip<net->npins; ip++ )
						{
							cpart * prt = net->pin[ip].part;
							CString ps;
							ps.Format("%s.%s", prt->ref_des, net->pin[ip].pin_name );
							if( ps.Compare(m_net_str) == 0 )
							{
								m_net_str = net->name;
								break;
							}
						}
				}
			}
			if( !m_map->Lookup( m_net_str, ptr ) )
			{
				CString str = "Net \"" + m_net_str + "\" not found in netlist\nCreate it ?"; 
				if( str.Find(".") > 0 )
					str = "Neither pin nor net \"" + m_net_str + "\" was found in netlist\nCreate this net ?";
				int ret = AfxMessageBox( str, MB_YESNO );
				if( ret == IDNO )
				{
					pDX->Fail();
				}
			}
			else if( m_nLOCK )
				pDX->Fail();
		}
		::ShowCursor( FALSE );	// restore cursor
	}
}


BEGIN_MESSAGE_MAP(DlgAssignNet, CDialog)
	ON_BN_CLICKED(IDC_BUTTON_NEW_NET, OnBnClickedButtonNewNet)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
END_MESSAGE_MAP()


// DlgAssignNet message handlers

void DlgAssignNet::OnBnClickedButtonNewNet()
{
	CString str;
	int i = 0;
	BOOL bFound = TRUE;
	while( bFound )
	{
		i++;
		str.Format( "N%.5d", i );
		void * ptr;
		if( !m_map->Lookup( str, ptr ) )
			bFound = FALSE;
	}
	m_combo_net.SetWindowText( str );
	created_name = str;
}

void DlgAssignNet::OnBnClickedCancel()
{
	::ShowCursor( FALSE );	// restore cursor
	OnCancel();
}
