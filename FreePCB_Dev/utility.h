// utility routines
//
#pragma once 

typedef struct PointTag
{
	double X,Y;
} Point;

typedef struct EllipseTag
{
	Point Center;			/* ellipse center	 */
//	double MaxRad,MinRad;	/* major and minor axis */
//	double Phi;				/* major axis rotation  */
	double xrad, yrad;		// radii on x and y
	double theta1, theta2;	// start and end angle for arc 
} EllipseKH;

const CPoint zero(0,0);
const CPoint pdc(2540,2540);

class my_circle {
public:
	my_circle(){};
	my_circle( int xx, int yy, int rr )
	{
		x = xx;
		y = yy;
		r = rr;
	};
	int x, y, r; 
};

class my_rect {
public:
	my_rect(){};
	my_rect( int xi, int yi, int xf, int yf )
	{
		xlo = min(xi,xf);
		xhi = max(xi,xf);
		ylo = min(yi,yf);
		yhi = max(yi,yf);
	};
	int xlo, ylo, xhi, yhi; 
};

class my_seg { 
public:
	my_seg(){};
	my_seg( int xxi, int yyi, int xxf, int yyf )
	{
		xi = xxi;
		yi = yyi;
		xf = xxf;
		yf = yyf;
	};
	int xi, yi, xf, yf; 
};

// map part angle to reported part angle
int GetReportedAngleForPart( int part_angle, int cent_angle, int side );
int GetPartAngleForReportedAngle( int angle, int cent_angle, int side );

// handle strings
char * mystrtok( LPCTSTR str, LPCTSTR delim );
double GetDimensionFromString( CString * str, int def_units=MIL, BOOL bRound10=TRUE );
void MakeCStringFromDimension( CString * str, int dim, int units, BOOL append_units=TRUE, 
							  BOOL lower_case = FALSE, BOOL space=FALSE, int max_dp=8, BOOL strip=TRUE );
void MakeCStringFromDouble( CString * str, double d );
BOOL CheckLegalPinName( CString * pinstr, 
					   CString * astr=NULL, 
					   CString * nstr=NULL, 
					   int * n=NULL );
int ParseRef( CString * ref, CString * prefix );
void SetGuidFromString( CString * str, GUID * guid  );
void GetStringFromGuid( GUID * guid, CString * str );
BOOL SplitString( CString * str, CString * a, CString * b, char split_at, BOOL bReverseFind=FALSE );

// for profiling
void CalibrateTimer();
void StartTimer();
double GetElapsedTime();

// math stuff for graphics
int ccw( int angle );
int sign(int thing);
int Rnd_Func( float xn, float yn, float x2, float y2, float x1, float y1, float xb, float yb, float * PTS, int PtsMaxValue);
BOOL Quadratic( double a, double b, double c, double *x1, double *x2 );
void DrawArc( CDC * pDC, int shape, int xxi, int yyi, int xxf, int yyf, BOOL bMeta=FALSE );
void DrawCurve( CDC * pDC, int shape, int xxi, int yyi, int xxf, int yyf, BOOL bMeta=FALSE );
void RotatePoint( CPoint *p, float angle, CPoint org );
void RotatePOINTS( CPoint * p, int np,  float angle, CPoint org );
void MirrorRect( RECT * rect, CPoint org );
void RotateRect( RECT * r, int angle, CPoint org );
void RotateRect( int * left, int * right, int * bottom, int * top, int angle, int orgX, int orgY );
void SwellRect( RECT *r, int swValue );
void SwellRect( RECT *r, RECT swell );
void SwellRect( RECT *r, CPoint Po );
void SwellRect( RECT *r, int x, int y );
void MoveRect( RECT *r, int dx, int dy );
RECT rect(int xi, int yi, int xf, int yf);
int SwellPolygon( CPoint * P, int np, CPoint * newP, int swValue );
int TestPolygon( int xi, int yi, CPoint * P, int np );

