#ifndef MARCHING_CUBES_GPU_H
#define MARCHING_CUBES_GPU_H

#include "vendor/glm/glm.hpp"
#include "tables.h"
#include "model.h"
#include "shader.h"
#include "error.h"
#include "FastNoiseLite.h"
#include <vector>
#include <GL/glew.h>
#include <unordered_map>
#include <iostream>
#include <functional>
#include <algorithm>
#include <string>
#include <tuple>
#include <chrono>

struct RayHit
{
	glm::vec3 position = glm::vec3{0.0};
	glm::vec3 normal = glm::vec3{0.0};
	float distance = 0.0f;
	bool hit = false;
};

// Custom hash function for tuple<float, float, float>
namespace std {
    template <>
    struct hash<std::tuple<float, float, float>> {
        size_t operator()(const std::tuple<float, float, float>& t) const {
            // Combine hashes of individual float values
            size_t h1 = std::hash<float>{}(std::get<0>(t));
            size_t h2 = std::hash<float>{}(std::get<1>(t));
            size_t h3 = std::hash<float>{}(std::get<2>(t));
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };
}



class TerrainGPU{
public:

	TerrainGPU()
	{
        // Load shaders from file
        std::string compShaderSource = LoadShader("./shaders/marching_cubes.compute");
        computeShaderProgram = CreateComputeShader(compShaderSource);

        // load once
        TriTableValues = LoadTriTableValues();
        GenerateVolume();
		InitNoiseGenerator();
	}

