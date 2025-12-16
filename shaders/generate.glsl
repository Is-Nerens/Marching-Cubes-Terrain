#version 460 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

uniform float densityThreshold;
uniform float chunkX;
uniform float chunkY;
uniform float chunkZ;
uniform int cubes;
uniform int partitionSubdivisions;

layout(binding = 0) readonly buffer TriTableBuffer {
    int TriTable[];
};
layout(binding = 1) writeonly buffer VertexBuffer {
    float vertices[];
};
layout(binding = 2) readonly buffer EditDensities {
    float editDensities[];
};
layout(binding = 3) coherent buffer PartitionOccupancy {
    int partitionOccupancyFlags[];
};


// cornerIndexAFromEdge array
const int cornerIndexAFromEdge[12] = int[](0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3);

// cornerIndexBFromEdge array
const int cornerIndexBFromEdge[12] = int[](1, 2, 3, 0, 5, 6, 7, 4, 4, 5, 6, 7);

vec3 VertexInterp(vec4 c1, vec4 c2)
{
    float t = (densityThreshold - c1.w) / (c2.w - c1.w);
    vec3 v;
    v.x = c1.x + t * (c2.x - c1.x);
    v.y = c1.y + t * (c2.y - c1.y);
    v.z = c1.z + t * (c2.z - c1.z);
    return v;
}

vec3 CalculateNormal(vec3 v1, vec3 v2, vec3 v3)
{
    vec3 vect1 = v2 - v1;
    vec3 vect2 = v3 - v1;
    vec3 unnormalized = cross(vect1, vect2);
    return normalize(unnormalized);
}

ivec3 GetGridDimensions()
{
    return ivec3(
        int(gl_NumWorkGroups.x) * int(gl_WorkGroupSize.x) + 1,
        int(gl_NumWorkGroups.y) * int(gl_WorkGroupSize.y) + 1,
        int(gl_NumWorkGroups.z) * int(gl_WorkGroupSize.z) + 1
    );
}

int GetDensityIndex(int x, int y, int z)
{
    int gridsizeX = int(gl_NumWorkGroups.x) * int(gl_WorkGroupSize.x) + 1;
    int gridsizeY = int(gl_NumWorkGroups.y) * int(gl_WorkGroupSize.y) + 1;
    int gridsizeZ = int(gl_NumWorkGroups.z) * int(gl_WorkGroupSize.z) + 1;
    int densityIndex = x + y * gridsizeX + z * gridsizeX * gridsizeY;
    int densitySlotCount = gridsizeX * gridsizeY * gridsizeZ;
    return densityIndex;
}

int TriTableGet(int cubeIndex, int i)
{
    return TriTable[cubeIndex * 16 + i];
}

vec3 RandomGradient3D(int ix, int iy, int iz)
{
    uint h = uint(ix*374761393 + iy*668265263 + iz*2147483647u);
    h = (h ^ (h >> 13u)) * 1274126177u;
    h = h ^ (h >> 16u);
    float theta = float(h & 0xFFFFu) / 65535.0 * 2.0 * 3.14159265;  // azimuth
    float phi   = float((h >> 16u) & 0xFFFFu) / 65535.0 * 3.14159265; // inclination
    vec3 gradient;
    gradient.x = sin(phi) * cos(theta);
    gradient.y = sin(phi) * sin(theta);
    gradient.z = cos(phi);
    return gradient;
}

vec2 RandomGradient2D(int ix, int iy)
{
    const uint w = 32u;
    const uint s = w / 2;
    uint a = uint(ix), b = uint(iy);
    a *= 3284157443U;
    b ^= a << s | a >> (w - s);
    b *= 1911520717U;
    a ^= b << s | b >> (w - s);
    a *= 2048419325U;
    float random = float(a) * (3.14159265 / float(0xFFFFFFFFU)); // in [0, 2*Pi]
    vec2 v;
    v.x = sin(random);
    v.y = cos(random);
    return v;
}


