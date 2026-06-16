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
    Hashmap editDensities;
    struct Array generationQueue;
} Terrain;

void TerrainInit(Terrain* terrain, int chunkSize)
{
    terrain->renderDistH = 3;
    terrain->renderDistV = 3;
    terrain->chunkSize = chunkSize;
    uint32_t chunkCapacity = (terrain->renderDistH * 2 - 1) * (terrain->renderDistH * 2 - 1) * (terrain->renderDistV * 2 - 1);
    MeshArray_Init(&terrain->chunks, chunkCapacity, sizeof(Pos));
    HashmapInit(&terrain->editDensities, sizeof(Pos), sizeof(struct Array), chunkCapacity);
    Array_Init(&terrain->generationQueue, sizeof(Pos), 4);
}

void TerrainFree(Terrain* terrain)
{
    MeshArray_Free(&terrain->chunks);
    HashmapFree(&terrain->editDensities);
    Array_Free(&terrain->generationQueue);
}

void TerrainAddDensity(Terrain* terrain, float x, float y, float z, int radius, float amount)
{
    // snap position to grid
    int snapWorldX = roundf(x);
    int snapWorldY = roundf(y);
    int snapWorldZ = roundf(z);
    vec3 snapVec = { snapWorldX, snapWorldY, snapWorldZ };

    // for each chunk
    for (uint32_t i=0; i<terrain->chunks.size; i++)
    {
        Mesh* mesh = MeshArray_Get(&terrain->chunks, i);
        Pos meshPos = { 
            mesh->x / terrain->chunkSize, 
            mesh->y / terrain->chunkSize, 
            mesh->z / terrain->chunkSize
        };
        Array* editDensities = HashmapGet(&terrain->editDensities, &meshPos);

        bool regenerateMesh = false;

        // for each corner
        for (int x = snapWorldX - radius + 1; x < snapWorldX + radius; x++) {
        for (int y = snapWorldY - radius + 1; y < snapWorldY + radius; y++) {
        for (int z = snapWorldZ - radius + 1; z < snapWorldZ + radius; z++) 
        {
            int cornerLocalX = x - mesh->x;
            int cornerLocalY = y - mesh->y;
            int cornerLocalZ = z - mesh->z;

            // if corner position is inside chunk
            if (cornerLocalX >= 0 && cornerLocalX <= terrain->chunkSize &&
                cornerLocalY >= 0 && cornerLocalY <= terrain->chunkSize &&
                cornerLocalZ >= 0 && cornerLocalZ <= terrain->chunkSize)
            {
                // If dist from snap pos to corner <= radius
                vec3 pos = { x, y, z };
                float dist = glm_vec3_distance(snapVec, pos);
                int densityIndex = cornerLocalX + (cornerLocalY * (terrain->chunkSize + 1)) + (cornerLocalZ * (terrain->chunkSize + 1) * (terrain->chunkSize + 1));
                float* val = Array_Get(editDensities, densityIndex);
                float falloff = 1.0f - (dist / radius);
                *val += amount * falloff;
                if (*val > 2.0f) *val = 2.0f;
                if (*val < -2.0f) *val = -2.0f;
                regenerateMesh = true;
            }
        }  
        }  
        }

        if (regenerateMesh) {
            Array_Push(&terrain->generationQueue, &meshPos);
        }
    }
}

void SearchForEmptyChunks(Terrain* terrain, float cameraX, float cameraY, float cameraZ)
{
    if (terrain->generationQueue.size == terrain->generationQueue.capacity) {
        return;
    }

    int centerX, centerY, centerZ;
    centerX = floorf(cameraX / terrain->chunkSize);
    centerY = floorf(cameraY / terrain->chunkSize);
    centerZ = floorf(cameraZ / terrain->chunkSize);
    
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

                    // create edit densities for the location
                    int editDensityCapacity = (terrain->chunkSize + 1) * (terrain->chunkSize + 1) * (terrain->chunkSize + 1);
                    Array editDensities;
                    Array_InitCalloc(&editDensities, sizeof(float), editDensityCapacity);
                    HashmapSet(&terrain->editDensities, &pos, &editDensities);

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

            void* found = HashmapGet(&terrain->editDensities, &pos);
            if (found != NULL) {
                Array* editDensities = (Array*)found;
                Array_Free(editDensities);
                HashmapDelete(&terrain->editDensities, &pos);
            }
        }
    }
}

void GenerateChunks(Terrain* terrain, GeneratorGPU* generator)
{
    for (int i=0; i<terrain->generationQueue.size; i++)
    {
        Pos* pos = (Pos*)Array_Get(&terrain->generationQueue, i);
        Mesh* mesh = MeshArray_KeyGet(&terrain->chunks, pos);
        void* found = HashmapGet(&terrain->editDensities, pos);
        if (mesh != NULL && found != NULL)
        {
            Array* editDensities = (Array*)found;
            GeneratorGPUGenerateChunk(
                generator, 
                editDensities, 
                mesh, 
                pos->x * terrain->chunkSize, 
                pos->y * terrain->chunkSize, 
                pos->z * terrain->chunkSize
            );
        }
    }
    Array_Clear(&terrain->generationQueue);
}