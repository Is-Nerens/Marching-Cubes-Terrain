#pragma once

#include "marching_cubes_gpu.h"
#include "vendor/glm/glm.hpp"
#include "raycast.h"
#include "model.h"

class TerrainSystem
{
public:

    TerrainSystem() {
        terrainGPU.width = width;
        terrainGPU.height = height;
    }
    ~TerrainSystem() {}

    struct Chunk {
        int x;
        int y;
        int z;
    };

    std::vector<Model> models;

    void Update(float playerX, float playerY, float playerZ)
    {
        // COORDINATES OF CHUNK THAT BOUNDS THE PLAYER
        int playerChunkX = static_cast<int>(std::floor(playerX / width) * width);
        int playerChunkY = static_cast<int>(std::floor(playerY / height) * height);
	    int playerChunkZ = static_cast<int>(std::floor(playerZ / width) * width);
	    int rendDistOffsetH = -(renderDistanceH - 1) * width / 2;
	    int rendDistOffsetV = -(renderDistanceV - 1) * height / 2;


        // GENERATE 8 CHUNKS PER FRAME
        int chunksGenerated = 0;

        for (int y = 0; y < renderDistanceV; ++y) {
            for (int x = 0; x < renderDistanceH; ++x) {
                for (int z = 0; z < renderDistanceH; ++z) {
                    int chunkX = (x * width)  + playerChunkX + rendDistOffsetH;
                    int chunkY = (y * height) + playerChunkY + rendDistOffsetV;
                    int chunkZ = (z * width)  + playerChunkZ + rendDistOffsetH;

                    bool chunkPresent = false;

                    // Check if there is a chunk at the position
                    for (int i = 0; i < chunks.size(); ++i) {

                        // 1 - check if chunk exists at current iteration position
                        if (chunks[i].x == chunkX && chunks[i].y == chunkY && chunks[i].z == chunkZ) {
                            chunkPresent = true;
                            break;
                        }
                    }


                    // No chunk present? Create a new one
                    if (!chunkPresent){
                        Model model = terrainGPU.ConstructMeshGPU(chunkX, chunkY, chunkZ);
                        models.push_back(model);
                        Chunk newChunk;
                        newChunk.x = chunkX;
                        newChunk.y = chunkY;
                        newChunk.z = chunkZ;
                        chunks.push_back(newChunk);
                        chunksGenerated += 1;

                        // std::cout << "chunk pos: " << chunkX << " " <<  chunkY << " " << chunkZ << std::endl;
                        // std::cout << "model pos: " << model.position.x << " " <<  model.position.y << " " << model.position.z << std::endl;
                    }

                    if (chunksGenerated >= 4) goto end_chunk_generations;
                }
            }
        }
        end_chunk_generations:


        // REMOVE CHUNKS OUTSIDE RENDER DISTANCE
        for (int i=0; i<chunksGenerated; ++i) {
            for (int i=0; i<chunks.size(); ++i) {
                int chunkDist = std::max(std::max(std::abs(chunks[i].x - playerChunkX), std::abs(chunks[i].z - playerChunkZ)), std::abs(chunks[i].y - playerChunkY));
                if (chunkDist > std::floor((renderDistanceH * width) / 2.0)) {
                    chunks.erase(chunks.begin() + i);
			        models.erase(models.begin() + i);
                    break;
                } 
            }
        }
    }
    
    RayHit Raycast(glm::vec3 origin, glm::vec3 direction)
    {
        Ray ray;
        ray.origin = origin;
        ray.direction = direction;

        RayHit hit;
        float closestHit = 1000000.0f;

        // Test each chunk for ray - bounding intersection
        for (int i=0; i<chunks.size(); ++i)
        {   
            // CONSTRUCT BoundingBox FROM CHUNK 
            float hWidth = width/2;
            float hHeight = height/2;
            glm::vec3 chunkBL = glm::vec3 {
                chunks[i].x - hWidth, 
                chunks[i].y - hHeight, 
                chunks[i].z - hWidth
            };  // Bottom-left corner of the cube

            // Calculate min and max points of the BoundingBox/cube
            BoundingBox boundingBox;
            boundingBox.min = chunkBL;
            boundingBox.max = chunkBL + glm::vec3(width, height, width);

            // RAY INTERSECTS CHUNK BOUNDING BOX, PERFORM RAY TRIANGLE INTERSECTIONS
            if (RayIntersectsBox(ray, boundingBox))
            {
                for (int k=0; k<models[i].indices.size(); k+=3) // LOOP OVER MODEL INDICES
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
        int posChunkX = std::floor(position.x / width)  * width  - (renderDistanceH - 1) * width  / 2;
        int posChunkY = std::floor(position.y / height) * height - (renderDistanceV - 1) * height / 2;
        int posChunkZ = std::floor(position.z / width)  * width  - (renderDistanceH - 1) * width  / 2;

        bool editChunkExists = false;
        for (int i=0; i<editedChunks.size(); ++i)
        {      
            // CHECK IF EDIT CHUNK EXISTS
            if (posChunkX == editedChunks[i].x && posChunkY == editedChunks[i].y && posChunkZ == editedChunks[i].z)
            {
                editChunkExists = true;
                int cornerX = 0;
                int cornerY = 0;
                int cornerZ = 0;
                int densityIndex = GetDensityIndex(cornerX, cornerY, cornerZ);
                // std::cout << densityIndex << std::endl;
                editedChunks[i].densities[densityIndex] -= amount;

                // for (int k=0; k<editedChunks[i].densities.size(); ++k)
                // {
                //     std::cout << editedChunks[i].densities[k] << " ";
                // }
            }
        }

        // CREATE NEW EDIT CHUNK
        if (!editChunkExists)
        {
            EditedChunk eChunk(width, height);
            eChunk.x = std::floor(position.x / width)  * width  - (renderDistanceH - 1) * width  / 2;
            eChunk.y = std::floor(position.y / height) * height - (renderDistanceV - 1) * height / 2;
            eChunk.z = std::floor(position.z / width)  * width  - (renderDistanceH - 1) * width  / 2;
            editedChunks.push_back(eChunk);
        }
    }

private:
    TerrainGPU terrainGPU;
    std::vector<Chunk> chunks;
    std::vector<EditedChunk> editedChunks;

    int renderDistanceH = 11;
    int renderDistanceV = 5;
    int width = 12;
    int height = 12;

    int GetDensityIndex(int x, int y, int z)
    {
        return x + (y * (width + 1)) + (z * (width + 1) * (height + 1));
    }
};