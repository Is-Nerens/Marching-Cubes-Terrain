#pragma once

#include <GL/glew.h>
#include <malloc.h>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "model.h"
#include "texture.h"
#include "camera.h"
#include "vendor/glm/glm.hpp"
#include "vendor/glm/gtc/matrix_transform.hpp"
#include "vendor/glm/gtc/type_ptr.hpp"


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
    
        // pass texture to shader
        Texture texture("textures/rock.png");
        texture.Bind(0);
        albedoUniformLocation = glGetUniformLocation(shaderProgram, "u_TextureAlbedo");
        glUniform1i(albedoUniformLocation, 0); 
        
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

    void Render(std::vector<Model> &models, Camera &camera)
    {   
        glUseProgram(shaderProgram);
        
        // sky colour
        glClearColor(0.53f, 0.81f, 0.92f, 1.0f);

        // Clear the colour and depth buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        
        for (int i = 0; i < models.size(); ++i)
        {   
            // transform uniforms
            glm::mat4 modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
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
            glBufferData(GL_ARRAY_BUFFER, models[i].vertices.size() * sizeof(float), models[i].vertices.data(), GL_STATIC_DRAW);

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
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, models[i].indices.size() * sizeof(unsigned int), models[i].indices.data(), GL_STATIC_DRAW);

            // Draw the model
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, models[i].indices.size(), GL_UNSIGNED_INT, nullptr);

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

    unsigned int shaderProgram;
    int MVP_UniformLocation;
    int modelMatrixUniformLocation;
    int cameraPosUniformLocation;
    int albedoUniformLocation;
};
