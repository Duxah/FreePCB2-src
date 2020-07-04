// DlgSetAreaHatch.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgSetAreaHatch.h"


// CDlgSetAreaHatch dialog

IMPLEMENT_DYNAMIC(CDlgSetAreaHatch, CDialog)
CDlgSetAreaHatch::CDlgSetAreaHatch(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgSetAreaHatch::IDD, pParent)
{
}

CDlgSetAreaHatch::~CDlgSetAreaHatch()
{
}

void CDlgSetAreaHatch::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_RADIO_HATCH_NONE, m_radio_none);
	DDX_Control(pDX, IDC_RADIO_HATCH_FULL, m_radio_full);
	DDX_Control(pDX, IDC_RADIO_HATCH_EDGE, m_radio_edge);
	if( pDX->m_bSaveAndValidate )
	{
		if( m_radio_none.GetCheck() )
			m_hatch = CPolyLine::NO_HATCH;
		else if( m_radio_full.GetCheck() )
			m_hatch = CPolyLine::DIAGONAL_FULL;
		else if( m_radio_edge.GetCheck() )
			m_hatch = CPolyLine::DIAGONAL_EDGE;
	}
	else
	{
		if( m_hatch == CPolyLine::NO_HATCH )
			m_radio_none.SetCheck(1);
		else if( m_hatch == CPolyLine::DIAGONAL_FULL )
			m_radio_full.SetCheck(1);
		else if( m_hatch == CPolyLine::DIAGONAL_EDGE )
			m_radio_edge.SetCheck(1);
	}
}


BEGIN_MESSAGE_MAP(CDlgSetAreaHatch, CDialog)
END_MESSAGE_MAP()


// CDlgSetAreaHatch message handlers

void CDlgSetAreaHatch::Init( int hatch )
{
	m_hatch = hatch;
}