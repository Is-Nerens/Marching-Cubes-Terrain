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
    bool isFilled = false;
};

struct Plane {
    glm::vec3 normal;
    float distance;
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

bool InBoundingBox(const glm::vec3& position, const BoundingBox& boundingBox) 
{
    bool insideX = (position.x >= boundingBox.min.x) && (position.x <= boundingBox.max.x);
    bool insideY = (position.y >= boundingBox.min.y) && (position.y <= boundingBox.max.y);
    bool insideZ = (position.z >= boundingBox.min.z) && (position.z <= boundingBox.max.z);
    return insideX && insideY && insideZ;
}

std::vector<Plane> ExtractFrustumPlanes(const glm::mat4& pvm) {
    std::vector<Plane> planes(6);

    // Left
    planes[0].normal = glm::vec3(pvm[0][3] + pvm[0][0], pvm[1][3] + pvm[1][0], pvm[2][3] + pvm[2][0]);
    planes[0].distance = pvm[3][3] + pvm[3][0];

    // Right
    planes[1].normal = glm::vec3(pvm[0][3] - pvm[0][0], pvm[1][3] - pvm[1][0], pvm[2][3] - pvm[2][0]);
    planes[1].distance = pvm[3][3] - pvm[3][0];

    // Bottom
    planes[2].normal = glm::vec3(pvm[0][3] + pvm[0][1], pvm[1][3] + pvm[1][1], pvm[2][3] + pvm[2][1]);
    planes[2].distance = pvm[3][3] + pvm[3][1];

    // Top
    planes[3].normal = glm::vec3(pvm[0][3] - pvm[0][1], pvm[1][3] - pvm[1][1], pvm[2][3] - pvm[2][1]);
    planes[3].distance = pvm[3][3] - pvm[3][1];

    // Near
    planes[4].normal = glm::vec3(pvm[0][3] + pvm[0][2], pvm[1][3] + pvm[1][2], pvm[2][3] + pvm[2][2]);
    planes[4].distance = pvm[3][3] + pvm[3][2];

    // Far
    planes[5].normal = glm::vec3(pvm[0][3] - pvm[0][2], pvm[1][3] - pvm[1][2], pvm[2][3] - pvm[2][2]);
    planes[5].distance = pvm[3][3] - pvm[3][2];

    for (auto& plane : planes) {
        float length = glm::length(plane.normal);
        plane.normal /= length;
        plane.distance /= length;
    }

    return planes;
}

bool IsBoxInFrustum(const std::vector<Plane>& planes, const glm::vec3& min, const glm::vec3& max) {
    for (const auto& plane : planes) {
        // Calculate the corner points of the AABB
        glm::vec3 corners[8] = {
            glm::vec3(min.x, min.y, min.z),
            glm::vec3(max.x, min.y, min.z),
            glm::vec3(min.x, max.y, min.z),
            glm::vec3(max.x, max.y, min.z),
            glm::vec3(min.x, min.y, max.z),
            glm::vec3(max.x, min.y, max.z),
            glm::vec3(min.x, max.y, max.z),
            glm::vec3(max.x, max.y, max.z)
        };

        // Check if all corners are outside this plane
        bool allOutside = true;
        for (const auto& corner : corners) {
            if (glm::dot(plane.normal, corner) + plane.distance >= 0) {
                allOutside = false;
                break;
            }
        }

        // If all corners are outside this plane, the box is outside the frustum
        if (allOutside) {
            return false;
        }
    }

    // If we passed all planes, the box is inside or intersects the frustum
    return true;
}


