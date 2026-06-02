// #ifndef CRYSTAL_CLUSTER_H
// #define CRYSTAL_CLUSTER_H

// #include <glm/glm.hpp>
// #include <vector>
// #include <memory>
// #include "Crystal.h"
// #include "Shader.h"

// class CrystalCluster {
// public:
//     glm::vec3 origin;       // centre of the cluster in world space
//     glm::vec3 lightColor;   // colour of light this cluster emits
//     float     lightIntensity;
//     float     pulseSpeed;   // how fast glow animates
//     float     pulseOffset;  // phase offset so clusters don't all pulse in sync

//     // The point light position for this cluster
//     // (slightly above the cluster tips — where the glow radiates from)
//     glm::vec3 getLightPosition() const;
//     glm::vec3 getAnimatedColor(float time) const;
//     float     getAnimatedIntensity(float time) const;

//     // Build a cluster of N crystals around an origin point
//     // wallNormal = which direction the wall faces
//     //             (crystals grow OUT from wall so we orient them that way)
//     CrystalCluster(glm::vec3 origin,
//                    glm::vec3 wallNormal,
//                    glm::vec3 lightColor,
//                    float     intensity,
//                    float     pulseSpeed,
//                    float     pulseOffset,
//                    int       count = 5);
//     ~CrystalCluster() = default;

//     void draw(Shader& shader,
//               const glm::mat4& view,
//               const glm::mat4& projection,
//               const glm::vec3& viewPos,
//               const glm::vec3& fogColor,
//               float fogDensity,
//               float time,
//               const std::vector<glm::vec3>& allLightPositions,
//               const std::vector<glm::vec3>& allLightColors,
//               const std::vector<float>&     allLightIntensities);

// private:
//     std::vector<std::unique_ptr<Crystal>> crystals;
//     glm::vec3 wallNormal; // direction crystals grow toward

//     // Simple deterministic pseudo-random from a seed
//     // Gives repeatable "random" values without std::rand weirdness
//     float pseudoRand(float seed) {
//         return glm::fract(glm::sin(seed * 127.1f) * 43758.5453f);
//     }
// };

// #endif

#ifndef CRYSTAL_CLUSTER_H
#define CRYSTAL_CLUSTER_H

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include "Crystal.h"
#include "Shader.h"

class CrystalCluster {
public:
    glm::vec3 origin;
    glm::vec3 m_lightColor;      // renamed from lightColor
    float     lightIntensity;
    float     pulseSpeed;
    float     pulseOffset;

    glm::vec3 getLightPosition() const;
    glm::vec3 getAnimatedColor(float time) const;
    float     getAnimatedIntensity(float time) const;

    CrystalCluster(glm::vec3 origin,
                   glm::vec3 wallNormal,
                   glm::vec3 lightColor,
                   float     intensity,
                   float     pulseSpeed,
                   float     pulseOffset,
                   int       count = 5);
    ~CrystalCluster() = default;

    void draw(Shader& shader,
              const glm::mat4& view,
              const glm::mat4& projection,
              const glm::vec3& viewPos,
              const glm::vec3& fogColor,
              float fogDensity,
              float time,
              const std::vector<glm::vec3>& allLightPositions,
              const std::vector<glm::vec3>& allLightColors,
              const std::vector<float>&     allLightIntensities);

private:
    std::vector<std::unique_ptr<Crystal>> crystals;
    glm::vec3 m_wallNormal;      // renamed from wallNormal

    float pseudoRand(float seed) {
        return glm::fract(glm::sin(seed * 127.1f) * 43758.5453f);
    }
};

#endif