// DlgAddPin.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgAddPin.h"

int ConvertMaskCurSelToShape( int cur_sel )
{
	if( cur_sel == 0 )
		return PAD_DEFAULT;
	else
		return cur_sel - 1;
}

int ConvertMaskShapeToCurSel( int shape )
{
	if( shape == PAD_DEFAULT )
		return 0;
	else
		return shape + 1;
}

double GetNameValue( CString * name )
{
	double value = 0.0;
	for( int ic=0; ic<name->GetLength(); ic++ ) 
	{
		char c = name->GetAt( ic );
		int v = 0;
		if( c >= '0' && c <= '9' )
			v = c - '0';
		else if( c >= 'a' && c <= 'z' )
			v = c - 'a' + 10;
		else if( c >= 'A' && c <= 'Z' )
			v = c - 'A' + 10;
		else v = 39;
		value = value*40.0 + v;
	}
	return value;
}

void SortByName( CArray<CString> * names ) 
{
	// bubble sort
	int n = names->GetSize();
	for( int is=0; is<n-1; is++ )
	{
		// swap name[is] with lowest value following in array
		double vmin = GetNameValue( &(*names)[is] );
		int imin = is;
		for( int it=is+1; it<n; it++ )
		{
			double vtest = GetNameValue( &(*names)[it] );
			if( vtest < vmin )
			{
				imin = it;
				vmin = vtest;
			}
		}
		if( imin != is )
		{
			// swap name[is] with name[imin]
			CString temp = (*names)[is];
			(*names)[is] = (*names)[imin];
			(*names)[imin] = temp;
		}
	}
}

// CDlgAddPin dialog

IMPLEMENT_DYNAMIC(CDlgAddPin, CDialog)
CDlgAddPin::CDlgAddPin(CWnd* pParent /*=NULL*/)
: CDialog(CDlgAddPin::IDD, pParent)
{
	// init params
	m_units = MIL;
	m_padstack_type = 0;
	m_hole_diam = 0;
	m_top_shape = 0;
	m_top_width = 0;
	m_top_length = 0;
	m_top_radius = 0;
	m_top_mask_shape = 0;
	m_top_mask_width = 0;
	m_top_mask_length = 0;
	m_top_mask_radius = 0;
	m_top_paste_shape = 0;
	m_top_paste_width = 0;
	m_top_paste_length = 0;
	m_top_paste_radius = 0;
	m_top_flags = 0;
	m_inner_shape = 0;
	m_inner_width = 0;
	m_inner_length = 0;
	m_inner_radius = 0;
	m_inner_flags = 0;
	m_bottom_shape = 0;
	m_bottom_width = 0;
	m_bottom_length = 0;
	m_bottom_radius = 0;
	m_bottom_mask_shape = 0;
	m_bottom_mask_width = 0;
	m_bottom_mask_length = 0;
	m_bottom_mask_radius = 0;
	m_bottom_paste_shape = 0;
	m_bottom_paste_width = 0;
	m_bottom_paste_length = 0;
	m_bottom_paste_radius = 0;
	m_bottom_flags = 0;
	m_x = 0;
	m_y = 0;
	m_x_edge = 1;
	m_y_edge = 1;
	m_row_spacing = 0;
}

CDlgAddPin::~CDlgAddPin()
{
}

