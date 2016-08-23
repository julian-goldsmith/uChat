// ImGui - standalone example application for SDL2 + OpenGL
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.

#include <stdio.h>
#include <SDL2/SDL.h>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <winsock2.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl_gl3.h"
#include "GL/gl3w.h"
#include "videoInput/videoInput.h"
#include "imgcoder.h"
#include "dct.h"

int main(int, char**)
{
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // Setup window
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_DisplayMode current;
    SDL_GetCurrentDisplayMode(0, &current);
    SDL_Window *window = SDL_CreateWindow("uChat",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          1280, 720, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
    SDL_GLContext glcontext = SDL_GL_CreateContext(window);
    gl3wInit();

    // Setup ImGui binding
    ImGui_ImplSdlGL3_Init(window);

    bool show_another_window = false;
    ImVec4 clear_color = ImColor(114, 144, 154);

    videoInput vi;

    vi.setupDevice(0, 640, 480);
    vi.setUseCallback(true);

    unsigned char* frame = (unsigned char*) malloc(vi.getSize(0));
    unsigned char* prevframe = (unsigned char*) malloc(vi.getSize(0));
    unsigned char* decodedframe = (unsigned char*) malloc(vi.getSize(0));
    unsigned char* rmsView = (unsigned char*) malloc(vi.getSize(0));

    memset(frame, 0xcc, vi.getSize(0) * sizeof(char));
    memset(prevframe, 0xcc, vi.getSize(0) * sizeof(char));
    memset(decodedframe, 0xcc, vi.getSize(0) * sizeof(char));
    memset(rmsView, 0xcc, vi.getSize(0) * sizeof(char));

    vi.getPixels(0, decodedframe, true, true); // get first frame

    GLuint rawinput_id;
    glGenTextures(1, &rawinput_id);

    GLuint decoded_id;
    glGenTextures(1, &decoded_id);

    GLuint rmsView_id;
    glGenTextures(1, &rmsView_id);

    dct_precompute_matrix();

    int totalSize = 0;
    int numFrames = 1;
    float avgSize = 0.0f;
/*
	WSADATA wsa;
	SOCKET s;

	if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
	{
		printf("Failed. Error Code : %d",WSAGetLastError());
		return 1;
	}

	if((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		printf("Could not create socket : %d" , WSAGetLastError());
		return 1;
	}

	struct sockaddr_in server;

	server.sin_addr.s_addr = inet_addr("108.61.193.139");
	server.sin_family = AF_INET;
	server.sin_port = htons(7777);

	//Connect to remote server
	if (connect(s, (struct sockaddr *)&server , sizeof(server)) < 0)
	{
		printf("connect error\n");
		return 1;
	}*/

    // Main loop
    bool done = false;
    while (!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSdlGL3_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
        }

        if(vi.isFrameNew(0))
        {
            numFrames++;

            memcpy(prevframe, decodedframe, vi.getSize(0));

            vi.getPixels(0, frame, true, true);

            unsigned char* encodedImage = ic_encode_image(frame, prevframe, rmsView, &totalSize);
            ic_decode_image(prevframe, encodedImage, decodedframe);
            /*if(send(s, (const char*) encodedImage, totalSize, 0) < 0)
            {
                printf("Send failed\n");
                return 1;
            }*/
            free(encodedImage);
            avgSize = ((avgSize * (numFrames - 1)) + totalSize) / numFrames;

            GLint last_texture;
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);

            glBindTexture(GL_TEXTURE_2D, rawinput_id);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 640, 480, 0, GL_RGB, GL_UNSIGNED_BYTE, frame);

            glBindTexture(GL_TEXTURE_2D, decoded_id);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 640, 480, 0, GL_RGB, GL_UNSIGNED_BYTE, decodedframe);

            glBindTexture(GL_TEXTURE_2D, rmsView_id);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 640, 480, 0, GL_RGB, GL_UNSIGNED_BYTE, rmsView);

            glBindTexture(GL_TEXTURE_2D, last_texture);
        }

        ImGui_ImplSdlGL3_NewFrame(window);

        ImGui::SetNextWindowSize(ImVec2(1280,700));
        ImGui::Begin("Window", &show_another_window);

        ImGui::Image((void *)(intptr_t)rmsView_id, ImVec2(640, 480));

        ImGui::SameLine();
        ImGui::Text("Last frame size: %i", totalSize);

        ImGui::SameLine();
        ImGui::Text("Avg frame size: %i", (int) avgSize);

        ImGui::Image((void *)(intptr_t)rawinput_id, ImVec2(640, 480));

        ImGui::SameLine();
        ImGui::Image((void *)(intptr_t)decoded_id, ImVec2(640, 480));

        ImGui::End();

        // Rendering
        glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui::Render();
        SDL_GL_SetSwapInterval(1);
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    ImGui_ImplSdlGL3_Shutdown();
    SDL_GL_DeleteContext(glcontext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
