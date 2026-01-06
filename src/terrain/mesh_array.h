#pragma once
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#include "../mesh.h"
#include "../hashmap.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>


typedef struct MeshArray {
    Mesh* meshes;
    Mesh* reserve;
    uint32_t size;
    uint32_t capacity;
    uint32_t reserveSize;
    uint32_t reserveCapacity;
    uint32_t keySize;
    Hashmap index;
    void* keys;
} MeshArray;

void MeshArray_Init(MeshArray* array, uint32_t capacity, uint32_t keySize)
{
    array->capacity = MAX(capacity, 2);
    array->reserveCapacity = 5;
    array->meshes = (Mesh*)malloc(sizeof(Mesh) * array->capacity);
    array->reserve = (Mesh*)malloc(array->reserveCapacity * sizeof(Mesh));
    array->keys = malloc(keySize * capacity);
    array->size = 0;
    array->reserveSize = 0;
    array->keySize = keySize;

    HashmapInit(&array->index, keySize, sizeof(uint32_t), array->capacity);
}

void MeshArray_Free(MeshArray* array)
{
    // free mesh memory
    for (uint32_t i=0; i<array->size; i++)
    {
        Mesh* mesh = &array->meshes[i];
        MeshFree(mesh);
    }
    for (uint32_t i=0; i<array->reserveSize; i++)
    {
        Mesh* mesh = &array->reserve[i];
        MeshFree(mesh);
    }

    free(array->meshes);
    free(array->reserve);
    free(array->keys);
    HashmapFree(&array->index);
    array->meshes = NULL;
    array->reserve = NULL;
    array->size = 0;
    array->capacity = 0;
    array->reserveSize = 0;
    array->reserveCapacity = 0;
}

void MeshArray_Push(MeshArray* array, Mesh* mesh, void* key)
{
    // grow
    if (array->size == array->capacity) {
        array->capacity *= 2;
        array->meshes = (Mesh*)realloc(array->meshes, array->capacity * sizeof(Mesh));
        array->keys = realloc(array->keys, array->capacity * array->keySize);
    }

    HashmapSet(&array->index, key, &array->size);

    // copy mesh + increase size
    array->meshes[array->size] = *mesh;
    memcpy((char*)array->keys + array->keySize * array->size, key, array->keySize);
    array->size++;
}

Mesh* MeshArray_Create(MeshArray* array, void* key)
{
    // grow
    if (array->size == array->capacity) {
        array->capacity *= 2;
        array->meshes = (Mesh*)realloc(array->meshes, array->capacity * sizeof(Mesh));
        array->keys = realloc(array->keys, array->capacity * array->keySize);
    }

    HashmapSet(&array->index, key, &array->size);

    // reuse reserved mesh
    if (array->reserveSize > 0) {
        Mesh* reserved = &array->reserve[array->reserveSize - 1];
        array->meshes[array->size] = *reserved;
        array->reserveSize--;
    }
    // create new mesh
    else
    {  
        Mesh* newMesh = &array->meshes[array->size];
        MeshInit(newMesh, 1000, 1000);
    }

    // copy key
    memcpy((char*)array->keys + array->keySize * array->size, key, array->keySize);

    array->size++;
    return &array->meshes[array->size-1];
}

void MeshArray_Delete(MeshArray* array, uint32_t index)
{
    // grow reserve
    if (array->reserveSize == array->reserveCapacity) {
        array->reserveCapacity *= 2;
        array->reserve = (Mesh*)realloc(array->reserve, array->reserveCapacity * sizeof(Mesh));
    }

    // store mesh in reserve
    Mesh* mesh = &array->meshes[index];
    MeshClearCPU(mesh);
    array->reserve[array->reserveSize] = *mesh;
    array->reserveSize++;

    // get key of mesh to delete
    char* deletedKey = (char*)array->keys + index * array->keySize;

    // remove deleted from index
    HashmapDelete(&array->index, deletedKey);

    if (index == array->size-1) 
    {
        array->size--;
    }
    else 
    {
        char* movedKey = (char*)array->keys + (array->size - 1) * array->keySize;

        // move last mesh into hole
        array->meshes[index] = array->meshes[array->size-1];
        memcpy(deletedKey, movedKey, array->keySize);
        
        // update index of moved
        HashmapSet(&array->index, movedKey, &index);
        array->size--;
    }
}

Mesh* MeshArray_Get(MeshArray* array, uint32_t index)
{
    return &array->meshes[index];
}

Mesh* MeshArray_KeyGet(MeshArray* array, void* key)
{
    void* found = HashmapGet(&array->index, key);
    if (!found) return NULL;
    uint32_t index = *(uint32_t*)found;
    return &array->meshes[index];
}

void* MeshArray_GetKey(MeshArray* array, uint32_t index)
{
    return (char*)array->keys + index * array->keySize;
}

bool MeshArray_Contains(MeshArray* array, void* key)
{
    return HashmapContains(&array->index, key);
}

void MeshArray_Clear(MeshArray* array)
{
    // move all meshes to reserve
    for (uint32_t i=0; i<array->size; i++)
    {
        Mesh* mesh = &array->meshes[i];
        MeshClearCPU(mesh);
        MeshFreeGPU(mesh);
        array->reserve[array->reserveSize] = *mesh;
        array->reserveSize++;
    }
    array->size = 0;
    HashmapClear(&array->index);
}