// NetList.h: interface for the CNetList class.
//
// This is a basically a map of nets
//	- each net is represented by an cnet object
//	- each cnet is mapped by its name
//	- each cnet contains arrays of pins, connections between pins, and copper areas
//	- each pin is represented by a cpin object
//	- each connection is represented by a cconnect object 
//  - each cconnect contains arrays of segments and vertices between segments 
//	- each segment is represented by a cseg object
//	- each vertex is represented by a cvertex object
//	- each copper area is represented by a carea object
//
/////////////////////////////////////////////////////////////////////// 

#pragma once
#include <afxcoll.h>
#include <afxtempl.h>
#include "ids.h"
#include "DisplayList.h"
#include "Merge.h"
#include "PartList.h"
#include "PolyLine.h"
#include "UndoList.h"
#include "LinkList.h"
#include "Cuid.h"

// global Cuid for netlist classes
static Cuid nl_cuid;

class cnet;
class CNetList;

#define MAX_NET_NAME_SIZE 39

// these definitions are for ImportSessionFile()
//
enum NODE_TYPE { NONE, NPIN, NVIA, NJUNCTION };

typedef class {
public:
	NODE_TYPE type;
	int x, y, layer, via_w;
	BOOL bUsed;
	int pin_index;	// if type == NPIN
	CArray<int> path_index;
	CArray<int> path_end;  // 0 or 1
} cnode;

typedef class {
public:
	int x, y, inode;
} cpath_pt;

typedef class {
public:
	// return inode at end of path
	int GetInode( int iend )
	{ 
		int last_pt = pt.GetSize()-1;
		if(iend)
			return pt[last_pt].inode; 
		else 
			return pt[0].inode; 
	};
	// member variables
	int layer, width;
	CArray<cpath_pt> pt;
	int n_used;		// number of times used
} cpath;
//
// end definitions for ImportSessionFile()


// these structures are used for undoing 
struct undo_pin {
	char ref_des[MAX_REF_DES_SIZE+1];
	char pin_name[CShape::MAX_PIN_NAME_SIZE+1];
};

struct undo_corner {
	int x, y, num_contour, style;	// style is for following side
};

struct undo_area {
	int size;
	CNetList * nlist;
	char net_name[MAX_NET_NAME_SIZE+1];
	int merge_name;
	int iarea;
	int layer;
	int hatch;
	int w;
	int sel_box_w;
	int ncorners;
	// array of undo_corners starts here
};

struct undo_seg {
	int uid;
	int layer;				// copper layer
	int width;				// width
	int via_w, via_hole_w;	// via width and hole width
};

struct undo_vtx {
	int uid;
	int x, y;				// coords
	int pad_layer;			// layer of pad if this is first or last vertex
	int force_via_flag;		// force a via even if no layer change
	int tee_ID;				// identifier for t-connection
	int via_w, via_hole_w;	// via width and hole width (via_w==0 means no via)
	int micro;
};

struct undo_con {
	int uid;
	int size;
	CNetList * nlist;
	char net_name[MAX_NET_NAME_SIZE+1];
	int merge_name;
	int start_pin, end_pin;		// indexes into net.pin array
	//CPoint start_pt, end_pt;
	int nsegs;					// # elements in seg array
	int locked;					// 1 if locked (will not be optimized away)
	int set_areas_flag;			// 1 to force setting of copper areas
	int seg_offset;				// offset to array of segments
	int vtx_offset;				// offset to array of vertices
	// array of undo_seg structs goes here
	// followed by array of undo_vtx structs
};

struct undo_net {
	int size;
	CNetList * nlist;
	char name[MAX_NET_NAME_SIZE+1];
	int npins;
	// array of undo_pin structs start here
};

// net_info structure
// used as a temporary copy of net info for editing in dialogs
// or importing/exporting netlists
struct net_info {
	CString name;
	cnet * net;
	BOOL visible;
	int w;
	int v_w;
	int v_h_w;
	BOOL apply_trace_width;
	BOOL apply_via_width;
	BOOL deleted;
	BOOL modified;
	CArray<CString> ref_des;
	CArray<CString> pin_name;
};

// netlist_info is an array of net_info for each net
typedef CArray<net_info> netlist_info;

