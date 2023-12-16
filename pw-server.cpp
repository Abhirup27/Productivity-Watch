#include<iostream>
#include <fstream>
#include <string>
#include <nlohmann/json.hpp>
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

//#define WINVER 0x0A00
//#define _WIN32_WINNT 0x0A00
#include<Windows.h>
#include<windowsx.h>
#include<winbase.h>
#include<shellapi.h>
#include<psapi.h> // For access to GetModuleFileNameEx
#include<tchar.h>
#include <crtdbg.h>
#include<direct.h>
#include <sys/stat.h>   // stat
#include <stdbool.h>    // bool type
#include <strsafe.h>
#include <mutex>
#include<future> // for async
#include <functional>
 #include<sstream>
std::mutex m; // FOR ASYNC with pipe function
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
//void WriteToPipe(void); 
//void ReadFromPipe(void); 
void ErrorExit(PTSTR); 

struct Time{

    unsigned short seconds=0;
    unsigned short minutes=0;
    unsigned short hours=0;
};

PROCESS_INFORMATION watcher_info = {};

bool IsWatcherRunning();
bool iswatcherRunning=false;
void WriteToAppArray(bool);
void WriteToAppArray(char [],std::string,std::string,std::string,Time*); // the named pipe writes to the struct array when pw-window-watcher sends data.
void WriteToJSONobj(); //Called when the server wants to write to file or when the pw-gui requests the JSON obj for current day data
void WriteToJSONobj(FILE*); // when ReadFromFile is called on startup. this func then calls WriteToAppArray()
void WriteToFile(); // Called when the system clock is 00:00 or the pw-server is closed.
void ReadFromFile();


void ResizeAppVector();
void ResizeAppVector(size_t);
void DeleteAppVector();
void genHash(std::string, std::string);

   struct Application{

    unsigned int pid;
    std::string hash= "";
    std::string name="";
    std::string dir="";
    Time time;
    bool use_pid;
    bool isEmpty=true;
   }*lastApp=NULL;
   std::vector<Application> apps;
   nlohmann::json currDayAppData;// =nlohmann::json::array();;
   SYSTEMTIME CURR_DATE_TIME;
struct X
{

    HANDLE hPipe;
    char buffer2[1024];
    DWORD dwRead;
void PipeRead()
{
      // Create the child process. The pw-window-watcher must be started by the concurrent thread, not in the main or winmain function (main thread)
   
   CreateChildProcess();
    std::lock_guard<std::mutex> lk(m);
    hPipe = CreateNamedPipe(TEXT("\\\\.\\pipe\\Pipe"),
                                PIPE_ACCESS_DUPLEX,
                                PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,   // FILE_FLAG_FIRST_PIPE_INSTANCE is not needed but forces CreateNamedPipe(..) to fail if the pipe already exists...
                                1,
                                1024 * 32,
                                1024 * 32,
                                NMPWAIT_USE_DEFAULT_WAIT,
                                NULL);
        while (hPipe != INVALID_HANDLE_VALUE)
        {
            if (ConnectNamedPipe(hPipe, NULL) != FALSE)   // wait for someone to connect to the pipe
            {
                while (ReadFile(hPipe, buffer2, sizeof(buffer2) - 1, &dwRead, NULL) != FALSE)
                {
                    /* add terminating zero */
                    buffer2[dwRead] = '\0';

                    /* do something with data in buffer */
                   // printf("%s", buffer2);
                   WriteToAppArray(buffer2, "","","",NULL);
                    if(dwRead==1024)
                    {
                        dwRead=0;
                    }
                }
             //   std::cout<<GetLastError()<<std::endl;
            }

            DisconnectNamedPipe(hPipe);
        }

}
};
X x;
   



