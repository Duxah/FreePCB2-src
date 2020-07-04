
//------------------------------------------------------------------
//+CRi+H  Start of CRi header block
//------------------------------------------------------------------
//               Copyright 1996 Coherent Research Inc.
//      Author:Randy More
//     Created:11/6/96
//   $Revision: 1.3 $
//Last Changed:  $Author: Allan $ $Date: 2003/08/03 23:51:52 $
//                Added this header block and comment blocks
//------------------------------------------------------------------
//-CRi-H  End of CRi header block
//------------------------------------------------------------------
#include "stdafx.h"
#include <afxtempl.h>
#include "resource.h"
#include <math.h>
#include "SMCharacter.h"
#include "SMFontUtil.h"

//CString smffile = "c:\\HersheyFonts\\Hershey.smf";
//CString xtbfile = "c:\\HersheyFonts\\Hershey.xtb";
CString smfpath;
CString smffile = "Hershey.smf";
CString xtbfile = "Hershey.xtb";

#define BASE_CHARACTER_WIDTH 16.0
#define BASE_CHARACTER_HEIGHT 22.0
#define TEXT_DROP 2
//#define WADJUST 2.25
//#define HADJUST 2.0
#define WADJUST 1.5
#define HADJUST 1.0



//------------------------------------------------------------------
//+CRi+M Start of CRi method block
//------------------------------------------------------------------
//      Class: SMFontUtil
//
//     Method: SMFontUtil
//
//    Returns: 
//
//  Arguments:
//
//Description:
//         Constructor method for SMFontUtil class
//
//------------------------------------------------------------------
//-CRi-M  End of CRi method block
//------------------------------------------------------------------

SMFontUtil::SMFontUtil(CString * path)
{
	smfpath = *path;
	cCharCount = 0;
	int outer, inner;
	for(outer = 0; outer < 12; outer++)
	{
		for(inner = 0; inner < 256; inner++)
		{
			cXlationTable[outer][inner]=-1;
		}
	}
	int err = LoadFontData();
	if( err )
		ASSERT(0);
	LoadXlationData();
}


//------------------------------------------------------------------
//+CRi+M Start of CRi method block
//------------------------------------------------------------------
//      Class: SMFontUtil
//
//     Method: ~SMFontUtil
//
//    Returns: 
//
//  Arguments:
//
//Description:
//         Destructor method for SMFontUtil class
//
//------------------------------------------------------------------
//-CRi-M  End of CRi method block
//------------------------------------------------------------------
SMFontUtil::~SMFontUtil()
{
	unsigned int itter;
	SMCharacter * chr;

	for(itter=0; itter<cCharCount; itter++)
	{
		chr = SMCharList[itter];
		delete chr;
	}
	SMCharList.RemoveAll();
}








//------------------------------------------------------------------
//+CRi+M Start of CRi method block
//------------------------------------------------------------------
//      Class: SMFontUtil
//
//     Method: LoadFontData
//
//    Returns: 0 if success, 1 if error
//
//  Arguments:
//         void
//
//Description:
//         Perform the LoadFontData operation    *AG*
//
//------------------------------------------------------------------
//-CRi-M  End of CRi method block
//------------------------------------------------------------------
int SMFontUtil::LoadFontData(void) 
{
	FILE * infile;
	SMCharacter * chr;
	unsigned int loop;
	CString full_path = smfpath + "\\" + smffile;
	if(!(infile = fopen(full_path,"rb")))
	{
		AfxMessageBox( "Font stroke file was not found" );
		return 1;
	}
	else
	{
		fread(&loop, sizeof(unsigned int), 1, infile);
		for(cCharCount = 0; cCharCount < loop; cCharCount++)
		{
			chr = new SMCharacter;//??
			chr->Read(infile);
			SMCharList.Add(chr);
		}
		cCharCount = loop;
		fclose(infile);
		return 0;
	}
}









