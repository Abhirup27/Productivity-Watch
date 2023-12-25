//#include"pw-gui.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#include <glfw3.h>
#include<string>
#define WIN32_LEAN_AND_MEAN
#include<iostream>
#include<vector>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <mutex>
#include<future> // for async
#include <nlohmann/json.hpp>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

std::mutex n; // FOR WEB SOCKET 
#define DEFAULT_BUFLEN 8096
#define DEFAULT_PORT "27015"

ImFont* AddDefaultFont(float);
void DoFitTextToWindow(ImFont*, const char *);
bool init_gui();

void fetch_item(const char* );
uint8_t bar_Y_axis_flag;  // 3= use hours, 2= use minutes 1= use seconds
uint16_t n_items_hours=0;
uint16_t n_items_minutes=0;
std::mutex m;
std::mutex l;

 nlohmann::json DayAppData;

std::vector<char**>apps;

 struct Day{

    const char** app_names;
    const char* date;
    const char** dir;

 };

struct X
{
    WSADATA wsaData;
    int iResult;
    SOCKET ConnectSocket = INVALID_SOCKET;
    char recvbuf[DEFAULT_BUFLEN];
void socket_init()
{
     
   std::lock_guard<std::mutex> lk(n);
   //  WSADATA wsaData;
   // SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;
    const char *sendbuf = "1";
    //char recvbuf[DEFAULT_BUFLEN];
    
    int recvbuflen = 16384;
    
    // Validate the parameters

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return ;
    }

    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo("localhost", DEFAULT_PORT, &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return ;
    }

    // Attempt to connect to an address until one succeeds
    for(ptr=result; ptr != NULL ;ptr=ptr->ai_next) {

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, 
            ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %d\n", WSAGetLastError());
            WSACleanup();
            return ;
        }

        // Connect to server.
        iResult = connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return ;
    }

    // Send an initial buffer
    iResult = send( ConnectSocket, sendbuf, (int)strlen(sendbuf), 0 );
    if (iResult == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return ;
    }

    printf("Bytes Sent: %ld\n", iResult);

    // int totalFiles;
    // iResult = recv(ConnectSocket, reinterpret_cast<char*>(&totalFiles), sizeof(totalFiles), 0);

    // if (iResult == SOCKET_ERROR || iResult == 0) {
    //     printf("recv failed with error: %d\n", WSAGetLastError());
    //     closesocket(ConnectSocket);
    //     WSACleanup();
    //     return ;
    // }

    // shutdown the connection since no more data will be sent
  //  iResult = shutdown(ConnectSocket, SD_SEND);
    //if (iResult == SOCKET_ERROR) {
     //   printf("shutdown failed with error: %d\n", WSAGetLastError());
       // closesocket(ConnectSocket);
       // WSACleanup();
       // return 1;
    //}

    // cleanup
    // closesocket(ConnectSocket);
    // WSACleanup();
}

void close_connection()
{

    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
       printf("shutdown failed with error: %d\n", WSAGetLastError());
       closesocket(ConnectSocket);
       WSACleanup();
       return ;
    }

    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();
}