void WriteToAppArray(char Buffer[] =NULL,std::string name="",std::string dir="",std::string pid="",Time* ti=NULL)
{
    
   // fetch name and other details from buffer
  // DWORD pid;
    int index=0;
    //NOW FOR APP'S NAME
    if(Buffer !=NULL)
    {
        while((Buffer[index]!=' ' || (Buffer[index+1]!='C' && Buffer[index+2]!=':'))
         ||( Buffer[index]!=' ' || (Buffer[index+1]!='D' && Buffer[index+2]!=':'))
         || ( Buffer[index]!=' ' || (Buffer[index+1]!='E' && Buffer[index+2]!=':'))
         || ( Buffer[index]!=' ' || (Buffer[index+1]!='F' && Buffer[index+2]!=':'))
        )  
        {
            if(Buffer[index]>=0 &&Buffer[index]<128)
            {
            name.push_back(Buffer[index]);
            }
            index++;
        }
        index++;
        //NOW FOR APP'S DIRECTORY
        while(Buffer[index]!=' ' || (Buffer[index-1]!='e'&& Buffer[index-2]!='x'&& Buffer[index-3]!='e'&& Buffer[index-4]!='.'))
        {
            dir.push_back(Buffer[index]);
           
            index++;
        }
        index++;
    //FOR PID
    int digits=1;
    
     while(Buffer[index]!='\0')
    {
      //   std::cout<<"execute";
        //num= num*digits + (int)Buffer[index];
        //digits*=10;
        pid.push_back(Buffer[index]);
        index++;
    }
    }std::cout<<name<<dir<<pid<<std::endl;
   
   size_t sizeApps= apps.size();
    size_t pid_len= pid.length();
    //std::cout<<"running";
    if(lastApp==NULL) //window title = name
    {
       
        //then find the app using a hash in the array, if not found, create new
        Application newApp;
        newApp.name=name; newApp.dir=dir; newApp.pid=stoi(pid);
        if(apps.empty()==true)
        {
            //std::cout<<"running";
            ResizeAppVector();
            std::cout<<name<<pid<<dir<<pid[0];
            apps[int(pid[0])- 48].name=name;
            std::cout<<"running";
            apps[int(pid[0])- 48].dir=dir;
            apps[int(pid[0])- 48].pid=stoi(pid);
            
            if(ti==NULL)
            {
            apps[int(pid[0])- 48].time.seconds=1;
            apps[int(pid[0])- 48].time.minutes=0;
            apps[int(pid[0])- 48].time.hours=0;
             apps[int(pid[0])- 48].isEmpty=false;
            
           }
           else{
            
            apps[int(pid[0])- 48].time.seconds=ti->seconds;
            apps[int(pid[0])- 48].time.minutes=ti->minutes;
            apps[int(pid[0])- 48].time.hours=ti->hours;
             apps[int(pid[0])- 48].isEmpty=false;
             
           }
           // *lastApp = apps[pid[0]];
            std::cout<<"it is at "<<pid[0] <<" index running for seconds: "<<apps[int(pid[0])- 48].time.seconds<<std::endl;
        }
        else
        {
            
            
            int j=0, digits=1;
            bool found=false, empty=false;
            //for(int i=0; i<pid_len;i++)
            //{
                int i=0;
                j= j*10+ (pid[i]-48); //j= pid in int 
               
                while(j<=sizeApps && i<pid_len) //&&(empty!=true && found!=true)
                {

                     std::cout<<j<< " "<<sizeApps<<" "<<pid_len<<std::endl;
                    //std::cout<<"running";
                    if(apps[j].pid==stoi(pid))
                    {
                        //std::cout<<std::endl<<apps[j].pid<<std::endl;
                        if(apps[j].name==newApp.name && apps[j].dir==newApp.dir)
                        {
                            apps[j].time.seconds++;
                            if(apps[j].time.seconds>=60)
                            {
                                apps[j].time.minutes++;
                                apps[j].time.seconds=0;
                                if(apps[j].time.minutes>=60)
                                {
                                    apps[j].time.hours++;
                                }
                                
                            }
                            std::cout<<"it is at "<<j <<" index running for seconds: "<<apps[j].time.seconds<<std::endl;
                            apps[j].isEmpty=false;
                            break;
                            //*lastApp=apps[j];
                        }
                        else // if the pid matches but the name and dir are different, that means the previous app with the pid was closed
                        {   // need to save that app object with the key value being a hash of window name + directory of exe.
                            
                            std::string hash;
                            if(apps[j].hash == "") //generate hash
                            {
                               // genHash(apps[j].name, apps[j].dir);
                                std::hash<std::string> appHash;
                               std::string key= apps[j].name + apps[j].dir;

                              // SHA256 genhash;
                               //genhash.update(key);
                               //std::array<uint8_t, 32> digest = genhash.digest();
                                std::stringstream ss;
                                 ss<<appHash(key);
                                 hash=ss.str();
                                apps[j].hash= ss.str();
                                
                                //hash= genhash(key);
                            }
                            unsigned int t; 
                            if(currDayAppData.contains(hash))
                            { 
                                std::cout<<currDayAppData[hash]<<std::endl;
                                if(currDayAppData[hash]["pid"] !=apps[j].pid) //means the app has restarted since it was saved on the json
                                {
                                    t=currDayAppData[hash]["time_used"]["seconds"];
                                    t= t+ apps[j].time.seconds;
                                    currDayAppData[hash]["time_used"]["seconds"] = t;
                                    if( currDayAppData[hash]["time_used"]["seconds"]>=60)
                                    {
                                        t=t-60;

                                        currDayAppData[hash]["time_used"]["seconds"] = t;
                                        t = currDayAppData[hash]["time_used"]["minutes"];
                                        t=t+1;
                                        currDayAppData[hash]["time_used"]["minutes"]=t;
                                    }
                                    if(currDayAppData[hash]["time_used"]["minutes"] >=60)
                                    {
                                        currDayAppData[hash]["time_used"]["minutes"]=0;

                                        t = currDayAppData[hash]["time_used"]["hours"];
                                        t=t+1;
                                        currDayAppData[hash]["time_used"]["hours"]=t;
                                    }
                                    t = currDayAppData[hash]["time_used"]["minutes"];
                                    t = t + apps[j].time.minutes;
                                    
                                    currDayAppData[hash]["time_used"]["minutes"] = t;

                                    if(currDayAppData[hash]["time_used"]["minutes"]>=60)
                                    {
                                      t=  currDayAppData[hash]["time_used"]["hours"];
                                      t = t +1;
                                      currDayAppData[hash]["time_used"]["hours"] = t;
                                       t = currDayAppData[hash]["time_used"]["minutes"];
                                       t = currDayAppData[hash]["time_used"]["minutes"]; t=t-60;
                                        currDayAppData[hash]["time_used"]["minutes"] = t;
                                    }
                                }
                                else{
                                    currDayAppData[hash]["time_used"]["seconds"]= apps[j].time.seconds;
                                    currDayAppData[hash]["time_used"]["minutes"]= apps[j].time.minutes;
                                    currDayAppData[hash]["time_used"]["hours"]= apps[j].time.hours;
                                }
                            }
                            else
                            {
                                currDayAppData[hash]={ {"name", apps[j].name}, {"directory", apps[j].dir}, {"pid",apps[j].pid},
                                {"time_used",{
                                    {"seconds",apps[j].time.seconds},
                                    {"minutes",apps[j].time.minutes},
                                    {"hours",apps[j].time.hours}
                                    }}};
                            }
                            
                            // currDayAppData[apps[j].hash]=apps[j];
                           // currDayAppData[apps[j].hash]= {{"name",apps[j].name},{"directory",apps[j].dir},{"pid",apps[j].pid},{"time",{{"seconds",apps[j].time.seconds},{"minutes",apps[j].time.minutes},{"hours",apps[j].time.hours}}}};
                            apps[j]=newApp;
                            apps[j].time.seconds++;
                            apps[j].isEmpty=false;
                            apps[j].hash= "";
                           // *lastApp=apps[j];
                        }
                    }
                    else if(apps[j].isEmpty==false)
                    {
                        if(i+1>pid_len) // This checks if the length of the recieved app is reached. if at the end and still not placed,
                        {               // this block  swaps the app in current j index, which has larger pid with the new one, then j and i is incremented and placed at higher index, this way the apps can still be acessed quickly.
                            Application temp;
                            temp =apps[j];
                            apps[j]=newApp;
                            apps[j].isEmpty=false;
                            apps[j].hash="";
                            if(ti==NULL)
                            {
                            apps[j].time.seconds++; // UPDATE THE TIME OF NEW APP
                            if(apps[j].time.seconds>=60)
                            {
                                apps[j].time.minutes++;
                                apps[j].time.seconds=0;
                                if(apps[j].time.minutes>=60)
                                {
                                    apps[j].time.hours++;
                                }
                                
                            }
                            }
                            else
                            {
                                 apps[j].time.seconds= ti->seconds;
                                apps[j].time.minutes= ti->minutes;
                                apps[j].time.hours= ti->hours;
                            }
                            //CHANGE the newApp to store the app with bigger Pid. used in next interation when j is increased
                            newApp=temp;
                            pid= temp.pid;
                            name=temp.name;
                            dir=temp.dir;
                            pid_len= pid.length();
                            apps[j].isEmpty=false;
                            ti=NULL;
                        }
                        pid_len= pid.length();
                        i++;
                        if(j*10 + (pid[i]-48) >sizeApps)
                        {
                            sizeApps=j*10 + (pid[i]-48);
                            std::cout<<" need to increase array size"<<std::endl;
                            ResizeAppVector(j*10 + (pid[i]-48) +1);
                            j=j*10 + (pid[i]-48);
                            sizeApps= apps.size();
                           // std::cout<<std::endl<<j<<std::endl<<std::endl<<std::endl<<sizeApps<<std::endl<<std::endl<<std::endl;
                            continue;
                        }
                        else
                        {
                            j=j*10 + (pid[i]-48);
                        }
                         
                    }
                    else{
                        apps[j]=newApp;
                        if(ti==NULL)
                        {
                        apps[j].time.seconds++;
                        }
                        else{
                            apps[j].time.seconds = ti->seconds;
                            apps[j].time.minutes = ti->minutes;
                            apps[j].time.hours = ti->hours;
                        }
                        apps[j].isEmpty=false;
                        std::cout<<"it is at "<<j <<" index running for seconds: "<<apps[j].time.seconds<<std::endl;
                        break;
                       // *lastApp=apps[j];
                    }

                   // j++;
            //    }
                // if(apps[pid[0]].isEmpty)
                //  {

                //  }
                }
               
            
        }

    }
    else
    { 
        lastApp->time.seconds++;
    }

}
void WriteToAppArray(bool t)
{
    std::cout<<"called Write TO APp"<<std::endl;
    if(currDayAppData.empty()== false)
    {

        for(auto it = currDayAppData.begin(); it != currDayAppData.end();++it)
        {
            
            std::string name,dir,pid;
            Time t;
            std::string hash=it.key();
            std::cout<<"HASH::"<<hash<<std::endl;
            name = currDayAppData[hash]["name"];
            
            dir=currDayAppData[hash]["directory"];
            pid =to_string( currDayAppData[hash]["pid"]);
            t.seconds=currDayAppData[hash]["time_used"]["seconds"];
            t.minutes=currDayAppData[hash]["time_used"]["minutes"];
            t.hours=currDayAppData[hash]["time_used"]["hours"];
            WriteToAppArray(NULL,name,dir,pid,&t);
            std::cout<<"\n "<<name<<dir<<pid<<"written ! \n";
        }
        
    }
   
}
void ResizeAppVector()
{
    apps.resize(10);
    std::cout<<"RESIZE APP VECTOR CALLED \n";
}
void ResizeAppVector(size_t s)
{
    apps.resize(s);
}