//------------------------------------------------------------------
//+CRi+M Start of CRi method block
//------------------------------------------------------------------
//      Class: SMFontUtil
//
//     Method: LoadXlationData
//
//    Returns: void
//
//  Arguments:
//         void
//
//Description:
//         Perform the LoadXlationData operation    *AG*
//
//------------------------------------------------------------------
//-CRi-M  End of CRi method block
//------------------------------------------------------------------
int SMFontUtil::LoadXlationData(void)
{
	FILE * infile;
	int i,j,k;
	CString full_path = smfpath + "\\" + xtbfile;
	if(!(infile = fopen(full_path,"rb")))
	{
		AfxMessageBox( "Font translation file was not found" );
		return 1;
	}
	else
	{
		for(i=0; i<12; i++)
		{
			for(j=0; j<128; j++)
			{
				if(!feof(infile))
				{
					fread(&k, sizeof(int), 1, infile);
					cXlationTable[i][j] = k;
				}
			}
		}
		for(i=0; i<12; i++)
		{
			for(j=128; j<256; j++)
			{
				if(!feof(infile))
				{
					fread(&k, sizeof(int), 1, infile);
					cXlationTable[i][j] = k;
				}
			}
		}
		fclose(infile);
		return 0;
	}
}









//------------------------------------------------------------------
//+CRi+M Start of CRi method block
//------------------------------------------------------------------
//      Class: SMFontUtil
//
//     Method: SaveXlationData
//
//    Returns: void
//
//  Arguments:
//         void
//
//Description:
//         Perform the SaveXlationData operation    *AG*
//
//------------------------------------------------------------------
//-CRi-M  End of CRi method block
//------------------------------------------------------------------
void SMFontUtil::SaveXlationData(void)
{
	FILE * outfile;
	int i,j,k;
	outfile = fopen(xtbfile,"wb");
	for(i=0; i<12; i++)
	{
		for(j=0; j<128; j++)
		{
			k = cXlationTable[i][j];
			fwrite(&k, sizeof(int), 1, outfile);
		}
	}
	for(i=0; i<12; i++)
	{
		for(j=128; j<256; j++)
		{
			k = cXlationTable[i][j];
			fwrite(&k, sizeof(int), 1, outfile);
		}
	}
	fclose(outfile);
}







//------------------------------------------------------------------
//+CRi+M Start of CRi method block
//------------------------------------------------------------------
//      Class: SMFontUtil
//
//     Method: SaveFontData
//
//    Returns: void
//
//  Arguments:
//         void
//
//Description:
//         Perform the SaveFontData operation    *AG*
//
//------------------------------------------------------------------
//-CRi-M  End of CRi method block
//------------------------------------------------------------------
void SMFontUtil::SaveFontData(void)
{
	FILE * outfile;
	unsigned int loop;
	outfile = fopen(smffile,"wb");
	loop = cCharCount;
	fwrite(&loop, sizeof(unsigned int), 1, outfile);
	for(loop = 0; loop < cCharCount; loop++)
	{
		SMCharList[loop]->Write(outfile);
		
	}
	fclose(outfile);
}







//------------------------------------------------------------------
//+CRi+M Start of CRi method block
//------------------------------------------------------------------
//      Class: SMFontUtil
//
//     Method: LoadOldStyleXlation
//
//    Returns: void
//
//  Arguments:
//         void
//
//Description:
//         Perform the LoadOldStyleXlation operation    *AG*
//
//------------------------------------------------------------------
//-CRi-M  End of CRi method block
//------------------------------------------------------------------
void SMFontUtil::LoadOldStyleXlation(void)
{

	FILE * infile;
	char inbuffer[4096];
	char * scanner;
	int i,j,k;

	cCharCount = 0;
	CFileDialog open(TRUE,NULL,"*.csv",
		OFN_HIDEREADONLY | OFN_FILEMUSTEXIST,
		"Font Translation File (*.csv)|*.csv||",NULL);
	open.m_ofn.lpstrInitialDir = "\\";
	open.m_ofn.lpstrTitle = "Select Font Translation File";
	open.DoModal();
	if(!(infile = fopen(open.GetPathName(),"rb")))
	{
		MessageBox(NULL,"Translation file did not open\nLoadOldStyleXlationFile","Error",MB_OK);
		_exit(0);
	}

	fgets((char *)inbuffer,4096,infile);
	for(i=0; i<256; i++)
	{
		fgets((char *)inbuffer,4096,infile);
		scanner = inbuffer;
		while(*scanner != ',') scanner++;
		scanner++;
		for(k=0; k<12; k++)
		{
			while(*scanner != ',') scanner++;
			scanner++;
			sscanf(scanner,"%d",&j);
			cXlationTable[k][i] = j;
		}
	}

	fclose(infile);

}








