#include "stdafx.h"
#include "LinkList.h"
//#include "partlist.h"

//******** Iterator for cnet (CIterator_cnet) *********
CDLinkList CIterator_cnet::m_LIST_Iterator;

// Constructor
CIterator_cnet::CIterator_cnet( CNetList const * netlist )
	: m_NetList(netlist)
	, m_pCurrentNet(NULL)
{
	m_LIST_Iterator.insert_after(this);
}

// Iterator operators: First/Next
cnet *CIterator_cnet::GetFirst()
{
	m_pCurrentNet = NULL;

	// test for no nets
	if( m_NetList->m_map.GetSize() != 0 )
	{
		// increment iterator and get first net
		m_CurrentPos = m_NetList->m_map.GetStartPosition();

		return GetNext();
	}

	return m_pCurrentNet;
}

cnet *CIterator_cnet::GetNext()
{
	if( m_CurrentPos != NULL )
	{
		CString name;
		void * ptr;

		m_NetList->m_map.GetNextAssoc( m_CurrentPos, name, ptr );

		m_pCurrentNet = (cnet*)ptr;
		ASSERT( m_pCurrentNet != NULL );
	}
	else
	{
		m_pCurrentNet = NULL;
	}

	return m_pCurrentNet;
}


// OnRemove() must be called when removing/deleting net
// to update any active iterators.
void CIterator_cnet::OnRemove( cnet const *net )
{
	// NOP
}

//******** Iterator for cconnect (CIterator_cconnect) *********
CDLinkList CIterator_cconnect::m_LIST_Iterator;

// Constructor
CIterator_cconnect::CIterator_cconnect( cnet * net )
//	: m_NetList(netlist)
//	, m_pCurrentNet(NULL)
{
	m_net = net;
	m_LIST_Iterator.insert_after(this);
}

// Iterator operators: First/Next
cconnect *CIterator_cconnect::GetFirst()
{
	m_CurrentPos = -1;	// -1 = position before first connection
	m_pCurrentConnection = NULL;

	// test for no connections
	if( m_net->nconnects != 0 )
	{
		// increment iterator and get first net
		GetNext();
	}

	return m_pCurrentConnection;
}

cconnect *CIterator_cconnect::GetNext()
{
	m_CurrentPos++;
	if( m_CurrentPos < m_net->nconnects )
	{
		m_pCurrentConnection = &m_net->connect[m_CurrentPos];
	}
	else
	{
		m_pCurrentConnection = NULL;
	}

	return m_pCurrentConnection;
}

// OnRemove() must be called before removing/deleting cconnect
// to update any active iterators.
void CIterator_cconnect::OnRemove( int ic )
{
	// For every iterator, adjust the "current connection" if that 
	// connection or earlier is being removed
	for( CDLinkList *pElement = m_LIST_Iterator.next; pElement != &m_LIST_Iterator; pElement = pElement->next )
	{
		CIterator_cconnect *pIterator = static_cast<CIterator_cconnect *>(pElement);

		if( ic <= pIterator->m_CurrentPos )
		{
			// Make adjustment so that the next GetNext() moves to 
			// the connection after the one removed.
			pIterator->m_pCurrentConnection--;
		}
	}
}

void CIterator_cconnect::OnRemove( cconnect * con )
{
	// get index of connection being removed
	int remove_ic = -1;
	for( int ic=0; ic<m_net->nconnects; ic++ )
	{
		if( con == &m_net->connect[ic] )
		{
			remove_ic = ic;
			break;
		}
	}
	if( remove_ic == -1 )
		ASSERT(0);

	OnRemove( remove_ic );

}

//******** Iterator for cseg (CIterator_cseg) *********
CDLinkList CIterator_cseg::m_LIST_Iterator;

// Constructor
CIterator_cseg::CIterator_cseg( cconnect * con )
//	: m_NetList(netlist)
//	, m_pCurrentNet(NULL)
{
	m_cconnect = con;
	m_LIST_Iterator.insert_after(this);
}

