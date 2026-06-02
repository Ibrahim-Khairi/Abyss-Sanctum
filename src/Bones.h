#ifndef BONES_H
#define BONES_H
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "Mesh.h"
#include "Shader.h"

// Fossilised creature skeleton placed on the cave floor
class Bones {
public:
    // World position of the skeleton root
    glm::vec3 origin;
    Bones(glm::vec3 origin);
    ~Bones();
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
    std::vector<Mesh*>     boneMeshes;
    std::vector<glm::mat4> boneTransforms;
    float hashf(glm::vec2 p);
    void buildEllipsoid(float sx, float sy, float sz,
                        int stacks, int slices, float roughness);
    void buildTooth(float height, float baseRadius, float curve);
    void buildRib(float length, float thickness, float curve);
    void buildSpineSegment(float radius, float height);
    void assembleSkeleton();
};
#endif