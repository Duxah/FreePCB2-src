#pragma once

class CUndoList;

// prototype for callback function:
typedef void(*UndoCallbackPtr)(int, void*, BOOL);

class CUndoItem
{
public:
	CUndoItem * prev;
	CUndoItem * next;
	int type;		// used by callback routine
	void * ptr;		// pointer to callback record
	UndoCallbackPtr callback;	// pointer to callback handler
	int size;		// used to track memory usage
};

class CUndoList
{
public:
	CUndoList( int max_items, int max_events );
	~CUndoList(void);
	void Push( int type, void * ptr, UndoCallbackPtr callback, int size=0 );
	BOOL Pop();
	void NewEvent();
	void DropFirstEvent();
	void Clear();
	CUndoItem m_start;
	CUndoItem m_end;
	int m_max_items;
	int m_max_events;
	int m_num_items;
	int m_num_events;
	unsigned long m_size;
};