void WriteToFile()
{
     unsigned short hour =CURR_DATE_TIME.wHour;
	unsigned short minute =CURR_DATE_TIME.wMinute;
	unsigned short day= CURR_DATE_TIME.wDay;
	unsigned short month =CURR_DATE_TIME.wMonth;
	unsigned short year =CURR_DATE_TIME.wYear;
    
    std::string filenamestr = "./data/" + std::to_string(day)+'-'+ std::to_string(month) +"-"+std::to_string(year)+ ".json";
    const char* filename = filenamestr.c_str();

  
    std::ofstream fi(filename);
    fi<<to_string(currDayAppData)<<std::endl;

}

void ReadFromFile()
{
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
    
    struct stat buffer;  
   // std::ifstream f("example.json");
    if(!stat (filename, &buffer) == 0) // if the file doesn't exist or is empty.  I need to add ERROR HANDLING in this section to check for other cases.
    {
       data = fopen(filename, "a");
       fprintf(data,"%s","{}"); 
       //std::ifstream f(filename);
        fclose(data);
    }
	data = fopen(filename, "r");
   // std::ifstream f(filename);
    
    WriteToJSONobj(data);
//std::cout<<currDayAppData["pi"]<<std::endl;

//auto obj = currDayAppData.find("ir1223@7237&*rfs");
//std::cout<<currDayAppData["ir1223@7237&*rfs"];

//currDayAppData[0]["pi"]=3.14;
fclose(data);
CURR_DATE_TIME=DATE_TIME;
// data=fopen(filename,"w");
// if (pFile!=NULL)
//   {
//     fputs ("fopen example",pFile);
//     fclose (pFile);
//   }
 //std::ofstream fi(filename);
 //fi<<currDayAppData<<std::endl;
}

