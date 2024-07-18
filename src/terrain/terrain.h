#pragma once
#include "marching_cubes_gpu.h"
#include "../vendor/glm/glm.hpp"
#include "../raycast.h"
#include "../model.h"

class TerrainSystem
{
public:

    TerrainSystem()
    {
        terrainGPU.width = width;
        terrainGPU.height = height;
    }
    ~TerrainSystem() {}

    struct Chunk 
    {
        int x;
        int y;
        int z;
        bool regenerate = false;
        bool inRenderDist = true;
        bool arrayIndex = 0;
        std::vector<float> densities;
    };

    std::vector<Model> models;

    void Update(float playerX, float playerY, float playerZ)
    {
        // COORDINATES OF CHUNK THAT BOUNDS THE PLAYER
        int playerChunkX = (static_cast<int>(std::round(playerX / width)  * width)  -(renderDistanceH - 1) * width  / 2);
        int playerChunkY = (static_cast<int>(std::round(playerY / height) * height) -(renderDistanceV - 1) * height / 2);
	    int playerChunkZ = (static_cast<int>(std::round(playerZ / width)  * width)  -(renderDistanceH - 1) * width  / 2);

        // GENERATE 8 CHUNKS PER FRAME
        int chunksGenerated = 0;

        // DEFAULT ALL CHUNKS TO BEING OUTSIDE RENDER DISTANCE
        for (int i=0; i<chunks.size(); ++i) {
            chunks[i].inRenderDist = false;
        }

        // GENERATE NEW CHUNKS || REGENERATE CHUNKS
        for (int y = 0; y < renderDistanceV; ++y) {
            for (int x = 0; x < renderDistanceH; ++x) {
                for (int z = 0; z < renderDistanceH; ++z) {
                    int chunkX = (x * width)  + playerChunkX;
                    int chunkY = (y * height) + playerChunkY;
                    int chunkZ = (z * width)  + playerChunkZ;

                    // CHECK IF CHUNK EXISTS AT POSITION
                    bool chunkPresent = false;
                    for (int i = 0; i < chunks.size(); ++i) 
                    {
                        if (chunks[i].x == chunkX && chunks[i].y == chunkY && chunks[i].z == chunkZ) 
                        {
                            chunkPresent = true;

                            chunks[i].inRenderDist = true;
                            
                            // REGENERATE CHUNK
                            if (chunks[i].regenerate)
                            {
                                Model model = terrainGPU.ConstructMeshGPU(chunkX, chunkY, chunkZ, chunks[i].densities);
                                models[i] = model; // ASSIGN NEW CHUNK MODEL
                                chunksGenerated += 1;
                                chunks[i].regenerate = false;
                            }
                        }
                    }

                    // NO CHUNK AT LOCATION -> GENERATE ONE
                    if (!chunkPresent) 
                    {
                        Model model = terrainGPU.ConstructMeshGPU(chunkX, chunkY, chunkZ);
                        models.push_back(model);
                        Chunk newChunk;
                        newChunk.x = chunkX;
                        newChunk.y = chunkY;
                        newChunk.z = chunkZ;
                        newChunk.densities = std::vector<float>((width+1)*(width+1)*(height+1), 0.0f);
                        chunks.push_back(newChunk);
                        chunksGenerated += 1;
                    }

                    // MAX CHUNKS GENERATED FOR CURRENT FRAME
                    if (chunksGenerated >= 2) return;
                }
            }
        }

        // REMOVE CHUNKS OUTSIDE RENDER DISTANCE
        for (int i=0; i<chunks.size(); ++i) {
            if (chunks[i].inRenderDist == false) {
                chunks.erase(chunks.begin() + i);
                models.erase(models.begin() + i);
                break;
            } 
        }
    }
    
