#include "CrystalCluster.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/compatibility.hpp>

CrystalCluster::CrystalCluster(glm::vec3 org, glm::vec3 wallNorm,
                                glm::vec3 lCol, float intensity,
                                float pSpeed, float pOffset, int count)
    : origin(org),
      m_wallNormal(wallNorm),   // fixed
      m_lightColor(lCol),       // fixed
      lightIntensity(intensity),
      pulseSpeed(pSpeed),
      pulseOffset(pOffset)
{
    glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 right, up;

    if (glm::abs(glm::dot(wallNorm, worldUp)) > 0.99f) {
        right = glm::vec3(1.0f, 0.0f, 0.0f);
        up    = glm::vec3(0.0f, 0.0f, -1.0f);
    } else {
        right = glm::normalize(glm::cross(wallNorm, worldUp));
        up    = glm::normalize(glm::cross(right, wallNorm));
    }

    for (int i = 0; i < count; i++) {
        float seed = (float)(i * 7 + 13);

        float h    = 0.4f  + pseudoRand(seed + 1.0f) * 1.2f;
        float r    = 0.07f + pseudoRand(seed + 2.0f) * 0.12f;
        float rotY = pseudoRand(seed + 3.0f) * 360.0f;
        float tilt = 5.0f  + pseudoRand(seed + 4.0f) * 20.0f;

        float spreadR = 0.6f * pseudoRand(seed + 5.0f);
        float spreadA = pseudoRand(seed + 6.0f) * 360.0f;
        float sr = spreadR * cos(glm::radians(spreadA));
        float su = spreadR * sin(glm::radians(spreadA));

        glm::vec3 pos = origin
            + right * sr
            + up    * su
            + wallNorm * 0.05f;

        // ── FIXED: use glm::cos/sin so types match cleanly ────────────
        float tiltAngleRad = glm::radians(pseudoRand(seed + 7.0f) * 360.0f);
        glm::vec3 tiltAx = glm::normalize(
            right * glm::cos(tiltAngleRad) +
            up    * glm::sin(tiltAngleRad)
        );

        float variation  = 0.8f + pseudoRand(seed + 8.0f) * 0.2f;
        glm::vec3 crystalColor = m_lightColor * variation;

        crystals.push_back(std::make_unique<Crystal>(
            pos, crystalColor, wallNorm, h, r, rotY, tilt, tiltAx
        ));
    }
}

glm::vec3 CrystalCluster::getLightPosition() const {
    return origin + m_wallNormal * 2.0f;   // fixed
}

glm::vec3 CrystalCluster::getAnimatedColor(float time) const {
    // float pulse = 0.75f + 0.25f * glm::sin(time * pulseSpeed + pulseOffset);
    return m_lightColor;        // fixed
}

float CrystalCluster::getAnimatedIntensity(float time) const {
    // float pulse = 0.75f + 0.25f * glm::sin(time * pulseSpeed + pulseOffset);
    // float pulse = 1.0f;
    // return lightIntensity * pulse;
    return lightIntensity;
}

void CrystalCluster::draw(Shader& shader,
                           const glm::mat4& view,
                           const glm::mat4& projection,
                           const glm::vec3& viewPos,
                           const glm::vec3& fogColor,
                           float fogDensity,
                           float time,
                           const std::vector<glm::vec3>& allLightPos,
                           const std::vector<glm::vec3>& allLightCol,
                           const std::vector<float>&     allLightInt)
{
    // float pulse = 0.75f + 0.25f * glm::sin(time * pulseSpeed + pulseOffset);
    float pulse = 1.0f;

    for (auto& crystal : crystals) {
        crystal->draw(shader, view, projection, viewPos,
                      fogColor, fogDensity, pulse,
                      allLightPos, allLightCol, allLightInt);
    }
}