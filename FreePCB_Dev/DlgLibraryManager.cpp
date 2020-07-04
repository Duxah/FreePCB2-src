// DlgLibraryManager.cpp : implementation file
//

#include "stdafx.h"
#include <math.h>
#include "FreePcb.h"
#include "DlgLibraryManager.h"
#include "cpdflib.h"
#include "DlgLog.h"
#include "PathDialog.h"


// CDlgLibraryManager dialog

IMPLEMENT_DYNAMIC(CDlgLibraryManager, CDialog)
CDlgLibraryManager::CDlgLibraryManager(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgLibraryManager::IDD, pParent)
{
}

CDlgLibraryManager::~CDlgLibraryManager()
{
}

void CDlgLibraryManager::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_MGR_FOLDER, m_edit_footlib);
	DDX_Control(pDX, IDC_COMBO_MGR_FILE, m_combo_libfile);
	DDX_Control(pDX, IDC_COMBO_PAGE_SIZE, m_combo_page_size);
	DDX_Control(pDX, IDC_COMBO_UNITS, m_combo_units);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		CString folder_path = *m_foldermap->GetLastFolder();
		m_footlib = m_foldermap->GetFolder( &folder_path, m_dlg_log );
		m_edit_footlib.SetWindowText( *m_footlib->GetFullPath() ); 
		for( int i=0; i<m_footlib->GetNumLibs(); i++ )
			m_combo_libfile.InsertString( i, *m_footlib->GetLibraryFullPath(i) );
		m_combo_libfile.InsertString( m_footlib->GetNumLibs(), "*** all library files ***" );
		m_combo_libfile.SetCurSel( 0 );

		m_combo_page_size.InsertString( PG_LETTER, "letter  (8.5 x 11.0 in)" );
		m_combo_page_size.InsertString( PG_A4, "A4  (210 x 297 mm)" );
		m_combo_page_size.SetCurSel( m_page_sel );

		m_combo_units.InsertString( U_NATIVE, "native for footprint" );
		m_combo_units.InsertString( U_MM, "mm" );
		m_combo_units.InsertString( U_MIL, "mils" );
		m_combo_units.InsertString( U_MM_MIL, "mm and mils" );
		m_combo_units.SetCurSel( m_units_sel );
	}
}


BEGIN_MESSAGE_MAP(CDlgLibraryManager, CDialog)
	ON_BN_CLICKED(IDC_BUTTON_MAKE_PDF, OnBnClickedButtonMakePdf)
	ON_BN_CLICKED(IDC_BUTTON_MGR_BROWSE, OnBnClickedButtonMgrBrowse)
END_MESSAGE_MAP()


// CDlgLibraryManager message handlers