float DotGridGradient3D(int ix, int iy, int iz, float x, float y, float z)
{
    vec3 gradient = RandomGradient3D(ix, iy, iz);
    float dx = x - float(ix);
    float dy = y - float(iy);
    float dz = z - float(iz);
    return dot(gradient, vec3(dx, dy, dz));
}

float DotGridGradient2D(int ix, int iy, float x, float y)
{
    vec2 gradient = RandomGradient2D(ix, iy);
    float dx = x - float(ix);
    float dy = y - float(iy);
    return dot(gradient, vec2(dx, dy));
}

float Interpolate(float a0, float a1, float w)
{
    return (a1 - a0) * (3.0f - w * 2.0f) * w * w + a0;
}

float Perlin3D(float x, float y, float z)
{
    // Grid cell corner coordinates
    int x0 = int(floor(x));
    int y0 = int(floor(y));
    int z0 = int(floor(z));
    int x1 = x0 + 1;
    int y1 = y0 + 1;
    int z1 = z0 + 1;

    // Interpolation weights
    float sx = x - float(x0);
    float sy = y - float(y0);
    float sz = z - float(z0);

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
    return (interpolatedValue + 1.0) * 0.5;
}

float Perlin2D(float x, float y)
{
    // Grid corner coordinates
    int x0 = int(floor(x));
    int y0 = int(floor(y));
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    // Interpolation weights
    float sx = x - float(x0);
    float sy = y - float(y0);

    // Corners
    float n0 = DotGridGradient2D(x0, y0, x, y);
    float n1 = DotGridGradient2D(x1, y0, x, y);
    float n2 = DotGridGradient2D(x0, y1, x, y);
    float n3 = DotGridGradient2D(x1, y1, x, y);

    // Interpolation
    float ix0 = Interpolate(n0, n1, sx);
    float ix1 = Interpolate(n2, n3, sx);

    float interpolatedValue = Interpolate(ix0, ix1, sy);
    return interpolatedValue;
}

float GetSurfaceHeight(float x, float z)
{
    return (Perlin2D((x + chunkX) * 0.005, (z + chunkZ) * 0.005) * 40) + 
    (Perlin2D((x + chunkX) * 0.01, (z + chunkZ) * 0.01) * 20) + 
    (Perlin2D((x + chunkX) * 0.02, (z + chunkZ) * 0.02) * 10) + 
    (Perlin2D((x + chunkX) * 0.04, (z + chunkZ) * 0.04) * 5) + 
    (Perlin2D((x + chunkX) * 0.08, (z + chunkZ) * 0.08) * 2.5);
}

float GetCaveDensity(float x, float y, float z)
{
    float sx = (x + chunkX) * 10.0;
    float sy = (y + chunkY) * 10.0;
    float sz = (z + chunkZ) * 10.0;
    return Perlin3D(sx, sy, sz) 
    + Perlin3D(sx * 2, sy * 2, sz * 2) * 0.5 
    + Perlin3D(sx * 4, sy * 4, sz * 4) * 0.25
    + Perlin3D(sx * 8, sy * 8, sz * 8) * 0.15;
}

float GetDensity(float x, float y, float z)
{
    float pointHeight = y + chunkY;

    // Calculate the density based on the height difference for the surface
    float surfaceHeight = GetSurfaceHeight(x, z) * 1.5;
    float surfaceDensity = surfaceHeight - pointHeight;
    surfaceDensity = clamp(surfaceDensity, 0.0, 1.0);

    float caveDensity = GetCaveDensity(x, y, z) * 0.85; 

    // BLEND BETWEEN CAVE DENSITY AND SURFACE 
    float blendDistance = 6;
    float density = 0;
    if (pointHeight < surfaceHeight + blendDistance)
    {
        float blendTerm = clamp((surfaceHeight + blendDistance - pointHeight) / blendDistance, 0, 1);
        density = mix(surfaceDensity, caveDensity, blendTerm);
    }
    else
    {
        density = surfaceDensity;
    }

    float result = density;
    return result;
}