// carea: describes a copper area
class carea
{
public:
	carea();
	carea( const carea& source );	// dummy copy constructor
	~carea();						// destructor
	carea &operator=( carea &a );	// dummy assignment operator
	void Initialize( CDisplayList * dlist );
	CPolyLine * poly;	// outline
	int npins;			// number of thru-hole pins within area on same net
	CArray<int> pin;	// array of thru-hole pins
	CArray<dl_element*> dl_thermal;	// graphics for thermals on pins
	int nvias;			// number of via connections to area
	CArray<int> vcon;	// connections 
	CArray<int> vtx;	// vertices
	CArray<dl_element*> dl_via_thermal; // graphics for thermals on stubs
	CDisplayList * m_dlist;
	int utility, utility2, selected;
};

// cpin: describes a pin in a net
class cpin
{
public:
	cpin(){ part = NULL; };
	CString ref_des;	// reference designator such as 'U1'
	CString pin_name;	// pin name such as "1" or "A23"
	cpart * part;		// pointer to part containing the pin
	int utility;
};

// cseg: describes a segment of a connection
class cseg
{
public:
	cseg()
	{
		// constructor
		m_uid = nl_cuid.GetNewUID();
		m_dlist = 0;  // this must be filled in with Initialize()
		m_nlist = 0;  // this must be filled in with Initialize()
		layer = 0;
		width = 0;
		selected = 0;
		dl_el = 0;
		utility = 0;
	}
	~cseg()
	{
		// destructor
		nl_cuid.ReleaseUID( m_uid );
		if( m_dlist )
		{
			if( dl_el )
				m_dlist->Remove( dl_el );
		}
	}
	void Initialize( CDisplayList * dlist, CNetList * nlist )
	{
		m_dlist = dlist;
		m_nlist = nlist;
	}
	int m_uid;				// unique id
	int layer;				// copper layer
	int width;				// width
	BOOL selected;			// 1 if selected for editing
	dl_element * dl_el;		// display element for segment
	CDisplayList * m_dlist;
	CNetList* m_nlist;
	int utility;
};

// cvertex: describes a vertex between segments
class cvertex
{
public:
	cvertex()
	{
		// constructor
		m_uid = nl_cuid.GetNewUID();
		selected = 0;
		m_dlist = 0;	// this must set with Initialize()
		m_nlist = 0;	// this must set with Initialize()
		x = 0; y = 0;
		pad_layer = 0;	// only for first or last 
		force_via_flag = 0;		// only used for end of stub trace
		via_w = 0; 
		via_hole_w = 0;
		dl_el = NULL;
		dl_hole = NULL;
		tee_ID = 0;
		utility = 0;
		utility2 = 0;
		// range via
		layer_bit = 0;
		m_micro = 0;
	}
	~cvertex()
	{
		// destructor
		nl_cuid.ReleaseUID( m_uid );
		if( m_dlist )
		{
			m_dlist->Remove( dl_el );
			m_dlist->Remove( dl_hole );
		}
	}
	cvertex &operator=( cvertex &v )	// assignment operator
	{
		// copy all params
		m_uid = v.m_uid;
		selected = v.selected;
		x = v.x;
		y = v.y;
		pad_layer = v.pad_layer;
		force_via_flag = v.force_via_flag;
		via_w = v.via_w;
		via_hole_w = v.via_hole_w;
		m_dlist = v.m_dlist;
		tee_ID = v.tee_ID;
		utility = v.utility;
		utility2 = v.utility2;
		layer_bit = v.layer_bit;
		m_micro = v.m_micro;

		// copy dl_elements and and remove from source
		// they still need to be renumbered

		// dl_hole 
		dl_hole = v.dl_hole;
		v.dl_hole = NULL;

		// dl_el
		dl_el = v.dl_el;
		v.dl_el = NULL;

		return *this;
	};
	void Initialize( CDisplayList * dlist, CNetList * nlist )
	{
		m_dlist = dlist;	// this must be filled in with Initialize()
		m_nlist = nlist;	// this must be filled in with Initialize()
	}
	int m_uid;					// unique id
	BOOL selected;				// TRUE if selected
	int x, y;					// coords
	int pad_layer;				// layer of pad if this is first or last vertex, otherwise 0
	int force_via_flag;			// force a via even if no layer change
	int via_w, via_hole_w;		// via width and hole width (via_w==0 means no via)
	// via range
	int layer_bit;				
	int m_micro;
	// display el
	dl_element * dl_el;		// array of display elements for each layer
	dl_element * dl_hole;	// hole in via
	CDisplayList * m_dlist;
	CNetList * m_nlist;
	int tee_ID;					// used to flag a t-connection point
	int utility, utility2;		// used for various functions
};

