#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <cglm/cglm.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "mesh.h"
#include "shader.h"
#include "terrain/generator_cpu.h"
#include "terrain/generator_gpu.h"
#include "camera.h"
#include "texture.h"
#include "terrain/terrain.h"
#include "terrain/terrain_raycast.h"
#include "terrain_renderer.h"


int main()
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_SetHint("SDL_MOUSE_FOCUS_CLICKTHROUGH", "1");
    SDL_Window* window = SDL_CreateWindow("Marching Cubes Terrain", 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    // Create OpenGL context for the main window
    SDL_GLContext context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, context);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK); 
    glFrontFace(GL_CW);
    SDL_GL_SetSwapInterval(0); // VSYNC ON
    glewInit();
    SDL_SetWindowRelativeMouseMode(window, true);



    int windowWidth;
    int windowHeight;


    GeneratorGPU generator;
    GeneratorGPUInit(&generator, 24);

    TerrainRenderer renderer;
    TerrainRendererInit(&renderer);

    Terrain terrain;
    TerrainInit(&terrain, 24);


    Camera camera;
    CameraInit(&camera);

    // ------------------------
    // --- Application loop ---
    // ------------------------
    uint64_t freq = SDL_GetPerformanceFrequency();
    uint64_t last = SDL_GetPerformanceCounter();

    bool mouseVisible = false;

    bool running = true;
    while (running)
    {
        uint64_t now = SDL_GetPerformanceCounter();
        float deltaTime = (float)(now - last) / (float)freq;
        last = now;

        
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
                break;
            }
            else if (event.type == SDL_EVENT_MOUSE_MOTION) {
                if (!mouseVisible) CameraMouseLook(&camera, (float)event.motion.xrel, (float)event.motion.yrel);
            }
            else if (event.type == SDL_EVENT_WINDOW_RESIZED)
            {
                SDL_GetWindowSize(window, &windowWidth, &windowHeight);
                glViewport(0, 0, windowWidth, windowHeight);
            }
            else if (event.type == SDL_EVENT_KEY_DOWN)
            {
                if (event.key.scancode == SDL_SCANCODE_ESCAPE) {
                    mouseVisible = !mouseVisible;
                    SDL_SetWindowRelativeMouseMode(window, !mouseVisible);
                }
            }
        }
        CameraMove(&camera, deltaTime);
        CameraUpdate(&camera);

        Uint32 buttons = SDL_GetMouseState(NULL, NULL);
        if (buttons & SDL_BUTTON_MASK(SDL_BUTTON_LEFT)) { // Check left button
            Ray ray;
            glm_vec3_copy(camera.position, ray.origin);
            glm_vec3_copy(camera.forward, ray.dir);
            RayHit hit = TerrainRaycast(&terrain, ray);
            if (hit.hit) 
            {
                TerrainAddDensity(
                    &terrain, 
                    hit.position[0],
                    hit.position[1],
                    hit.position[2],
                    3, -1.5f * deltaTime
                );
            }
        }
        if (buttons & SDL_BUTTON_MASK(SDL_BUTTON_RIGHT)) { // Check right button
            Ray ray;
            glm_vec3_copy(camera.position, ray.origin);
            glm_vec3_copy(camera.forward, ray.dir);
            RayHit hit = TerrainRaycast(&terrain, ray);
            if (hit.hit) 
            {
                TerrainAddDensity(
                    &terrain, 
                    hit.position[0],
                    hit.position[1],
                    hit.position[2],
                    3, 1.5f * deltaTime
                );
            }
        }
        

        uint64_t start = SDL_GetPerformanceCounter();
        SearchForEmptyChunks(&terrain, camera.position[0], camera.position[1], camera.position[2]);
        DeleteDistantChunks(&terrain, camera.position[0], camera.position[1], camera.position[2]);
        GenerateChunks(&terrain, &generator);
        uint64_t end = SDL_GetPerformanceCounter();
        float time = (float)(end - start) / (float)freq;
        // printf("chunk generation time: %fms\n", time*1000);
        

        // Draw scene
        glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        DrawTerrain(&renderer, &terrain, &camera);


        SDL_GL_SwapWindow(window);
    }

    GeneratorGPUFree(&generator);
    TerrainRendererFree(&renderer);
    SDL_Quit();
}