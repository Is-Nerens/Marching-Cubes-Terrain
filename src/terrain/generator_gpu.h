#pragma once
#include <GL/glew.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include "../mesh.h"
#include "../shader.h"
#include "tables.h"

typedef struct GeneratorGPU {
    GLuint computeShaderProgram;
    int densityThresholdLocation;
    int chunkXLocation;
    int chunkYLocation;
    int chunkZLocation;
    int cubesLocation;
    int partitionSubdivisionsLocation;
    GLuint triTableBuffer;
    GLuint vertexBuffer;
    GLuint partitionOccupancyBuffer;
    int chunkSize;
} GeneratorGPU;

void GeneratorGPUInit(GeneratorGPU* generator, int chunkSize)
{
    generator->computeShaderProgram = Create_Compute_Shader_Program_From_File("shaders/generate.glsl");
    generator->densityThresholdLocation = glGetUniformLocation(generator->computeShaderProgram, "densityThreshold");
    generator->chunkXLocation = glGetUniformLocation(generator->computeShaderProgram, "chunkX");
    generator->chunkYLocation = glGetUniformLocation(generator->computeShaderProgram, "chunkY");
    generator->chunkZLocation = glGetUniformLocation(generator->computeShaderProgram, "chunkZ");
    generator->cubesLocation = glGetUniformLocation(generator->computeShaderProgram, "cubes");
    generator->partitionSubdivisionsLocation = glGetUniformLocation(generator->computeShaderProgram, "partitionSubdivisions");
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

    // partition occupancy buffer
    glGenBuffers(1, &generator->partitionOccupancyBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, generator->partitionOccupancyBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 64, NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, generator->partitionOccupancyBuffer);
}

void GeneratorGPUFree(GeneratorGPU* generator)
{
    glDeleteBuffers(1, &generator->vertexBuffer);
    glDeleteBuffers(1, &generator->triTableBuffer);
    glDeleteBuffers(1, &generator->partitionOccupancyBuffer);
    generator->densityThresholdLocation = 0;
    generator->chunkXLocation = 0;
    generator->chunkYLocation = 0;
    generator->chunkZLocation = 0;
}

struct Slice {
    int start;
    int numFloats;
};

void GeneratorGPUGenerateChunk(GeneratorGPU* generator, Mesh* mesh, float x, float y, float z)
{
    int cubes = generator->chunkSize * generator->chunkSize * generator->chunkSize;
    MeshClearCPU(mesh);
    glUseProgram(generator->computeShaderProgram);
    glUniform1f(generator->densityThresholdLocation, 0.7f);
    glUniform1f(generator->chunkXLocation, x);
    glUniform1f(generator->chunkYLocation, y);
    glUniform1f(generator->chunkZLocation, z);
    glUniform1i(generator->cubesLocation, cubes);
    glUniform1i(generator->partitionSubdivisionsLocation, 3);
    uint8_t zeros[64] = {0};
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, generator->partitionOccupancyBuffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(zeros), zeros); // clear partition bits
    int workGroupSizeX = 8;
    int workGroupSizeY = 8;
    int workGroupSizeZ = 8;
    int workGroupsX = generator->chunkSize / workGroupSizeX;
    int workGroupsY = generator->chunkSize / workGroupSizeY;
    int workGroupsZ = generator->chunkSize / workGroupSizeZ; 
    glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    glFinish();

    // get pointer to partition occupancy buffer
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, generator->partitionOccupancyBuffer); 
    GLint* partitionOccupancyFlags = (GLint*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);

    // get pointer to raw vertex data output
    int vertexCount = generator->chunkSize * generator->chunkSize * generator->chunkSize * 48;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, generator->vertexBuffer); 
    GLfloat* rawVertexData = (GLfloat*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);

    // generate mesh
    int partitions = (int)pow(8.0, (double)3);
    int cubesPerPartition = cubes / partitions;
    int floatsPerCube = cubesPerPartition * 48;
    for (int i=0; i<partitions; i++) 
    {
        if ((partitionOccupancyFlags[i >> 5] >> (31 - (i & 31))) & 1) // if partition contains mesh data
        {
            GLfloat* slicePtr = &rawVertexData[i * floatsPerCube];

            // loop over 48 floats at a time (48 floats are allocated to each cube)
            for (int fl=0; fl<floatsPerCube; fl+=48) {   

                // for each face f
                for (int fa=0; fa<4; fa++) {
                    float* faceData = &slicePtr[fl + fa * 12];
                    if (faceData[0] == 0.0f) break;
                    MeshChunkGenerateAddFace(mesh, faceData);
                }
            }
        }
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, generator->vertexBuffer);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, generator->partitionOccupancyBuffer);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    mesh->x = x;
    mesh->y = y;
    mesh->z = z;
}