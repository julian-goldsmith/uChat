// ImGui - standalone example application for SDL2 + OpenGL
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.

#include <stdio.h>
#include <SDL2/SDL.h>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <winsock2.h>
#include <pthread.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl_gl3.h"
#include "GL/gl3w.h"
#include "videoInput/videoInput.h"
#include "imgcoder.h"
#include "dct.h"

typedef struct
{
    unsigned char* encoded_frame;
    unsigned char* raw_frame;
    unsigned char* rms_view;
    int total_size;
    int numFrames;
} frame_data_t;

float avgSize = 0.0f;
float avgEncodeTime = 0.0f;

pthread_mutex_t sync_mutex;

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

void update_views(GLuint rawinput_id, GLuint decoded_id, GLuint rmsView_id, unsigned char* prev_frame, frame_data_t* data)
{
    unsigned char* decoded_frame = (unsigned char*) malloc(3 * 640 * 480);      // FIXME: don't hardcode

    if(data->encoded_frame == NULL)
    {
        return;
    }

    ic_decode_image(prev_frame, data->encoded_frame, data->total_size, decoded_frame);
    memcpy(prev_frame, decoded_frame, 3 * 640 * 480);       // FIXME: don't hardcode

    GLint last_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);

    glBindTexture(GL_TEXTURE_2D, rawinput_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 640, 480, 0, GL_RGB, GL_UNSIGNED_BYTE, data->raw_frame);

    glBindTexture(GL_TEXTURE_2D, decoded_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 640, 480, 0, GL_RGB, GL_UNSIGNED_BYTE, decoded_frame);

    glBindTexture(GL_TEXTURE_2D, rmsView_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 640, 480, 0, GL_RGB, GL_UNSIGNED_BYTE, data->rms_view);

    glBindTexture(GL_TEXTURE_2D, last_texture);

    free(decoded_frame);
}

void update_ui(SDL_Window* window, GLuint rmsView_id, GLuint rawinput_id, GLuint decoded_id,
               int avgSize, int avgEncodeTime, frame_data_t* data)
{
    // update stats
    if(data->numFrames > 0)
    {
        avgSize = ((avgSize * (data->numFrames - 1)) + data->total_size) / data->numFrames;
        //avgEncodeTime = ((avgEncodeTime * (data->numFrames - 1)) + encodeTimeTemp) / data->numFrames;
    }

    bool show_another_window = false;
    ImGui::SetNextWindowSize(ImVec2(1280,700));
    ImGui::Begin("Window", &show_another_window);

    ImGui::Image((void *)(intptr_t)rmsView_id, ImVec2(640, 480));

    ImGui::SameLine();
    ImGui::Text("Last frame size: %i", data->total_size);

    ImGui::SameLine();
    ImGui::Text("Avg frame size: %i", (int) avgSize);

    //ImGui::SameLine();
    //ImGui::Text("Avg encode time: %i", (int) avgEncodeTime);

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

void* run_frame(void* param)
{
    frame_data_t* data = (frame_data_t*) param;
    unsigned char* encoded_frame = data->encoded_frame;
    unsigned char* raw_frame = data->raw_frame;
    unsigned char* rms_view = data->rms_view;

    videoInput vi;

    vi.setupDevice(0, 640, 480);
    vi.setUseCallback(true);

    unsigned char* prev_frame = (unsigned char*) malloc(vi.getSize(0));

    vi.getPixels(0, prev_frame, true, true);

    while(true)
    {
        if(vi.isFrameNew(0))
        {
            pthread_mutex_lock(&sync_mutex);

            data->numFrames++;

            vi.getPixels(0, raw_frame, true, true);

            if(encoded_frame != NULL)
            {
                free(encoded_frame);
            }

            int encodeTimeTemp = SDL_GetTicks();

            encoded_frame = ic_encode_image(raw_frame, prev_frame, rms_view, &data->total_size);

            encodeTimeTemp = SDL_GetTicks() - encodeTimeTemp;

            memcpy(prev_frame, raw_frame, vi.getSize(0));

            pthread_mutex_unlock(&sync_mutex);
        }
    }

    pthread_exit(NULL);

    return NULL;
}

int main(int, char**)
{
    GLuint rawinput_id;
    GLuint decoded_id;
    GLuint rmsView_id;
    SDL_GLContext glcontext;
    SDL_Window *window = init_ui(&glcontext, &rawinput_id, &decoded_id, &rmsView_id);

    dct_precompute_matrix();

    frame_data_t* data = (frame_data_t*) calloc(1, sizeof(frame_data_t));
    data->raw_frame = (unsigned char*) malloc(3*640*480);
    //decoded_frame = (unsigned char*) malloc(vi.getSize(0));
    data->rms_view = (unsigned char*) malloc(3*640*480);

    // Main loop
    bool done = false;

    pthread_t thread;

    pthread_mutex_init(&sync_mutex, NULL);

    int rc = pthread_create(&thread, NULL, run_frame, data);
    if(rc)
    {
        printf("Unable to create thread: %i\n", rc);
        exit(-1);
    }

    unsigned char* prev_frame = (unsigned char*) malloc(3 * 640 * 480);     // FIXME: don't hardcode

    while (!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSdlGL3_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
        }

        pthread_mutex_lock(&sync_mutex);
        update_views(rawinput_id, decoded_id, rmsView_id, prev_frame, data);
        pthread_mutex_unlock(&sync_mutex);

        ImGui_ImplSdlGL3_NewFrame(window);

        update_ui(window, rmsView_id, rawinput_id, decoded_id, (int) avgSize, (int) avgEncodeTime, data);

        finish_frame(window);
    }

    // Cleanup
    ImGui_ImplSdlGL3_Shutdown();
    SDL_GL_DeleteContext(glcontext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    pthread_mutex_lock(&sync_mutex);
    int err = pthread_cancel(thread);
    if(err)
    {
        printf("Unable to cancel thread: %i\n", err);
    }
    pthread_mutex_unlock(&sync_mutex);

    err = pthread_mutex_destroy(&sync_mutex);
    if(err)
    {
        printf("Unable to cancel mutex: %i\n", err);
    }

    return 0;
}
