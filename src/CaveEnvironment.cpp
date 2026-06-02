#include "CaveEnvironment.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/compatibility.hpp>
#include <cmath>
#include <iostream>

float CaveEnvironment::hashf(glm::vec2 p) {
    float v = glm::sin(glm::dot(p, glm::vec2(127.1f, 311.7f))) * 43758.5453f;
    return v - glm::floor(v);
}

float CaveEnvironment::smoothNoise(glm::vec2 p) {
    glm::vec2 i = glm::floor(p);
    glm::vec2 f = p - i;
    glm::vec2 u = f * f * (glm::vec2(3.0f) - 2.0f * f);
    float a = hashf(i);
    float b = hashf(i + glm::vec2(1.0f, 0.0f));
    float c = hashf(i + glm::vec2(0.0f, 1.0f));
    float d = hashf(i + glm::vec2(1.0f, 1.0f));
    return glm::mix(glm::mix(a, b, u.x), glm::mix(c, d, u.x), u.y);
}

// 5 octaves of smooth noise layered at doubling frequencies
float CaveEnvironment::fbm(glm::vec2 p) {
    float value = 0.0f, amp = 0.5f, freq = 1.0f;
    for (int i = 0; i < 5; i++) {
        value += amp * smoothNoise(p * freq);
        freq  *= 2.1f;
        amp   *= 0.5f;
    }
    return value;
}

// Sweeps a circle along Z, displacing each vertex radially with FBM noise
void CaveEnvironment::buildTube(float startZ, float endZ,
                                 float baseRadius,
                                 float irregularity,
                                 int rings, int segments)
{
    std::vector<Vertex> verts;
    std::vector<unsigned int> indices;

    float TWO_PI = 2.0f * 3.14159265f;

    for (int r = 0; r <= rings; r++) {
        float t = (float)r / (float)rings;
        float z = glm::mix(startZ, endZ, t);

        for (int s = 0; s < segments; s++) {
            float angle = (float)s / (float)segments * TWO_PI;
            float cosA = cos(angle);
            float sinA = sin(angle);

            glm::vec2 noiseCoord = glm::vec2(
                angle * 1.5f + z * 0.3f,
                z    * 0.4f  + angle * 0.8f
            );

            float noise  = fbm(noiseCoord);
            float radius = baseRadius + (noise - 0.5f) * 2.0f * irregularity;
            radius = glm::max(radius, baseRadius * 0.4f);

            // Ceiling pushed higher, floor flattened slightly
            if (sinA > 0.0f)
                radius += sinA * baseRadius * 0.35f;
            else
                radius += sinA * baseRadius * 0.15f;

            glm::vec3 pos(cosA * radius, sinA * radius, z);

            // Normal points inward toward the tube centre
            glm::vec3 normal = -glm::normalize(glm::vec3(cosA, sinA, 0.0f));

            Vertex v;
            v.Position = pos;
            v.Normal   = normal;
            v.TexCoord = glm::vec2((float)s / (float)segments, t);
            verts.push_back(v);
        }
    }

    // Connect rings into triangle pairs
    for (int r = 0; r < rings; r++) {
        for (int s = 0; s < segments; s++) {
            int curr      = r * segments + s;
            int next      = r * segments + (s + 1) % segments;
            int below     = (r + 1) * segments + s;
            int belowNext = (r + 1) * segments + (s + 1) % segments;

            // Counter-clockwise winding from inside
            indices.push_back(curr);
            indices.push_back(belowNext);
            indices.push_back(next);

            indices.push_back(curr);
            indices.push_back(below);
            indices.push_back(belowNext);
        }
    }

    for (auto& v : verts) v.Normal = glm::vec3(0.0f);
    for (int i = 0; i < (int)indices.size(); i += 3) {
        auto& v0 = verts[indices[i]];
        auto& v1 = verts[indices[i+1]];
        auto& v2 = verts[indices[i+2]];
        glm::vec3 faceNorm = glm::normalize(
            glm::cross(v1.Position - v0.Position,
                       v2.Position - v0.Position));
        v0.Normal += faceNorm;
        v1.Normal += faceNorm;
        v2.Normal += faceNorm;
    }
    for (auto& v : verts)
        if (glm::length(v.Normal) > 0.001f)
            v.Normal = glm::normalize(v.Normal);

    meshes.push_back(new Mesh(verts, indices));
}

