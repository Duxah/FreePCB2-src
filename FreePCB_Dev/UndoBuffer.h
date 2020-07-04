#pragma once

class CUndoBuffer
{
public:
	CUndoBuffer( const UINT32 size);
	~CUndoBuffer(void);
	void Reset();
	int NewEvent();
	void Append( CString * str );
	void EndEvent();
	char * GetLastEvent( CString * str );
	BOOL IsEmpty();
private:
	UINT32 DropOldestEvent();
	BOOL m_busy;
	char * m_buf;
	UINT32 m_size;
	UINT32 m_start_offset;		// start of first event in buffer
	UINT32 m_current_offset;	// next available position
};