void CDlgLibraryManager::OnBnClickedButtonMakePdf()
{
#if 0
	//** temporarily disabled
	AfxMessageBox( "Sorry, this function is currently disabled" );
	return;
#endif
//#if 0
	// set page size
	double PageWidth;	
	double PageHeight;	
	m_page_sel = m_combo_page_size.GetCurSel();
	switch( m_page_sel )
	{
		case PG_LETTER: PageWidth = 8.5; PageHeight = 11.5; break;			// letter size
		case PG_A4: PageWidth = 210.0/25.4; PageHeight = 297.0/25.4; break;	// A4
		default: ASSERT(0); break;											// illegal
	}

	// set format for dimensional units
	int format;
	m_units_sel = m_combo_units.GetCurSel();
	switch( m_units_sel )
	{
		case U_NATIVE: format = NATIVE; break;		// use native units for each footprint
		case U_MM: format = MM; break;				// use mm
		case U_MIL: format = MIL; break;			// use mils
		case U_MM_MIL: format = MM_MIL; break;		// use both mm and mils
		default: ASSERT(0); break;					// illegal
	}

	// magic constant for converting ellipses to Bezier curves
	const double MagicBezier = 0.2761423749154;

	// formatting constants
	const double LeftMargin = 0.75;			// left margin width
	const double RightMargin = 0.75;		// right margin width
	const double TopMargin = 0.75;			// top margin height
	const double BottomMargin = 0.75;		// bottom margin height
	const double TitleHeightPts = 12.0;		// size of title font
	const double TitleAboveLine = 0.1;		// spacing of title above header line
	const double PageTitleLineY = PageHeight - TopMargin - TitleHeightPts/72.0 - TitleAboveLine; // height of header line
	const double NameHeightPts = 13.0;		// size of footprint name font
	const double NameLeadingPts = 30.0;		// line spacing of footprint name font
	const double TextHeightPts = 10.0;		// size of font for general text
	const double TextLeadingPts = 14.0;		// line spacing of general text font
	const double TableTextHeightPts = 10.0;		// size of font for table text
	const double TableTextLeadingPts = 14.0;	// line spacing of table text
	const double TableTextOffsetY = 0.05;		// spacing of table text above bottom cell border
	const double TableTypeX = LeftMargin + 0.7;	// position of border between "PIN NUM" and "TYPE"
	const double TableShapeX = TableTypeX + 0.7;// position of border between "TYPE and "SHAPE"
	const double TableSizeX = TableShapeX + 0.7;// position of border between "SHAPE and "SIZE"
	const double TableHoleX = TableSizeX + 1.4;	// position of border between "SIZE and "HOLE"
	const double TableEndX = TableHoleX + 0.7;	// position of right border of table
	const double FootprintSpaceY = 0.2;			// spacing between scale text and footprint drawing

	int ilib = m_combo_libfile.GetCurSel();

	int first_ilib = ilib;
	int last_ilib = ilib;

	// pop up log dialog
	m_dlg_log->ShowWindow( SW_SHOW );
	m_dlg_log->UpdateWindow();
	m_dlg_log->BringWindowToTop();
	m_dlg_log->Clear();
	m_dlg_log->UpdateWindow();

	if( ilib == m_footlib->GetNumLibs() )
	{
		// do all libraries
		first_ilib = 0;
		last_ilib = ilib - 1;
	}

	CPDFdoc *pdf;
	float y;

	for( ilib=first_ilib; ilib<=last_ilib; ilib++ )
	{
		// make PDF file
		CString title_str = *m_footlib->GetLibraryFullPath( ilib );
		title_str = title_str.Left( title_str.GetLength() - 4 );
		title_str = title_str + ".pdf";

		// post message to log, if it exists
		if( m_dlg_log )
		{
			CString log_message;
			log_message.Format( "Creating file: \"%s\"\r\n", title_str );
			m_dlg_log->AddLine( log_message );
		}

		/* == Initialization == */
		pdf = cpdf_open(0, NULL);
		cpdf_init(pdf);
		int pagenum = 1;
		int n_footprints = 6;
		BOOL new_page = TRUE;
		y = PageTitleLineY;
		// init first page
		if( m_page_sel == PG_LETTER )
			cpdf_pageInit(pdf, pagenum, PORTRAIT, LETTER, LETTER);		/* page orientation */
		else if( m_page_sel == PG_A4 )
			cpdf_pageInit(pdf, pagenum, PORTRAIT, A4, A4);		/* page orientation */
		BOOL new_page_init = TRUE;

		// now loop through all headings
		for( int i=0; i<m_footlib->GetNumFootprints(ilib) ; i++ )
		{
			// get next footprint
			CShape foot;
			int err = foot.MakeFromFile( NULL, *m_footlib->GetFootprintName( ilib, i ), 
				*m_footlib->GetLibraryFullPath(ilib), m_footlib->GetFootprintOffset( ilib, i ) );
			if( err )
				ASSERT(0);
			// make array of padstack info
			int num_padinfo = 0;
			CArray<int> start_num;
			CArray<int> num_same;
			CArray<int> pad_shape;
			CArray<int> pad_w;
			CArray<int> pad_l;
			CArray<int> pad_hole;
			for( int ip=0; ip<foot.m_padstack.GetSize(); ip++ )
			{
				int this_pad_shape = foot.m_padstack[ip].top.shape;
				int this_pad_w = foot.m_padstack[ip].top.size_Y;
				int this_pad_l = 0;
				if( this_pad_shape == PAD_RECT || this_pad_shape == PAD_RRECT || this_pad_shape == PAD_OVAL )
					this_pad_l = foot.m_padstack[ip].top.size_X;
				int this_pad_hole = foot.m_padstack[ip].hole_size;
				// now see if we need a new line in the pad table
				BOOL new_pad_line = FALSE;
				if( num_padinfo == 0 )
				{
					new_pad_line = TRUE;
				}
				else
				{
					if(	   this_pad_shape != pad_shape[num_padinfo-1]
					|| this_pad_w != pad_w[num_padinfo-1]
					|| this_pad_l != pad_l[num_padinfo-1]
					|| this_pad_hole != pad_hole[num_padinfo-1] )
						new_pad_line = TRUE;
				}
				if( new_pad_line && num_padinfo )
				{
					// finish last info line
					int the_num_same = ip - start_num[num_padinfo-1] + 1;
					num_same.Add( the_num_same );
				}
				if( new_pad_line )
				{
					// start new info line
					start_num.Add( ip+1 );
					pad_shape.Add( this_pad_shape );
					pad_w.Add( this_pad_w );
					pad_l.Add( this_pad_l );
					pad_hole.Add( this_pad_hole );
					num_padinfo = pad_shape.GetSize();
				}
				if( ip == (foot.m_padstack.GetSize()-1) )
				{
					// last pin, finish last info line
					int the_num_same = ip - start_num[num_padinfo-1] + 2;
					num_same.Add( the_num_same );
				}
			}
			// now remove any lines for non-existent pads (i.e. pad shape = PAD_NONE and hole_size = 0)
			for( int i=num_same.GetSize()-1; i>=0; i-- )
			{
				if( pad_shape[i] == PAD_NONE && pad_hole[i] == 0 )
				{
					start_num.RemoveAt(i);
					num_same.RemoveAt(i);
					pad_shape.RemoveAt(i);
					pad_w.RemoveAt(i);
					pad_l.RemoveAt(i);
					pad_hole.RemoveAt(i);
				}
			}
			num_padinfo = pad_shape.GetSize();


			// now split author, source and description strings into 2 lines if needed
			CString author_1;
			CString author_2;
			CString source_1;
			CString source_2;
			CString desc_1;
			CString desc_2;

			cpdf_setFont(pdf, "Times-Italic", "MacRomanEncoding", TextHeightPts);
			double start_x_rel = cpdf_stringWidth( pdf, (const unsigned char*)(LPCSTR)"Author: " )/72.0;
			double max_len = TableEndX - LeftMargin - start_x_rel;
			cpdf_setFont(pdf, "Times-Roman", "MacRomanEncoding", TextHeightPts);
			double str_len = cpdf_stringWidth( pdf, (const unsigned char*)(LPCSTR)foot.m_author )/72.0;
			if( str_len > max_len )
			{
				// text string too long, need to break it up at a space
				int isp = 0;
				int last_isp;
				while( max_len > cpdf_stringWidth( pdf, (const unsigned char*)(LPCSTR)foot.m_author.Left(isp) )/72.0
					&& isp != -1 ) 
				{
					last_isp = isp+1;
					isp = foot.m_author.Find( " ", last_isp );
				}
				author_1 = foot.m_author.Left( last_isp-1 );
				author_2 = foot.m_author.Right( foot.m_author.GetLength() - last_isp );
			}
			else
			{
				author_1 = foot.m_author;
				author_2 = "";
			}

			cpdf_setFont(pdf, "Times-Italic", "MacRomanEncoding", TextHeightPts);
			start_x_rel = cpdf_stringWidth( pdf, (const unsigned char*)(LPCSTR)"Package: " )/72.0;
			max_len = TableEndX - LeftMargin - start_x_rel;
			cpdf_setFont(pdf, "Times-Roman", "MacRomanEncoding", TextHeightPts);
			str_len = cpdf_stringWidth( pdf, (const unsigned char*)(LPCSTR)foot.m_package )/72.0;
			if( str_len > max_len )
			{
				// text string too long, need to break it up at a space
				int isp = 0;
				int last_isp;
				while( max_len > cpdf_stringWidth( pdf, (const unsigned char*)(LPCSTR)foot.m_package.Left(isp) )/72.0
					&& isp != -1 ) 
				{
					last_isp = isp+1;
					isp = foot.m_package.Find( " ", last_isp );
				}
				source_1 = foot.m_package.Left( last_isp-1 );
				source_2 = foot.m_package.Right( foot.m_package.GetLength() - last_isp );
			}
			else
			{
				source_1 = foot.m_package;
				source_2 = "";
			}

			cpdf_setFont(pdf, "Times-Italic", "MacRomanEncoding", TextHeightPts);
			start_x_rel = cpdf_stringWidth( pdf, (const unsigned char*)(LPCSTR)"Description: " )/72.0;
			max_len = TableEndX - LeftMargin - start_x_rel;
			cpdf_setFont(pdf, "Times-Roman", "MacRomanEncoding", TextHeightPts);
			str_len = cpdf_stringWidth( pdf, (const unsigned char*)(LPCSTR)foot.m_desc )/72.0;
			if( str_len > max_len )
			{
				// text string too long, need to break it up at a space
				int isp = 0;
				int last_isp;
				while( max_len > cpdf_stringWidth( pdf, (const unsigned char*)(LPCSTR)foot.m_desc.Left(isp) )/72.0
					&& isp != -1 ) 
				{
					last_isp = isp+1;
					isp = foot.m_desc.Find( " ", last_isp );
				}
				desc_1 = foot.m_desc.Left( last_isp-1 );
				desc_2 = foot.m_desc.Right( foot.m_desc.GetLength() - last_isp );
			}
			else
			{
				desc_1 = foot.m_desc;
				desc_2 = "";
			}

			// get height rquired for footprint info and see if we need a new page
			int npadlines = num_padinfo;
			float foot_height = NameLeadingPts/72.0 + (2+npadlines)*TableTextLeadingPts/72.0
				+ 2*TextLeadingPts/72.0;
			if( author_1.GetLength() )
				foot_height += TextLeadingPts/72.0;
			if( author_2.GetLength() )
				foot_height += TextLeadingPts/72.0;
			if( source_1.GetLength() )
				foot_height += TextLeadingPts/72.0;
			if( source_2.GetLength() )
				foot_height += TextLeadingPts/72.0;
			if( desc_1.GetLength() )
				foot_height += TextLeadingPts/72.0;
			if( desc_2.GetLength() )
				foot_height += TextLeadingPts/72.0;

			if( (y - foot_height) < BottomMargin )
			{
				// new page
				pagenum++;
				new_page = TRUE;
				new_page_init = FALSE;
			}

			if( new_page )
			{
				// new page
				if( !new_page_init )
				{
					if( m_page_sel == PG_LETTER )
						cpdf_pageInit(pdf, pagenum, PORTRAIT, LETTER, LETTER);		
					else if( m_page_sel == PG_A4 )
						cpdf_pageInit(pdf, pagenum, PORTRAIT, A4, A4);		
					new_page_init = TRUE;
				}
				CString pagenum_str;
				pagenum_str.Format( "page %d", pagenum );

				// draw header
				cpdf_setlinecap( pdf, 1 );	// round end-caps
				cpdf_setrgbcolorStroke(pdf, 0.0, 0.0, 0.0);	/* black as stroke color */
				cpdf_setgrayFill(pdf, 0.0);					/* Black */
				cpdf_moveto( pdf, LeftMargin, PageTitleLineY );
				cpdf_lineto( pdf, PageWidth-RightMargin, PageTitleLineY );
				cpdf_stroke( pdf );
				cpdf_beginText(pdf, 0);
				cpdf_setFont(pdf, "Times-Italic", "MacRomanEncoding", TitleHeightPts);
				cpdf_text(pdf, LeftMargin, PageTitleLineY + TitleAboveLine, 0.0, title_str);
				cpdf_textAligned( pdf, PageWidth-RightMargin, PageTitleLineY + TitleAboveLine,
					0.0, TEXTPOS_LR, pagenum_str );
				cpdf_endText(pdf);
				y = PageTitleLineY;
				new_page = FALSE;
			}

			// get bounds for drawing footprint later
			float top_y = y - NameLeadingPts/72.0 - FootprintSpaceY;
			float right_x = PageWidth - RightMargin;

			// draw footprint info
			// decide on units to use (if format == MM_MIL, use both mm and mil)
			int units;
			units = foot.m_units;		// native units for footprint
			if( units == NM )
				units = MM;
			if( format == MM )
				units = MM;				// use mm
			else if( format == MIL )
				units = MIL;			// use mil
			// draw footprint name
			cpdf_beginText(pdf, 0);
			cpdf_setTextPosition( pdf, LeftMargin, y );
			cpdf_setTextLeading( pdf, NameLeadingPts );
			cpdf_setFont(pdf, "Times-Bold", "MacRomanEncoding", NameHeightPts);
			cpdf_textCRLFshow(pdf, foot.m_name );
			y -= NameLeadingPts/72.0;
			cpdf_setTextLeading( pdf, TextLeadingPts );
			if( author_1.GetLength() )
			{
				cpdf_setFont(pdf, "Times-Italic", "MacRomanEncoding", TextHeightPts);
				cpdf_textCRLFshow(pdf, "Author: " );
				cpdf_setFont(pdf, "Times-Roman", "MacRomanEncoding", TextHeightPts);
				cpdf_textShow(pdf, author_1 );
				y -= TextLeadingPts/72.0;
				if( author_2.GetLength() )
				{
					cpdf_textCRLFshow(pdf, author_2 );
					y -= TextLeadingPts/72.0;
				}
			}
			if( source_1.GetLength() )
			{
				cpdf_setFont(pdf, "Times-Italic", "MacRomanEncoding", TextHeightPts);
				cpdf_textCRLFshow(pdf, "Package: " );
				cpdf_setFont(pdf, "Times-Roman", "MacRomanEncoding", TextHeightPts);
				cpdf_textShow(pdf, source_1 );
				y -= TextLeadingPts/72.0;
				if( source_2.GetLength() )
				{
					cpdf_textCRLFshow(pdf, source_2 );
					y -= TextLeadingPts/72.0;
				}
			}
			if( desc_1.GetLength() )
			{
				cpdf_setFont(pdf, "Times-Italic", "MacRomanEncoding", TextHeightPts);
				cpdf_textCRLFshow(pdf, "Description: " );
				cpdf_setFont(pdf, "Times-Roman", "MacRomanEncoding", TextHeightPts);
				cpdf_textShow(pdf, desc_1 );
				y -= TextLeadingPts/72.0;
				if( desc_2.GetLength() )
				{
					cpdf_textCRLFshow(pdf, desc_2 );
					y -= TextLeadingPts/72.0;
				}
			}
			cpdf_setFont(pdf, "Times-Italic", "MacRomanEncoding", TextHeightPts);
			if( format == MM_MIL )
				cpdf_textCRLFshow(pdf, "Size ( mm[mil] ): " );
			else
				cpdf_textCRLFshow(pdf, "Size: " );
			y -= TextLeadingPts/72.0;
			cpdf_setFont(pdf, "Times-Roman", "MacRomanEncoding", TextHeightPts);
			CString x_str;
			CString y_str;
			RECT br;
			br = foot.GetCornerBounds();
			if( format == MM_MIL )
			{
				::MakeCStringFromDimension( &x_str, br.right-br.left, MM, FALSE, FALSE, FALSE, 2 );
				::MakeCStringFromDimension( &y_str, br.top-br.bottom, MM, FALSE, FALSE, FALSE, 2 );
				cpdf_textShow( pdf, x_str + " x " + y_str + " [" );
				::MakeCStringFromDimension( &x_str, br.right-br.left, MIL, FALSE, FALSE, FALSE, 0 );
				::MakeCStringFromDimension( &y_str, br.top-br.bottom, MIL, FALSE, FALSE, FALSE, 0 );
				cpdf_textShow( pdf, x_str + " x " + y_str + "]" );
			}
			else
			{
				if( units == MM )
				{
					::MakeCStringFromDimension( &x_str, br.right-br.left, units, FALSE, FALSE, FALSE, 2 );
					::MakeCStringFromDimension( &y_str, br.top-br.bottom, units, TRUE, TRUE, TRUE, 2 );
				}
				else
				{
					::MakeCStringFromDimension( &x_str, br.right-br.left, units, FALSE, FALSE, FALSE, 0 );
					::MakeCStringFromDimension( &y_str, br.top-br.bottom, units, TRUE, TRUE, TRUE, 0 );
				}
				cpdf_textShow( pdf, x_str + " x " + y_str );
			}
			if( foot.GetNumPins() == 2 && foot.m_padstack[0].hole_size != 0 )
			{
				CString pin_spacing_str;
				double pin_spacing;
				double dx = foot.m_padstack[0].x_rel - foot.m_padstack[1].x_rel;
				double dy = foot.m_padstack[0].y_rel - foot.m_padstack[1].y_rel;
				if( dx == 0.0 )
					pin_spacing = fabs(dy);
				else if( dy == 0.0 )
					pin_spacing = fabs(dx);
				else
					pin_spacing = sqrt(dx*dx + dy*dy);
				cpdf_setFont(pdf, "Times-Italic", "MacRomanEncoding", TextHeightPts);
				cpdf_textShow( pdf, "        Pin spacing: " );
				cpdf_setFont(pdf, "Times-Roman", "MacRomanEncoding", TextHeightPts);
				if( format == MM_MIL )
				{
					CString mil_str;
					::MakeCStringFromDimension( &pin_spacing_str, pin_spacing, MM, FALSE, FALSE, FALSE, 2 );
					::MakeCStringFromDimension( &mil_str, pin_spacing, MIL, FALSE, FALSE, FALSE, 2 );
					pin_spacing_str += " [" + mil_str + "]";
				}
				else
				{
					::MakeCStringFromDimension( &pin_spacing_str, pin_spacing, units,
						TRUE, TRUE, TRUE, 2 );
				}
				cpdf_textShow( pdf, pin_spacing_str );
			}
			cpdf_textCRLF(pdf);
			y -= TextLeadingPts/72.0;
			cpdf_endText(pdf);

			// draw pad table
			float y_line1 = y;
			cpdf_setlinewidth( pdf, 1 );
			cpdf_setlinecap( pdf, 1 );	// round end-caps
			cpdf_setrgbcolorStroke(pdf, 0.0, 0.0, 0.0);	/* black as stroke color */
			cpdf_moveto( pdf, LeftMargin, y );
			cpdf_lineto( pdf, TableEndX, y );
			cpdf_stroke( pdf );
			y -= TableTextLeadingPts/72.0;
			float y_line2 = y;
			cpdf_beginText( pdf, 0 );
			cpdf_textAligned( pdf, (LeftMargin+TableTypeX)/2, y_line2, 0, TEXTPOS_MM, "PIN(s)" );
			cpdf_textAligned( pdf, (TableTypeX+TableEndX)/2, y_line2+TableTextOffsetY, 0, TEXTPOS_LM, "PAD" );
			cpdf_endText( pdf );
			cpdf_moveto( pdf, TableTypeX, y );
			cpdf_lineto( pdf, TableEndX, y );
			cpdf_stroke( pdf );
			y -= TableTextLeadingPts/72.0;
			float y_line3 = y;
			cpdf_beginText( pdf, 0 );
			cpdf_textAligned( pdf, (TableTypeX+TableShapeX)/2, y_line3+TableTextOffsetY, 0, TEXTPOS_LM, "TYPE" );
			cpdf_textAligned( pdf, (TableShapeX+TableSizeX)/2, y_line3+TableTextOffsetY, 0, TEXTPOS_LM, "SHAPE" );
			cpdf_textAligned( pdf, (TableSizeX+TableHoleX)/2, y_line3+TableTextOffsetY, 0, TEXTPOS_LM, "SIZE" );
			cpdf_textAligned( pdf, (TableHoleX+TableEndX)/2, y_line3+TableTextOffsetY, 0, TEXTPOS_LM, "HOLE" );
			cpdf_endText( pdf );
			cpdf_moveto( pdf, LeftMargin, y );
			cpdf_lineto( pdf, TableEndX, y );
			cpdf_stroke( pdf );
			for( int i=0; i<npadlines; i++ )
			{
				CString pin_str;
				if( num_same[i] > 1 )
					pin_str.Format( "%d-%d", start_num[i], start_num[i]+num_same[i]-1 );
				else
					pin_str.Format( "%d", start_num[i] );
				CString type_str;
				if( pad_hole[i] == 0 )
					type_str = "SMT";
				else
					type_str = "TH";
				CString shape_str;
				if( pad_shape[i] == PAD_NONE )
					shape_str = "None";
				else if( pad_shape[i] == PAD_ROUND )
					shape_str = "Round";
				else if( pad_shape[i] == PAD_SQUARE )
					shape_str = "Square";
				else if( pad_shape[i] == PAD_RECT )
					shape_str = "Rect";
				else if( pad_shape[i] == PAD_RRECT )
					shape_str = "RndRect";
				else if( pad_shape[i] == PAD_OVAL )
					shape_str = "Oval";
				else if( pad_shape[i] == PAD_OCTAGON )
					shape_str = "Octagon";
				else
					ASSERT(0);
				CString size_str;
				if( pad_shape[i] == PAD_NONE )
					size_str = "---";
				else if( pad_shape[i] == PAD_RECT || pad_shape[i] == PAD_RRECT || pad_shape[i] == PAD_OVAL )
				{
					if( format == MM_MIL )
					{
						CString dim_str;
						MakeCStringFromDimension( &size_str, pad_l[i], MM, FALSE, FALSE, FALSE, 2 );
						MakeCStringFromDimension( &dim_str, pad_w[i], MM, FALSE, FALSE, FALSE, 2 );
						size_str += " x " + dim_str + " [";
						MakeCStringFromDimension( &dim_str, pad_l[i], MIL, FALSE, FALSE, FALSE, 1 );
						size_str += dim_str;
						MakeCStringFromDimension( &dim_str, pad_w[i], MIL, FALSE, FALSE, FALSE, 1 );
						size_str += " x " + dim_str + "]";
					}
					else
					{
						CString wid_str;
						MakeCStringFromDimension( &size_str, pad_l[i], units, FALSE, FALSE, FALSE, 2 );
						MakeCStringFromDimension( &wid_str, pad_w[i], units, TRUE, TRUE, TRUE, 2 );
						size_str = size_str + " x " + wid_str;
					}
				}
				else
				{
					if( format == MM_MIL )
					{
						CString mil_str;
						MakeCStringFromDimension( &size_str, pad_w[i], MM, FALSE, FALSE, FALSE, 2 );
						MakeCStringFromDimension( &mil_str, pad_w[i], MIL, FALSE, FALSE, FALSE, 1 );
						size_str += " [" + mil_str + "]";
					}
					else
					{
						MakeCStringFromDimension( &size_str, pad_w[i], units, TRUE, TRUE, TRUE, 2 );
					}

				}
				size_str.MakeLower();
				CString hole_str;
				if( pad_hole[i] )
				{
					if( format == MM_MIL )
					{
						CString mil_str;
						MakeCStringFromDimension( &hole_str, pad_hole[i], MM, FALSE, FALSE, FALSE, 2 );
						MakeCStringFromDimension( &mil_str, pad_hole[i], MIL, FALSE, FALSE, FALSE, 1 );
						hole_str += " [" + mil_str + "]";
					}
					else
					{
						MakeCStringFromDimension( &hole_str, pad_hole[i], units, TRUE, TRUE, TRUE, 2 );
					}
				}
				else
					hole_str = "---";
				y -= TableTextLeadingPts/72.0;
				cpdf_moveto( pdf, LeftMargin, y );
				cpdf_lineto( pdf, TableEndX, y );
				cpdf_stroke( pdf );
				cpdf_beginText( pdf, 0 );
				cpdf_textAligned( pdf, (LeftMargin+TableTypeX)/2, y+TableTextOffsetY, 0, TEXTPOS_LM, pin_str );
				cpdf_textAligned( pdf, (TableTypeX+TableShapeX)/2, y+TableTextOffsetY, 0, TEXTPOS_LM, type_str );
				cpdf_textAligned( pdf, (TableShapeX+TableSizeX)/2, y+TableTextOffsetY, 0, TEXTPOS_LM, shape_str );
				cpdf_textAligned( pdf, (TableSizeX+TableHoleX)/2, y+TableTextOffsetY, 0, TEXTPOS_LM, size_str );
				cpdf_textAligned( pdf, (TableHoleX+TableEndX)/2, y+TableTextOffsetY, 0, TEXTPOS_LM, hole_str );
				cpdf_endText( pdf );
			}
			float y_bottom = y;
			cpdf_moveto( pdf, LeftMargin, y );
			cpdf_lineto( pdf, TableEndX, y );
			cpdf_stroke( pdf );
			cpdf_moveto( pdf, LeftMargin, y_line1 );
			cpdf_lineto( pdf, LeftMargin, y_bottom );
			cpdf_stroke( pdf );
			cpdf_moveto( pdf, TableTypeX, y_line1 );
			cpdf_lineto( pdf, TableTypeX, y_bottom );
			cpdf_stroke( pdf );
			cpdf_moveto( pdf, TableShapeX, y_line2 );
			cpdf_lineto( pdf, TableShapeX, y_bottom );
			cpdf_stroke( pdf );
			cpdf_moveto( pdf, TableSizeX, y_line2 );
			cpdf_lineto( pdf, TableSizeX, y_bottom );
			cpdf_stroke( pdf );
			cpdf_moveto( pdf, TableHoleX, y_line2 );
			cpdf_lineto( pdf, TableHoleX, y_bottom );
			cpdf_stroke( pdf );
			cpdf_moveto( pdf, TableEndX, y_line1 );
			cpdf_lineto( pdf, TableEndX, y_bottom );
			cpdf_stroke( pdf );
			y = y_bottom;

			// get rest of footprint space boundaries
			float bottom_y = y_bottom;
			float left_x = TableEndX + 0.5;

			// get bounding rectangle and convert to inches
			const float NM_PER_INCH = 25400000.0;
			RECT foot_r;
			foot_r = foot.GetBounds(1);
			float foot_left = foot_r.left/NM_PER_INCH;
			float foot_right = foot_r.right/NM_PER_INCH;
			float foot_top = foot_r.top/NM_PER_INCH;
			float foot_bottom = foot_r.bottom/NM_PER_INCH;
			// get scale factor
			float x_scale = (right_x - left_x)/(foot_right - foot_left);
			float y_scale = (top_y - bottom_y )/(foot_top - foot_bottom);
			float scale;
			if( x_scale > 8.0 && y_scale > 8.0 )
				scale = 8.0;
			else if( x_scale > 4.0 && y_scale > 4.0 )
				scale = 4.0;
			else if( x_scale > 2.0 && y_scale > 2.0 )
				scale = 2.0;
			else if( x_scale > 1.0 && y_scale > 1.0 )
				scale = 1.0;
			else if( x_scale > 0.5 && y_scale > 0.5 )
				scale = 0.5;
			else if( x_scale > 0.25 && y_scale > 0.25 )
				scale = 0.25;
			else if( x_scale > 0.125 && y_scale > 0.125 )
				scale = 0.125;
			else if( x_scale > 0.0625 && y_scale > 0.0625 )
				scale = 0.0625;
			else if( x_scale > 0.03125 && y_scale > 0.03125 )
				scale = 0.03125;
			else if( x_scale > 0.015625 && y_scale > 0.015625 )
				scale = 0.015625;
			else 
				ASSERT(0);

			// draw scale
			float middle_x = (left_x + right_x)/2.0;
			float middle_y = (bottom_y + top_y)/2.0;
			cpdf_beginText( pdf, 0 );
			if( abs(scale - 8.0) < 0.01 )
				cpdf_textAligned( pdf, middle_x, top_y + FootprintSpaceY, 0.0, TEXTPOS_LM, "Scale 8:1" );
			else if( abs(scale - 4.0) < 0.01 )
				cpdf_textAligned( pdf, middle_x, top_y + FootprintSpaceY, 0.0, TEXTPOS_LM, "Scale 4:1" );
			else if( abs(scale - 2.0) < 0.01 )
				cpdf_textAligned( pdf, middle_x, top_y + FootprintSpaceY, 0.0, TEXTPOS_LM, "Scale 2:1" );
			else if( abs(scale - 1.0) < 0.01 )
				cpdf_textAligned( pdf, middle_x, top_y + FootprintSpaceY, 0.0, TEXTPOS_LM, "Scale 1:1" );
			else if( abs(scale - 0.5) < 0.01 )
				cpdf_textAligned( pdf, middle_x, top_y + FootprintSpaceY, 0.0, TEXTPOS_LM, "Scale 1:2" );
			else if( abs(scale - 0.25) < 0.01 )
				cpdf_textAligned( pdf, middle_x, top_y + FootprintSpaceY, 0.0, TEXTPOS_LM, "Scale 1:4" );
			else if( abs(scale - 0.125) < 0.01 )
				cpdf_textAligned( pdf, middle_x, top_y + FootprintSpaceY, 0.0, TEXTPOS_LM, "Scale 1:8" );
			else if( abs(scale - 0.0625) < 0.01 )
				cpdf_textAligned( pdf, middle_x, top_y + FootprintSpaceY, 0.0, TEXTPOS_LM, "Scale 1:16" );
			else if( abs(scale - 0.03125) < 0.01 )
				cpdf_textAligned( pdf, middle_x, top_y + FootprintSpaceY, 0.0, TEXTPOS_LM, "Scale 1:32" );
			else if( abs(scale - 0.015625) < 0.01 )
				cpdf_textAligned( pdf, middle_x, top_y + FootprintSpaceY, 0.0, TEXTPOS_LM, "Scale 1:64" );
			cpdf_endText( pdf );

			// set origin for footprint
			float shift_x = ((right_x - left_x) - (foot_right - foot_left)*scale)/2.0;
			float org_x = right_x - foot_right*scale - shift_x;
			float org_y = top_y - foot_top*scale;

			// draw pads
			cpdf_setlinecap( pdf, 1 );		// round end-caps 
			cpdf_setlinewidth( pdf, 0.3 );	// 5 mil
			for( int step=0; step<2; step++ )
			{
				if(step == 0)
					cpdf_setrgbcolor( pdf, 0.0, 1.0, 0.0 );
				else
					cpdf_setrgbcolor( pdf, 1.0, 1.0, 1.0 );
				for( int ip=0; ip<foot.GetNumPins(); ip++ )
				{
					padstack * ps = &foot.m_padstack[ip];
					float x = org_x + ps->x_rel*scale/NM_PER_INCH;
					float y = org_y + ps->y_rel*scale/NM_PER_INCH;
					float w = ps->top.size_Y*scale/NM_PER_INCH;
					float l = ps->top.size_X*scale/NM_PER_INCH;
					float r = ps->top.radius*scale/NM_PER_INCH;
					if( ps->top.shape == PAD_OVAL )
						r = min(l,w)/2.0;
					float hole = ps->hole_size*scale/NM_PER_INCH;
					if( step == 1 && ps->hole_size )
					{
						cpdf_newpath( pdf );
						if( ps->top.shape == PAD_NONE ) 
							cpdf_setrgbcolor( pdf, 1.0, 1.0, 0.0 );
						else if( ps->top.size_X < ps->hole_size || ps->top.size_Y < ps->hole_size )
							cpdf_setrgbcolor( pdf, 1.0, 1.0, 0.0 );
						else
							cpdf_setrgbcolor( pdf, 1.0, 1.0, 1.0 );
						cpdf_arc( pdf, x, y, hole/2, 360.0, 0.0, 1 );
						cpdf_fill( pdf );
						cpdf_closepath( pdf );	
						continue;
					}
					if( ps->top.shape == PAD_NONE )
					{
						cpdf_newpath( pdf );
						cpdf_circle( pdf, x, y, hole/2 );
						cpdf_stroke( pdf );
						cpdf_fill( pdf );
						cpdf_closepath( pdf );				
					}
					else if( ps->top.shape == PAD_ROUND )
					{
						cpdf_newpath( pdf );
						cpdf_circle( pdf, x, y, w/2 );
						cpdf_fill( pdf );
						cpdf_closepath( pdf );	
					}
					else if( ps->top.shape == PAD_OCTAGON )
					{
						const double pi = 3.14159265359;
						cpdf_newpath( pdf );
						cpdf_moveto( pdf, x+w/2, y );
						double angle = -(double)ps->angle*pi/180.0 + pi/8.0;
						for( int i=0; i<8; i++ )
						{
							if( i==0 )
								cpdf_moveto( pdf, x+w*cos(angle)/2.0, y+w*sin(angle)/2.0 );
							else
								cpdf_lineto( pdf, x+w*cos(angle)/2.0, y+w*sin(angle)/2.0 );
							angle += pi/4.0;
						}
						cpdf_fill( pdf );
						cpdf_closepath( pdf );	
					}
					else if( ps->top.shape == PAD_SQUARE )
					{
						cpdf_newpath( pdf );
						if( ps->angle%90 )
						{
							float x1,y1,x2,y2,x3,y3,x4,y4;
							x1 = -w/2;
							y1 = -w/2;
							x2 = +w/2;
							y2 = -w/2;
							x3 = +w/2;
							y3 = +w/2;
							x4 = -w/2;
							y4 = +w/2;
							Rotate_Vertex(&x1,&y1,-ps->angle);
							Rotate_Vertex(&x2,&y2,-ps->angle);
							Rotate_Vertex(&x3,&y3,-ps->angle);
							Rotate_Vertex(&x4,&y4,-ps->angle);
							x1 += x;
							y1 += y;
							x2 += x;
							y2 += y;
							x3 += x;
							y3 += y;
							x4 += x;
							y4 += y;
							cpdf_moveto( pdf, x1, y1 );
							cpdf_lineto( pdf, x2, y2 );
							cpdf_lineto( pdf, x3, y3 );
							cpdf_lineto( pdf, x4, y4 );
							cpdf_lineto( pdf, x1, y1 );
						}
						else
							cpdf_rect( pdf, x-w/2, y-w/2, w, w );
						cpdf_fill( pdf );
						cpdf_closepath( pdf );	
					}
					else if( ps->top.shape == PAD_RECT)
					{
						cpdf_newpath( pdf );
						if( ps->angle%180 == 0 )
							cpdf_rect( pdf, x-l/2, y-w/2, l, w );
						else if( ps->angle%90 == 0 && ps->angle%180 )
							cpdf_rect( pdf, x-w/2, y-l/2, w, l );
						else
						{
							float x1,y1,x2,y2,x3,y3,x4,y4;
							x1 = -l/2;
							y1 = -w/2;
							x2 = +l/2;
							y2 = -w/2;
							x3 = +l/2;
							y3 = +w/2;
							x4 = -l/2;
							y4 = +w/2;
							Rotate_Vertex(&x1,&y1,-ps->angle);
							Rotate_Vertex(&x2,&y2,-ps->angle);
							Rotate_Vertex(&x3,&y3,-ps->angle);
							Rotate_Vertex(&x4,&y4,-ps->angle);
							x1 += x;
							y1 += y;
							x2 += x;
							y2 += y;
							x3 += x;
							y3 += y;
							x4 += x;
							y4 += y;
							cpdf_moveto( pdf, x1, y1 );
							cpdf_lineto( pdf, x2, y2 );
							cpdf_lineto( pdf, x3, y3 );
							cpdf_lineto( pdf, x4, y4 );
							cpdf_lineto( pdf, x1, y1 );
						}
						cpdf_fill( pdf );
						cpdf_closepath( pdf );	
					}
					else if( ps->top.shape == PAD_OVAL )
					{
						cpdf_newpath( pdf );
						double h = l;	// horizontal length of pad
						double v = w;	// vertical width of pad
						if( ps->angle%90 == 0 && ps->angle%180 )
						{
							h = w;
							v = l;
						}
						if( ps->angle%90 )
						{
							int np = 20;
							CPoint rnd[20];
							float radius = min(h,v)/2;
							int nv = Gen_RndRectPoly(	x*(float)NM_PER_MIL,
														y*(float)NM_PER_MIL,
														h*(float)NM_PER_MIL,
														v*(float)NM_PER_MIL,
														radius*(float)NM_PER_MIL,
														-ps->angle,
														&rnd[0],np);
							if(nv)
							{
								float x0 = (float)rnd[0].x/(float)NM_PER_MIL;
								float y0 = (float)rnd[0].y/(float)NM_PER_MIL;
								cpdf_moveto( pdf, x0, y0 );
								for( int i=1; i<nv; i++ )
								{
									float px = (float)rnd[i].x/(float)NM_PER_MIL;
									float py = (float)rnd[i].y/(float)NM_PER_MIL;
									cpdf_lineto( pdf, px, py );
								}
								cpdf_lineto( pdf, x0, y0 );
							}
						}
						else if( h > v )
						{
							// horizontal, clockwise
							cpdf_moveto( pdf, x-h/2.0+r, y+v/2.0 );
							cpdf_arc( pdf, x-h/2.0+r, y, r, 90.0, 270.0, 0 );
							cpdf_arc( pdf, x+h/2.0-r, y, r, 270.0, 450.0, 0 );
						}
						else
						{
							// vertical, clockwise
							cpdf_moveto( pdf, x-h/2.0, y-v/2.0+r );
							cpdf_arc( pdf, x, y-v/2.0+r, r, 180.0, 360.0, 0 );
							cpdf_arc( pdf, x, y+v/2.0-r, r, 0.0, 180.0, 0 );					
						}
						cpdf_fill( pdf );
						cpdf_closepath( pdf );	
					}
					else if( ps->top.shape == PAD_RRECT  )
					{
						cpdf_newpath( pdf );
						double h = l;	// horizontal length of pad
						double v = w;	// vertical width of pad
						if( ps->angle%90 == 0 && ps->angle%180 )
						{
							h = w;
							v = l;
						}
						if( ps->angle%90 )
						{
							int np = 20;
							CPoint rnd[20];
							int nv = Gen_RndRectPoly(	x*(float)NM_PER_MIL,
														y*(float)NM_PER_MIL,
														h*(float)NM_PER_MIL,
														v*(float)NM_PER_MIL,
														r*(float)NM_PER_MIL,
														-ps->angle,
														rnd,np);
							if(nv)
							{
								float x0 = (float)rnd[0].x/(float)NM_PER_MIL;
								float y0 = (float)rnd[0].y/(float)NM_PER_MIL;
								cpdf_moveto( pdf, x0, y0 );
								for( int i=1; i<nv; i++ )
								{
									float px = (float)rnd[i].x/(float)NM_PER_MIL;
									float py = (float)rnd[i].y/(float)NM_PER_MIL;
									cpdf_lineto( pdf, px, py );
								}
								cpdf_lineto( pdf, x0, y0 );
							}		
						}
						else 
						{
							cpdf_moveto( pdf, x-h/2.0, y+v/2.0-r );
							cpdf_arc( pdf, x-h/2.0+r, y+v/2.0-r, r, 180.0, 90.0, 0 );
							cpdf_lineto( pdf, x+h/2.0-r, y+v/2.0 );
							cpdf_arc( pdf, x+h/2.0-r, y+v/2.0-r, r, 90.0, 0.0, 0 );
							cpdf_lineto( pdf, x+h/2.0, y-v/2.0+r );
							cpdf_arc( pdf, x+h/2.0-r, y-v/2.0+r, r, 360.0, 270.0, 0 );
							cpdf_lineto( pdf, x-h/2.0+r, y-v/2.0 );
							cpdf_arc( pdf, x-h/2.0+r, y-v/2.0+r, r, 270.0, 180.0, 0 );
							cpdf_closepath( pdf );
							cpdf_fill( pdf );
						}
						cpdf_fill( pdf );
						cpdf_closepath( pdf );	
					}
					else
						ASSERT(0);
				}
			}
			cpdf_setrgbcolor( pdf, 0.0, 0.0, 0.0 );
			// draw outline
			for(int ip=0; ip<foot.m_outline_poly.GetSize(); ip++ )
			{
				float last_x, last_y;
				CPolyLine * poly = &foot.m_outline_poly[ip];
				if( poly->GetLayer() != LAY_FP_SILK_TOP )
					continue;
				cpdf_newpath( pdf );
				int nc = poly->GetNumCorners();
				for( int is=0; is<=nc; is++ )
				{
					float x,y,sst;
					if( is == 0 )
					{
						last_x = org_x + poly->GetX(0)*scale/NM_PER_INCH;
						last_y = org_y + poly->GetY(0)*scale/NM_PER_INCH;
						cpdf_moveto( pdf, last_x, last_y );
						continue;
					}
					else if( is == nc )
					{
						if( poly->GetClosed() == 0 )
							break;
						x = org_x + poly->GetX(0)*scale/NM_PER_INCH;
						y = org_y + poly->GetY(0)*scale/NM_PER_INCH;
					}
					else
					{
						x = org_x + poly->GetX(is)*scale/NM_PER_INCH;
						y = org_y + poly->GetY(is)*scale/NM_PER_INCH;
					}		
					sst = poly->GetSideStyle(is-1);
					//
					if( sst == CPolyLine::STRAIGHT )
					{
						// straight line segment
						cpdf_lineto( pdf, x, y );
					}
					else if( sst == CPolyLine::ARC_CW )
					{
						// ellipse quadrant, clockwise, start vertical
						double x1 = last_x;
						double y1 = last_y + (y-last_y)*2.0*MagicBezier;
						double x2 = x - (x-last_x)*2.0*MagicBezier;
						double y2 = y;
						if( (x>last_x && y<last_y) || (x<last_x && y>last_y) )
						{
							// ellipse quadrant, clockwise, start horizontal
							x1 = last_x + (x-last_x)*2.0*MagicBezier;
							y1 = last_y;
							x2 = x;
							y2 = y - (y-last_y)*2.0*MagicBezier;
						}
						cpdf_curveto( pdf, x1, y1, x2, y2, x, y );
					}
					else if( sst == CPolyLine::ARC_CCW )
					{
						// ellipse quadrant, clockwise, start vertical
						double x1 = last_x;
						double y1 = last_y + (y-last_y)*2.0*MagicBezier;
						double x2 = x - (x-last_x)*2.0*MagicBezier;
						double y2 = y;
						if( (x>last_x && y>last_y) || (x<last_x && y<last_y) )
						{
							// ellipse quadrant, clockwise, start horizontal
							x1 = last_x + (x-last_x)*2.0*MagicBezier;
							y1 = last_y;
							x2 = x;
							y2 = y - (y-last_y)*2.0*MagicBezier;
						}
						cpdf_curveto( pdf, x1, y1, x2, y2, x, y );
					}
					last_x = x;
					last_y = y;
				}	
				cpdf_setlinewidth( pdf, 72.0*scale*poly->GetW()/NM_PER_INCH );
				cpdf_stroke( pdf );
				cpdf_closepath( pdf );
				
			}

			// draw text
			for(int it=0; it<foot.m_tl->text_ptr.GetSize(); it++ )
			{
				CText * t = foot.m_tl->text_ptr[it];
				SMFontUtil * smfontutil = ((CFreePcbApp*)AfxGetApp())->m_Doc->m_smfontutil;	
				CArray<CPoint> gP;
				t->Draw( NULL, smfontutil, &gP );
				if( t->m_mirror == 0 )
				{
					for( int is=0; is<gP.GetSize(); is+=2 )
					{
						double xi = org_x + gP[is].x*scale/NM_PER_INCH;
						double xf = org_x + gP[is+1].x*scale/NM_PER_INCH;
						double yi = org_y + gP[is].y*scale/NM_PER_INCH;
						double yf = org_y + gP[is+1].y*scale/NM_PER_INCH;
						cpdf_moveto( pdf, xi, yi );	
						cpdf_lineto( pdf, xf, yf );
						cpdf_setlinewidth( pdf, 72.0*scale*t->m_stroke_width/NM_PER_INCH );
						cpdf_stroke( pdf );
					}
				}
				t->Undraw();
			}
		}

		cpdf_finalizeAll(pdf);		/* PDF file/memstream is actually written here */
		int err = cpdf_savePDFmemoryStreamToFile(pdf, title_str );
		if( err == -1 )
		{
			if( !m_dlg_log )
			{
				AfxMessageBox( "Error: Unable to write file\nIt may be read-only or open in another application", MB_OK );
			}
			else
			{
				CString log_message = "*** Error: unable to write file ***\r\n";
				m_dlg_log->AddLine( log_message );
			}
		}
		cpdf_close(pdf);			/* shut down */
	}
//#endif
}

void CDlgLibraryManager::Initialize( CFootLibFolderMap * foldermap, CDlgLog * log )
{
	// if editor_footlib exists, use it, otherwise use project_footlib
	m_foldermap = foldermap;
	m_dlg_log = log;
}

void CDlgLibraryManager::OnBnClickedButtonMgrBrowse()
{
	CPathDialog dlg( "Open Folder", "Select footprint library folder", *m_footlib->GetFullPath() );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		CString path_str = dlg.GetPathName();
		m_edit_footlib.SetWindowText( path_str );
		m_footlib = m_foldermap->GetFolder( &path_str, m_dlg_log );
		if( !m_footlib )
		{
			ASSERT(0);
		}
		m_foldermap->SetLastFolder( &path_str );
		while( m_combo_libfile.GetCount() != 0 )
		{
			m_combo_libfile.DeleteString(0);
		}
		for( int i=0; i<m_footlib->GetNumLibs(); i++ )
		{
			m_combo_libfile.InsertString( i, *m_footlib->GetLibraryFullPath(i) );
		}
		m_combo_libfile.InsertString( m_footlib->GetNumLibs(), "*** all library files ***" );
		m_combo_libfile.SetCurSel( 0 );
	}
}