void AddFace(vec3 v1, vec3 v2, vec3 v3, vec3 normal, int index)
{
    // vertex 1
    vertices[index] = v1.x;
    vertices[index + 1] = v1.y;
    vertices[index + 2] = v1.z;

    // vertex 2
    vertices[index + 3] = v2.x;
    vertices[index + 4] = v2.y;
    vertices[index + 5] = v2.z;

    // vertex 3
    vertices[index + 6] = v3.x;
    vertices[index + 7] = v3.y;
    vertices[index + 8] = v3.z;

    // normal
    vertices[index + 9] = normal.x;
    vertices[index + 10] = normal.y;
    vertices[index + 11] = normal.z;
}

void main()
{
    ivec3 globalPos = ivec3(
        int(gl_GlobalInvocationID.x),
        int(gl_GlobalInvocationID.y), 
        int(gl_GlobalInvocationID.z)
    );

    ivec3 gridDims = GetGridDimensions();
    int threadID = globalPos.x + 
                   globalPos.y * gridDims.x + 
                   globalPos.z * gridDims.x * gridDims.y;
    int partitions = int(pow(8.0, partitionSubdivisions));
    int cubesPerPartition = cubes / partitions;
    int partitionIndex = threadID / cubesPerPartition;

    vec4 corners[8];
    corners[0] = vec4(globalPos.x + 1, globalPos.y + 1, globalPos.z + 2, 0);
    corners[1] = vec4(globalPos.x + 2, globalPos.y + 1, globalPos.z + 2, 0);
    corners[2] = vec4(globalPos.x + 2, globalPos.y + 1, globalPos.z + 1, 0);
    corners[3] = vec4(globalPos.x + 1, globalPos.y + 1, globalPos.z + 1, 0);
    corners[4] = vec4(globalPos.x + 1, globalPos.y + 2, globalPos.z + 2, 0);
    corners[5] = vec4(globalPos.x + 2, globalPos.y + 2, globalPos.z + 2, 0);
    corners[6] = vec4(globalPos.x + 2, globalPos.y + 2, globalPos.z + 1, 0);
    corners[7] = vec4(globalPos.x + 1, globalPos.y + 2, globalPos.z + 1, 0);

    int cubeIndex = 0;
    for (int i=0; i<8; i++)
    {
        float density = GetDensity(corners[i].x, corners[i].y, corners[i].z);
        corners[i].w = density;
        cubeIndex |= (int(density > densityThreshold) << i);
    }

    int i = 0;
    int vertexIndex = threadID * 48;
    while(TriTableGet(cubeIndex, i) != -1)
    {
        int a0 = cornerIndexAFromEdge[TriTableGet(cubeIndex, i)];
        int b0 = cornerIndexBFromEdge[TriTableGet(cubeIndex, i)];
        int a1 = cornerIndexAFromEdge[TriTableGet(cubeIndex, i+1)];
        int b1 = cornerIndexBFromEdge[TriTableGet(cubeIndex, i+1)];
        int a2 = cornerIndexAFromEdge[TriTableGet(cubeIndex, i+2)];
        int b2 = cornerIndexBFromEdge[TriTableGet(cubeIndex, i+2)];
        vec3 v1 = VertexInterp(corners[a0], corners[b0]);
        vec3 v2 = VertexInterp(corners[a1], corners[b1]);
        vec3 v3 = VertexInterp(corners[a2], corners[b2]);
        vec3 normal = CalculateNormal(v1, v2, v3);
        AddFace(v1, v2, v3, normal, vertexIndex + i * 4);
        i += 3;

        // mark partition as "containing mesh vertices"
        int word = partitionIndex >> 5;
        int bit  = partitionIndex & 31;
        atomicOr(partitionOccupancyFlags[word], 1 << (31 - bit));
    }

    // set remaining vertices with empty values
    int cubeVerticesRemainingStart = threadID * 48;
    for (int j=i*4; j<48; ++j) {
        vertices[cubeVerticesRemainingStart + j] = 0.0f;
    }
}