char** get_file_list(int *size)
{
    std::lock_guard<std::mutex> lk(m);  
    int totalFiles;
    //std::vector<char*> list;

    iResult = recv(ConnectSocket, reinterpret_cast<char*>(&totalFiles), sizeof(totalFiles), 0);

    if (iResult == SOCKET_ERROR || iResult == 0) {
        printf("recv failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
       // return ;
    }
    *size= totalFiles;
    char** listArray = (char**) malloc(sizeof(char*)*(totalFiles+1));

     for (int i = 0; i < totalFiles + 1; ++i) {
        if(i==0)
        {
            listArray[i] = (char*) malloc(sizeof(char)* 12);
            strcpy(listArray[i],"Current Day\0");
        }
        else
        {
            iResult = recv(ConnectSocket, recvbuf, 11, 0);
            
            if (iResult > 0) {
                printf("File name received: %s\n", recvbuf);
                listArray[i] = (char*) malloc(sizeof(char)* 11);
                //listArray[i]=recvbuf;
                strcpy(listArray[i], recvbuf);

    //            list.push_back(recvbuf);
    
            } else if (iResult == 0) {
                printf("connection closed");
            } else {
                printf("recv failed with error: %d\n", WSAGetLastError());
                closesocket(ConnectSocket);
                WSACleanup();
            }
        }
    }
    return listArray;
}

char* fetch_day_data(char* file)
 {
    char* ext=".json";
    std::lock_guard<std::mutex> lk(l);  
    size_t len1 = strlen(file);
    size_t len2 = strlen(ext);
   
    char* result = (char*)malloc(len1 + len2 + 1);

    if (result == NULL) {
        // Memory allocation failed
     //   return NULL;
    }

    // Copy the first string to the result buffer
    strcpy(result, file);

    // Copy the second string to the result buffer, starting from the null terminator of the first string
    strcpy(result + len1, ext);
std::cout<<std::endl<<result<<std::endl;
int fileSize;
   int iSendResult=send(ConnectSocket,result,sizeof(char)*16,0);
   do{
iSendResult = recv(ConnectSocket, reinterpret_cast<char*>(&fileSize), sizeof(fileSize), 0);
if(iSendResult>0)
{
    break;
}
else if(iSendResult==0)
{
    std::cout<<"closed";
}
   }while(iSendResult>0);
    //std::vector<char*> list;
    
    std::cout<<"Connection closed 3\n"<<fileSize;
     char* buffer = (char*) malloc(sizeof(char)* fileSize);
    char* num="1";
    iResult = send( ConnectSocket, num, sizeof(char), 0 );
    std::cout<<"Connection closed \n";
    if (iResult == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return nullptr;
    }
    int receive=0;
    do{
        receive=recv(ConnectSocket, buffer, fileSize, 0);
        if(receive>0)
        {
            std::cout<<std::endl<<buffer<<std::endl;
            return buffer;
        }
        else if(receive==0)
        {
            std::cout<<"Connection closed \n";
        }
        else
            printf("recv failed with error: %d\n", WSAGetLastError());
    }while(receive>0);
    

 }
};
X x;

