#pragma once
#include <cglm/cglm.h>
#include "math.h"

void RandomGradient3D(int ix, int iy, int iz, vec3 gradient)
{
    uint32_t h = (uint32_t)(ix*374761393 + iy*668265263 + iz*2147483647); // some hash
    h = (h ^ (h >> 13)) * 1274126177;
    h = h ^ (h >> 16);
    float theta = (float)(h & 0xFFFF) / 0xFFFF * 2.0f * 3.14159265f; // azimuth
    float phi   = (float)((h >> 16) & 0xFFFF) / 0xFFFF * 3.14159265f; // inclination
    gradient[0] = sinf(phi) * cosf(theta);
    gradient[1] = sinf(phi) * sinf(theta);
    gradient[2] = cosf(phi);
}


float DotGridGradient3D(int ix, int iy, int iz, float x, float y, float z)
{
    vec3 gradient;
    RandomGradient3D(ix, iy, iz, gradient);
    vec3 delta;
    delta[0] = x - (float)ix;
    delta[1] = y - (float)iy;
    delta[2] = z - (float)iz;
    float result = glm_vec3_dot(gradient, delta);
    return result;
}

float Interpolate(float a0, float a1, float w)
{
    return (a1 - a0) * (3.0f - w * 2.0f) * w * w + a0;
}


float Perlin3D(float x, float y, float z)
{
    // Grid cell corner coordinates
    int x0 = (int)floor(x);
    int y0 = (int)floor(y);
    int z0 = (int)floor(z);
    int x1 = x0 + 1;
    int y1 = y0 + 1;
    int z1 = z0 + 1;

    // Interpolation weights
    float sx = x - (float)x0;
    float sy = y - (float)y0;
    float sz = z - (float)z0;

    // Bottom corners
    float n0 = DotGridGradient3D(x0, y0, z0, x, y, z);
    float n1 = DotGridGradient3D(x1, y0, z0, x, y, z);
    float n2 = DotGridGradient3D(x0, y1, z0, x, y, z);
    float n3 = DotGridGradient3D(x1, y1, z0, x, y, z);

    // Top corners
    float n4 = DotGridGradient3D(x0, y0, z1, x, y, z);
    float n5 = DotGridGradient3D(x1, y0, z1, x, y, z);
    float n6 = DotGridGradient3D(x0, y1, z1, x, y, z);
    float n7 = DotGridGradient3D(x1, y1, z1, x, y, z);

    // Trilinear interpolation
    float ix0 = Interpolate(n0, n1, sx);
    float ix1 = Interpolate(n2, n3, sx);
    float ix2 = Interpolate(n4, n5, sx);
    float ix3 = Interpolate(n6, n7, sx);
    float iy0 = Interpolate(ix0, ix1, sy);
    float iy1 = Interpolate(ix2, ix3, sy);

    float interpolatedValue = Interpolate(iy0, iy1, sz);
    return (interpolatedValue + 1.0) / 2.0;
}