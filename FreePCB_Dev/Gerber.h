// make Gerber file
#include "stdafx.h"

enum {
	GERBER_BOARD_OUTLINE = 0x1,
	GERBER_AUTO_MOIRES = 0x2,
	GERBER_LAYER_TEXT = 0x4,
	GERBER_PILOT_HOLES = 0x8,
	GERBER_NO_PIN_THERMALS = 0x10,
	GERBER_NO_VIA_THERMALS = 0x20,
	GERBER_MASK_VIAS = 0x40,
	GERBER_90_THERMALS = 0x80,
	GERBER_RENDER_ALL = 0x100,
	GERBER_RENDER_TOP_BOTTOM = 0x200,
	GERBER_MIRROR_BOTTOM_PNG = 0x400,
	GERBER_NO_SMT_THERMALS = 0x800,
};

class CAperture;

typedef CArray<CAperture,CAperture> aperture_array;

class CAperture
{
public:
	enum {
		AP_NONE = 0,
		AP_CIRCLE,
		AP_SQUARE,
		AP_RECT,
		AP_RRECT,
		AP_MOIRE,
		AP_THERMAL,
		AP_RECT_THERMAL,
		AP_RECT_THERMAL_45,
		AP_OCTAGON,
		AP_OVAL
	};
	int m_type;						// type of aperture, see enum above
	int m_size1, m_size2, m_size3, m_size4;	// in NM
	//	Width	 Height	  ThermW   ThermClearance
	//	Width	 Height	  Radius
	CAperture();
	CAperture( int type, int size1, int size2, int size3=0, int size4=0 );
	~CAperture();
	BOOL Equals( CAperture * ap );
	int FindInArray( aperture_array * ap_array, BOOL ok_to_add=FALSE );
};

int WriteGerberFile( CStdioFile * f, 
					int flags, 
					int layer, 
					CDlgLog * log, 
					int paste_mask_shrink,
					int fill_clearance, 
					int mask_clearance, 
					int pilot_diameter,
					int min_silkscreen_stroke_wid, 
					int highlight_width, 
					int thermal_wid, 
					int hole_clearance, 
					int thermal_clearance,
					int n_x, 
					int n_y, 
					int step_x, 
					int step_y,
					CArray<CPolyLine> * op, 
					CPartList * pl, 
					CNetList * nl, 
					CTextList * tl, 
					CDisplayList * dl,
					Merge * ml ); 
int WriteDrillFile( CStdioFile * file, CPartList * pl, CNetList * nl, CArray<CPolyLine> * bd,
				   int n_x=1, int n_y=1, int offset_x=0, int offset_y=0 );

