#include "stdafx.h"
#include "undolist.h"

// the CUndoList class is a doubly-linked list of CUndoItems
//
// each CUndoItem contains:
//	prev = pointer to previous item
//	next = pointer to next item
//	type = indicates type of undo record for callback function
//	ptr = pointer to undo record for callback function
//	callback = pointer to callback function to handle the undo operation
//
// Normally, there will be user-generated undo "events" (such as removing a part) 
// which will generate multiple undo "items" (such as deleting the part and deleting
// each connections to the part)
// An event is indicated by adding a CUndoItem to the list with ptr = 0.
// All of the items generated by the event are then added to the list.
//
// To undo an event, each item following the event is popped off the list,
// and the callback function for that item is called. The callback function
// actually does the work of undoing the item, and should then destroy the 
// item's undo record.
//
// If the list gets too full, the first event will be dropped.
// In this case the callback functions for the items of the first event
// will be called with undo = FALSE, indicating that they should
// just destroy the undo record without actually performing the undo.

CUndoList::CUndoList( int max_items, int max_events )
{
	m_start.prev = 0;		// dummy first element in list
	m_start.next = &m_end;
	m_end.next = 0;			// dummy last element in list
	m_end.prev = &m_start;
	m_max_items = max_items;	// size limit
	m_max_events = max_events;  // size limit
	m_num_items = 0;			
	m_num_events = 0;
	m_size = 0;
}

CUndoList::~CUndoList(void)
{
	Clear();
}

// clear all items
//
void CUndoList::Clear()
{
	while( m_num_items > 0 )
		DropFirstEvent();
}

// Make list smaller by dropping all items for first event
//
void CUndoList::DropFirstEvent()
{
	if( m_num_items == 0 )
		return;
	// remove items until we reach an event (i.e. an item with ptr == 0)
	BOOL bValidItem;
	do
	{
		// remove first event in list
		CUndoItem * first = m_start.next;	// get first item
		m_start.next = first->next;			// set m_start to point to next item
		first->next->prev = &m_start;		// set next item to point to m_start
		if( first->ptr )
			first->callback( first->type, first->ptr, FALSE ); // allow callback to destroy record
		delete first;
		m_num_items--;
		bValidItem = ( m_start.next->ptr || m_start.next->type || m_start.next->callback );
	} while( bValidItem && m_num_items > 0 );
	m_num_events--;
}

// Add an item (which may be an event if ptr == 0) to the end of the list
//
void CUndoList::Push( int type, void * ptr, UndoCallbackPtr callback, int size )
{
	// create new instance and link into list
	CUndoItem * item = new CUndoItem;//ok
	item->prev = m_end.prev;
	item->next = &m_end;
	item->prev->next = item;
	item->next->prev = item;
	item->type = type;
	item->ptr = ptr;
	item->callback = callback;
	item->size = size;
	m_num_items++;
	m_size += size;
}

// Remove last item from list and call the callback with type and ptr
// Then destroy the item
// The callback should destroy the record, if necessary.
// Returns FALSE if list is empty or item is a new event
//
BOOL CUndoList::Pop()
{
	CUndoItem * item = m_end.prev;
	if( item == &m_start )
	{
		// list is empty
		if( m_num_items != 0 )
			ASSERT(0);
		return 0;
	}
	// pop item
	int type = item->type;
	void * ptr = item->ptr;
	UndoCallbackPtr callback = item->callback;
	item->prev->next = &m_end;
	m_end.prev = item->prev;
	m_size -= item->size;
	delete item;
	m_num_items--;
	// check for normal callback
	BOOL bValidItem = ptr || type || callback;
	if( bValidItem )
		// normal callback
		callback( type, ptr, TRUE );
	else
		// end of this event, no callback
		m_num_events--;
	return bValidItem;
}

void CUndoList::NewEvent()
{
	if( m_num_events >= m_max_events || m_num_items >= m_max_items )
		DropFirstEvent();
	Push( 0, 0, 0 );	// this marks the beginning of items for an undo event
	m_num_events++;
}