// Builds a stalactite (pointsDown=true) or stalagmite (pointsDown=false)
void CaveEnvironment::buildDripstone(glm::vec3 base, float height,
                                      float radius, bool pointsDown)
{
    std::vector<Vertex>       verts;
    std::vector<unsigned int> indices;

    const int SEGMENTS = 10;
    const int RINGS    = 12;
    float TWO_PI = 2.0f * 3.14159265f;

    glm::vec3 tip = base + glm::vec3(0.0f,
        pointsDown ? -height : height, 0.0f);

    for (int r = 0; r <= RINGS; r++) {
        float t = (float)r / RINGS;
        float y = glm::mix(base.y, tip.y, t);

        // Power curve taper — stays wide longer before sharpening
        float taperT  = pow(t, 0.6f);
        float baseRad = radius * (1.0f - taperT);

        // Sine bulge peaks around t=0.3 for knobby mid-section
        float bulge = sin(t * 3.14159f) * radius * 0.25f;

        // FBM bump fades toward tip
        float heightBump = fbm(glm::vec2(
            t * 5.0f + base.x * 2.0f,
            base.z * 2.0f
        )) * radius * 0.3f * (1.0f - t);

        float ringRad = baseRad + bulge + heightBump;
        ringRad = glm::max(ringRad, 0.01f);

        for (int s = 0; s < SEGMENTS; s++) {
            float angle = (float)s / SEGMENTS * TWO_PI;

            // Per-vertex noise keeps cross section from being a perfect circle
            float angularNoise = fbm(glm::vec2(
                angle * 2.1f + base.x + t * 3.0f,
                t    * 4.7f  + base.z + angle
            ));
            float bumpedRad = ringRad * (0.75f + angularNoise * 0.5f)
                            * (1.0f - t * 0.3f);
            bumpedRad = glm::max(bumpedRad, 0.005f);

            // Slight Z wobble so dripstone isnt perfectly straight
            float zWobble = fbm(glm::vec2(
                angle * 1.5f + t * 2.0f,
                base.x + base.z
            )) * 0.12f * (1.0f - t);

            glm::vec3 pos(
                base.x + cos(angle) * bumpedRad,
                y,
                base.z + sin(angle) * bumpedRad + zWobble
            );

            glm::vec3 outward = glm::normalize(glm::vec3(cos(angle), 0.0f, sin(angle)));
            glm::vec3 tipDir  = glm::normalize(tip - pos);
            glm::vec3 normal  = glm::normalize(outward + tipDir * 0.5f);

            Vertex v;
            v.Position = pos;
            v.Normal   = normal;
            v.TexCoord = glm::vec2((float)s / SEGMENTS, t);
            verts.push_back(v);
        }
    }

    unsigned int tipIdx = (unsigned int)verts.size();
    verts.push_back({
        tip,
        glm::vec3(0.0f, pointsDown ? -1.0f : 1.0f, 0.0f),
        glm::vec2(0.5f, 1.0f)
    });

    for (int r = 0; r < RINGS; r++) {
        for (int s = 0; s < SEGMENTS; s++) {
            int curr      = r * SEGMENTS + s;
            int next      = r * SEGMENTS + (s + 1) % SEGMENTS;
            int below     = (r + 1) * SEGMENTS + s;
            int belowNext = (r + 1) * SEGMENTS + (s + 1) % SEGMENTS;

            indices.push_back(curr);
            indices.push_back(next);
            indices.push_back(belowNext);

            indices.push_back(curr);
            indices.push_back(belowNext);
            indices.push_back(below);
        }
    }

    int lastRing = RINGS * SEGMENTS;
    for (int s = 0; s < SEGMENTS; s++) {
        int curr = lastRing + s;
        int next = lastRing + (s + 1) % SEGMENTS;
        indices.push_back(curr);
        indices.push_back(next);
        indices.push_back(tipIdx);
    }

    for (auto& v : verts) v.Normal = glm::vec3(0.0f);
    for (int i = 0; i < (int)indices.size(); i += 3) {
        auto& v0 = verts[indices[i]];
        auto& v1 = verts[indices[i+1]];
        auto& v2 = verts[indices[i+2]];
        glm::vec3 fn = glm::normalize(
            glm::cross(v1.Position-v0.Position,
                       v2.Position-v0.Position));
        v0.Normal += fn;
        v1.Normal += fn;
        v2.Normal += fn;
    }
    for (auto& v : verts)
        if (glm::length(v.Normal) > 0.001f)
            v.Normal = glm::normalize(v.Normal);

    meshes.push_back(new Mesh(verts, indices));
}