void CDlgAddPin::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX); 
	DDX_Control(pDX, IDC_RADIO_ADD_PIN, m_radio_add_pin);
	DDX_Control(pDX, IDC_RADIO_ADD_ROW, m_radio_add_row);
	DDX_Control(pDX, IDC_COMBO_PIN_UNITS, m_combo_units);
	DDX_Control(pDX, IDC_RADIO_SAME_AS, m_check_same_as_pin);
	DDX_Control(pDX, IDC_RADIO_SMT, m_radio_smt);
	DDX_Control(pDX, IDC_RADIO_TH, m_radio_th);
	DDX_Control(pDX, IDC_COMBO_SAME_AS_PIN, m_combo_same_as_pin);
	DDX_Control(pDX, IDC_EDIT_PIN_NAME, m_edit_pin_name);
	DDX_Control(pDX, IDC_EDIT_PIN_DESCRIPTION, m_edit_pin_description);
	DDX_Control(pDX, IDC_EDIT_NUM_PINS, m_edit_num_pins);
	DDX_Control(pDX, IDC_EDIT_INCREMENT, m_edit_increment);
	DDX_Control(pDX, IDC_EDIT_HOLE_DIAM, m_edit_hole_diam);
	DDX_Control(pDX, IDC_RADIO_DRAG_PIN, m_radio_drag);
	DDX_Control(pDX, IDC_EDIT_PIN_X, m_edit_pin_x);
	DDX_Control(pDX, IDC_EDIT_PIN_Y, m_edit_pin_y);
	DDX_Control(pDX, IDC_COMBO_TOP_PAD_SHAPE, m_combo_top_shape);
	DDX_Control(pDX, IDC_EDIT_TOP_PAD_W, m_edit_top_width);
	DDX_Control(pDX, IDC_EDIT_TOP_PAD_L, m_edit_top_length);
	DDX_Control(pDX, IDC_EDIT_TOP_PAD_RAD, m_edit_top_radius);
	DDX_Control(pDX, IDC_COMBO_TOP_PAD_SHAPE2, m_combo_top_shape2);
	DDX_Control(pDX, IDC_EDIT_TOP_PAD_W2, m_edit_top_width2);
	DDX_Control(pDX, IDC_EDIT_TOP_PAD_L2, m_edit_top_length2);
	DDX_Control(pDX, IDC_EDIT_TOP_PAD_RAD2, m_edit_top_radius2);
	DDX_Control(pDX, IDC_COMBO_TOP_PAD_SHAPE3, m_combo_top_shape3);
	DDX_Control(pDX, IDC_EDIT_TOP_PAD_W3, m_edit_top_width3);
	DDX_Control(pDX, IDC_EDIT_TOP_PAD_L3, m_edit_top_length3);
	DDX_Control(pDX, IDC_EDIT_TOP_PAD_RAD3, m_edit_top_radius3);
	DDX_Control(pDX, IDC_COMBO_INNER_PAD_SHAPE, m_combo_inner_shape);
	DDX_Control(pDX, IDC_EDIT_INNER_PAD_W, m_edit_inner_width);
	DDX_Control(pDX, IDC_EDIT_INNER_PAD_L, m_edit_inner_length);
	DDX_Control(pDX, IDC_EDIT_INNER_PAD_RAD, m_edit_inner_radius);
	DDX_Control(pDX, IDC_CHECK_BOTTOM_SAME_AS, m_check_bottom_same_as);
	DDX_Control(pDX, IDC_COMBO_BOTTOM_PAD_SHAPE, m_combo_bottom_shape);
	DDX_Control(pDX, IDC_EDIT1_BOTTOM_PAD_W, m_edit_bottom_width);
	DDX_Control(pDX, IDC_EDIT_BOTTOM_PAD_L, m_edit_bottom_length);
	DDX_Control(pDX, IDC_EDIT_BOTTOM_PAD_RAD, m_edit_bottom_radius);
	DDX_Control(pDX, IDC_COMBO_BOTTOM_PAD_SHAPE2, m_combo_bottom_shape2);
	DDX_Control(pDX, IDC_EDIT_BOTTOM_PAD_W2, m_edit_bottom_width2);
	DDX_Control(pDX, IDC_EDIT_BOTTOM_PAD_L2, m_edit_bottom_length2);
	DDX_Control(pDX, IDC_EDIT_BOTTOM_PAD_RAD2, m_edit_bottom_radius2);
	DDX_Control(pDX, IDC_COMBO_BOTTOM_PAD_SHAPE3, m_combo_bottom_shape3);
	DDX_Control(pDX, IDC_EDIT_BOTTOM_PAD_W3, m_edit_bottom_width3);
	DDX_Control(pDX, IDC_EDIT_BOTTOM_PAD_L3, m_edit_bottom_length3);
	DDX_Control(pDX, IDC_EDIT_BOTTOM_PAD_RAD3, m_edit_bottom_radius3);
	DDX_Control(pDX, IDC_COMBO_ROW_ORIENT, m_combo_row_orient);
	DDX_Control(pDX, IDC_EDIT_ROW_SPACING, m_edit_row_spacing);
	DDX_Control(pDX, IDC_RADIO_SET_PIN_POS, m_radio_set_pos);
	DDX_Control(pDX, IDC_LIST_PINS, m_list_pins);
	DDX_Control(pDX, IDC_RADIO_1_1, m_radio_connect[0][0] );
	DDX_Control(pDX, IDC_RADIO_1_2, m_radio_connect[0][1] );
	DDX_Control(pDX, IDC_RADIO_1_3, m_radio_connect[0][2] );
	DDX_Control(pDX, IDC_RADIO_1_4, m_radio_connect[0][3] );
	DDX_Control(pDX, IDC_RADIO_2_1, m_radio_connect[1][0] );
	DDX_Control(pDX, IDC_RADIO_2_2, m_radio_connect[1][1] );
	DDX_Control(pDX, IDC_RADIO_2_3, m_radio_connect[1][2] );
	DDX_Control(pDX, IDC_RADIO_2_4, m_radio_connect[1][3] );
	DDX_Control(pDX, IDC_RADIO_3_1, m_radio_connect[2][0] );
	DDX_Control(pDX, IDC_RADIO_3_2, m_radio_connect[2][1] );
	DDX_Control(pDX, IDC_RADIO_3_3, m_radio_connect[2][2] );
	DDX_Control(pDX, IDC_RADIO_3_4, m_radio_connect[2][3] );
	DDX_Control(pDX, IDC_COMBO_X_EDGE, m_combo_x_edge);
	DDX_Control(pDX, IDC_COMBO_Y_EDGE, m_combo_y_edge);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		CString str;
		for( int i=0; i<7; i++ ) 
		{
			CComboBox * cbox;
			BOOL bMask;
			switch(i) {
				case 0: cbox = &m_combo_top_shape; bMask = FALSE; break;
				case 1: cbox = &m_combo_top_shape2; bMask = TRUE; break;
				case 2: cbox = &m_combo_top_shape3; bMask = TRUE; break;
				case 3: cbox = &m_combo_inner_shape; bMask = FALSE; break;
				case 4: cbox = &m_combo_bottom_shape; bMask = FALSE; break;
				case 5: cbox = &m_combo_bottom_shape2; bMask = TRUE; break;
				case 6: cbox = &m_combo_bottom_shape3; bMask = TRUE; break;
			}
			if( bMask == FALSE )
			{
				cbox->InsertString( PAD_NONE, "none" );
				cbox->InsertString( PAD_ROUND, "round" );
				cbox->InsertString( PAD_SQUARE, "square" );
				cbox->InsertString( PAD_RECT, "rect" );
				cbox->InsertString( PAD_RRECT, "rounded-rect" );
				cbox->InsertString( PAD_OVAL, "oval" );
				cbox->InsertString( PAD_OCTAGON, "octagon" );
				cbox->SetCurSel( 0 );
			}
			else
			{
				cbox->InsertString( 0, "<default>" );
				cbox->InsertString( PAD_NONE+1, "none" );
				cbox->InsertString( PAD_ROUND+1, "round" );
				cbox->InsertString( PAD_SQUARE+1, "square" );
				cbox->InsertString( PAD_RECT+1, "rect" );
				cbox->InsertString( PAD_RRECT+1, "rounded-rect" );
				cbox->InsertString( PAD_OVAL+1, "oval" );
				cbox->InsertString( PAD_OCTAGON+1, "octagon" );
				cbox->SetCurSel( 0 );
			}
		}
		m_radio_add_pin.SetCheck( 1 );
		OnBnClickedRadioAddPin();
		m_combo_units.AddString( "mil" );
		m_combo_units.AddString( "mm" );
		if( m_units == MIL )
			m_combo_units.SetCurSel( 0 );
		else
			m_combo_units.SetCurSel( 1 );

		m_combo_x_edge.AddString( "left" );
		m_combo_x_edge.AddString( "center");
		m_combo_x_edge.AddString( "right");
		m_combo_x_edge.SetCurSel(1);

		m_combo_y_edge.AddString( "top" );
		m_combo_y_edge.AddString( "middle");
		m_combo_y_edge.AddString( "bottom");
		m_combo_y_edge.SetCurSel(1);

		m_combo_row_orient.InsertString( 0, "horiz" );
		m_combo_row_orient.InsertString( 1, "vert" );
		m_combo_row_orient.SetCurSel( 0 );
		if( m_fp->GetNumPins() )
		{
			CArray<CString> pin_name;
			pin_name.SetSize( m_fp->GetNumPins() );
			for( int i=0; i<m_fp->GetNumPins(); i++ )
			{
				pin_name[i] = m_fp->m_padstack[i].name;
			}
			/// ::SortByName( pin_name ); cannot use sorting, the relationship between the indexes will be broken!!!
			int ipp = 0; 
			for( int i=0; i<m_fp->GetNumPins(); i++ )
			{
				pad * p = &m_fp->m_padstack[i].top;
				for( int stp=0; stp<2; stp++ )
				{
					if(p)
						if( p->shape )
							break;
					p = &m_fp->m_padstack[i].bottom;
				}
				pad * copper_pad = p;
				BOOL isMask = FALSE;
				if( !copper_pad )
					continue;
				if( p->shape == PAD_NONE || p->shape == PAD_DEFAULT )
				{
					p = &m_fp->m_padstack[i].top_mask;
					for( int stp=0; stp<2; stp++ )
					{
						if(p)
							if( p->shape && p->shape != PAD_DEFAULT )
							{
								isMask = TRUE;
								break;
							}
						p = &m_fp->m_padstack[i].bottom_mask;
					}
				}
				BOOL isPaste= FALSE;
				if( !p )
					continue;
				if( p->shape == PAD_NONE || p->shape == PAD_DEFAULT )
				{
					p = &m_fp->m_padstack[i].top_paste;
					for( int stp=0; stp<2; stp++ )
					{
						if(p)
							if( p->shape && p->shape != PAD_DEFAULT )
							{
								isPaste = TRUE;
								break;
							}
						p = &m_fp->m_padstack[i].bottom_paste;
					}
				}
				// insert
				CString pStr = "???";
				if( !p )
					continue;
				POINT SIZE;
				SIZE.x = copper_pad->size_X;
				SIZE.y = copper_pad->size_Y;
				if( copper_pad->shape != PAD_NONE && m_fp->m_padstack[i].hole_size )
					pStr = "thru pad";
				else if( copper_pad->shape == PAD_NONE && m_fp->m_padstack[i].hole_size )
				{
					pStr = "hole";
					SIZE.x = m_fp->m_padstack[i].hole_size;
					SIZE.y = m_fp->m_padstack[i].hole_size;
				}
				else if( copper_pad->shape == PAD_NONE && isMask )
				{
					pStr = "mask";
					SIZE.x = p->size_X;
					SIZE.y = p->size_Y;
				}
				else if( copper_pad->shape == PAD_NONE && isPaste )
				{
					pStr = "paste";
					SIZE.x = p->size_X;
					SIZE.y = p->size_Y;
				}
				else if( copper_pad->shape == PAD_ROUND )
					pStr = "rnd ";
				else if( copper_pad->shape == PAD_SQUARE )
					pStr = "sqr ";
				else if( copper_pad->shape == PAD_RECT )
					pStr = "rect ";
				else if( copper_pad->shape == PAD_RRECT )
					pStr = "rrect ";
				else if( copper_pad->shape == PAD_OVAL )
					pStr = "oval ";
				else if( copper_pad->shape == PAD_OCTAGON )
					pStr = "oct ";
				CString szStr, sx, sy;
				::MakeCStringFromDimension( &sx, SIZE.x, m_units, 0, 0, 0, 2 );
				::MakeCStringFromDimension( &sy, SIZE.y, m_units, 0, 0, 0, 2 );
				szStr.Format( "%s ( %s %s x %s )", pin_name[i], pStr, sx, sy );
				CString szStr_mod = szStr;
				int f = m_combo_same_as_pin.FindString( -1, szStr_mod );
				int ipd = 2;			
				while( f >= 0 )
				{
					szStr_mod.Format( "%s(%d)", szStr, ipd );
					ipd++;
					f = m_combo_same_as_pin.FindString( -1, szStr_mod );
				}
				m_combo_same_as_pin.AddString( szStr_mod );
				if( m_mode == ADD || i != m_pin_num )
					m_list_pins.InsertString( ipp++, szStr );
			}
		} 
		if( m_mode == ADD && m_fp->GetNumPins() )
		{
			// add pin, pins already defined
			m_ps_angle = 0;
			m_pin_num = m_fp->GetNumPins();
			str.Format( "%d", m_fp->GetNumPins()+1 );
			m_edit_pin_name.SetWindowText( str );
			m_edit_pin_description.SetWindowText("");
			m_combo_same_as_pin.SetCurSel( m_as_pin );
			m_check_same_as_pin.SetCheck(1);
			OnChange();
			m_radio_drag.SetCheck( 1 );
			OnBnClickedRadioDragPin();
		}
		else if( m_mode == ADD )
		{
			// add pin, no pins already defined
			m_ps_angle = 0;
			m_pin_num = 0;
			m_edit_pin_name.SetWindowText( "1" );
			m_edit_pin_description.SetWindowText("");
			m_check_same_as_pin.EnableWindow( FALSE );
			m_combo_same_as_pin.EnableWindow( FALSE );
			m_radio_smt.SetCheck( 1 );
			OnChange();
			m_radio_drag.SetCheck( 1 );
			OnBnClickedRadioDragPin();
		}
		else if( m_mode == EDIT )
		{
			// edit existing pin
			m_edit_pin_name.SetWindowText( m_fp->GetPinNameByIndex( m_pin_num ) );
			m_edit_pin_description.SetWindowText( m_fp->GetPinDescriptionByIndex( m_pin_num ) );
			m_check_same_as_pin.SetCheck(1);
			m_combo_same_as_pin.SetCurSel( m_pin_num );
			m_check_same_as_pin.SetCheck(0);
			OnChange();
			m_radio_add_pin.SetCheck( 0 );
			m_radio_add_pin.EnableWindow( FALSE );
			m_radio_add_row.EnableWindow( FALSE );
			m_edit_num_pins.EnableWindow( FALSE );
			m_edit_increment.EnableWindow( FALSE );
			m_radio_set_pos.SetCheck( 1 );
			OnBnClickedRadioSetPinPos();
			GetFields();
			CString flag_str;
			padstack * ps = &m_fp->m_padstack[m_pin_num];
			m_x = ps->x_rel;
			m_y = ps->y_rel;
			m_hole_diam = ps->hole_size;
			if( m_hole_diam != 0 )
				m_padstack_type = 1;
			else
				m_padstack_type = 0;
			m_ps_angle = ps->angle;
			m_top_shape = ps->top.shape;
			m_top_width = ps->top.size_Y;
			m_top_length = ps->top.size_X;
			m_top_radius = ps->top.radius;
			m_top_flags = ps->top.connect_flag;
			m_top_mask_shape = ps->top_mask.shape;
			m_top_mask_width = ps->top_mask.size_Y;
			m_top_mask_length = ps->top_mask.size_X;
			m_top_mask_radius = ps->top_mask.radius;
			m_top_paste_shape = ps->top_paste.shape;
			m_top_paste_width = ps->top_paste.size_Y;
			m_top_paste_length = ps->top_paste.size_X;
			m_top_paste_radius = ps->top_paste.radius;
			m_inner_shape = ps->inner.shape;
			m_inner_width = ps->inner.size_Y;
			m_inner_length = ps->inner.size_X;
			m_inner_radius = ps->inner.radius;
			m_inner_flags = ps->inner.connect_flag;
			m_bottom_shape = ps->bottom.shape;
			m_bottom_width = ps->bottom.size_Y;
			m_bottom_length = ps->bottom.size_X;
			m_bottom_radius = ps->bottom.radius;
			m_bottom_flags = ps->bottom.connect_flag;
			m_bottom_mask_shape = ps->bottom_mask.shape;
			m_bottom_mask_width = ps->bottom_mask.size_Y;
			m_bottom_mask_length = ps->bottom_mask.size_X;
			m_bottom_mask_radius = ps->bottom_mask.radius;
			m_bottom_paste_shape = ps->bottom_paste.shape;
			m_bottom_paste_width = ps->bottom_paste.size_Y;
			m_bottom_paste_length = ps->bottom_paste.size_X;
			m_bottom_paste_radius = ps->bottom_paste.radius;
			m_bottom_same_as_top = FALSE;
			if( m_hole_diam
				&& ps->top == ps->bottom
				&& ps->top_mask == ps->bottom_mask
				&& ps->top_paste == ps->bottom_paste
				)
			{
				m_bottom_same_as_top = TRUE;
			}
		}
		SetFields();
	}
	else
	{
		// outgoing
		CString str; 
		CString astr;
		CString nstr;
		int n;
		int conflict = 0;
		// update fields
		GetFields();
		// check for legal pin name
		if( !CheckLegalPinName( &m_pin_name, &astr, &nstr, &n ) )
		{
			str = "Pin name must consist of zero or more letters\n";
			str	+= "Followed by zero or more numbers\n";
			str	+= "The characters \" .,;:/!@#$%^&*(){}[]|<>?\\~\'\" are illegal\n";
			str	+= "For example: 1, 23, A12, SOURCE are legal\n";
			str	+= "while 1A, A2B3, A:3 are not\n";
			AfxMessageBox( str );
			pDX->Fail();
		}
		if( m_mode == ADD )
		{
			if( m_num_pins > 1 )
			{
				if( m_row_spacing == 0 )
				{
					AfxMessageBox( "illegal row spacing" );
					pDX->Fail();
				}
				if( nstr.GetLength() == 0 ) 
					m_increment = 0;
			}
		}
		if( m_padstack_type == 1 && m_hole_diam <= 0 )
		{
			AfxMessageBox( "For through-hole pin, hole diameter must be > 0" );
			pDX->Fail();
		}
		if( m_top_shape != PAD_NONE && m_top_width <= 0 )
		{
			AfxMessageBox( "Illegal top pad width" );
			pDX->Fail();
		}
		if( m_padstack_type == 1 && m_inner_shape != PAD_NONE && m_inner_width <= 0 )
		{
			AfxMessageBox( "Illegal inner pad width" );
			pDX->Fail();
		}
		if( m_bottom_shape != PAD_NONE && m_bottom_width <= 0 )
		{
			AfxMessageBox( "Illegal bottom pad width" );
			pDX->Fail();
		}
		if( m_padstack_type == 0 ) 
		{
			if( m_top_shape == PAD_NONE &&    ( m_top_paste_shape == PAD_NONE || m_top_paste_shape == PAD_DEFAULT ) && 
											  ( m_top_mask_shape == PAD_NONE || m_top_mask_shape == PAD_DEFAULT )) 
			{
				if( m_bottom_shape == PAD_NONE && ( m_bottom_paste_shape == PAD_NONE || m_bottom_paste_shape == PAD_DEFAULT) && 
												  ( m_bottom_mask_shape == PAD_NONE || m_bottom_mask_shape == PAD_DEFAULT ) ) 
				{
					AfxMessageBox( "SMT pad shape can't be \"none\"" );
					pDX->Fail();
				} 
			}
		}
		if (( m_top_paste_shape != PAD_NONE && m_top_paste_shape != PAD_DEFAULT && m_top_paste_width <= 0 ) ||
			( m_bottom_paste_shape != PAD_NONE && m_bottom_paste_shape != PAD_DEFAULT && m_bottom_paste_width <= 0 ))
		{
			AfxMessageBox( "Illegal paste width" );
			pDX->Fail();
		}
		if (( m_top_mask_shape != PAD_NONE && m_top_mask_shape != PAD_DEFAULT && m_top_mask_width <= 0 ) ||
			( m_bottom_mask_shape != PAD_NONE && m_bottom_mask_shape != PAD_DEFAULT && m_bottom_mask_width <= 0 ))
		{
			AfxMessageBox( "Illegal mask width" );
			pDX->Fail();
		}
		if( m_top_shape == PAD_RRECT && 
			(( m_top_radius > m_top_length/2 ) || ( m_top_radius > m_top_width/2 )) )
		{
			AfxMessageBox( "Radius of top rounded-rect pad > length/2 or width/2" );
			pDX->Fail();
		}
		if( m_top_mask_shape == PAD_RRECT && 
			(( m_top_mask_radius > m_top_mask_length/2 ) || ( m_top_mask_radius > m_top_mask_width/2 )) )
		{
			AfxMessageBox( "Radius of top mask rounded-rect pad > length/2 or width/2" );
			pDX->Fail();
		}
		if( m_top_paste_shape == PAD_RRECT && 
			(( m_top_paste_radius > m_top_paste_length/2 ) || ( m_top_paste_radius > m_top_paste_width/2 )) )
		{
			AfxMessageBox( "Radius of top paste rounded-rect pad > length/2 or width/2" );
			pDX->Fail();
		}
		if( m_bottom_shape == PAD_RRECT && 
			(( m_bottom_radius > m_bottom_length/2 ) || ( m_bottom_radius > m_bottom_width/2 )) )
		{
			AfxMessageBox( "Radius of bottom rounded-rect pad > length/2 or width/2" );
			pDX->Fail();
		}
		if( m_bottom_mask_shape == PAD_RRECT && 
			(( m_bottom_mask_radius > m_bottom_mask_length/2 ) || ( m_bottom_mask_radius > m_bottom_mask_width/2 )) )
		{
			AfxMessageBox( "Radius of bottom mask rounded-rect pad > length/2 or width/2" );
			pDX->Fail();
		}
		if( m_bottom_paste_shape == PAD_RRECT && 
			(( m_bottom_paste_radius > m_bottom_paste_length/2 ) || ( m_bottom_paste_radius > m_bottom_paste_width/2 )) )
		{
			AfxMessageBox( "Radius of bottom paste rounded-rect pad > length/2 or width/2" );
			pDX->Fail();
		}
		if( m_padstack_type == 1 )
		{
			if( m_inner_shape == PAD_RRECT && 
				(( m_inner_radius > m_inner_length/2 ) || ( m_inner_radius > m_inner_width/2 )) )
			{
				AfxMessageBox( "Radius of inner rounded-rect pad > length/2 or width/2" );
				pDX->Fail();
			}
		}
		// now check for conflicts
		if( nstr.GetLength() == 0 )
		{
			// check for conflicts
			int n_conf = 0;
			int npins = m_fp->m_padstack.GetSize();
			for( int i=0; i<npins; i++ )
			{
				if( astr == m_fp->m_padstack[i].name )
				{
					if( (m_mode == EDIT && i != m_pin_num) || m_mode == ADD )
					{
						n_conf++;
					}
				}
			}
			if( n_conf == 1 )
				conflict = 1;
		}
		else
		{
			// pin name ends in a number, allow insertion
			int npins = m_fp->m_padstack.GetSize();
			for( int ip=0; ip<m_num_pins; ip++ )
			{
				CString pin_name;
				pin_name.Format( "%s%d", astr, n+ip*m_increment );
				int n_conf = 0;
				for( int i=0; i<npins; i++ )
				{
					if( pin_name == m_fp->m_padstack[i].name )
					{
						if( (m_mode == EDIT && i != (m_pin_num)) || m_mode == ADD )
						{
							n_conf++;
						}
					}
				}
				if( n_conf == 1 )
				{
					conflict = 1;
					break;
				}
			}
		}
		if( conflict )
		{
			if( m_num_pins == 1 )
				str.Format( "Pin name \"%s\" conflicts with an existing pin\nCreate multipin?", m_pin_name );
			else
				str.Format( "Pin names \"%s%d\" through \"%s%d\" conflict with existing pins\nCreate multipins?", 
				astr, n, astr, n+(m_num_pins-1)*m_increment );
			int ret = IDYES;
			if( m_mode == ADD || m_pin_name.Compare(m_fp->m_padstack[m_pin_num].name) )
				ret = AfxMessageBox( str, MB_YESNOCANCEL );
			if( ret == IDCANCEL )
				pDX->Fail();
			else if( ret == IDNO )
			{
				if( nstr.GetLength() == 0 )
					pDX->Fail();
				else
					conflict = 2;
			}
		}
		// OK, we are ready to go
		m_fp->Undraw();
		// check if we will be dragging
		m_drag_flag = m_radio_drag.GetCheck();
		// now insert padstacks
		padstack ps;
		ps.hole_size = m_hole_diam;
		if( m_padstack_type == 0 )
			ps.hole_size = 0;
		if( m_padstack_type == 0 )
			m_inner_shape = PAD_NONE;
		ps.angle = m_ps_angle;
		ps.top.shape = m_top_shape;
		ps.top.size_Y = m_top_width;
		ps.top.size_X = m_top_length; 
		ps.top.radius = m_top_radius;
		ps.top.connect_flag = m_top_flags;
		ps.top_mask.shape = m_top_mask_shape;
		ps.top_mask.size_Y = m_top_mask_width;
		ps.top_mask.size_X = m_top_mask_length; 
		ps.top_mask.radius = m_top_mask_radius;
		ps.top_paste.shape = m_top_paste_shape;
		ps.top_paste.size_Y = m_top_paste_width;
		ps.top_paste.size_X = m_top_paste_length; 
		ps.top_paste.radius = m_top_paste_radius;
		ps.inner.shape = m_inner_shape;
		ps.inner.size_Y = m_inner_width;
		ps.inner.size_X = m_inner_length; 
		ps.inner.radius = m_inner_radius;
		ps.inner.connect_flag = m_inner_flags;
		ps.bottom.shape = m_bottom_shape;
		ps.bottom.size_Y = m_bottom_width;
		ps.bottom.size_X = m_bottom_length; 
		ps.bottom.radius = m_bottom_radius;
		ps.bottom.connect_flag = m_bottom_flags;
		ps.bottom_mask.shape = m_bottom_mask_shape;
		ps.bottom_mask.size_Y = m_bottom_mask_width;
		ps.bottom_mask.size_X = m_bottom_mask_length; 
		ps.bottom_mask.radius = m_bottom_mask_radius;
		ps.bottom_paste.shape = m_bottom_paste_shape;
		ps.bottom_paste.size_Y = m_bottom_paste_width;
		ps.bottom_paste.size_X = m_bottom_paste_length; 
		ps.bottom_paste.radius = m_bottom_paste_radius;
		
		int adj_x;
		switch(m_padstack_type)
		{
			case 0:	adj_x = m_top_length / 2;		break;	// SM_Top 
			case 1: adj_x = 0;						break;	// TH
			case 2: adj_x = m_bottom_length / 2;	break;	// SM_Bottom
		}

		int adj_y;
		switch(m_padstack_type)
		{
			case 0:	adj_y = m_top_width / 2;	break;	// SM_Top
			case 1: adj_y = 0;					break;	// TH
			case 2: adj_y = m_bottom_width / 2;	break;	// SM_Bottom
		}

		switch(m_x_edge)
		{
			case 0: ps.x_rel = m_x + adj_x;	break;
			case 1:	ps.x_rel = m_x;			break;
			case 2: ps.x_rel = m_x - adj_x;	break;
		}

		switch(m_y_edge)
		{
			case 0: ps.y_rel = m_y - adj_y;	break;
			case 1:	ps.y_rel = m_y;			break;
			case 2: ps.y_rel = m_y + adj_y;	break;
		}

		// apply to other pins if requested
		if( m_list_pins.GetCount() )
		{
			// see if there are existing SMT pads on top and bottom layers
			BOOL bSMTtop = FALSE;
			BOOL bSMTbottom = FALSE;
			BOOL bPreserveLayer = FALSE;
			int Count = m_list_pins.GetCount();
			for( int ip=Count-1; ip>=0; ip-- )
			{
				if( m_list_pins.GetSel( ip ) )
				{
					int i = ip;
					if( Count != m_fp->m_padstack.GetSize() )
					{
						if( i >= m_pin_num )
							i++;
					}
					padstack * sel_ps = &m_fp->m_padstack[i];
					if( sel_ps->hole_size == 0 )
					{
						if( sel_ps->top.shape != PAD_NONE && sel_ps->bottom.shape == PAD_NONE )
							bSMTtop = TRUE;
						if( sel_ps->bottom.shape != PAD_NONE && sel_ps->top.shape == PAD_NONE )
							bSMTbottom = TRUE;
					}
				}
			}
			if( bSMTtop && bSMTbottom )
			{
				CString mess = "You are applying new pad parameters to SMT pads\n";
				mess += "on both top and bottom layers.\n\n";
				mess += "Click YES to preserve the layers of these pads,\n";
				mess += "or NO to set them all to the layer of the new pad";
				int ret = AfxMessageBox( mess, MB_YESNO );
				if( ret == IDYES )
					bPreserveLayer = TRUE;
			}
			for( int ip=Count-1; ip>=0; ip-- )
			{
				if( m_list_pins.GetSel( ip ) )
				{
					int i = ip;
					if( Count != m_fp->m_padstack.GetSize() )
					{
						if( i >= m_pin_num )
							i++;
					}
					padstack * sel_ps = &m_fp->m_padstack[i];
					sel_ps->angle = ps.angle;
					sel_ps->hole_size = ps.hole_size;
					sel_ps->inner = ps.inner;
					if( ps.hole_size == 0 && sel_ps->hole_size == 0
						&& bPreserveLayer 
						&& (ps.top.shape == PAD_NONE && sel_ps->top.shape != PAD_NONE
						|| ps.bottom.shape == PAD_NONE && sel_ps->bottom.shape != PAD_NONE) )
					{
						// preserve pad layer by reversing assignment
						sel_ps->top = ps.bottom;
						sel_ps->top_mask = ps.bottom_mask;
						sel_ps->top_paste = ps.bottom_paste;
						sel_ps->bottom = ps.top;
						sel_ps->bottom_mask = ps.top_mask;
						sel_ps->bottom_paste = ps.top_paste;
					}
					else
					{
						// just copy pad layer
						sel_ps->top = ps.top;
						sel_ps->top_mask = ps.top_mask;
						sel_ps->top_paste = ps.top_paste;
						sel_ps->bottom = ps.bottom;
						sel_ps->bottom_mask = ps.bottom_mask;
						sel_ps->bottom_paste = ps.bottom_paste;
					}
				}
			}
		}
		// add or replace selected pin
		if( m_mode == EDIT )
		{
			// remove pin
			m_fp->m_padstack.RemoveAt(m_pin_num);
		}
		// add pins to footprint
		int dx = 0;
		int dy = 0;
		if( m_row_orient == 0 )
			dx = m_row_spacing;
		else
			dy = m_row_spacing;
		if( m_num_pins > 1 )
		{
			for( int ip=0; ip<m_num_pins; ip++ )
			{
				if( n > 0 || conflict == 2 )
					ps.name.Format( "%s%d", astr, n+ip*m_increment );
				else
					ps.name = astr;
				if( conflict == 2 )
					m_fp->ShiftToInsertPadName( &astr, n+ip*m_increment );
				m_fp->m_padstack.InsertAt(  m_pin_num+ip, ps );
				ps.x_rel += dx;
				ps.y_rel += dy;
			}
		}
		else
		{
			ps.name.Format( "%s%s", astr, nstr );
			ps.pDescription = m_pin_description;
			if( conflict == 2 )
				m_fp->ShiftToInsertPadName( &astr, n );
			m_fp->m_padstack.InsertAt(  m_pin_num, ps );
		}
	}
}


