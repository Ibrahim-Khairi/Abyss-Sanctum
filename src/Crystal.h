#ifndef CRYSTAL_H
#define CRYSTAL_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "Mesh.h"
#include "Shader.h"

class Crystal {
public:
    glm::vec3 position;
    glm::vec3 color;
    glm::vec3 wallNormal;   // NEW — which direction this crystal grows toward
    float     height;
    float     radius;
    float     rotationY;
    float     tiltAngle;
    glm::vec3 tiltAxis;

    Crystal(glm::vec3 pos,
            glm::vec3 col,
            glm::vec3 wallNormal,   // NEW
            float height,
            float radius,
            float rotY,
            float tilt,
            glm::vec3 tiltAx);
    ~Crystal();

    void draw(Shader& shader,
              const glm::mat4& view,
              const glm::mat4& projection,
              const glm::vec3& viewPos,
              const glm::vec3& fogColor,
              float fogDensity,
              float pulse,
              const std::vector<glm::vec3>& lightPositions,
              const std::vector<glm::vec3>& lightColors,
              const std::vector<float>&     lightIntensities);

private:
    Mesh* mesh = nullptr;
    void buildCrystalMesh();
};

#endif