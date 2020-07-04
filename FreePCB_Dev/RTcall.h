#pragma once

HANDLE SpawnAndRedirect( LPCTSTR commandLine, 
						 HANDLE *hStdOutputReadPipe, 
						 LPCTSTR lpCurrentDirectory );

int RunConsoleProcess( LPCTSTR commandLine, CDlgLog * log );
