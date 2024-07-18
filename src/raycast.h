#pragma once

struct RayHit {
    glm::vec3 position;
    glm::vec3 normal;
    float distance;
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

    glm::vec3 pvec = glm::cross(ray.direction, edge2);
    float determinant = glm::dot(edge1, pvec);

    const float EPSILON = 1e-8f;
    if (fabs(determinant) < EPSILON) {
        // The ray is parallel to the triangle plane
        return hit;
    }

    float invDeterminant = 1.0f / determinant;

    glm::vec3 tvec = ray.origin - v1;
    float u = glm::dot(tvec, pvec) * invDeterminant;
    if (u < 0.0f || u > 1.0f) return hit;

    glm::vec3 qvec = glm::cross(tvec, edge1);
    float v = glm::dot(ray.direction, qvec) * invDeterminant;
    if (v < 0.0f || u + v > 1.0f) return hit;

    float t = glm::dot(edge2, qvec) * invDeterminant;
    if (t < EPSILON) return hit;

    hit.position = ray.origin + ray.direction * t;
    hit.normal = glm::normalize(glm::cross(edge1, edge2));
    hit.distance = t;
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

