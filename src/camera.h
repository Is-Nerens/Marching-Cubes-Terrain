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
    float yaw;
    float pitch;
} Camera; 

void CameraUpdate(Camera* camera)
{
    versor invRot;
    glm_quat_inv(camera->rotation, invRot);
    mat4 rot;
    glm_quat_mat4(invRot, rot);
    mat4 trans;
    glm_mat4_identity(trans);
    vec3 negPos;
    glm_vec3_negate_to(camera->position, negPos);
    glm_translate(trans, negPos);
    glm_mat4_mul(rot, trans, camera->viewMatrix);
    glm_mat4_mul(camera->projectionMatrix,
                 camera->viewMatrix,
                 camera->projectionViewMatrix);
    vec3 f = { 0.0f, 0.0f, -1.0f };
    vec3 r = { 1.0f, 0.0f,  0.0f };
    vec3 u = { 0.0f, 1.0f,  0.0f };
    glm_quat_rotatev(camera->rotation, f, camera->forward);
    glm_quat_rotatev(camera->rotation, r, camera->right);
    glm_quat_rotatev(camera->rotation, u, camera->up);
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
    camera->yaw = -90.0f;
    camera->pitch = 0.0f;

    // projection matrix
    glm_perspective(glm_rad(70.0f), 16.0f / 9.0f, 0.1f, 2000.0f, camera->projectionMatrix);

    // update internals
    CameraUpdate(camera);
}

void CameraMouseMove(Camera* camera, float dx, float dy)
{
    camera->yaw -= dx * 0.002f * 180.0f;
    camera->pitch -= dy * 0.002f * 180.0f;
    if (camera->pitch > 89.0f) camera->pitch = 89.0f;
    if (camera->pitch < -89.0f) camera->pitch = -89.0f;
    versor qYaw, qPitch;
    glm_quatv(qYaw, glm_rad(camera->yaw), (vec3){0.0f, 1.0f, 0.0f});
    glm_quatv(qPitch, glm_rad(camera->pitch), (vec3){1.0f, 0.0f, 0.0f});
    glm_quat_mul(qYaw, qPitch, camera->rotation);
    glm_quat_normalize(camera->rotation);
}

void CameraMove(Camera* camera, float deltaTime)
{
    const bool *state = SDL_GetKeyboardState(NULL);
    vec3 move = { 0.0f, 0.0f, 0.0f };
    if (state[SDL_SCANCODE_W]) {
        glm_vec3_add(move, camera->forward, move);
    }
    if (state[SDL_SCANCODE_A]) {
        vec3 left;
        glm_vec3_scale(camera->right, -1.0f, left);
        glm_vec3_add(move, left, move);
    }
    if (state[SDL_SCANCODE_S]) {
        vec3 backward;
        glm_vec3_scale(camera->forward, -1.0f, backward);
        glm_vec3_add(move, backward, move);
    }
    if (state[SDL_SCANCODE_D]) {
        glm_vec3_add(move, camera->right, move);
    }
    if (state[SDL_SCANCODE_E]) {
        glm_vec3_add(move, camera->up, move);
    }
    if (state[SDL_SCANCODE_Q]) {
        vec3 down;
        glm_vec3_scale(camera->up, -1.0f, down);
        glm_vec3_add(move, down, move);
    }
    glm_vec3_normalize(move);
    vec3 move_scaled;
    glm_vec3_scale(move, deltaTime * 15.0f, move_scaled);
    glm_vec3_add(camera->position, move_scaled, camera->position);
}