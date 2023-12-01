#include<iostream>
#include <fstream>
#include <string>
#include <nlohmann/json.hpp>
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

//#define WINVER 0x0A00
//#define _WIN32_WINNT 0x0A00
#include<Windows.h>
#include<shellapi.h>
#include<psapi.h> // For access to GetModuleFileNameEx
#include<tchar.h>
#include <crtdbg.h>
#include<direct.h>
#include <sys/stat.h>   // stat
#include <stdbool.h>    // bool type
#include <strsafe.h>
#define DOTASK 0x8111

#ifndef UNICODE
#define UNICODE
#endif 

#define BUFSIZE 4096 
 
HANDLE g_hChildStd_IN_Rd = NULL;
HANDLE g_hChildStd_IN_Wr = NULL;
HANDLE g_hChildStd_OUT_Rd = NULL;
HANDLE g_hChildStd_OUT_Wr = NULL;

HANDLE g_hInputFile = NULL;
 
void CreateChildProcess(void); 
void WriteToPipe(void); 
void ReadFromPipe(void); 
void ErrorExit(PTSTR); 

/*typedef struct _NOTIFYICONDATAA {
  DWORD cbSize;
  HWND  hWnd;
  UINT  uID;
  UINT  uFlags;
  UINT  uCallbackMessage;
  HICON hIcon;

  CHAR  szTip[64];
  CHAR  szTip[128];
  DWORD dwState;
  DWORD dwStateMask;
  CHAR  szInfo[256];
  union {
    UINT uTimeout;
    UINT uVersion;
  } DUMMYUNIONNAME;
  CHAR  szInfoTitle[64];
  DWORD dwInfoFlags;
  GUID  guidItem;
  HICON hBalloonIcon;
} NOTIFYICONDATAA, *PNOTIFYICONDATAA;
*/

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int main()
{
        SECURITY_ATTRIBUTES saAttr; 
 
   printf("\n->Start of parent execution.\n");

// Set the bInheritHandle flag so pipe handles are inherited. 
 
   saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
   saAttr.bInheritHandle = TRUE; 
   saAttr.lpSecurityDescriptor = NULL; 

// Create a pipe for the child process's STDOUT. 
 
   if ( ! CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0) ) 
      ErrorExit(TEXT("StdoutRd CreatePipe")); 

// Ensure the read handle to the pipe for STDOUT is not inherited.

   if ( ! SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0) )
      ErrorExit(TEXT("Stdout SetHandleInformation")); 

// Create a pipe for the child process's STDIN. 
 
   if (! CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0)) 
      ErrorExit(TEXT("Stdin CreatePipe")); 

// Ensure the write handle to the pipe for STDIN is not inherited. 
 
   if ( ! SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0) )
      ErrorExit(TEXT("Stdin SetHandleInformation")); 
 
// Create the child process. 
   
   CreateChildProcess();

// Get a handle to an input file for the parent. 
// This example assumes a plain text file and uses string output to verify data flow. 
 std::cout<<"after create process";
//   char str0[]= "comm.txt";
//   LPCSTR filen= "C:/Users/Abhirup/Documents/ActivityTracker/comm.txt";

//    g_hInputFile = CreateFile(
//        filen, 
//        GENERIC_READ | GENERIC_WRITE, 
//        FILE_SHARE_READ, 
//        NULL, 
//        OPEN_EXISTING, 
//        FILE_ATTRIBUTE_NORMAL, 
//        NULL); 

//    if ( g_hInputFile == INVALID_HANDLE_VALUE ) 
//       ErrorExit(TEXT("CreateFile")); 


// Write to the pipe that is the standard input for a child process. 
// Data is written to the pipe's buffers, so it is not necessary to wait
// until the child process is running before writing data.
 
   
    HINSTANCE window;
    //LPSTR cmdline= GetCommandLine();
    wWinMain(window,NULL,GetCommandLineW(),SW_SHOW);

    //WriteToPipe(); 
   //printf( "\n->Contents of %S written to child STDIN pipe.\n", str0);
 
