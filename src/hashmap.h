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
    capacity = max(capacity, 10); // Ensure capacity for at least 10 elements

    // Calculate number of bytes needed for occupancy bit array
    uint32_t occupancy_remainder = capacity & 7; // capacity % 8 
    uint32_t occupancy_bytes = capacity >> 3;    // capacity / 8
    if (occupancy_remainder != 0) occupancy_bytes += 1;

    // Allocate space for occupancy bit array and sparse data array
    hmap->occupancy = calloc(occupancy_bytes, 1); 
    hmap->data = malloc((key_size + item_size) * capacity);

    // Initialise tracking variables
    hmap->key_size = key_size;
    hmap->item_size = item_size;
    hmap->item_count = 0;
    hmap->capacity = capacity;
    hmap->max_probes = 0;
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

void Hashmap_Resize_Add(Hashmap* hmap, void* key, void* value)
{
    uint32_t probes = 0;
    uint32_t hash = Hash_Generic(key, hmap->key_size);
    while (probes < hmap->capacity) {
        uint32_t i = (hash + probes) % hmap->capacity;
        uint32_t rem = i & 7; // i % 8
        uint32_t occupancy_index = i >> 3; // i / 8

        if (!(hmap->occupancy[occupancy_index] & (uint8_t)(1 << rem))) { // Found empty slot
            hmap->occupancy[occupancy_index] |= (uint8_t)(1 << rem);     // Mark occupied

            // --- Set key and data ---
            char* base = (char*)hmap->data + i * (hmap->key_size + hmap->item_size);
            memcpy(base, key, hmap->key_size);
            memcpy(base + hmap->key_size, value, hmap->item_size);

            hmap->item_count++;
            break;
        }
        probes++;
    }
    hmap->max_probes = max(hmap->max_probes, probes);
}