// cconnect: describes a connection between two pins or a stub trace with no end pin
class cconnect
{
public:
	enum {
		NO_END = -1		// used for end_pin if stub trace
	};
	cconnect()
	{ 
		m_uid = nl_cuid.GetNewUID();
		m_nlist = NULL;
		locked = 0;
		nsegs = 0;
		m_selected = 0;
		seg.SetSize( 0 );
		vtx.SetSize( 0 );
		utility = 0;
	}
	~cconnect()
	{
		nl_cuid.ReleaseUID( m_uid );
	}
	void Initialize( CNetList * nlist )
	{
		m_nlist = nlist;
	}
	int start_pin, end_pin;		// indexes into net.pin array
	int start_pin_shape, end_pin_shape; // indexes into part.pin array ( shape.m_pad_stack )
	int nsegs;					// # elements in seg array
	int locked;					// 1 if locked (will not be optimized away)
	BOOL m_selected;			// flag
	CArray<cseg> seg;			// array of segments
	CArray<cvertex> vtx;		// array of vertices, size = nsegs + 1
	int utility;				// used for various temporary ops
	// these params used only by DRC
	int min_x, max_x;			// bounding rect
	int min_y, max_y;
	BOOL vias_present;			// flag to indicate that vias are pesent
	int seg_layers;				// mask for all layers used by segments
	CNetList * m_nlist;
	int m_uid;					// unique id
	int m_merge;			    // merge with other obj
};

// cnet: describes a net
class cnet
{
public:
	cnet( CDisplayList * dlist ){ m_dlist = dlist; }
	id id;				// net id
	CString name;		// net name
	int nconnects;		// number of connections
	CArray<cconnect> connect; // array of connections (size = max_pins-1)
	int npins;			// number of pins
	CArray<cpin> pin;	// array of pins
	int nareas;			// number of copper areas
	CArray<carea,carea> area;	// array of copper areas
	int def_w;			// default trace width
	int def_via_w;		// default via width
	int def_via_hole_w;	// default via hole width
	BOOL visible;		// FALSE to hide ratlines and make unselectable
	int selected;		// true if selected element of net
	int utility;		// used to keep track of which nets have been optimized
	int utility2;		// used to keep track of which nets have been optimized
	CDisplayList * m_dlist;
};

// CNetlist
class CNetList  
{
public:
	enum{ MAX_ITERATORS=10 };
	enum {
		VIA_NO_CONNECT = 0,
		VIA_TRACE = 1,
		VIA_AREA = 2,
		VIA_INSIDE_AREA_FULL = 4
	};
	enum {						// used for UNDO records
		UNDO_CONNECT_MODIFY=1,	// undo modify connection
		UNDO_AREA_CLEAR_ALL,	// flag to remove all areas
		UNDO_AREA_ADD,			// undo add area (i.e. delete area)
		UNDO_AREA_MODIFY,		// undo modify area
		UNDO_AREA_DELETE,		// undo delete area (i.e. add area) 
		UNDO_NET_ADD,			// undo add net (i.e delete net)
		UNDO_NET_MODIFY,		// undo modify net
		UNDO_NET_OPTIMIZE		// flag to optimize net on undo
	};
	CMapStringToPtr m_map;	// map net names to pointers
	CNetList( CDisplayList * dlist, CPartList * plist );
	~CNetList();
	void SetNumCopperLayers( int layers ){ m_layers = layers;};
	int GetNumCopperLayers(){ return m_layers;};
	void SetWidths( int w, int via_w, int via_hole_w );
	void SetViaAnnularRing( int ring ){ m_annular_ring = ring; };
	void SetSMTconnect( BOOL bSMTconnect ){ m_bSMT_connect = bSMTconnect; };
	void AddHighlightLines( int X, int Y, int mode );
	void SetMerge( cnet * n, id m_id );
	void SetPartList( CPartList * plist );
	CString GetMerge( cnet * n, id m_id );

