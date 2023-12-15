#include <windows.h>
#include <stdio.h>
#include<psapi.h>
#include<iostream>
#define BUFSIZE 16384
 
int main(void) 
{ 
   //  TCHAR exepath[MAX_PATH+1];
  
   // DWORD dwRead, dwWritten; 
   // HANDLE hStdin, hStdout; 
   // BOOL bSuccess; 
 
   // hStdout = GetStdHandle(STD_OUTPUT_HANDLE); 
   // hStdin = GetStdHandle(STD_INPUT_HANDLE); 
   // if ( 
   //     (hStdout == INVALID_HANDLE_VALUE) || 
   //     (hStdin == INVALID_HANDLE_VALUE) 
   //    ) {
   //    ExitProcess(1);} 

   
   //HWND windowHandle = GetConsoleWindow();
//ShowWindow(windowHandle, SW_HIDE);
   // Send something to this process's stdout using printf.
   printf("\n ** This is a message from the child process. ** \n");

   // This simple algorithm uses the existence of the pipes to control execution.
   // It relies on the pipe buffers to ensure that no data is lost.
   // Larger applications would use more advanced process control.

  /* for (;;) 
   { 
   // Read from standard input and stop on error or no data.
      bSuccess = ReadFile(hStdin, chBuf, BUFSIZE, &dwRead, NULL); 
      
      if (! bSuccess || dwRead == 0) 
         break; 
 
   // Write to standard output and stop on error.
      bSuccess = WriteFile(hStdout, chBuf, dwRead, &dwWritten, NULL); 
      
      if (! bSuccess) 
         break; 
   }
   */ 
   	DWORD pid;
	HWND info;// = GetForegroundWindow();
	
	LPSTR str; LPSTR str2;
   // CHAR chBuf[BUFSIZE]; 
while(TRUE)
{
   HANDLE hPipe;
    DWORD dwWritten;


    hPipe = CreateFile(TEXT("\\\\.\\pipe\\Pipe"), 
                       GENERIC_READ | GENERIC_WRITE, 
                       0,
                       NULL,
                       OPEN_EXISTING,
                       0,
                       NULL);
	Sleep(1000);
	info = GetForegroundWindow();
	GetWindowTextA(info, str, GetWindowTextLength(info) + 1);
	GetWindowThreadProcessId(info, &pid);
	std::cout<<str<<std::endl;
	HANDLE Handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
	if (Handle)
	{
		TCHAR Buffer[BUFSIZE];
		DWORD buffSize = 16384;
		//CHAR buffer[1024];
		//if (GetModuleFileNameEx(Handle, 0, Buffer, MAX_PATH)) // Slower method
		if(QueryFullProcessImageNameA(Handle, 0, Buffer, &buffSize))  //Faster method
		{
        // chBuf =Buffer;
			std::cout << str << Buffer<<std::endl;  // At this point, buffer contains the full path to the executable
		}
		else
		{
			std::cout << str << "Can't get executable path" << GetLastError() <<std::endl;// You better call GetLastError() here
		}
     

      // bSuccess = WriteFile(hStdout, chBuf, dwRead, &dwWritten, NULL); 
		// CloseHandle(Handle);
	}
	else
	{
		std::cout << str << "Can't get handle to procces from PID" << GetLastError() << std::endl;
	}
  
  /*  bSuccess = ReadFile(hStdin, chBuf, BUFSIZE, &dwRead, NULL); 
      
      if (! bSuccess || dwRead == 0) 
         break; 
 */
   // Write to standard output and stop on error.
    // bSuccess = WriteFile(hStdout, chBuf, dwRead, &dwWritten, NULL); 
      
      
	//out << str<<pid;
	
	//QueryFullProcessImageName(info,0,str2,(PDWORD)50);
	
	//Sleep(3000);
	//SwitchToThisWindow(info, FALSE);
	
}
   return 0;
}