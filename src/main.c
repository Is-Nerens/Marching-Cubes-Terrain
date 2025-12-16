#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <cglm/cglm.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "mesh.h"
#include "shader.h"
#include "generate.h"
#include "generator_gpu.h"
#include "camera.h"
#include "texture.h"
#include "terrain/terrain.h"
#include "terrain_renderer.h"


int main()
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_SetHint("SDL_MOUSE_FOCUS_CLICKTHROUGH", "1");
    SDL_Window* window = SDL_CreateWindow("Marching Cubes Terrain", 1000, 800, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    // Create OpenGL context for the main window
    SDL_GLContext context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, context);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    // glEnable(GL_CULL_FACE);
    // glCullFace(GL_BACK); 
    glFrontFace(GL_CW);
    SDL_GL_SetSwapInterval(0); // VSYNC ON
    glewInit();






    GeneratorGPU generator;
    GeneratorGPUInit(&generator, 24);

    TerrainRenderer renderer;
    TerrainRendererInit(&renderer);

    Terrain terrain;
    TerrainInit(&terrain);


    Camera camera;
    CameraInit(&camera);

    // ------------------------
    // --- Application loop ---
    // ------------------------
    uint64_t freq = SDL_GetPerformanceFrequency();
    uint64_t last = SDL_GetPerformanceCounter();

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
            if (event.type == SDL_EVENT_MOUSE_MOTION) {
                CameraMouseMove(&camera, (float)event.motion.xrel, (float)event.motion.yrel);
            }
        }
        CameraMove(&camera, deltaTime);
        CameraUpdate(&camera);

        SearchForEmptyChunks(&terrain, camera.position[0], camera.position[1], camera.position[2]);
        GenerateChunks(&terrain, &generator);


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