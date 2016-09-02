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

SDL_Window* init_ui(SDL_GLContext* glcontext, GLuint* rawinput_id, GLuint* decoded_id, GLuint* rmsView_id)
{
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        exit(-1);
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
    *glcontext = SDL_GL_CreateContext(window);
    gl3wInit();

    // Setup ImGui binding
    ImGui_ImplSdlGL3_Init(window);

    glGenTextures(1, rawinput_id);
    glGenTextures(1, decoded_id);
    glGenTextures(1, rmsView_id);

    return window;
}

void update_views(GLuint rawinput_id, unsigned char* frame, GLuint decoded_id,
                  unsigned char* decodedframe, GLuint rmsView_id, unsigned char* rmsView)
{
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

void update_ui(SDL_Window* window, GLuint rmsView_id, GLuint rawinput_id, GLuint decoded_id,
               int totalSize, int avgSize, int avgEncodeTime)
{
    ImGui_ImplSdlGL3_NewFrame(window);

    bool show_another_window = false;
    ImGui::SetNextWindowSize(ImVec2(1280,700));
    ImGui::Begin("Window", &show_another_window);

    ImGui::Image((void *)(intptr_t)rmsView_id, ImVec2(640, 480));

    ImGui::SameLine();
    ImGui::Text("Last frame size: %i", totalSize);

    ImGui::SameLine();
    ImGui::Text("Avg frame size: %i", (int) avgSize);

    ImGui::SameLine();
    ImGui::Text("Avg encode time: %i", (int) avgEncodeTime);

    ImGui::Image((void *)(intptr_t)rawinput_id, ImVec2(640, 480));

    ImGui::SameLine();
    ImGui::Image((void *)(intptr_t)decoded_id, ImVec2(640, 480));

    ImGui::End();
}

void finish_frame(SDL_Window* window)
{
    ImVec4 clear_color = ImColor(114, 144, 154);

    glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui::Render();
    SDL_GL_SetSwapInterval(1);
    SDL_GL_SwapWindow(window);
}

int totalSize = 0;
int numFrames = 1;
float avgSize = 0.0f;
float avgEncodeTime = 0.0f;

void run_frame(videoInput& vi, unsigned char* prevframe, unsigned char* decodedframe,
               unsigned char* frame, unsigned char* rmsView)
{
    numFrames++;

    memcpy(prevframe, decodedframe, vi.getSize(0));

    vi.getPixels(0, frame, true, true);

    int encodeTimeTemp = SDL_GetTicks();
    unsigned char* encodedImage = ic_encode_image(frame, prevframe, rmsView, &totalSize);
    encodeTimeTemp = SDL_GetTicks() - encodeTimeTemp;

    ic_decode_image(prevframe, encodedImage, totalSize, decodedframe);

    free(encodedImage);

    avgSize = ((avgSize * (numFrames - 1)) + totalSize) / numFrames;
    avgEncodeTime = ((avgEncodeTime * (numFrames - 1)) + encodeTimeTemp) / numFrames;
}

int main(int, char**)
{
    GLuint rawinput_id;
    GLuint decoded_id;
    GLuint rmsView_id;
    SDL_GLContext glcontext;
    SDL_Window *window = init_ui(&glcontext, &rawinput_id, &decoded_id, &rmsView_id);

    videoInput vi;

    vi.setupDevice(0, 640, 480);
    vi.setUseCallback(true);

    unsigned char* frame = (unsigned char*) malloc(vi.getSize(0));
    unsigned char* prevframe = (unsigned char*) malloc(vi.getSize(0));
    unsigned char* decodedframe = (unsigned char*) malloc(vi.getSize(0));
    unsigned char* rmsView = (unsigned char*) malloc(vi.getSize(0));

    vi.getPixels(0, decodedframe, true, true); // get first frame

    dct_precompute_matrix();

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
            run_frame(vi, prevframe, decodedframe, frame, rmsView);
            update_views(rawinput_id, frame, decoded_id, decodedframe, rmsView_id, rmsView);
        }

        update_ui(window, rmsView_id, rawinput_id, decoded_id, totalSize, (int) avgSize, (int) avgEncodeTime);

        finish_frame(window);
    }

    // Cleanup
    ImGui_ImplSdlGL3_Shutdown();
    SDL_GL_DeleteContext(glcontext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
