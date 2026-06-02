#include "ParticleSystem.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>

// ─────────────────────────────────────────────
// initParticle
// Spawns a particle in a small radius around the camera.
// Bubbles rise from just below eye level.
// Silt drifts lazily at a wider radius.
// ─────────────────────────────────────────────
void ParticleSystem::initParticle(Particle& p, bool bubble, glm::vec3 cameraPos) {
    static float seed = 0.0f;
    seed += 1.0f;

    p.isBubble = bubble;

    float angle  = randF(seed + 1) * 6.2831f;

    if (bubble) {
        float radius = randF(seed + 2) * 1.8f;
        p.position = glm::vec3(
            cameraPos.x + cos(angle) * radius,
            cameraPos.y - 0.5f - randF(seed + 3) * 0.5f,
            cameraPos.z + sin(angle) * radius
        );
        p.velocity = glm::vec3(
            (randF(seed + 4) - 0.5f) * 0.2f,
            0.3f + randF(seed + 5) * 0.4f,
            (randF(seed + 6) - 0.5f) * 0.2f
        );
        p.size    = 0.8f + randF(seed + 7) * 1.5f;   // was 2-6, now tiny
        p.opacity = 0.15f + randF(seed + 8) * 0.15f; // was 0.4-0.7, now very faint
    } else {
        float radius = randF(seed + 2) * 2.5f;
        p.position = glm::vec3(
            cameraPos.x + cos(angle) * radius,
            cameraPos.y - randF(seed + 3) * 2.0f,
            cameraPos.z + sin(angle) * radius
        );
        p.velocity = glm::vec3(
            (randF(seed + 4) - 0.5f) * 0.06f,
            0.02f + randF(seed + 5) * 0.04f,
            (randF(seed + 6) - 0.5f) * 0.06f
        );
        p.size    = 0.5f + randF(seed + 7) * 0.8f;   // was 1.5-3.5, now tiny
        p.opacity = 0.05f + randF(seed + 8) * 0.08f; // barely visible
    }

    p.life = 0.0f;
}

// ─────────────────────────────────────────────
// setupBuffers
// One VAO/VBO for point rendering.
// Updated every frame with new particle positions.
// ─────────────────────────────────────────────
void ParticleSystem::setupBuffers() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER,
                 particles.size() * 5 * sizeof(float),
                 nullptr,
                 GL_DYNAMIC_DRAW);

    // Position: attribute 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Size: attribute 1
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE,
                          5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Opacity: attribute 2
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE,
                          5 * sizeof(float), (void*)(4 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

// Constructor — camera not available yet, spawn at origin as placeholder.
// Particles will immediately respawn around the real camera on first update.
ParticleSystem::ParticleSystem(int bubbleCount, int siltCount) {
    glm::vec3 defaultPos(0.0f);

    for (int i = 0; i < bubbleCount; i++) {
        Particle p;
        initParticle(p, true, defaultPos);
        // Scatter life so they don't all pop at once on frame 1
        p.life = (float)i / bubbleCount;
        particles.push_back(p);
    }

    for (int i = 0; i < siltCount; i++) {
        Particle p;
        initParticle(p, false, defaultPos);
        p.life = (float)i / siltCount;
        particles.push_back(p);
    }

    setupBuffers();
}

ParticleSystem::~ParticleSystem() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

// ─────────────────────────────────────────────
// update
// Move particles, sway them, reset if too far
// from camera or life expires.
// ─────────────────────────────────────────────
void ParticleSystem::update(float deltaTime, glm::vec3 cameraPos) {
    static float totalTime = 0.0f;
    totalTime += deltaTime;

    for (auto& p : particles) {
        float swayFreq = 0.5f + p.size * 0.1f;
        float swayAmt  = p.isBubble ? 0.12f : 0.03f;

        p.position.x += sin(totalTime * swayFreq + p.position.z) * swayAmt * deltaTime;
        p.position.z += cos(totalTime * swayFreq * 0.7f + p.position.x) * swayAmt * deltaTime;
        p.position   += p.velocity * deltaTime;
        p.life += deltaTime * (p.isBubble ? 0.8f : 0.3f);  // was 0.3/0.12, dies ~3x faster
        float distToCamera = glm::length(glm::vec2(
            p.position.x - cameraPos.x,
            p.position.z - cameraPos.z
        ));

        if (p.life > 1.0f || distToCamera > 3.0f) {
            initParticle(p, p.isBubble, cameraPos);
        }
    }
}

// ─────────────────────────────────────────────
// draw
// Upload to GPU and render as GL_POINTS.
// ─────────────────────────────────────────────
void ParticleSystem::draw(Shader& shader,
                           const glm::mat4& view,
                           const glm::mat4& projection,
                           const glm::vec3& viewPos,
                           const glm::vec3& fogColor,
                           float fogDensity)
{
    std::vector<float> data;
    data.reserve(particles.size() * 5);
    for (auto& p : particles) {
        data.push_back(p.position.x);
        data.push_back(p.position.y);
        data.push_back(p.position.z);
        data.push_back(p.size);
        data.push_back(p.opacity);
    }

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    data.size() * sizeof(float),
                    data.data());

    shader.use();
    shader.setMat4("view",        view);
    shader.setMat4("projection",  projection);
    shader.setVec3("viewPos",     viewPos);
    shader.setVec3("fogColor",    fogColor);
    shader.setFloat("fogDensity", fogDensity);

    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glBindVertexArray(VAO);
    glDrawArrays(GL_POINTS, 0, (int)particles.size());
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glDisable(GL_PROGRAM_POINT_SIZE);
}