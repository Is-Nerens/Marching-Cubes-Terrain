#pragma once
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>


typedef struct VertexMap {
    uint32_t* edgeX;
    uint32_t* edgeY;
    uint32_t* edgeZ;
    int size;
} VertexMap;

void VertexMapInit(VertexMap* map, int size)
{
    int edges = (size+1)*(size+1)*size;
    map->edgeX = (uint32_t*)malloc(edges * sizeof(uint32_t));
    map->edgeY = (uint32_t*)malloc(edges * sizeof(uint32_t));
    map->edgeZ = (uint32_t*)malloc(edges * sizeof(uint32_t));
    map->size = size;
    for (int i=0; i<edges; i++) {
        map->edgeX[i] = UINT32_MAX;
    }
    for (int i=0; i<edges; i++) {
        map->edgeY[i] = UINT32_MAX;
    }
    for (int i=0; i<edges; i++) {
        map->edgeZ[i] = UINT32_MAX;
    }
}

void VertexMapFree(VertexMap* map)
{
    free(map->edgeX);
    free(map->edgeY);
    free(map->edgeZ);
    map->edgeX = NULL;
    map->edgeY = NULL;
    map->edgeZ = NULL;
}

uint32_t VertexMapGetIndex(VertexMap* map, float x, float y, float z)
{
    if (x != (int)(x)) { // X-edge
        int ix = (int)(x) % map->size;
        int iy = (int)(y) % (map->size+1);
        int iz = (int)(z) % (map->size+1);
        int index = ix + iy * map->size + iz * map->size * (map->size+1);
        return map->edgeX[index];
    }
    else if (fabs(y - roundf(y)) > 0.001f) { // Y-edge
        int ix = (int)(y) % map->size;
        int iy = (int)(z) % (map->size+1);
        int iz = (int)(x) % (map->size+1);
        int index = ix + iy * map->size + iz * map->size * (map->size+1);
        return map->edgeY[index];
    }
    else if (fabs(z - roundf(z)) > 0.001f) { // Z-edge
        int ix = (int)(z) % map->size;
        int iy = (int)(x) % (map->size+1);
        int iz = (int)(y) % (map->size+1);
        int index = ix + iy * map->size + iz * map->size * (map->size+1);
        return map->edgeZ[index];
    }
    return -1;
}

void VertexMapSetIndex(VertexMap* map, float x, float y, float z, uint32_t value)
{
    if (fabs(x - roundf(x)) > 0.001f) { // X-edge
        int ix = (int)(x) % map->size;
        int iy = (int)(y) % (map->size+1);
        int iz = (int)(z) % (map->size+1);
        int index = ix + iy * map->size + iz * map->size * (map->size+1);
        map->edgeX[index] = value;
        return;
    }
    else if (fabs(y - roundf(y)) > 0.001f) { // Y-edge
        int ix = (int)(y) % map->size;
        int iy = (int)(z) % (map->size+1);
        int iz = (int)(x) % (map->size+1);
        int index = ix + iy * map->size + iz * map->size * (map->size+1);
        map->edgeY[index] = value;
        return;
    }
    else if (fabs(z - roundf(z)) > 0.001f) { // Z-edge
        int ix = (int)(z) % map->size;
        int iy = (int)(x) % (map->size+1);
        int iz = (int)(y) % (map->size+1);
        int index = ix + iy * map->size + iz * map->size * (map->size+1);
        map->edgeZ[index] = value;
        return;
    }
}