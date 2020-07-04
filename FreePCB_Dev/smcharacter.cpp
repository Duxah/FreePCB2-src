
//------------------------------------------------------------------
//+CRi+H  Start of CRi header block
//------------------------------------------------------------------
//               Copyright 1996 Coherent Research Inc.
//      Author:Randy More
//     Created:11/6/96
//   $Revision: 1.1 $
//Last Changed:  $Author: allan $ $Date: 2003/02/11 08:39:09 $
//                Added this header block and comment blocks
//------------------------------------------------------------------
//-CRi-H  End of CRi header block
//------------------------------------------------------------------
#include "stdafx.h"
#include "SMCharacter.h"








//------------------------------------------------------------------
//+CRi+M Start of CRi method block
//------------------------------------------------------------------
//      Class: SMCharacter
//
//     Method: SMCharacter
//
//    Returns: 
//
//  Arguments:
//
//Description:
//         Constructor method for SMCharacter class
//
//------------------------------------------------------------------
//-CRi-M  End of CRi method block
//------------------------------------------------------------------
SMCharacter::SMCharacter()
{
	cVertexCount = 0;
	cVertex = NULL;
}

SMCharacter::SMCharacter(SMCharacter * pChar)
{
	cVertexCount = pChar->cVertexCount;
	cVertex = new CharVertex[cVertexCount];//??
	int loop;
	for(loop = 0; loop < cVertexCount; loop++)
	{
		cVertex[loop] = pChar->cVertex[loop];
	}
}






//------------------------------------------------------------------
//+CRi+M Start of CRi method block
//------------------------------------------------------------------
//      Class: SMCharacter
//
//     Method: ~SMCharacter
//
//    Returns: 
//
//  Arguments:
//
//Description:
//         Destructor method for SMCharacter class
//
//------------------------------------------------------------------
//-CRi-M  End of CRi method block
//------------------------------------------------------------------
SMCharacter::~SMCharacter()
{
	if(cVertex!=NULL)
	{
		delete cVertex;
	}
}








//------------------------------------------------------------------
//+CRi+M Start of CRi method block
//------------------------------------------------------------------
//      Class: SMCharacter
//
//     Method: Read
//
//    Returns: void
//
//  Arguments:
//         FILE * infile
//
//Description:
//         Perform the Read operation    *AG*
//
//------------------------------------------------------------------
//-CRi-M  End of CRi method block
//------------------------------------------------------------------
void SMCharacter::Read(FILE * infile)
{
	fread(&cVertexCount,sizeof(int),1,infile);
	cVertex = new CharVertex[cVertexCount];//??
	fread(cVertex,sizeof(CharVertex),cVertexCount,infile);		
}







//------------------------------------------------------------------
//+CRi+M Start of CRi method block
//------------------------------------------------------------------
//      Class: SMCharacter
//
//     Method: Write
//
//    Returns: void
//
//  Arguments:
//         FILE * outfile
//
//Description:
//         Perform the Write operation    *AG*
//
//------------------------------------------------------------------
//-CRi-M  End of CRi method block
//------------------------------------------------------------------
void SMCharacter::Write(FILE * outfile)
{
	fwrite(&cVertexCount,sizeof(int),1,outfile);
	fwrite(cVertex,sizeof(CharVertex),cVertexCount,outfile);		
}







//------------------------------------------------------------------
//+CRi+M Start of CRi method block
//------------------------------------------------------------------
//      Class: SMCharacter
//
//     Method: ReadOldStyle
//
//    Returns: int
//
//  Arguments:
//         FILE * infile
//
//Description:
//         Perform the ReadOldStyle operation    *AG*
//
//------------------------------------------------------------------
//-CRi-M  End of CRi method block
//------------------------------------------------------------------
int SMCharacter::ReadOldStyle(FILE * infile)
{
	unsigned char inbuffer[4096];
	char * scanner;
	int i;
		
	if(feof(infile))
	{
		return(FALSE);
	}

//read the data from the file
	fgets((char *)inbuffer,2048,infile);


//Get the data size
	scanner = (char *)inbuffer;
	sscanf(scanner,"%d",&cVertexCount);
	cVertexCount = (cVertexCount / 2);
	if(cVertexCount == 0)
	{
		return(FALSE);
	}


//Build the data structure
	cVertex = new CharVertex[cVertexCount];//??

	for(i=0; i<cVertexCount; i++)
	{
		while(*scanner!=',') scanner++;
		scanner++;
		sscanf(scanner,"%lf,%lf",&(cVertex[i].X), &(cVertex[i].Y));
		while(*scanner!=',') scanner++;
		scanner++;
	}

	return(TRUE);
}







//------------------------------------------------------------------
//+CRi+M Start of CRi method block
//------------------------------------------------------------------
//      Class: SMCharacter
//
//     Method: GetFirstVertex
//
//    Returns: CHARVERTEX_TYPE
//
//  Arguments:
//         CharVertex &pVertex
//         int &pItter
//
//Description:
//         Perform the GetFirstVertex operation    *AG*
//
//------------------------------------------------------------------
//-CRi-M  End of CRi method block
//------------------------------------------------------------------
SMCharacter::CHARVERTEX_TYPE SMCharacter::GetFirstVertex(
	CharVertex &pVertex, int &pItter)
{
	if(cVertex == NULL)
		return(TERMINATE);
	pItter = 1;
	pVertex = cVertex[pItter];
	return(MOVE_TO);
}








//------------------------------------------------------------------
//+CRi+M Start of CRi method block
//------------------------------------------------------------------
//      Class: SMCharacter
//
//     Method: GetNextVertex
//
//    Returns: CHARVERTEX_TYPE
//
//  Arguments:
//         CharVertex &pVertex
//         int &pItter
//
//Description:
//         Perform the GetNextVertex operation    *AG*
//
//------------------------------------------------------------------
//-CRi-M  End of CRi method block
//------------------------------------------------------------------
SMCharacter::CHARVERTEX_TYPE SMCharacter::GetNextVertex(
	CharVertex &pVertex, int &pItter)
{
	pItter++;
	if(pItter < cVertexCount-1)
	{
		pVertex = cVertex[pItter];
		if(pVertex.X < -63.5)
		{
			pItter++;
			pVertex = cVertex[pItter];
			return(MOVE_TO);
		}
		return(DRAW_TO);
	}
	return(TERMINATE);
}