std::vector<char**> get_app_details()
{
    int j=0;
   std::vector<char**> details;
    size_t data_size= DayAppData.size();
    //std::cout<<data_size<<std::endl;
    bar_Y_axis_flag=1;
    n_items_hours=0;
    n_items_minutes=0;
   details.resize(5,nullptr);
    for(int i =0; i<5;i++)
    {
    details[i] = (char**) malloc(sizeof(char*)*data_size);
    }
    
    for(auto it =DayAppData.begin(); it != DayAppData.end();++it)
                    {
                        std::string hash=it.key();
                        if(DayAppData[hash]["name"]!="")
                        {
                            
                            if(std::string( DayAppData[hash]["name"]).length()<45)
                        {
                       details[0][j]= (char*) malloc(sizeof(char)* std::string( DayAppData[hash]["name"]).length() +1 );
                       strcpy(details[0][j],std::string(DayAppData[hash]["name"]).c_str());
                        }
                        else
                        {
                            
                            std::string filePath = DayAppData[hash]["directory"] ;

                            // Find the position of the last forward slash
                            size_t lastSlashPos = filePath.find_last_of('\\');

                            if (lastSlashPos >0) {
                                
                                std::cout<<"THIS IS RUNNING \n \n \n";
                                std::string executableName = filePath.substr(lastSlashPos + 1);
                                 details[0][j]= (char*) malloc(sizeof(char)* 30 + executableName.size() +1 );
                                 std::string substring = std::string( DayAppData[hash]["name"]).substr(0, 25);
                                 strcpy(details[0][j],substring.c_str());
                                 strcat(details[0][j],executableName.c_str());
                                std::cout<<std::endl<<std::endl<<std::endl<<details[0][j]<<std::endl<<std::endl<<std::endl<<std::endl<<std::endl;
                            }
                        }
                        }
                      else
                      {
                         details[0][j]= (char*) malloc(sizeof(char)* std::string( DayAppData[hash]["directory"]).length() +1 );
                        strcpy(details[0][j],std::string(DayAppData[hash]["directory"]).c_str());
                      }
                        details[1][j]= (char*) malloc(sizeof(char)*std::string( DayAppData[hash]["directory"]).length() +1);
                        details[2][j]= (char*) malloc(sizeof(char)*std::string( to_string(DayAppData[hash]["time_used"]["seconds"])).length() +1);
                        details[3][j]= (char*) malloc(sizeof(char)*std::string( to_string(DayAppData[hash]["time_used"]["minutes"])).length() +1);
                        details[4][j]= (char*) malloc(sizeof(char)*std::string( to_string(DayAppData[hash]["time_used"]["hours"])).length() +1);
                        
                        strcpy(details[1][j],std::string(DayAppData[hash]["directory"]).c_str());
                        strcpy(details[2][j],std::string(to_string(DayAppData[hash]["time_used"]["seconds"])).c_str());
                        strcpy(details[3][j],std::string(to_string(DayAppData[hash]["time_used"]["minutes"])).c_str());
                        strcpy(details[4][j],std::string(to_string(DayAppData[hash]["time_used"]["hours"])).c_str());

                        if(DayAppData[hash]["time_used"]["hours"]>1)
                        {
                            n_items_hours++;
                        }
                        else if(DayAppData[hash]["time_used"]["minutes"]>3)
                        {
                            n_items_minutes++;
                        }

                   // std::cout<<details[0][j]<<"/t"<<details[1][j]<<"/n";
                        // name = DayAppData[hash]["name"];
                        
                        // dir=DayAppData[hash]["directory"];
                        // pid =to_string( DayAppData[hash]["pid"]);
                        // t.seconds=cDayAppData[hash]["time_used"]["seconds"];
                        // t.minutes=DayAppData[hash]["time_used"]["minutes"];
                        // t.hours=DayAppData[hash]["time_used"]["hours"];

                        j++;
                       
                    }
                    if(n_items_hours>n_items_minutes)
                    {
                        bar_Y_axis_flag=3;
                    }
                    else if(n_items_minutes>1)
                    {
                        bar_Y_axis_flag=2;
                    }

                    return details;

}
void convert(const char** source, const int size, const char* target[]) {
    for (int i = 0; i < size; ++i) {
        target[i] = source[i];
    }
}
const char** get_X_axis_names(std::vector<char**>& apps)
{
    std::cout<<"this finished \n";
    int data_size= DayAppData.size();
   const char** names= new const char*[data_size+1];
    for(int i=0; i<data_size;i++)
    {
        std::cout<<"this finished \n";
       // names[i]= new char[std::string(apps[0][i]).length()+1];
        names[i]= strdup(apps[0][i]);
    }
    std::cout<<"this finished \n";
    names[data_size]=nullptr;
    return names;
}
   
float* load_data(std::vector<char**>apps)