// Read from pipe that is the standard output for child process. 
 
   //printf( "\n->Contents of child process STDOUT:\n\n");
   //ReadFromPipe(); 
    
    return 0;
}
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    //BOOL MyTaskBarAddIcon(HWND, UINT, HICON, LPSTR); 
   // FILE* fp;
    //fp=fopen("example.ico","rb");
    //HICON ico =CreateIconFromResource(fp,sizeof(HICON),TRUE,0x00030000);
    HICON ico =(HICON)LoadImageA(NULL,"example.ico",IMAGE_ICON,64,64,LR_LOADFROMFILE);
//std::cout<<GetLastError();
    // Register the window class.
    //const wchar_t CLASS_NAME[]  = L"Sample Window Class";
    LPCSTR CLASS_NAME = "Sample Window Class";
    LPCSTR WINDOW_NAME = "Sample Window";
    WNDCLASS wc = { };

    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hIcon=ico;

    RegisterClass(&wc);

    // Create the window.

    HWND hwnd = CreateWindowEx(
        0,                              // Optional window styles.
        CLASS_NAME,                     // Window class
        WINDOW_NAME,    // Window text
        WS_OVERLAPPEDWINDOW,            // Window style

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

        NULL,       // Parent window    
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
        );
   // MyTaskBarAddIcon(hwnd,1,ico,NULL);
    if (hwnd == NULL)
    {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
        BOOL res; 
         LPSTR lpszTip=NULL;
    NOTIFYICONDATA tnid={}; 
 
    tnid.cbSize = sizeof(NOTIFYICONDATA); 
    tnid.hWnd = hwnd; 
    tnid.uID = 1; 
    tnid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP; 
    //tnid.uCallbackMessage = MYWM_NOTIFYICON;
    tnid.uCallbackMessage = DOTASK; 
    tnid.hIcon = ico; 
    tnid.uVersion = NOTIFYICON_VERSION_4;
    /*if (lpszTip) 
        hr = StringCbCopyN(tnid.szTip, sizeof(tnid.szTip), lpszTip, 
                           sizeof(tnid.szTip));
        // TODO: Add error handling for the HRESULT.
    else 
        tnid.szTip[0] = (TCHAR)'\0'; */
 
    res = Shell_NotifyIcon(NIM_ADD, &tnid); 
 
    if (ico) 
        DestroyIcon(ico);
    // Run the message loop.

    MSG msg = { };
   msg.hwnd = hwnd;  // THIS IS NOT REQUIRED, PROBABLY

   SYSTEMTIME DATE_TIME;
	GetLocalTime(&DATE_TIME);

	unsigned short hour =DATE_TIME.wHour;
	unsigned short minute =DATE_TIME.wMinute;
	unsigned short day= DATE_TIME.wDay;
	unsigned short month =DATE_TIME.wMonth;
	unsigned short year =DATE_TIME.wYear;
std::string filenamestr = "./data/" + std::to_string(day)+'-'+ std::to_string(month) +"-"+std::to_string(year)+ ".json";
const char* filename = filenamestr.c_str();
	FILE* currDay; // Used to point to a file which stores data about the current day's app data
	FILE* data;		// Used to check if the current day's data exists, if it doesn't a new json is created at filename dir
    struct stat   buffer;  
    std::ifstream f("example.json");
    if(!stat (filename, &buffer) == 0)
    {
       data = fopen(filename, "a");
       fprintf(data,"%s","{}"); 
       //std::ifstream f(filename);
        fclose(data);
    }
    
	data = fopen(filename, "r");
   // std::ifstream f(filename);
    
nlohmann::json j = nlohmann::json::parse(data);
j["pi"]=3.14;
//std::cout<<j["pi"];
	DWORD pid;
	HWND info;// = GetForegroundWindow();
    TCHAR exepath[MAX_PATH+1];
	GetModuleFileName(0, exepath, MAX_PATH + 1);
	
	LPSTR str; LPSTR str2;
    
	//out << str<<pid;
	
	//QueryFullProcessImageName(info,0,str2,(PDWORD)50);
	
	//Sleep(3000);
	//SwitchToThisWindow(info, FALSE);
    HANDLE hPipe;
    char buffer[1024];
    DWORD dwRead;


    hPipe = CreateNamedPipe(TEXT("\\\\.\\pipe\\Pipe"),
                            PIPE_ACCESS_DUPLEX,
                            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,   // FILE_FLAG_FIRST_PIPE_INSTANCE is not needed but forces CreateNamedPipe(..) to fail if the pipe already exists...
                            1,
                            1024 * 16,
                            1024 * 16,
                            NMPWAIT_USE_DEFAULT_WAIT,
                            NULL);
 
	
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
  
        TranslateMessage(&msg);
       // std::cout<<msg.message<<std::endl;
        if(msg.message==0x8111)
        {
            POINT mousePos ={};
            GetCursorPos(&mousePos);
            msg.lParam=(LPARAM)&mousePos;
           // std::cout<<mousePos.x<<std::endl;
        }
        DispatchMessage(&msg);
    }
        
            while (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &dwRead, NULL) != FALSE)
            {
                /* add terminating zero */
                buffer[dwRead] = '\0';

                /* do something with data in buffer */
                printf("%s", buffer);
            }
        


    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if(uMsg==0x8111)
    {
        std::cout<<uMsg<<std::endl;
     
    }
    
    if(uMsg==0x0024)
    {
       // MINMAXINFO* min={};
        //min=(MINMAXINFO*)LOWORD((DWORD)lParam);
        //POINT cord= min->ptMaxPosition;
   // std::cout<<uMsg<<std::endl<<cord.y; //lParam->ptMaxPosition->x;
    }
   switch (uMsg)
    {
      case DOTASK:
    {
        //std::cout<<LOWORD(lParam);
       // LPPOINT mousePos={};
        POINT mousePos = {};
         GetCursorPos(&mousePos);
        std::cout<<mousePos.x<<std::endl;
           HMENU barmenu= CreatePopupMenu();
        TrackPopupMenu(barmenu,TPM_LEFTALIGN,mousePos.x,mousePos.y,0,hwnd,NULL);
        DestroyMenu(barmenu);
        //std::cout<<GetLastError();
        //HandlePopupMenu(hwnd,mousePos);
        //mousePos=(LPPOINT)lParam;
        //OutputDebugStringW((LPCWSTR)mousePos->x);
        //std::cout<<mousePos->x;    
     
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            // All painting occurs here, between BeginPaint and EndPaint.

            FillRect(hdc, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW+1));

            EndPaint(hwnd, &ps);
        }
        return 0;

    
    

    }

    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);  
}


