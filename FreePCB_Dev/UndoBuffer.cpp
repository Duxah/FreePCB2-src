#include "stdafx.h"
#include "UndoBuffer.h"

CUndoBuffer::CUndoBuffer( UINT32 size )
{
	m_size = size;
	m_buf = (char*)calloc( size, sizeof(char) );
	m_start_offset = 0;
	m_current_offset = 0;;
	m_busy = FALSE;
}

CUndoBuffer::~CUndoBuffer(void)
{
	free( m_buf );
}

// Empty the buffer
//
void CUndoBuffer::Reset()
{
	m_start_offset = 0;
	m_current_offset = 0;
	m_busy = FALSE;
}

// Test for buffer empty
//
BOOL CUndoBuffer::IsEmpty()
{
	return( m_start_offset == m_current_offset );
}

// start new undo event in buffer
// returns 0 if OK
//
int CUndoBuffer::NewEvent()
{
	if( m_busy )
		return 1;
	m_busy = TRUE;
	Append( &CString( "\n[UNDO]\n" ) );
	return 0;
}

// Append string to event
//
void CUndoBuffer::Append( CString * str )	
{
	int len = str->GetLength();
	int final_offset = m_current_offset + len;
	if( final_offset >= m_size )
	{
		// buffer wrap-around pending
		final_offset -= m_size;
		while( m_current_offset < m_start_offset || final_offset > m_start_offset )
		{
			// overrun, drop oldest event
			DropOldestEvent();
		}
	}
	else
	{
		// no buffer wrap-around
		while( m_current_offset < m_start_offset && final_offset > m_start_offset )
		{
			// overrun
			DropOldestEvent();
		}
	}
	for( int ic=0; ic<len; ic++ )
	{
		*(m_buf+m_current_offset++) = str->GetAt( ic );
		if( m_current_offset >= m_size )
			m_current_offset = 0;
	}
}

// Finish the event
//
void CUndoBuffer::EndEvent()	
{
	m_busy = FALSE;
}

// Get last event, put into CString, and remove event from buffer
// returns 0 if there are no events or busy, 
// otherwise returns pointer to event in buffer
//
char * CUndoBuffer::GetLastEvent( CString * str )	
{
	char *ch, *ch2, *ch3, *ch4, *ch5, *ch6, *ch7, *ch8;
	int n = 0;

	if( m_start_offset == m_current_offset )
		return 0;	// there are no events left

	if( m_busy )
		return 0;

	ch = m_buf + m_current_offset;
	while( n < m_size )
	{
		ch--;
		if( ch < m_buf )
			ch = m_buf + m_size - 1;
		if( *ch == '\n' )
		{
			ch2 = ch - 1;
			if( ch2 < m_buf )
				ch2 = m_buf + m_size - 1;
			if( *ch2 == ']' )
			{
				ch3 = ch2 - 1;
				if( ch3 < m_buf )
					ch3 = m_buf + m_size - 1;
				if( *ch3 == 'O' )
				{
					ch4 = ch3 - 1;
					if( ch4 < m_buf )
						ch4 = m_buf + m_size - 1;
					if( *ch4 == 'D' )
					{
						ch5 = ch4 - 1;
						if( ch5 < m_buf )
							ch5 = m_buf + m_size - 1;
						if( *ch5 == 'N' )
						{
							ch6 = ch5 - 1;
							if( ch6 < m_buf )
								ch6 = m_buf + m_size - 1;
							if( *ch6 == 'U' )
							{
								ch7 = ch6 - 1;
								if( ch7 < m_buf )
									ch7 = m_buf + m_size - 1;
								if( *ch7 == '[' )
								{
									ch8 = ch7 - 1;
									if( ch8 < m_buf )
										ch8 = m_buf + m_size - 1;
									if( *ch8 == '\n' )
									{
										// success, now drop event
										ch++;
										if( ch >= (m_buf + m_size) )
											ch = m_buf;
										char * ch_ret = ch;
										int ic = 0;
										*str = "";
										do
										{
											str->AppendChar( *ch );
											ch++;
											if( ch >= (m_buf + m_size) )
												ch = m_buf;
										} while( ch != (m_buf+m_current_offset) );
										m_current_offset = ch8 - m_buf;
										return ch_ret;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return 0;
}

// Drop oldest event from buffer, reset pointers
// return offset of new oldest event or UINT_MAX if error
//
UINT32 CUndoBuffer::DropOldestEvent()
{
	char * ch = m_buf + m_start_offset;
	char * end_buf = m_buf + m_size - 1;
	char *ch2, *ch3, *ch4, *ch5, *ch6, *ch7, *ch8;
	int n = 0;
	while( n < m_size )
	{
		ch++;
		if( ch > end_buf )
			ch = m_buf;
		if( *ch == '\n' )
		{
			ch2 = ch+1;
			if( ch2 > end_buf )
				ch2 = m_buf;
			if( *ch2 == '[' )
			{
				ch3 = ch2+1;
				if( ch3 > end_buf )
					ch3 = m_buf;
				if( *ch3 == 'U' )
				{
					ch4 = ch3+1;
					if( ch4 > end_buf )
						ch4 = m_buf;
					if( *ch4 == 'N' )
					{
						ch5 = ch4+1;
						if( ch5 > end_buf )
							ch5 = m_buf;
						if( *ch5 == 'D' )
						{
							ch6 = ch5+1;
							if( ch6 > end_buf )
								ch6 = m_buf;
							if( *ch6 == 'O' )
							{
								ch7 = ch6+1;
								if( ch7 > end_buf )
									ch7 = m_buf;
								if( *ch7 == ']' )
								{
								}
								ch8 = ch7+1;
								if( ch8 > end_buf )
									ch8 = m_buf;
								if( *ch8 == '\n' )
								{
									// success
									ch8++;
									if( ch8 > end_buf )
										ch8 = m_buf;
									m_start_offset = ch - m_buf;
									return m_start_offset;
								}
							}
						}
					}
				}
			}
		}
		n++;
	}
	return UINT_MAX;	// error
}

