#pragma once

#include <vector>
#include "vendor/glm/glm.hpp"

class Model {
public:
    Model() {}
    ~Model() {}

    void AddVertex(float x, float y, float z, float nx, float ny, float nz) 
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
    glm::vec3 position;
};