// Iterator operators: First/Next
cseg *CIterator_cseg::GetFirst()
{
	m_CurrentPos = -1;	// -1 = position before first seg
	m_pCurrentSegment = NULL;

	// test for no vertices
	if( m_cconnect->nsegs != 0 )
	{
		// increment iterator and get first net
		GetNext();
	}

	return m_pCurrentSegment;
}

cseg *CIterator_cseg::GetNext()
{
	m_CurrentPos++;
	if( m_CurrentPos <= m_cconnect->nsegs )
	{
		m_pCurrentSegment = &m_cconnect->seg[m_CurrentPos];
	}
	else
	{
		m_pCurrentSegment = NULL;
	}

	return m_pCurrentSegment;
}

// OnRemove(is) must be called when removing a cseg
// to update any active iterators.
void CIterator_cseg::OnRemove( int is )
{
	// For every iterator, adjust the "current segion" if that 
	// segion or earlier is being removed
	for( CDLinkList *pElement = m_LIST_Iterator.next; pElement != &m_LIST_Iterator; pElement = pElement->next )
	{
		CIterator_cseg *pIterator = static_cast<CIterator_cseg *>(pElement);

		if( is <= pIterator->m_CurrentPos )
		{
			// Make adjustment so that the next GetNext() moves to 
			// the segion after the one removed.
			pIterator->m_pCurrentSegment--;
		}
	}
}

// OnRemove(seg) must be called BEFORE removing a cseg
// to update any active iterators.
void CIterator_cseg::OnRemove( cseg * seg )
{
	// get index of segion being removed
	int remove_is = -1;
	for( int is=0; is<m_cconnect->nsegs; is++ )
	{
		if( seg == &m_cconnect->seg[is] )
		{
			remove_is = is;
			break;
		}
	}
	if( remove_is == -1 )
		ASSERT(0);

	OnRemove( remove_is );

}

//******** Iterator for cvertex (CIterator_cvertex) *********
CDLinkList CIterator_cvertex::m_LIST_Iterator;

// Constructor
CIterator_cvertex::CIterator_cvertex( cconnect * con )
//	: m_NetList(netlist)
//	, m_pCurrentNet(NULL)
{
	m_cconnect = con;
	m_LIST_Iterator.insert_after(this);
}

// Iterator operators: First/Next
cvertex *CIterator_cvertex::GetFirst()
{
	m_CurrentPos = -1;	// -1 = position before first vertex
	m_pCurrentVertex = NULL;

	// test for no vertices
	if( m_cconnect->nsegs != 0 )
	{
		// increment iterator and get first net
		GetNext();
	}

	return m_pCurrentVertex;
}

cvertex *CIterator_cvertex::GetNext()
{
	m_CurrentPos++;
	if( m_CurrentPos <= m_cconnect->nsegs )
	{
		m_pCurrentVertex = &m_cconnect->vtx[m_CurrentPos];
	}
	else
	{
		m_pCurrentVertex = NULL;
	}

	return m_pCurrentVertex;
}

// OnRemove(iv) must be called when removing/deleting cvertex
// to update any active iterators.
void CIterator_cvertex::OnRemove( int iv )
{
	// For every iterator, adjust the "current vertexion" if that 
	// vertex or earlier is being removed
	for( CDLinkList *pElement = m_LIST_Iterator.next; pElement != &m_LIST_Iterator; pElement = pElement->next )
	{
		CIterator_cvertex *pIterator = static_cast<CIterator_cvertex *>(pElement);

		if( iv <= pIterator->m_CurrentPos )
		{
			// Make adjustment so that the next GetNext() moves to 
			// the vertex after the one removed.
			pIterator->m_pCurrentVertex--;
		}
	}
}

// OnRemove(vtx) must be called BEFORE removing/deleting cvertex
// to update any active iterators.
void CIterator_cvertex::OnRemove( cvertex * vtx )
{
	// get index of vertex being removed
	int remove_iv = -1;
	for( int iv=0; iv<=m_cconnect->nsegs; iv++ )
	{
		if( vtx == &m_cconnect->vtx[iv] )
		{
			remove_iv = iv;
			break;
		}
	}
	if( remove_iv == -1 )
		ASSERT(0);

	OnRemove( remove_iv );

}