    RayHit Raycast(glm::vec3 origin, glm::vec3 direction)
    {
        RayHit hit;
        Ray ray;
        ray.origin = origin;
        ray.direction = direction;
        float closestHit = 1000000.0f;

        // RAY, CHUNK BOUNDING BOX INTERSECTION TEST
        for (int i=0; i<chunks.size(); ++i)
        {   
            // CONSTRUCT CHUNK BOUNDING BOX
            float hWidth = width/2;
            float hHeight = height/2;
            glm::vec3 chunkBL = glm::vec3 {
                chunks[i].x - hWidth, 
                chunks[i].y - hHeight, 
                chunks[i].z - hWidth};
            BoundingBox boundingBox;
            boundingBox.min = chunkBL;
            boundingBox.max = chunkBL + glm::vec3(width, height, width);

            // IF RAY INTERSECTS CHUNK BOUNDING BOX, PERFORM RAY TRIANGLE INTERSECTIONS
            if (RayIntersectsBox(ray, boundingBox))
            {
                // LOOP OVER MODEL INDICES TO EXTRACT TRIANGLE VERTICES
                for (int k=0; k<models[i].indices.size(); k+=3) 
                {
                    glm::vec3 v1 = glm::vec3{
                        models[i].vertices[(k + 0) * 3 + 0] + models[i].position.x, 
                        models[i].vertices[(k + 0) * 3 + 1] + models[i].position.y, 
                        models[i].vertices[(k + 0) * 3 + 2] + models[i].position.z};
                    glm::vec3 v2 = glm::vec3{
                        models[i].vertices[(k + 1) * 3 + 0] + models[i].position.x, 
                        models[i].vertices[(k + 1) * 3 + 1] + models[i].position.y, 
                        models[i].vertices[(k + 1) * 3 + 2] + models[i].position.z};
                    glm::vec3 v3 = glm::vec3{
                        models[i].vertices[(k + 2) * 3 + 0] + models[i].position.x, 
                        models[i].vertices[(k + 2) * 3 + 1] + models[i].position.y, 
                        models[i].vertices[(k + 2) * 3 + 2] + models[i].position.z};
                    
                    // RAY TRIANGLE INTERSECTION WITH EVERY FACE
                    RayHit newhit = RayTriangleIntersection(ray, v1, v2, v3);
                    if (newhit.hit) {
                        if (newhit.distance < closestHit) {
                            newhit.distance = closestHit;
                            hit = newhit;
                        }
                    }
                }
            }
        }
        return hit;
    }

    void AddDensity(glm::vec3& position, float radius, float amount)
    {
        // COORDINATES OF CHUNK THAT BOUNDS THE POSITION
        int chunkX = std::round(position.x / width)  * width  - (renderDistanceH - 1) * width  / 2;
        int chunkY = std::round(position.y / height) * height - (renderDistanceV - 1) * height / 2;
        int chunkZ = std::round(position.z / width)  * width  - (renderDistanceH - 1) * width  / 2;

        std::cout << "\n" << "Editing Chunk Density..." << std::endl;
        std::cout << "edit chunk pos: " << chunkX <<  " " << chunkY <<  " " << chunkZ << std::endl;

        // FIND WHICH CHUNK NEEDS TO HAVE ITS DENSITIES UPDATED
        for (int i=0; i<chunks.size(); ++i)
        {
            // FOUND CHUNK
            if (chunks[i].x == chunkX && chunks[i].y == chunkY && chunks[i].z == chunkZ)
            {
                // CALCULATE INDEX OF DENSITY ARRAY
                int cornerX = WorldToChunkCorner(std::round(position.x), width+1);
                int cornerY = WorldToChunkCorner(std::round(position.y), height+1);
                int cornerZ = WorldToChunkCorner(std::round(position.z), width+1);
                int densityIndex = GetDensityIndex(cornerX, cornerY, cornerZ);
                chunks[i].densities[densityIndex] += amount;
                chunks[i].regenerate = true;
                std::cout << "Found Chunk" << std::endl;
            }
        }
    }

private:
    TerrainGPU terrainGPU;
    std::vector<Chunk> chunks;
    std::vector<Chunk> flaggedForEdit;

    int renderDistanceH = 3;
    int renderDistanceV = 3;
    int width = 32;
    int height = 32;

    int GetDensityIndex(int x, int y, int z) {
        return x + (y * (width + 1)) + (z * (width + 1) * (height + 1));
    }

    int WorldToChunkCorner(int displacement, int dimension) {
        int offset = (dimension / 2);
        int result = (displacement + offset) % (dimension);
        if (result < 0) result += dimension;
        return result;
    }
};