// DlgReport.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgReport.h"
#include "FreePcbDoc.h"
#include "Gerber.h"


// CDlgReport dialog

IMPLEMENT_DYNAMIC(CDlgReport, CDialog)
CDlgReport::CDlgReport(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgReport::IDD, pParent) 
{
}

CDlgReport::~CDlgReport()
{
}

void CDlgReport::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHECK_STATS, m_check_board);
	DDX_Control(pDX, IDC_CHECK_DRILL, m_check_drill);
	DDX_Control(pDX, IDC_CHECK_PARTS, m_check_parts);
	DDX_Control(pDX, IDC_CHECK_CAM, m_check_cam);
	DDX_Control(pDX, IDC_CHECK_DRC_PARAMS, m_check_drc_params);
	DDX_Control(pDX, IDC_CHECK_DRC, m_check_drc);
	DDX_Control(pDX, IDC_CHECK_CONNECTIVITY, m_check_connectivity);
	DDX_Control(pDX, IDC_RADIO_INCH, m_radio_inch);
	DDX_Control(pDX, IDC_RADIO_MM, m_radio_mm);
	DDX_Control(pDX, IDC_RADIO_CCW, m_radio_ccw);
	DDX_Control(pDX, IDC_RADIO_TOP, m_radio_top);
	DDX_Radio(pDX, IDC_RADIO_CCW, m_ccw);
	DDX_Radio(pDX, IDC_RADIO_TOP, m_top);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		m_check_board.SetCheck( !(m_flags & NO_PCB_STATS) );
		m_check_drill.SetCheck( !(m_flags & NO_DRILL_LIST) );
		m_check_parts.SetCheck( !(m_flags & NO_PARTS_LIST) );
		m_check_cam.SetCheck( !(m_flags & NO_CAM_PARAMS) );
		m_check_drc_params.SetCheck( !(m_flags & NO_DRC_PARAMS) );
		m_check_drc.SetCheck( m_flags & DRC_LIST );
		m_check_connectivity.SetCheck( m_flags & CONNECTIVITY_LIST );
		if( m_flags & USE_MM )
			m_radio_mm.SetCheck( 1 );
		else
			m_radio_inch.SetCheck( 1 );
	}
	else
	{
		// outgoing
	}
}

void CDlgReport::Initialize( CFreePcbDoc * doc )
{
	m_doc = doc;
	m_pl = doc->m_plist;
	m_nl = doc->m_nlist;
	m_flags = doc->m_report_flags;
	m_ccw = (m_flags & CW)/CW;		// set to 0 to use CCW (default)
	m_top = 1 - (m_flags & TOP)/TOP;	// set to 1 to use bottom (default)
}


