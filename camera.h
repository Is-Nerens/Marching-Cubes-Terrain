#pragma once
#include <cglm/cglm.h>

typedef struct Camera {
    mat4 viewMatrix;
    mat4 projectionMatrix;
    mat4 projectionViewMatrix;
    versor rotation;
    vec3 position;
    vec3 forward;
    vec3 up;
    vec3 right;
} Camera; 

void CameraUpdate(Camera* camera)
{
    // calculate view and projectionView matrices
    vec3 negatedPosition;
    glm_vec3_scale(camera->position, -1.0f, negatedPosition);
    mat4 translationMatrix;
    mat4 identity;
    glm_mat4_identity(identity);
    glm_translate_to(identity, negatedPosition, translationMatrix);
    mat4 rotationMatrix;
    glm_quat_mat4(camera->rotation, rotationMatrix);
    glm_mat4_mul(rotationMatrix, translationMatrix, camera->viewMatrix);
    glm_mat4_mul(camera->projectionMatrix, camera->viewMatrix, camera->projectionViewMatrix);
    
    // update dir unit vectors
    camera->forward[0] = -camera->viewMatrix[0][2];
    camera->forward[1] = -camera->viewMatrix[1][2];
    camera->forward[2] = -camera->viewMatrix[2][2];
    camera->up[0]      = camera->viewMatrix[0][1];
    camera->up[1]      = camera->viewMatrix[1][1];
    camera->up[2]      = camera->viewMatrix[2][1];
    camera->right[0]   = camera->viewMatrix[0][0];
    camera->right[1]   = camera->viewMatrix[1][0];
    camera->right[2]   = camera->viewMatrix[2][0];
}

void CameraInit(Camera* camera)
{
    // position
    camera->position[0] = 0.0;
    camera->position[1] = 0.0;
    camera->position[2] = 0.0;

    // rotation
    camera->rotation[0] = 0.0f;
    camera->rotation[1] = 0.0f;
    camera->rotation[2] = 0.0f;
    camera->rotation[3] = 1.0f;

    // projection matrix
    glm_perspective(glm_rad(70.0f), 16.0f / 9.0f, 0.1f, 2000.0f, camera->projectionMatrix);

    // update internals
    CameraUpdate(camera);
}