bool file_exists (char *filename) {
  struct stat   buffer;   
  return (stat (filename, &buffer) == 0);
}


/*BOOL MyTaskBarAddIcon(HWND hwnd, UINT uID, HICON hicon, LPSTR lpszTip) 
{ 
    BOOL res; 
    NOTIFYICONDATA tnid; 
 
    tnid.cbSize = sizeof(NOTIFYICONDATA); 
    tnid.hWnd = hwnd; 
    tnid.uID = uID; 
    tnid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP; 
    tnid.uCallbackMessage = MYWM_NOTIFYICON; 
    tnid.hIcon = hicon; 
    if (lpszTip) 
        hr = StringCbCopyN(tnid.szTip, sizeof(tnid.szTip), lpszTip, 
                           sizeof(tnid.szTip));
        // TODO: Add error handling for the HRESULT.
    else 
        tnid.szTip[0] = (TCHAR)'\0'; 
 
    res = Shell_NotifyIcon(NIM_ADD, &tnid); 
 
    if (hicon) 
        DestroyIcon(hicon); 
 
    return res; 
}
*/

void CreateChildProcess()
// Create a child process that uses the previously created pipes for STDIN and STDOUT.
{ 
   TCHAR szCmdline[]=TEXT("child");
   PROCESS_INFORMATION piProcInfo; 
   STARTUPINFO siStartInfo;
   BOOL bSuccess = FALSE; 
 
// Set up members of the PROCESS_INFORMATION structure. 
 
   ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );
 