void WriteToJSONobj(FILE* file)
{
    currDayAppData = nlohmann::json::parse(file);
    WriteToAppArray(true);
}

void WriteToJSONobj()
{
    for(size_t i=0; i<apps.size();i++)
    {
        if(apps[i].isEmpty==false)
        {
            std::cout<<"CLOSING!";
            if(apps[i].hash=="")
            {
                std::hash<std::string> appHash; 
                std::string key= apps[i].name + apps[i].dir;                             
                // std::size_t usingedintstring = std::hash<std::string>{}(key);
                std::stringstream ss;
                ss<<appHash(key);
                apps[i].hash= ss.str();
               
            }
            if(!currDayAppData.contains(apps[i].hash))
            {
            currDayAppData[apps[i].hash]["pid"]= apps[i].pid;
            currDayAppData[apps[i].hash]["name"]= apps[i].name;
            currDayAppData[apps[i].hash]["directory"]= apps[i].dir;
            currDayAppData[apps[i].hash]["time_used"]["seconds"]= apps[i].time.seconds;
            currDayAppData[apps[i].hash]["time_used"]["minutes"]= apps[i].time.minutes;
            currDayAppData[apps[i].hash]["time_used"]["hours"]= apps[i].time.hours;
            // std::cout<<apps[i].hash<<std::endl<< apps[i].pid<<std::endl;
            }
            else
            {
                currDayAppData[apps[i].hash]["pid"]= apps[i].pid;
                currDayAppData[apps[i].hash]["name"]= apps[i].name;
                currDayAppData[apps[i].hash]["directory"]= apps[i].dir;
                Time t;
                
                t.seconds = currDayAppData[apps[i].hash]["time_used"]["seconds"];
                t.seconds += apps[i].time.seconds;
                t.minutes = currDayAppData[apps[i].hash]["time_used"]["minutes"];
                t.minutes += apps[i].time.minutes;
                t.hours = currDayAppData[apps[i].hash]["time_used"]["hours"];
                t.hours += apps[i].time.hours;
                if(t.seconds>=60)
                {
                    t.minutes++;
                    t.seconds=t.seconds-60;
                }
                if(t.minutes>=60)
                {
                    t.hours++;
                    t.minutes=t.minutes-60;
                }
                currDayAppData[apps[i].hash]["time_used"]["seconds"]= t.seconds;
                currDayAppData[apps[i].hash]["time_used"]["minutes"]= t.minutes;
                currDayAppData[apps[i].hash]["time_used"]["hours"]= t.hours;
            }

        }
    }

    std::cout<<"\n Write TO OBJECT completed!";

}

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
     
