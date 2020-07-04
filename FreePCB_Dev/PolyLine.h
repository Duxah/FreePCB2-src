// PolyLine.h ... definition of CPolyLine class
//
// A polyline contains one or more contours, where each contour
// is defined by a list of corners and side-styles
// There may be multiple contours in a polyline.
// The last contour may be open or closed, any others must be closed.
// All of the corners and side-styles are concatenated into 2 arrays,
// separated by setting the end_contour flag of the last corner of 
// each contour.
//
// When used for copper areas, the first contour is the outer edge 
// of the area, subsequent ones are "holes" in the copper.
//
// If a CDisplayList pointer is provided, the polyline can draw itself 

#pragma once
#include <afxcoll.h>
#include <afxtempl.h>
#include "DisplayList.h"
#include "gpc_232.h"

class polygon;

class CArc {
public: 
	enum{ MAX_STEP = 50*25400 };	// max step is 20 mils
	enum{ MIN_STEPS = 18 };			// min step is 5 degrees
	int style;
	int xi, yi, xf, yf;
	int n_steps;	// number of straight-line segments in gpc_poly 
	BOOL bFound;
};

class CPolyPt
{
public:
	CPolyPt( int qx=0, int qy=0, BOOL qf=FALSE )
	{ x=qx; y=qy; num_contour=qf; utility = 0; };
	int x;
	int y;
	int num_contour;
	int utility;
};

class CContourData
{
public:
	int start_c;
	int end_c;
	RECT rect_cont;
};

class CPolyLine
{
public:
	enum { STRAIGHT, ARC_CW, ARC_CCW };	// side styles
	enum { NO_HATCH, DIAGONAL_FULL, DIAGONAL_EDGE }; // hatch styles
	enum { DEF_SIZE = 50, DEF_ADD = 50 };	// number of array elements to add at a time
	CArray <BOOL> select_c;				// flag selecded corner
	CArray <BOOL> select_s;				// flag selecded side
	CArray <dl_element*> dl_side;		// graphic elements
	CArray <dl_element*> dl_corner_sel;
	BOOL bDrawn;
	// constructors/destructor
	CPolyLine( CDisplayList * dl );
	CPolyLine();
	~CPolyLine();

	// functions for modifying polyline
	void Start( int layer, int w, int sel_box, int x, int y,
		int hatch, id * id, void * ptr, int SIZE = 0 );
	void AppendCorner( int x, int y, int style = STRAIGHT, BOOL bDraw=TRUE );
	void InsertCorner( int ic, int x, int y, BOOL bDraw=TRUE );
	void DeleteCorner( int ic, int nc=1, BOOL Rec=TRUE, BOOL bDraw=TRUE );
	void MoveCorner( int ic, int x, int y, BOOL bDraw=TRUE );
	void Close( int style = STRAIGHT, BOOL bDraw=TRUE, BOOL bSetSize=TRUE );
	void UnClose();
	void RemoveContour( int icont, BOOL bDraw=TRUE );
	void RecalcRectC( int icont );

	// drawing functions
	void HighlightSide( int is, int w=0, int bTRANSPARENT=0 );
	dl_element * HighlightCorner( int ic, int w=0 );
	void StartDraggingToInsertCorner( CDC * pDC, int ic, int x, int y, int crosshair = 1 );
	void StartDraggingToMoveCorner( CDC * pDC, int ic, int x, int y, int crosshair = 1 );
	void CancelDraggingToInsertCorner( int ic );
	void CancelDraggingToMoveCorner( int ic );
	void Undraw();
	void Draw( CDisplayList * dl = NULL );
	void Hatch();
	void TransparentHatch( int bTRANSPARENT );
	void MakeVisible( BOOL visible = TRUE );
	void MoveOrigin( int x_off, int y_off );
	void SetSideVisible( int is, int visible );

