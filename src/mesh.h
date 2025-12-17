#pragma once

typedef struct Mesh {
    float* vertexBuffer;
    uint32_t* indexBuffer;
    uint32_t vertexCapacity;
    uint32_t indexCapacity;
    uint32_t vertexCount;
    uint32_t indexCount;
    GLuint VAO;
    GLuint VBO;
    GLuint IBO;
    float x, y, z;
    bool uploaded;
} Mesh;

void MeshInit(Mesh* mesh, uint32_t vertexCapacity, uint32_t indexCapacity)
{
    mesh->vertexCapacity = vertexCapacity;
    mesh->indexCapacity = indexCapacity;
    mesh->vertexCount = 0;
    mesh->indexCount = 0;
    mesh->vertexBuffer = malloc(6 * sizeof(float) * vertexCapacity);
    mesh->indexBuffer = malloc(sizeof(uint32_t) * indexCapacity);
    mesh->x = 0.0f;
    mesh->y = 0.0f;
    mesh->z = 0.0f;
    mesh->uploaded = false;
}

void MeshFree(Mesh* mesh)
{
    free(mesh->vertexBuffer);
    free(mesh->indexBuffer);
    if (mesh->uploaded) {
        glDeleteBuffers(1, &mesh->VBO);
        glDeleteVertexArrays(1, &mesh->VAO);
        glDeleteBuffers(1, &mesh->IBO);
    }
    mesh->uploaded = false;
    mesh->VAO = 0;
    mesh->VBO = 0;
    mesh->IBO = 0;
}

void MeshClearCPU(Mesh* mesh)
{
    mesh->vertexCount = 0;
    mesh->indexCount = 0;
}

void MeshUploadToGPU(Mesh* mesh)
{
    if (!mesh->uploaded)
    {
        glGenVertexArrays(1, &mesh->VAO);
        glGenBuffers(1, &mesh->VBO);
        glGenBuffers(1, &mesh->IBO);
        glBindVertexArray(mesh->VAO);
        glBindBuffer(GL_ARRAY_BUFFER, mesh->VBO);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->IBO);
        glBindVertexArray(0);
    }

    // upload data
    glBindVertexArray(mesh->VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->VBO);
    glBufferData(GL_ARRAY_BUFFER, mesh->vertexCount * 6 * sizeof(float), mesh->vertexBuffer, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->indexCount * sizeof(uint32_t), mesh->indexBuffer, GL_DYNAMIC_DRAW);
    glBindVertexArray(0);
    mesh->uploaded = true;
}

void MeshFreeGPU(Mesh* mesh)
{
    if (!mesh->uploaded) return;
    glDeleteBuffers(1, &mesh->VBO);
    glDeleteVertexArrays(1, &mesh->VAO);
    glDeleteBuffers(1, &mesh->IBO);
    mesh->uploaded = false;
    mesh->VAO = 0;
    mesh->VBO = 0;
    mesh->IBO = 0;
}

void MeshGrowVertexCapacity(Mesh* mesh)
{
    mesh->vertexCapacity *= 2;
    mesh->vertexBuffer = realloc(mesh->vertexBuffer, mesh->vertexCapacity * 6 * sizeof(float));
}

void MeshGrowIndexCapacity(Mesh* mesh)
{
    mesh->indexCapacity *= 2;
    mesh->indexBuffer = realloc(mesh->indexBuffer, mesh->indexCapacity * sizeof(uint32_t));
}

