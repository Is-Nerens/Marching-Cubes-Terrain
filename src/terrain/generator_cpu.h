#pragma once
#include "../mesh.h"
#include "tables.h"
#include "perlin.h"

float GetDensity(float x, float y, float z)
{
    return Perlin3D(x * 0.1f + 0.01f, y * 0.1f + 0.01f, z * 0.1f + 0.01f);
}

void VertexInterp(vec3 p1, vec3 p2, float density1, float density2, float densityThreshold, vec3 result)
{
    float t = (densityThreshold - density1) / (density2 - density1);
    result[0] = p1[0] + t * (p2[0] - p1[0]);
    result[1] = p1[1] + t * (p2[1] - p1[1]);
    result[2] = p1[2] + t * (p2[2] - p1[2]);
}

void CalculateNormal(vec3 v1, vec3 v2, vec3 v3, vec3 normal)
{
    vec3 vect1, vect2;
    glm_vec3_sub(v2, v1, vect1);
    glm_vec3_sub(v3, v1, vect2);
    if (glm_vec3_norm(vect1) < 0.00001f || glm_vec3_norm(vect2) < 0.00001f) {
        glm_vec3_zero(normal);
        return;
    }
    glm_vec3_cross(vect1, vect2, normal);
    glm_vec3_normalize(normal);
}

void GenerateChunk(Mesh* mesh, int size)
{
    MeshClearCPU(mesh);

    float densityThreshold = 0.5f;

    for (int x=0; x<size; x++) {
    for (int y=0; y<size; y++) {
    for (int z=0; z<size; z++) {

        // Create corner positions
        vec3 corners[8] = {
            {x, y, z + 1},
            {x + 1, y, z + 1},
            {x + 1, y, z},
            {x, y, z},
            {x, y + 1, z + 1},
            {x + 1, y + 1, z + 1},
            {x + 1, y + 1, z},
            {x, y + 1, z}
        };

        // Get corner densities
        float cornerDensities[8];
        cornerDensities[0] = GetDensity(x, y, z + 1);
        cornerDensities[1] = GetDensity(x + 1, y, z + 1);
        cornerDensities[2] = GetDensity(x + 1, y, z);
        cornerDensities[3] = GetDensity(x, y, z);
        cornerDensities[4] = GetDensity(x, y + 1, z + 1);
        cornerDensities[5] = GetDensity(x + 1, y + 1, z + 1);
        cornerDensities[6] = GetDensity(x + 1, y + 1, z);
        cornerDensities[7] = GetDensity(x, y + 1, z);

        // Calculate cube index
        int cubeIndex = 0;
        for (int i=0; i<8; i++) {
            cubeIndex |= ((cornerDensities[i] > densityThreshold) << i);
        }

        // Fill cube vertices
        int i=0; 
        while(TriTable[cubeIndex][i] != -1)
        {
            int a0 = cornerIndexAFromEdge[TriTable[cubeIndex][i]];
            int b0 = cornerIndexBFromEdge[TriTable[cubeIndex][i]];
            int a1 = cornerIndexAFromEdge[TriTable[cubeIndex][i+1]];
            int b1 = cornerIndexBFromEdge[TriTable[cubeIndex][i+1]];
            int a2 = cornerIndexAFromEdge[TriTable[cubeIndex][i+2]];
            int b2 = cornerIndexBFromEdge[TriTable[cubeIndex][i+2]];
            vec3 v1, v2, v3, normal;
            VertexInterp(corners[a0], corners[b0], cornerDensities[a0], cornerDensities[b0], densityThreshold, v1);
            VertexInterp(corners[a1], corners[b1], cornerDensities[a1], cornerDensities[b1], densityThreshold, v2);
            VertexInterp(corners[a2], corners[b2], cornerDensities[a2], cornerDensities[b2], densityThreshold, v3);
            CalculateNormal(v1, v2, v3, normal);
            MeshAddFace(mesh, v1, v2, v3, normal);
            i+=3;
        }
    }
    }
    }
}

