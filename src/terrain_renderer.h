#pragma once

typedef struct TerrainRenderer {
    GLuint shaderProgram;
    Texture rockAlbedo;
    Texture rockNormal;
    Texture grassAlbedo;
    Texture grassNormal;
    int rockAlbedoLoc;
    int rockNormalLoc;
    int grassAlbedoLoc;
    int grassNormalLoc;
    int cameraPosUniformLocation;
    int mvpUniformLocation;
    int modelMatrixUniformLocation;
} TerrainRenderer;

void TerrainRendererInit(TerrainRenderer* renderer)
{
    renderer->shaderProgram = Create_Shader_Program_From_File("shaders/chunk_vert.glsl", "shaders/chunk_frag.glsl");
    renderer->cameraPosUniformLocation = glGetUniformLocation(renderer->shaderProgram, "u_CameraPos");
    renderer->mvpUniformLocation = glGetUniformLocation(renderer->shaderProgram, "u_MVP");
    renderer->modelMatrixUniformLocation = glGetUniformLocation(renderer->shaderProgram, "u_ModelPositionMatrix");
    TextureLoadFile(&renderer->rockAlbedo, "textures/rock_albedo.jpg");
    TextureLoadFile(&renderer->rockNormal, "textures/rock_normal.jpg");
    TextureLoadFile(&renderer->grassAlbedo, "textures/grass_albedo.jpg");
    TextureLoadFile(&renderer->grassNormal, "textures/grass_normal.jpg");
    renderer->rockAlbedoLoc = glGetUniformLocation(renderer->shaderProgram, "u_rock_albedo_texture");
    renderer->rockNormalLoc = glGetUniformLocation(renderer->shaderProgram, "u_rock_normal_texture");
    renderer->grassAlbedoLoc = glGetUniformLocation(renderer->shaderProgram, "u_grass_albedo_texture");
    renderer->grassNormalLoc = glGetUniformLocation(renderer->shaderProgram, "u_grass_normal_texture");
}

void TerrainRendererFree(TerrainRenderer* renderer)
{
    TextureFree(&renderer->rockAlbedo);
    TextureFree(&renderer->rockNormal);
    TextureFree(&renderer->grassAlbedo);
    TextureFree(&renderer->grassNormal);
}

bool IsAABBInFrustum(AABB* aabb, mat4 viewProjectionMatrix)
{
    vec3 center = {
        (aabb->minX + aabb->maxX) * 0.5f,
        (aabb->minY + aabb->maxY) * 0.5f,
        (aabb->minZ + aabb->maxZ) * 0.5f
    };
    
    vec3 extents = {
        (aabb->maxX - aabb->minX) * 0.5f,
        (aabb->maxY - aabb->minY) * 0.5f,
        (aabb->maxZ - aabb->minZ) * 0.5f
    };
    
    // Transform center to clip space
    vec4 clipCenter;
    glm_mat4_mulv3(viewProjectionMatrix, center, 1.0f, clipCenter);
    float w = clipCenter[3];
    
    // Quick reject if center is far outside
    // Use array indexing instead of .x, .y, .z
    if (clipCenter[0] + extents[0] < -w || clipCenter[0] - extents[0] > w ||
        clipCenter[1] + extents[1] < -w || clipCenter[1] - extents[1] > w ||
        clipCenter[2] + extents[2] < -w || clipCenter[2] - extents[2] > w) {
        return false;
    }
    
    return true;
}

void DrawTerrain(TerrainRenderer* renderer, Terrain* terrain, Camera* camera)
{
    glUseProgram(renderer->shaderProgram);

    // Bind textures to texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer->rockAlbedo.handle);
    glUniform1i(renderer->rockAlbedoLoc, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, renderer->rockNormal.handle);
    glUniform1i(renderer->rockNormalLoc, 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, renderer->grassAlbedo.handle);
    glUniform1i(renderer->grassAlbedoLoc, 2);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, renderer->grassNormal.handle);
    glUniform1i(renderer->grassNormalLoc, 3);

    for (int i=0; i<terrain->chunks.size; i++)
    {
        Mesh* mesh = MeshArray_Get(&terrain->chunks, i);
        if (mesh->indexCount == 0) continue;

        // compute mesh matrix
        mat4 identity;
        mat4 meshMatrix;
        glm_mat4_identity(identity);
        vec3 position = { mesh->x, mesh->y, mesh->z };
        glm_translate_to(identity, position, meshMatrix);

        // compute mvp
        mat4 mvp;
        glm_mat4_mul(camera->projectionViewMatrix, meshMatrix, mvp);

        // Frustum culling - skip if not visible
        if (!IsAABBInFrustum(&mesh->aabb, mvp)) {
            continue;
        }

        // set shader uniforms
        glUniformMatrix4fv(renderer->mvpUniformLocation, 1, GL_FALSE, (const GLfloat*)mvp);
        glUniformMatrix4fv(renderer->modelMatrixUniformLocation, 1, GL_FALSE, (const GLfloat*)meshMatrix);
        glUniform3fv(renderer->cameraPosUniformLocation, 1, camera->position);

        // draw
        MeshUploadToGPU(mesh);
        glBindVertexArray(mesh->VAO);
        glDrawElements(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, NULL);
    }
}