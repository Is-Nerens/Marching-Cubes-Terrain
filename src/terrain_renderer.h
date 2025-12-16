#pragma once

typedef struct TerrainRenderer {
    GLuint shaderProgram;
    Texture rockAlbedo;
    Texture rockNormal;
    Texture grassAlbedo;
    Texture grassNormal;
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
    TextureLoadFile(&renderer->rockAlbedo, "textures/grass_albedo.jpg");
    TextureLoadFile(&renderer->rockNormal, "textures/grass_normal.jpg");
    glUniform1i(glGetUniformLocation(renderer->shaderProgram, "u_rock_albedo_texture"), 0); 
    glUniform1i(glGetUniformLocation(renderer->shaderProgram, "u_rock_normal_texture"), 1); 
    glUniform1i(glGetUniformLocation(renderer->shaderProgram, "u_grass_albedo_texture"), 2); 
    glUniform1i(glGetUniformLocation(renderer->shaderProgram, "u_grass_normal_texture"), 3); 
}

void TerrainRendererFree(TerrainRenderer* renderer)
{
    TextureFree(&renderer->rockAlbedo);
    TextureFree(&renderer->rockNormal);
    TextureFree(&renderer->grassAlbedo);
    TextureFree(&renderer->grassNormal);
}

void DrawTerrain(TerrainRenderer* renderer, Terrain* terrain, Camera* camera)
{
    glUseProgram(renderer->shaderProgram);

    for (int i=0; i<terrain->chunks.size; i++)
    {
        Mesh* mesh = MeshArray_Get(&terrain->chunks, i);

        // compute mvp
        mat4 mvp;
        glm_mat4_mul(camera->projectionViewMatrix, mesh->meshMatrix, mvp);

        // set shader uniforms
        glUniformMatrix4fv(renderer->mvpUniformLocation, 1, GL_FALSE, (const GLfloat*)mvp);
        glUniformMatrix4fv(renderer->modelMatrixUniformLocation, 1, GL_FALSE, (const GLfloat*)mesh->meshMatrix);
        glUniform3fv(renderer->cameraPosUniformLocation, 1, camera->position);

        // draw
        MeshUploadToGPU(mesh);
        glBindVertexArray(mesh->VAO);
        glDrawElements(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, NULL);
    }
}