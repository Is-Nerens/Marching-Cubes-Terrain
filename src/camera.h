#ifndef CAMERA_H
#define CAMERA_H

#include "vendor/glm/glm.hpp"
#include "vendor/glm/gtc/matrix_transform.hpp"

class Camera
{
public:
    Camera()
    {
        projMatrix = glm::perspectiveFov(glm::radians(80.0f), 2.0f, 1.5f, 1.0f, 2000.0f);
        viewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-0.1f, 0.1f, -20.0f));
        position = glm::vec3(0.0f);
        rotation = glm::vec3(0.0f);
    }

    ~Camera() {}

    const glm::mat4 GetProjectionViewMatrix() { return projViewMat; }

    void UpdateProjectionView()
    {
        // Create rotation matrices
        glm::mat4 rotationX = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 rotationZ = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

        // Combine rotation matrices
        glm::mat4 rotationMatrix = rotationX * rotationY * rotationZ;

        // Calculate the view matrix
        glm::vec3 negatedPosition = -1.0f * position;
        glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), negatedPosition);
        viewMatrix = rotationMatrix * translationMatrix;
        projViewMat = projMatrix * viewMatrix;
    }

    void SetViewport(int w, int h)
    {
        float aspectW = 1.0f;
        float aspectH = static_cast<float>(h) / static_cast<float>(w);
        projMatrix = glm::perspectiveFov(glm::radians(80.0f), aspectW, aspectH, 0.1f, 2000.0f);
    }

    void LookAt(glm::vec3 pos)
    {
        glm::vec3 direction = glm::normalize(pos - position);
        rotation.y = -glm::degrees(atan2(-direction.x, -direction.z));
        rotation.x = glm::degrees(asin(direction.y));
    }

    glm::vec3 Forward() { return -glm::vec3(viewMatrix[0][2], viewMatrix[1][2], viewMatrix[2][2]); }
    glm::vec3 Right() { return glm::vec3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]); }
    glm::vec3 Up() { return glm::vec3(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]); }





    glm::vec3 position;
    glm::vec3 rotation;

private:
    glm::mat4 projMatrix;
    glm::mat4 viewMatrix;
    glm::mat4 projViewMat;
};

#endif