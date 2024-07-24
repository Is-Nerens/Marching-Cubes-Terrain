#pragma once
#define STB_IMAGE_IMPLEMENTATION

#include <GL/glew.h>
#include <malloc.h>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "model.h"
#include "error.h"
// #include "texture.h"
#include "camera.h"
#include "vendor/glm/glm.hpp"
#include "vendor/glm/gtc/matrix_transform.hpp"
#include "vendor/glm/gtc/type_ptr.hpp"
#include "vendor/stb_image.h"


class RenderPipeline {
public:
    RenderPipeline() {}

    void Init()
    {   
        // Load shaders from file
        std::string vertexShaderSource = LoadShader("./shaders/shader.vert");
        std::string fragmentShaderSource = LoadShader("./shaders/shader.frag");

        shaderProgram = CreateShader(vertexShaderSource, fragmentShaderSource);
        glUseProgram(shaderProgram);


    

        // passing data to the shader via uniforms
        int location = glGetUniformLocation(shaderProgram, "u_Colour");
        glUniform4f(location, 0.74f, 0.43f, 0.439f, 1.0f);
    




        int width, height, bpp;
        stbi_set_flip_vertically_on_load(1);

        // ROCK TEXTURE ////////////////////////
        std::string path1 = "textures/rock_1_albedo.jpg";
        unsigned char* localbuffer1 = stbi_load(path1.c_str(), &width, &height, &bpp, 4);
        glGenTextures(1, &texture1);
        glBindTexture(GL_TEXTURE_2D, texture1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, localbuffer1);
        stbi_image_free(localbuffer1);

        // ROCK NORMAL TEXTURE /////////////////////
        std::string path2 = "textures/rock_1_normal.jpg";
        unsigned char* localbuffer2 = stbi_load(path2.c_str(), &width, &height, &bpp, 4);
        glGenTextures(1, &texture2);
        glBindTexture(GL_TEXTURE_2D, texture2);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, localbuffer2);
        stbi_image_free(localbuffer2);

