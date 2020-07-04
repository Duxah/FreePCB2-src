// DlgPadFlags.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgPadFlags.h"
#include "Shape.h"


// CDlgPadFlags dialog

IMPLEMENT_DYNAMIC(CDlgPadFlags, CDialog)
CDlgPadFlags::CDlgPadFlags(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgPadFlags::IDD, pParent)
{
}

CDlgPadFlags::~CDlgPadFlags()
{
}

void CDlgPadFlags::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHECK_NO_AREA, m_check_area_no_connect);
	DDX_Control(pDX, IDC_CHECK_AREA_THERMAL, m_check_area_thermal);
	DDX_Control(pDX, IDC_CHECK_AREA, m_check_area_no_thermal);
	DDX_Control(pDX, IDC_CHECK_NO_MASK, m_check_no_mask);
	if( pDX->m_bSaveAndValidate )
	{
		// outgoing
		m_flags.mask = 0;
		if( m_check_no_mask.GetCheck() )
			m_flags.mask = 1;
		m_flags.area = 0;
		if( m_check_area_no_connect.GetCheck() )
			m_flags.area = PAD_AREA_NEVER;
		else if( m_check_area_no_thermal.GetCheck() )
			m_flags.area = PAD_AREA_CONNECT_NO_THERMAL;
		else if( m_check_area_thermal.GetCheck() )
			m_flags.area = PAD_AREA_CONNECT_THERMAL;
	}
	else
	{
		// incoming
		m_check_no_mask.SetCheck( m_flags.mask == PAD_MASK_NONE );
		m_check_area_no_connect.SetCheck( m_flags.area == PAD_AREA_NEVER );
		m_check_area_no_thermal.SetCheck( m_flags.area == PAD_AREA_CONNECT_NO_THERMAL );
		m_check_area_thermal.SetCheck( m_flags.area == PAD_AREA_CONNECT_THERMAL );
	}
}


BEGIN_MESSAGE_MAP(CDlgPadFlags, CDialog)
	ON_BN_CLICKED(IDC_CHECK_NO_AREA, OnBnClickedCheckNoArea)
	ON_BN_CLICKED(IDC_CHECK_AREA_THERMAL, OnBnClickedCheckAreaThermal)
	ON_BN_CLICKED(IDC_CHECK_AREA, OnBnClickedCheckArea)
END_MESSAGE_MAP()


// CDlgPadFlags message handlers

void CDlgPadFlags::Initialize( flag flags )
{
	m_flags = flags;
}

void CDlgPadFlags::OnBnClickedCheckNoArea()
{
	if( m_check_area_no_connect.GetCheck() == 1 )
	{
		m_check_area_no_thermal.SetCheck( 0 );
		m_check_area_thermal.SetCheck( 0 );
	}
}

void CDlgPadFlags::OnBnClickedCheckAreaThermal()
{
	if( m_check_area_thermal.GetCheck() == 1 )
	{
		m_check_area_no_connect.SetCheck( 0 );
		m_check_area_no_thermal.SetCheck( 0 );
	}
}

void CDlgPadFlags::OnBnClickedCheckArea()
{
	if( m_check_area_no_thermal.GetCheck() == 1 )
	{
		m_check_area_no_connect.SetCheck( 0 );
		m_check_area_thermal.SetCheck( 0 );
	}
}