{
      float* data;
      float time;
     // std::vector<char*>
    int data_size= DayAppData.size();
    
    data= (float*) malloc(sizeof(float)*data_size);
   
    for(int i=0;i<data_size;i++)
    {
        //std::string name= apps[0][i];
       // app_names[i]= (char*)malloc(sizeof(char)*name.length()+1);   
        //strcpy(app_names[i],name.c_str());   
        //std::cout<<app_names[i];  
        if(bar_Y_axis_flag==3)
        {
            if(std::stoi(apps[4][i],nullptr,10)>1)
            {
                time= (float)std::stoi(apps[4][i],nullptr,10);
            }
            if(std::stoi(apps[3][i],nullptr,10)>1)
            {
                time+= ((float)std::stoi(apps[3][i],nullptr,10)/10.0);
            }
            if(std::stoi(apps[2][i],nullptr,10)>1)
            {
                time+= ((float)std::stoi(apps[2][i],nullptr,10)/100.0);
            }
        }
        else if(bar_Y_axis_flag==2)
        {
            if(std::stoi(apps[3][i],nullptr,10)>1)
            {
                time= (float)std::stoi(apps[3][i],nullptr,10);
            }
            if(std::stoi(apps[4][i],nullptr,10)>1)
            {
                time+= ((float)std::stoi(apps[4][i],nullptr,10)*60.0);
            }
            if(std::stoi(apps[2][i],nullptr,10)>1)
            {
                time+= ((float)std::stoi(apps[2][i],nullptr,10)/10.0);
            }
        }
        else if(bar_Y_axis_flag==1)
        {
            if(std::stoi(apps[2][i],nullptr,10)>1)
            {
                time= (float)std::stoi(apps[2][i],nullptr,10);
            }
            if(std::stoi(apps[4][i],nullptr,10)>1)
            {
                time+= ((float)std::stoi(apps[4][i],nullptr,10)*3600.0);
            }
            if(std::stoi(apps[3][i],nullptr,10)>1)
            {
                time+= ((float)std::stoi(apps[3][i],nullptr,10)*60.0);
            }
        }
       // std::string len=  std::to_string((roundf(time * 100) / 100));

            //data=(char*) malloc(sizeof(char)* len.length() +1);
          //  strcpy(data[i],len.c_str());
            data[i]= (roundf(time * 100) / 100);
          //  data++;
    }
    
   // data= data-(data_size-1);
 
    return data;
}



double s_xpos = 0, s_ypos = 0;
int w_xsiz = 0, w_ysiz = 0;
int dragState = 0;
void processInput(GLFWwindow *window)
    {

        if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {

            glfwSetWindowShouldClose(window,true);
        }
        if (glfwGetMouseButton(window, 0) == GLFW_PRESS && dragState == 0) {
		glfwGetCursorPos(window, &s_xpos, &s_ypos);
		glfwGetWindowSize(window, &w_xsiz, &w_ysiz);
		dragState = 1;
	}
	if (glfwGetMouseButton(window, 0) == GLFW_PRESS && dragState == 1) {
		double c_xpos, c_ypos;
		int w_xpos, w_ypos;
		glfwGetCursorPos(window, &c_xpos, &c_ypos);
		glfwGetWindowPos(window, &w_xpos, &w_ypos);
		if (
			s_xpos >= 0 && s_xpos <= ((double)w_xsiz - 170) &&
			s_ypos >= 0 && s_ypos <= 25) {
			glfwSetWindowPos(window, w_xpos + (c_xpos - s_xpos), w_ypos + (c_ypos - s_ypos));
		}
		if (
			s_xpos >= ((double)w_xsiz - 15) && s_xpos <= ((double)w_xsiz) &&
			s_ypos >= ((double)w_ysiz - 15) && s_ypos <= ((double)w_ysiz)) {
			glfwSetWindowSize(window, w_xsiz + (c_xpos - s_xpos), w_ysiz + (c_ypos - s_ypos));
		}
	}
	if (glfwGetMouseButton(window, 0) == GLFW_RELEASE && dragState == 1) {
		dragState = 0;
	}
    }


static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// Main code
int main(int, char**)
{
    int nooffiles;
    char* filen;

    // std::vector<char**> apps;
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;
    
    auto a1 = std::async(std::launch::deferred,&X::socket_init, &x);
    auto a2 = std::async(std::launch::deferred,&X::get_file_list, &x,&nooffiles);
    //auto a3 = std::async(std::launch::deferred,&X::fetch_day_data, &x,filen);
    a1.get();
    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Productivity Watch", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
	ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; 
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; 
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

 static ImGuiWindowFlags titlebar_flags = ImGuiWindowFlags_NoCollapse
	| ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
	 | ImGuiWindowFlags_NoScrollbar
	| ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollWithMouse;
    
const ImGuiViewport* viewport = ImGui::GetMainViewport();
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();


    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);


    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);

    // Our state
    bool show_window = true;
    bool disable_mouse_wheel=false;
    ImVec4 clear_color = ImVec4(0.10f, 0.10f, 0.25f, 1.00f);

