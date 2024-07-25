#pragma once
#include <unordered_map>
#include "marching_cubes_gpu.h"
#include "../vendor/glm/glm.hpp"
#include "../raycast.h"
#include "../model.h"
#include "../error.h"

struct TupleHash {
    template <class T1, class T2, class T3>
    std::size_t operator()(const std::tuple<T1, T2, T3>& tuple) const {
        auto h1 = std::hash<T1>{}(std::get<0>(tuple));
        auto h2 = std::hash<T2>{}(std::get<1>(tuple));
        auto h3 = std::hash<T3>{}(std::get<2>(tuple));
        // Combine hash values using a bitwise XOR and a shift
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

class TerrainSystem
{
public:

    TerrainSystem()
    {
        terrainGPU.width = width;
        terrainGPU.height = height;
    }
    ~TerrainSystem() {}

    std::vector<Model*> models;
    std::unordered_map<std::tuple<int, int, int>, size_t, TupleHash> chunkPosToIndex;

    void Update(float playerX, float playerY, float playerZ)
    {
        // COORDINATES OF CHUNK THAT BOUNDS THE PLAYER
        int minChunkX = (std::round(playerX / width)  * width)  -(renderDistanceH - 1) * width  / 2;
        int minChunkY = (std::round(playerY / height) * height) -(renderDistanceV - 1) * height / 2;
	    int minChunkZ = (std::round(playerZ / width)  * width)  -(renderDistanceH - 1) * width  / 2;
        int maxChunkX = minChunkX + renderDistanceH * width - 1;
        int maxChunkY = minChunkY + renderDistanceV * height - 1;
        int maxChunkZ = minChunkZ + renderDistanceH * width - 1;

        // CHECK ALL CHUNKS TO SEE IF THEY ARE INSIDE RENDER DISTANCE
        int chunksRemoved = 0;
        for (int i=0; i<chunks.size(); ++i) {
            bool inRenderDist = chunks[i].x >= minChunkX && chunks[i].x <= maxChunkX && 
                                chunks[i].y >= minChunkY && chunks[i].y <= maxChunkY && 
                                chunks[i].z >= minChunkZ && chunks[i].z <= maxChunkZ;
            if (!inRenderDist)
            {
                chunkPosToIndex.erase(std::make_tuple(chunks[i].x, chunks[i].y, chunks[i].z));

                // DECREMENT EVERY FOLLOWING CHUNK INDEX
                for (int k=i+1; k<chunks.size(); ++k)
                {
                    chunkPosToIndex[std::make_tuple(chunks[k].x, chunks[k].y, chunks[k].z)] -= 1; // POTENTIAL OPTIMISATION!! FROM O(n^2) to O(n) - perform one pass over the chunkPosToIndex elements rather than multiple passes
                }

                // THIS CAN BE OPTIMISED FURTHER
                delete models[i];
                models.erase(models.begin() + i);
                chunks.erase(chunks.begin() + i);
                chunksRemoved += 1;
                --i;

                if (chunksRemoved > 2) break;
            }
        }

        // GENERATE 8 CHUNKS PER FRAME
        int chunksGenerated = 0;

        std::vector<Chunk*> chunksToGeneratePtrs;
        std::vector<Model*> modelsToGeneratePtrs;

        // REGENERATE CHUNKS - EXCLUDE OUTERMOST
        for (int i=0; i<chunks.size(); ++i) {
            bool inRenderDist = chunks[i].x > minChunkX && chunks[i].x < maxChunkX && 
                                chunks[i].y > minChunkY && chunks[i].y < maxChunkY && 
                                chunks[i].z > minChunkZ && chunks[i].z < maxChunkZ;

            // REGENERATE CHUNK
            if (inRenderDist && chunks[i].regenerate)
            {
                chunks[i].regenerate = false;
                models[i]->vertices.clear();
                models[i]->indices.clear();
                chunksToGeneratePtrs.push_back(&chunks[i]);
                modelsToGeneratePtrs.push_back(models[i]);
                chunksGenerated += 1;
            }

            if (chunksGenerated >= 8) goto endOfGenerationCheck;
        }

        // GENERATE NEW CHUNKS
        for (int y=0; y <renderDistanceV; ++y) {
            for (int x=0; x <renderDistanceH; ++x) {
                for (int z=0; z<renderDistanceH; ++z) {
                    int chunkX = (x * width)  + minChunkX;
                    int chunkY = (y * height) + minChunkY;
                    int chunkZ = (z * width)  + minChunkZ;

                    // NO CHUNK AT LOCATION -> GENERATE ONE
                    auto pos = std::make_tuple(chunkX, chunkY, chunkZ);
                    auto it = chunkPosToIndex.find(pos);
                    if (it == chunkPosToIndex.end()) 
                    {
                        Chunk newChunk;
                        newChunk.x = chunkX;
                        newChunk.y = chunkY;
                        newChunk.z = chunkZ;
                        newChunk.densities = std::vector<float>((width+1)*(width+1)*(height+1), 0.0f);
                        chunks.push_back(newChunk);
                        chunkPosToIndex[std::make_tuple(chunkX, chunkY, chunkZ)] = chunks.size() -1;

                        Model* model = new Model;
                        models.push_back(model);

                        chunksToGeneratePtrs.push_back(&chunks[chunks.size()-1]);
                        modelsToGeneratePtrs.push_back(model);
                        chunksGenerated += 1;
                    }

                    if (chunksGenerated >= 8) goto endOfGenerationCheck;
                }
            }
        }
        endOfGenerationCheck:

        // GENERATE ALL CHUNKS
        terrainGPU.GenerateMeshes(chunksToGeneratePtrs, modelsToGeneratePtrs);
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
                for (int k=0; k<models[i]->indices.size(); k+=3) 
                {
                    int v1Index = models[i]->indices[k + 0];
                    int v2Index = models[i]->indices[k + 1];
                    int v3Index = models[i]->indices[k + 2];
                    glm::vec3 v1 = glm::vec3{
                        models[i]->vertices[v1Index * 6 + 0] + models[i]->position.x, 
                        models[i]->vertices[v1Index * 6 + 1] + models[i]->position.y, 
                        models[i]->vertices[v1Index * 6 + 2] + models[i]->position.z};
                    glm::vec3 v2 = glm::vec3{
                        models[i]->vertices[v2Index * 6 + 0] + models[i]->position.x, 
                        models[i]->vertices[v2Index * 6 + 1] + models[i]->position.y, 
                        models[i]->vertices[v2Index * 6 + 2] + models[i]->position.z};
                    glm::vec3 v3 = glm::vec3{
                        models[i]->vertices[v3Index * 6 + 0] + models[i]->position.x, 
                        models[i]->vertices[v3Index * 6 + 1] + models[i]->position.y, 
                        models[i]->vertices[v3Index * 6 + 2] + models[i]->position.z};
                    
                    // RAY TRIANGLE INTERSECTION WITH EVERY FACE
                    RayHit newhit = RayTriangleIntersection(ray, v1, v2, v3);
                    if (newhit.hit) {
                        if (newhit.distance < closestHit) {
                            closestHit = newhit.distance;
                            hit = newhit;
                        }
                    }
                }
            }
        }
        return hit;
    }