	Model ConstructMeshGPU()
	{
        auto start = std::chrono::high_resolution_clock::now();

        glUseProgram(computeShaderProgram);

		Model model;
        std::vector<float> Vertices;
        std::vector<unsigned int> Indices;
        offset = 0;

        VertexHashMap.clear();

        // shader uniforms
        int locc = BindUniformFloat1(computeShaderProgram, "densityThreshold", densityThreshold);
        BindUniformInt1(computeShaderProgram, "width", width);
        BindUniformInt1(computeShaderProgram, "height", height);
        BindUniformInt1(computeShaderProgram, "offset", offset);
        PrintGLErrors();

        GLuint vbo1, vbo2, vbo3; 

        // Bind buffer for the first vector (VolumeData)
        glGenBuffers(1, &vbo1);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, vbo1);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * VolumeData.size(), VolumeData.data(), GL_STATIC_READ);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vbo1);

        // Bind buffer Vertices
        glGenBuffers(1, &vbo2);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, vbo2);
        size_t vertexBufferSize = static_cast<size_t>((width + 1) * (width + 1) * (height + 1) * 48 * sizeof(float)); // for vertices
        glBufferData(GL_SHADER_STORAGE_BUFFER, vertexBufferSize, nullptr, GL_DYNAMIC_DRAW); // Allocate space without initializing data
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, vbo2);

        // Bind buffer for TriTable
        glGenBuffers(1, &vbo3);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, vbo3);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * 4096, TriTableValues.data(), GL_STATIC_READ); 
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, vbo3);

        // Atomic flag
        GLuint atomicFlagBuffer;
        GLuint initialAtomicValue = 0; // Initial value for the atomic integer
        glGenBuffers(1, &atomicFlagBuffer);
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomicFlagBuffer);
        glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &initialAtomicValue, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 3, atomicFlagBuffer);
        GLuint *ptr = (GLuint*)glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_READ_WRITE);
        *ptr = 0; // Initialize counter value
        glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
 
        // use compute shader
        glUseProgram(computeShaderProgram);
        glDispatchCompute(width, height, width);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
        glFinish();


        // copy vertex data from GPU to CPU
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, vbo2); 
        GLfloat* verticesDataPtr = nullptr;
        verticesDataPtr = (GLfloat*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
        if (verticesDataPtr) {
            Vertices.assign(verticesDataPtr, verticesDataPtr + ((width + 1) * (width + 1) * (height + 1) * 48));
            glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        }

        // free GPU memory
        glDeleteBuffers(1, &vbo1);
        glDeleteBuffers(1, &vbo2);
        glDeleteBuffers(1, &vbo3);


        // set model vertices and indices
        int vCount = (width + 1) * (width + 1) * (height + 1) * 48;
        for (int i=0; i<vCount; i+=12)
        {
            if (Vertices[i] != 0)
            {
                // vertex 1
                if (GetVertexIndex(Vertices[i], Vertices[i+1], Vertices[i+2]) == -1)
                {
                    SetVertexIndex(Vertices[i], Vertices[i+1], Vertices[i+2], model.VertexCount());
                    model.indices.push_back(model.VertexCount());
                    model.vertices.push_back(Vertices[i]);
                    model.vertices.push_back(Vertices[i+1]);
                    model.vertices.push_back(Vertices[i+2]);
                    model.vertices.push_back(Vertices[i+9]);
                    model.vertices.push_back(Vertices[i+10]);
                    model.vertices.push_back(Vertices[i+11]);
                }
                else
                {
                    model.indices.push_back(GetVertexIndex(Vertices[i], Vertices[i+1], Vertices[i+2]));
                }

                // vertex 2
                if (GetVertexIndex(Vertices[i+3], Vertices[i+4], Vertices[i+5]) == -1)
                {
                    SetVertexIndex(Vertices[i+3], Vertices[i+4], Vertices[i+5], model.VertexCount());
                    model.indices.push_back(model.VertexCount());
                    model.vertices.push_back(Vertices[i+3]);
                    model.vertices.push_back(Vertices[i+4]);
                    model.vertices.push_back(Vertices[i+5]);
                    model.vertices.push_back(Vertices[i+9]);
                    model.vertices.push_back(Vertices[i+10]);
                    model.vertices.push_back(Vertices[i+11]);
                }
                else
                {
                    model.indices.push_back(GetVertexIndex(Vertices[i+3], Vertices[i+4], Vertices[i+5]));
                }

                // vertex 3
                if (GetVertexIndex(Vertices[i+6], Vertices[i+7], Vertices[i+8]) == -1)
                {
                    SetVertexIndex(Vertices[i+6], Vertices[i+7], Vertices[i+8], model.VertexCount());
                    model.indices.push_back(model.VertexCount());
                    model.vertices.push_back(Vertices[i+6]);
                    model.vertices.push_back(Vertices[i+7]);
                    model.vertices.push_back(Vertices[i+8]);
                    model.vertices.push_back(Vertices[i+9]);
                    model.vertices.push_back(Vertices[i+10]);
                    model.vertices.push_back(Vertices[i+11]);
                }
                else
                {
                    model.indices.push_back(GetVertexIndex(Vertices[i+6], Vertices[i+7], Vertices[i+8]));
                }
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
        float meshGenTime = duration.count();

        std::cout << "mesh generation time: " <<  meshGenTime * 1000 << "ms" << std::endl;
        std::cout << " " << std::endl;
        std::cout << " " << std::endl;
        return model;
    }

    void AddDensity(glm::vec3 position, float amount)
	{
        int xI = int(floor(position.x + 0.5f));
        int yI = int(floor(position.y + 0.5f));
        int zI = int(floor(position.z + 0.5f));
        int index = zI + (width+1) * (yI + (height+1) * xI);
        VolumeData[index] += amount;

        if (VolumeData[index] < 0.0f) VolumeData[index] = 0.0f;
        else if (VolumeData[index] > 1.0f) VolumeData[index] = 1.0f;
	}

	RayHit Raycast(glm::vec3 start, glm::vec3 dir) 
	{
		RayHit hit;
		float maxDist = 20.0f;
		float traveled = 0.0f;
		float densityThreshold = 0.6f;
		dir = glm::normalize(dir);

		// ensure not zero
		start.x += 0.0000001f;
		start.y += 0.0000001f;
		start.z += 0.0000001f;

		// Calculate scaling factors for each axis
		float s1 = std::numeric_limits<float>::infinity();
		if (dir.x != 0){
			s1 = std::sqrt(   std::pow(std::sqrt(1 + std::pow(dir.y / dir.x, 2)), 2)   +   std::pow(std::sqrt(1 + std::pow(dir.z / dir.x, 2)), 2));  
		}
		float s2 = std::numeric_limits<float>::infinity();
		if (dir.z != 0){
			s2 = std::sqrt(   std::pow(std::sqrt(1 + std::pow(dir.x / dir.y, 2)), 2)   +   std::pow(std::sqrt(1 + std::pow(dir.z / dir.y, 2)), 2));  
		}
		float s3 = std::numeric_limits<float>::infinity();
		if (dir.z != 0){
			s3 = std::sqrt(   std::pow(std::sqrt(1 + std::pow(dir.y / dir.z, 2)), 2)   +   std::pow(std::sqrt(1 + std::pow(dir.x / dir.z, 2)), 2));  
		}
		glm::vec3 stepScaling = glm::vec3(s1, s2, s3);
		glm::vec3 mapCheck = SnapToCube(start.x, start.y, start.z);
		glm::vec3 rayLength1D;
		glm::vec3 step;


		if (dir.x < 0) {
			step.x = -1;
			rayLength1D.x = (start.x - mapCheck.x) * stepScaling.x; 
		}
		else {
			step.x = 1;
			rayLength1D.x = (mapCheck.x - start.x + 1) * stepScaling.x; 
		}

		if (dir.y < 0) {
			step.y = -1;
			rayLength1D.y = (start.y - mapCheck.y) * stepScaling.y; 
		}
		else {
			step.y = 1;
			rayLength1D.y = (mapCheck.y - start.y + 1) * stepScaling.y; 
		}

		if (dir.z < 0) {
			step.z = -1;
			rayLength1D.z = (start.x - mapCheck.x) * stepScaling.z; 
		}
		else {
			step.z = 1;
			rayLength1D.z = (mapCheck.z - start.z + 1) * stepScaling.z;
		}


		bool didHit = false;
		while (traveled < maxDist)
		{
			if (rayLength1D.x < rayLength1D.y && rayLength1D.x < rayLength1D.z){
				mapCheck.x += step.x;
				traveled = rayLength1D.x;
				rayLength1D.x += stepScaling.x;
			}
			if (rayLength1D.y < rayLength1D.x && rayLength1D.y < rayLength1D.z){
				mapCheck.y += step.y;
				traveled = rayLength1D.y;
				rayLength1D.y += stepScaling.y;
			}
			if (rayLength1D.z < rayLength1D.x && rayLength1D.z < rayLength1D.y){
				mapCheck.z += step.z;
				traveled = rayLength1D.z;
				rayLength1D.z += stepScaling.z;
			}


			// boundary check
			int offset = width/2;
			float minBoundary = -offset;
			float maxBoundary = minBoundary + width;
			if (mapCheck.x >= minBoundary && mapCheck.x < maxBoundary
			&&  mapCheck.z >= minBoundary && mapCheck.z < maxBoundary
			&&  mapCheck.y >= 0 && mapCheck.y < height)
			{
				bool checkForCollisions = false;
				int cubeIndex = 0;
				// Cube cube{mapCheck.x, mapCheck.y, mapCheck.z};
				// for (int i=0; i<8; ++i)
				// {	
				// 	float density = GetNoise3D(cube.corners[i].x, cube.corners[i].y, cube.corners[i].z);
				// 	cube.corners[i].density = density;
				// 	if (density > densityThreshold){
				// 		cubeIndex |= (1 << i);
				// 		checkForCollisions = true;
				// 	}
				// }

				// // ray-face intersection tests
				// float closestHitDistance = 100000000.0f;
				// if (checkForCollisions)
				// {	
				// 	std::cout << " " << std::endl;
				// 	std::cout << " " << std::endl;
				// 	std::cout << " " << std::endl;
	
					
				// 	const int* TriTableRow = TriTable[cubeIndex];
				// 	int i = 0;
				// 	while(TriTableRow[i] != -1){
				// 		int a0 = cornerIndexAFromEdge[TriTableRow[i]];
				// 		int b0 = cornerIndexBFromEdge[TriTableRow[i]];
				// 		int a1 = cornerIndexAFromEdge[TriTableRow[i+1]];
				// 		int b1 = cornerIndexBFromEdge[TriTableRow[i+1]];
				// 		int a2 = cornerIndexAFromEdge[TriTableRow[i+2]];
				// 		int b2 = cornerIndexBFromEdge[TriTableRow[i+2]];
				// 		Corner c1 = VertexInterp(densityThreshold, cube.corners[a0], cube.corners[b0]);
				// 		Corner c2 = VertexInterp(densityThreshold, cube.corners[a1], cube.corners[b1]);
				// 		Corner c3 = VertexInterp(densityThreshold, cube.corners[a2], cube.corners[b2]);
				// 		glm::vec3 normal = CalculateNormal(c1, c2, c3);
				// 		glm::vec3 v1 = glm::vec3(c1.x, c1.y, c1.z);
				// 		glm::vec3 v2 = glm::vec3(c2.x, c2.y, c2.z);
				// 		glm::vec3 v3 = glm::vec3(c3.x, c3.y, c3.z);
				// 		float fx = (v1.x + v2.x + v3.x) / 3;
				// 		float fy = (v1.y + v2.y + v3.y) / 3;
				// 		float fz = (v1.z + v2.z + v3.z) / 3;

				

				// 		// ray triangle intersection
				// 		glm::vec3 edge1 = v2 - v1;
				// 		glm::vec3 edge2 = v3 - v1;
				// 		glm::vec3 p = glm::cross(dir, edge2);
				// 		float determinant = glm::dot(edge1, p);
				// 		if (determinant == 0.0) break;					// break
				// 		float invDeterminant = 1.0f / determinant;
				// 		glm::vec3 origin = start - v1;
				// 		float u = glm::dot(origin, p) * invDeterminant;
				// 		glm::vec3 q = glm::cross(origin, edge1);
				// 		float v = glm::dot(dir, q) * invDeterminant;
				// 		float dist = glm::dot(edge2, q) * invDeterminant;
				// 		if (dist <= 0.0f) break;						// break
				// 		glm::vec3 intersectionPos = start + dist * dir;
						
				// 		std::cout << "triangle position     : " << fx << " " << fy << " " << fz << std::endl;
				// 		std::cout << "intersection position : " << intersectionPos.x << " " <<  intersectionPos.y << " " <<  intersectionPos.z << std::endl;

				// 		float d = glm::distance(glm::vec3(fx, fy, fz), intersectionPos);
				// 		std::cout << "dist: " << d << std::endl;
	

				// 		// Check if the intersection point lies within the triangle
				// 		if (d < 1.0f && dist < closestHitDistance) 
				// 		{
				// 			closestHitDistance = dist;
				// 			hit.distance = dist;
				// 			hit.position = intersectionPos;
				// 			hit.normal = normal;
				// 			hit.hit = true;
				// 			didHit = true;
				// 			std::cout << "hit detected!" << std::endl;
				// 		}
				// 		i+=3;
				// 	}
				// }

                hit.distance = 3.0f;
                hit.position = glm::vec3(0.5f, 0.5f, 0.5f);
                hit.normal = glm::vec3(0.0f, 0.0f, 1.0f);
                hit.hit = true;
                didHit = true;
			}
			if (didHit) break;
		}
		return hit;
	}

private:
	int width = 24;
	int height = 24;
    int offset = 0;
    float densityThreshold = 0.6f;
    unsigned int computeShaderProgram;

    std::vector<int> TriTableValues;
    std::vector<float> VolumeData;
	std::unordered_map<std::tuple<float, float, float>, unsigned int> VertexHashMap;
	FastNoiseLite noiseGenerator3D;
	FastNoiseLite noiseGenerator2D;

    unsigned int GetVertexIndex(float x, float y, float z)
	{
		std::tuple<float, float, float> key(x, y, z);

		auto it = VertexHashMap.find(key);
		if (it != VertexHashMap.end()) 
		{
			return it->second;
		} 
		else 
		{
			return -1;
		}
	}

	void SetVertexIndex(float x, float y, float z, unsigned int value)
	{
		std::tuple<float, float, float> key(x, y,z);
		VertexHashMap[key] = value;
	}

    void GenerateVolume()
    {
        for (int y = 0; y < height+1; ++y){
	        for (int x = 0; x < width+1; ++x){
	            for (int z = 0; z < width+1; ++z){
                    float density = GetVolume3D(x, y, z);
                    VolumeData.push_back(density);
                }
            }
        }
    }

	float GetVolume3D(float x, float y, float z)
	{
		int octaves = 6;
    	float amplitude = 1.0f;
		float prominance = 0.4f;
		float frequency = 4.0f;
		float totalNoise = 0.0f;
		for (int i = 0; i < octaves; ++i) 
		{
        	totalNoise += noiseGenerator3D.GetNoise((x - offset) * frequency, y * frequency, (z-offset) * frequency) * amplitude;
        	frequency *= 2.0f;  // increase the frequency for each octave
        	amplitude *= prominance;  // decrease the amplitude for each octave
    	}
		return (totalNoise + 1) / 2.0f;
	}

	float GetNoise2D(float x, float y) 
	{
		int octaves = 3;
		float amplitude = 1.0f;
		float prominance = 0.4f;
		float frequency = 5.0f;
		float totalNoise = 0.0f;
		for (int i = 0; i < octaves; ++i) 
		{
	    	totalNoise += noiseGenerator3D.GetNoise(x * frequency, y * frequency) * amplitude;
	    	frequency *= 2.0f;  // Increase the frequency for each octave
	    	amplitude *= prominance;  // Decrease the amplitude for each octave
		}
		return (totalNoise + 1) / 2.0f;
	}

	void InitNoiseGenerator()
	{
		noiseGenerator3D.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
		noiseGenerator2D.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
		noiseGenerator3D.SetSeed(1);
		noiseGenerator2D.SetSeed(1);
	}

    glm::vec3 SnapToCube(float x, float y, float z)
	{	
		x = std::round(x);
		y = std::round(y);
		z = std::round(z);
		return glm::vec3(x, y, z);
	}

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