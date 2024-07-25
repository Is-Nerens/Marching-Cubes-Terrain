#ifndef MARCHING_CUBES_H
#define MARCHING_CUBES_H

#include "vendor/glm/glm.hpp"
#include "tables.h"
#include "model.h"
#include "FastNoiseLite.h"
#include <vector>
#include <unordered_map>
#include <iostream>
#include <functional>
#include <algorithm>
#include <string>
#include <tuple>


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


class MeshReconstructor{
public:

	MeshReconstructor()
	{
		InitNoiseGenerator();
	}

	struct Corner
	{
		float x;
		float y;
		float z;
		float density = 0.0f;

		bool operator==(const Corner &other) const
		{ 
			return (x == other.x && y == other.y && z == other.z);
		}

		// Constructor
		Corner(float _x, float _y, float _z) 
		: x(_x), y(_y), z(_z) {}
	};

	struct Cube 
	{
		float x;
		float y;
		float z;
		Corner corners[8];

		// Constructor 
		Cube(float _x, float _y, float _z)
		: x(_x), y(_y), z(_z), corners{
			{x - 0.5f, y - 0.5f, z + 0.5f}, 
			{x + 0.5f, y - 0.5f, z + 0.5f},
			{x + 0.5f, y - 0.5f, z - 0.5f}, 
			{x - 0.5f, y - 0.5f, z - 0.5f}, 
			{x - 0.5f, y + 0.5f, z + 0.5f},
			{x + 0.5f, y + 0.5f, z + 0.5f},
			{x + 0.5f, y + 0.5f, z - 0.5f},
			{x - 0.5f, y + 0.5f, z - 0.5f}  
		} {}
	};

	struct CornerHasher {
        std::size_t operator()(const Corner& c) const {
            using std::size_t;
            using std::hash;

            return ((hash<float>()(c.x)
                     ^ (hash<float>()(c.y) << 1)) >> 1)
                     ^ (hash<float>()(c.z) << 1);
        }
    };
	
	Model ConstructMesh()
	{
		Model model;

		// Hashmap to check for duplicate vertices
		VertexMap = GenerateVertexMap();

		std::unordered_map<Corner, uint32_t, CornerHasher> vertexMap;

		TriTableValues = LoadTriTableValues();

		// settings
		int offset = 0;
		float densityThreshold = 0.6f;

		// Generate cube density volume
		for (int y = 0; y < height; ++y){
	        for (int x = 0; x < width; ++x){
				#pragma omp parallel for
	            for (int z = 0; z < width; ++z)
				{
					// create cube at position
					Cube cube{static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)};

					// calculate corner densities
					int cubeIndex = 0;
					for (int i=0; i<8; ++i){	
						float density = GetNoise3D(cube.corners[i].x, cube.corners[i].y, cube.corners[i].z);
						cube.corners[i].density = density;
						if (cube.corners[i].density > densityThreshold){
							cubeIndex |= (1 << i);
						}
					}

					// generate cube vertices
					const int* TriTableRow = TriTable[cubeIndex];
					int i = 0;
					while(TriTableRow[i] != -1){
						int a0 = cornerIndexAFromEdge[TriTableGet(cubeIndex, i)];
						int b0 = cornerIndexBFromEdge[TriTableGet(cubeIndex, i)];
						int a1 = cornerIndexAFromEdge[TriTableGet(cubeIndex, i+1)];
						int b1 = cornerIndexBFromEdge[TriTableGet(cubeIndex, i+1)];
						int a2 = cornerIndexAFromEdge[TriTableGet(cubeIndex, i+2)];
						int b2 = cornerIndexBFromEdge[TriTableGet(cubeIndex, i+2)];
						Corner p1 = VertexInterp(densityThreshold, cube.corners[a0], cube.corners[b0]);
						Corner p2 = VertexInterp(densityThreshold, cube.corners[a1], cube.corners[b1]);
						Corner p3 = VertexInterp(densityThreshold, cube.corners[a2], cube.corners[b2]);

						// Calculate the normal
						glm::vec3 normal = CalculateNormal(p1, p2, p3);



						// if (VertexMapGet(p1) == 0)
						// {
						// 	VertexMapSet(p1, model.VertexCount());
						// 	model.indices.push_back(model.VertexCount());
						// 	model.AddVertex(p1.x - offset, p1.y, p1.z - offset, normal.x, normal.y, normal.z);
						// }
						// else
						// {
						// 	model.indices.push_back(VertexMapGet(p1));
						// }

						// if (VertexMapGet(p2) == 0)
						// {
						// 	VertexMapSet(p2, model.VertexCount());
						// 	model.indices.push_back(model.VertexCount());
						// 	model.AddVertex(p2.x - offset, p2.y, p2.z - offset, normal.x, normal.y, normal.z);
						// }
						// else
						// {
						// 	model.indices.push_back(VertexMapGet(p2));
						// }

						// if (VertexMapGet(p3) == 0)
						// {
						// 	VertexMapSet(p3, model.VertexCount());
						// 	model.indices.push_back(model.VertexCount());
						// 	model.AddVertex(p3.x - offset, p3.y, p3.z - offset, normal.x, normal.y, normal.z);
						// }
						// else
						// {
						// 	model.indices.push_back(VertexMapGet(p3));
						// }

						//Check for vertex duplicates
						auto it1 = vertexMap.find(p1);
						if (it1 != vertexMap.end()) { model.indices.push_back(it1->second); } 
						else 
						{
							vertexMap[p1] = model.VertexCount();
							model.indices.push_back(model.VertexCount());
							model.AddVertex(p1.x - offset, p1.y, p1.z, normal.x, normal.y, normal.z);
						}

						auto it2 = vertexMap.find(p2);
						if (it2 != vertexMap.end()) { model.indices.push_back(it2->second); } 
						else 
						{
							vertexMap[p2] = model.VertexCount();
							model.indices.push_back(model.VertexCount());
							model.AddVertex(p2.x - offset, p2.y, p2.z, normal.x, normal.y, normal.z);
						}

						auto it3 = vertexMap.find(p3);
						if (it3 != vertexMap.end()) { model.indices.push_back(it3->second); } 
						else 
						{
							vertexMap[p3] = model.VertexCount();
							model.indices.push_back(model.VertexCount());
							model.AddVertex(p3.x - offset, p3.y, p3.z, normal.x, normal.y, normal.z);
						}

					
						i+=3;
					}
				}
			}
		}

