
#ifndef _SMFONT_UTIL_
#define _SMFONT_UTIL_
#include "SMCharacter.h"
#include <afxtempl.h>


enum FONT_TYPE
{
	SMALL_SIMPLEX,
	SMALL_DUPLEX,
	SIMPLEX,
	DUPLEX,
	TRIPLEX,
	MODERN,
	SCRIPT_SIMPLEX,
	SCRIPT_DUPLEX,
	ITALLIC_DUPLEX,
	ITALLIC_TRIPLEX,
	FANCY,
	GOTHIC
};


class SMFontUtil
{
friend class SMCMainFrame;
friend class CFontManagerDoc;
friend class CFontManagerView;
friend class CCharEditDlg;

private:
	unsigned int cCharCount;
	CTypedPtrArray<CPtrArray, SMCharacter*> SMCharList;
	int cXlationTable[12][256];

protected:
	void LoadOldStyleFile(void);
	int LoadFontData(void);
	void SaveFontData(void);
	int LoadXlationData(void);
	void SaveXlationData(void);
	void LoadOldStyleXlation(void);

public:

	void DrawString(
		CDC * pDC,				//device context to draw to
		CPoint pStart,			//starting point
		double pRotation,		//rotation angle clockwise in radians (0 = 12:00)
		double pCharWidth,		//width of each character
		double pCharHeight,		//height of each character
		FONT_TYPE pFontType,	//the font to use
		CString pString);		//the string



	SMFontUtil(CString * path);
	~SMFontUtil();
	int GetMaxChar(void)
	{
		return(cCharCount - 1);
	}

	int AddCharacter(SMCharacter * pChar)
	{
		cCharCount++;
		return SMCharList.Add(pChar);
	}

	void SetCharID(unsigned char pCharValue,
		FONT_TYPE pFont, int ID)
	{
		cXlationTable[(int)pFont][(int)pCharValue] = ID;
	}

	SMCharacter * GetCharacter(int pCharID)
	{
		return(SMCharList[pCharID]);
	}

	int GetCharID(unsigned char pCharValue,
		FONT_TYPE pFont);

	// added by Allan Wright Feb. 2002
	int GetCharStrokes( char ch, FONT_TYPE pFont, double * min_x, double * min_y, 
			double * max_x, double * max_y, double coords[][4], int max_strokes );
	  
};
#endif