ReadFromFile();

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
    wWinMain(window,NULL,GetCommandLineW(),SW_HIDE);
 
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
    //HMENU hMenu = GetSystemMenu( hwnd, FALSE );
    //EnableMenuItem( hMenu, SC_CLOSE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );
   // EnableMenuItem( hMenu, SC_CLOSE, MF_BYPOSITION);

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

   
    auto a1 = std::async(std::launch::async,&X::PipeRead, &x);
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
    
	
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
   
  
        TranslateMessage(&msg);
      // std::cout<<msg.message<<std::endl;
        if(msg.message==0x8111)
        {
            //POINT mousePos ={};
            //GetCursorPos(&mousePos);
            //msg.lParam=(LPARAM)&mousePos;
           //std::cout<<mousePos.x<<std::endl;
        }
        DispatchMessage(&msg);
    }
        
           
        


    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   // std::cout<<uMsg<<std::endl;
    // if(uMsg==0x8111)
    // {
    //     std::cout<<uMsg<<std::endl;
     
    // }
    
    if(uMsg==0x0024)
    {
       // MINMAXINFO* min={};
        //min=(MINMAXINFO*)LOWORD((DWORD)lParam);
        //POINT cord= min->ptMaxPosition;
   // std::cout<<uMsg<<std::endl<<cord.y; //lParam->ptMaxPosition->x;
    }
   switch (uMsg)
    {
         case WM_SYSCOMMAND:
        {if (wParam == SC_CLOSE)
        {   
            std::cout<<"SC_CLOSE \n";
             ShowWindow(hwnd, SW_HIDE);

        }
        return 0;}
        case WM_COMMAND:
        {
            //std::cout<<LOWORD(wParam)<<std::endl;
            if(LOWORD(wParam)==0)
            {
                ShowWindow(hwnd, SW_SHOW);
            }
            else if(LOWORD(wParam)==1)
            {
                UINT exitcode=258;
                TerminateProcess(watcher_info.hProcess,exitcode);
                SendMessage(hwnd,WM_DESTROY,NULL,NULL);
                  ExitProcess(1);
            }
            else if(LOWORD(wParam)==2)
            {
                 if(IsWatcherRunning())
                {
                    UINT exitcode=258;
                    TerminateProcess(watcher_info.hProcess,exitcode);
                }
                else
                {
                    CreateChildProcess();
                }
            }
            return 0;
        }
      case DOTASK:
    {
        
        int xPos = GET_X_LPARAM(lParam); 
            int yPos = GET_Y_LPARAM(lParam);
            if(lParam==WM_RBUTTONDOWN)
            {
                int xPos = GET_X_LPARAM(lParam); 
                int yPos = GET_Y_LPARAM(lParam);
                POINT mousePos = {};
                GetCursorPos(&mousePos);
                //std::cout<<xPos<<" "<<yPos<<std::endl;
                //ShowWindow(hwnd, SW_SHOW);
                HMENU barmenu= CreatePopupMenu();
                MENUITEMINFO me1= {};
                me1.cbSize=sizeof(MENUITEMINFO);
                me1.fMask= MIIM_TYPE |MIIM_ID |MIIM_STATE;
                me1.fType= MFT_STRING;
                me1.fState=MFS_ENABLED;
                me1.wID=0;
                me1.hSubMenu = NULL;
                me1.dwTypeData= (LPSTR)"Open";
                me1.cch=5;
                MENUITEMINFO me2= {};
                me2.cbSize=sizeof(MENUITEMINFO);
                me2.fMask= MIIM_TYPE |MIIM_ID |MIIM_STATE;
                me2.fType= MFT_STRING;
                me2.fState=MFS_ENABLED;
                me2.wID=1;
                me2.hSubMenu = NULL;
                me2.dwTypeData= (LPSTR)"Exit";
                me2.cch=4;
                 MENUITEMINFO me3= {};
                me3.cbSize=sizeof(MENUITEMINFO);
                me3.fMask= MIIM_TYPE |MIIM_ID |MIIM_STATE;
                me3.fType= MFT_STRING;
                me3.fState=MFS_ENABLED;
                me3.wID=2;
                me3.hSubMenu = NULL;
                
                if(IsWatcherRunning())
                {
                    me3.dwTypeData= (LPSTR)"Stop Window Watcher";
                }
                else
                {
                    me3.dwTypeData= (LPSTR)"Start Window Watcher";
                }
                me3.cch=lstrlenA(me3.dwTypeData);
                InsertMenuItemA(barmenu,1,true,&me1);
                InsertMenuItemA(barmenu,2,true,&me2);
                InsertMenuItemA(barmenu,0,true,&me3);
        TrackPopupMenu(barmenu,TPM_LEFTALIGN,mousePos.x,mousePos.y,0,hwnd,NULL);
            }
        //std::cout<<LOWORD(lParam);
       // LPPOINT mousePos={};
        // POINT mousePos = {};
        //  GetCursorPos(&mousePos);
        //std::cout<<mousePos.x<<std::endl;
           //HMENU barmenu= CreatePopupMenu();

        //TrackPopupMenu(barmenu,TPM_LEFTALIGN,mousePos.x,mousePos.y,0,hwnd,NULL);
        //DestroyMenu(barmenu);
        //std::cout<<GetLastError();
        //HandlePopupMenu(hwnd,mousePos);
        //mousePos=(LPPOINT)lParam;
        //OutputDebugStringW((LPCWSTR)mousePos->x);
        //std::cout<<mousePos->x;    
     
        }
        return 0;
    case WM_QUIT:
        std::cout<<"WM_QUIT called .\n";
        return 0;
    case WM_DESTROY:
        std::cout<<"WM_DESTROY called .\n";
        WriteToJSONobj();
        WriteToFile();
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
    LPCSTR windwatcher = "./pw-window-watcher2.exe";
   iswatcherRunning = CreateProcess(windwatcher, // the exe to start
      szCmdline,     // command line 
      NULL,          // process security attributes 
      NULL,          // primary thread security attributes 
      TRUE,          // handles are inherited 
      0,             // creation flags 
      NULL,          // use parent's environment 
      NULL,          // use parent's current directory 
      &siStartInfo,  // STARTUPINFO pointer 
      &watcher_info);  // receives PROCESS_INFORMATION 
   
   // If an error occurs, exit the application. 
   if ( !iswatcherRunning ) 
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

bool IsWatcherRunning()
{
    DWORD exitcode;
    GetExitCodeProcess(watcher_info.hProcess,&exitcode);
    std::cout<<STILL_ACTIVE<<std::endl;
    if(exitcode==STILL_ACTIVE)
    {
        return true;
    }
    return false;
}