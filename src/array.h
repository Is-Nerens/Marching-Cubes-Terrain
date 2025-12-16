#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Array
{
    void* data;  
    uint32_t element_size;
    uint32_t size; 
    uint32_t capacity;         
};

void Array_Init(struct Array* arr, uint32_t element_size, uint32_t capacity) 
{
    arr->data = malloc(capacity * element_size);
    arr->element_size = element_size;
    arr->size = 0;
    arr->capacity = capacity;
}

void Array_Push(struct Array* array, void* value) 
{
    memcpy((char*)array->data + (array->size * array->element_size), value, array->element_size);
    array->size++;
}

void Array_Set(struct Array* array, uint32_t index, void* value)
{
    memcpy((char*)array->data + (index * array->element_size), value, array->element_size);
}

void* Array_Get(const struct Array* array, uint32_t index) 
{
    return (char*)array->data + (index * array->element_size);
}

void* Array_Get_Unsafe(const struct Array* array, uint32_t index) 
{
    return (char*)array->data + (index * array->element_size);
}

void* Array_Top(const struct Array* array, uint32_t index) 
{
    uint32_t reverse_index = array->size - 1 - index;
    return (char*)array->data + (reverse_index * array->element_size);
}

void Array_Delete_Shift(struct Array* array, uint32_t index)
{
    if (index < array->size - 1)
    {
        void* dest = (char*)array->data + (index * array->element_size);
        void* src  = (char*)array->data + ((index + 1) * array->element_size);
        uint32_t num_bytes_to_move = (array->size - index - 1) * array->element_size;

        memmove(dest, src, num_bytes_to_move);
    }
    array->size--;
}

void Array_Insert(struct Array* array, uint32_t index, void* value)
{
    void* dest = (char*)array->data + ((index + 1) * array->element_size);
    void* src  = (char*)array->data + (index * array->element_size);
    uint32_t num_bytes_to_move = (array->size - index) * array->element_size;

    memmove(dest, src, num_bytes_to_move);  // Shift to the right
    memcpy(src, value, array->element_size); // Insert new value
    array->size++;
}

void Array_Clear(struct Array* array) 
{
    array->size = array->capacity = 0;
}


void Array_Free(struct Array* array) 
{
    free(array->data);
    array->size = array->capacity = 0;
    array->data = NULL;
}
