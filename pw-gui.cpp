#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#include <glfw3.h>


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
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

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
    bool show_demo_window = true;
    bool show_another_window = false;
    bool show_window = true;
    ImVec4 clear_color = ImVec4(0.10f, 0.10f, 0.25f, 1.00f);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
            
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
ImGui::SetNextWindowSize(ImVec2{ viewport->WorkSize.x, 25.0f });
ImGui::SetNextWindowViewport(viewport->ID);
ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2{ 0.0f, 0.0f });
ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 40.0f, 2.0f });
ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4{ 0.01f, 0.01f, 0.01f, 1.0f });
            ImGui::Begin("Productivity Watch", &show_window, titlebar_flags);
ImGui::PopStyleVar(3);
ImGui::PopStyleColor(1);
//ImGui::Image(img_icon, ImVec2{ 1.0f, 1.0f });
ImGui::SetWindowFontScale(1.75f);
 static float f = 0.0f;
            static int counter = 0;

                                    // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
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
