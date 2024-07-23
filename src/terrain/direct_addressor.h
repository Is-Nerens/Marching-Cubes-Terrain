#pragma once

namespace DirectAddressing
{
    int IndicesA[34848];
    int IndicesB[34848];
    int IndicesC[34848];

    int GetIndex(int x, int y, int z)
    {
        return x + y * (32) + z * (1089);
    }

    int Hash(float x, float y, float z)
    {
        return GetIndex(std::round(x), std::round(y), std::round(z));
    }

    void ResetIndices()
    {
        std::memset(IndicesA, -1, sizeof(IndicesA));
        std::memset(IndicesB, -1, sizeof(IndicesB));
        std::memset(IndicesC, -1, sizeof(IndicesC));
    }


    int GetVertexIndex(float x, float y, float z)
    {
        float dX = x - std::floor(x);
        float dY = y - std::floor(y);
        float dZ = z - std::floor(z);
        int index = Hash(x, y, z);

        if (dX != 0.5f) {
            return IndicesA[index];
        }

        if (dY != 0.5f) {
            return IndicesB[index];
        }

        if (dZ != 0.5f) {
            return IndicesC[index];
        }

        return -1;
    }

    void SetVertexIndex(float x, float y, float z, int value)
    {
        float dX = x - std::floor(x);
        float dY = y - std::floor(y);
        float dZ = z - std::floor(z);
        int index = Hash(x, y, z);

        if (dX != 0.5f) {
            IndicesA[index] = value;
        }

        if (dY != 0.5f) {
            IndicesB[index] = value;
        }

        if (dZ != 0.5f) {
            IndicesC[index] = value;
        }
    }
}