        // GRASS TEXTURE /////////////////////
        std::string path3 = "textures/grass_2_albedo.jpg";
        unsigned char* localbuffer3 = stbi_load(path3.c_str(), &width, &height, &bpp, 4);
        glGenTextures(1, &texture3);
        glBindTexture(GL_TEXTURE_2D, texture3);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, localbuffer3);
        stbi_image_free(localbuffer3);

        // GRASS NORMAL TEXTURE /////////////////////
        std::string path4 = "textures/grass_2_normal.jpg";
        unsigned char* localbuffer4 = stbi_load(path4.c_str(), &width, &height, &bpp, 4);
        glGenTextures(1, &texture4);
        glBindTexture(GL_TEXTURE_2D, texture4);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, localbuffer4);
        stbi_image_free(localbuffer4);

        glUniform1i(glGetUniformLocation(shaderProgram, "u_rock_albedo_texture"), 0); 
        glUniform1i(glGetUniformLocation(shaderProgram, "u_rock_normal_texture"), 1); 
        glUniform1i(glGetUniformLocation(shaderProgram, "u_grass_albedo_texture"), 2); 
        glUniform1i(glGetUniformLocation(shaderProgram, "u_grass_normal_texture"), 3); 









        
        MVP_UniformLocation = glGetUniformLocation(shaderProgram, "u_MVP");
        modelMatrixUniformLocation = glGetUniformLocation(shaderProgram, "u_ModelPositionMatrix");
        cameraPosUniformLocation = glGetUniformLocation(shaderProgram, "u_CameraPos");

        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
        glEnable(GL_DEPTH_TEST);
        glClearDepth(1.0f);
        glDepthRange(0.0f, 1.0f);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
    }

    void Render(std::vector<Model*> &models, Camera &camera)
    {   
        glUseProgram(shaderProgram);
        
        // sky colour
        glClearColor(0.53f, 0.81f, 0.92f, 1.0f);

        // Clear the colour and depth buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        
        for (int i = 0; i < models.size(); ++i)
        {   
            // CHUNK HAS NO VERTICES
            if (models[i]->VertexCount() == 0) continue;

            // APPLY FRUSTUM CULLING
            if (!InFrustum(*models[i], camera.GetProjectionViewMatrix())) continue;

            // transform uniforms
            glm::mat4 modelMat = glm::translate(glm::mat4(1.0f), models[i]->position);
            glm::mat4 mvp = camera.GetProjectionViewMatrix() * modelMat;
            glm::vec3 cameraPos = camera.position; 

            if (MVP_UniformLocation != -1) glUniformMatrix4fv(MVP_UniformLocation, 1, GL_FALSE, &mvp[0][0]);
            else std::cerr << "Failed to locate uniform u_MVP in shader program" << std::endl;

            if (modelMatrixUniformLocation != -1) glUniformMatrix4fv(modelMatrixUniformLocation, 1, GL_FALSE, &modelMat[0][0]);
            else std::cerr << "Failed to locate uniform u_ModelPositionMatrix in shader program" << std::endl;
            
            if (cameraPosUniformLocation != -1) glUniform3fv(cameraPosUniformLocation, 1, glm::value_ptr(cameraPos));
            else std::cerr << "Failed to locate uniform u_CameraPos in shader program" << std::endl;

            




            // Vertex buffer object
            unsigned int VBO;
            glGenBuffers(1, &VBO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, models[i]->vertices.size() * sizeof(float), models[i]->vertices.data(), GL_STATIC_DRAW);

            // Vertex array object 
            unsigned int VAO;
            glGenVertexArrays(1, &VAO);
            glBindVertexArray(VAO);
            // Position attribute
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            // Normal attribute
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(1);
            // UV attribute
            // glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
            // glEnableVertexAttribArray(2);

            // Index buffer object 
            unsigned int IBO;
            glGenBuffers(1, &IBO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, models[i]->indices.size() * sizeof(unsigned int), models[i]->indices.data(), GL_STATIC_DRAW);

            // Bind Textures
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture1);
            glActiveTexture(GL_TEXTURE0 + 1);
            glBindTexture(GL_TEXTURE_2D, texture2);
            glActiveTexture(GL_TEXTURE0 + 2);
            glBindTexture(GL_TEXTURE_2D, texture3);
            glActiveTexture(GL_TEXTURE0 + 3);
            glBindTexture(GL_TEXTURE_2D, texture4);

            // Draw the model
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, models[i]->indices.size(), GL_UNSIGNED_INT, nullptr);

            // Clean up resources
            glDeleteBuffers(1, &VBO);
            glDeleteBuffers(1, &IBO);
            glDeleteVertexArrays(1, &VAO);
        }
    }

    ~RenderPipeline() {}

private:
    unsigned int CompileShader(unsigned int type, const std::string &source)
    {
        unsigned int id = glCreateShader(type);
        const char* src = source.c_str();
        glShaderSource(id, 1, &src, NULL);
        glCompileShader(id);

        // Error Handling
        int result;
        glGetShaderiv(id, GL_COMPILE_STATUS, &result);
        if (result == GL_FALSE)
        {
            int length;
            glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
            char* message = (char*)alloca(length * sizeof(char));
            glGetShaderInfoLog(id, length, &length, message);
            std::cout << "[Shader Compile Error] " << message << std::endl;
            glDeleteShader(id);
            return 0;
        }

        return id;
    }

    unsigned int CreateShader(const std::string& vertexShaderSource, const std::string& fragmentShaderSource)
    {
        unsigned int vertexShader = CompileShader(GL_VERTEX_SHADER, vertexShaderSource);
        unsigned int fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
        unsigned int shaderProgram = glCreateProgram();

        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);
        glValidateProgram(shaderProgram);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return shaderProgram;
    }

    std::string LoadShader(const std::string &filepath)
    {
        std::ifstream stream(filepath);
        if (!stream.is_open()) {
            throw std::runtime_error("Failed to find shader file: " + filepath);
        }
        std::stringstream buffer;
        buffer << stream.rdbuf(); 
        return buffer.str();
    }

    bool InFrustum(Model& model, const glm::mat4& ProjectView) {
        if (!model.boundingBox.isFilled) return true;
    
        std::vector<Plane> planes = ExtractFrustumPlanes(ProjectView);

        return IsBoxInFrustum(planes, model.boundingBox.min, model.boundingBox.max);
    }


    unsigned int shaderProgram;
    int MVP_UniformLocation;
    int modelMatrixUniformLocation;
    int cameraPosUniformLocation;

    unsigned int texture1, texture2, texture3, texture4;
};
