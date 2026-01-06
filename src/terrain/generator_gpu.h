#pragma once
#include <GL/glew.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include "../mesh.h"
#include "../shader.h"
#include "../array.h"
#include "tables.h"
#include "vertex_map.h"

typedef struct GeneratorGPU {
    GLuint computeShaderProgram;
    int densityThresholdLocation;
    int chunkXLocation;
    int chunkYLocation;
    int chunkZLocation;
    int cubesLocation;
    int partitionSubdivisionsLocation;
    GLuint triTableBuffer;
    GLuint edgeTable;
    GLuint vertexBuffer;
    GLuint editDensityBuffer;
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

    // edge buffer
    glGenBuffers(1, &generator->edgeTable);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, generator->edgeTable);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * 256, edges, GL_STATIC_READ); 
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, generator->edgeTable);
    
    // vertex buffer
    glGenBuffers(1, &generator->vertexBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, generator->vertexBuffer);
    // number of cubes * faces per cube(4) * vertices per face (3) * floats per vertex(x,y,z,nx,ny,nz) * sizeof(float)
    int vertexBufferBytes = chunkSize * chunkSize * chunkSize * 5 * 3 * 6 * sizeof(float);
    glBufferData(GL_SHADER_STORAGE_BUFFER, vertexBufferBytes, NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, generator->vertexBuffer);

    // edit density buffer
    glGenBuffers(1, &generator->editDensityBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, generator->editDensityBuffer);
    int editDensityFloatCount = (generator->chunkSize + 1) * (generator->chunkSize + 1) * (generator->chunkSize + 1);
    glBufferData(GL_SHADER_STORAGE_BUFFER, editDensityFloatCount * sizeof(float), NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, generator->editDensityBuffer);

    // partition occupancy buffer
    glGenBuffers(1, &generator->partitionOccupancyBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, generator->partitionOccupancyBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 64, NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, generator->partitionOccupancyBuffer);
}

void GeneratorGPUFree(GeneratorGPU* generator)
{
    glDeleteBuffers(1, &generator->vertexBuffer);
    glDeleteBuffers(1, &generator->triTableBuffer);
    glDeleteBuffers(1, &generator->edgeTable);
    glDeleteBuffers(1, &generator->editDensityBuffer);
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

void GeneratorGPUGenerateChunk(GeneratorGPU* generator, Array* editDensities, Mesh* mesh, float x, float y, float z)
{
    // Set uniforms
    int cubes = generator->chunkSize * generator->chunkSize * generator->chunkSize;
    glUseProgram(generator->computeShaderProgram);
    glUniform1f(generator->densityThresholdLocation, 0.0f);
    glUniform1f(generator->chunkXLocation, x);
    glUniform1f(generator->chunkYLocation, y);
    glUniform1f(generator->chunkZLocation, z);
    glUniform1i(generator->cubesLocation, cubes);
    glUniform1i(generator->partitionSubdivisionsLocation, 3);

    // Clear partition bits
    uint8_t zeros[64] = {0};
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, generator->partitionOccupancyBuffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(zeros), zeros);

    // Upload edit density volume
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, generator->editDensityBuffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, editDensities->capacity * sizeof(float), editDensities->data);

    // Run compute shader
    int workGroupSizeX = 8;
    int workGroupSizeY = 8;
    int workGroupSizeZ = 8;
    int workGroupsX = generator->chunkSize / workGroupSizeX;
    int workGroupsY = generator->chunkSize / workGroupSizeY;
    int workGroupsZ = generator->chunkSize / workGroupSizeZ; 
    glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    glFinish();

    // Clear mesh
    MeshClearCPU(mesh);

    // get pointer to partition occupancy buffer
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, generator->partitionOccupancyBuffer); 
    GLint* partitionOccupancyFlags = (GLint*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);

    // get pointer to raw vertex data output
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, generator->vertexBuffer); 
    GLfloat* rawVertexData = (GLfloat*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);

    // Create vertex map
    VertexMap vmap;
    VertexMapInit(&vmap, generator->chunkSize);

    // Generate mesh
    int partitions = (int)pow(8.0, (double)3);
    int cubesPerPartition = cubes / partitions;
    int floatsPerCube = cubesPerPartition * 90;
    for (int i=0; i<partitions; i++) 
    {
        if ((partitionOccupancyFlags[i >> 5] >> (31 - (i & 31))) & 1) // if partition contains mesh data
        {
            GLfloat* slicePtr = &rawVertexData[i * floatsPerCube];

            // loop over 90 floats at a time (90 floats are allocated to each cube)
            for (int fl=0; fl<floatsPerCube; fl+=90) {   

                // for each face f
                for (int fa=0; fa<5; fa++) {

                    float* faceData = &slicePtr[fl + fa * 18];
                    if (faceData[0] == -1.0f) break;

                    if (mesh->vertexCount + 3 > mesh->vertexCapacity) {
                        MeshGrowVertexCapacity(mesh);
                    }

                    if (mesh->indexCount + 3 > mesh->indexCapacity) {
                        MeshGrowIndexCapacity(mesh);
                    }

                    // vertex 1
                    if (VertexMapGetIndex(&vmap, faceData[0], faceData[1], faceData[2]) == UINT32_MAX) {
                        VertexMapSetIndex(&vmap, faceData[0], faceData[1], faceData[2], mesh->vertexCount);
                        MeshAddIndex(mesh, mesh->vertexCount);
                        MeshAddVertex(mesh, faceData[0], faceData[1], faceData[2], faceData[3], faceData[4], faceData[5]);
                    }
                    else {
                        MeshAddIndex(mesh, VertexMapGetIndex(&vmap, faceData[0], faceData[1], faceData[2]));
                    }

                    // vertex 2
                    if (VertexMapGetIndex(&vmap, faceData[6], faceData[7], faceData[8]) == UINT32_MAX) {
                        VertexMapSetIndex(&vmap, faceData[6], faceData[7], faceData[8], mesh->vertexCount);
                        MeshAddIndex(mesh, mesh->vertexCount);
                        MeshAddVertex(mesh, faceData[6], faceData[7], faceData[8], faceData[9], faceData[10], faceData[11]);
                    }
                    else {
                        MeshAddIndex(mesh, VertexMapGetIndex(&vmap, faceData[6], faceData[7], faceData[8]));
                    }

                    // vertex 3
                    if (VertexMapGetIndex(&vmap, faceData[12], faceData[13], faceData[14]) == UINT32_MAX) {
                        VertexMapSetIndex(&vmap, faceData[12], faceData[13], faceData[14], mesh->vertexCount);
                        MeshAddIndex(mesh, mesh->vertexCount);
                        MeshAddVertex(mesh, faceData[12], faceData[13], faceData[14], faceData[15], faceData[16], faceData[17]);
                    }
                    else {
                        MeshAddIndex(mesh, VertexMapGetIndex(&vmap, faceData[12], faceData[13], faceData[14]));
                    }
                }
            }
        }
    }

    // Free and unbind
    VertexMapFree(&vmap);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, generator->vertexBuffer);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, generator->partitionOccupancyBuffer);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    // Set mesh position
    mesh->x = x;
    mesh->y = y;
    mesh->z = z;
    mesh->aabb.minX = x;
    mesh->aabb.maxX = x + generator->chunkSize;
    mesh->aabb.minY = y;
    mesh->aabb.maxY = y + generator->chunkSize;
    mesh->aabb.minZ = z;
    mesh->aabb.maxZ = z + generator->chunkSize;
}