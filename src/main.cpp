#include "window.h"
#include "Input.h"
#include "render_pipeline.h"
#include "model.h"
#include "camera.h"
#include "filesystem.h"
#include "marching_cubes_gpu.h"
#include <SFML/Graphics.hpp>
#include <GL/glew.h>
#include <iostream>
#include <chrono>


struct GLOBAL
{
    int WIDTH = 800;
    int HEIGHT = 600;
    float FRAME_TIME;
};


GLOBAL global;
sf::RenderWindow window;  
InputSystem Input{&window};
RenderPipeline renderPipeline;
Camera camera;

struct Chunk
{
    int x;
    int y;
    int z;
};



void ProcessInput() 
{
    // CLEAR INPUT SYSTEM STATE
    Input.NewFrame();

    sf::Event event;
    while (window.pollEvent(event)) 
    {
        // CLOSE WINDOW EVENT
        if (event.type == sf::Event::Closed) window.close();

        // RESIZE WINDOW EVENT
        else if (event.type == sf::Event::Resized) 
        {
            // ADJUST THE VIEW ON WINDOW RESIZE
            sf::FloatRect visibleArea(0, 0, event.size.width, event.size.height);
            window.setView(sf::View(visibleArea));

            // APPLY MINIMUM WINDOW SIZE
            window.setSize(sf::Vector2u(
                static_cast<unsigned int>(std::max(static_cast<int>(window.getSize().x), 100)),
                static_cast<unsigned int>(std::max(static_cast<int>(window.getSize().y), 400))
            ));       

            // UPDATE GLOBAL WINDO SIZE VARIABLES
            global.WIDTH = static_cast<int>(window.getSize().x);
            global.HEIGHT = static_cast<int>(window.getSize().y);

            // UPDATE CAMERA MATRIX
            camera.SetViewport(global.WIDTH, global.HEIGHT);
            camera.UpdateProjectionView();

            // UPDATE GL VIEWPORT
            glViewport(0, 0, global.WIDTH, global.HEIGHT);  
        }

        // INPUT SYSTEM HANDLE IO EVENTS
        Input.HandleEvent(event);
    } 
    // UPDATE INPUT SYSTEM STATE
    Input.UpdateKeyStates();
}

void Init()
{
    // INITIALISE MOUSE CURSOR 
    window.setMouseCursorVisible(true);
    Input.ShowMouse();

    // INITIALISE CAMERA
    camera.SetViewport(global.WIDTH, global.HEIGHT);
    camera.position = {0.0f, 0.0f, 0.0f};
    camera.UpdateProjectionView(); 

    // INITIALISE RENDER PIPELINE
    renderPipeline.Init();
}

