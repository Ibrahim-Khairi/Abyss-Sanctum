#ifndef ROCK_FORMATION_H
#define ROCK_FORMATION_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "Mesh.h"
#include "Shader.h"

class RockFormation {
public:
    // Single rock — position, scale, rotation for variety
    struct RockInstance {
        glm::vec3 position;
        float     scale;
        float     rotY;
        float     rotX;   // slight tilt for natural look
        int       seed;   // unique noise seed per rock
    };

    RockFormation();
    ~RockFormation();

    void draw(Shader& shader,
              const glm::mat4& view,
              const glm::mat4& projection,
              const glm::vec3& viewPos,
              const glm::vec3& fogColor,
              float fogDensity,
              const std::vector<glm::vec3>& lightPositions,
              const std::vector<glm::vec3>& lightColors,
              const std::vector<float>&     lightIntensities);

private:
    // One shared mesh — each rock is the same geometry
    // drawn at different positions/scales (instancing-lite)
    Mesh* rockMesh   = nullptr;
    Mesh* pebbleMesh = nullptr;

    std::vector<RockInstance> boulders;  // large rocks on walls/floor
    std::vector<RockInstance> pebbles;   // small stones scattered on floor

    float hashf(glm::vec2 p);
    float smoothNoise(glm::vec2 p);
    float fbm(glm::vec2 p);

    void buildRockMesh(float baseRadius, int seed);
    void buildPebbleMesh();
    void placeRocks();
};

#endif