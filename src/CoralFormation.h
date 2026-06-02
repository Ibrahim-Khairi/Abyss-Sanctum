#ifndef CORAL_FORMATION_H
#define CORAL_FORMATION_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <memory>
#include "Mesh.h"
#include "Shader.h"
#include "CaveEnvironment.h"

class CoralFormation {
public:
    CoralFormation(CaveEnvironment* cave);
    ~CoralFormation();

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
    CaveEnvironment* caveEnv = nullptr;
    struct CoralInstance {
        glm::vec3 position;
        float     scale;
        float     rotY;
        glm::vec3 color;   // corals have variety — dark red, grey, white
    };

    std::vector<Mesh*>          branchMeshes; // different branch shapes
    std::vector<CoralInstance>  instances;

    float hashf(glm::vec2 p);
    float smoothNoise(glm::vec2 p);
    float fbm(glm::vec2 p);
        float raycastFloorY(float x, float z,
        const std::vector<CaveEnvironment::Triangle>& tris);

    // Builds one jagged coral branch — tapered cylinder with spikes
    void buildBranchMesh(float height, float baseRadius,
                         float jaggedness, int seed);
    void placeCoral();
};

#endif