BEGIN_MESSAGE_MAP(CDlgReport, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
END_MESSAGE_MAP()


// CDlgReport message handlers

// global helper for qsort(), for quicksort of array of string pointers
//
int mycompare( const void *arg1, const void *arg2 )
{
	CString * str1 = *(CString**)arg1;
	CString * str2 = *(CString**)arg2;
	return str1->CompareNoCase( *str2 );		
}

void CDlgReport::OnBnClickedOk()
{
	CString line, str1, str2, str3, str4, str_units;
	int dp;			// decimal places for dimensions
	m_flags = 0;

	if( m_radio_ccw.GetCheck() == 0 )
		m_flags = m_flags | CW;	// set flag if cw
	if( m_radio_top.GetCheck() )
		m_flags = m_flags | TOP;	// set flag if top
	if( m_radio_mm.GetCheck() )
	{
		m_flags |= USE_MM;
		m_units = MM;
		dp = 3;
		str_units = "MM";
	}
	else
	{
		m_units = MIL;
		dp = 1;
		str_units = "MIL";
	}
	if( !m_check_board.GetCheck() )
		m_flags |= NO_PCB_STATS;
	if( !m_check_drill.GetCheck() )
		m_flags |= NO_DRILL_LIST;
	if( !m_check_parts.GetCheck() )
		m_flags |= NO_PARTS_LIST;
	if( !m_check_cam.GetCheck() )
		m_flags |= NO_CAM_PARAMS;
	if( !m_check_drc_params.GetCheck() )
		m_flags |= NO_DRC_PARAMS;
	if( m_check_drc.GetCheck() )
		m_flags |= DRC_LIST;
	if( m_check_connectivity.GetCheck() )
		m_flags |= CONNECTIVITY_LIST;
	m_doc->m_report_flags = m_flags;
	m_doc->ProjectModified( TRUE );

	CString fn = m_doc->m_path_to_folder + "\\" + "report.txt"; 
	CStdioFile file;
	int ok = file.Open( fn, CFile::modeCreate | CFile::modeWrite ); 
	if( !ok )     
	{ 
		CString mess = "Unable to open file \" " + fn + "\""; 
		AfxMessageBox( mess, MB_OK ); 
		OnCancel();
	}
	file.WriteString( "FreePCB project report (default units = " + str_units + ")\n" );   
	file.WriteString( "============================================\n" );
	file.WriteString( "Project name: " + m_doc->m_name + "\n" );
	file.WriteString( "Project file: " + m_doc->m_pcb_full_path + "\n" );
	file.WriteString( "Default library folder: " + m_doc->m_full_lib_dir + "\n" );
	CMap <int,int,int,int> hole_size_map;

	if( !(m_flags & NO_PCB_STATS) )
	{
		file.WriteString( "\nBoard statistics\n" );
		file.WriteString( "==============\n" );
		line.Format( "Number of copper layers: %d\n", m_doc->m_num_copper_layers );
		file.WriteString( line );
		int nbo = m_doc->GetNumBoards();
		line.Format( "Number of board outlines: %d\n", nbo );
		file.WriteString( line );
		RECT all_board_bounds;
		all_board_bounds.left = INT_MAX;
		all_board_bounds.bottom = INT_MAX;
		all_board_bounds.right = INT_MIN;
		all_board_bounds.top = INT_MIN;
		for( int ib=0; ib<m_doc->m_outline_poly.GetSize(); ib++ )
		{
			id bid = m_doc->m_outline_poly[ib].GetId();
			if( bid.st == ID_BOARD )
			{
				RECT r;
				r = m_doc->m_outline_poly[ib].GetBounds();
				SwellRect( &all_board_bounds, r );
			}
		}
		int x = abs(all_board_bounds.right - all_board_bounds.left);
		int y = abs(all_board_bounds.top - all_board_bounds.bottom);
		::MakeCStringFromDimension( &str1, x, m_units, TRUE, FALSE, TRUE, 3 ); 
		::MakeCStringFromDimension( &str2, y, m_units, TRUE, FALSE, TRUE, 3 );
		file.WriteString( "Board outline(s) size: X = " + str1 + "; Y = " + str2 + "\n" );
	}
	int num_parts = 0;
	int num_parts_with_fp = 0;
	int num_pins = 0;
	int num_th_pins = 0;
	int num_nets = 0;
	int num_vias = 0;
	cpart * part = m_pl->GetFirstPart();
	while( part )
	{
		num_parts++;
		if( part->shape )
		{
			num_parts_with_fp++;
			int npins = part->shape->GetNumPins();
			for( int ip=0; ip<npins; ip++ )
			{
				num_pins++;
				int hole_size = part->shape->m_padstack[ip].hole_size;
				if( hole_size > 0 )
				{
					int num_holes;
					BOOL bFound = hole_size_map.Lookup( hole_size, num_holes );
					if( bFound )
						hole_size_map.SetAt( hole_size, num_holes+1 );
					else
						hole_size_map.SetAt( hole_size, 1 );
					num_th_pins++;
				}
			}
		}
		part = m_pl->GetNextPart( part );
	}
	if( !(m_flags & NO_PCB_STATS) )
	{
		if( num_parts_with_fp != num_parts )
			line.Format( "Number of parts: %d (without footprints = %d)\n",
			num_parts, num_parts-num_parts_with_fp );
		else
			line.Format( "Number of parts: %d\n", num_parts );
		file.WriteString( line );
		line.Format( "Number of pins: %d (%d through-hole, %d SMT)\n", 
			num_pins, num_th_pins, num_pins-num_th_pins );
		file.WriteString( line );
	}
	cnet * net = m_nl->GetFirstNet();
	while( net )
	{
		num_nets++;
		for( int ic=0; ic<net->nconnects; ic++ )
		{
			for( int iv=0; iv<net->connect[ic].vtx.GetSize(); iv++ )
			{
				cvertex * v = &net->connect[ic].vtx[iv];
				int hole_size = v->via_hole_w; 
				if( hole_size > 0 )
				{
					int num_holes;
					BOOL bFound = hole_size_map.Lookup( hole_size, num_holes );
					if( bFound )
						hole_size_map.SetAt( hole_size, num_holes+1 );
					else
						hole_size_map.SetAt( hole_size, 1 );
					num_vias++;
				}
			}
		}
		net = m_nl->GetNextNet(/*LABEL*/);
	}
	if( !(m_flags & NO_PCB_STATS) )
	{
		line.Format( "Number of vias: %d\n", num_vias );
		file.WriteString( line );
		line.Format( "Number of holes: %d\n", num_vias + num_th_pins );
		file.WriteString( line );
	}

	if( !(m_flags & NO_DRILL_LIST) )
	{
		file.WriteString( "\nDrill list\n" );
		file.WriteString( "==========\n" );
		file.WriteString( "NUMBER  DIAMETER\n" );
		file.WriteString( "------  --------\n" );
		POSITION pos = hole_size_map.GetStartPosition();
		int hole_size;
		int num_holes;
		while (pos != NULL)
		{
			hole_size_map.GetNextAssoc( pos, hole_size, num_holes );
			if( m_units == MIL )
			{
				::MakeCStringFromDimension( &str1, hole_size, MIL, TRUE, FALSE, TRUE, 1 );
				line.Format( "%5d   %s\n", num_holes, str1 );
			}
			else
			{
				::MakeCStringFromDimension( &str1, hole_size, MM, TRUE, FALSE, TRUE, 3 );
				::MakeCStringFromDimension( &str2, hole_size, MIL, TRUE, FALSE, TRUE, 1 );
				line.Format( "%5d   %s (%s)\n", num_holes, str1, str2 );
			}
			file.WriteString( line );
		}
	}
	if( !(m_flags & NO_PARTS_LIST) )
	{
		// make array of pointers to ref_des strings, used for sorting
		int nparts = m_pl->GetNumParts();
		CString ** ref_ptr = (CString**)malloc( nparts * sizeof(CString*) );
		cpart * part = m_pl->GetFirstPart();
		int ip = 0;
		while( part )
		{
			ref_ptr[ip] = &part->ref_des;
			ip++;
			part = m_pl->GetNextPart( part );
		}
		// quicksort
		qsort( ref_ptr, nparts, sizeof(CString*), mycompare );
		// make arrays of strings for table
		CArray <CString> ref_des, package, value, footprint, pins, holes; 
		CArray <CString> side, angle, c_x, c_y, p1_x, p1_y;
		CArray< CArray <CString> > dot_w, dot_x, dot_y;
		ref_des.SetSize(nparts);
		package.SetSize(nparts);
		value.SetSize(nparts);
		footprint.SetSize(nparts); 
		pins.SetSize(nparts); 
		holes.SetSize(nparts); 
		side.SetSize(nparts);
		angle.SetSize(nparts);
		c_x.SetSize(nparts);
		c_y.SetSize(nparts);
		p1_x.SetSize(nparts);
		p1_y.SetSize(nparts);
		dot_w.SetSize(nparts);
		dot_x.SetSize(nparts);
		dot_y.SetSize(nparts);
		int maxlen_ref_des = 3;
		int maxlen_package = 7;
		int maxlen_value = 5;
		int maxlen_footprint = 9;
		int maxlen_pins = 4;
		int maxlen_holes = 5;
		int maxlen_side = 6;
		int maxlen_angle = 5;
		int maxlen_y = 5;
		int maxlen_c_x = 6;
		int maxlen_c_y = 6;
		int maxlen_p1_x = 6;
		int maxlen_p1_y = 6;
		int maxnum_dots = 0;
		int maxlen_dot_w = 6;
		int maxlen_dot_x = 6;
		int maxlen_dot_y = 6;
		int dp;
		file.WriteString( "\nPart list\n" );   
		file.WriteString(   "=========\n" );
		CString cw_str = "CCW";
		if( m_flags & CW )
			cw_str = "CW";
		line.Format( "For parts on top: the angle is in degrees %s, as viewed from the top\n", cw_str );
		file.WriteString( line );
		if( m_flags & TOP )
			line.Format( "For parts on the bottom: the angle is in degrees %s, as viewed from the top\n", cw_str );
		else
			line.Format( "For parts on the bottom: the angle is in degrees %s, as viewed from the bottom by flipping the PCB left-right\n", cw_str );
		file.WriteString( line );
		file.WriteString( "All centroid coordinates are as viewed from the top\n\n" );
		if( m_units == MIL )
			dp = 1;
		else if( m_units == MM )
			dp = 3;
		else
			ASSERT(0);
		for( int ip=0; ip<m_pl->GetNumParts(); ip++ )
		{
			part = m_pl->GetPart( *ref_ptr[ip] );			
			if( !part )
				ASSERT(0);
			if( !part->selected )
				continue;
			ref_ptr[ip] = &part->ref_des;
			ref_des[ip] = part->ref_des;
			value[ip] = part->value;
			footprint[ip] = "";
			package[ip] = "";
			pins[ip] = "";
			holes[ip] = "";
			c_x[ip] = "";
			c_y[ip] = "";
			p1_x[ip] = "";
			p1_y[ip] = "";
			angle[ip] = "";
			if( part->shape )  
			{
				package[ip] = part->shape->m_package;
				BOOL bSMT = TRUE;
				int nholes = 0;
				for( int ipin=0; ipin<part->shape->GetNumPins(); ipin++ )
				{
					if( part->shape->m_padstack[ipin].hole_size > 0 )
					{
						bSMT = FALSE;
						nholes++;
					}
				}
				footprint.SetAtGrow( ip, part->shape->m_name );
				str1.Format( "%d", part->shape->GetNumPins() );
				pins[ip] = str1;
				str1.Format( "%d", nholes );
				holes[ip] = str1;
				if( part->side == 0 )
					str1 = "   top";
				else
					str1 = "bottom";
				side[ip] = str1;
				int a = ::GetReportedAngleForPart( part->angle, 
				part->shape->m_centroid_angle, part->side );				
				if( m_flags & CW )
					a = ccw(a);		// degrees cw
				if( !(m_flags & TOP) )
					a = 180 - a;	// viewed from bottom, flipped left-right
				if( a < 0.0 )
					a += 360.0;
				str1.Format( "%d", a );
				angle[ip] = str1;
				CPoint centroid_pt = m_pl->GetCentroidPoint( part );
				::MakeCStringFromDimension( &str1, centroid_pt.x, m_units, FALSE, FALSE, TRUE, dp );
				c_x[ip] = str1;
				::MakeCStringFromDimension( &str1, centroid_pt.y, m_units, FALSE, FALSE, TRUE, dp );
				c_y[ip] = str1;
				int index_p1 = part->shape->GetPinIndexByName( "1", -1 );
				if( index_p1 == -1 )
					index_p1 = part->shape->GetPinIndexByName( "A1", -1 );
				if( index_p1 != -1 )
				{
					CPoint pt1 = m_pl->GetPinPoint( part, index_p1, part->side, part->angle );
					::MakeCStringFromDimension( &str1, pt1.x, m_units, FALSE, FALSE, TRUE, dp );
					p1_x[ip] = str1;
					::MakeCStringFromDimension( &str1, pt1.y, m_units, FALSE, FALSE, TRUE, dp );
					p1_y[ip] = str1;
				}
				int ndots = part->shape->m_glue.GetSize();
				if( ndots == 0 && bSMT )
					ndots = 1;
				maxnum_dots = max( maxnum_dots, ndots );
				CArray< CString > * g_w_str = &dot_w[ip];	// pointer to array of glue widths
				CArray< CString > * g_x_str = &dot_x[ip];	// pointer to array of glue x
				CArray< CString > * g_y_str = &dot_y[ip];	// pointer to array of glue y
				g_w_str->SetSize( ndots );
				g_x_str->SetSize( ndots );
				g_y_str->SetSize( ndots );
				for( int idot=0; idot<ndots; idot++ ) 
				{
					int g_w;
					CPoint g_pt;
					if( part->shape->m_glue.GetSize() == 0 )
					{
						g_w = m_doc->m_default_glue_w;
						g_pt = m_pl->GetCentroidPoint( part );
					}
					else
					{
						g_w = part->shape->m_glue[idot].w;
						if( g_w == 0 )
							g_w = m_doc->m_default_glue_w;
						g_pt = m_pl->GetGluePoint( part, idot );
					}
					CString temp;
					::MakeCStringFromDimension( &temp, g_w, m_units, FALSE, FALSE, TRUE, dp );
					(*g_w_str)[idot] = temp;
					maxlen_dot_w = max( maxlen_dot_w, temp.GetLength() );
					::MakeCStringFromDimension( &temp, g_pt.x, m_units, FALSE, FALSE, TRUE, dp );
					(*g_x_str)[idot] = temp;
					maxlen_dot_x = max( maxlen_dot_x, temp.GetLength() );
					::MakeCStringFromDimension( &temp, g_pt.y, m_units, FALSE, FALSE, TRUE, dp );
					(*g_y_str)[idot] = temp;
					maxlen_dot_y = max( maxlen_dot_y, temp.GetLength() );
				}
			}
			maxlen_ref_des = max( maxlen_ref_des, ref_des[ip].GetLength() );
			maxlen_package = max( maxlen_package, package[ip].GetLength() );
			maxlen_value = max( maxlen_value, value[ip].GetLength() );
			maxlen_footprint = max( maxlen_footprint, footprint[ip].GetLength() );
			maxlen_pins = max( maxlen_pins, pins[ip].GetLength() );
			maxlen_holes = max( maxlen_holes, holes[ip].GetLength() );
			maxlen_c_x = max( maxlen_c_x, c_x[ip].GetLength() );
			maxlen_c_y = max( maxlen_c_y, c_y[ip].GetLength() );
			maxlen_p1_x = max( maxlen_p1_x, p1_x[ip].GetLength() );
			maxlen_p1_y = max( maxlen_p1_y, p1_y[ip].GetLength() );
		}
		CString format_str;
		format_str.Format( "%%%ds  %%%ds  %%%ds  %%%ds  %%%ds  %%%ds  %%%ds  %%%ds  %%%ds  %%%ds  %%%ds  %%%ds", 
			maxlen_ref_des, maxlen_package, maxlen_value, maxlen_footprint, maxlen_pins, maxlen_holes,
			maxlen_side, maxlen_angle, 
			maxlen_c_x, maxlen_c_y, maxlen_p1_x, maxlen_p1_y );
		CString dot_format_str;
		dot_format_str.Format( "  %%%ds  %%%ds  %%%ds", 
				maxlen_dot_w, maxlen_dot_x, maxlen_dot_y );
		str1.Format( format_str, "REF", "PACKAGE", "VALUE", "FOOTPRINT", "PINS", "HOLES",
			"SIDE", "ANGLE", "CENT-X", "CENT-Y", "PIN1-X", "PIN1-Y" );
		for( int id=0; id<maxnum_dots; id++ ) 
		{
			CString temp;
			temp.Format( dot_format_str, "GLUE-W", "GLUE-X", "GLUE-Y" );
			str1 += temp;
		}
		file.WriteString( str1 + "\n" );
		str1.Format( format_str, "---", "-------", "-----", "---------", "----", "-----",
			"----", "-----", "------", "------", "------", "------" );
		for( int id=0; id<maxnum_dots; id++ )
		{
			CString temp;
			temp.Format( dot_format_str, "------", "------", "------" );
			str1 += temp;
		}
		file.WriteString( str1 + "\n" );
		int bREAL = 0;
		for( int ip=0; ip<ref_des.GetSize(); ip++ )
		{
			CString pad_ref_des;
			str1.Format( format_str, ref_des[ip], package[ip], value[ip], footprint[ip],
				pins[ip], holes[ip], side[ip], angle[ip], c_x[ip], c_y[ip], p1_x[ip], p1_y[ip] );
			CArray< CString > * g_w_str = &dot_w[ip];
			for( int id=0; id<g_w_str->GetSize(); id++ )
			{
				CString temp;
				temp.Format( dot_format_str, dot_w[ip][id], dot_x[ip][id], dot_y[ip][id] );
				str1 += temp;
			}
			str1 = str1.TrimRight();
			if( str1.GetLength() )
			{
				bREAL = 1;
				file.WriteString( str1 + "\n" );
			}
		}
		if(!bREAL)
			file.WriteString( "No selected parts\n" );
	}
	if( !(m_flags & NO_CAM_PARAMS) )  
	{
		file.WriteString( "\nGerber file settings\n" );
		file.WriteString( "====================\n" );
		CString str_SMT_connect = "NO";
		CString str_board_outline = "NO";
		CString str_moires = "NO";
		CString str_layer_desc_text = "NO";
		CString str_pilot_holes = "NO";
		CString str_SMT_thermals = "YES";
		CString str_pin_thermals = "YES";
		CString str_via_thermals = "YES";
		CString str_SM_cutouts_vias = "NO";
		CString str_90_degree_thermals = "NO";
		if( m_doc->m_cam_flags & GERBER_BOARD_OUTLINE )
			str_board_outline = "YES";
		if( m_doc->m_cam_flags & GERBER_AUTO_MOIRES )
			str_moires = "YES";
		if( m_doc->m_cam_flags & GERBER_LAYER_TEXT )
			str_layer_desc_text = "YES";
		if( m_doc->m_cam_flags & GERBER_PILOT_HOLES )
			str_pilot_holes = "YES";
		if( m_doc->m_bSMT_copper_connect )
			str_SMT_connect = "YES";
		if( m_doc->m_cam_flags & GERBER_NO_SMT_THERMALS )
			str_SMT_thermals = "NO";
		if( m_doc->m_cam_flags & GERBER_NO_PIN_THERMALS )
			str_pin_thermals = "NO";
		if( m_doc->m_cam_flags & GERBER_NO_VIA_THERMALS )
			str_via_thermals = "NO";
		if( m_doc->m_cam_flags & GERBER_MASK_VIAS )
			str_SM_cutouts_vias = "YES";
		if( m_doc->m_cam_flags & GERBER_90_THERMALS )
			str_90_degree_thermals = "YES";
		file.WriteString( "Include board outline: " + str_board_outline + "\n" );
		file.WriteString( "Include moires: " + str_moires + "\n" );
		file.WriteString( "Include layer description text: " + str_layer_desc_text + "\n" );
		file.WriteString( "Include pilot holes: " + str_pilot_holes + "\n" );
		file.WriteString( "Connect SMT pins to copper areas: " + str_SMT_connect + "\n" );
		file.WriteString( "Use thermals for SMT pins: " + str_SMT_thermals + "\n" );
		file.WriteString( "Use thermals for through-hole pins: " + str_pin_thermals + "\n" );
		file.WriteString( "Use thermals for vias: " + str_via_thermals + "\n" );
		file.WriteString( "Make solder-mask cutouts for vias: " + str_SM_cutouts_vias + "\n" );
		file.WriteString( "Use 90-degree thermals for round pins: " + str_90_degree_thermals + "\n" );
		CString str_fill_clearance;
		CString str_mask_clearance;
		CString str_paste_shrink;
		CString str_thermal_width;
		CString str_min_silkscreen_stroke_wid;
		CString str_high_wid;
		CString str_pilot_diameter;
		CString str_hole_clearance;
		CString str_cam_flags;
		CString str_cam_layers;
		CString str_cam_drill_file;
		CString str_annular_ring_pins;
		CString str_annular_ring_vias;
		CString str_n_x; 
		CString str_n_y; 
		CString str_space_x; 
		CString str_space_y;
		::MakeCStringFromDimension( &str_fill_clearance, m_doc->m_fill_clearance,
			m_units, TRUE, FALSE, TRUE, dp );
		::MakeCStringFromDimension( &str_mask_clearance, m_doc->m_mask_clearance,
			m_units, TRUE, FALSE, TRUE, dp );
		::MakeCStringFromDimension( &str_paste_shrink, m_doc->m_paste_shrink,
			m_units, TRUE, FALSE, TRUE, dp );
		::MakeCStringFromDimension( &str_thermal_width, m_doc->m_thermal_width,
			m_units, TRUE, FALSE, TRUE, dp );
		::MakeCStringFromDimension( &str_min_silkscreen_stroke_wid, 
			m_doc->m_min_silkscreen_stroke_wid, m_units, TRUE, FALSE, TRUE, dp );
		::MakeCStringFromDimension( &str_high_wid, 
			m_doc->m_highlight_wid, m_units, TRUE, FALSE, TRUE, dp );
		::MakeCStringFromDimension( &str_pilot_diameter, m_doc->m_pilot_diameter,
			m_units, TRUE, FALSE, TRUE, dp );
		::MakeCStringFromDimension( &str_hole_clearance, m_doc->m_hole_clearance,
			m_units, TRUE, FALSE, TRUE, dp );
		::MakeCStringFromDimension( &str_annular_ring_pins, m_doc->m_annular_ring_pins,
			m_units, TRUE, FALSE, TRUE, dp );
		::MakeCStringFromDimension( &str_annular_ring_vias, m_doc->m_annular_ring_vias,
			m_units, TRUE, FALSE, TRUE, dp );
		file.WriteString( "Copper to copper-fill clearance: " + str_fill_clearance + "\n" );
		file.WriteString( "Hole-edge to copper-fill clearance: " + str_hole_clearance + "\n" );
		file.WriteString( "Solder mask clearance: " + str_mask_clearance + "\n" );
		file.WriteString( "Pilot hole diameter: " + str_pilot_diameter + "\n" );
		file.WriteString( "Minimum silkscreen line width: " + str_min_silkscreen_stroke_wid + "\n" );
		file.WriteString( "Thermal relief line width: " + str_thermal_width + "\n" );
		file.WriteString( "Minimum annular ring width (pins): " + str_annular_ring_pins + "\n" );
		file.WriteString( "Minimum annular ring width (vias): " + str_annular_ring_vias + "\n" );
		file.WriteString( "Amount to shrink SMT pads for paste mask: " + str_paste_shrink + "\n" );
	}
	if( !(m_flags & NO_DRC_PARAMS) )  
	{
		file.WriteString( "\nDRC settings\n" );
		file.WriteString( "============\n" );
		CString str_min_trace_width;
		::MakeCStringFromDimension( &str_min_trace_width, m_doc->m_dr.trace_width,
			m_units, TRUE, FALSE, TRUE, dp ); 
		file.WriteString( "Minimum trace width: " + str_min_trace_width + "\n" );
		CString str_min_pad_pad;
		::MakeCStringFromDimension( &str_min_pad_pad, m_doc->m_dr.pad_pad,
			m_units, TRUE, FALSE, TRUE, dp ); 
		file.WriteString( "Minimum pad to pad clearance(between parts): " + str_min_pad_pad + "\n" );
		CString str_min_pad_trace;
		::MakeCStringFromDimension( &str_min_pad_trace, m_doc->m_dr.pad_trace,
			m_units, TRUE, FALSE, TRUE, dp ); 
		file.WriteString( "Minimum pad to trace clearance: " + str_min_pad_trace + "\n" );
		CString str_min_trace_trace;
		::MakeCStringFromDimension( &str_min_trace_trace, m_doc->m_dr.trace_trace,
			m_units, TRUE, FALSE, TRUE, dp ); 
		file.WriteString( "Minimum trace to trace clearance: " + str_min_trace_trace + "\n" );
		CString str_min_hole_copper;
		::MakeCStringFromDimension( &str_min_hole_copper, m_doc->m_dr.hole_copper,
			m_units, TRUE, FALSE, TRUE, dp ); 
		file.WriteString( "Minimum hole to copper clearance: " + str_min_hole_copper + "\n" );
		CString str_min_hole_hole;
		::MakeCStringFromDimension( &str_min_hole_hole, m_doc->m_dr.hole_hole,
			m_units, TRUE, FALSE, TRUE, dp ); 
		file.WriteString( "Minimum hole to hole clearance: " + str_min_hole_hole + "\n" );
		CString str_min_annular_ring_pins;
		::MakeCStringFromDimension( &str_min_annular_ring_pins, m_doc->m_dr.annular_ring_pins,
			m_units, TRUE, FALSE, TRUE, dp ); 
		file.WriteString( "Minimum annular ring (pins): " + str_min_annular_ring_pins + "\n" );
		CString str_min_annular_ring_vias;
		::MakeCStringFromDimension( &str_min_annular_ring_vias, m_doc->m_dr.annular_ring_vias,
			m_units, TRUE, FALSE, TRUE, dp ); 
		file.WriteString( "Minimum annular ring (vias): " + str_min_annular_ring_vias + "\n" );
		CString str_min_board_edge_copper;
		::MakeCStringFromDimension( &str_min_board_edge_copper, m_doc->m_dr.board_edge_copper,
			m_units, TRUE, FALSE, TRUE, dp ); 
		file.WriteString( "Minimum board edge to copper clearance: " + str_min_board_edge_copper + "\n" );
		CString str_min_board_edge_hole;
		::MakeCStringFromDimension( &str_min_board_edge_hole, m_doc->m_dr.board_edge_hole,
			m_units, TRUE, FALSE, TRUE, dp ); 
		file.WriteString( "Minimum board edge to hole clearance: " + str_min_board_edge_hole + "\n" );
		CString str_min_copper_copper;
		::MakeCStringFromDimension( &str_min_copper_copper, m_doc->m_dr.copper_copper,
			m_units, TRUE, FALSE, TRUE, dp ); 
		file.WriteString( "Minimum copper area to copper area clearance: " + str_min_copper_copper + "\n" );
	}
	if( m_flags & (DRC_LIST | CONNECTIVITY_LIST) )
	{
		BOOL bDrc = m_flags & DRC_LIST;
		BOOL bCon = m_flags & CONNECTIVITY_LIST;
		BOOL bConHeading = FALSE;
		DesignRules dr = m_doc->m_dr;
		dr.bCheckUnrouted = bCon;
		m_doc->m_drelist->Clear();
		m_doc->DRC();
		if( bDrc )
		{
			file.WriteString( "\nDRC errors\n" );
			file.WriteString( "==========\n" );
		}
		else
		{
		}
		POSITION pos;
		for( pos = m_doc->m_drelist->list.GetHeadPosition(); pos != NULL; )
		{
			DRError * dre = (DRError*)m_doc->m_drelist->list.GetNext( pos );
			CString str = dre->str;
			BOOL bConError = (str.Find( "routed connection from" ) != -1);
			if( bConError && bCon && !bConHeading )
			{
				file.WriteString( "\nConnectivity errors\n" );
				file.WriteString( "===================\n" );
				bConHeading = TRUE;
			}
			if( bCon && bConError || bDrc && !bConError )
			{
				int colon = str.Find( ":" ); 
				if( colon > -1 )
					str = str.Right( str.GetLength() - colon - 2 );
				file.WriteString( str ); 
			}
		}
		m_doc->m_drelist->Clear();
	}
	file.Close();
	OnOK();
}

void CDlgReport::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	OnCancel();
}
