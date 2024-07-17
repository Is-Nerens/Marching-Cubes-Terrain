#pragma once

struct RayHit {
    glm::vec3 position = glm::vec3{0.0};
    glm::vec3 normal = glm::vec3{0.0};
    float distance = 0.0f;
    bool hit = false;
};

struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
};

struct BoundingBox {
    glm::vec3 min;
    glm::vec3 max;
};

bool RayIntersectsBox(const Ray& ray, const BoundingBox& BoundingBox)
{
    float tMin = (BoundingBox.min.x - ray.origin.x) / ray.direction.x;
    float tMax = (BoundingBox.max.x - ray.origin.x) / ray.direction.x;
    if (tMin > tMax) std::swap(tMin, tMax);
    float tYMin = (BoundingBox.min.y - ray.origin.y) / ray.direction.y;
    float tYMax = (BoundingBox.max.y - ray.origin.y) / ray.direction.y;
    if (tYMin > tYMax) std::swap(tYMin, tYMax);
    if ((tMin > tYMax) || (tYMin > tMax)) return false;
    if (tYMin > tMin) tMin = tYMin;
    if (tYMax < tMax) tMax = tYMax;
    float tZMin = (BoundingBox.min.z - ray.origin.z) / ray.direction.z;
    float tZMax = (BoundingBox.max.z - ray.origin.z) / ray.direction.z;
    if (tZMin > tZMax) std::swap(tZMin, tZMax);
    if ((tMin > tZMax) || (tZMin > tMax)) return false;
    return true;
}

RayHit RayTriangleIntersection(const Ray& ray, glm::vec3 v1, glm::vec3 v2, glm::vec3 v3)
{
    RayHit hit;
    glm::vec3 edge1 = v2 - v1;
    glm::vec3 edge2 = v3 - v1;

    glm::vec3 p = glm::cross(ray.direction, edge2);
    float determinant = glm::dot(edge1, p);
    if (determinant == 0.0f) return hit;
    float invDeterminant = 1.0f / determinant;

    glm::vec3 o = ray.origin - v1;
    float u = glm::dot(o, p) * invDeterminant;
    if (u < 0.0 || u > 1.0) return hit;

    glm::vec3 q = glm::cross(o, edge1);
    float v = glm::dot(ray.direction, q) * invDeterminant;
    if (v < 0.0 || u + v > 1.0) return hit;

    float dist = glm::dot(edge2, q) * invDeterminant;
    if (dist < 0.0) return hit;

    hit.position = ray.origin + ray.direction * dist;
    hit.normal = glm::normalize(glm::cross(edge1, edge2));
    hit.distance = dist;
    hit.hit = true;
    return hit;
}

// Check if position is inside bounding box
bool InBoundingBox(const glm::vec3& position, const BoundingBox& boundingBox) 
{
    bool insideX = (position.x >= boundingBox.min.x) && (position.x <= boundingBox.max.x);
    bool insideY = (position.y >= boundingBox.min.y) && (position.y <= boundingBox.max.y);
    bool insideZ = (position.z >= boundingBox.min.z) && (position.z <= boundingBox.max.z);
    return insideX && insideY && insideZ;
}