// Set up members of the STARTUPINFO structure. 
// This structure specifies the STDIN and STDOUT handles for redirection.
 
   ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
   siStartInfo.cb = sizeof(STARTUPINFO); 
   siStartInfo.hStdError = g_hChildStd_OUT_Wr;
   siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
   siStartInfo.hStdInput = g_hChildStd_IN_Rd;
   siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
 
// Create the child process. 
    LPCSTR windwatcher = "C:/Users/Abhirup/Documents/ActivityTracker/pw-window-watcher.exe";
   bSuccess = CreateProcess(windwatcher, // the exe to start
      szCmdline,     // command line 
      NULL,          // process security attributes 
      NULL,          // primary thread security attributes 
      TRUE,          // handles are inherited 
      0,             // creation flags 
      NULL,          // use parent's environment 
      NULL,          // use parent's current directory 
      &siStartInfo,  // STARTUPINFO pointer 
      &piProcInfo);  // receives PROCESS_INFORMATION 
   
   // If an error occurs, exit the application. 
   if ( ! bSuccess ) 
      ErrorExit(TEXT("CreateProcess"));
   else 
   {
      // Close handles to the child process and its primary thread.
      // Some applications might keep these handles to monitor the status
      // of the child process, for example. 

      CloseHandle(piProcInfo.hProcess);
      CloseHandle(piProcInfo.hThread);
      
      // Close handles to the stdin and stdout pipes no longer needed by the child process.
      // If they are not explicitly closed, there is no way to recognize that the child process has ended.
      
      CloseHandle(g_hChildStd_OUT_Wr);
      CloseHandle(g_hChildStd_IN_Rd);
   }
}
 
void WriteToPipe(void) 

// Read from a file and write its contents to the pipe for the child's STDIN.
// Stop when there is no more data. 
{ 
   DWORD dwRead, dwWritten; 
   CHAR chBuf[BUFSIZE];
   BOOL bSuccess = FALSE;
 
   for (;;) 
   { 
      bSuccess = ReadFile(g_hInputFile, chBuf, BUFSIZE, &dwRead, NULL);
      if ( ! bSuccess || dwRead == 0 ) break; 
      
      bSuccess = WriteFile(g_hChildStd_IN_Wr, chBuf, dwRead, &dwWritten, NULL);
      if ( ! bSuccess ) break; 
   } 
 
// Close the pipe handle so the child process stops reading. 
 
   if ( ! CloseHandle(g_hChildStd_IN_Wr) ) 
      ErrorExit(TEXT("StdInWr CloseHandle")); 
} 
 
void ReadFromPipe(void) 

// Read output from the child process's pipe for STDOUT
// and write to the parent process's pipe for STDOUT. 
// Stop when there is no more data. 
{ 
   DWORD dwRead, dwWritten; 
   CHAR chBuf[BUFSIZE]; 
   BOOL bSuccess = FALSE;
   HANDLE hParentStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    std::cout<<GetStdHandle(STD_OUTPUT_HANDLE);
   for (;;) 
   { 
      bSuccess = ReadFile( g_hChildStd_OUT_Rd, chBuf, BUFSIZE, &dwRead, NULL);
      if( ! bSuccess || dwRead == 0 ) break; 

      bSuccess = WriteFile(hParentStdOut, chBuf,dwRead, &dwWritten, NULL);
      if (! bSuccess ) break; 
   } 
} 
 
void ErrorExit(PTSTR lpszFunction) 

// Format a readable error message, display a message box, 
// and exit from the application.
{ 
    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
        (lstrlen((LPCTSTR)lpMsgBuf)+lstrlen((LPCTSTR)lpszFunction)+40)*sizeof(TCHAR)); 
    StringCchPrintf((LPTSTR)lpDisplayBuf, 
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("%s failed with error %d: %s"), 
        lpszFunction, dw, lpMsgBuf); 
    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK); 

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
    ExitProcess(1);
}