	// functions for nets and pins
	cnet * SplitNet( cnet * net, int iconnect, int STEP=0 );
	void MarkAllNets( int utility );
	void MoveOrigin( int x_off, int y_off );
	cnet * GetNetPtrByName( CString * name );
	cnet * AddNet( CString name, int def_width, int def_via_w, int def_via_hole_w );
	void RemoveNet( cnet * net );
	void RemoveAllNets();
	void AddNetPin( cnet * net, CString * ref_des, CString * pin_name, BOOL set_areas=TRUE );
	void RemoveNetPin( cnet * net, CString * ref_des, CString * pin_name, BOOL bSetAreas=TRUE );
	void RemoveNetPin( cnet * net, int net_pin_index, BOOL bSetAreas=TRUE, BOOL bSetPartData=TRUE );
	void DisconnectNetPin( cpart * part, CString * pin_name, BOOL bSetAreas=TRUE );
	void DisconnectNetPin( cnet * net, CString * ref_des, CString * pin_name, BOOL bSetAreas=TRUE );
	int GetNetPinIndex( cnet * net, CString * ref_des, CString * pin_name );
	int GetPinIndexByNameForPart( cpart * part, CString pin, int x, int y );
	int SetNetWidth( cnet * net, int w, int via_w, int via_hole_w );
	void SetNetVisibility( cnet * net, BOOL visible );
	BOOL GetNetVisibility( cnet * net );
	int CheckNetlist( CString * logstr );
	int CheckConnectivity( CString * logstr );
	void HighlightNetConnections( cnet * net, int bTRANSPARENT=0, BOOL W0=0, int ic=-1, int is=-1 );
	void HighlightNetVertices( cnet * net, BOOL vias_only=FALSE, BOOL IncludeEndVias=TRUE );
	void HighlightNet( cnet * net, int bTRANSPARENT=0 );
	cnet * GetFirstNet();
	cnet * GetNextNet();
	void CancelNextNet();
	void GetWidths( cnet * net, int * w, int * via_w, int * via_hole_w );
	BOOL GetNetBoundaries( RECT * r, BOOL bForSelected );
	int GetSelCount(cnet * net);
	cnet * FindPin( CString * ref_des, CString * pin_name );

	// functions for connections  
	int AddNetConnect( cnet * net, int p1, int p2, int x1=0, int y1=0, int x2=0, int y2=0 );
	int MirrorNetConnect( cnet * net, int ic, int num_copper_layers, BOOL bDraw=TRUE );
	BOOL ReverseNetConnect( cnet * net, int ic, BOOL bRev );
	void RepairBranch( cnet * net, int ic, BOOL bMove );
	void RepairAllBranches( BOOL bMove );
	int AddNetStub( cnet * net, int p1, int x1=0, int y1=0 );
	int RemoveNetConnect( cnet * net, int ic, BOOL set_areas=TRUE );
	int UnrouteNetConnect( cnet * net, int ic );
	void RouteTrace( cnet * net, int ic, int layer, int width, int via_w=0, int via_h=0 );
	int SetConnectionWidth( cnet * net, int ic, int w, int via_w, int via_hole_w );
	void OptimizeConnections( BOOL bBelowPinCount=FALSE, int pin_count=0, BOOL bVisibleNetsOnly=TRUE );
	int OptimizeConnections( cnet * net, int ic, BOOL bBelowPinCount, int pin_count, BOOL bVisibleNetsOnly=TRUE );
	void OptimizeConnections( cpart * part, BOOL bBelowPinCount, int pin_count, BOOL bVisibleNetsOnly=TRUE );
	void RenumberConnection( cnet * net, int ic );
	void RenumberConnections( cnet * net );                      
	BOOL TestHitOnConnectionEndPad( int x, int y, cnet * net, int ic, int layer, int dir );
	int TestHitOnAnyPadInNet( int x, int y, int layer, cnet * net, int * part_pin_index );
	int ChangeConnectionPin( cnet * net, int ic, int end_flag, cnet * nnet, cpart * part, CString * pin_name, int x2=0, int y2=0 );
	void HighlightConnection( cnet * net, int ic, int bTRANSPARENT=0 );
	void HighlightConnection( cnet * net, int ic, int is, BOOL W0, BOOL hv1, BOOL hv2, int transparent );
	void UndrawConnection( cnet * net, int ic );
	void DrawConnection( cnet * net, int ic );
	void DrawConnections( cnet * n );
	void CleanUpConnections( cnet * net, CString * logstr=NULL );
	void CleanUpAllConnections( CString * logstr=NULL );