BEGIN_MESSAGE_MAP(CDlgAddPin, CDialog)
	ON_BN_CLICKED(IDC_RADIO_ADD_PIN, OnBnClickedRadioAddPin)
	ON_BN_CLICKED(IDC_RADIO_ADD_ROW, OnBnClickedRadioAddRow)
	ON_BN_CLICKED(IDC_RADIO_SAME_AS, OnChange)
	ON_BN_CLICKED(IDC_RADIO_SMT, OnChange)
	ON_BN_CLICKED(IDC_RADIO_TH, OnChange)
	ON_BN_CLICKED(IDC_RADIO_DRAG_PIN, OnBnClickedRadioDragPin)
	ON_BN_CLICKED(IDC_RADIO_SET_PIN_POS, OnBnClickedRadioSetPinPos)
	ON_BN_CLICKED(IDC_CHECK_INNER_SAME_AS, OnChange)
	ON_BN_CLICKED(IDC_CHECK_BOTTOM_SAME_AS, OnChange)
	ON_CBN_SELCHANGE(IDC_COMBO_PIN_UNITS, OnCbnSelchangeComboPinUnits)
	ON_CBN_SELCHANGE(IDC_COMBO_ROW_ORIENT, OnChange)
	ON_CBN_SELCHANGE(IDC_COMBO_TOP_PAD_SHAPE, OnChange)
	ON_CBN_SELCHANGE(IDC_COMBO_TOP_PAD_SHAPE2, OnChange)
	ON_CBN_SELCHANGE(IDC_COMBO_TOP_PAD_SHAPE3, OnChange)
	ON_CBN_SELCHANGE(IDC_COMBO_INNER_PAD_SHAPE, OnChange)
	ON_CBN_SELCHANGE(IDC_COMBO_BOTTOM_PAD_SHAPE, OnChange)
	ON_CBN_SELCHANGE(IDC_COMBO_BOTTOM_PAD_SHAPE2, OnChange)
	ON_CBN_SELCHANGE(IDC_COMBO_BOTTOM_PAD_SHAPE3, OnChange)
	ON_CBN_SELCHANGE(IDC_COMBO_SAME_AS_PIN, OnChange)
	ON_CBN_SELCHANGE(IDC_COMBO_PAD_ORIENT, OnChange)
	ON_CBN_SELCHANGE(IDC_COMBO_X_EDGE, OnChange)
	ON_CBN_SELCHANGE(IDC_COMBO_Y_EDGE, OnChange)
	ON_EN_CHANGE(IDC_EDIT_TOP_PAD_W, OnChangeTopPadParam)
	ON_EN_CHANGE(IDC_EDIT_TOP_PAD_L, OnChangeTopPadParam)
	ON_EN_CHANGE(IDC_EDIT_TOP_PAD_RAD, OnChangeTopPadParam)
	ON_EN_CHANGE(IDC_EDIT_TOP_PAD_W2, OnChangeTopPadParam)
	ON_EN_CHANGE(IDC_EDIT_TOP_PAD_L2, OnChangeTopPadParam)
	ON_EN_CHANGE(IDC_EDIT_TOP_PAD_RAD2, OnChangeTopPadParam)
	ON_EN_CHANGE(IDC_EDIT_TOP_PAD_W3, OnChangeTopPadParam)
	ON_EN_CHANGE(IDC_EDIT_TOP_PAD_L3, OnChangeTopPadParam)
	ON_EN_CHANGE(IDC_EDIT_TOP_PAD_RAD3, OnChangeTopPadParam)
	ON_BN_CLICKED(IDC_CHECK_BOTTOM_SAME_AS2, &CDlgAddPin::OnBnClickedCheckBottomSameAs2)