void Hashmap_Resize(Hashmap* hmap)
{
    uint32_t old_capacity = hmap->capacity;
    uint8_t* old_occupancy = hmap->occupancy;
    void* old_data = hmap->data;

    // Resize
    hmap->capacity *= 2;
    uint32_t occupancy_remainder = hmap->capacity & 7; // hmap->capacity % 8
    uint32_t occupancy_bytes = hmap->capacity >> 3; // hmap->capacity / 8
    if (occupancy_remainder != 0) occupancy_bytes += 1;
    hmap->occupancy = calloc(occupancy_bytes, 1);
    hmap->data = malloc(hmap->capacity * (hmap->key_size + hmap->item_size));
    hmap->max_probes = 0;
    hmap->item_count = 0;

    // Re-insert all old items
    for (uint32_t i=0; i<old_capacity; i++) {
        uint32_t rem = i & 7; // i % 8
        uint32_t occupancy_index = i >> 3; // i / 8
        if (old_occupancy[occupancy_index] & (1u << rem)) // Found item
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
    uint32_t max_probes = min(hmap->capacity, hmap->max_probes + 1);
    while (probes < max_probes) {
        uint32_t i = (hash + probes) % hmap->capacity;
        uint32_t rem = i & 7; // i % 8
        uint32_t occupancy_index = i >> 3; // i / 8

        if (hmap->occupancy[occupancy_index] & (1u << rem)) { // Found item
            
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
    uint32_t max_probes = min(hmap->capacity, hmap->max_probes + 1);
    while (probes < max_probes) {
        uint32_t i = (hash + probes) % hmap->capacity;
        uint32_t rem = i & 7; // i % 8
        uint32_t occupancy_index = i >> 3; // i / 8

        if (hmap->occupancy[occupancy_index] & (1u << rem)) { // Found item
            
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
    // Resize if surpased max load factor 
    if ((float)hmap->item_count / (float)hmap->capacity > 0.5f) {
        Hashmap_Resize(hmap);
    }

    uint32_t probes = 0;
    uint32_t hash = Hash_Generic(key, hmap->key_size);
    while (probes < hmap->capacity) {
        uint32_t i = (hash + probes) % hmap->capacity;
        uint32_t rem = i & 7; // i % 8
        uint32_t occupancy_index = i >> 3; // i / 8

        if (!(hmap->occupancy[occupancy_index] & (1u << rem))) { // Found empty slot
            hmap->occupancy[occupancy_index] |= (uint8_t)(1 << rem); // Mark occupied
            
            // --- Set key and data ---
            char* base = (char*)hmap->data + i * (hmap->key_size + hmap->item_size);
            memcpy(base, key, hmap->key_size);
            memcpy(base + hmap->key_size, value, hmap->item_size);

            hmap->item_count++;
            break;
        }
        probes++;
    }
    hmap->max_probes = max(hmap->max_probes, probes);
}

void Hashmap_Delete(Hashmap* hmap, void* key)
{
    uint32_t probes = 0;
    uint32_t hash = Hash_Generic(key, hmap->key_size);
    uint32_t max_probes = min(hmap->capacity, hmap->max_probes + 1);
    int found_index = -1;
    int last_collision_match = -1;
    while (probes < max_probes) {
        uint32_t i = (hash + probes) % hmap->capacity;
        uint32_t rem = i & 7; // i % 8
        uint32_t occupancy_index = i >> 3; // i / 8
        uint8_t is_present = hmap->occupancy[occupancy_index] & (1u << rem);

        if (found_index == -1 && is_present) { // Found item
            
            // Check if key matches
            void* check_key = hmap->data + i * (hmap->key_size + hmap->item_size);
            if (memcmp(check_key, key, hmap->key_size) == 0) { // Found correct item -> delete
                hmap->occupancy[occupancy_index] &= ~(1u << rem); // Mark as free
                found_index = (int)i;
                hmap->item_count--;
            }
        } 
        else if (found_index != -1 && is_present) // Check if this one hashes to the same value
        {
            void* candidate_key = (char*)hmap->data + i * (hmap->key_size + hmap->item_size);
            uint32_t candidate_hash = Hash_Generic(candidate_key, hmap->key_size);
            if ((candidate_hash % hmap->capacity) == (uint32_t)found_index) 
            {
                last_collision_match = (int)i;
            }
            else if (last_collision_match != -1) // Backfill data and exit early 
            { 
                // Backfill data and exit
                uint32_t rem_from = last_collision_match & 7;
                uint32_t occupancy_index_from = last_collision_match >> 3;
                uint32_t rem_to = found_index & 7;
                uint32_t occupancy_index_to = found_index >> 3;
                memcpy((char*)hmap->data + found_index * (hmap->key_size + hmap->item_size),
                    (char*)hmap->data + last_collision_match * (hmap->key_size + hmap->item_size),
                    hmap->key_size + hmap->item_size);
                hmap->occupancy[occupancy_index_from] &= ~(1u << rem_from); // old slot free
                hmap->occupancy[occupancy_index_to] |= (1u << rem_to); 
                return;
            }
        }
        else if (found_index != -1 && !is_present) 
        {
            if (last_collision_match == -1) return; // No backfill necessary

            // Backfill data and exit
            uint32_t rem_from = last_collision_match & 7;
            uint32_t occupancy_index_from = last_collision_match >> 3;
            uint32_t rem_to = found_index & 7;
            uint32_t occupancy_index_to = found_index >> 3;
            memcpy((char*)hmap->data + found_index * (hmap->key_size + hmap->item_size),
                (char*)hmap->data + last_collision_match * (hmap->key_size + hmap->item_size),
                hmap->key_size + hmap->item_size);
            hmap->occupancy[occupancy_index_from] &= ~(1u << rem_from); // old slot free
            hmap->occupancy[occupancy_index_to] |= (1u << rem_to); 
            return;
        }
        probes++;
    }
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
        return_key = NULL;
        return_val = NULL;
    }

    // Probe until next item is found
    while(hmap->iterate_index < hmap->capacity)
    {
        uint32_t rem = hmap->iterate_index & 7; // i % 8
        uint32_t occupancy_index = hmap->iterate_index >> 3; // i / 8

        // Found item -> return pointer
        if (hmap->occupancy[occupancy_index] & (1u << rem)) { 
            char* base = (char*)hmap->data + hmap->iterate_index * (hmap->key_size + hmap->item_size);
            hmap->iterate_index++;
            *return_key = base;
            *return_val = base + hmap->key_size;
            return;
        }

        hmap->iterate_index++;
    }

    // Error
    return_key = NULL;
    return_val = NULL;
    return;
}

void Hashmap_Clear(Hashmap* hmap)
{
    if (hmap->capacity == 0) return;

    // Calculate number of bytes needed for occupancy bit array
    uint32_t occupancy_remainder = hmap->capacity & 7; // capacity % 8 
    uint32_t occupancy_bytes = hmap->capacity >> 3;    // capacity / 8
    if (occupancy_remainder != 0) occupancy_bytes += 1;

    // Set occupancy bits to 0
    memset(hmap->occupancy, 0, occupancy_bytes);
    
    // Clear items
    hmap->item_count = 0;
}

void Hashmap_Free(Hashmap* hmap)
{
    free(hmap->occupancy);
    free(hmap->data);
}