	// functions for segments
	int DrawSegment( cnet * net, int ic, int is );
	int AppendSegment( cnet * net, int ic, int x, int y, int layer, int width, int vw=0, int vh=0, BOOL bAddVia=0 );
	int InsertSegment( cnet * net, int ic, int iseg, int x, int y, int layer, int width,
						int via_width, int via_hole_width, int dir, BOOL bDrawConnection=TRUE );
	void MergeUnroutedSegments( cnet * net, int ic, BOOL bDraw=TRUE );
	int RouteSegment( cnet * net, int ic, int iseg, int layer, int width, int via_w, int via_h, int next_via=0, int next_hole=0 );
	int RemoveSegment( cnet * net, int ic, int is );							 
	int ChangeSegmentLayer( cnet * net, int ic, int iseg, int layer, int vw, int vh );							 
	int SetSegmentWidth( cnet * net, int ic, int is, int w, int via_w, int via_hole_w );
	void HighlightSegment( cnet * net, int ic, int iseg, int fClearance=0, int bTRANSPARENT=0, BOOL W0=0 );
	int StartMovingSegment( CDC * pDC, cnet * net, int ic, int ivtx,
								   int x, int y, int crosshair, int use_third_segment );
	int StartDraggingSegment( CDC * pDC, cnet * net, int ic, int iseg,
						int x, int y, int layer1, int layer2, int w, 
						int layer_no_via, int via_w, int via_hole_w, int dir,
						int crosshair = 1 );
	int CancelDraggingSegment( cnet * net, int ic, int iseg );
	int StartDraggingSegmentNewVertex( CDC * pDC, cnet * net, int ic, int iseg,
								   int x, int y, int layer, int w, int crosshair );
	int CancelDraggingSegmentNewVertex( cnet * net, int ic, int iseg );
	void StartDraggingStub( CDC * pDC, cnet * net, int ic, int iseg,
						int x, int y, int layer1, int w, 
						int layer_no_via, int via_w, int via_hole_w, 
						int crosshair, int inflection_mode );
	void CancelDraggingStub( cnet * net, int ic, int iseg );
	int CancelMovingSegment( cnet * net, int ic, int ivtx );

	// functions for vias
	int ReconcileVia( cnet * net, int ic, int ivtx, BOOL bDrawVia=TRUE, int vw=0, int vh=0 );
	int ForceVia( cnet * net, int ic, int ivtx, BOOL set_areas=TRUE, int vw=0, int vh=0 );
	int UnforceVia( cnet * net, int ic, int ivtx, BOOL set_areas=TRUE );
	int DrawVia( cnet * net, int ic, int iv );
	void UndrawVia( cnet * net, int ic, int iv );
	void SetViaVisible( cnet * net, int ic, int iv, BOOL visible );
	void HighlightDrcCircles( int Value);

	// functions for vertices
	void HighlightVertex( cnet * net, int ic, int ivtx, int fLines=0, int fClearance=0 );
	int StartDraggingVertex( CDC * pDC, cnet * net, int ic, int iseg,
						int x, int y, int cosshair = 1 );
	int CancelDraggingVertex( cnet * net, int ic, int ivtx );
	void StartDraggingEndVertex( CDC * pDC, cnet * net, int ic, 
		int ivtx, int crosshair = 1 );
	void CancelDraggingEndVertex( cnet * net, int ic, int ivtx );
	void MoveEndVertex( cnet * net, int ic, int ivtx, int x, int y );
	void MoveVertex( cnet * net, int ic, int ivtx, int x, int y );
	int GetViaConnectionStatus( cnet * net, int ic, int iv, int layer );
	void GetViaPadInfo( cnet * net, int ic, int iv, int layer,
		int * pad_w, int * hole_w, int * connect_status );
	BOOL TestForHitOnVertex( cnet * net, int layer, int x, int y, 
		cnet ** hit_net, int * hit_ic, int * hit_iv );

