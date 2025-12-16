#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct DynamicArray
{
    void* data;  
    uint32_t element_size;
    uint32_t size;     
    uint32_t capacity; 
} DynamicArray; 

void DynamicArray_Init (DynamicArray* arr, uint32_t element_size, uint32_t initial_capacity) 
{
    arr->data = malloc(initial_capacity * element_size);
    arr->element_size = element_size;
    arr->size = 0;
    arr->capacity = initial_capacity;
}

void DynamicArray_Push(DynamicArray* array, void* value) 
{
    if (array->size >= array->capacity) 
    {
        array->capacity *= 2;
        array->data = realloc(array->data, array->capacity * array->element_size);
        if (!array->data) 
        {
            fprintf(stderr, "[DynamicArray_Push] Memory allocation failed\n");
            return;
        }
    }
    memcpy((char*)array->data + (array->size * array->element_size), value, array->element_size);
    array->size++;
}

void* DynamicArray_Push_Empty(DynamicArray* array)
{
    if (array->size >= array->capacity) 
    {
        array->capacity *= 2;
        array->data = realloc(array->data, array->capacity * array->element_size);
        if (!array->data) 
        {
            fprintf(stderr, "[DynamicArray_Push] Memory allocation failed\n");
            return NULL;
        }
    }
    char* dst = (char*)array->data + (array->size * array->element_size);
    array->size++;
    return dst;
}

void DynamicArray_Delete_Backfill(DynamicArray* array, uint32_t index)
{
    if (index >= array->size) 
    {
        fprintf(stderr, "[Dynamic_Array_Delete_Backfill] Index out of bounds: index=%u, size=%u\n", index, array->size);
        return;
    }
    if (index == array->size - 1) 
    {
        array->size--;
        return;
    }
    void* target = (char*)array->data + (index * array->element_size);
    void* last   = (char*)array->data + ((array->size - 1) * array->element_size);
    memcpy(target, last, array->element_size);
    array->size--;
}


void* DynamicArray_Get(DynamicArray* array, uint32_t index) 
{
    if (index >= array->size) 
    {
        fprintf(stderr, "[DynamicArray_Get] Index out of bounds: index=%u, array size=%u", index, array->size);
        return NULL;
    }
    return (char*)array->data + (index * array->element_size);
}

void DynamicArray_Clear(DynamicArray* array)
{
    array->size = 0;
}

void DynamicArray_Free(DynamicArray* array) 
{
    free(array->data);
    array->size = array->capacity = 0;
    array->data = NULL;
}

#endif