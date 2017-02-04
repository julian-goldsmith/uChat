// ImGui - standalone example application for SDL2 + OpenGL
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.

#define SDL_MAIN_HANDLED

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
#include "net.h"
#include "array.h"
#include "lzw.h"
#include "arraypool.h"

typedef struct
{
    unsigned char* decoded_frame;
    unsigned char* encoded_frame;
    unsigned char* raw_frame;
    unsigned char* rms_view;
    int total_size;
    int num_frames;
    int last_encode_time;
    float avg_size;
    float avg_encode_time;
} frame_data_t;

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

    unsigned char* blank = (unsigned char*) calloc(1, 3*640*480);
    glBindTexture(GL_TEXTURE_2D, *rawinput_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 640, 480, 0, GL_RGB, GL_UNSIGNED_BYTE, blank);

    glBindTexture(GL_TEXTURE_2D, *decoded_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 640, 480, 0, GL_RGB, GL_UNSIGNED_BYTE, blank);

    glBindTexture(GL_TEXTURE_2D, *rmsView_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 640, 480, 0, GL_RGB, GL_UNSIGNED_BYTE, blank);

    return window;
}

void update_views(GLuint rawinput_id, GLuint decoded_id, GLuint rmsView_id, unsigned char* prev_frame, frame_data_t* data)
{
    if(data->decoded_frame == NULL)
    {
        data->decoded_frame = (unsigned char*) malloc(3 * 640 * 480);      // FIXME: don't hardcode
    }

    if(data->encoded_frame == NULL)
    {
        return;
    }

    short num_blocks;
    compressed_macroblock_t* cblocks = net_deserialize_compressed_blocks(data->encoded_frame, &num_blocks);
    ic_decode_image(prev_frame, cblocks, num_blocks, data->decoded_frame);
    memcpy(prev_frame, data->decoded_frame, 3 * 640 * 480);       // FIXME: don't hardcode
    ic_clean_up_compressed_blocks(cblocks, num_blocks);

    GLint last_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);

    glBindTexture(GL_TEXTURE_2D, rawinput_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 640, 480, 0, GL_RGB, GL_UNSIGNED_BYTE, data->raw_frame);

    glBindTexture(GL_TEXTURE_2D, decoded_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 640, 480, 0, GL_RGB, GL_UNSIGNED_BYTE, data->decoded_frame);

    glBindTexture(GL_TEXTURE_2D, rmsView_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 640, 480, 0, GL_RGB, GL_UNSIGNED_BYTE, data->rms_view);

    glBindTexture(GL_TEXTURE_2D, last_texture);
}

void update_ui(SDL_Window* window, GLuint rmsView_id, GLuint rawinput_id, GLuint decoded_id, frame_data_t* data)
{
    bool show_another_window = false;
    ImGui::SetNextWindowSize(ImVec2(1280,700));
    ImGui::Begin("Window", &show_another_window);

    ImGui::Image((void *)(intptr_t)rmsView_id, ImVec2(640, 480));

    ImGui::SameLine();
    ImGui::Text("Last frame size: %i", data->total_size);

    ImGui::SameLine();
    ImGui::Text("Avg frame size: %i", (int) data->avg_size);

    ImGui::SameLine();
    ImGui::Text("Avg encode time: %i", (int) data->avg_encode_time);

    ImGui::SameLine();
    ImGui::Text("Avg KB/s: %i", (int) ((data->avg_size * 30) / 1000));

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

    videoInput vi;

    vi.setupDevice(0, 640, 480);

    data->raw_frame = (unsigned char*) malloc(vi.getSize(0));
    data->rms_view = (unsigned char*) malloc(vi.getSize(0));

    unsigned char* decoded_frame = (unsigned char*) malloc(vi.getSize(0));
    unsigned char* encoded_frame = NULL;
    unsigned char* raw_frame = (unsigned char*) malloc(vi.getSize(0));
    unsigned char* rms_view = (unsigned char*) malloc(vi.getSize(0));
    int total_size = 0;
    int num_frames = 0;
    int last_encode_time = 0;
    float avg_size = 0;
    float avg_encode_time = 0;

    while(true)
    {
        int loopStart = SDL_GetTicks();
        if(vi.isFrameNew(0))
        {
            pthread_mutex_lock(&sync_mutex);
            memcpy(decoded_frame, data->decoded_frame, vi.getSize(0));
            pthread_mutex_unlock(&sync_mutex);

            vi.getPixels(0, raw_frame, true, true);

            int encodeTimeTemp = SDL_GetTicks();

            short num_blocks;
            compressed_macroblock_t* cblocks = ic_encode_image(raw_frame, decoded_frame, rms_view, &num_blocks);
            encoded_frame = net_serialize_compressed_blocks(cblocks, &total_size, num_blocks);
            ic_clean_up_compressed_blocks(cblocks, num_blocks);

            // update stats
            last_encode_time = SDL_GetTicks() - encodeTimeTemp;
            avg_size = (avg_size * num_frames + total_size) / (num_frames + 1);
            avg_encode_time = (avg_encode_time * num_frames + last_encode_time) / (num_frames + 1);
            num_frames++;

            pthread_mutex_lock(&sync_mutex);

            memcpy(data->raw_frame, raw_frame, vi.getSize(0));
            memcpy(data->rms_view, rms_view, vi.getSize(0));

            if(data->encoded_frame != NULL)
            {
                free(data->encoded_frame);
            }

            data->encoded_frame = (unsigned char*) malloc(total_size);
            memcpy(data->encoded_frame, encoded_frame, total_size);

            data->total_size = total_size;
            data->num_frames = num_frames;
            data->last_encode_time = last_encode_time;
            data->avg_size = avg_size;
            data->avg_encode_time = avg_encode_time;

            pthread_mutex_unlock(&sync_mutex);

            free(encoded_frame);
        }

        int loopLen = SDL_GetTicks() - loopStart;

        if(loopLen < 34)
        {
            // updates every 34ms ~= 30FPS
            SDL_Delay(34 - loopLen);
        }
    }

    pthread_exit(NULL);

    return NULL;
}

#include "lzw.h"

int main(int argc, char** argv)
{
    SDL_SetMainReady();
    SDL_Init(SDL_INIT_VIDEO);

    printf("PATH %s\n", getenv("PATH"));

    GLuint rawinput_id;
    GLuint decoded_id;
    GLuint rmsView_id;
    SDL_GLContext glcontext;
    SDL_Window *window = init_ui(&glcontext, &rawinput_id, &decoded_id, &rmsView_id);

    dct_precompute_matrix();
    array_pool_init();

    frame_data_t* data = (frame_data_t*) calloc(1, sizeof(frame_data_t));

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

        if(pthread_mutex_trylock(&sync_mutex) == 0)
        {
            update_views(rawinput_id, decoded_id, rmsView_id, prev_frame, data);
            pthread_mutex_unlock(&sync_mutex);
        }

        ImGui_ImplSdlGL3_NewFrame(window);

        update_ui(window, rmsView_id, rawinput_id, decoded_id, data);

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
