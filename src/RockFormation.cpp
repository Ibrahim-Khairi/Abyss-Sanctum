#include "RockFormation.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

float RockFormation::hashf(glm::vec2 p) {
    float v = glm::sin(glm::dot(p, glm::vec2(127.1f, 311.7f))) * 43758.5453f;
    return v - glm::floor(v);
}
float RockFormation::smoothNoise(glm::vec2 p) {
    glm::vec2 i = glm::floor(p);
    glm::vec2 f = p - i;
    glm::vec2 u = f * f * (glm::vec2(3.0f) - 2.0f * f);
    float a = hashf(i), b = hashf(i + glm::vec2(1,0));
    float c = hashf(i + glm::vec2(0,1)), d = hashf(i + glm::vec2(1,1));
    return glm::mix(glm::mix(a,b,u.x), glm::mix(c,d,u.x), u.y);
}
float RockFormation::fbm(glm::vec2 p) {
    float v=0, a=0.5f, f=1.0f;
    for(int i=0;i<4;i++){v+=a*smoothNoise(p*f);f*=2.1f;a*=0.5f;}
    return v;
}

// ─────────────────────────────────────────────
// buildRockMesh
// A rock is a sphere with heavy FBM displacement
// applied to each vertex radially outward.
// More displacement = more jagged and irregular.
// ─────────────────────────────────────────────
void RockFormation::buildRockMesh(float baseRadius, int noiseSeed) {
    std::vector<Vertex>       verts;
    std::vector<unsigned int> indices;

    // UV sphere generation
    // stacks = rows from top to bottom
    // slices = columns around equator
    int stacks = 10;
    int slices  = 12;
    float PI    = 3.14159265f;

    for (int i = 0; i <= stacks; i++) {
        float phi = PI * (float)i / stacks; // 0=top, PI=bottom

        for (int j = 0; j <= slices; j++) {
            float theta = 2.0f * PI * (float)j / slices;

            // Base sphere position
            float x = sin(phi) * cos(theta);
            float y = cos(phi);
            float z = sin(phi) * sin(theta);

            // FBM displacement — use sphere surface coords as noise input
            // Different seed per rock so they all look different
            float noise = fbm(glm::vec2(
                phi  * 3.0f + (float)noiseSeed * 0.7f,
                theta * 2.0f + (float)noiseSeed * 1.3f
            ));

            // Large irregular displacement — rocks are chunky not smooth
            float disp   = 0.6f + noise * 0.8f;
            float radius = baseRadius * disp;

            glm::vec3 pos(x * radius, y * radius, z * radius);

            // Normal: outward from displaced position
            glm::vec3 normal = glm::normalize(glm::vec3(x, y, z));

            Vertex v;
            v.Position = pos;
            v.Normal   = normal;
            v.TexCoord = glm::vec2((float)j/slices, (float)i/stacks);
            verts.push_back(v);
        }
    }

    // Indices
    for (int i = 0; i < stacks; i++) {
        for (int j = 0; j < slices; j++) {
            int curr  = i * (slices+1) + j;
            int next  = curr + 1;
            int below = curr + slices + 1;
            int belowNext = below + 1;

            indices.push_back(curr);
            indices.push_back(below);
            indices.push_back(next);
            indices.push_back(next);
            indices.push_back(below);
            indices.push_back(belowNext);
        }
    }

    // Recalc normals from actual geometry
    for (auto& v : verts) v.Normal = glm::vec3(0.0f);
    for (int i = 0; i < (int)indices.size(); i += 3) {
        auto& v0 = verts[indices[i]];
        auto& v1 = verts[indices[i+1]];
        auto& v2 = verts[indices[i+2]];
        glm::vec3 fn = glm::normalize(
            glm::cross(v1.Position-v0.Position,
                       v2.Position-v0.Position));
        v0.Normal+=fn; v1.Normal+=fn; v2.Normal+=fn;
    }
    for (auto& v : verts)
        if (glm::length(v.Normal) > 0.001f)
            v.Normal = glm::normalize(v.Normal);

    if (rockMesh) { rockMesh->cleanup(); delete rockMesh; }
    rockMesh = new Mesh(verts, indices);
}

