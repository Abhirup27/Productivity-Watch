/*
pw-window-watcher returns text to pw-server in the following format:
 [Type of message <int>] [ the message]
 message code 1: returns the message that indicates what process is being used. pw-server reads 1 and adds application to the vector if new and adds seconds to the struct.
 e.g. : 1 <pid><app path>.exe <window title> 

 message code 2:	returns if the process id is still running. if not, pw-server sets 0 to the use_pid variable in app vector struct.
					which means if the same application.exe of same window starts again, pw-server  has to find it using pid and change and change the pid of i
e.g.: 2 <pid> <bool>

message code 3:		if the user has changed the window focused, it sends 
e.g. 3 <pid><app path>.exe <window title> 
					The pw-server compares the app path with the recieved app path at the pid app index, if different, pw-server hashes the app name at the index and stores it in json array, it increments the seconds used. 

*/

#include"pw-window-watcher2.h"
#include<iostream>
#include<algorithm>
#include <fstream>
#include <string>
#include <nlohmann/json.hpp>
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
//#define _WIN32_WINNT_WINTHRESHOLD           0x0A00 // Windows 10
//#define _WIN32_WINNT_WIN10                  0x0A00 // Windows 10
//#define WINVER 0x0A00
//#define _WIN32_WINNT 0x0A00
#include<Windows.h>
#include<psapi.h> // For access to GetModuleFileNameEx
#include<tchar.h>
#include<sys/stat.h>

typedef struct data_block
{
    LPSTR app_name;
    TCHAR app_path[MAX_PATH];
} data_block;


int main()
{
	
      HANDLE hPipe;
    DWORD dwWritten;
	TCHAR exepath[MAX_PATH+1];
	GetModuleFileName(0, exepath, MAX_PATH + 1);
	HKEY hkey = NULL;
	LPCSTR appName = "test";
	LPSTR appName2;
	LPDWORD size=(long unsigned int*)1024;
	LPDWORD type, datasize;
	LPBYTE data1;
	LPCSTR regPath = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
	LPCSTR subKey = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run\\test" ;
	DWORD index = 0;
	DWORD dwSize = 1024;
	//RegCreateKeyA(HKEY_CURRENT_USER, regPath, &hkey);

	//RegSetValueExA(hkey, appName, 0, REG_SZ, (BYTE*)exepath, MAX_PATH);
	//out<<(int)RegCopyTreeA(HKEY_CURRENT_USER, regPath, hkey);
	
	int lResult = RegOpenKeyExA(HKEY_CURRENT_USER, regPath, 0, KEY_READ, &hkey);
	//out << lResult;
	//out << RegEnumValueA(hkey, index, appName2, &dwSize, NULL, NULL, NULL, NULL);

	// NEED___TO__FIX THE WHILE BLOCK
/*
	while (ERROR_SUCCESS ==lResult)
	{
		lResult = RegEnumValueA(hkey, index, appName2, (LPDWORD)MAXDWORD, NULL, NULL, NULL, NULL);
		out << lResult<<std::endl<<index;
		//lResult = 1;
		if (appName2 == appName)
		{

			break;
		}
		dwSize = 1024;
		index++;
	}
	*/
	if (lResult!= ERROR_SUCCESS) //if the "test" StringName is found in hkey
	{
		//out << appName2;
		//out << lResult;
		if (lResult == ERROR_FILE_NOT_FOUND) {
			
			int code=MessageBox(0,"Automatically start ActivityWatch on system startup?","Alert",MB_OKCANCEL);
			if(code== IDOK)
			{
				HKEY hkey2 = NULL;
				RegCreateKeyA(HKEY_CURRENT_USER, regPath, &hkey2);
				RegSetValueExA(hkey2, appName, 0, REG_SZ, (BYTE*)exepath,MAX_PATH);
				//std::cout<<"ok";
			}
			else if(code == IDCANCEL)
			{
				//std::cout<<"cancel";
			}
		}

	}
	
	 // NEED ____ TO _FIX
	/*if (data = fopen(filename, "r"))
	{
		//out << "file exists \n";
		
	}
	else
	{
		//out<< "file doesn't exist \n";
		out<<filename;
		fclose(data);
		data =fopen(filename, "a");
	}*/
	
    
	DWORD pid;
	HWND info;// = GetForegroundWindow();
	
	LPSTR str; LPSTR str2; 
	//uint8_t code=1;

    
while(TRUE)
{

	Sleep(1000);
	info = GetForegroundWindow();
	GetWindowTextA(info, str, GetWindowTextLength(info) + 1);
	GetWindowThreadProcessId(info, &pid);

	/*if(str2 == NULL)
	{
		str2=str;
		
	}
	else
	{
		if(str==str2)
		{
			code=1;
		}
		else
		{
			code=3;
			str2=str;
		}
	}
	*/
	HANDLE Handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
	if (Handle)
	{
        
		TCHAR Buffer[MAX_PATH];
		DWORD buffSize = MAX_PATH;
		//CHAR buffer[1024];
		//if (GetModuleFileNameEx(Handle, 0, Buffer, MAX_PATH)) // Slower method
		if(QueryFullProcessImageNameA(Handle, 0, Buffer, &buffSize))  //Faster method
		{
			//std::cout <<std::endl<< str << Buffer << std::endl<<exepath<<std::endl;  // At this point, buffer contains the full path to the executable
		}
		else
		{
			std::cout << str << "Can't get executable path" << GetLastError() <<std::endl;// You better call GetLastError() here
		}
       // lstrcatA(str,(LPCSTR)'\n');
	   lstrcatA(str, " ");
        lstrcatA(str,Buffer);
		TCHAR buf[5];
		_itot(pid, buf, 10);
		lstrcatA(str, " ");
		lstrcatA(str,buf);
		std::cout<<str;
          hPipe = CreateFile(TEXT("\\\\.\\pipe\\Pipe"), 
                       GENERIC_READ | GENERIC_WRITE, 
                       0,
                       NULL,
                       OPEN_EXISTING,
                       0,
                       NULL);
         if (hPipe != INVALID_HANDLE_VALUE)
    {
        WriteFile(hPipe,
                  str,
                  lstrlen(str)+1 * sizeof(TCHAR),   // = length of string + terminating '\0' !!!
                  &dwWritten,
                  NULL);

        CloseHandle(hPipe);
    }
    else{
      std::cout<<"couldn't write"<<std::endl<<GetLastError()<<std::endl;
    }
		CloseHandle(Handle);
        
	}
	else
	{
		std::cout << str << "Can't get handle to procces from PID" << GetLastError() << std::endl;
	}
	//out << str<<pid;
	
	//QueryFullProcessImageName(info,0,str2,(PDWORD)50);
	
	//Sleep(3000);
	//SwitchToThisWindow(info, FALSE);
	
}
	
return 0;
}