//------------------------------------------------------------------
//+CRi+M Start of CRi method block
//------------------------------------------------------------------
//      Class: SMFontUtil
//
//     Method: LoadOldStyleFile
//
//    Returns: void
//
//  Arguments:
//         void
//
//Description:
//         Perform the LoadOldStyleFile operation    *AG*
//
//------------------------------------------------------------------
//-CRi-M  End of CRi method block
//------------------------------------------------------------------
void SMFontUtil::LoadOldStyleFile(void)
{

	FILE * infile;
	SMCharacter * chr;

	cCharCount = 0;
	CFileDialog open(TRUE,NULL,"*.csv",
		OFN_HIDEREADONLY | OFN_FILEMUSTEXIST,
		"Font Stroke File (*.csv)|*.csv||",NULL);
	open.m_ofn.lpstrInitialDir = "\\";
	open.m_ofn.lpstrTitle = "Select Font Stroke File";
	open.DoModal();
	if(!(infile = fopen(open.GetPathName(),"rb")))
	{
		MessageBox(NULL,"Font file did not open\nLoadOldStyleFile","Error",MB_OK);
		_exit(0);
	}

	for(;;)
	{
		chr = new SMCharacter;//ok
		if(chr->ReadOldStyle(infile))
		{
			SMCharList.Add(chr);
			cCharCount++;
		}
		else
		{
			delete chr;
			break;
		}
	}

	fclose(infile);

}








//------------------------------------------------------------------
//+CRi+M Start of CRi method block
//------------------------------------------------------------------
//      Class: SMFontUtil
//
//     Method: GetCharID
//
//    Returns: int
//
//  Arguments:
//         unsigned char pCharValue
//         FONT_TYPE pFont
//
//Description:
//         Perform the GetCharID operation    *AG*
//
//------------------------------------------------------------------
//-CRi-M  End of CRi method block
//------------------------------------------------------------------
int SMFontUtil::GetCharID(unsigned char pCharValue,
	FONT_TYPE pFont)
{
	int index;
	int charindex;
	index = (int)pFont;
	charindex = pCharValue;
	if(index < 0)
		return(-1);
	if(index > (int)GOTHIC)
		return(-1);
	return(cXlationTable[index][charindex]);
}