//ImGuiIO& io = ImGui::GetIO();
ImFont* font1 = io.Fonts->AddFontFromFileTTF("./misc/fonts/Roboto-Medium.ttf", 24.0f);

ImFont* font2 =io.Fonts->AddFontFromFileTTF("./misc/fonts/Roboto-Medium.ttf", 16.0f);
io.Fonts->Build();
  
//ImFont *fontA = AddDefaultFont(13);
//ImFont *fontB = AddDefaultFont(64);
//ImFont *fontC = AddDefaultFont(256);
 
char** items= a2.get();
    // Main loop        
    std::cout<<items[0];
     
     
    // const char* x_axis_names[DayAppData.size()];
    const char** x_axis_names;
    char**  ilabels;
    static int groups;
    static double* positions;
    while (!glfwWindowShouldClose(window))
    {
       
            static float* data;
 //ImGui::PushFont(font1);
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
       processInput(window);
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

       // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
       if(!show_window)
       {
        ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
       }
       ImGui::SetNextWindowPos(viewport->WorkPos);
ImGui::SetNextWindowSize(ImVec2{ viewport->WorkSize.x, viewport->WorkSize.y });
ImGui::SetNextWindowViewport(viewport->ID);
ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2{ 0.0f, 0.0f });
ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 40.0f, 2.0f });
ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4{ 0.01f, 0.01f, 0.01f, 1.0f });
ImGui::PushFont(font1);
//float origScale = ImGui::GetFont()->Scale;
//ImGui::GetFont()->Scale =3.0f;


            ImGui::Begin("Productivity Watch", &show_window, titlebar_flags);