		return model;
	}

	void AddDensity(float x, float y, float z, float amount)
	{
		glm::vec3 pos = ClosestCornerPosition(x, y, z);
		SetEditDensity(pos.x, pos.y, pos.z, GetEditDensity(pos.x, pos.y, pos.z) + amount);
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
				Cube cube{mapCheck.x, mapCheck.y, mapCheck.z};
				for (int i=0; i<8; ++i)
				{	
					float density = GetNoise3D(cube.corners[i].x, cube.corners[i].y, cube.corners[i].z);
					cube.corners[i].density = density;
					if (density > densityThreshold){
						cubeIndex |= (1 << i);
						checkForCollisions = true;
					}
				}

				// ray-face intersection tests
				float closestHitDistance = 100000000.0f;
				if (checkForCollisions)
				{	
					std::cout << " " << std::endl;
					std::cout << " " << std::endl;
					std::cout << " " << std::endl;
	
					
					const int* TriTableRow = TriTable[cubeIndex];
					int i = 0;
					while(TriTableRow[i] != -1){
						int a0 = cornerIndexAFromEdge[TriTableRow[i]];
						int b0 = cornerIndexBFromEdge[TriTableRow[i]];
						int a1 = cornerIndexAFromEdge[TriTableRow[i+1]];
						int b1 = cornerIndexBFromEdge[TriTableRow[i+1]];
						int a2 = cornerIndexAFromEdge[TriTableRow[i+2]];
						int b2 = cornerIndexBFromEdge[TriTableRow[i+2]];
						Corner c1 = VertexInterp(densityThreshold, cube.corners[a0], cube.corners[b0]);
						Corner c2 = VertexInterp(densityThreshold, cube.corners[a1], cube.corners[b1]);
						Corner c3 = VertexInterp(densityThreshold, cube.corners[a2], cube.corners[b2]);
						glm::vec3 normal = CalculateNormal(c1, c2, c3);
						glm::vec3 v1 = glm::vec3(c1.x, c1.y, c1.z);
						glm::vec3 v2 = glm::vec3(c2.x, c2.y, c2.z);
						glm::vec3 v3 = glm::vec3(c3.x, c3.y, c3.z);
						float fx = (v1.x + v2.x + v3.x) / 3;
						float fy = (v1.y + v2.y + v3.y) / 3;
						float fz = (v1.z + v2.z + v3.z) / 3;

				

						// ray triangle intersection
						glm::vec3 edge1 = v2 - v1;
						glm::vec3 edge2 = v3 - v1;
						glm::vec3 p = glm::cross(dir, edge2);
						float determinant = glm::dot(edge1, p);
						if (determinant == 0.0) break;					// break
						float invDeterminant = 1.0f / determinant;
						glm::vec3 origin = start - v1;
						float u = glm::dot(origin, p) * invDeterminant;
						glm::vec3 q = glm::cross(origin, edge1);
						float v = glm::dot(dir, q) * invDeterminant;
						float dist = glm::dot(edge2, q) * invDeterminant;
						if (dist <= 0.0f) break;						// break
						glm::vec3 intersectionPos = start + dist * dir;
						
						std::cout << "triangle position     : " << fx << " " << fy << " " << fz << std::endl;
						std::cout << "intersection position : " << intersectionPos.x << " " <<  intersectionPos.y << " " <<  intersectionPos.z << std::endl;

						float d = glm::distance(glm::vec3(fx, fy, fz), intersectionPos);
						std::cout << "dist: " << d << std::endl;
	

						// Check if the intersection point lies within the triangle
						if (d < 1.0f && dist < closestHitDistance) 
						{
							closestHitDistance = dist;
							hit.distance = dist;
							hit.position = intersectionPos;
							hit.normal = normal;
							hit.hit = true;
							didHit = true;
							std::cout << "hit detected!" << std::endl;
						}
						i+=3;
					}
				}
			}
			if (didHit) break;
		}
		return hit;
	}