inline void MeshAddFace(Mesh* mesh, vec3 v1, vec3 v2, vec3 v3, vec3 normal)
{
    if (mesh->vertexCount + 3 > mesh->vertexCapacity) {
        MeshGrowVertexCapacity(mesh);
    }

    if (mesh->indexCount + 3 > mesh->indexCapacity) {
        MeshGrowIndexCapacity(mesh);
    }

    // v1
    uint32_t vert_i = mesh->vertexCount * 6; 
    mesh->vertexBuffer[vert_i   ]   = v1[0];
    mesh->vertexBuffer[vert_i + 1 ] = v1[1];
    mesh->vertexBuffer[vert_i + 2 ] = v1[2];
    mesh->vertexBuffer[vert_i + 3 ] = normal[0];
    mesh->vertexBuffer[vert_i + 4 ] = normal[1];
    mesh->vertexBuffer[vert_i + 5 ] = normal[2];

    // v2
    mesh->vertexBuffer[vert_i + 6 ] = v2[0];
    mesh->vertexBuffer[vert_i + 7 ] = v2[1];
    mesh->vertexBuffer[vert_i + 8 ] = v2[2];
    mesh->vertexBuffer[vert_i + 9 ] = normal[0];
    mesh->vertexBuffer[vert_i + 10] = normal[1];
    mesh->vertexBuffer[vert_i + 11] = normal[2];

    // v3
    mesh->vertexBuffer[vert_i + 12 ] = v3[0];
    mesh->vertexBuffer[vert_i + 13 ] = v3[1];
    mesh->vertexBuffer[vert_i + 14 ] = v3[2];
    mesh->vertexBuffer[vert_i + 15 ] = normal[0];
    mesh->vertexBuffer[vert_i + 16 ] = normal[1];
    mesh->vertexBuffer[vert_i + 17 ] = normal[2];

    // indices
    mesh->indexBuffer[mesh->indexCount  ] = mesh->vertexCount;
    mesh->indexBuffer[mesh->indexCount+1] = mesh->vertexCount+1;
    mesh->indexBuffer[mesh->indexCount+2] = mesh->vertexCount+2;

    // update counts
    mesh->vertexCount += 3;
    mesh->indexCount += 3;
}

inline void MeshChunkGenerateAddFace(Mesh* mesh, float* verts)
{
    if (mesh->vertexCount + 3 > mesh->vertexCapacity) {
        MeshGrowVertexCapacity(mesh);
    }

    if (mesh->indexCount + 3 > mesh->indexCapacity) {
        MeshGrowIndexCapacity(mesh);
    }

    // v1
    uint32_t vert_i = mesh->vertexCount * 6; 
    mesh->vertexBuffer[vert_i   ]   = verts[0];
    mesh->vertexBuffer[vert_i + 1 ] = verts[1];
    mesh->vertexBuffer[vert_i + 2 ] = verts[2];
    mesh->vertexBuffer[vert_i + 3 ] = verts[9];
    mesh->vertexBuffer[vert_i + 4 ] = verts[10];
    mesh->vertexBuffer[vert_i + 5 ] = verts[11];

    // v2
    mesh->vertexBuffer[vert_i + 6 ] = verts[3];
    mesh->vertexBuffer[vert_i + 7 ] = verts[4];
    mesh->vertexBuffer[vert_i + 8 ] = verts[5];
    mesh->vertexBuffer[vert_i + 9 ] = verts[9];
    mesh->vertexBuffer[vert_i + 10] = verts[10];
    mesh->vertexBuffer[vert_i + 11] = verts[11];

    // v3
    mesh->vertexBuffer[vert_i + 12 ] = verts[6];
    mesh->vertexBuffer[vert_i + 13 ] = verts[7];
    mesh->vertexBuffer[vert_i + 14 ] = verts[8];
    mesh->vertexBuffer[vert_i + 15 ] = verts[9];
    mesh->vertexBuffer[vert_i + 16 ] = verts[10];
    mesh->vertexBuffer[vert_i + 17 ] = verts[11];

    // indices
    mesh->indexBuffer[mesh->indexCount  ] = mesh->vertexCount;
    mesh->indexBuffer[mesh->indexCount+1] = mesh->vertexCount+1;
    mesh->indexBuffer[mesh->indexCount+2] = mesh->vertexCount+2;

    // update counts
    mesh->vertexCount += 3;
    mesh->indexCount += 3;
}