#ifndef OBJMODEL_H
#define OBJMODEL_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>
#include "Shader.h"

struct ObjVertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoord;
};

struct ObjMesh {
    std::vector<ObjVertex>       vertices;
    std::vector<unsigned int>    indices;
    unsigned int VAO, VBO, EBO;
    unsigned int textureID;   // baseColor texture
    std::string  materialName;
};

class ObjModel {
public:
    ObjModel(const std::string& objPath,
             const std::string& textureDir);
    ~ObjModel();

    void draw(Shader& shader,
              const glm::mat4& view,
              const glm::mat4& projection,
              const glm::vec3& viewPos,
              const glm::vec3& position,
              float            scale,
              float            rotY,
              const glm::vec3& fogColor,
              float            fogDensity,
              const std::vector<glm::vec3>& lightPositions,
              const std::vector<glm::vec3>& lightColors,
              const std::vector<float>&     lightIntensities);

private:
    std::vector<ObjMesh> meshes;

    void load(const std::string& objPath,
              const std::string& textureDir);
    unsigned int loadTexture(const std::string& path);
    void setupMesh(ObjMesh& mesh);
};

#endif