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

struct EditedChunk 
{
    int x, y, z;
    int densityCount;
    std::vector<float> densities;

    EditedChunk(int chunkWidth=0, int chunkHeight=0)
    {
        densityCount = (chunkWidth + 1) * (chunkWidth + 1) * (chunkHeight + 1);
        densities.resize(densityCount, 0.0f);
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
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, triTableMemory);
	}

    ~TerrainGPU()
    {
        // FREE TRITABLE GPU MEMORY
        glDeleteBuffers(1, &triTableMemory);
    }

	Model ConstructMeshGPU(int x, int y, int z, const std::vector<float>& densities = {})
	{
        glUseProgram(computeShaderProgram);

		Model model;
        std::vector<unsigned int> Indices;
        std::vector<float> Vertices = std::vector<float>((width + 1) * (width + 1) * (height + 1) * 48);
        std::vector<float> DensityCache = std::vector<float>((width+1)*(width+1)*(height+1), -1.0f);
        VertexHasher::ResetIndices();

        // shader uniforms
        BindUniformFloat1(computeShaderProgram, "densityThreshold", densityThreshold);
        BindUniformInt1(computeShaderProgram, "offsetX", x);
        BindUniformInt1(computeShaderProgram, "offsetY", y);
        BindUniformInt1(computeShaderProgram, "offsetZ", z);
        BindUniformInt1(computeShaderProgram, "densityCount", densities.size());

        GLuint vbo1, vbo3, vbo4; 

		// Bind buffer for vertices
		glGenBuffers(1, &vbo1);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, vbo1);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * Vertices.size(), Vertices.data(), GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vbo1);

        // Bind buffer for densities
        glGenBuffers(1, &vbo3);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, vbo3);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * densities.size(), densities.data(), GL_STATIC_READ);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, vbo3);

        // Bind buffer for density cache
        glGenBuffers(1, &vbo4);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, vbo4);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * DensityCache.size(), DensityCache.data(), GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, vbo4);

        // use compute shader
        glUseProgram(computeShaderProgram);
        glDispatchCompute(width, height, width);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
        glFinish();

		// Copy vertex data from GPU to CPU
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, vbo1); 
		GLfloat* verticesDataPtr = nullptr;
		verticesDataPtr = (GLfloat*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
		if (verticesDataPtr) {
			Vertices.assign(verticesDataPtr, verticesDataPtr + Vertices.size());
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		}

        // free GPU memory
        glDeleteBuffers(1, &vbo1);
        glDeleteBuffers(1, &vbo3);
        glDeleteBuffers(1, &vbo4);

		float vertOffsetX = width * -0.5f + 0.5f;
		float vertOffsetY = width * -0.5f + 0.5f;
		float vertOffsetZ = width * -0.5f + 0.5f;

        // set model vertices and indices
        for (int i=0; i<Vertices.size(); i+=12)
        {
            if (Vertices[i] != -1.0f)
            {
                // vertex 1
                if (VertexHasher::GetVertexIndex(Vertices[i], Vertices[i+1], Vertices[i+2]) == -1)
                {
                    VertexHasher::SetVertexIndex(Vertices[i], Vertices[i+1], Vertices[i+2], model.VertexCount());
                    model.indices.push_back(model.VertexCount());
                    model.vertices.push_back(Vertices[i] + vertOffsetX);
                    model.vertices.push_back(Vertices[i+1] + vertOffsetY);
                    model.vertices.push_back(Vertices[i+2] + vertOffsetZ);
                    model.vertices.push_back(Vertices[i+9]);
                    model.vertices.push_back(Vertices[i+10]);
                    model.vertices.push_back(Vertices[i+11]);
                }
                else
                {
                    model.indices.push_back(VertexHasher::GetVertexIndex(Vertices[i], Vertices[i+1], Vertices[i+2]));
                }

                // vertex 2
                if (VertexHasher::GetVertexIndex(Vertices[i+3], Vertices[i+4], Vertices[i+5]) == -1)
                {
                    VertexHasher::SetVertexIndex(Vertices[i+3], Vertices[i+4], Vertices[i+5], model.VertexCount());
                    model.indices.push_back(model.VertexCount());
                    model.vertices.push_back(Vertices[i+3] + vertOffsetX);
                    model.vertices.push_back(Vertices[i+4] + vertOffsetY);
                    model.vertices.push_back(Vertices[i+5] + vertOffsetZ);
                    model.vertices.push_back(Vertices[i+9]);
                    model.vertices.push_back(Vertices[i+10]);
                    model.vertices.push_back(Vertices[i+11]);
                }
                else
                {
                    model.indices.push_back(VertexHasher::GetVertexIndex(Vertices[i+3], Vertices[i+4], Vertices[i+5]));
                }

                // vertex 3
                if (VertexHasher::GetVertexIndex(Vertices[i+6], Vertices[i+7], Vertices[i+8]) == -1)
                {
                    VertexHasher::SetVertexIndex(Vertices[i+6], Vertices[i+7], Vertices[i+8], model.VertexCount());
                    model.indices.push_back(model.VertexCount());
                    model.vertices.push_back(Vertices[i+6] + vertOffsetX);
                    model.vertices.push_back(Vertices[i+7] + vertOffsetY);
                    model.vertices.push_back(Vertices[i+8] + vertOffsetZ);
                    model.vertices.push_back(Vertices[i+9]);
                    model.vertices.push_back(Vertices[i+10]);
                    model.vertices.push_back(Vertices[i+11]);
                }
                else
                {
                    model.indices.push_back(VertexHasher::GetVertexIndex(Vertices[i+6], Vertices[i+7], Vertices[i+8]));
                }
            }
        }

		model.position = {x, y, z};
        model.boundingBox.min = glm::vec3(x + width/2, y + height/2, z + width/2);
        model.boundingBox.max = glm::vec3(x - width/2, y - height/2, z - width/2);
        model.boundingBox.isFilled = true;
        return model;
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