// Extracts upward-facing triangles from the cave mesh for coral raycasting
std::vector<CaveEnvironment::Triangle> CaveEnvironment::getFloorTriangles() const {
    std::vector<Triangle> tris;
    for (auto* mesh : meshes) {
        auto& verts   = mesh->vertices;
        auto& indices = mesh->indices;
        for (int i = 0; i + 2 < (int)indices.size(); i += 3) {
            glm::vec3 v0 = verts[indices[i]].Position;
            glm::vec3 v1 = verts[indices[i+1]].Position;
            glm::vec3 v2 = verts[indices[i+2]].Position;
            glm::vec3 fn = glm::normalize(glm::cross(v1-v0, v2-v0));
            if (fn.y < -0.3f) tris.push_back({v0, v1, v2});
        }
    }
    return tris;
}

void CaveEnvironment::buildCave() {

    // Tight tunnel section
    buildTube(
        -1.0f,   // startZ
        22.0f,   // endZ
        2.2f,    // baseRadius
        0.8f,    // irregularity
        40,      // rings
        20       // segments
    );

    // Flare section where tunnel opens into the chamber
    {
        std::vector<Vertex>       verts;
        std::vector<unsigned int> indices;

        float TWO_PI   = 2.0f * 3.14159265f;
        int   rings    = 30;
        int   segments = 20;
        float startZ   =  2.0f;
        float endZ     = -8.0f;
        float startRad =  2.2f;
        float endRad   =  9.5f;

        for (int r = 0; r <= rings; r++) {
            float t   = (float)r / rings;
            float z   = glm::mix(startZ, endZ, t);
            float st  = t * t * (3.0f - 2.0f * t); // smoothstep
            float rad = glm::mix(startRad, endRad, st);

            for (int s = 0; s < segments; s++) {
                float angle = (float)s / segments * TWO_PI;
                float cosA  = cos(angle);
                float sinA  = sin(angle);

                // Same noise formula as buildTube so seams match
                glm::vec2 noiseCoord = glm::vec2(
                    angle * 1.5f + z * 0.3f,
                    z    * 0.4f  + angle * 0.8f
                );
                float noise  = fbm(noiseCoord);
                float radius = rad + (noise - 0.5f) * 2.0f
                             * glm::mix(0.8f, 2.5f, st);

                radius = glm::max(radius, rad * 0.4f);

                if (sinA > 0.0f)
                    radius += sinA * rad * 0.35f;
                else
                    radius += sinA * rad * 0.15f;

                glm::vec3 pos(cosA * radius, sinA * radius, z);
                glm::vec3 normal = -glm::normalize(glm::vec3(cosA, sinA, 0.0f));

                verts.push_back({
                    pos, normal,
                    glm::vec2((float)s / segments, t)
                });
            }
        }

        for (int r = 0; r < rings; r++) {
            for (int s = 0; s < segments; s++) {
                int curr      = r * segments + s;
                int next      = r * segments + (s + 1) % segments;
                int below     = (r + 1) * segments + s;
                int belowNext = (r + 1) * segments + (s + 1) % segments;
                indices.push_back(curr);
                indices.push_back(belowNext);
                indices.push_back(next);
                indices.push_back(curr);
                indices.push_back(below);
                indices.push_back(belowNext);
            }
        }

        for (auto& v : verts) v.Normal = glm::vec3(0.0f);
        for (int i = 0; i < (int)indices.size(); i += 3) {
            auto& v0 = verts[indices[i]];
            auto& v1 = verts[indices[i+1]];
            auto& v2 = verts[indices[i+2]];
            glm::vec3 fn = glm::normalize(
                glm::cross(v1.Position - v0.Position,
                           v2.Position - v0.Position));
            v0.Normal += fn;
            v1.Normal += fn;
            v2.Normal += fn;
        }
        for (auto& v : verts)
            if (glm::length(v.Normal) > 0.001f)
                v.Normal = glm::normalize(v.Normal);

        meshes.push_back(new Mesh(verts, indices));
    }

    // Main chamber
    buildTube(
        -7.0f,
       -28.0f,
        10.0f,
        2.5f,
        50,
        20
    );

    // Tunnel stalactites
    buildDripstone(glm::vec3(-0.3f,  3.0f, 14.0f), 1.0f, 0.25f, true);
    buildDripstone(glm::vec3( 0.5f,  3.0f,  9.0f), 1.3f, 0.30f, true);
    buildDripstone(glm::vec3(-0.6f,  3.0f,  5.0f), 0.8f, 0.20f, true);
    buildDripstone(glm::vec3( 0.2f,  3.0f, 17.0f), 0.6f, 0.18f, true);

    // Chamber stalactites
    buildDripstone(glm::vec3( 2.0f,  12.3f,  -7.0f), 2.5f, 0.55f, true);
    buildDripstone(glm::vec3(-3.0f,  13.0f, -10.0f), 3.0f, 0.65f, true);
    buildDripstone(glm::vec3( 4.0f,  12.9f, -14.0f), 2.0f, 0.45f, true);
    buildDripstone(glm::vec3(-1.0f,  14.2f, -18.0f), 3.5f, 0.70f, true);
    buildDripstone(glm::vec3( 3.5f,  13.4f, -22.0f), 2.8f, 0.60f, true);
    buildDripstone(glm::vec3(-4.0f,  10.2f,  -5.0f), 1.8f, 0.40f, true);
    buildDripstone(glm::vec3( 0.5f,  13.7f, -12.0f), 4.0f, 0.75f, true);
    buildDripstone(glm::vec3(-2.5f,  13.4f, -20.0f), 2.2f, 0.50f, true);
    buildDripstone(glm::vec3( 6.0f,  10.7f,  -8.0f), 1.5f, 0.35f, true);
    buildDripstone(glm::vec3(-5.0f,  11.8f, -16.0f), 2.0f, 0.48f, true);

    // Tunnel stalagmites
    buildDripstone(glm::vec3( 0.6f, -2.0f, 12.0f), 0.9f, 0.22f, false);
    buildDripstone(glm::vec3(-0.5f, -2.0f,  6.0f), 0.7f, 0.18f, false);

    // Chamber stalagmites
    buildDripstone(glm::vec3( 2.5f, -8.0f,  -8.0f), 2.0f, 0.45f, false);
    buildDripstone(glm::vec3(-3.5f, -8.0f, -12.0f), 2.8f, 0.55f, false);
    buildDripstone(glm::vec3( 1.0f, -8.0f, -16.0f), 1.5f, 0.35f, false);
    buildDripstone(glm::vec3(-2.0f, -8.0f, -21.0f), 3.2f, 0.65f, false);
    buildDripstone(glm::vec3( 4.5f, -8.0f, -19.0f), 1.8f, 0.40f, false);
    buildDripstone(glm::vec3(-6.0f, -8.0f,  -9.0f), 2.2f, 0.50f, false);
    buildDripstone(glm::vec3( 6.5f, -8.0f, -23.0f), 1.6f, 0.38f, false);

    // End caps to seal both ends of the cave
    buildEndCap(22.0f,  2.8f,  1.0f, 20);
    buildEndCap(-28.0f, 12.0f, 1.0f, 20);
}