ImGui::PopStyleVar(3);
ImGui::PopStyleColor(1);
ImGui::PopFont();
//ImGui::GetFont()->Scale = origScale;
//ImGui::Image(img_icon, ImVec2{ 1.0f, 1.0f });
//ImGui::SetWindowFontScale(1.75f);


                 {
            ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
            if (disable_mouse_wheel)
                window_flags |= ImGuiWindowFlags_NoScrollWithMouse;
            
           ImGui::PushFont(font2);
            //ImGui::GetFont()->Scale =1.0f;
            ImGui::BeginChild("ChildL", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), ImGuiChildFlags_None, window_flags);
           //ImGui::PushFont(font2);
           //ImGui::GetFont()->Scale =1.0f;
           //ImGui::PushFont(ImGui::GetFont());
            //ImGui::Begin("Hello, world!", &show_window); 
          // ImGui::PushFont(font2);
         // DoFitTextToWindow(fontB,"retard");
           
            
            
       // std::vector<char*> list;

           // char** items = a2.get(); //{ "current day" };
            //items = new char[items[i].length() + 1];
    
            static int item_current = 0;
            static int prev_item=0;
            static bool changed=false;
            static bool display_overall_barchart=false;
            static bool update_app_day_info=false;
            static int use;//=bar_Y_axis_flag;
           static std::string unit;
            //  static  char*  ilabels;//   = {"Midterm Exam","shit term","retard"};
  //  static  char* glabels;
    //static  double positions[] = {0,1,2,3,4,5,6,7,8,9};

            ImGui::Combo("combo", &item_current, items, nooffiles+1);
            if(prev_item!=item_current)
            {
                changed=true;
            }
            if(changed==true)
            {
                std::cout<<"changed!";
                prev_item=item_current;
        
                if(item_current!=0)
                {
                auto a3 = std::async(std::launch::deferred,&X::fetch_day_data, &x,items[item_current]);
              char* json= a3.get(); 
                //std::cout<<json;
                DayAppData=nlohmann::json::parse(json);
                 
                     display_overall_barchart=true;
                     update_app_day_info=true;
                changed=false;
                
                }
                else
                {
                    // FETCH REALTIME DATA AND SHOW
                }
            }
            if(update_app_day_info)
            {
                std::cout<<"called update app day \n";
               apps= get_app_details();
               std::cout<<"finished update app day \n";
                //data=load_data(apps);
                data=  load_data(apps);
                 std::cout<<" load app day \n";

              groups= DayAppData.size();   
             positions = (double*) malloc(sizeof(double)*DayAppData.size());
             for(int i=0;i<DayAppData.size();i++)
             {
                positions[i]=i;
             }

                x_axis_names= (const char**)malloc(sizeof(char*)*DayAppData.size());
                x_axis_names=get_X_axis_names(apps);
                ilabels=(char**)malloc(sizeof(char*));
                ilabels[0]=(char*)malloc(sizeof(char)*11);
                ilabels[0]=items[item_current];
                use=bar_Y_axis_flag;
                if(use==1)
                {
                    
                    unit ="Seconds";
                }
                if(use==2)
                {
                
                    unit="Minutes";
                }
                if(use==3)
                {
                
                    unit="Hours";
                }
                // for(int i=0;i<DayAppData.size();i++)
                // {
                //     x_axis_names[i]= apps[0][i];
                // }
            x_axis_names[DayAppData.size()] =nullptr;

               update_app_day_info=false;



            }
            
            if(display_overall_barchart)
            {
                

  

    


// if(use_hours)
// {
//     use_minutes=false; use_seconds=false;
// }
// if(use_minutes)
// {
//     use_seconds=false; use_hours=false;

// }
// if(use_seconds)
// {
//     use_hours=false; use_minutes=false;
// }
   
    static char* no_of_items[]={"All","3","5","10","15"}; 
    static int no_of_items_pos=0; 
     
    static int last_use=use;
if(use!=last_use)
{
    if(use==1)
    {
        
        unit ="Seconds";
    }
    if(use==2)
    {
     
        unit="Minutes";
    }
    if(use==3)
    {
     
        unit="Hours";
    }
    last_use=use;
}    
    ImGui::Text("Y Axis:"); ImGui::SameLine();
    ImGui::Text("Hours");ImGui::SameLine();
  ImGui::RadioButton("",&use,3); ImGui::SameLine();
 ImGui::Text("Minutes");ImGui::SameLine();
  ImGui::RadioButton("",&use,2); ImGui::SameLine();
  ImGui::Text("Seconds");ImGui::SameLine();
  ImGui::RadioButton("",&use,1);

ImGui::Text("Show"); ImGui::SameLine();
ImGui::Combo(".", &no_of_items_pos, no_of_items, 5);


 //   ImGui::SliderFloat("Size",&size,0,1);

//    if (ImPlot::BeginPlot("My Plot")) {
//    // ImPlot::PlotBars("My Bar Plot", bar_data, 11);
//       ImPlot::PlotBars("Vertical",data,no_of_items,0.5,1);
//        // ImPlot::PlotBars("Horizontal",data,10,0.4,1,ImPlotBarsFlags_Horizontal);
//     //ImPlot::PlotLine("My Line Plot", x_data, y_data, 1000);

//     ImPlot::EndPlot();
// }

    
    //static const char**  glabels   =get_X_axis_names(apps);
    

    //const int sized = sizeof(glabels) / sizeof(glabels[0]);

    // Create an array of pointers to constant characters
   
    
    //convert(glabels, sized, convertedArray);

    static int items  = 1;
    
    static float size = 0.67f;

    static ImPlotBarGroupsFlags flags = 0;
    static bool horz = false;

    ImGui::CheckboxFlags("Stacked", (unsigned int*)&flags, ImPlotBarGroupsFlags_Stacked);
    ImGui::SameLine();
    ImGui::Checkbox("Horizontal",&horz);

    ImGui::SliderInt("Items",&items,1,3);
    ImGui::SliderFloat("Size",&size,0,1);

    if (ImPlot::BeginPlot("Single Day App usage")) {
        ImPlot::SetupLegend(ImPlotLocation_East, ImPlotLegendFlags_Outside);
        if (horz) {
            ImPlot::SetupAxes(unit.c_str(),"Applications",ImPlotAxisFlags_AutoFit,ImPlotAxisFlags_AutoFit);
            ImPlot::SetupAxisTicks(ImAxis_Y1,positions, groups, x_axis_names);
            ImPlot::PlotBarGroups((const char**) ilabels,data,items,groups,size,0,flags|ImPlotBarGroupsFlags_Horizontal);
        }
        else {
            ImPlot::SetupAxes("Applications",unit.c_str(),ImPlotAxisFlags_AutoFit,ImPlotAxisFlags_AutoFit);
            ImPlot::SetupAxisTicks(ImAxis_X1,positions, groups, x_axis_names);
            ImPlot::PlotBarGroups(ilabels,data,items,groups,size,0,flags);
        }
        ImPlot::EndPlot();
    }
            }



            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
          //  ImGui::End();
           
           ImGui::PopFont();
            ImGui::EndChild();
        }

                                    // Create a window called "Hello, world!" and append into it.

