#pragma once

#include <iostream>
#include <functional>

namespace VertexHasher
{
    int vertexMap[65536];
    float vertexMapKeys[196608];
    int maxProbeDistance = 0;

    void ResetHashTable()
    {
        std::memset(vertexMap, -1, sizeof(vertexMap));
        int maxProbeDistance = 0;
    }

    int Hash(float x, float y, float z)
    {
        size_t h1 = std::hash<float>{}(x);
        size_t h2 = std::hash<float>{}(y);
        size_t h3 = std::hash<float>{}(z);
        size_t hash = h1;
        hash ^= h2 + 0x9e3779b9 + (hash << 6) + (hash >> 2);  // 0x9e3779b9 is a prime number
        hash ^= h3 + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        return hash % 65536;
    }

    unsigned int GetVertexIndex(float x, float y, float z)
    {
        int keyIndex = Hash(x, y, z);

        for (int i=0; i<maxProbeDistance+1; ++i)
        {
            int index = (keyIndex + i) % 65536;
            if (vertexMapKeys[index * 3] == x && vertexMapKeys[index * 3 + 1] == y && vertexMapKeys[index * 3 + 2] == z)
            {
                return vertexMap[index];
            }
        }

        // key not found
        return -1;
    }

    void SetVertexIndex(float x, float y, float z, unsigned int value)
    {
        int keyIndex = Hash(x, y, z);

        for (int i=0; i<maxProbeDistance+1; ++i)
        {
            int index = (keyIndex + i) % 65536;
            if (vertexMapKeys[index * 3] == x && vertexMapKeys[index * 3 + 1] == y && vertexMapKeys[index * 3 + 2] == z)
            {
                vertexMap[index] = value;
                return;
            }
        }


        int stepCount = 0;
        while(stepCount < 65536)
        {
            int index = (keyIndex + stepCount) % 65536;
            if (vertexMap[index] == -1)
            {
                vertexMapKeys[index * 3] = x;
                vertexMapKeys[index * 3 + 1] = y;
                vertexMapKeys[index * 3 + 2] = z;
                vertexMap[index] = value;
                return;
            }

            stepCount += 1;
        }
        if (stepCount > maxProbeDistance) maxProbeDistance = stepCount;
    }
};
