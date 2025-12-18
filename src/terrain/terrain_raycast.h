#pragma once
#include <cglm/cglm.h>
#include "../mesh.h"
#include "terrain.h"

typedef struct RayHit {
    vec3 position;
    vec3 normal;
    float distance;
    bool hit;
} RayHit;

typedef struct Ray {
    vec3 origin;
    vec3 dir;
} Ray;

bool RayIntersectsAABB(Ray ray, AABB aabb)
{
    float invRayDirX = 1.0f / ray.dir[0];
    float tMin = (aabb.minX - ray.origin[0]) * invRayDirX;
    float tMax = (aabb.maxX - ray.origin[0]) * invRayDirX;
    if (tMin > tMax) {
        float temp = tMin;
        tMin = tMax;
        tMax = temp;
    }
    float invRayDirY = 1.0f / ray.dir[1];
    float tYMin = (aabb.minY - ray.origin[1]) * invRayDirY;
    float tYMax = (aabb.maxY - ray.origin[1]) * invRayDirY;
    if (tYMin > tYMax) {
        float temp = tYMin;
        tYMin = tYMax;
        tYMax = temp;
    }
    if (tMin > tYMax || tYMin > tMax) return false;
    if (tYMin > tMin) tMin = tYMin;
    if (tYMax < tMax) tMax = tYMax;
    float invRayDirZ = 1.0f / ray.dir[2];
    float tZMin = (aabb.minZ - ray.origin[2]) * invRayDirZ;
    float tZMax = (aabb.maxZ - ray.origin[2]) * invRayDirZ;
    if (tZMin > tZMax) {
        float temp = tZMin;
        tZMin = tZMax;
        tZMax = temp;
    }
    return !(tMin > tZMax || tZMin > tMax);
}

RayHit RayTriangleIntersection(Ray ray, vec3 v1, vec3 v2, vec3 v3)
{
    RayHit hit = {0};
    hit.normal[1] = 1.0f;
    hit.distance = -1.0f;
    hit.hit = false;

    vec3 edge1, edge2, pvec;
    glm_vec3_sub(v2, v1, edge1);
    glm_vec3_sub(v3, v1, edge2);
    glm_cross(ray.dir, edge2, pvec);
    float determinant = glm_dot(edge1, pvec);

    const float EPSILON = 1e-8f;
    if (fabs(determinant) < EPSILON) return hit;

    float invDet = 1.0f / determinant;

    vec3 tvec;
    glm_vec3_sub(ray.origin, v1, tvec);
    float u = glm_dot(tvec, pvec) * invDet;
    if (u < 0.0f || u > 1.0f) return hit;

    vec3 qvec;
    glm_cross(tvec, edge1, qvec);
    float v = glm_dot(ray.dir, qvec) * invDet;
    if (v < 0.0f || u + v > 1.0f) return hit;
    float t = glm_dot(edge2, qvec) * invDet;
    if (t < EPSILON) return hit;

    glm_vec3_scale(ray.dir, t, hit.position);
    glm_vec3_add(ray.origin, hit.position, hit.position);
    glm_cross(edge1, edge2, hit.normal);
    glm_normalize(hit.normal);
    hit.distance = t;
    hit.hit = true;
    return hit;
}

bool InBoundingBox(vec3 pos, AABB aabb)
{
    bool insideX = pos[0] >= aabb.minX && pos[0] <= aabb.maxX;
    bool insideY = pos[1] >= aabb.minY && pos[1] <= aabb.maxY;
    bool insideZ = pos[2] >= aabb.minZ && pos[2] <= aabb.maxZ;
    return insideX && insideY && insideZ;
}

RayHit TerrainRaycast(Terrain* terrain, Ray ray)
{
    RayHit hit = {0};
    hit.normal[1] = 1.0f;
    hit.distance = -1.0f;
    hit.hit = false;
    
    float closestHit = 1000000.0f;

    for (uint32_t i=0; i<terrain->chunks.size; i++)
    {
        Mesh* mesh = MeshArray_Get(&terrain->chunks, i);

        // If ray intersects mesh AABB
        if (RayIntersectsAABB(ray, mesh->aabb)) 
        {
            for (uint32_t j=0; j<mesh->indexCount; j+=3) 
            {
                uint32_t v1Index = mesh->indexBuffer[j];
                uint32_t v2Index = mesh->indexBuffer[j+1];
                uint32_t v3Index = mesh->indexBuffer[j+2];
                vec3 v1 = { 
                    mesh->vertexBuffer[v1Index * 6    ] + mesh->x, 
                    mesh->vertexBuffer[v1Index * 6 + 1] + mesh->y, 
                    mesh->vertexBuffer[v1Index * 6 + 2] + mesh->z 
                };
                vec3 v2 = { 
                    mesh->vertexBuffer[v2Index * 6    ] + mesh->x, 
                    mesh->vertexBuffer[v2Index * 6 + 1] + mesh->y, 
                    mesh->vertexBuffer[v2Index * 6 + 2] + mesh->z 
                };
                vec3 v3 = { 
                    mesh->vertexBuffer[v3Index * 6    ] + mesh->x, 
                    mesh->vertexBuffer[v3Index * 6 + 1] + mesh->y, 
                    mesh->vertexBuffer[v3Index * 6 + 2] + mesh->z 
                };

                // Ray triangle intersection
                RayHit newHit = RayTriangleIntersection(ray, v1, v2, v3);
                if (newHit.hit && newHit.distance < closestHit) {
                    closestHit = newHit.distance;
                    hit = newHit;
                }
            }
        }
    }
    return hit;
}