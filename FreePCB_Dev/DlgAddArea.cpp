// DlgAddArea.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgAddArea.h"
#include "layers.h"

// globals
int gHatch = CPolyLine::NO_HATCH;

// CDlgAddArea dialog

IMPLEMENT_DYNAMIC(CDlgAddArea, CDialog)
CDlgAddArea::CDlgAddArea(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgAddArea::IDD, pParent)
{
}

CDlgAddArea::~CDlgAddArea()
{
}

void CDlgAddArea::DoDataExchange(CDataExchange* pDX)
{
	CString str;
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_NET, m_combo_net);
	DDX_Control(pDX, IDC_AREA_WIDTH, m_area_width);
	DDX_Control(pDX, IDC_LIST_LAYER, m_list_layer);
	DDX_Control(pDX, IDC_RADIO_NONE, m_radio_none);
	DDX_Control(pDX, IDC_RADIO_FULL, m_radio_full);
	DDX_Control(pDX, IDC_RADIO_EDGE, m_radio_edge);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming, initialize net list
		cnet * net = m_nlist->GetFirstNet();
		while( net )
		{
			m_combo_net.AddString( net->name );
			net = m_nlist->GetNextNet(/*LABEL*/); 
		}
		if( m_net )
		{
			bNewArea = FALSE;
			int i = m_combo_net.SelectString( -1, m_net->name );
			if( i == CB_ERR )
				ASSERT(0);
		}
		else
			bNewArea = TRUE;

		// initialize layers
		m_num_layers = m_num_layers-LAY_TOP_COPPER;
		for( int il=0; il<m_num_layers; il++ )
		{
			m_list_layer.InsertString( il, &layer_str[il+LAY_TOP_COPPER][0] );
		}
		m_list_layer.SetCurSel( max(m_layer,LAY_TOP_COPPER) - LAY_TOP_COPPER );
		if( m_hatch == -1 )
			m_hatch = gHatch;	
		if( m_hatch == CPolyLine::NO_HATCH )
			m_radio_none.SetCheck( 1 );
		else if( m_hatch == CPolyLine::DIAGONAL_EDGE )
			m_radio_edge.SetCheck( 1 );
		else if( m_hatch == CPolyLine::DIAGONAL_FULL )
			m_radio_full.SetCheck( 1 );	 
		::MakeCStringFromDimension( &str, m_width, m_units, FALSE, FALSE, FALSE, m_units==MIL?0:2 );
		m_area_width.SetWindowTextA( str );
		m_area_width.EnableWindow( m_en_w );
	}
	else
	{
		// outgoing
		m_layer = m_list_layer.GetCurSel() + LAY_TOP_COPPER;
		m_combo_net.GetWindowText( m_net_name );

		POSITION pos;
		CString name;
		void * ptr;
		m_net = m_nlist->GetNetPtrByName( &m_net_name );
		if( !m_net )
		{
			AfxMessageBox( "Illegal net name" );
			pDX->Fail();
		}
		if( m_radio_none.GetCheck() )
			m_hatch = CPolyLine::NO_HATCH;
		else if( m_radio_full.GetCheck() )
			m_hatch = CPolyLine::DIAGONAL_FULL;
		else if( m_radio_edge.GetCheck() )
			m_hatch = CPolyLine::DIAGONAL_EDGE;
		else 
			ASSERT(0);
		if( bNewArea )
			gHatch = m_hatch;
		m_area_width.GetWindowText( str );
		double mult = NM_PER_MIL;
		if( m_units == MM )
			mult = 1000000.0;
		m_width = mult*atof( str ); 
	}
}


BEGIN_MESSAGE_MAP(CDlgAddArea, CDialog)
END_MESSAGE_MAP()


// CDlgAddArea message handlers

void CDlgAddArea::Initialize( CNetList * nl, int nlayers, 
							 cnet * net, int layer, int hatch, int width, int units, BOOL EN_W )
{
	m_nlist = nl;
	m_num_layers = nlayers;
	m_net = net;
	m_layer = layer;
	m_hatch = hatch;
	m_width = width;
	m_units = units;
	m_en_w = EN_W;
}
