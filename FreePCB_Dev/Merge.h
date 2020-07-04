/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *		Author:    Duxah (ver. 2.0 - 2.033)			       *
 *		email: duxah@yahoo.com							   *
 *		URL: www.freepcb.dev							   *
 *		Copyright: (C) Duxah 2014 - 2020.				   *
 *		This software is free for non-commercial use.	   *
 *		It may be copied, modified, and redistributed	   *
 *		provided that this copyright notice is 			   *
 *		preserved on all copies. You may not use this	   *
 *		software, in whole or in part, in support of	   *
 *		any commercial product without the express 		   *
 *		consent of the authors.							   *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */



#pragma once
#include "stdafx.h"
class Merge
{
#define dERROR -1
public:
	Merge(void)
	{
		m_size = 0;
		if( m_merges.GetSize() )
			Clear();
	}

	virtual ~Merge(void){}

	int AddNew ( CString mName, int area_clearance )
	{
		if( !mName.GetLength() )
			return -1;
		for( int i=m_size-1; i>=0; i-- )
		{
			if( m_merges[i].Compare(mName) == 0 )
			{
				m_clearances[i] = area_clearance;
				return i;
			}
		}
		m_merges.Add( mName );
		m_clearances.Add( area_clearance );
		utility.Add(0);
		m_size++;
		return m_size-1;
	}

	void mark0 ()
	{
		for( int i=m_size-1; i>=0; i-- )
			utility[i] = 0;
	}

	void mark1 (int i, int value=1)
	{
		if( i >= 0 && i < m_size )
			utility[i] = value;
	}

	int GetMark ( int i )
	{
		if( i >= 0 && i < m_size )
			return utility[i];
		return 0;
	}

	int GetIndex ( CString mName )
	{
		for( int i=m_size-1; i>=0; i-- )
		{
			if( m_merges[i].Compare(mName) == 0 )
				return i;
		}
		return -1;
	}

	CString GetMerge (int index)
	{
		if( index >= 0 && index < m_size )
			return m_merges[index];
		else
			return "";
	}

	int GetClearance (int index)
	{
		if( index >= 0 && index < m_size )
			return m_clearances[index];
		else
			return 0;
	}

	void SetMerge (int index, CString Name)
	{
		if( index >= 0 && index < m_size )
			m_merges[index] = Name;
	}

	void SetClearance (int index, int cl)
	{
		if( index >= 0 && index < m_size )
			m_clearances[index] = cl;
	}

	int GetSize ()
	{
		return m_size;
	}

	void Clear()
	{
		m_merges.RemoveAll();
		m_clearances.RemoveAll();
		utility.RemoveAll();
		m_size = 0;
	}

	BOOL MergeSelected()
	{
		for( int i=m_size-1; i>=0; i-- )
			if( GetMark(i) )
				return true;
		return false;
	}

	void CopyFrom( Merge * from )
	{
		int new_size = from->GetSize();
		m_merges.SetSize(new_size);
		m_clearances.SetSize(new_size);
		utility.SetSize(new_size);
		m_size = new_size;
		mark0();
		for( int i=new_size-1; i>=0; i-- )
		{
			m_merges[i] = from->GetMerge(i);
			m_clearances[i] = from->GetClearance(i);   
			utility[i] = from->GetMark(i);
		}
	}

private:
	CArray<CString> m_merges;
	CArray<int> m_clearances;
	CArray<int> utility;
	int m_size;
};