void RockFormation::buildPebbleMesh() {
    // Reuse rock mesh logic but smaller and with different seed
    buildRockMesh(0.15f, 42);
    pebbleMesh = rockMesh;
    rockMesh   = nullptr;
    buildRockMesh(0.5f, 7);
}

// ─────────────────────────────────────────────
// placeRocks
// Define where each boulder and pebble sits.
// Y positions approximate the cave floor level.
// ─────────────────────────────────────────────
void RockFormation::placeRocks() {
    // ── Boulders — large, against walls and floor ─────────────────────
    // Tunnel boulders
    boulders.push_back({ glm::vec3(-1.2f, -1.6f, 13.0f), 1.0f,  30.0f, 5.0f,  1 });
    boulders.push_back({ glm::vec3( 1.0f, -1.6f,  8.0f), 0.8f,  75.0f, 0.0f,  2 });
    boulders.push_back({ glm::vec3(-0.8f, -1.6f,  4.5f), 0.7f, 120.0f, 8.0f,  3 });

    // Chamber boulders — larger, more dramatic
    boulders.push_back({ glm::vec3(-6.0f, -7.5f,  -7.0f), 2.2f,  45.0f, 12.0f, 4 });
    boulders.push_back({ glm::vec3( 5.5f, -7.5f, -11.0f), 1.8f,  90.0f,  6.0f, 5 });
    boulders.push_back({ glm::vec3(-4.0f, -7.5f, -16.0f), 2.5f, 160.0f, 15.0f, 6 });
    boulders.push_back({ glm::vec3( 6.5f, -7.5f, -20.0f), 2.0f,  20.0f,  3.0f, 7 });
    boulders.push_back({ glm::vec3(-7.0f, -7.0f, -22.0f), 1.6f, 200.0f, 10.0f, 8 });
    boulders.push_back({ glm::vec3( 2.0f, -7.5f, -24.0f), 1.4f, 310.0f,  0.0f, 9 });

    // Cluster of boulders near back wall
    boulders.push_back({ glm::vec3(-2.5f, -7.5f, -23.0f), 1.2f,  55.0f,  7.0f, 10 });
    boulders.push_back({ glm::vec3( 4.0f, -7.0f, -22.5f), 1.0f, 135.0f,  4.0f, 11 });

    // ── Pebbles — scattered on floor ──────────────────────────────────
    // Tunnel floor
    pebbles.push_back({ glm::vec3( 0.3f, -1.7f, 16.0f), 0.8f,  20.0f, 0.0f, 20 });
    pebbles.push_back({ glm::vec3(-0.5f, -1.7f, 12.5f), 0.5f, 145.0f, 0.0f, 21 });
    pebbles.push_back({ glm::vec3( 0.7f, -1.7f,  7.0f), 0.6f,  80.0f, 0.0f, 22 });
    pebbles.push_back({ glm::vec3(-0.3f, -1.7f,  3.0f), 0.4f, 210.0f, 0.0f, 23 });
    pebbles.push_back({ glm::vec3( 0.9f, -1.7f, 18.5f), 0.4f, 310.0f, 0.0f, 24 });
    pebbles.push_back({ glm::vec3(-0.8f, -1.7f, 17.2f), 0.6f,  55.0f, 0.0f, 25 });
    pebbles.push_back({ glm::vec3( 0.4f, -1.7f, 15.1f), 0.3f, 190.0f, 0.0f, 26 });
    pebbles.push_back({ glm::vec3(-0.6f, -1.7f, 13.8f), 0.7f, 270.0f, 0.0f, 27 });
    pebbles.push_back({ glm::vec3( 0.2f, -1.7f, 11.3f), 0.5f,  35.0f, 0.0f, 28 });
    pebbles.push_back({ glm::vec3(-0.9f, -1.7f,  9.7f), 0.4f, 125.0f, 0.0f, 29 });
    pebbles.push_back({ glm::vec3( 0.6f, -1.7f,  8.4f), 0.8f, 230.0f, 0.0f, 30 });
    pebbles.push_back({ glm::vec3(-0.2f, -1.7f,  6.1f), 0.3f, 315.0f, 0.0f, 31 });
    pebbles.push_back({ glm::vec3( 0.8f, -1.7f,  4.8f), 0.6f,  70.0f, 0.0f, 32 });
    pebbles.push_back({ glm::vec3(-0.7f, -1.7f,  2.2f), 0.5f, 160.0f, 0.0f, 33 });

    // Chamber floor — scattered widely
    pebbles.push_back({ glm::vec3( 1.5f, -8.0f,  -6.0f), 0.9f,  33.0f, 0.0f, 24 });
    pebbles.push_back({ glm::vec3(-2.0f, -8.0f,  -9.0f), 0.6f, 170.0f, 0.0f, 25 });
    pebbles.push_back({ glm::vec3( 3.5f, -8.0f, -13.0f), 0.7f,  95.0f, 0.0f, 26 });
    pebbles.push_back({ glm::vec3(-1.0f, -8.0f, -17.0f), 1.0f, 250.0f, 0.0f, 27 });
    pebbles.push_back({ glm::vec3( 0.5f, -8.0f, -21.0f), 0.5f,  60.0f, 0.0f, 28 });
    pebbles.push_back({ glm::vec3(-3.5f, -8.0f, -14.0f), 0.8f, 190.0f, 0.0f, 29 });
    pebbles.push_back({ glm::vec3( 2.5f, -8.0f, -19.0f), 0.6f, 310.0f, 0.0f, 30 });
}

