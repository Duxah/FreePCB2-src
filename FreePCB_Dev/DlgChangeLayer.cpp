// DlgChangeLayer.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgChangeLayer.h"


// CDlgChangeLayer dialog

IMPLEMENT_DYNAMIC(CDlgChangeLayer, CDialog)
CDlgChangeLayer::CDlgChangeLayer(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgChangeLayer::IDD, pParent)
{
}

CDlgChangeLayer::~CDlgChangeLayer()
{
}

void CDlgChangeLayer::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_combo_layer);
	DDX_Control(pDX, IDC_RADIO_SEGMENT, m_radio_segment);
	DDX_Control(pDX, IDC_RADIO_TRACE, m_radio_trace);
	DDX_Control(pDX, IDC_RADIO_NET, m_radio_net);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		for( int il=0; il<m_num_copper_layers; il++ )
		{
			m_combo_layer.InsertString( il, &layer_str[il+LAY_TOP_COPPER][0] );
		}
		if( m_old_layer >= LAY_TOP_COPPER )
			m_combo_layer.SetCurSel( m_old_layer - LAY_TOP_COPPER );
		else
			m_combo_layer.SetCurSel( 0 );
		if( m_mode == 0 )
			m_radio_segment.SetCheck(1);
		else if( m_mode == 1 )
		{
			m_radio_trace.SetCheck(1);
			m_radio_segment.EnableWindow(0);
		}
		else if( m_mode == 2 )
		{
			m_radio_net.SetCheck(1);
			m_radio_segment.EnableWindow(0);
			m_radio_trace.EnableWindow(0);
		}
	}
	else
	{
		// outgoing
		m_new_layer = m_combo_layer.GetCurSel() + LAY_TOP_COPPER;
		if( m_radio_segment.GetCheck() )
			m_apply_to = 0;
		else if( m_radio_trace.GetCheck() )
			m_apply_to = 1;
		else if( m_radio_net.GetCheck() )
			m_apply_to = 2;
	}
}

BEGIN_MESSAGE_MAP(CDlgChangeLayer, CDialog)
END_MESSAGE_MAP()


// CDlgChangeLayer message handlers

void CDlgChangeLayer::Initialize( int mode, int old_layer, int num_copper_layers )
{
	m_mode = mode;
	m_old_layer = old_layer;
	m_num_copper_layers = num_copper_layers;
}
