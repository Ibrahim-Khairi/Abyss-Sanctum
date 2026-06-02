#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "Shader.h"

struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    float     size;
    float     opacity;
    float     life;      // 0.0 = dead, 1.0 = fully alive
    bool      isBubble;  // true = bubble, false = silt
};

class ParticleSystem {
public:
    ParticleSystem(int bubbleCount, int siltCount);
    ~ParticleSystem();

    void update(float deltaTime, glm::vec3 cameraPos);
    void draw(Shader& shader,
              const glm::mat4& view,
              const glm::mat4& projection,
              const glm::vec3& viewPos,
              const glm::vec3& fogColor,
              float fogDensity);

private:
    std::vector<Particle> particles;

    unsigned int VAO, VBO;

    void initParticle(Particle& p, bool isBubble, glm::vec3 cameraPos);
    void setupBuffers();

    // Simple deterministic random
    float randF(float seed) {
        float v = glm::sin(seed * 127.1f) * 43758.5453f;
        return v - glm::floor(v);
    }
};

#endif