#pragma once
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#include "../mesh.h"
#include "../hashmap.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>


// This data structure holds an array of mesh objects
// A generic index can be set to allow fast mesh lookups

typedef struct MeshArray {
    Mesh* meshes;
    uint32_t size;
    uint32_t capacity;
    Hashmap index;
    char* keys;
    bool indexAvailable;
} MeshArray;

void MeshArray_Init(MeshArray* array, uint32_t capacity, uint32_t keySize)
{
    array->capacity = MAX(capacity, 2);
    array->meshes = (Mesh*)malloc(sizeof(Mesh) * array->capacity);
    array->size = 0;

    if (keySize > 0)
    {
        Hashmap_Init(&array->index, keySize, sizeof(uint32_t), array->capacity);
        array->keys = malloc(array->capacity * keySize);
        array->indexAvailable = true;
    }
    else
    {
        array->keys = NULL;
        array->indexAvailable = false;
    }
}

void MeshArray_Free(MeshArray* array)
{
    free(array->meshes);
    if (array->indexAvailable) {
        Hashmap_Free(&array->index);
        free(array->keys);
    }
    array->meshes = NULL;
    array->keys = NULL;
    array->capacity = 0;
    array->size = 0;
}

void MeshArray_Push(MeshArray* array, Mesh* mesh, void* key)
{
    if (array->size == array->capacity) {
        array->capacity *= 2;
        array->meshes = (Mesh*)realloc(array->meshes, array->capacity * sizeof(Mesh));
        if (array->indexAvailable) array->keys = (char*)realloc(array->keys, array->capacity * array->index.key_size);
    }
    array->meshes[array->size] = *mesh;
    array->size++;

    if (array->indexAvailable && key)
    {
        uint32_t index = array->size - 1;
        void* dstKey = array->keys + index * array->index.key_size;
        memcpy(dstKey, key, array->index.key_size);
        Hashmap_Set(&array->index, dstKey, &index);
    }
}

Mesh* MeshArray_CreateInplace(MeshArray* array, void* key)
{
    if (array->size == array->capacity) {
        array->capacity *= 2;
        array->meshes = (Mesh*)realloc(array->meshes, array->capacity * sizeof(Mesh));
        if (array->indexAvailable) array->keys = (char*)realloc(array->keys, array->capacity * array->index.key_size);
    }
    Mesh* newMesh = &array->meshes[array->size];
    array->size++;

    if (array->indexAvailable && key)
    {
        uint32_t index = array->size - 1;
        void* dstKey = array->keys + index * array->index.key_size;
        memcpy(dstKey, key, array->index.key_size);
        Hashmap_Set(&array->index, dstKey, &index);
    }
    return newMesh;
}

void MeshArray_DeleteBackfill(MeshArray* array, uint32_t index)
{
    if (index >= array->size) return;

    uint32_t lastIndex = array->size - 1;

    if (array->indexAvailable)
    {
        void* deadKey = array->keys + index * array->index.key_size;
        Hashmap_Delete(&array->index, deadKey);
    }

    if (index == lastIndex)
    {
        array->size--;
        return;
    }

    array->meshes[index] = array->meshes[lastIndex];

    if (array->indexAvailable)
    {
        void* dstKey = array->keys + index * array->index.key_size;
        void* srcKey = array->keys + lastIndex * array->index.key_size;
        memcpy(dstKey, srcKey, array->index.key_size);
        Hashmap_Set(&array->index, dstKey, &index);
    }

    array->size--;
}

Mesh* MeshArray_Get(MeshArray* array, uint32_t index)
{
    return &array->meshes[index];
}

Mesh* MeshArray_KeyGet(MeshArray* array, void* key)
{
    void* found = Hashmap_Get(&array->index, key);
    if (!found) return NULL;
    uint32_t index = *(uint32_t*)found;
    return &array->meshes[index];
}

bool MeshArray_Contains(MeshArray* array, void* key)
{
    void* found = Hashmap_Get(&array->index, key);
    return found != NULL;
}

void MeshArray_Clear(MeshArray* array)
{
    array->size = 0;
    if (array->indexAvailable) Hashmap_Clear(&array->index);
}