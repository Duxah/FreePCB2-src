#include "stdafx.h"

// The following function is a shortened variant of Q190351 - HOWTO: Spawn Console Processes with Redirected Standard Handles
// There is no special magic here, and this version doesn't address issues like:
// - redirecting Input handle
// - spawning 16-bits process (well, RTconsole is a 32-bit process anyway so it should solve the problem)
// - command-line limitations (unsafe 1024-char buffer)
// So you might want to use more advanced versions such as the ones you can find on CodeProject
HANDLE SpawnAndRedirect( LPCTSTR commandLine, 
						 HANDLE *hStdOutputReadPipe, 
						 LPCTSTR lpCurrentDirectory )
{
	HANDLE hStdOutputWritePipe, hStdOutput, hStdError;
	CreatePipe(hStdOutputReadPipe, &hStdOutputWritePipe, NULL, 0);	// create a non-inheritable pipe
	DuplicateHandle(GetCurrentProcess(), hStdOutputWritePipe,
								GetCurrentProcess(), &hStdOutput,	// duplicate the "write" end as inheritable stdout
								0, TRUE, DUPLICATE_SAME_ACCESS);
	DuplicateHandle(GetCurrentProcess(), hStdOutput,
								GetCurrentProcess(), &hStdError,	// duplicate stdout as inheritable stderr
								0, TRUE, DUPLICATE_SAME_ACCESS);
	CloseHandle(hStdOutputWritePipe);								// no longer need the non-inheritable "write" end of the pipe

	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	ZeroMemory(&si, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
	si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);	// (this is bad on a GUI app)
	si.hStdOutput = hStdOutput;
	si.hStdError  = hStdError;
	si.wShowWindow = SW_HIDE;						// IMPORTANT: hide subprocess console window
	TCHAR commandLineCopy[1024];					// CreateProcess requires a modifiable buffer
	_tcscpy(commandLineCopy, commandLine);
	if (!CreateProcess(	NULL, commandLineCopy, NULL, NULL, TRUE,
						CREATE_NEW_CONSOLE, NULL, lpCurrentDirectory, &si, &pi))
	{
		CloseHandle(hStdOutput);
		CloseHandle(hStdError);
		CloseHandle(*hStdOutputReadPipe);
		*hStdOutputReadPipe = INVALID_HANDLE_VALUE;
		return NULL;
	}

	CloseHandle(pi.hThread);
	CloseHandle(hStdOutput);
	CloseHandle(hStdError);
	return pi.hProcess;
}

// execute a command-line as a console process, with output redirected to log
//
int RunConsoleProcess( LPCTSTR commandLine, CDlgLog * log )
{
		HANDLE hOutput, hProcess;
		hProcess = SpawnAndRedirect(commandLine, &hOutput, NULL); 
		if (!hProcess) 
		{
			if( log )
				log->AddLine( "Failed!\r\n" );
			return 1;
		}

		// if necessary, this could be put in a separate thread so the GUI thread is not blocked
		AfxGetApp()->DoWaitCursor(1);		
		CHAR buffer[65];
		DWORD read;
		while (ReadFile(hOutput, buffer, 64, &read, NULL))
		{
			buffer[read] = '\0';
			if( log )
				log->AddLine( buffer );
		}
		CloseHandle(hOutput);
		CloseHandle(hProcess);
		AfxGetApp()->EndWaitCursor();
		return 0;
}


#if 0
void CDemoDlg::Go(LPCTSTR commandLine)
{
	m_log.SetWindowText(NULL);
	HANDLE hOutput, hProcess;
	hProcess = SpawnAndRedirect(commandLine, &hOutput, NULL);
	if (!hProcess) return;
	m_log.SetWindowText("Child started ! Receiving output pipe :\r\n");
	m_log.SetSel(MAXLONG, MAXLONG);
	m_log.RedrawWindow();
	
	// (in a real program, you would put the following in a separate thread so your GUI thread is not blocked)
	BeginWaitCursor();
	CHAR buffer[65];
	DWORD read;
	while (ReadFile(hOutput, buffer, 64, &read, NULL))
	{
		buffer[read] = '\0';
		m_log.ReplaceSel(buffer);
	}
	CloseHandle(hOutput);
	CloseHandle(hProcess);
	EndWaitCursor();
}

void CDemoDlg::OnGo() 
{
	// spawn console program directly
	Go("\"DemoConsole.exe\"");
}

void CDemoDlg::OnGo2() 
{
	// spawn RTconsole.exe, passing it the command-line to run the console program
	Go("\"RTconsole.exe\"  \"DemoConsole.exe\"");
}
#endif