#pragma once

#include <vector>


class Model {
public:
    Model() {
        // vertices = {
        //     // Front face
        //     -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  // Bottom-left
        //     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,  // Bottom-right
        //     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,  // Top-right
        //     -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,  // Top-left

        //     // Back face
        //     -0.5f, -0.5f, -0.5f,  1.0f, 0.0f,  // Bottom-left
        //     0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  // Bottom-right
        //     0.5f,  0.5f, -0.5f,  0.0f, 1.0f,  // Top-right
        //     -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  // Top-left

        //     // Left face
        //     -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  // Bottom-front
        //     -0.5f, -0.5f,  0.5f,  1.0f, 0.0f,  // Bottom-back
        //     -0.5f,  0.5f,  0.5f,  1.0f, 1.0f,  // Top-back
        //     -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,  // Top-front

        //     // Right face
        //     0.5f, -0.5f, -0.5f,  1.0f, 0.0f,  // Bottom-front
        //     0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  // Bottom-back
        //     0.5f,  0.5f,  0.5f,  0.0f, 1.0f,  // Top-back
        //     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  // Top-front

        //     // Top face
        //     -0.5f,  0.5f, -0.5f,  0.0f, 0.0f,  // Front-left
        //     0.5f,  0.5f, -0.5f,  1.0f, 0.0f,  // Front-right
        //     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,  // Back-right
        //     -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,  // Back-left

        //     // Left face
        //     -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  // Front-left
        //     0.5f, -0.5f, -0.5f,  1.0f, 0.0f,  // Front-right
        //     0.5f, -0.5f,  0.5f,  1.0f, 1.0f,  // Back-right
        //     -0.5f, -0.5f,  0.5f,  0.0f, 1.0f   // Back-left
        // };

        // indices = {
        //     // Front face
        //     0, 1, 2,
        //     2, 3, 0,

        //     // Back face
        //     4, 5, 6,
        //     6, 7, 4,

        //     // Left face
        //     8, 9, 10,
        //     10, 11, 8,

        //     // Right face
        //     12, 13, 14,
        //     14, 15, 12,

        //     // Top face
        //     16, 17, 18,
        //     18, 19, 16,

        //     // Left face
        //     20, 21, 22,
        //     20, 22, 23
        // };
    }

    void AddVertex(float x, float y, float z, float nx, float ny, float nz) //void AddVertex(float x, float y, float z, float nx, float ny, float nz, float u, float v)
    {
        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(z);
        vertices.push_back(nx);
        vertices.push_back(ny);
        vertices.push_back(nz);
        // vertices.push_back(u);
        // vertices.push_back(v);
    }

    int VertexCount()
    {
        return static_cast<int>(vertices.size()/6);
    }
    
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
};
