#pragma once
#include <GL/glew.h>
#include <stdint.h>
#include <stdlib.h>
#include "mesh.h"
#include "shader.h"
#include "tables.h"

typedef struct GeneratorGPU {
    GLuint computeShaderProgram;
    int densityThresholdLocation;
    int chunkXLocation;
    int chunkYLocation;
    int chunkZLocation;
    GLuint vertexBuffer;
    GLuint triTableBuffer;
    int chunkSize;
} GeneratorGPU;

void GeneratorGPUInit(GeneratorGPU* generator, int chunkSize)
{
    generator->computeShaderProgram = Create_Compute_Shader_Program_From_File("shaders/generate.glsl");
    generator->densityThresholdLocation = glGetUniformLocation(generator->computeShaderProgram, "densityThreshold");
    generator->chunkXLocation = glGetUniformLocation(generator->computeShaderProgram, "chunkX");
    generator->chunkYLocation = glGetUniformLocation(generator->computeShaderProgram, "chunkY");
    generator->chunkZLocation = glGetUniformLocation(generator->computeShaderProgram, "chunkZ");
    generator->chunkSize = chunkSize;
    
    // tritable buffer
    glGenBuffers(1, &generator->triTableBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, generator->triTableBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * 4096, TriTable, GL_STATIC_READ); 
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0,generator->triTableBuffer);
    
    // vertex buffer
    glGenBuffers(1, &generator->vertexBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, generator->vertexBuffer);
    int vertexBufferBytes = chunkSize * chunkSize * chunkSize * 48 * sizeof(float);
    glBufferData(GL_SHADER_STORAGE_BUFFER, vertexBufferBytes, NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, generator->vertexBuffer);
}

void GeneratorGPUFree(GeneratorGPU* generator)
{
    glDeleteBuffers(1, &generator->vertexBuffer);
    glDeleteBuffers(1, &generator->triTableBuffer);
    generator->densityThresholdLocation = 0;
    generator->chunkXLocation = 0;
    generator->chunkYLocation = 0;
    generator->chunkZLocation = 0;
}

void GeneratorGPUGenerateChunk(GeneratorGPU* generator, Mesh* mesh)
{
    MeshClearCPU(mesh);
    glUseProgram(generator->computeShaderProgram);
    glUniform1f(generator->densityThresholdLocation, 0.2f);
    glUniform1f(generator->chunkXLocation, 0.0f);
    glUniform1f(generator->chunkYLocation, 0.0f);
    glUniform1f(generator->chunkZLocation, 0.0f);
    int workGroupSizeX = 8;
    int workGroupSizeY = 8;
    int workGroupSizeZ = 8;
    int workGroupsX = generator->chunkSize / workGroupSizeX;
    int workGroupsY = generator->chunkSize / workGroupSizeY;
    int workGroupsZ = generator->chunkSize / workGroupSizeZ; 
    glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    glFinish();

    // get pointer to raw vertex data output
    int vertexCount = generator->chunkSize * generator->chunkSize * generator->chunkSize * 48;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, generator->vertexBuffer); 
    GLfloat* rawVertexData = (GLfloat*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);

    // construct mesh
    for (int i=0; i<vertexCount; i += 48)
    {
        for (int f=0; f<4; f++)
        {
            float* faceData = &rawVertexData[i + f * 12];
            if (faceData[0] > 99999.0f) break;
            MeshChunkGenerateAddFace(mesh, faceData);
        }
    }
}