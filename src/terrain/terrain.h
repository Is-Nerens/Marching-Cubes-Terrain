#pragma once
#include "generator_gpu.h"
#include "../array.h"
#include "../mesh.h"
#include "mesh_array.h"
#include <math.h>

typedef struct Pos {
    int x, y, z;
} Pos;

typedef struct Terrain {
    int renderDistH;
    int renderDistV;
    int chunkSize;
    MeshArray chunks;
    struct Array generationQueue;
} Terrain;

void TerrainInit(Terrain* terrain, int chunkSize)
{
    terrain->renderDistH = 3;
    terrain->renderDistV = 3;
    terrain->chunkSize = chunkSize;
    uint32_t chunkCapacity = (terrain->renderDistH * 2 - 1) * (terrain->renderDistH * 2 - 1) * (terrain->renderDistV * 2 - 1);
    MeshArray_Init(&terrain->chunks, chunkCapacity, sizeof(Pos));
    Array_Init(&terrain->generationQueue, sizeof(Pos), 4);
}

void TerrainFree(Terrain* terrain)
{
    MeshArray_Free(&terrain->chunks);
    Array_Free(&terrain->generationQueue);
}

void SearchForEmptyChunks(Terrain* terrain, float cameraX, float cameraY, float cameraZ)
{
    int centerX, centerY, centerZ;
    centerX = floorf(cameraX / terrain->chunkSize);
    centerY = floorf(cameraY / terrain->chunkSize);
    centerZ = floorf(cameraZ / terrain->chunkSize);

    Array_Clear(&terrain->generationQueue);
    
    bool _exit = false;
    for (int x=centerX - terrain->renderDistH + 1; x < centerX + terrain->renderDistH; x++) {

        if (_exit) break;
        for (int z=centerZ - terrain->renderDistH + 1; z < centerZ + terrain->renderDistH; z++) {

            if (_exit) break;
            for (int y=centerY - terrain->renderDistV + 1; y < centerY + terrain->renderDistV; y++) {

                Pos pos = { x, y, z };
                if (!MeshArray_Contains(&terrain->chunks, &pos)) {

                    // create mesh for the location
                    Mesh* mesh = MeshArray_Create(&terrain->chunks, &pos);

                    // add position to generation queue
                    Array_Push(&terrain->generationQueue, &pos);

                    if (terrain->generationQueue.size >= terrain->generationQueue.capacity) {
                        _exit = true; break;
                    }
                }
            }
        }
    }
}

void DeleteDistantChunks(Terrain* terrain, float cameraX, float cameraY, float cameraZ)
{
    int centerX, centerY, centerZ;
    centerX = floorf(cameraX / terrain->chunkSize);
    centerY = floorf(cameraY / terrain->chunkSize);
    centerZ = floorf(cameraZ / terrain->chunkSize);

    for (int i=terrain->chunks.size-1; i>=0; i--)
    {
        Pos pos = *(Pos*)MeshArray_GetKey(&terrain->chunks, i);
        bool outX = pos.x < centerX - terrain->renderDistH + 1 || pos.x > centerX + terrain->renderDistH - 1; 
        bool outY = pos.y < centerY - terrain->renderDistV + 1 || pos.y > centerY + terrain->renderDistV - 1; 
        bool outZ = pos.z < centerZ - terrain->renderDistH + 1 || pos.z > centerZ + terrain->renderDistH - 1; 

        if (outX || outY || outZ) {
            MeshArray_Delete(&terrain->chunks, i);
        }
    }
}

void GenerateChunks(Terrain* terrain, GeneratorGPU* generator)
{
    for (int i=0; i<terrain->generationQueue.size; i++)
    {
        Pos* pos = (Pos*)Array_Get(&terrain->generationQueue, i);
        Mesh* mesh = MeshArray_KeyGet(&terrain->chunks, pos);
        if (mesh != NULL) {
            GeneratorGPUGenerateChunk(generator, mesh, pos->x * terrain->chunkSize, pos->y * terrain->chunkSize, pos->z * terrain->chunkSize);
        }
    }
}