#ifndef MARCHING_CUBES_GPU_H
#define MARCHING_CUBES_GPU_H

#include "../vendor/glm/glm.hpp"
#include "../model.h"
#include "../shader.h"
#include "../error.h"
#include "tables.h"
#include "vertex_hashmap.h"
#include "direct_addressor.h"
#include <vector>
#include <omp.h>
#include <GL/glew.h>
#include <iostream>
#include <functional>
#include <algorithm>
#include <string>
#include <tuple>
#include <chrono>



/*
The speed error has nothing to do with offsets
it has nothing to do with the hash function
nothing to do with the GenerateVolume
generating perlin noise on the gpu is ~3.5 ms compared to ~16 ms on the cpu
the app runs faster when mesh uses an index buffer, however the generation takes longer
chunk size of 24 generates in less than half the time of 32 * 32 chunk size
chunk unloading has nothing to do with the slowdown
the GPU perlin noise system needs to have octaves and a surface
i could shave off 2 - 3 ms if i improve the vertex hash speed
*/

struct Chunk 
{
    int x;
    int y;
    int z;
    bool checked = false;
    bool regenerate = false;
    std::vector<float> densities;
    ~Chunk() {
        densities.clear();
    }
};

class TerrainGPU {
public:

	int width = 12;
	int height = 12;