int RectsIntersection ( RECT r1, RECT r2 );
BOOL Colinear( int x1, int y1, int x2, int y2, int x3, int y3 );
int FindArcElements ( cconnect * c, int * i );
int FindArcElements ( CPolyLine * p, int * i, BOOL * EndFlag );
BOOL Check_45 ( CPoint pv, CPoint v );
BOOL Check_90 ( CPoint pv, CPoint v );
float Angle ( CPoint pv, CPoint v, CPoint nv ); 
// import from 'Copper Areas Splitter'
	float Angle (float dx, float dy, float x0, float y0, BOOL b_0_360=TRUE );
	void Rotate_Vertex (float *X, float *Y, float Ang);
	void Rotate_i_Vertex (int *X, int *Y, int Ang, int orgx, int orgy );
	int Generate_Arc (int xi, int yi, int xf, int yf,  int type_L, CPoint * OutPut, int n_sides );
	int TripplePointArc (int xi, int yi, int xf, int yf, int xc, int yc, CPoint * OutPut, int npoints );
	int Gen_RndRectPoly (int x, int y, int dx, int dy, float rad, float ang, CPoint * OutPut, int npoints );
	int Gen_HollowLinePoly (int xi, int yi, int xf, int yf, int w, CPoint * OutPut, int npoints );
//
int TestLineHit( int xi, int yi, int xf, int yf, int x, int y, double dist );
int FindLineIntersection( double a, double b, double c, double d, double * x, double * y );
int FindLineIntersection( double x0, double y0, double x1, double y1,
						  double x2, double y2, double x3, double y3,
						  double *linx, double *liny);
int FindLineSegmentIntersection( double a, double b, int xi, int yi, int xf, int yf, int style, 
				double * x1, double * y1, double * x2, double * y2, double * dist=NULL );
int FindSegmentIntersections( int xi, int yi, int xf, int yf, int style, 
								 int xi2, int yi2, int xf2, int yf2, int style2,
								 double x[]=NULL, double y[]=NULL );
BOOL FindLineEllipseIntersections( double a, double b, double c, double d, double *x1, double *x2 );
BOOL FindVerticalLineEllipseIntersections( double a, double b, double x, double *y1, double *y2 );
BOOL TestForIntersectionOfStraightLineSegments( int x1i, int y1i, int x1f, int y1f, 
									   int x2i, int y2i, int x2f, int y2f,
									   int * x=NULL, int * y=NULL, double * dist=NULL );
void GetPadElements( int type, int x, int y, int wid, int len, int radius, int angle,
					int * nr, my_rect r[], int * nc, my_circle c[], int * ns, my_seg s[] );
int GetClearanceBetweenPads( int type1, int x1, int y1, int w1, int l1, int r1, int angle1,
							 int type2, int x2, int y2, int w2, int l2, int r2, int angle2 );
int GetClearanceBetweenSegmentAndPad(	int x1, int y1, int x2, int y2, int w,
										int type, int x, int y, int wid, int len, 
										int radius, int angle );
int GetClearanceBetweenSegments(	int x1i, int y1i, int x1f, int y1f, int style1, int w1,
									int x2i, int y2i, int x2f, int y2f, int style2, int w2,
									int max_cl, int * x, int * y );
double GetPointToLineSegmentDistance( int x, int y, int xi, int yi, int xf, int yf );
double GetPointToLineDistance( double a, double b, int x, int y, double * xpp=NULL, double * ypp=NULL );
BOOL InRange( double x, double xi, double xf );
double Distance( int x1, int y1, int x2, int y2 );
CPoint AlignPoints (CPoint p, CPoint pback, CPoint pnext, BOOL CH, float SwitchAngle=7.5);
int GetArcIntersections( EllipseKH * el1, EllipseKH * el2, 
						double * x1=NULL, double * y1=NULL, 
						double * x2=NULL, double * y2=NULL );						
CPoint GetInflectionPoint( CPoint pi, CPoint pf, int mode );

// quicksort (2-way or 3-way)
void quickSort(int numbers[], int index[], int array_size);
void q_sort(int numbers[], int index[], int left, int right);
void q_sort_3way( int a[], int b[], int left, int right );

BYTE getbit( int rgstr, int bit );