int main() 
{
    sf::ContextSettings settings;
    settings.depthBits = 24;
    window.create(sf::VideoMode(global.WIDTH, global.HEIGHT), "SFML Window", sf::Style::Default, settings);
    window.setActive(true); 
    if (glewInit() != GLEW_OK) 
    {
        std::cout << "ERROR: Failed to initialize GLEW" << std::endl;
        return EXIT_FAILURE;
    }

    Init();


    TerrainGPU terrainGPU;
    std::vector<Model> models;
    std::vector<Chunk> chunks;
    Model model;


    float moveSpeed = 10.0f;
    float lookSensitivity = 15.0f;




    // MAIN UPDATE LOOP
    while (window.isOpen()) {
        Debug::StartTimer();
        auto start = std::chrono::high_resolution_clock::now();
        window.clear();
        ProcessInput();


        // TOGGLE MOUSE
        if (Input.GetKeyDown(KeyCode::Escape))
        {
            if (Input.MouseHidden())
            {
                // SHOW MOUSE
                window.setMouseCursorVisible(true);
                Input.ShowMouse();
            }
            else
            {   
                // HIDE MOUSE
                window.setMouseCursorVisible(false);
                Input.LockMouse(window.getSize().x / 2, window.getSize().y / 2);
            }
        }

        // BASIC CAMERA MOVEMENT
        if (Input.GetKey(KeyCode::W)) camera.position += moveSpeed * camera.Forward() * global.FRAME_TIME;
        if (Input.GetKey(KeyCode::A)) camera.position -= moveSpeed * camera.Right() * global.FRAME_TIME;
        if (Input.GetKey(KeyCode::S)) camera.position -= moveSpeed * camera.Forward() * global.FRAME_TIME;
        if (Input.GetKey(KeyCode::D)) camera.position += moveSpeed * camera.Right() * global.FRAME_TIME;
        if (Input.GetKey(KeyCode::E)) camera.position += moveSpeed * glm::vec3(0.0f, 1.0f, 0.0f) * global.FRAME_TIME;
        if (Input.GetKey(KeyCode::Q)) camera.position -= moveSpeed * glm::vec3(0.0f, 1.0f, 0.0f) * global.FRAME_TIME;

        // BASIC CAMERA LOOK
        if (Input.MouseHidden())
        {
            float dX = Input.MouseDeltaX() * lookSensitivity * global.FRAME_TIME;
            float dY = Input.MouseDeltaY() * lookSensitivity * global.FRAME_TIME;

            camera.rotation.y += dX;
            camera.rotation.x += dY;
            camera.rotation.x = std::max(-89.0f, std::min(89.0f, camera.rotation.x));
        }
        camera.UpdateProjectionView(); 



        // CHUNK COORDINATES THAT BOUND THE PLAYER
        int renderDistanceH = 8;
        int renderDistanceV = 5;
        int width = 16;
        int height = 16;
        int CenterChunkX = static_cast<int>(std::floor(camera.position.x / width) * width);
        int CenterChunkY = static_cast<int>(std::floor(camera.position.y / height) * height);
	    int CenterChunkZ = static_cast<int>(std::floor(camera.position.z / width) * width);
	    int offsetH = (renderDistanceH - 1) * width / 2;
	    int offsetV = (renderDistanceV - 1) * height / 2;


        // GENERATE 1 CHUNK PER FRAME
        bool generatedChunkThisFrame = false;

        for (int y = 0; y < renderDistanceV; ++y) {
            for (int x = 0; x < renderDistanceH; ++x) {
                for (int z = 0; z < renderDistanceH; ++z) {
                    int worldX = x * width - offsetH + CenterChunkX;
                    int worldY = y * height - offsetV + CenterChunkY;
                    int worldZ = z * width - offsetH + CenterChunkZ;

                    // Stop if chunk was generated this frame
                    if (generatedChunkThisFrame) break;

                    bool chunkPresent = false;

                    // Check if there is a chunk at the position
                    for (int i = 0; i < chunks.size(); ++i) {

                        // 1 - check if chunk exists at current iteration position
                        //if (chunks[i].x == worldX && chunks[i].y == worldY && chunks[i].z == worldZ) {
                        if (chunks[i].x == worldX && chunks[i].y == worldY && chunks[i].z == worldZ) {
                            chunkPresent = true;
                            break;
                        }
                    }


                    // No chunk present? Create a new one
                    if (!chunkPresent){
                        model = terrainGPU.ConstructMeshGPU(worldX, worldY, worldZ);
                        models.push_back(model);
                        Chunk newChunk;
                        newChunk.x = worldX;
                        newChunk.y = worldY;
                        newChunk.z = worldZ;
                        chunks.push_back(newChunk);
                        generatedChunkThisFrame = true;
                    }
                }
            }
        }


        // Remove one chunks
        if (generatedChunkThisFrame)
        {
            for (int i=0; i<chunks.size(); ++i)
            {
                
                int chunkDist = std::max(std::max(std::abs(chunks[i].x - CenterChunkX), std::abs(chunks[i].z - CenterChunkZ)), std::abs(chunks[i].y - CenterChunkY));
                if (chunkDist > std::floor((renderDistanceH * width) / 2.0)){
                    chunks.erase(chunks.begin() + i);
			        models.erase(models.begin() + i);
                    break;
                } 
            }
        }
















        // // test terrain raycast
        // if (Input.MouseDown())
        // {
        //     RayHit hit = terrainGPU.Raycast(camera.position, camera.Forward());
        //     if (hit.hit) 
        //     {
        //         std::cout << "Fired ray, hit position: " << hit.position.x << " " <<  hit.position.y << " " << hit.position.z << std::endl;
        //         glm::vec3 p = hit.position - camera.Forward() * 0.5f;

        //         terrainGPU.AddDensity(hit.position, -0.02f);

        //         // REGENERATE MODEL
        //         model = terrainGPU.ConstructMeshGPU(10, 0, 0);
        //         models.clear();
        //         models.push_back(model);
        //     }
        // }

        // if (Input.GetKey(KeyCode::Space))
        // {
        //     RayHit hit = terrainGPU.Raycast(camera.position, camera.Forward());
        //     if (hit.hit) 
        //     {
        //         std::cout << "Fired ray, hit position: " << hit.position.x << " " <<  hit.position.y << " " << hit.position.z << std::endl;
        //         glm::vec3 p = hit.position - camera.Forward() * 0.5f;

        //         terrainGPU.AddDensity(hit.position, 0.02f);

        //         // REGENERATE MODEL
        //         model = terrainGPU.ConstructMeshGPU(10, 0, 0);
        //         models.clear();
        //         models.push_back(model);
        //     }
        // }





        // RENDER PIPELINE
        renderPipeline.Render(models, camera);

        // DISPLAY RENDERED FRAME
        window.display();

        // FRAME TIME CALCULATION
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
        global.FRAME_TIME = duration.count();

        Debug::EndTimer();
    }

    return EXIT_SUCCESS;
}