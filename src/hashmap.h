#pragma once
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

// ------------------------------------
// --- Import -------------------------
// ------------------------------------
#include <stdint.h>
#include <string.h>
#include <stdlib.h>


// ------------------------------------------------------
// --- Datastructure For Mapping (generic -> generic) ---
// ------------------------------------------------------
typedef struct Hashmap
{
    uint8_t* occupancy;
    void* data;
    uint32_t key_size;
    uint32_t item_size;
    uint32_t item_count;
    uint32_t capacity;
    uint32_t max_probes;
    uint32_t iterate_index;
} Hashmap;

void Hashmap_Init(Hashmap* hmap, uint32_t key_size, uint32_t item_size, uint32_t capacity)
{
    capacity = MAX(capacity, 10); // Ensure capacity for at least 10 elements

    // allocate space for occupancy bit array
    uint32_t occupancy_remainder = capacity & 7;
    uint32_t occupancy_bytes = capacity >> 3;
    if (occupancy_remainder != 0) occupancy_bytes += 1;
    hmap->occupancy = calloc(occupancy_bytes, 1); 

    // Allocate space for sparse data array
    hmap->data = malloc((key_size + item_size) * capacity);

    // Initialise tracking variables
    hmap->key_size = key_size;
    hmap->item_size = item_size;
    hmap->item_count = 0;
    hmap->capacity = capacity;
    hmap->max_probes = 1;
}

// FNV algorithm https://github.com/aappleby/smhasher/blob/master/src/Hashes.cpp
static uint32_t Hash_Generic(void* key, uint32_t len) // FNV algorithm https://github.com/aappleby/smhasher/blob/master/src/Hashes.cpp
{
    uint32_t hash = 2166136261u;
    uint8_t* p = (uint8_t*)key;
    for (uint32_t i=0; i<len; i++) {
        hash ^= p[i];
        hash *= 16777619u;
    }
    return hash;
}

inline uint8_t Hashmap_Slot_Present(Hashmap* hmap, uint32_t i)
{
    return hmap->occupancy[i >> 3] & (1u << (i & 7));
}

inline void Hashmap_Mark_Slot(Hashmap* hmap, uint32_t i)
{
    hmap->occupancy[i >> 3] |= (uint8_t)(1 << (i & 7));
}

inline void Hashmap_Clear_Slot(Hashmap* hmap, uint32_t i)
{
    hmap->occupancy[i >> 3] &= ~(1u << (i & 7));
}

void Hashmap_Resize_Add(Hashmap* hmap, void* key, void* value)
{
    uint32_t probes = 0;
    uint32_t hash = Hash_Generic(key, hmap->key_size);
    while (probes < hmap->capacity) {
        uint32_t i = (hash + probes) % hmap->capacity;

        if (!Hashmap_Slot_Present(hmap, i)) { // Found empty slot

            // set key and data
            char* base = (char*)hmap->data + i * (hmap->key_size + hmap->item_size);
            memcpy(base, key, hmap->key_size);
            memcpy(base + hmap->key_size, value, hmap->item_size);

            // mark slot and increase item count
            Hashmap_Mark_Slot(hmap, i);
            hmap->item_count++;
            break;
        }
        probes++;
    }
    hmap->max_probes = MAX(hmap->max_probes, probes + 1);
}

void Hashmap_Resize(Hashmap* hmap)
{
    uint32_t old_capacity = hmap->capacity;
    uint8_t* old_occupancy = hmap->occupancy;
    void* old_data = hmap->data;

    // Resize
    hmap->capacity *= 2;
    uint32_t occupancy_remainder = hmap->capacity & 7;
    uint32_t occupancy_bytes = hmap->capacity >> 3;
    if (occupancy_remainder != 0) occupancy_bytes += 1;
    hmap->occupancy = calloc(occupancy_bytes, 1);
    hmap->data = malloc(hmap->capacity * (hmap->key_size + hmap->item_size));
    hmap->max_probes = 0;
    hmap->item_count = 0;

    // Re-insert all old items
    for (uint32_t i=0; i<old_capacity; i++) {

        uint8_t is_present = old_occupancy[i >> 3] & (1u << (i & 7));
        if (is_present) // Found item
        { 
            char* base = (char*)old_data + i * (hmap->key_size + hmap->item_size);
            void* key = base;
            void* value = base + hmap->key_size;
            Hashmap_Resize_Add(hmap, key, value); // Re-add item 
        }
    }

    free(old_occupancy);
    free(old_data);
}

int Hashmap_Contains(Hashmap* hmap, void* key)
{
    uint32_t probes = 0;
    uint32_t hash = Hash_Generic(key, hmap->key_size);
    while (probes < hmap->max_probes) {
        uint32_t i = (hash + probes) % hmap->capacity;
        if (Hashmap_Slot_Present(hmap, i)) { // Found item
            
            // Check if key matches
            char* base = (char*)hmap->data + i * (hmap->key_size + hmap->item_size);
            void* check_key = base;
            if (memcmp(check_key, key, hmap->key_size) == 0) {
                return 1;
            }
        } else {
            return 0;
        }
        probes++;
    }

    return 0;
}