	// functions related to parts
	int RehookPartsToNet( cnet * net );
	void PartAdded( cpart * part );
	int PartMoved( cpart * part , BOOL UNROUTE_SEGMENTS, BOOL bDraw=TRUE );
	int PartFootprintChanged( cpart * part );
	int PartDeleted( cpart * part, BOOL bSetAreas=TRUE );
	int PartDisconnected( cpart * part, BOOL bSetAreas=TRUE );
	int PartCheckConnect( cpart * part );
	void SwapPins( cpart * part1, CString * pin_name1,
						cpart * part2, CString * pin_name2 );
	void SwapConnects (cnet * n1, cnet * n2, id id1, id id2);
	int PartRefChanged( CString * old_ref_des, CString * new_ref_des );

	// functions for copper areas
	int AddArea( cnet * net, int layer, int x, int y, int hatch, int N_CORNERS = CPolyLine::DEF_SIZE );
	void InsertArea( cnet * net, int iarea, int layer, int w, int x, int y, int hatch, int SIZE = CPolyLine::DEF_SIZE );
	int AppendAreaCorner( cnet * net, int iarea, int x, int y, int style, BOOL bDraw=TRUE );
	int InsertAreaCorner( cnet * net, int iarea, int icorner, 
		int x, int y, int style, BOOL bDraw=0 );
	void MoveAreaCorner( cnet * net, int iarea, int icorner, int x, int y, BOOL bDraw=1 );
	void HighlightAreaCorner( cnet * net, int iarea, int icorner, int w=-1 );
	void HighlightAreaSides( cnet * net, int ia, int w, int bTRANSPARENT=0 );
	CPoint GetAreaCorner( cnet * net, int iarea, int icorner );
	int CompleteArea( cnet * net, int iarea, int style );
	void SetAreaConnections();
	void SetAreaConnections( cnet * net, int iarea, BOOL HIGHLIGHT=FALSE ); 
	void SetAreaConnections( cnet * net );
	void SetAreaConnections( cpart * part );
	BOOL TestPointInArea( cnet * net, int x, int y, int layer, int * iarea );
	int RemoveArea( cnet * net, int iarea );
	void SelectAreaSide( cnet * net, int iarea, int iside );
	void SelectAreaCorner( cnet * net, int iarea, int icorner );
	void SetAreaSideStyle( cnet * net, int iarea, int iside, int style );
	int StartDraggingAreaCorner( CDC *pDC, cnet * net, int iarea, int icorner, int x, int y, int crosshair = 1 );
	int CancelDraggingAreaCorner( cnet * net, int iarea, int icorner );
	int StartDraggingInsertedAreaCorner( CDC *pDC, cnet * net, int iarea, int icorner, int x, int y, int crosshair = 1 );
	int CancelDraggingInsertedAreaCorner( cnet * net, int iarea, int icorner );
	void RenumberAreas( cnet * net );
	int TestAreaPolygon( cnet * net, int iarea, int test_contour, int test_corner );
	int TestAreaPolygon( cnet * net, int iarea );
	int ClipAreaPolygon( cnet * net, int iarea, int corner,  
		BOOL bMessageBoxArc, BOOL bMessageBoxInt, BOOL bRetainArcs=TRUE, int*nContours=NULL );
	int AreaPolygonModified( cnet * net, int iarea, BOOL bMessageBoxArc, BOOL bMessageBoxInt, int corner=0 );
	int CombineAllAreasInNet( cnet * net, BOOL bMessageBox, BOOL bUseUtility );
	BOOL TestAreaIntersections( cnet * net, int ia, int corner );
	BOOL TestAreaIntersections( cnet * net, int ia );
	int TestAreaIntersection( cnet * net, int ia1, int ia2 );
	int CombineAreas( cnet * net, int ia1, int ia2 );
	void DrawAreas( cnet * n );
	int AddCutoutsForArea( cnet * net, int ia, int gerber_cl, int hole_cl, int bSMT_copper_connect, Merge * m_ml );

