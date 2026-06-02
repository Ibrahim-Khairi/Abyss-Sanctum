#include "Crystal.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

Crystal::Crystal(glm::vec3 pos, glm::vec3 col, glm::vec3 wNormal,
                 float h, float r,
                 float rotY, float tilt, glm::vec3 tiltAx)
    : position(pos), color(col), wallNormal(wNormal),
      height(h), radius(r),
      rotationY(rotY), tiltAngle(tilt), tiltAxis(tiltAx)
{
    buildCrystalMesh();
}

Crystal::~Crystal() {
    if (mesh) { mesh->cleanup(); delete mesh; }
}

// ─────────────────────────────────────────────
// buildCrystalMesh
//
// We build a hexagonal prism with a pyramid tip.
//
// HEXAGON CROSS-SECTION (top view):
// 6 points equally spaced around a circle at angles
// 0°, 60°, 120°, 180°, 240°, 300°
//
//        p0 (0°)
//       /    \
//    p5        p1 (60°)
//    |          |
//    p4        p2 (120°)
//       \    /
//        p3 (180°)
//
// The crystal has:
//   - 6 side faces (each a triangle: base edge → tip)
//   - A bottom cap (hexagon base, 6 triangles)
//
// We skip the full prism and go straight spiky —
// each side face goes from a base edge directly to the tip apex.
// This gives that sharp mana crystal look.
// ─────────────────────────────────────────────
void Crystal::buildCrystalMesh() {
    std::vector<Vertex>       verts;
    std::vector<unsigned int> indices;

    const int   SIDES  = 6;
    const float TWO_PI = 2.0f * 3.14159265f;

    // The tip (apex) of the crystal
    glm::vec3 apex(0.0f, height, 0.0f);

    // Base centre (for the bottom cap)
    glm::vec3 baseCenter(0.0f, 0.0f, 0.0f);

    // Compute the 6 base vertices around a circle
    // radius * 1.0 at y=0
    std::vector<glm::vec3> baseRing;
    for (int i = 0; i < SIDES; i++) {
        float angle = (float)i / (float)SIDES * TWO_PI;
        // Offset angle by 90° so a flat face faces forward
        float a = angle + glm::radians(90.0f);
        baseRing.push_back(glm::vec3(
            radius * cos(a),
            0.0f,
            radius * sin(a)
        ));
    }

    // ── Side faces ────────────────────────────────────────────────────
    // Each side face = one triangle: two adjacent base points + apex
    // Normal = cross product of two edges of that triangle
    for (int i = 0; i < SIDES; i++) {
        int next = (i + 1) % SIDES;

        glm::vec3 a = baseRing[i];
        glm::vec3 b = baseRing[next];
        glm::vec3 c = apex;

        // Face normal: cross product of two edge vectors
        glm::vec3 edge1 = b - a;
        glm::vec3 edge2 = c - a;
        glm::vec3 norm  = glm::normalize(glm::cross(edge1, edge2));

        unsigned int base = (unsigned int)verts.size();

        verts.push_back({ a, norm, glm::vec2(0.0f, 0.0f) });
        verts.push_back({ b, norm, glm::vec2(1.0f, 0.0f) });
        verts.push_back({ c, norm, glm::vec2(0.5f, 1.0f) });

        // One triangle per face
        indices.push_back(base);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
    }

    // ── Bottom cap ────────────────────────────────────────────────────
    // Hexagonal base — 6 triangles from centre to each edge
    // Normal points DOWN (0,-1,0) — faces the floor
    glm::vec3 downNorm(0.0f, -1.0f, 0.0f);

    unsigned int centreIdx = (unsigned int)verts.size();
    verts.push_back({ baseCenter, downNorm, glm::vec2(0.5f, 0.5f) });

    for (int i = 0; i < SIDES; i++) {
        float angle = (float)i / (float)SIDES * TWO_PI + glm::radians(90.0f);
        glm::vec3 p(radius * cos(angle), 0.0f, radius * sin(angle));
        float u = 0.5f + 0.5f * cos(angle);
        float v = 0.5f + 0.5f * sin(angle);
        verts.push_back({ p, downNorm, glm::vec2(u, v) });
    }

    for (int i = 0; i < SIDES; i++) {
        int curr = centreIdx + 1 + i;
        int next = centreIdx + 1 + (i + 1) % SIDES;
        // Wind counter-clockwise for downward-facing normal
        indices.push_back(centreIdx);
        indices.push_back(next);
        indices.push_back(curr);
    }

    mesh = new Mesh(verts, indices);
}

