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
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK); 
    SDL_GL_SetSwapInterval(0); // VSYNC ON
    glewInit();






    GeneratorGPU generator;
    GeneratorGPUInit(&generator, 48);



    Texture rockAlbedo;
    Texture rockNormal;
    Texture grassAlbedo;
    Texture grassNormal;
    TextureLoadFile(&rockAlbedo, "textures/rock_albedo.jpg");
    TextureLoadFile(&rockNormal, "textures/rock_normal.jpg");
    TextureLoadFile(&rockAlbedo, "textures/grass_albedo.jpg");
    TextureLoadFile(&rockNormal, "textures/grass_normal.jpg");
    glUniform1i(glGetUniformLocation(generator.computeShaderProgram, "u_rock_albedo_texture"), 0); 
    glUniform1i(glGetUniformLocation(generator.computeShaderProgram, "u_rock_normal_texture"), 1); 
    glUniform1i(glGetUniformLocation(generator.computeShaderProgram, "u_grass_albedo_texture"), 2); 
    glUniform1i(glGetUniformLocation(generator.computeShaderProgram, "u_grass_normal_texture"), 3); 
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, rockAlbedo.handle);
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, rockNormal.handle);
    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_2D, grassAlbedo.handle);
    glActiveTexture(GL_TEXTURE0 + 3);
    glBindTexture(GL_TEXTURE_2D, grassNormal.handle);









    uint64_t freq = SDL_GetPerformanceFrequency();
    uint64_t start = SDL_GetPerformanceCounter();
    Mesh mesh;
    MeshInit(&mesh, 20000, 20000);
    GeneratorGPUGenerateChunk(&generator, &mesh);
    printf("number of vertices: %d\n", (int)mesh.vertexCount);
    uint64_t end = SDL_GetPerformanceCounter();
    float duration = (float)(end - start) / (float)freq;
    printf("generation time: %f\n", duration);


    // uint64_t freq = SDL_GetPerformanceFrequency();
    // uint64_t start = SDL_GetPerformanceCounter();
    // Mesh mesh;
    // MeshInit(&mesh, 1000, 10000);
    // GenerateChunk(&mesh, 32);
    // printf("number of vertices: %d\n", (int)mesh.vertexCount);
    // uint64_t end = SDL_GetPerformanceCounter();
    // float duration = (float)(end - start) / (float)freq;
    // printf("generation time: %f\n", duration);

    Camera camera;
    CameraInit(&camera);


    GLuint chunk_shader_program = Create_Shader_Program_From_File("shaders/chunk_vert.glsl", "shaders/chunk_frag.glsl");
    int cameraPosUniformLocation = glGetUniformLocation(chunk_shader_program, "u_CameraPos");
    int mvpUniformLocation = glGetUniformLocation(chunk_shader_program, "u_MVP");
    int modelMatrixUniformLocation = glGetUniformLocation(chunk_shader_program, "u_ModelPositionMatrix");
    glUseProgram(chunk_shader_program);



    mat4 chunkMatrix;
    mat4 identity;
    vec3 chunkPosition = { 0.0f, 0.0f, 0.0f };
    glm_mat4_identity(identity);
    glm_translate_to(identity, chunkPosition, chunkMatrix);


    // upload vertex and index data
    MeshUploadToGPU(&mesh);

    // ------------------------
    // --- Application loop ---
    // ------------------------
    float mouseSensitivity = 0.002f; // adjust for speed
    float mx = 0, my = 0;

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
        }

        const bool *state = SDL_GetKeyboardState(NULL);
        vec3 move = { 0.0f, 0.0f, 0.0f };
        if (state[SDL_SCANCODE_W]) {
            glm_vec3_add(move, camera.forward, move);
        }
        if (state[SDL_SCANCODE_A]) {
            vec3 left;
            glm_vec3_scale(camera.right, -1.0f, left);
            glm_vec3_add(move, left, move);
        }
        if (state[SDL_SCANCODE_S]) {
            vec3 backward;
            glm_vec3_scale(camera.forward, -1.0f, backward);
            glm_vec3_add(move, backward, move);
        }
        if (state[SDL_SCANCODE_D]) {
            glm_vec3_add(move, camera.right, move);
        }
        if (state[SDL_SCANCODE_E]) {
            glm_vec3_add(move, camera.up, move);
        }
        if (state[SDL_SCANCODE_Q]) {
            vec3 down;
            glm_vec3_scale(camera.up, -1.0f, down);
            glm_vec3_add(move, down, move);
        }
        glm_vec3_normalize(move);
        vec3 move_scaled;
        glm_vec3_scale(move, deltaTime * 15.0f, move_scaled);
        glm_vec3_add(camera.position, move_scaled, camera.position);


        // printf("x: %f y: %f z: %f\n", camera.position[0], camera.position[1], camera.position[2]);


        CameraUpdate(&camera);

        mat4 mvp;
        glm_mat4_mul(camera.projectionViewMatrix, chunkMatrix, mvp);


        // set shader uniforms
        glUniformMatrix4fv(mvpUniformLocation, 1, GL_FALSE, (const GLfloat*)mvp);
        glUniformMatrix4fv(modelMatrixUniformLocation, 1, GL_FALSE, (const GLfloat*)chunkMatrix);
        glUniform3fv(cameraPosUniformLocation, 1, camera.position);




        // render
        glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        MeshDraw(&mesh);

        SDL_GL_SwapWindow(window);
    }

    TextureFree(&rockAlbedo);
    TextureFree(&rockNormal);
    TextureFree(&grassAlbedo);
    TextureFree(&grassNormal);
    GeneratorGPUFree(&generator);
    MeshFree(&mesh);
    SDL_Quit();
}