// Fills the open end of a tube with a lumpy disc
void CaveEnvironment::buildEndCap(float centreZ, float radius,
                                   float normalDir, int segments)
{
    std::vector<Vertex>       verts;
    std::vector<unsigned int> indices;

    float TWO_PI = 2.0f * 3.14159265f;
    int rings = 12;

    for (int r = 0; r <= rings; r++) {
        float t   = (float)r / rings;
        float rad = radius * (1.0f - t);

        for (int s = 0; s < segments; s++) {
            float angle = (float)s / segments * TWO_PI;
            float cosA  = cos(angle);
            float sinA  = sin(angle);

            // Same noise as buildTube so edge vertices align
            glm::vec2 noiseCoord = glm::vec2(
                angle * 1.5f + centreZ * 0.3f,
                centreZ * 0.4f + angle * 0.8f
            );
            float noise = fbm(noiseCoord);

            float radiusNoise = (noise - 0.5f) * 2.0f * 0.8f * (1.0f - t);
            float bumpedRad   = rad + radiusNoise;
            bumpedRad = glm::max(bumpedRad, 0.0f);

            if (sinA > 0.0f)
                bumpedRad += sinA * radius * 0.35f * (1.0f - t);
            else
                bumpedRad += sinA * radius * 0.15f * (1.0f - t);

            // Z displacement for a rocky wall face
            float zNoise = fbm(glm::vec2(
                cosA * 0.8f + t * 2.0f,
                sinA * 0.8f + centreZ * 0.2f
            ));
            float zDisp = (zNoise - 0.5f) * 2.5f * (1.0f - t * t);

            glm::vec3 pos(cosA * bumpedRad, sinA * bumpedRad, centreZ + zDisp);

            Vertex v;
            v.Position = pos;
            v.Normal   = glm::vec3(0.0f, 0.0f, normalDir);
            v.TexCoord = glm::vec2((float)s / segments, t);
            verts.push_back(v);
        }
    }

    float centreNoise = fbm(glm::vec2(centreZ * 0.5f, 1.3f));
    float centreZDisp = (centreNoise - 0.5f) * 1.0f;
    unsigned int centreIdx = (unsigned int)verts.size();
    verts.push_back({
        glm::vec3(0.0f, 0.0f, centreZ + centreZDisp),
        glm::vec3(0.0f, 0.0f, normalDir),
        glm::vec2(0.5f, 0.5f)
    });

    for (int r = 0; r < rings - 1; r++) {
        for (int s = 0; s < segments; s++) {
            int curr      = r * segments + s;
            int next      = r * segments + (s + 1) % segments;
            int below     = (r + 1) * segments + s;
            int belowNext = (r + 1) * segments + (s + 1) % segments;

            if (normalDir > 0.0f) {
                indices.push_back(curr);
                indices.push_back(below);
                indices.push_back(belowNext);
                indices.push_back(curr);
                indices.push_back(belowNext);
                indices.push_back(next);
            } else {
                indices.push_back(curr);
                indices.push_back(belowNext);
                indices.push_back(below);
                indices.push_back(curr);
                indices.push_back(next);
                indices.push_back(belowNext);
            }
        }
    }

    int lastRing = (rings - 1) * segments;
    for (int s = 0; s < segments; s++) {
        int curr = lastRing + s;
        int next = lastRing + (s + 1) % segments;
        if (normalDir > 0.0f) {
            indices.push_back(curr);
            indices.push_back(centreIdx);
            indices.push_back(next);
        } else {
            indices.push_back(curr);
            indices.push_back(next);
            indices.push_back(centreIdx);
        }
    }

    for (auto& v : verts) v.Normal = glm::vec3(0.0f);
    for (int i = 0; i < (int)indices.size(); i += 3) {
        auto& v0 = verts[indices[i]];
        auto& v1 = verts[indices[i+1]];
        auto& v2 = verts[indices[i+2]];
        glm::vec3 fn = glm::normalize(
            glm::cross(v1.Position - v0.Position,
                       v2.Position - v0.Position));
        v0.Normal += fn;
        v1.Normal += fn;
        v2.Normal += fn;
    }
    for (auto& v : verts)
        if (glm::length(v.Normal) > 0.001f)
            v.Normal = glm::normalize(v.Normal);

    meshes.push_back(new Mesh(verts, indices));
}

CaveEnvironment::CaveEnvironment() {
    buildCave();
}

CaveEnvironment::~CaveEnvironment() {
    for (auto* m : meshes) {
        m->cleanup();
        delete m;
    }
}

void CaveEnvironment::draw(Shader& shader,
                            const glm::mat4& view,
                            const glm::mat4& projection,
                            const glm::vec3& viewPos) {
    shader.use();
    shader.setVec3 ("fogColor",         fogColor);
    shader.setFloat("fogDensity",       fogDensity);
    shader.setVec3 ("viewPos",          viewPos);
    shader.setMat4 ("view",             view);
    shader.setMat4 ("projection",       projection);
    shader.setBool ("isEmissive",       false);
    shader.setFloat("emissiveStrength", 0.0f);
    shader.setFloat("specularStrength", 0.04f);
    shader.setFloat("shininess",        4.0f);
    shader.setMat4 ("model",            glm::mat4(1.0f));
    shader.setVec3 ("objectColor",      rockColor);
    shader.setBool ("useProceduralTexture", true);
    shader.setBool ("isBone",           false);
    shader.setBool ("useTexture",       false);

    for (auto* m : meshes)
        m->draw();
}