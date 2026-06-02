#ifndef CAVE_ENVIRONMENT_H
#define CAVE_ENVIRONMENT_H
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "Mesh.h"
#include "Shader.h"

class CaveEnvironment {
public:
    glm::vec3 fogColor   = glm::vec3(0.01f, 0.02f, 0.05f);
    float     fogDensity = 0.045f;
    glm::vec3 rockColor  = glm::vec3(0.10f, 0.09f, 0.07f);

    CaveEnvironment();
    ~CaveEnvironment();

    struct Triangle {
        glm::vec3 v0, v1, v2;
    };

    std::vector<Triangle> getFloorTriangles() const;

    void draw(Shader& shader,
              const glm::mat4& view,
              const glm::mat4& projection,
              const glm::vec3& viewPos);

private:
    std::vector<Mesh*> meshes;

    // Noise functions for vertex displacement
    float hashf(glm::vec2 p);
    float smoothNoise(glm::vec2 p);
    float fbm(glm::vec2 p);

    // startZ/endZ: extent along Z, baseRadius: average radius,
    // irregularity: noise warp amount, rings/segments: mesh resolution
    void buildTube(float startZ, float endZ,
                   float baseRadius, float irregularity,
                   int rings, int segments);

    // pointsDown=true for stalactite, false for stalagmite
    void buildDripstone(glm::vec3 base, float height,
                        float radius, bool pointsDown);

    void buildCave();
    void buildEndCap(float centreZ, float radius,
                     float normalDir, int segments = 20);
};
#endif