RockFormation::RockFormation() {
    buildPebbleMesh();
    placeRocks();
}

RockFormation::~RockFormation() {
    if (rockMesh)   { rockMesh->cleanup();   delete rockMesh;   }
    if (pebbleMesh) { pebbleMesh->cleanup(); delete pebbleMesh; }
}

void RockFormation::draw(Shader& shader,
                          const glm::mat4& view,
                          const glm::mat4& projection,
                          const glm::vec3& viewPos,
                          const glm::vec3& fogColor,
                          float fogDensity,
                          const std::vector<glm::vec3>& lightPositions,
                          const std::vector<glm::vec3>& lightColors,
                          const std::vector<float>&     lightIntensities)
{
    shader.use();
    shader.setMat4 ("view",             view);
    shader.setMat4 ("projection",       projection);
    shader.setVec3 ("viewPos",          viewPos);
    shader.setVec3 ("fogColor",         fogColor);
    shader.setFloat("fogDensity",       fogDensity);
    shader.setBool ("isEmissive",       false);
    shader.setFloat("emissiveStrength", 0.0f);
    shader.setFloat("specularStrength", 0.03f);
    shader.setFloat("shininess",        4.0f);  
    shader.setBool("useProceduralTexture", false);
    shader.setBool("isBone", false);
    shader.setBool("useTexture", false);

    // Send lights
    int numLights = (int)std::min(lightPositions.size(), (size_t)12);
    shader.setInt("numLights", numLights);
    for (int i = 0; i < numLights; i++) {
        shader.setVec3 ("lightPositions["  +std::to_string(i)+"]", lightPositions[i]);
        shader.setVec3 ("lightColors["     +std::to_string(i)+"]", lightColors[i]);
        shader.setFloat("lightIntensities["+std::to_string(i)+"]", lightIntensities[i]);
    }

    // Draw boulders — each one is a freshly generated mesh with unique seed
    for (auto& b : boulders) {
        // Rebuild mesh with this rock's unique seed
        buildRockMesh(0.5f * b.scale, b.seed);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, b.position);
        model = glm::rotate(model, glm::radians(b.rotY),
                            glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(b.rotX),
                            glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(b.scale));

        shader.setMat4("model",       model);
        // Slightly darker than cave walls — rocks are denser stone
        shader.setVec3("objectColor", glm::vec3(0.05f, 0.04f, 0.04f));  // boulders
        rockMesh->draw();
    }

    // Draw pebbles
    for (auto& p : pebbles) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, p.position);
        model = glm::rotate(model, glm::radians(p.rotY),
                            glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(p.scale * 0.3f));

        shader.setMat4("model",       model);
        shader.setVec3("objectColor", glm::vec3(0.07f, 0.06f, 0.05f));  // pebbles
        pebbleMesh->draw();
    }
}