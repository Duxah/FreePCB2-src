// This is a class to generate fast, simple unique IDs within an application
// The UIDs are integers with values from 0 to 0x1fffff, so there are about 
// 2 million possible values which should be enough for FreePCB
// A value of -1 can be used by the app to indicate an invalid uid
//
#include "stdafx.h"
#include "Cuid.h"

Cuid::Cuid()
{
	if( MAX_VALUE > 0x1fffff )
		ASSERT(0);
	// initialize mask table
	for( int i=0; i<32; i++ )
		mask_table[i] = 0x1 << i;
	// initialize bit array
	for( int i=0; i<BITS_SIZE; i++ )
		bits[i] = 0;
	n_uids = 0;		// uid counter
	srand(0);		// seed random number generator
}

// Return new uid and set bit in bits[]
//
int Cuid::GetNewUID()
{
	int i;	// index into bits[]

	// find an element of bits[] that has a bit = 0
	UINT32 test_bits = 0xffffffff;
	while( test_bits == 0xffffffff )
	{
		i = rand() & INDEX_MASK;	
		test_bits = bits[i];
	}
	// get position of first zero bit
	for( int npos = 0; npos<32; npos++ )
	{
		if( test_bits | ~0x1 )
		{
			bits[i] |= mask_table[npos];		// set bit flag 1
			n_uids++;
			if( n_uids > MAX_VALUE/2 )
				ASSERT(0);
			return i<<5 + npos;
		}
	}
	return -1;	// failed
}

// Release uid by clearing bit in bits[]
//
void Cuid::ReleaseUID( UINT32 uid )
{
	if( uid < 0 || uid > MAX_VALUE )
		ASSERT(0);
	int i = uid>>5;			// index into bits[]
	int npos = uid & 0x1f;	// bit number to clear
	bits[i] &= ~mask_table[npos];	// clear bit
	n_uids--;				// decrement counter
}