	TerrainGPU()
	{
        // LOAD COMPUTESHADER FROM FILE
        std::string compShaderSource = LoadShader("./shaders/marching_cubes.compute");
        computeShaderProgram = CreateComputeShader(compShaderSource);

        // LOAD TRI TABLE
		TriTableValues = LoadTriTableValues();

        // Bind buffer for TriTable
        glUseProgram(computeShaderProgram);
        glGenBuffers(1, &triTableMemory);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, triTableMemory);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * 4096, TriTableValues.data(), GL_STATIC_READ); 
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, triTableMemory);
	}

    ~TerrainGPU()
    {
        // FREE TRITABLE GPU MEMORY
        glDeleteBuffers(1, &triTableMemory);
    }

    void GenerateMeshes(std::vector<Chunk*>& chunks, std::vector<Model*>& modelPtrs)
    {
        glUseProgram(computeShaderProgram);

        
        std::vector<unsigned int> Indices;
        std::vector<float> Vertices = std::vector<float>((width+1)*(width+1)*(height + 1) * 48 * chunks.size());
        std::vector<float> DensityCache = std::vector<float>((width+1)*(width+1)*(height+1) * chunks.size(), -1.0f);
        std::vector<float> densities;
        std::vector<int> offsets; // x0, y0, z0, x1, y1, z1...
        std::vector<int> editBooleans; // 0, 1, 1...

        for (int i=0; i<chunks.size(); ++i)
        {
            offsets.push_back(chunks[i]->x);
            offsets.push_back(chunks[i]->y);
            offsets.push_back(chunks[i]->z);
            editBooleans.push_back(chunks[i]->densities.size()>0);

            for (int j=0; j<chunks[i]->densities.size(); ++j) {
                densities.push_back(chunks[i]->densities[j]);
            }
        }

        // shader uniforms
        BindUniformFloat1(computeShaderProgram, "densityThreshold", densityThreshold);
        BindUniformInt1(computeShaderProgram, "chunkCount", chunks.size());

        GLuint vertBuffer, densityBuffer, densityCache, offsetsBuffer, editFlags; 

        // Bind buffer for vertices
		glGenBuffers(1, &vertBuffer);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, vertBuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * Vertices.size(), Vertices.data(), GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, vertBuffer);

        // Bind buffer for densities
        glGenBuffers(1, &densityBuffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, densityBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * densities.size(), densities.data(), GL_STATIC_READ);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, densityBuffer);

        // Bind buffer for density cache
        glGenBuffers(1, &densityCache);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, densityCache);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * DensityCache.size(), DensityCache.data(), GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, densityCache);

        // Bind buffer for chunk offsets
        glGenBuffers(1, &offsetsBuffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, offsetsBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * offsets.size(), offsets.data(), GL_STATIC_READ);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, offsetsBuffer);

        // Bind buffer for edit flags
        glGenBuffers(1, &editFlags);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, editFlags);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * editBooleans.size(), editBooleans.data(), GL_STATIC_READ);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, editFlags);

        // Compute
        glUseProgram(computeShaderProgram);
        glDispatchCompute(width, height, width);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
        glFinish();

        // Copy vertex data from GPU to CPU
        std::vector<std::vector<float>> chunksVertices(chunks.size());
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, vertBuffer); 
		GLfloat* verticesDataPtr = nullptr;
		verticesDataPtr = (GLfloat*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
		if (verticesDataPtr) 
        {
            int size = (width+1)*(width+1)*(height+1) * 48;
            for (int i=0; i<chunks.size(); ++i)
            {
                int index = i * size;
                chunksVertices[i] = std::vector<float>(size);
                std::memcpy(chunksVertices[i].data(), verticesDataPtr + index, size * sizeof(float));
            }
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		}

        // Free GPU memory
        glDeleteBuffers(1, &vertBuffer);
        glDeleteBuffers(1, &densityBuffer);
        glDeleteBuffers(1, &densityCache);
        glDeleteBuffers(1, &offsetsBuffer);
        glDeleteBuffers(1, &editFlags);


        // CPU OPERATIONS - CLEANING RAW VERTEX DATA 
        float vertOffsetX = width * -0.5f + 0.5f;
		float vertOffsetY = height * -0.5f + 0.5f;
		float vertOffsetZ = width * -0.5f + 0.5f;

        // FOR EACH CHUNK
        #pragma omp parallel for
        for (int c=0; c<chunks.size(); ++c)
        {
            Model& model = *modelPtrs[c];
            std::vector<float>& vertices = chunksVertices[c];
            VertexHasher vertexHasher;
            
            for (int i=0; i<vertices.size(); i+=12)
            {
                if (vertices[i] != -1.0f)
                {
                    // vertex 1
                    if (vertexHasher.GetVertexIndex(vertices[i], vertices[i+1], vertices[i+2]) == -1)
                    {
                        vertexHasher.SetVertexIndex(vertices[i], vertices[i+1], vertices[i+2], model.VertexCount());
                        model.indices.push_back(model.VertexCount());
                        model.vertices.push_back(vertices[i] + vertOffsetX);
                        model.vertices.push_back(vertices[i+1] + vertOffsetY);
                        model.vertices.push_back(vertices[i+2] + vertOffsetZ);
                        model.vertices.push_back(vertices[i+9]);
                        model.vertices.push_back(vertices[i+10]);
                        model.vertices.push_back(vertices[i+11]);
                    }
                    else
                    {
                        model.indices.push_back(vertexHasher.GetVertexIndex(vertices[i], vertices[i+1], vertices[i+2]));
                    }

                    // vertex 2
                    if (vertexHasher.GetVertexIndex(vertices[i+3], vertices[i+4], vertices[i+5]) == -1)
                    {
                        vertexHasher.SetVertexIndex(vertices[i+3], vertices[i+4], vertices[i+5], model.VertexCount());
                        model.indices.push_back(model.VertexCount());
                        model.vertices.push_back(vertices[i+3] + vertOffsetX);
                        model.vertices.push_back(vertices[i+4] + vertOffsetY);
                        model.vertices.push_back(vertices[i+5] + vertOffsetZ);
                        model.vertices.push_back(vertices[i+9]);
                        model.vertices.push_back(vertices[i+10]);
                        model.vertices.push_back(vertices[i+11]);
                    }
                    else
                    {
                        model.indices.push_back(vertexHasher.GetVertexIndex(vertices[i+3], vertices[i+4], vertices[i+5]));
                    }

                    // vertex 3
                    if (vertexHasher.GetVertexIndex(vertices[i+6], vertices[i+7], vertices[i+8]) == -1)
                    {
                        vertexHasher.SetVertexIndex(vertices[i+6], vertices[i+7], vertices[i+8], model.VertexCount());
                        model.indices.push_back(model.VertexCount());
                        model.vertices.push_back(vertices[i+6] + vertOffsetX);
                        model.vertices.push_back(vertices[i+7] + vertOffsetY);
                        model.vertices.push_back(vertices[i+8] + vertOffsetZ);
                        model.vertices.push_back(vertices[i+9]);
                        model.vertices.push_back(vertices[i+10]);
                        model.vertices.push_back(vertices[i+11]);
                    }
                    else
                    {
                        model.indices.push_back(vertexHasher.GetVertexIndex(vertices[i+6], vertices[i+7], vertices[i+8]));
                    }
                }
            }

            model.position = {chunks[c]->x, chunks[c]->y, chunks[c]->z};
            model.boundingBox.min = glm::vec3(chunks[c]->x + width/2, chunks[c]->y + height/2, chunks[c]->z + width/2);
            model.boundingBox.max = glm::vec3(chunks[c]->x - width/2, chunks[c]->y - height/2, chunks[c]->z - width/2);
            model.boundingBox.isFilled = true;
        }
    }

private:
    float densityThreshold = 0.7f;
    unsigned int computeShaderProgram;
	std::vector<int> TriTableValues;
    GLuint triTableMemory;

	std::vector<int> LoadTriTableValues()
    {
        std::vector<int> data;
        for (int y=0; y<256; ++y)
        {
            for (int x=0; x< 16; ++x)
            {
                data.push_back(TriTable[y][x]);
            }
        }
        return data;
    }
};
#endif