    void AddDensity(glm::vec3& position, int radius, float amount) 
    {
        // snap position to grid
        int snapWorldX = std::round(position.x);
        int snapWorldY = std::round(position.y);
        int snapWorldZ = std::round(position.z);
        
        // FOR EACH CHUNK
        for (int i = 0; i < chunks.size(); ++i) 
        {
            Chunk& chunk = chunks[i];

            // FOR EACH CORNER
            for (int x = snapWorldX - radius + 1; x < snapWorldX + radius; ++x) {
            for (int y = snapWorldY - radius + 1; y < snapWorldY + radius; ++y) {
            for (int z = snapWorldZ - radius + 1; z < snapWorldZ + radius; ++z) 
                    {
                        int cornerLocalX = x + width/2 - chunk.x;
                        int cornerLocalY = y + width/2 - chunk.y;
                        int cornerLocalZ = z + width/2 - chunk.z;

                        // IF CORNER POSITION IS INSIDE CHUNK
                        if (cornerLocalX >= 0 && cornerLocalX <= width &&
                            cornerLocalY >= 0 && cornerLocalY <= height &&
                            cornerLocalZ >= 0 && cornerLocalZ <= width)
                        {

                            // IF DIST FROM SNAP POS TO CORNER POS <= RADIUS
                            float distance = glm::distance(glm::vec3(snapWorldX, snapWorldY, snapWorldZ), glm::vec3(x, y, z));
                            if (distance <= radius) 
                            {
                                int densityIndex = GetDensityIndex(cornerLocalX, cornerLocalY, cornerLocalZ);
                                chunk.densities[densityIndex] += amount / distance;
                                chunk.regenerate = true;
                            }
                        }
                    }
                }
            }
        }
    }

private:
    TerrainGPU terrainGPU;
    std::vector<Chunk> chunks;

    int renderDistanceH = 17;
    int renderDistanceV = 9;
    int width = 16;
    int height = 16;

    int GetDensityIndex(int x, int y, int z) {
        return x + (y * (width + 1)) + (z * (width + 1) * (height + 1));
    }
};