	// I/O  functions
	int WriteNets( CStdioFile * file );
	int ReadNets( CStdioFile * pcb_file, double read_version, int * InLayer=NULL );
	void ExportNetListInfo( netlist_info * nl );
	void ImportNetListInfo( netlist_info * nl, int flags, CDlgLog * log,
		int def_w, int def_w_v, int def_w_v_h );
	void Copy( CNetList * nl );
	void RestoreConnectionsAndAreas( CNetList * old_nl, int flags, CDlgLog * log=NULL );
	void ReassignCopperLayers( int n_new_layers, int * layer );
	void ImportNetRouting( CString * name, CArray<cnode> * nodes, 
		CArray<cpath> * paths, int tolerance, CDlgLog * log=NULL, BOOL bVerbose=TRUE );

	// undo functions
	undo_con * CreateConnectUndoRecord( cnet * net, int icon, BOOL set_areas=TRUE );
	undo_area * CreateAreaUndoRecord( cnet * net, int iarea, int type );
	undo_net * CreateNetUndoRecord( cnet * net );
	static void ConnectUndoCallback( int type, void * ptr, BOOL undo );
	static void AreaUndoCallback( int type, void * ptr, BOOL undo );
	static void NetUndoCallback( int type, void * ptr, BOOL undo );

	// functions for tee_IDs
	void ClearTeeIDs();
	int GetNewTeeID();
	int FindTeeID( int id );
	void RemoveTeeID( int id );
	void AddTeeID( int id );

	// functions for tees and branches
	BOOL FindTeeVertexInNet( cnet * net, int id, int * ic, int * iv );
	BOOL FindTeeVertex( int id, cnet ** net, int * ic=NULL, int * iv=NULL );
	int RemoveTee( cnet * net, int id, int numconnect=-1 );
	BOOL DisconnectBranch( cnet * net, int ic );
	BOOL TeeViaNeeded( cnet * net, int id );
	int RemoveOrphanBranches( cnet * net, int id, BOOL CombineOrphan=FALSE, int via=0, int hole=0 );

private:
	CDisplayList * m_dlist;
	CPartList * m_plist;
	int m_layers;	// number of copper layers
	int m_def_w, m_def_via_w, m_def_via_hole_w;	
	POSITION m_pos[MAX_ITERATORS];	// iterators for nets
	CArray<int> m_tee;

public:
	int m_annular_ring;
	int m_pos_i;	// index for iterators
	BOOL m_bSMT_connect;
};

// definitions of Iterators for nets, connections, segments and vertices

class CIterator_cnet : protected CDLinkList
{
	// List of all active iterators
	static CDLinkList m_LIST_Iterator;

	CNetList const * m_NetList;

	POSITION m_CurrentPos;
	cnet * m_pCurrentNet;

public:
	explicit CIterator_cnet( CNetList const * netlist );
	~CIterator_cnet() {}

	cnet *GetFirst();
	cnet *GetNext();

public:
	static void OnRemove( cnet const * net );
};

class CIterator_cconnect : protected CDLinkList
{
	// List of all active iterators
	static CDLinkList m_LIST_Iterator;

	cnet * m_net;

	int m_CurrentPos;
	cconnect * m_pCurrentConnection;

public:
	explicit CIterator_cconnect( cnet * net );
	~CIterator_cconnect() {}

	cconnect *GetFirst();
	cconnect *GetNext();

public:
	void OnRemove( int ic );
	void OnRemove( cconnect * con );
};

class CIterator_cseg : protected CDLinkList
{
	// List of all active iterators
	static CDLinkList m_LIST_Iterator;

	cconnect * m_cconnect;

	int m_CurrentPos;
	cseg * m_pCurrentSegment;

public:
	explicit CIterator_cseg( cconnect * con );
	~CIterator_cseg() {}

	cseg *GetFirst();
	cseg *GetNext();

public:
	void OnRemove( int is );
	void OnRemove( cseg * seg );
};

class CIterator_cvertex : protected CDLinkList
{
	// List of all active iterators
	static CDLinkList m_LIST_Iterator;

	cconnect * m_cconnect;

	int m_CurrentPos;
	cvertex * m_pCurrentVertex;

public:
	explicit CIterator_cvertex( cconnect * con );
	~CIterator_cvertex() {}

	cvertex *GetFirst();
	cvertex *GetNext();

public:
	void OnRemove( int iv );
	void OnRemove( cvertex * vtx );
};

