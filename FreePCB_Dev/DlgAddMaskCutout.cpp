// DlgAddMaskCutout.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgAddMaskCutout.h"


// CDlgAddMaskCutout dialog

IMPLEMENT_DYNAMIC(CDlgAddMaskCutout, CDialog)
CDlgAddMaskCutout::CDlgAddMaskCutout(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgAddMaskCutout::IDD, pParent)
{
}

CDlgAddMaskCutout::~CDlgAddMaskCutout()
{
}

void CDlgAddMaskCutout::Initialize( int l, int h )
{
	m_layer = l;
	m_hatch = h;
}
void CDlgAddMaskCutout::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_combo_layer);
	DDX_Control(pDX, IDC_RADIO1, m_radio_none);
	DDX_Control(pDX, IDC_RADIO2, m_radio_edge);
	DDX_Control(pDX, IDC_RADIO3, m_radio_full);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		m_combo_layer.InsertString( 0, "BOARD OUTLINE" );
		m_combo_layer.InsertString( 1, "SCRIBING" );
		m_combo_layer.InsertString( 2, "TOP NOTES" );
		m_combo_layer.InsertString( 3, "BOTTOM NOTES" );
		m_combo_layer.InsertString( 4, "TOP SOLDER MASK" );
		m_combo_layer.InsertString( 5, "BOTTOM SOLDER MASK" );
		m_combo_layer.InsertString( 6, "TOP SILK" );
		m_combo_layer.InsertString( 7, "BOTTOM SILK" );

		if( m_layer == LAY_BOARD_OUTLINE )
			m_combo_layer.SetCurSel(0);
		else if( m_layer == LAY_SCRIBING )
			m_combo_layer.SetCurSel(1);
		else if( m_layer == LAY_REFINE_TOP )
			m_combo_layer.SetCurSel(2);
		else if( m_layer == LAY_REFINE_BOT )
			m_combo_layer.SetCurSel(3);
		else if( m_layer == LAY_SM_TOP )
			m_combo_layer.SetCurSel(4);
		else if( m_layer == LAY_SM_BOTTOM )
			m_combo_layer.SetCurSel(5);
		else if( m_layer == LAY_SILK_TOP )
			m_combo_layer.SetCurSel(6);
		else if( m_layer == LAY_SILK_BOTTOM )
			m_combo_layer.SetCurSel(7);
		else 
			m_combo_layer.SetCurSel(0);

		m_radio_none.SetCheck(0);
		m_radio_edge.SetCheck(0);
		m_radio_full.SetCheck(0);
		if( m_hatch == CPolyLine::NO_HATCH )
			m_radio_none.SetCheck(1);
		else if( m_hatch == CPolyLine::DIAGONAL_EDGE )
			m_radio_edge.SetCheck(1);
		else
			m_radio_full.SetCheck(1);
	}
	else
	{	
		// outgoing
		if( m_combo_layer.GetCurSel() == 0 )
			m_layer = LAY_BOARD_OUTLINE;
		else if( m_combo_layer.GetCurSel() == 1 )
			m_layer = LAY_SCRIBING;
		else if( m_combo_layer.GetCurSel() == 2 )
			m_layer = LAY_REFINE_TOP;
		else if( m_combo_layer.GetCurSel() == 3 )
			m_layer = LAY_REFINE_BOT;
		else if( m_combo_layer.GetCurSel() == 4 )
			m_layer = LAY_SM_TOP;
		else if( m_combo_layer.GetCurSel() == 5 )
			m_layer = LAY_SM_BOTTOM;
		else if( m_combo_layer.GetCurSel() == 6 )
			m_layer = LAY_SILK_TOP;
		else if( m_combo_layer.GetCurSel() == 7 )
			m_layer = LAY_SILK_BOTTOM;
		else
			ASSERT(0);
		//
		if( m_radio_none.GetCheck() )
			m_hatch = CPolyLine::NO_HATCH;
		else if( m_radio_edge.GetCheck() )
			m_hatch = CPolyLine::DIAGONAL_EDGE;
		else if( m_radio_full.GetCheck() )
			m_hatch = CPolyLine::DIAGONAL_FULL;
		else
			ASSERT(0);
	}
}


BEGIN_MESSAGE_MAP(CDlgAddMaskCutout, CDialog)
END_MESSAGE_MAP()


// CDlgAddMaskCutout message handlers