void SMFontUtil::DrawString(
	CDC * pDC,				//device context to draw to
	CPoint pStart,			//starting point
	double pRotation,		//rotation angle clockwise in radians (0 = 12:00)
	double pCharWidth,		//width of each character
	double pCharHeight,		//height of each character
	FONT_TYPE pFontType,	//the font to use
	CString pString)		//the string
{
	int curx = pStart.x;
	int cury = pStart.y;
	int length = pString.GetLength();
	double x_scale = (pCharWidth / WADJUST) / BASE_CHARACTER_WIDTH;
	double y_scale = (pCharHeight / HADJUST)  / BASE_CHARACTER_HEIGHT;

	double sin_val = sin(pRotation);
	double cos_val = cos(pRotation);

	double cZoomValue = 1.0;

	double deltax = sin_val * ((pCharWidth / WADJUST) + 2);
	double deltay = cos_val * ((pCharWidth / WADJUST) + 2);

	for(int loop = 0; loop<length; loop++)
	{
		int charid = GetCharID(pString.GetAt(loop),pFontType);
		if(charid>=0)
		{
			CharVertex chrvertex;
			int iter;
			SMCharacter * chr = GetCharacter(charid);
			SMCharacter::CHARVERTEX_TYPE result = 
				chr->GetFirstVertex(chrvertex,iter);
			while(result != SMCharacter::TERMINATE)
			{
				chrvertex.X = chrvertex.X * x_scale;
				chrvertex.Y = (chrvertex.Y+TEXT_DROP) * y_scale;
				if(result == SMCharacter::MOVE_TO)
				{
					pDC->MoveTo(
						(int)(((curx + (sin_val * chrvertex.X - 
							cos_val * chrvertex.Y))) / cZoomValue),
						(int)(((cury + (cos_val * chrvertex.X + 
							sin_val * chrvertex.Y))) / cZoomValue));
				}
				else
				{
					pDC->LineTo(
						(int)(((curx + (sin_val * chrvertex.X - 
							cos_val * chrvertex.Y))) / cZoomValue),
						(int)(((cury + (cos_val * chrvertex.X + 
							sin_val * chrvertex.Y))) / cZoomValue));
				}
				result = chr->GetNextVertex(chrvertex,iter);
			}
		}
		curx += (int)deltax;
		cury += (int)deltay;
	}
}

// GetCharStrokes ... get description of font character including array of strokes
//
// added by Allan Wright Feb. 2003
//
// enter with:
//		coords = pointer to array[max_coords][4] of doubles
//		max_coords = size of coords array
//
// on return: 
//		return value is number of strokes, or negative number if errors
//		coords array is filled with stroke data where:
//			coords[i][0] = starting x for stroke i	
//			coords[i][1] = starting y for stroke i	
//			coords[i][2] = ending x for stroke i	
//			coords[i][3] = ending y for stroke i
//		min_x, min_y, max_x, max_y are filled with boundary box values
//
int SMFontUtil::GetCharStrokes( char ch, FONT_TYPE pFont, double * min_x, double * min_y, 
				   double * max_x, double * max_y, double coords[][4], int max_strokes )
{
	double xi = 0.0, yi = 0.0;
	double min_xx = DEFAULT, min_yy = DEFAULT;
	double max_xx = -DEFAULT, max_yy = -DEFAULT;
	double x, y;
	int charid = GetCharID( ch, pFont );
	int n_strokes = 0;
	if( charid>=0 )
	{
		CharVertex chrvertex;
		SMCharacter::CHARVERTEX_TYPE result;
		SMCharacter * chr;
		int iter;
		chr = GetCharacter( charid );
		result = chr->GetFirstVertex( chrvertex, iter );
		while( result != SMCharacter::TERMINATE )
		{
			x = chrvertex.X;
			y = -chrvertex.Y;
			if( x < min_xx )
				min_xx = x;
			if( x > max_xx )
				max_xx = x;
			if( y < min_yy )
				min_yy = y;
			if( y > max_yy )
				max_yy = y;
			if( result == SMCharacter::MOVE_TO )
			{
				xi = x;
				yi = y;
			}
			else if( result == SMCharacter::DRAW_TO )
			{
				if( n_strokes > max_strokes )
					return -2;		// too many strokes 
				if( coords )
				{
					coords[n_strokes][0] = xi;
					coords[n_strokes][1] = yi;
					coords[n_strokes][2] = x;
					coords[n_strokes][3] = y;
				}
				n_strokes++;
				xi = x;
				yi = y;
			}
			result = chr->GetNextVertex( chrvertex, iter );
		}
		if( min_x )
			*min_x = min_xx;
		if( min_y )
			*min_y = min_yy;
		if( max_x )
			*max_x = max_xx;
		if( max_y )
			*max_y = max_yy;
		return n_strokes;
	}
	else
	{
		if( min_x )
			*min_x = 0;
		if( min_y )
			*min_y = 0;
		if( max_x )
			*max_x = 0;
		if( max_y )
			*max_y = 0;
		return -1;		// illegal charid i.e. unable to find character
	}
}