END_MESSAGE_MAP()


void CDlgAddPin::InitDialog( CShape * fp, int mode, int pin_num, int units )
{
	m_units = units;
	m_mode = mode;
	m_as_pin = 0;
	m_pin_num = 0;
	if( m_mode == EDIT)
		m_pin_num = pin_num;
	else
		m_as_pin = pin_num;
	if( fp )
		m_fp = fp;
	else
		ASSERT(0);
	return;
}

// CDlgAddPin message handlers

void CDlgAddPin::OnBnClickedRadioAddPin()
{
	CString str;
	m_radio_add_pin.SetCheck( 1 );
	m_edit_num_pins.EnableWindow( FALSE );
	m_edit_increment.EnableWindow( FALSE );
	m_edit_num_pins.SetWindowText( "1" );
	m_edit_increment.SetWindowText( "1" );
	m_combo_row_orient.EnableWindow( FALSE );
	m_edit_row_spacing.EnableWindow( FALSE );
}

void CDlgAddPin::OnBnClickedRadioAddRow()
{
	CString str;
	m_edit_num_pins.EnableWindow( TRUE );
	m_edit_num_pins.SetWindowText( "1" );
	m_edit_increment.EnableWindow( TRUE );
	m_edit_increment.SetWindowText( "1" );
	m_combo_row_orient.EnableWindow( TRUE );
	m_edit_row_spacing.EnableWindow( TRUE );
}

