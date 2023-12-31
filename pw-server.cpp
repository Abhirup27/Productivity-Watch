#undef UNICODE
#define DEFAULT_PORT "27015"
#define DEFAULT_BUFLEN 8192
#pragma comment (lib, "Ws2_32.lib")
#include<iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <nlohmann/json.hpp>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

//#define WINVER 0x0A00
//#define _WIN32_WINNT 0x0A00
#include<Windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
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
 
#include<filesystem>
std::mutex m; // FOR ASYNC with pipe function
std::mutex n; // FOR WEB SOCKET SERVER WITH GUI
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
 
 bool init();
void CreateChildProcess(void); 
//void WriteToPipe(void); 
//void ReadFromPipe(void); 
void ErrorExit(PTSTR); 

struct Time{

    unsigned short seconds=0;
    unsigned short minutes=0;
    unsigned short hours=0;
};

PROCESS_INFORMATION watcher_info ;
PROCESS_INFORMATION gui_info ;
bool IsWatcherRunning();
bool iswatcherRunning=false;
void WriteToAppArray(bool);
void WriteToAppArray(char [],std::string,std::string,std::string,std::string,Time*); // the named pipe writes to the struct array when pw-window-watcher sends data.
void WriteToJSONobj(); //Called when the server wants to write to file or when the pw-gui requests the JSON obj for current day data
void WriteToJSONobj(FILE*); // when ReadFromFile is called on startup. this func then calls WriteToAppArray()
void WriteToFile(); // Called when the system clock is 00:00 or the pw-server is closed.
void ReadFromFile();

bool FindProcess( DWORD ,std::string);
bool closeProcessbyPID(DWORD, DWORD, LPHANDLE);

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