// ─────────────────────────────────────────────
// draw
// Builds the model matrix from position/rotation/tilt,
// sends all uniforms, draws the mesh.
// ─────────────────────────────────────────────
void Crystal::draw(Shader& shader,
                   const glm::mat4& view,
                   const glm::mat4& projection,
                   const glm::vec3& viewPos,
                   const glm::vec3& fogColor,
                   float fogDensity,
                   float pulse,
                   const std::vector<glm::vec3>& lightPositions,
                   const std::vector<glm::vec3>& lightColors,
                   const std::vector<float>&     lightIntensities)
{
    shader.use();

    // ── Build model matrix ────────────────────────────────────────────
    glm::mat4 model = glm::mat4(1.0f);

    // Step 1 — Move to world position
    model = glm::translate(model, position);

    // Step 2 — align crystal growth direction with wall normal
    // Crystal mesh grows along +Y by default.
    // We need to rotate it so +Y aligns with wallNormal.
    //
    // glm::rotation(from, to) finds the shortest rotation between
    // two unit vectors — perfect for this.
    // We use a manual approach since glm::rotation needs gtx/quaternion:

    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f); // crystal default growth axis
    glm::vec3 target = glm::normalize(wallNormal);

    float dotVal = glm::dot(up, target);

    if (dotVal < -0.999f) {
        // Exactly opposite — rotate 180° around X axis
        model = glm::rotate(model,
            glm::radians(180.0f),
            glm::vec3(1.0f, 0.0f, 0.0f));

    } else if (dotVal < 0.999f) {
        // Normal case — find rotation axis and angle
        // axis  = cross(up, target) — perpendicular to both
        // angle = acos(dot(up, target))
        glm::vec3 rotAxis  = glm::normalize(glm::cross(up, target));
        float     rotAngle = glm::acos(dotVal);
        model = glm::rotate(model, rotAngle, rotAxis);
    }
    // If dotVal >= 0.999 — already aligned (floor cluster), no rotation needed

    // Step 3 — random spin around growth axis for variety
    // Now that growth axis = wallNormal, we spin around wallNormal
    model = glm::rotate(model,
        glm::radians(rotationY),
        target);  // spin around growth direction

    // Step 4 — organic tilt off the wall normal
    model = glm::rotate(model,
        glm::radians(tiltAngle),
        tiltAxis);

    // ── Uniforms ──────────────────────────────────────────────────────
    shader.setMat4("model",      model);
    shader.setMat4("view",       view);
    shader.setMat4("projection", projection);
    shader.setVec3("viewPos",    viewPos);

    // Crystal colour pulsed by the glow animation
    shader.setVec3("objectColor", color * pulse);

    // Fog
    shader.setVec3 ("fogColor",   fogColor);
    shader.setFloat("fogDensity", fogDensity);

    // ── Emissive — crystal glows from within, no external lighting ────
    shader.setBool ("isEmissive",       true);
    // shader.setFloat("emissiveStrength", pulse * 1.4f);
    shader.setFloat("emissiveStrength", 1.4f);
    // pulse animates between 0.75-1.0, so emissive strength = 1.05-1.4
    // Multiplying by 1.4 makes it brighter than objectColor alone
    // giving that intense inner-glow look

    // shader.setVec3("objectColor", color * pulse);
    shader.setVec3("objectColor", color);

    // Crystals are very shiny — tight specular highlight
    shader.setFloat("specularStrength", 0.0f);
    shader.setFloat("shininess",        1.0f);

    // ── Multi-light uniforms ───────────────────────────────────────────
    // Send all lights as arrays. GLSL arrays are set element by element.
    int numLights = (int)std::min(lightPositions.size(), (size_t)12);
    shader.setInt("numLights", numLights);
    for (int i = 0; i < numLights; i++) {
        shader.setVec3 ("lightPositions["  + std::to_string(i) + "]",
                        lightPositions[i]);
        shader.setVec3 ("lightColors["     + std::to_string(i) + "]",
                        lightColors[i]);
        shader.setFloat("lightIntensities["+ std::to_string(i) + "]",
                        lightIntensities[i]);
    }

    mesh->draw();
}