void* Hashmap_Get(Hashmap* hmap, void* key)
{
    uint32_t probes = 0;
    uint32_t hash = Hash_Generic(key, hmap->key_size);
    while (probes < hmap->max_probes) {
        uint32_t i = (hash + probes) % hmap->capacity;
        if (Hashmap_Slot_Present(hmap, i)) { // Found item
            
            // Check if key matches
            char* base = (char*)hmap->data + i * (hmap->key_size + hmap->item_size);
            void* check_key = base;
            if (memcmp(check_key, key, hmap->key_size) == 0) {
                return base + hmap->key_size;
            }
        }
        else {
            return NULL;
        }
        probes++;
    }

    return NULL;
}

void Hashmap_Set(Hashmap* hmap, void* key, void* value)
{
    // Resize if surpassed max load factor 
    if ((float)hmap->item_count / (float)hmap->capacity > 0.5f) {
        Hashmap_Resize(hmap);
    }

    uint32_t probes = 0;
    uint32_t hash = Hash_Generic(key, hmap->key_size);
    while (probes < hmap->max_probes) {
        uint32_t i = (hash + probes) % hmap->capacity;

        if (Hashmap_Slot_Present(hmap, i)) {
            char* base = (char*)hmap->data + i * (hmap->key_size + hmap->item_size);
            if (memcmp(base, key, hmap->key_size) == 0) {
                memcpy(base + hmap->key_size, value, hmap->item_size);
                return;
            }
        } 
        else 
        {
            Hashmap_Mark_Slot(hmap, i);
            
            // set key and value
            char* base = (char*)hmap->data + i * (hmap->key_size + hmap->item_size);
            memcpy(base, key, hmap->key_size);
            memcpy(base + hmap->key_size, value, hmap->item_size);

            hmap->item_count++;
            break;
        }
        probes++;
    }
    hmap->max_probes = MAX(hmap->max_probes, probes + 1);
}

void Hashmap_Delete(Hashmap* hmap, void* key)
{
    uint32_t probes = 0;
    uint32_t hash = Hash_Generic(key, hmap->key_size);

    int holeIndex = -1;
    while(probes < hmap->max_probes) 
    {
        uint32_t i = (hash + probes) % hmap->capacity;

        if (Hashmap_Slot_Present(hmap, i)) {
            void* check_key = (char*)hmap->data + i * (hmap->key_size + hmap->item_size);
            if (memcmp(check_key, key, hmap->key_size) == 0) {
                holeIndex = (int)i;
                Hashmap_Clear_Slot(hmap, i);
                break;
            }
        }
        else break;

        probes++;
    }

    if (holeIndex == -1) return; // key not found

    uint32_t i = (holeIndex + 1) % hmap->capacity;
    while (Hashmap_Slot_Present(hmap, i)) 
    {
        void* candidate_key = (char*)hmap->data + i * (hmap->key_size + hmap->item_size);
        uint32_t candidate_hash = Hash_Generic(candidate_key, hmap->key_size);
        uint32_t candidate_home = candidate_hash % hmap->capacity;

        // Can the candidate move into the hole?
        bool canMoveCandidate;
        if (holeIndex <= i)
            canMoveCandidate = (candidate_home <= holeIndex || candidate_home > i);
        else
            canMoveCandidate = (candidate_home <= holeIndex && candidate_home > i);

        if (!canMoveCandidate) {
            i = (i + 1) % hmap->capacity;
            continue;
        }

        // Move candidate into hole
        memcpy(
            (char*)hmap->data + holeIndex * (hmap->key_size + hmap->item_size),
            candidate_key,
            hmap->key_size + hmap->item_size
        );

        Hashmap_Clear_Slot(hmap, i);
        Hashmap_Mark_Slot(hmap, holeIndex);

        holeIndex = i;
        i = (i + 1) % hmap->capacity;
    }

    hmap->item_count--;
}

void Hashmap_Iterate_Begin(Hashmap* hmap)
{
    hmap->iterate_index = 0;
}

int Hashmap_Iterate_Continue(Hashmap* hmap)
{   
    return hmap->iterate_index < hmap->capacity;
}

void Hashmap_Iterate_Get(Hashmap* hmap, void** return_key, void** return_val)
{
    if (hmap->item_count == 0) 
    {
        *return_key = NULL;
        *return_val = NULL;
    }

    // Probe until next item is found
    while(hmap->iterate_index < hmap->capacity)
    {

        // Found item -> return pointer
        if (Hashmap_Slot_Present(hmap, hmap->iterate_index)) { 
            char* base = (char*)hmap->data + hmap->iterate_index * (hmap->key_size + hmap->item_size);
            hmap->iterate_index++;
            *return_key = base;
            *return_val = base + hmap->key_size;
            return;
        }

        hmap->iterate_index++;
    }

    // Error
    *return_key = NULL;
    *return_val = NULL;
    return;
}

void Hashmap_Clear(Hashmap* hmap)
{
    if (hmap->capacity == 0) return;

    // Set occupancy bits to 0
    uint32_t occupancy_remainder = hmap->capacity & 7;
    uint32_t occupancy_bytes = hmap->capacity >> 3;
    if (occupancy_remainder != 0) occupancy_bytes += 1;
    memset(hmap->occupancy, 0, occupancy_bytes);
    
    // Clear items
    hmap->item_count = 0;
}

void Hashmap_Free(Hashmap* hmap)
{
    free(hmap->occupancy);
    free(hmap->data);
    hmap->occupancy = NULL;
    hmap->data = NULL;
    hmap->capacity = 0;
    hmap->item_count = 0;
}