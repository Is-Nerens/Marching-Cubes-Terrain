#pragma once

#include <GL/glew.h>
#include <string>
#include <iostream>
#include "error.h"

int BindUniformFloat1(unsigned int shaderProgram, std::string uniformName, float value)
{
    int loc = glGetUniformLocation(shaderProgram, uniformName.c_str());
    if (loc != -1) glUniform1f(loc, value);
    else std::cerr << "[BindUniformFloat1] Error: Failed to locate uniform '" << uniformName << "' in shader" << std::endl;
    return loc;
}

int BindUniformInt1(unsigned int shaderProgram, std::string uniformName, int value)
{
    int loc = glGetUniformLocation(shaderProgram, uniformName.c_str());
    if (loc != -1) glUniform1i(loc, value);
    else std::cerr << "[BindUniformInt1] Error: Failed to locate uniform '" << uniformName << "' in shader" << std::endl;
    return loc;
}

void SetUniform1i(const std::string& name, int value, unsigned int shaderProgram)
{
    glUniform1i(glGetUniformLocation(shaderProgram, "u_albedo_texture"), value);
}

int GetShaderProgramInUse()
{
    int currentProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    std::cout << "Currently bound shader program ID: " << currentProgram << std::endl;
    return currentProgram;
}


std::string LoadShader(const std::string &filepath)
{
    std::ifstream stream(filepath);
    if (!stream.is_open()) {
        throw std::runtime_error("[LoadShader] Failed to find shader file: " + filepath);
    }
    std::stringstream buffer;
    buffer << stream.rdbuf(); 
    return buffer.str();
}

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

unsigned int CreateComputeShader(const std::string& computeShaderSource)
{
    unsigned int computeShader = CompileShader(GL_COMPUTE_SHADER, computeShaderSource);
    unsigned int computeShaderProgram = glCreateProgram();
    glAttachShader(computeShaderProgram, computeShader);
    glLinkProgram(computeShaderProgram);
    //glValidateProgram(computeShaderProgram);
    //glDeleteShader(computeShader);
    return computeShaderProgram;
}

