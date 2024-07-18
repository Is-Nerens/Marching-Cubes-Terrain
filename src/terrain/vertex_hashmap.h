#pragma once

#include <iostream>
#include <functional>

namespace VertexHasher
{
    int vertexMap[32768];
    float vertexMapKeys[131072];
    int maxProbeDistance = 0;

    void ResetHashTable()
    {
        std::fill(std::begin(vertexMap), std::end(vertexMap), -1);
        std::fill(std::begin(vertexMapKeys), std::end(vertexMapKeys), -1.0f);
        int maxProbeDistance = 0;
    }

    int Hash(float x, float y, float z)
    {
        size_t h1 = std::hash<float>{}(x);
        size_t h2 = std::hash<float>{}(y);
        size_t h3 = std::hash<float>{}(z);
        int hash = abs(static_cast<int>(h1 ^ (h2 << 1) ^ (h3 << 2)) % 32768);
        return hash;
    }

    unsigned int GetVertexIndex(float x, float y, float z)
    {
        int keyIndex = Hash(x, y, z);

        for (int i=0; i<maxProbeDistance+1; ++i)
        {
            int index = (keyIndex + i) % 32768;
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
            int index = (keyIndex + i) % 32768;
            if (vertexMapKeys[index * 3] == x && vertexMapKeys[index * 3 + 1] == y && vertexMapKeys[index * 3 + 2] == z)
            {
                vertexMap[index] = value;
                return;
            }
        }


        int stepCount = 0;
        while(stepCount < 32768)
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
};