struct Web_Socket
{
   std::vector<std::string> GetfileList()
{
    std::vector<std::string> list;
    std::string path("./data/");
    std::string ext(".json");
    for (auto &p : std::filesystem::recursive_directory_iterator(path))
    {
        //std::cout<<p.path().stem().string()<<std::endl;
        if (p.path().extension() == ext)
            list.push_back( p.path().stem().string());
    }
    return list;
}

FILE* open_file(char* filename)
{
    FILE *fp = fopen(filename, "r");
    if (fp != NULL) {
        return fp;
    }
    else{
        //ERROR
    }

    return fp;
}
long get_json_size(FILE *fp)
{
    long bufsize;
    if (fp != NULL) {
    /* Go to the end of the file. */
    if (fseek(fp, 0L, SEEK_END) == 0) {
        /* Get the size of the file. */
        bufsize = ftell(fp);
        if (bufsize == -1) { /* Error */ }
        

    }
    fclose(fp);
    bufsize=bufsize+1;
    return bufsize;
}


}
char* ReadAFile(char* fileName)
{
        
char *source = NULL;
FILE *fp = fopen(fileName, "r");
if (fp != NULL) {
    /* Go to the end of the file. */
    if (fseek(fp, 0L, SEEK_END) == 0) {
        /* Get the size of the file. */
        long bufsize = ftell(fp);
        if (bufsize == -1) { /* Error */ }

        /* Allocate our buffer to that size. */
        source =(char*) malloc(sizeof(char) * (bufsize + 1));

        /* Go back to the start of the file. */
        if (fseek(fp, 0L, SEEK_SET) != 0) { /* Error */ }

        /* Read the entire file into memory. */
        size_t newLen = fread(source, sizeof(char), bufsize, fp);
        if ( ferror( fp ) != 0 ) {
            fputs("Error reading file", stderr);
        } else {
            source[newLen++] = '\0'; /* Just to be safe. */
        }
    }
    fclose(fp);

    return source;
}

free(source); /* Don't forget to call free() later! */

}

int __cdecl init_server(void) 
{
     std::lock_guard<std::mutex> lk(n);
    WSADATA wsaData;
    int iResult;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo *result = NULL;
    struct addrinfo hints;

    int iSendResult;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;
    
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for the server to listen for client connections.
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    iResult = bind( ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // Accept a client socket
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        printf("accept failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // No longer need server socket
    closesocket(ListenSocket);
    //#define strcpy_instead_use_StringCbCopyA_or_StringCchCopyA
    // Receive until the peer shuts down the connection
    do 
    {
		//char* buf;

        iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0) 
        
        {
            printf("Bytes received: %d\n", iResult);
            
         if(strstr(recvbuf,".json"))
        {
            struct stat buffer; 
            // const char* dir = "./data/";
            // char* filename= (char*) malloc(strlen(dir)+strlen(recvbuf)+1);
            // const char* f=recvbuf;
            //  strcpy_s(filename,sizeof(filename), dir);
            //  strcpy_s(filename+strlen(dir),sizeof(filename),f);

             std::string filename=std::string("./data/") +std::string( recvbuf);

            std::cout<<"Json Requested \n"<<filename<<std::endl<<strlen(recvbuf);
            if(!stat (filename.c_str(), &buffer) == 0) // if the file doesn't exist or is empty. 
            {
                std::cout<<"File doesn't exist or is empty... not delivered. \n";

            }
            else
            {
                
                FILE* fp= open_file(filename.data());
                long size= get_json_size(fp);
                char* buf = (char*) malloc(size);
                iSendResult = send(ClientSocket, reinterpret_cast<char*>(&size), sizeof(size), 0);
                do{
                    iSendResult = recv(ClientSocket, buf, size, 0); //tells that that pw-gui is ready
                    if(buf[0]=='1')
                    {
                        buf= ReadAFile(filename.data());
                        iSendResult = send(ClientSocket, buf, size, 0);
                        iSendResult=-1;
                    }

                }while(iSendResult!=-1);
            }
        }
			else if(recvbuf[0]=='1')     // 1 means the pw-gui is requesting list of json from server
			{
				std::vector<std::string>list = GetfileList();
				std::cout<<list.size()<<std::endl;
                int totalFiles = list.size();
                iSendResult = send(ClientSocket, reinterpret_cast<char*>(&totalFiles), sizeof(totalFiles), 0);
                
            for (int i = 0; i < list.size(); i++)
            {
                //char* buf = new char[list[i].length() + 1];
               std::string str=list[i];
              // buf=str.c_str();
               // strcpy_s(buf,sizeof(buf), list[i].c_str());
                //std::cout<<buf<<std::endl;
            // std::cout<<std::endl<<buf<<std::endl;
                iSendResult = send(ClientSocket, str.c_str(), str.size() + 1, 0);
                //std::cout<<iSendResult<<std::endl;
               // delete[] buf;

                if (iSendResult == SOCKET_ERROR) {
                    // Handle send error
                    printf("send failed with error: %d\n", WSAGetLastError());
                                    closesocket(ClientSocket);
                                    WSACleanup();
                                    return 1;
                }
            }
  }
  
        // Echo the buffer back to the sender
        
        }

    } while (iResult>=0);
    init_server();
    // shutdown the connection since we're done
    // iResult = shutdown(ClientSocket, SD_SEND);
    // if (iResult == SOCKET_ERROR) {
    //     printf("shutdown failed with error: %d\n", WSAGetLastError());
    //     closesocket(ClientSocket);
    //     WSACleanup();
    //     return 1;
    // }

    // cleanup
   

    return 0;

}
  
};
Web_Socket server;


struct X
{