void CDlgAddPin::OnBnClickedRadioDragPin()
{
	m_edit_pin_x.EnableWindow( FALSE );
	m_edit_pin_y.EnableWindow( FALSE );
	m_combo_x_edge.EnableWindow(FALSE);
	m_combo_y_edge.EnableWindow(FALSE);
	m_drag_flag = TRUE;
}

void CDlgAddPin::OnBnClickedRadioSetPinPos()
{
	m_edit_pin_x.EnableWindow( TRUE );
	m_edit_pin_y.EnableWindow( TRUE );
	m_combo_x_edge.EnableWindow(m_padstack_type != 1);
	m_combo_y_edge.EnableWindow(m_padstack_type != 1);
	m_drag_flag = FALSE;
}

void CDlgAddPin::OnBnClickedCheckInnerSameAs()
{
	GetFields(); 
	SetFields();
}

void CDlgAddPin::OnBnClickedCheckBottomSameAs()
{
	GetFields();
	SetFields();
}

void CDlgAddPin::OnCbnSelchangeComboPinUnits()
{
	GetFields();
	if( m_combo_units.GetCurSel() == 0 )
		m_units = MIL;
	else
		m_units = MM;
	SetFields();
}

// Set fields in dialog
//
void CDlgAddPin::SetFields()
{
	CString str;
	double denom;
	int max_dp;

	if( m_units == MIL )
		max_dp = 0;
	else
		max_dp =4;

	// "same as pin" stuff
	m_same_as_pin_flag = m_check_same_as_pin.GetCheck(); 
	m_combo_same_as_pin.EnableWindow( m_same_as_pin_flag ); 

	// padstack
	m_edit_hole_diam.EnableWindow( !m_same_as_pin_flag && m_padstack_type == 1 );
	::MakeCStringFromDimension( &str, m_hole_diam, m_units, FALSE, FALSE, FALSE, max_dp );
	m_edit_hole_diam.SetWindowText( str );
	m_radio_smt.SetCheck( m_padstack_type == 0 );
	m_radio_th.SetCheck( m_padstack_type == 1 );

	// top pad
	BOOL bTopEnable = !m_same_as_pin_flag;
	m_radio_smt.EnableWindow( !m_same_as_pin_flag );
	m_radio_th.EnableWindow( !m_same_as_pin_flag );
	m_combo_top_shape.EnableWindow( bTopEnable );
	m_edit_top_width.EnableWindow( bTopEnable );
	m_edit_top_length.EnableWindow( bTopEnable );
	m_edit_top_radius.EnableWindow( bTopEnable );
	m_combo_top_shape2.EnableWindow( bTopEnable );
	m_edit_top_width2.EnableWindow( bTopEnable );
	m_edit_top_length2.EnableWindow( bTopEnable );
	m_edit_top_radius2.EnableWindow( bTopEnable );
	m_combo_top_shape3.EnableWindow( bTopEnable );
	m_edit_top_width3.EnableWindow( bTopEnable );
	m_edit_top_length3.EnableWindow( bTopEnable );
	m_edit_top_radius3.EnableWindow( bTopEnable );
	m_combo_top_shape.SetCurSel( m_top_shape ); 
	m_combo_top_shape2.SetCurSel( ConvertMaskShapeToCurSel(m_top_mask_shape) ); 
	m_combo_top_shape3.SetCurSel( ConvertMaskShapeToCurSel(m_top_paste_shape) ); 
	::MakeCStringFromDimension( &str, m_top_width, m_units, FALSE, FALSE, FALSE, max_dp );
	m_edit_top_width.SetWindowText( str );
	::MakeCStringFromDimension( &str, m_top_length, m_units, FALSE, FALSE, FALSE, max_dp );
	m_edit_top_length.SetWindowText( str );
	::MakeCStringFromDimension( &str, m_top_radius, m_units, FALSE, FALSE, FALSE, max_dp );
	m_edit_top_radius.SetWindowText( str );
	::MakeCStringFromDimension( &str, m_top_mask_width, m_units, FALSE, FALSE, FALSE, max_dp );
	m_edit_top_width2.SetWindowText( str );
	::MakeCStringFromDimension( &str, m_top_mask_length, m_units, FALSE, FALSE, FALSE, max_dp );
	m_edit_top_length2.SetWindowText( str );
	::MakeCStringFromDimension( &str, m_top_mask_radius, m_units, FALSE, FALSE, FALSE, max_dp );
	m_edit_top_radius2.SetWindowText( str );
	::MakeCStringFromDimension( &str, m_top_paste_width, m_units, FALSE, FALSE, FALSE, max_dp );
	m_edit_top_width3.SetWindowText( str );
	::MakeCStringFromDimension( &str, m_top_paste_length, m_units, FALSE, FALSE, FALSE, max_dp );
	m_edit_top_length3.SetWindowText( str );
	::MakeCStringFromDimension( &str, m_top_paste_radius, m_units, FALSE, FALSE, FALSE, max_dp );
	m_edit_top_radius3.SetWindowText( str );
	if( bTopEnable )
	{
		m_edit_top_width.EnableWindow( m_top_shape > PAD_NONE ); 
		m_edit_top_length.EnableWindow( m_top_shape >= PAD_RECT && m_top_shape <= PAD_OVAL );
		m_edit_top_radius.EnableWindow( m_top_shape == PAD_RRECT );
		m_edit_top_width2.EnableWindow( m_top_mask_shape != PAD_DEFAULT && m_top_mask_shape > PAD_NONE ); 
		m_edit_top_length2.EnableWindow( m_top_mask_shape >= PAD_RECT && m_top_mask_shape <= PAD_OVAL );
		m_edit_top_radius2.EnableWindow( m_top_mask_shape == PAD_RRECT );
		m_edit_top_width3.EnableWindow( m_top_paste_shape != PAD_DEFAULT && m_top_paste_shape > PAD_NONE ); 
		m_edit_top_length3.EnableWindow( m_top_paste_shape >= PAD_RECT && m_top_paste_shape <= PAD_OVAL );
		m_edit_top_radius3.EnableWindow( m_top_paste_shape == PAD_RRECT );
	}
	for( int i=0; i<4; i++ )
	{
		m_radio_connect[0][i].SetCheck( 0 );
		m_radio_connect[0][i].EnableWindow( bTopEnable );
	}
	m_radio_connect[0][m_top_flags].SetCheck( 1 );


	// inner pad
	BOOL bInnerEnable = !m_same_as_pin_flag && m_padstack_type == 1;
	m_combo_inner_shape.EnableWindow( bInnerEnable );
	m_edit_inner_width.EnableWindow( bInnerEnable );
	m_edit_inner_length.EnableWindow( bInnerEnable );
	m_edit_inner_radius.EnableWindow( bInnerEnable );
	m_combo_inner_shape.SetCurSel( m_inner_shape ); 
	::MakeCStringFromDimension( &str, m_inner_width, m_units, FALSE, FALSE, FALSE, max_dp );
	m_edit_inner_width.SetWindowText( str );
	::MakeCStringFromDimension( &str, m_inner_length, m_units, FALSE, FALSE, FALSE, max_dp );
	m_edit_inner_length.SetWindowText( str );
	::MakeCStringFromDimension( &str, m_inner_radius, m_units, FALSE, FALSE, FALSE, max_dp );
	m_edit_inner_radius.SetWindowText( str );
	if( bInnerEnable )
	{
		m_edit_inner_width.EnableWindow( m_inner_shape > PAD_NONE ); 
		m_edit_inner_length.EnableWindow( m_inner_shape >= PAD_RECT && m_inner_shape <= PAD_OVAL );
		m_edit_inner_radius.EnableWindow( m_inner_shape == PAD_RRECT );
	}
	for( int i=0; i<4; i++ )
	{
		m_radio_connect[1][i].SetCheck( 0 );
		m_radio_connect[1][i].EnableWindow( bInnerEnable );
	}
	m_radio_connect[1][m_inner_flags].SetCheck( 1 );

	// bottom pad
	BOOL bBottomEnable = !m_same_as_pin_flag; 
	m_check_bottom_same_as.SetCheck( m_bottom_same_as_top ); 
	m_check_bottom_same_as.EnableWindow( bBottomEnable );
	bBottomEnable &= !m_bottom_same_as_top;
	m_combo_bottom_shape.EnableWindow( bBottomEnable );
	m_edit_bottom_width.EnableWindow( bBottomEnable );
	m_edit_bottom_length.EnableWindow( bBottomEnable );
	m_edit_bottom_radius.EnableWindow( bBottomEnable );
	m_combo_bottom_shape2.EnableWindow( bBottomEnable );
	m_edit_bottom_width2.EnableWindow( bBottomEnable );
	m_edit_bottom_length2.EnableWindow( bBottomEnable );
	m_edit_bottom_radius2.EnableWindow( bBottomEnable );
	m_combo_bottom_shape3.EnableWindow( bBottomEnable );
	m_edit_bottom_width3.EnableWindow( bBottomEnable );
	m_edit_bottom_length3.EnableWindow( bBottomEnable );
	m_edit_bottom_radius3.EnableWindow( bBottomEnable );
	m_combo_bottom_shape.SetCurSel( m_bottom_shape ); 
	m_combo_bottom_shape2.SetCurSel( ConvertMaskShapeToCurSel(m_bottom_mask_shape) ); 
	m_combo_bottom_shape3.SetCurSel( ConvertMaskShapeToCurSel(m_bottom_paste_shape) ); 
	::MakeCStringFromDimension( &str, m_bottom_width, m_units, FALSE, FALSE, FALSE, max_dp );
	m_edit_bottom_width.SetWindowText( str );
	::MakeCStringFromDimension( &str, m_bottom_length, m_units, FALSE, FALSE, FALSE, max_dp );
	m_edit_bottom_length.SetWindowText( str );
	::MakeCStringFromDimension( &str, m_bottom_radius, m_units, FALSE, FALSE, FALSE, max_dp );
	m_edit_bottom_radius.SetWindowText( str );
	::MakeCStringFromDimension( &str, m_bottom_mask_width, m_units, FALSE, FALSE, FALSE, max_dp );
	m_edit_bottom_width2.SetWindowText( str );
	::MakeCStringFromDimension( &str, m_bottom_mask_length, m_units, FALSE, FALSE, FALSE, max_dp );
	m_edit_bottom_length2.SetWindowText( str );
	::MakeCStringFromDimension( &str, m_bottom_mask_radius, m_units, FALSE, FALSE, FALSE, max_dp );
	m_edit_bottom_radius2.SetWindowText( str );
	::MakeCStringFromDimension( &str, m_bottom_paste_width, m_units, FALSE, FALSE, FALSE, max_dp );
	m_edit_bottom_width3.SetWindowText( str );
	::MakeCStringFromDimension( &str, m_bottom_paste_length, m_units, FALSE, FALSE, FALSE, max_dp );
	m_edit_bottom_length3.SetWindowText( str );
	::MakeCStringFromDimension( &str, m_bottom_paste_radius, m_units, FALSE, FALSE, FALSE, max_dp );
	m_edit_bottom_radius3.SetWindowText( str );
	if( bBottomEnable )
	{
		m_edit_bottom_width.EnableWindow( m_bottom_shape != PAD_DEFAULT && m_bottom_shape > PAD_NONE ); 
		m_edit_bottom_length.EnableWindow( m_bottom_shape >= PAD_RECT && m_bottom_shape <= PAD_OVAL );
		m_edit_bottom_radius.EnableWindow( m_bottom_shape == PAD_RRECT );
		m_edit_bottom_width2.EnableWindow( m_bottom_mask_shape != PAD_DEFAULT && m_bottom_mask_shape > PAD_NONE ); 
		m_edit_bottom_length2.EnableWindow( m_bottom_mask_shape >= PAD_RECT && m_bottom_mask_shape <= PAD_OVAL );
		m_edit_bottom_radius2.EnableWindow( m_bottom_mask_shape == PAD_RRECT );
		m_edit_bottom_width3.EnableWindow( m_bottom_paste_shape != PAD_DEFAULT && m_bottom_paste_shape > PAD_NONE ); 
		m_edit_bottom_length3.EnableWindow( m_bottom_paste_shape >= PAD_RECT && m_bottom_paste_shape <= PAD_OVAL );
		m_edit_bottom_radius3.EnableWindow( m_bottom_paste_shape == PAD_RRECT );
	}
	for( int i=0; i<4; i++ )
	{
		m_radio_connect[2][i].SetCheck( 0 );
		m_radio_connect[2][i].EnableWindow( bBottomEnable );
	}
	m_radio_connect[2][m_bottom_flags].SetCheck( 1 );

	// position info
	::MakeCStringFromDimension( &str, m_x, m_units, FALSE, FALSE, FALSE, max_dp );
	m_edit_pin_x.SetWindowText( str );
	::MakeCStringFromDimension( &str, m_y, m_units, FALSE, FALSE, FALSE, max_dp );
	m_edit_pin_y.SetWindowText( str );
	::MakeCStringFromDimension( &str, m_row_spacing, m_units, FALSE, FALSE, FALSE, max_dp );
	m_edit_row_spacing.SetWindowText( str );
	
//	Only SM pads have edge alignment:
	if(m_padstack_type != 1)
	{
		m_combo_x_edge.SetCurSel(m_x_edge);
		m_combo_y_edge.SetCurSel(m_y_edge);
	} else {
		m_combo_x_edge.SetCurSel(1);
		m_combo_y_edge.SetCurSel(1);
	}

	m_combo_x_edge.EnableWindow(!m_drag_flag && (m_padstack_type != 1));
	m_combo_y_edge.EnableWindow(!m_drag_flag && (m_padstack_type != 1));
}

