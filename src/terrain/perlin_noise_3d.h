#pragma once

#include <vector>
#include <iostream>
#include <chrono>

typedef struct{
    float x, y, z;
} vector3;

vector3 RandomGradient(int ix, int iy, int iz) {
    // No precomputed gradients mean this works for any number of grid coordinates
    const unsigned w = 8 * sizeof(unsigned);
    const unsigned s = w / 2; 
    unsigned a = ix, b = iy, c = iz;
    a *= 3284157443;
    b ^= a << s | a >> (w - s);
    b *= 1911520717;
    a ^= b << s | b >> (w - s);
    a *= 2048419325;
    float randomX = a * (3.14159265 / ~(~0u >> 1)); // in [0, 2*Pi]

    // Generate a random angle for the z-axis
    c *= 3329032859;
    c ^= a << s | a >> (w - s);
    c *= 1431691223;
    b ^= c << s | c >> (w - s);
    b *= 1812433253;
    float randomZ = b * (3.14159265 / ~(~0u >> 1)); // in [0, 2*Pi]

    // Create the vector from the angles
    vector3 v;
    v.x = sin(randomX) * cos(randomZ);
    v.y = cos(randomX) * cos(randomZ);
    v.z = sin(randomZ);

    return v;
}

float DotGridGradient(int ix, int iy, int iz, float x, float y, float z)
{
    vector3 gradient = RandomGradient(ix, iy, iz);

    float dx = x - (float)ix;
    float dy = y - (float)iy;
    float dz = z - (float)iz;
    
    // compute dot-product
    float dot = (dx * gradient.x + dy * gradient.y + dz * gradient.z);
    return dot;
}

float Interpolate(float a0, float a1, float w)
{
    return (a1 - a0) * (3.0f - w * 2.0f) * w * w + a0;
}

float PerlinNoise3D(float x, float y, float z) 
{
    // grid cell corner coordinates
    int x0 = (int)x;
    int y0 = (int)y;
    int z0 = (int)z;
    int x1 = x0 + 1;
    int y1 = y0 + 1;
    int z1 = z0 + 1;

    // interpolation weights
    float sx = x - (float)x0;
    float sy = y - (float)y0;
    float sz = z - (float)z0;

    // compute interpolation
    // bottom corners
    float n0 = DotGridGradient(x0, y0, z0, x, y, z);
    float n1 = DotGridGradient(x1, y0, z0, x, y, z);
    float n2 = DotGridGradient(x0, y1, z0, x, y, z);
    float n3 = DotGridGradient(x1, y1, z0, x, y, z);

    // top corners
    float n4 = DotGridGradient(x0, y0, z1, x, y, z);
    float n5 = DotGridGradient(x1, y0, z1, x, y, z);
    float n6 = DotGridGradient(x0, y1, z1, x, y, z);
    float n7 = DotGridGradient(x1, y1, z1, x, y, z);

    
    // trilinear interpolation
    float ix0 = Interpolate(n0, n1, sx);
    float ix1 = Interpolate(n2, n3, sx);
    float ix2 = Interpolate(n4, n5, sx);
    float ix3 = Interpolate(n6, n7, sx);

    float iy0 = Interpolate(ix0, ix1, sy);
    float iy1 = Interpolate(ix2, ix3, sy);

    float interpolatedValue = Interpolate(iy0, iy1, sz);
    return interpolatedValue;
}