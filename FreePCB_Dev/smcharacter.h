#ifndef _SMCHARACTER_
#define _SMCHARACTER_

struct CharVertex
{
	double X;
	double Y;
};

class SMCharacter
{

friend class CCharEditDlg;

public:
	enum CHARVERTEX_TYPE
	{
		MOVE_TO,	//Pen up
		DRAW_TO,	//Pen down
		TERMINATE	//End of data
	};


private:
	int cVertexCount;
	CharVertex * cVertex;

public:
	SMCharacter();
	SMCharacter(SMCharacter * pChar);
	~SMCharacter();
	void Read(FILE * infile);
	void Write(FILE * outfile);
	int ReadOldStyle(FILE * infile);
	double GetMinX(void)
	{
		return(cVertex[0].X);
	}
	double GetMaxX(void)
	{
		return(cVertex[0].Y);
	}
	CHARVERTEX_TYPE GetFirstVertex(CharVertex &pVertex,
		int &pItter);
	CHARVERTEX_TYPE GetNextVertex(CharVertex &pVertex,
		int &pItter);
};
#endif