// Get data from dialog
//
void CDlgAddPin::GetFields()
{
	CString str;
	int val;
	double mult = NM_PER_MIL;
	if( m_units == MM )
		mult = 1000000.0;

	// get pin name
	m_edit_pin_name.GetWindowText( m_pin_name );
	m_edit_pin_description.GetWindowText( m_pin_description );
	// "same as pin" data
	m_same_as_pin_flag = m_check_same_as_pin.GetCheck();
	if( m_same_as_pin_flag ) 
	{
		// set all parameters to be the same as another pin
		m_combo_same_as_pin.GetWindowText( m_same_as_pin_name );
		int f = m_combo_same_as_pin.FindString( -1, m_same_as_pin_name );
		if( f != -1 )
		{
			m_as_pin = f;
			padstack * copy_ps = &m_fp->m_padstack[m_as_pin];
			if( copy_ps->hole_size )
				m_padstack_type = 1;
			else 
				m_padstack_type = 0;
			m_hole_diam = copy_ps->hole_size;
			m_ps_angle =  copy_ps->angle;
			m_top_shape = copy_ps->top.shape;
			m_top_width = copy_ps->top.size_Y;
			m_top_length = copy_ps->top.size_X;
			if( !m_top_length )
				m_top_length = copy_ps->top.size_Y;
			m_top_radius = copy_ps->top.radius;
			m_top_mask_shape = copy_ps->top_mask.shape;
			m_top_mask_width = copy_ps->top_mask.size_Y;
			m_top_mask_length = copy_ps->top_mask.size_X;
			if( !m_top_mask_length )
				m_top_mask_length = copy_ps->top_mask.size_Y;
			m_top_mask_radius = copy_ps->top_mask.radius;
			m_top_paste_shape = copy_ps->top_paste.shape;
			m_top_paste_width = copy_ps->top_paste.size_Y;
			m_top_paste_length = copy_ps->top_paste.size_X;
			if( !m_top_paste_length )
				m_top_paste_length = copy_ps->top_paste.size_Y;
			m_top_paste_radius = copy_ps->top_paste.radius;
			m_top_flags = copy_ps->top.connect_flag;
			m_inner_shape = copy_ps->inner.shape;
			m_inner_width = copy_ps->inner.size_Y;
			m_inner_length = copy_ps->inner.size_X;
			if( !m_inner_length )
				m_inner_length = copy_ps->inner.size_Y;
			m_inner_radius = copy_ps->inner.radius;
			m_inner_flags = copy_ps->inner.connect_flag;
			m_bottom_shape = copy_ps->bottom.shape;
			m_bottom_width = copy_ps->bottom.size_Y;
			m_bottom_length = copy_ps->bottom.size_X;
			if( !m_bottom_length )
				m_bottom_length = copy_ps->bottom.size_Y;
			m_bottom_radius = copy_ps->bottom.radius;
			m_bottom_mask_shape = copy_ps->bottom_mask.shape;
			m_bottom_mask_width = copy_ps->bottom_mask.size_Y;
			m_bottom_mask_length = copy_ps->bottom_mask.size_X;
			if( !m_bottom_mask_length )
				m_bottom_mask_length = copy_ps->bottom_mask.size_Y;
			m_bottom_mask_radius = copy_ps->bottom_mask.radius;
			m_bottom_paste_shape = copy_ps->bottom_paste.shape;
			m_bottom_paste_width = copy_ps->bottom_paste.size_Y;
			m_bottom_paste_length = copy_ps->bottom_paste.size_X;
			if( !m_bottom_paste_length )
				m_bottom_paste_length = copy_ps->bottom_paste.size_Y;
			m_bottom_paste_radius = copy_ps->bottom_paste.radius;
			m_bottom_flags = copy_ps->bottom.connect_flag;
		}
	}
	else
	{
		m_edit_hole_diam.GetWindowText( str );
		m_hole_diam = mult*atof( str );
		if( m_radio_smt.GetCheck() )
			m_padstack_type = 0;
		else if( m_radio_th.GetCheck() )
			m_padstack_type = 1;

		// top pad
		m_top_shape = m_combo_top_shape.GetCurSel();
		m_edit_top_width.GetWindowText( str );
		m_top_width = mult*atof( str );
		m_edit_top_length.GetWindowText( str );
		m_top_length = mult*atof( str );
		if( !m_top_length )
			m_top_length = m_top_width;
		m_edit_top_radius.GetWindowText( str );
		m_top_radius = mult*atof( str );
		m_top_mask_shape = ConvertMaskCurSelToShape( m_combo_top_shape2.GetCurSel() );
		m_edit_top_width2.GetWindowText( str );
		m_top_mask_width = mult*atof( str );
		m_edit_top_length2.GetWindowText( str );
		m_top_mask_length = mult*atof( str );
		if( !m_top_mask_length )
			m_top_mask_length = m_top_mask_width;
		m_edit_top_radius2.GetWindowText( str );
		m_top_mask_radius = mult*atof( str );
		m_top_paste_shape = ConvertMaskCurSelToShape( m_combo_top_shape3.GetCurSel() );
		m_edit_top_width3.GetWindowText( str );
		m_top_paste_width = mult*atof( str );
		m_edit_top_length3.GetWindowText( str );
		m_top_paste_length = mult*atof( str );
		if( !m_top_paste_length )
			m_top_paste_length = m_top_paste_width;
		m_edit_top_radius3.GetWindowText( str );
		m_top_paste_radius = mult*atof( str );
		for( int i=0; i<4; i++ )
			if( m_radio_connect[0][i].GetCheck() )
			{
				m_top_flags = i;
				break;
			}

		// inner pad
		m_inner_shape = m_combo_inner_shape.GetCurSel();
		m_edit_inner_width.GetWindowText( str );
		m_inner_width = mult*atof( str );
		m_edit_inner_length.GetWindowText( str );
		m_inner_length = mult*atof( str );
		if( !m_inner_length )
			m_inner_length = m_inner_width;
		m_edit_inner_radius.GetWindowText( str );
		m_inner_radius = mult*atof( str );
		for( int i=0; i<4; i++ )
			if( m_radio_connect[1][i].GetCheck() )
			{
				m_inner_flags = i;
				break;
			}

		// bottom pad
		m_bottom_same_as_top = m_check_bottom_same_as.GetCheck();
		if( m_bottom_same_as_top )
		{
			m_bottom_shape = m_top_shape;
			m_bottom_width = m_top_width;
			m_bottom_length = m_top_length;
			m_bottom_radius = m_top_radius;
			m_bottom_mask_shape = m_top_mask_shape;
			m_bottom_mask_width = m_top_mask_width;
			m_bottom_mask_length = m_top_mask_length;
			m_bottom_mask_radius = m_top_mask_radius;
			m_bottom_paste_shape = m_top_paste_shape;
			m_bottom_paste_width = m_top_paste_width;
			m_bottom_paste_length = m_top_paste_length;
			m_bottom_paste_radius = m_top_paste_radius;
			m_bottom_flags = m_top_flags;
		}
		else
		{
			m_bottom_shape = m_combo_bottom_shape.GetCurSel();
			m_edit_bottom_width.GetWindowText( str );
			m_bottom_width = mult*atof( str );
			m_edit_bottom_length.GetWindowText( str );
			m_bottom_length = mult*atof( str );
			if( !m_bottom_length )
				m_bottom_length = m_bottom_width;
			m_edit_bottom_radius.GetWindowText( str );
			m_bottom_radius = mult*atof( str );
			m_bottom_mask_shape = ConvertMaskCurSelToShape( m_combo_bottom_shape2.GetCurSel() );
			m_edit_bottom_width2.GetWindowText( str );
			m_bottom_mask_width = mult*atof( str );
			m_edit_bottom_length2.GetWindowText( str );
			m_bottom_mask_length = mult*atof( str );
			if( !m_bottom_mask_length )
				m_bottom_mask_length = m_bottom_mask_width;
			m_edit_bottom_radius2.GetWindowText( str );
			m_bottom_mask_radius = mult*atof( str );
			m_bottom_paste_shape = ConvertMaskCurSelToShape( m_combo_bottom_shape3.GetCurSel() );
			m_edit_bottom_width3.GetWindowText( str );
			m_bottom_paste_width = mult*atof( str );
			m_edit_bottom_length3.GetWindowText( str );
			m_bottom_paste_length = mult*atof( str );
			if( !m_bottom_paste_length )
				m_bottom_paste_length = m_bottom_paste_width;
			m_edit_bottom_radius3.GetWindowText( str );
			m_bottom_paste_radius = mult*atof( str );
			for( int i=0; i<4; i++ )
				if( m_radio_connect[2][i].GetCheck() )
				{
					m_bottom_flags = i;
					break;
				}
		}
	}

	// position and spacing
	m_edit_pin_x.GetWindowText( str );
	m_x = mult*atof( str );
	m_edit_pin_y.GetWindowText( str );
	m_y = mult*atof( str );
	m_edit_row_spacing.GetWindowText( str );
	m_row_spacing = mult*atof( str );

	m_x_edge = m_combo_x_edge.GetCurSel();
	m_y_edge = m_combo_y_edge.GetCurSel();

	// number of pins to add
	if( m_radio_add_pin.GetCheck() )
		m_num_pins = 1;
	else
	{
		m_edit_num_pins.GetWindowText( str );
		m_num_pins = atoi( str );
		m_edit_increment.GetWindowText( str );
		m_increment = atoi( str );
	}

	// row orientation
	m_row_orient = m_combo_row_orient.GetCurSel();
}