	// misc. functions
	RECT GetBounds();
	RECT GetCornerBounds( int icont );
	void Copy( CPolyLine * src );
	BOOL TestPointInside( int x, int y, int num_c = -1 );
	BOOL TestPointInsideContour( int x, int y );
	int TestPolygonIntersection( CPolyLine * poly2 );
	int TestPolygonIntersection( int test_contour, int test_corner ); 
	void AppendArc( int xi, int yi, int xf, int yf, int xc, int yc, int num );
	void ApproximateAllArcSides();

	// access functions
	int GetLength();
	int GetSel(int i);
	int GetSideSel(int i);
	int GetMerge();
	int GetNumCorners();
	int GetNumSides();
	int GetNumArcs();
	int GetClosed();
	int GetNumContours();
	int GetContour( int ic );
	int GetContourStart( int icont );
	int GetContourEnd( int icont );
	int GetIndexCornerBack( int crnr );
	int GetIndexCornerNext( int crnr );
	int GetContourSize( int icont );
	int GetX( int ic );
	int GetY( int ic );
	int GetHatchSize ();
	int GetHatch(){ return m_hatch; }
	int GetNumContour( int ic );
	int GetVisible ();
	int GetUtility(){ return utility; };
	int GetUtility( int ic ){ return corner[ic].utility; };
	int GetLayer();
	int GetW();
	int GetSideStyle( int is );
	int GetSelBoxSize();
	id  GetId();
	void * GetPtr(){ return m_ptr; };
	RECT GetHatchLoc( int index );
	CDisplayList * GetDisplayList(){ return m_dlist; };
	
	void SetX( int ic, int x );
	void SetY( int ic, int y );
	void SetEndContour( int ic, BOOL end_contour );
	void SetUtility( int u ){ utility = u; };
	void SetUtility( int ic, int utility ){ corner[ic].utility = utility; };
	void SetLayer( int layer );
	void SetW( int w );
	void SetVisible ( int visibe );
	void SetSideStyle( int is, int style, BOOL DRAW=TRUE );
	void SetId( id * id );
	void SetPtr( void * ptr ){ m_ptr = ptr; };
	void SetSelBoxSize( int sel_box );
	void SetHatch( int hatch ){ Undraw(); m_hatch = hatch; Draw(); };
	void SetDisplayList( CDisplayList * dl );
	void SetMerge(int merge);
	void SetSideSel(int i, BOOL sel);
	void SetSel(int i, BOOL sel);

	// GPC functions
	int MakeGpcPoly( int icontour=0, CArray<CArc> * arc_array=NULL );
	int FreeGpcPoly();
	gpc_polygon * GetGpcPoly(){ return m_gpc_poly; };
	int NormalizeWithGpc( CArray<CPolyLine*> * pa=NULL, BOOL bRetainArcs=FALSE );
	int RestoreArcs( CArray<CArc> * arc_array, CArray<CPolyLine*> * pa=NULL );
	CPolyLine * MakePolylineForPad( int type, int x, int y, int w, int l, int r, int angle );
	void ClipGpcPolygon( gpc_op op, CPolyLine * poly );

	// PHP functions
	int MakePhpPoly();
	void FreePhpPoly();
	void ClipPhpPolygon( int php_op, CPolyLine * poly );

private:
	CDisplayList * m_dlist;		// display list 
	id m_id;		// root id
	void * m_ptr;	// pointer to parent object (or NULL)
	int m_layer;	// layer to draw on
	int m_w;		// line width
	int m_sel_box;	// corner selection box width/2
	int m_ncorners;	// number of corners
	int m_visible;	// visible sides
	int utility;
	int m_merge;    // merge with other obj
	
	CArray <CPolyPt> corner;			// array of points for corners
	CArray <CContourData> cont_data;	// array of contours data: start cor, end cor, rect cont
	CArray <int> side_style;			// array of styles for sides
	int m_hatch;	// hatch style, see enum above
	int m_nhatch;	// number of hatch lines
	CArray <dl_element*>  dl_hatch;	// hatch lines	
	gpc_polygon * m_gpc_poly;	// polygon in gpc format
	polygon * m_php_poly;
};
