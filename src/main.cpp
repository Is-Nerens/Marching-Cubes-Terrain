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
    camera.position = {2.0f, 28.0f, 8.0f};
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
    Model model = terrainGPU.ConstructMeshGPU(0, 0, 0);
    models.push_back(model);


    float moveSpeed = 10.0f;
    float lookSensitivity = 15.0f;




    // MAIN UPDATE LOOP
    while (window.isOpen()) {
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







        if (Input.GetKey(KeyCode::Space))
        {
            // REGENERATE MODEL
            model = terrainGPU.ConstructMeshGPU(0, 0, 0);
            models.clear();
            models.push_back(model);
        }





        // RENDER PIPELINE
        renderPipeline.Render(models, camera);

        // DISPLAY RENDERED FRAME
        window.display();

        // FRAME TIME CALCULATION
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
        global.FRAME_TIME = duration.count();
    }

    return EXIT_SUCCESS;
}