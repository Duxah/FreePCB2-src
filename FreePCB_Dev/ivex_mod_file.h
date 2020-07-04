// functions to convert Ivex .mod file to FreePCB .lib file
//
#include "stdafx.h"
#include "Shape.h"

int ConvertIvex( CStdioFile * mod_file, CStdioFile * lib_file, CEdit * edit_ctrl );
int WriteFootprint( CStdioFile * file, CShape * s );
