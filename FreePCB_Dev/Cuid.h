// This is a class to generate fast, simple unique IDs within an application
//
#pragma once

class Cuid {
private:
	enum{ MAX_VALUE = 0x1fffff };		// can't exceed 0x1fffff
	enum{ BITS_SIZE = (MAX_VALUE+1)/32 };	// size of bit array
	enum{ INDEX_MASK = MAX_VALUE >> 5 };	// mask for index into array
	UINT32 bits[BITS_SIZE];		// array of bits, one bit for each possible uid 
	int n_uids;
	UINT32 mask_table[32];

public:
	Cuid();
	~Cuid(){};

	int GetNewUID();
	void ReleaseUID( UINT32 uid ); 
};