void CDlgAddPin::OnChange()
{
	GetFields();
	SetFields();
}

void CDlgAddPin::OnChangeTopPadParam()
{
	if( m_check_bottom_same_as.GetCheck() )
	{
		CString str;
		m_edit_top_width.GetWindowText( str );
		m_edit_bottom_width.SetWindowText( str );
		m_edit_top_length.GetWindowText( str );
		m_edit_bottom_length.SetWindowText( str );
		m_edit_top_radius.GetWindowText( str );
		m_edit_bottom_radius.SetWindowText( str );
		m_edit_top_width2.GetWindowText( str );
		m_edit_bottom_width2.SetWindowText( str );
		m_edit_top_length2.GetWindowText( str );
		m_edit_bottom_length2.SetWindowText( str );
		m_edit_top_radius2.GetWindowText( str );
		m_edit_bottom_radius2.SetWindowText( str );
		m_edit_top_width3.GetWindowText( str );
		m_edit_bottom_width3.SetWindowText( str );
		m_edit_top_length3.GetWindowText( str );
		m_edit_bottom_length3.SetWindowText( str );
		m_edit_top_radius3.GetWindowText( str );
		m_edit_bottom_radius3.SetWindowText( str );
	}
}

void CDlgAddPin::OnBnClickedCheckBottomSameAs2()
{
	// TODO: добавьте свой код обработчика уведомлений
}