//ImGui::PushFont(font_k12hl2);
//ImGui::TextColored(ImVec4{ 1.0f,1.0f,1.0f,1.0f }, "Title");
//ImGui::PopFont();

// if (ImGui::BeginMainMenuBar()) 
        // {
	    //     if (ImGui::BeginMenu("File"))
	    //     {
		//         if (ImGui::MenuItem("Exit")) // It would be nice if this was a "X" like in the windows title bar set off to the far right
		//         {
		// 	        glfwSetWindowShouldClose(window, GLFW_TRUE);
		//         }
        
        //                 // It would also be nice if there could be a button for minimizing the window and a button for maximizing the window
        
		//         ImGui::EndMenu();
	    //     }
        
	    //     ImGui::EndMainMenuBar();
        // }
       // ImGui::PopFont();
ImGui::End();
        

    //     if(show_demo_window)
    //     {
    //         ImGui::ShowDemoWindow();
    //     }
    //   // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
    //     {
    //         static float f = 0.0f;
    //         static int counter = 0;

    //         ImGui::Begin("Hello, world!", &show_window);                          // Create a window called "Hello, world!" and append into it.

    //         ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
    //         ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
    //         ImGui::Checkbox("Another Window", &show_another_window);

    //         ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
    //         ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

    //         if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
    //             counter++;
    //         ImGui::SameLine();
    //         ImGui::Text("counter = %d", counter);

    //         ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    //         ImGui::End();
    //     }
        
	// //ImPlot::ShowDemoWindow();
    //     // 3. Show another simple window.
    //     if (show_another_window)
    //     {
    //         ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
    //         ImGui::Text("Hello from another window!");
    //         if (ImGui::Button("Close Me"))
    //             show_another_window = false;
    //         ImGui::End();
    //     }

//     // Setup Platform/Renderer backends
    

        
        


        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        ImGui::UpdatePlatformWindows();
ImGui::RenderPlatformWindowsDefault();
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);

    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
	ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

   
    return 0;
}

bool init_gui()
{


}


ImFont* AddDefaultFont( float pixel_size )
{
ImGuiIO &io = ImGui::GetIO();
ImFontConfig config;
config.SizePixels = pixel_size;
config.OversampleH = config.OversampleV = 1;
config.PixelSnapH = true;
ImFont *font = io.Fonts->AddFontFromFileTTF("./misc/fonts/Roboto-Medium.ttf",16.0f,&config);
return font;
}

void DoFitTextToWindow(ImFont *font, const char *text)
{
ImGui::PushFont( font );
ImVec2 sz = ImGui::CalcTextSize(text);
ImGui::PopFont();
float canvasWidth = ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x;
float origScale = font->Scale;
font->Scale = canvasWidth / sz.x;
ImGui::PushFont( font );
ImGui::Text("%s", text);
ImGui::PopFont();
font->Scale = origScale;
}