#pragma once

#include <iostream>
#include <functional>
#include <cstring>
#include <cstring>

class VertexHasher
{
public:
    VertexHasher()
        : vertexMap(32768),        // Create vertexMap with 32768 elements
          vertexMapKeys(98304)    // Create vertexMapKeys with 98304 elements
    {
        // Initialize vertexMap with -1 using std::memset
        std::memset(vertexMap.data(), -1, vertexMap.size() * sizeof(int));
    }

    unsigned int GetVertexIndex(float x, float y, float z)
    {
        int keyIndex = Hash(x, y, z);

        for (int i = 0; i < maxProbeDistance + 1; ++i)
        {
            int index = (keyIndex + i) % 32768;
            if (vertexMapKeys[index * 3] == x &&
                vertexMapKeys[index * 3 + 1] == y &&
                vertexMapKeys[index * 3 + 2] == z)
            {
                return vertexMap[index];
            }
        }

        return -1;
    }

    void SetVertexIndex(float x, float y, float z, unsigned int value)
    {
        int keyIndex = Hash(x, y, z);

        for (int i = 0; i < maxProbeDistance + 1; ++i)
        {
            int index = (keyIndex + i) % 32768;
            if (vertexMapKeys[index * 3] == x &&
                vertexMapKeys[index * 3 + 1] == y &&
                vertexMapKeys[index * 3 + 2] == z)
            {
                vertexMap[index] = value;
                return;
            }
        }

        int stepCount = 0;
        while (stepCount < 32768)
        {
            int index = (keyIndex + stepCount) % 32768;
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

private:
    std::vector<int> vertexMap;
    std::vector<float> vertexMapKeys;
    int maxProbeDistance = 0;

    int Hash(float x, float y, float z)
    {
        size_t h1 = std::hash<float>{}(x);
        size_t h2 = std::hash<float>{}(y);
        size_t h3 = std::hash<float>{}(z);
        size_t hash = h1;
        hash ^= h2 + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= h3 + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        return hash % 32768;
    }
};