private:

	int width = 64;
	int height = 64;
	int offset = 0;

	std::unordered_map<std::tuple<float, float, float>, float> editsHashMap;
	FastNoiseLite noiseGenerator3D;
	FastNoiseLite noiseGenerator2D;

	Corner VertexInterp(float isolevel, const Corner& c1, const Corner& c2) {
		float t = (isolevel - c1.density) / (c2.density - c1.density);
		Corner p {
			c1.x + t * (c2.x - c1.x),
			c1.y + t * (c2.y - c1.y),
			c1.z + t * (c2.z - c1.z)
		};
		return p;
	}
	
	glm::vec3 CalculateNormal(const Corner& p1, const Corner& p2, const Corner& p3) {
		glm::vec3 v1(p2.x - p1.x, p2.y - p1.y, p2.z - p1.z);
		glm::vec3 v2(p3.x - p1.x, p3.y - p1.y, p3.z - p1.z);

		// calculate the cross product
		glm::vec3 normal = glm::cross(v1, v2);

		// return the normalized vector
		return glm::normalize(normal);
	}

	glm::vec3 ClosestCornerPosition(float x, float y, float z)
	{
		x = std::round(x) - 0.5f;
		y = std::round(y) - 0.5f;
		z = std::round(z) - 0.5f;
		return glm::vec3(x, y, z);
	}

	glm::vec3 SnapToCube(float x, float y, float z)
	{	
		x = std::round(x);
		y = std::round(y);
		z = std::round(z);
		return glm::vec3(x, y, z);
	}

	float GetNoise3D(float x, float y, float z)
	{
		int octaves = 6;
    	float amplitude = 1.0f;
		float prominance = 0.4f;
		float frequency = 1.0f;
		float totalNoise = 0.0f;
		for (int i = 0; i < octaves; i++) 
		{
        	totalNoise += noiseGenerator3D.GetNoise((x - offset) * frequency, y * frequency, (z-offset) * frequency) * amplitude;
        	frequency *= 2.0f;  // increase the frequency for each octave
        	amplitude *= prominance;  // decrease the amplitude for each octave
    	}
		return (totalNoise + 1) / 2.0f + GetEditDensity(x, y, z);
	}

	float GetNoise2D(float x, float y) 
	{
		int octaves = 3;
		float amplitude = 1.0f;
		float prominance = 0.4f;
		float frequency = 4.0f;
		float totalNoise = 0.0f;
		for (int i = 0; i < octaves; i++) 
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

	float GetEditDensity(float x, float y, float z)
	{
		std::tuple<float, float, float> key(x, y, z);

		auto it = editsHashMap.find(key);
		if (it != editsHashMap.end()) 
		{
			return it->second;
		} 
		else 
		{
			return 0.0f;
		}
	}

	void SetEditDensity(float x, float y, float z, float value)
	{
		std::tuple<float, float, float> key(x, y,z);
		editsHashMap[key] = value;
	}

	std::vector<int> GenerateVertexMap()
    {
        std::vector<int> map;       
        for (int i=0; i<(width+1) * (width+1) * (height+1); ++i)
        {   
            map.push_back(0);
        }
        return map;
    }

	int HashIndex(Corner v)
	{
		int x = int(floor(v.x + 0.5));
		int y = int(floor(v.y + 0.5));
		int z = int(floor(v.z + 0.5));
		int index = x + (width+1) * (y + (height+1) * z);
		return index;
	}


	int VertexMapGet(Corner v)
	{
		int index = HashIndex(v);
    	return VertexMap[index];
	}

	void VertexMapSet(Corner v, int value)
	{
		int index = HashIndex(v);
		VertexMap[index] = value;
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

	int TriTableGet(int cubeIndex, int i)
	{
		int index = cubeIndex * 16 + i;
		return TriTableValues[index];
	}

	std::vector<int> VertexMap;
	std::vector<int> TriTableValues;
};
#endif