    HANDLE hPipe;
    char buffer2[MAX_PATH*32];
    DWORD dwRead;
void PipeRead()
{
      // Create the child process. The pw-window-watcher must be started by the concurrent thread, not in the main or winmain function (main thread)
   std::lock_guard<std::mutex> lk(m);
 
   // std::cout<<found;
    //CreateChildProcess();
     DWORD aProcesses[1024], cbNeeded, cProcesses;
   // unsigned int i;
     bool found=false;
//   if(found==false)
//   {
//     std::cout<<"PROCESS IS TO BE CREATED!!! \n \n \n \n";
//     //CreateChildProcess();
//       TCHAR szCmdline[]=TEXT("child");
//                 PROCESS_INFORMATION piProcInfo; 

//                  STARTUPINFO siStartInfo;
//                 BOOL bSuccess = FALSE; 
 
//                 // Set up members of the PROCESS_INFORMATION structure. 
                
//                 ZeroMemory( &watcher_info, sizeof(PROCESS_INFORMATION) );
                
//                 // Set up members of the STARTUPINFO structure. 
//                 // This structure specifies the STDIN and STDOUT handles for redirection.
                
//                 ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
//                 siStartInfo.cb = sizeof(STARTUPINFO); 
//                 siStartInfo.hStdError = g_hChildStd_OUT_Wr;
//                 siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
//                 siStartInfo.hStdInput = g_hChildStd_IN_Rd;
//                 siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
                
//                 // Create the child process. 
//                     LPCSTR gui = "./pw-window-watcher2.exe";
//                 CreateProcess(gui, // the exe to start
//                     szCmdline,     // command line 
//                     NULL,          // process security attributes 
//                     NULL,          // primary thread security attributes 
//                     TRUE,          // handles are inherited 
//                     0,             // creation flags 
//                     NULL,          // use parent's environment 
//                     NULL,          // use parent's current directory 
//                     &siStartInfo,  // STARTUPINFO pointer 
//                     &watcher_info);  // receives PROCESS_INFORMATION
//   } 
  
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
                            std::cout<<"RUNNING"<<std::endl<<iswatcherRunning<<std::endl;
                            if(iswatcherRunning==false)
                            {
                                //std::cout<<"INSIDE IF"<<std::endl<<iswatcherRunning<<std::endl;
                                if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
                                    {
                                        std::cout<<"INSIDE 2nd IF"<<std::endl<<iswatcherRunning<<std::endl;
                                        return ;
                                    }


                // Calculate how many process identifiers were returned.

                cProcesses = cbNeeded / sizeof(DWORD);

                //std::cout<<"CProcesses"<<std::endl<<cProcesses<<std::endl;

                found=false;
                for (unsigned int i = 0; i < cProcesses; i++ )
                {
                    if( aProcesses[i] != 0 )
                    {
                    found= FindProcess( aProcesses[i] ,"pw-window-watcher2.exe"); //don't forget .exe
                    }
                    if(found==true)
                    {iswatcherRunning=true;
                        break;}
                }
                            }
                while (ReadFile(hPipe, buffer2, sizeof(buffer2) - 1, &dwRead, NULL) != FALSE)
                {
                    /* add terminating zero */
                    buffer2[dwRead] = '\0';

                    /* do something with data in buffer */
                    printf("%s \n", buffer2);
                   WriteToAppArray(buffer2, "","","","",NULL);
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
   



void WriteToAppArray(char Buffer[] =NULL,std::string name="",std::string dir="",std::string pid="",std::string hash="",Time* ti=NULL)
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
           //std::cout<<Buffer[index];
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
    //std::cout<<name <<dir<<std::endl;
     while(Buffer[index]!='\0')
    {
      //   std::cout<<"execute";
        //num= num*digits + (int)Buffer[index];
        //digits*=10;
        pid.push_back(Buffer[index]);
        index++;
    }
    }//std::cout<<name<<dir<<pid<<std::endl;
   
   size_t sizeApps= apps.size();
    size_t pid_len= pid.length();
    //std::cout<<"running";
    if(lastApp==NULL) //window title = name
    {
       
        //then find the app using a hash in the array, if not found, create new
        Application newApp;
        newApp.name=name; newApp.dir=dir; newApp.pid=stoi(pid); 
        if(hash !="")
        {
            newApp.hash=hash;
        }
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
           if(hash!="")
           {
            apps[int(pid[0])- 48].hash=hash;
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
                            
                            std::string n_hash;
                            //n_hash =hash;
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
                                 n_hash=ss.str();
                                apps[j].hash= ss.str();
                                
                                //hash= genhash(key);
                            }
                            else
                            {
                                n_hash=apps[j].hash;
                            }
                            unsigned int t; 
                            if(currDayAppData.contains(n_hash))
                            { 
                                std::cout<<name<<dir<<pid<<std::endl;//ti->seconds<<std::endl;
                                std::cout<<currDayAppData[n_hash]<<std::endl;
                                if(currDayAppData[n_hash]["pid"] !=apps[j].pid) //means the app has restarted since it was saved on the json
                                {
                                    t=currDayAppData[n_hash]["time_used"]["seconds"];
                                    t= t+ apps[j].time.seconds;
                                    currDayAppData[n_hash]["time_used"]["seconds"] = t;
                                    if( currDayAppData[n_hash]["time_used"]["seconds"]>=60)
                                    {
                                        t=t-60;

                                        currDayAppData[n_hash]["time_used"]["seconds"] = t;
                                        t = currDayAppData[n_hash]["time_used"]["minutes"];
                                        t=t+1;
                                        currDayAppData[n_hash]["time_used"]["minutes"]=t;
                                    }
                                    if(currDayAppData[n_hash]["time_used"]["minutes"] >=60)
                                    {
                                        currDayAppData[n_hash]["time_used"]["minutes"]=0;

                                        t = currDayAppData[n_hash]["time_used"]["hours"];
                                        t=t+1;
                                        currDayAppData[n_hash]["time_used"]["hours"]=t;
                                    }
                                    t = currDayAppData[n_hash]["time_used"]["minutes"];
                                    t = t + apps[j].time.minutes;
                                    
                                    currDayAppData[n_hash]["time_used"]["minutes"] = t;

                                    if(currDayAppData[n_hash]["time_used"]["minutes"]>=60)
                                    {
                                      t=  currDayAppData[n_hash]["time_used"]["hours"];
                                      t = t +1;
                                      currDayAppData[n_hash]["time_used"]["hours"] = t;
                                       t = currDayAppData[n_hash]["time_used"]["minutes"];
                                       t = currDayAppData[n_hash]["time_used"]["minutes"]; t=t-60;
                                        currDayAppData[n_hash]["time_used"]["minutes"] = t;
                                    }
                                }
                                else{
                                    std::cout<<std::endl<<"THIS RAN"<<std::endl;
                                    currDayAppData[n_hash]["time_used"]["seconds"]= apps[j].time.seconds;
                                    currDayAppData[n_hash]["time_used"]["minutes"]= apps[j].time.minutes;
                                    currDayAppData[n_hash]["time_used"]["hours"]= apps[j].time.hours;
                                    std::cout<<std::endl<<"THIS RAN 2"<<std::endl;
                                }
                            }
                            else
                            {
                                std::cout<<std::endl<<"THIS RAN 2"<<std::endl<<n_hash;
                                currDayAppData[n_hash]={ {"name", apps[j].name}, {"directory", apps[j].dir}, {"pid",apps[j].pid},
                                {"time_used",{
                                    {"seconds",apps[j].time.seconds},
                                    {"minutes",apps[j].time.minutes},
                                    {"hours",apps[j].time.hours}
                                    }}};
                                std::cout<<std::endl<<currDayAppData[n_hash];
                            }
                            
                            // currDayAppData[apps[j].hash]=apps[j];
                           // currDayAppData[apps[j].hash]= {{"name",apps[j].name},{"directory",apps[j].dir},{"pid",apps[j].pid},{"time",{{"seconds",apps[j].time.seconds},{"minutes",apps[j].time.minutes},{"hours",apps[j].time.hours}}}};
                            std::cout<<std::endl<<"THIS RAN 3"<<std::endl;
                            apps[j]=newApp;
                            std::cout<<std::endl<<"THIS RAN 4"<<std::endl;

                            if(hash=="")
                            {
                                std::hash<std::string> appHash;
                               std::string key= apps[j].name + apps[j].dir;
                                std::stringstream ss;
                                 ss<<appHash(key);
                                 n_hash=ss.str();
                                apps[j].hash= ss.str();
                            }
                            if(currDayAppData.contains(apps[j].hash) && ti==NULL)
                            {
                                std::cout<<std::endl<<"THIS RAN 5.1"<<std::endl;
                                apps[j].time.seconds = currDayAppData[apps[j].hash]["time_used"]["seconds"]; // apps[j].time.seconds= ti->seconds;
                                std::cout<<std::endl<<"THIS RAN 5.2"<<std::endl;
                                apps[j].time.minutes = currDayAppData[apps[j].hash]["time_used"]["minutes"];
                                apps[j].time.hours = currDayAppData[apps[j].hash]["time_used"]["hours"];
                                std::cout<<std::endl<<"THIS RAN 5.3"<<std::endl;
                            }
                            else if(ti!=NULL)
                            {
                                std::cout<<std::endl<<"THIS RAN 5"<<std::endl;
                                apps[j].time.seconds= ti->seconds;
                                apps[j].time.minutes= ti->minutes;
                                apps[j].time.hours= ti->hours;
                            }
                            apps[j].time.seconds++;
                            apps[j].isEmpty=false;
                            std::cout<<std::endl<<"THIS RAN 6"<<std::endl;
                           // apps[j].hash= "";
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
                            apps[j].time.seconds=0; apps[j].time.minutes=0; apps[j].time.hours=0;
                           // apps[j].hash="";
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
            WriteToAppArray(NULL,name,dir,pid,hash,&t);
            std::cout<<"\n "<<name<<dir<<pid<<"written ! \n";
        }
        
    }
   
}
void ResizeAppVector()
{
    apps.resize(10);
    
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

char* ReadAFile(char* fileName)
{

char *source = NULL;
FILE *fp = fopen(fileName, "r");
if (fp != NULL) {
    /* Go to the end of the file. */
    if (fseek(fp, 0L, SEEK_END) == 0) {
        /* Get the size of the file. */
        long bufsize = ftell(fp);
        if (bufsize == -1) { /* Error */ }

        /* Allocate our buffer to that size. */
        source = (char*)malloc(sizeof(char) * (bufsize + 1));

        /* Go back to the start of the file. */
        if (fseek(fp, 0L, SEEK_SET) != 0) { /* Error */ }

        /* Read the entire file into memory. */
        size_t newLen = fread(source, sizeof(char), bufsize, fp);
        if ( ferror( fp ) != 0 ) {
            fputs("Error reading file", stderr);
        } else {
            source[newLen++] = '\0'; /* Just to be safe. */
        }
    }
    fclose(fp);
}

free(source); /* Don't forget to call free() later! */

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
std::cout<<"read from file \n";
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
     
// ReadFromFile();

//     auto a1 = std::async(std::launch::async,&X::PipeRead, &x);

//    int iResult;

//    struct addrinfo *result = NULL, *ptr = NULL, hints;

// ZeroMemory(&hints, sizeof (hints));
// hints.ai_family = AF_INET;
// hints.ai_socktype = SOCK_STREAM;
// hints.ai_protocol = IPPROTO_TCP;
// hints.ai_flags = AI_PASSIVE;
// WSADATA wsaData;
// // Initialize Winsock
// iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
// if (iResult != 0) {
//     printf("WSAStartup failed: %d\n", iResult);
//     return 1;
// }
// iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
// if (iResult != 0) {
//     printf("getaddrinfo failed: %d\n", iResult);
//     WSACleanup();
//     return 1;
// }
// SOCKET ListenSocket = INVALID_SOCKET;
// ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
// if (ListenSocket == INVALID_SOCKET) {
//     printf("Error at socket(): %ld\n", WSAGetLastError());
//     freeaddrinfo(result);
//     WSACleanup();
//     return 1;
// }

 
   
//     HINSTANCE window;
//     //LPSTR cmdline= GetCommandLine();
//     wWinMain(window,NULL,GetCommandLineW(),SW_HIDE);


    init();
   
    
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
    LPCSTR CLASS_NAME = "Window Class";
    LPCSTR WINDOW_NAME = "pw server";
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

                //ShowWindow(hwnd, SW_SHOW);
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
                    LPCSTR gui = "./pw-gui.exe";
                CreateProcess(gui, // the exe to start
                    szCmdline,     // command line 
                    NULL,          // process security attributes 
                    NULL,          // primary thread security attributes 
                    TRUE,          // handles are inherited 
                    0,             // creation flags 
                    NULL,          // use parent's environment 
                    NULL,          // use parent's current directory 
                    &siStartInfo,  // STARTUPINFO pointer 
                    &gui_info);  // receives PROCESS_INFORMATION
            }
            else if(LOWORD(wParam)==1)
            {
                UINT exitcode=258;
               // TerminateProcess(watcher_info.hProcess,exitcode);
                 closeProcessbyPID(watcher_info.dwProcessId,exitcode,&watcher_info.hProcess);
                SendMessage(hwnd,WM_DESTROY,NULL,NULL);
                  ExitProcess(1);
            }
            else if(LOWORD(wParam)==2)
            {
                 if(IsWatcherRunning())
                {
                    UINT exitcode=258;
                    //TerminateProcess(watcher_info.hProcess,exitcode);
                    closeProcessbyPID(watcher_info.dwProcessId,exitcode,&watcher_info.hProcess);
                }
                else
                {
                    std::cout<<" THIS IS A REALLY BIG TEXT TO CHECK IF THIS LOOP RAN OR NOT! \n THIS IS A REALLY BIG TEXT TO CHECK IF THIS LOOP RAN OR NOT! \n" ;
                    CreateChildProcess();
                }
            }
            else if(LOWORD(wParam)==3)
            {

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
 
   ZeroMemory( &watcher_info, sizeof(PROCESS_INFORMATION) );
 
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
   bSuccess = CreateProcess(windwatcher, // the exe to start
      NULL,     // command line 
      NULL,          // process security attributes 
      NULL,          // primary thread security attributes 
      TRUE,          // handles are inherited 
      0,             // creation flags 
      NULL,          // use parent's environment 
      NULL,          // use parent's current directory 
      &siStartInfo,  // STARTUPINFO pointer 
      &watcher_info);  // receives PROCESS_INFORMATION 
      std::cout<<GetLastError()<<std::endl;
   std::cout<<"CREATED\n";
   // If an error occurs, exit the application. 
   if ( ! bSuccess ) 
      ErrorExit(TEXT("CreateProcess"));
   else 
   {
      // Close handles to the child process and its primary thread.
      // Some applications might keep these handles to monitor the status
      // of the child process, for example. 

     //CloseHandle(watcher_info.hProcess);
      //CloseHandle(watcher_info.hThread);
      
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
   // std::cout<<STILL_ACTIVE<<std::endl;
    // if(exitcode==STILL_ACTIVE)
    // {
    //     std::cout<<"STIL RUNNING \n \n";
    //     return true;
    // }
    // std::cout<<"NOT RUNNING \n \n";
    if(GetExitCodeProcess(watcher_info.hProcess,&exitcode)!=0)
    {
        std::cout<<"STIL RUNNING \n \n";
        return true;
    }
    iswatcherRunning=false;
    
    std::cout<<"NOT RUNNING \n \n";//<<iswatcherRunning<<std::endl;
    return false;
}

bool IsRunning(HANDLE hProcess)
{
    DWORD exitcode;
    GetExitCodeProcess(hProcess,&exitcode);
    std::cout<<STILL_ACTIVE<<std::endl;
    if(exitcode==STILL_ACTIVE)
    {
        return true;
    }
    return false;
}

bool FindProcess( DWORD processID , std::string needed)
{
    TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");

    // Get a handle to the process.

    HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
                                   PROCESS_VM_READ,
                                   FALSE, processID );

    // Get the process name.

    if (NULL != hProcess )
    {
        HMODULE hMod;
        DWORD cbNeeded;

        if ( EnumProcessModules( hProcess, &hMod, sizeof(hMod), 
             &cbNeeded) )
        {
            GetModuleBaseName( hProcess, hMod, szProcessName, 
                               sizeof(szProcessName)/sizeof(TCHAR) );
        }
    }
	std::string name=szProcessName;
    
	if(name==needed)
	{
        //CloseHandle( hProcess );
       // UINT exitcode=258;
               // TerminateProcess(hProcess,exitcode);
        watcher_info.hProcess=hProcess; // TO TERMINATE USING hProcess, OPENPROCCESS FUNCTION ABOVE NEEDS MORE PRIVILEDGES
        watcher_info.dwProcessId=processID;
       // CloseHandle( hProcess );
		return true;
	}
	else
	{
        CloseHandle( hProcess );
        return false;
	}
    // Print the process name and identifier.

    //_tprintf( TEXT("%s  (PID: %u)\n"), szProcessName, processID );
    return false;
}

bool closeProcessbyPID(DWORD p_id, DWORD exitcode, LPHANDLE o_hProcess=NULL)
{
    DWORD ecode;
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |PROCESS_TERMINATE, false, p_id);
    TerminateProcess(hProcess, exitcode);
    CloseHandle(*o_hProcess);
    //*o_hProcess=hProcess;
   iswatcherRunning=false;
    if(GetExitCodeProcess(o_hProcess,&ecode)!=0)
    {
         CloseHandle(hProcess);
        return false;
    }
     CloseHandle(hProcess);
    return true;
}


std::vector<std::string> GetfileList()
{
    std::vector<std::string> list;
    std::string path("./data/");
    std::string ext(".json");
    for (auto &p : std::filesystem::recursive_directory_iterator(path))
    {
        if (p.path().extension() == ext)
            list.push_back( p.path().stem().string());
    }
    return list;
}

   

bool init()
{
    ReadFromFile();

    auto a1 = std::async(std::launch::async,&X::PipeRead, &x);
    auto a2 = std::async(std::launch::async,&Web_Socket::init_server, &server);
    
    HINSTANCE window;
    //LPSTR cmdline= GetCommandLine();
    wWinMain(window,NULL,GetCommandLineW(),SW_HIDE);

    return true;

}