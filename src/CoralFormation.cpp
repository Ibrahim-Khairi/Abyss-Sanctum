#include "CoralFormation.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

float CoralFormation::hashf(glm::vec2 p) {
    float v = glm::sin(glm::dot(p, glm::vec2(127.1f, 311.7f)))*43758.5453f;
    return v - glm::floor(v);
}

float CoralFormation::smoothNoise(glm::vec2 p) {
    glm::vec2 i=glm::floor(p), f=p-i;
    glm::vec2 u=f*f*(glm::vec2(3.0f)-2.0f*f);
    return glm::mix(glm::mix(hashf(i),hashf(i+glm::vec2(1,0)),u.x),
                    glm::mix(hashf(i+glm::vec2(0,1)),hashf(i+glm::vec2(1,1)),u.x),u.y);
}

float CoralFormation::fbm(glm::vec2 p) {
    float v=0,a=0.5f,f=1.0f;
    for(int i=0;i<4;i++){v+=a*smoothNoise(p*f);f*=2.1f;a*=0.5f;}
    return v;
}

// Tapered cylinder with lean, jaggedness noise, and random spike protrusions
void CoralFormation::buildBranchMesh(float height, float baseRadius,
                                      float jaggedness, int seed)
{
    std::vector<Vertex>       verts;
    std::vector<unsigned int> indices;

    int   RINGS    = 14;
    int   SEGMENTS = 8;
    float TWO_PI   = 2.0f * 3.14159265f;

    glm::vec3 tip(0.0f, height, 0.0f);

    // Seed-hashed lean direction so each branch curves differently
    float leanX = (hashf(glm::vec2(seed, 1)) - 0.5f) * 0.4f;
    float leanZ = (hashf(glm::vec2(seed, 2)) - 0.5f) * 0.4f;

    for (int r = 0; r <= RINGS; r++) {
        float t    = (float)r / RINGS;
        float y    = height * t;

        float taper   = 1.0f - pow(t, 0.7f);
        float ringRad = baseRadius * taper;

        // Jaggedness increases toward tip
        float jag = fbm(glm::vec2(t * 4.0f + seed, seed * 0.3f))
                  * jaggedness * (0.3f + t * 0.7f);

        // Progressive lean offset per ring
        float lx = leanX * t * height * 0.3f;
        float lz = leanZ * t * height * 0.3f;

        for (int s = 0; s < SEGMENTS; s++) {
            float angle = (float)s / SEGMENTS * TWO_PI;

            // Angular noise keeps cross section from being a perfect circle
            float angNoise = fbm(glm::vec2(
                angle * 1.8f + t * 3.0f + seed,
                t * 5.0f + seed * 2.0f
            ));

            // ~18% of vertices get a sharp spike protrusion
            float spikeNoise = hashf(glm::vec2(
                (float)r * 7.3f + (float)seed,
                (float)s * 3.7f
            ));
            float spike = (spikeNoise > 0.82f) ?
                (spikeNoise - 0.82f) * 4.0f * baseRadius : 0.0f;

            float finalRad = ringRad * (0.7f + angNoise * 0.6f) + jag + spike;
            finalRad = glm::max(finalRad, 0.005f);

            glm::vec3 pos(lx + cos(angle) * finalRad, y, lz + sin(angle) * finalRad);
            glm::vec3 outward = glm::normalize(glm::vec3(cos(angle), 0.1f, sin(angle)));

            Vertex v;
            v.Position = pos;
            v.Normal   = outward;
            v.TexCoord = glm::vec2((float)s/SEGMENTS, t);
            verts.push_back(v);
        }
    }

    unsigned int tipIdx = (unsigned int)verts.size();
    verts.push_back({ tip + glm::vec3(leanX*height*0.3f, 0, leanZ*height*0.3f),
                      glm::vec3(0,1,0), glm::vec2(0.5f,1.0f) });

    for (int r = 0; r < RINGS; r++) {
        for (int s = 0; s < SEGMENTS; s++) {
            int curr=r*SEGMENTS+s, next=r*SEGMENTS+(s+1)%SEGMENTS;
            int below=(r+1)*SEGMENTS+s, belowNext=(r+1)*SEGMENTS+(s+1)%SEGMENTS;
            indices.push_back(curr); indices.push_back(next);
            indices.push_back(belowNext);
            indices.push_back(curr); indices.push_back(belowNext);
            indices.push_back(below);
        }
    }

    int lastRing = RINGS * SEGMENTS;
    for (int s = 0; s < SEGMENTS; s++) {
        indices.push_back(lastRing+s);
        indices.push_back(lastRing+(s+1)%SEGMENTS);
        indices.push_back(tipIdx);
    }

    for (auto& v : verts) v.Normal = glm::vec3(0.0f);
    for (int i = 0; i < (int)indices.size(); i+=3) {
        auto& v0=verts[indices[i]]; auto& v1=verts[indices[i+1]];
        auto& v2=verts[indices[i+2]];
        glm::vec3 fn=glm::normalize(glm::cross(v1.Position-v0.Position,
                                               v2.Position-v0.Position));
        v0.Normal+=fn; v1.Normal+=fn; v2.Normal+=fn;
    }
    for (auto& v : verts)
        if (glm::length(v.Normal)>0.001f)
            v.Normal=glm::normalize(v.Normal);

    branchMeshes.push_back(new Mesh(verts, indices));
}

// Fires a ray downward and returns the Y of the first floor triangle hit
float CoralFormation::raycastFloorY(float x, float z,
    const std::vector<CaveEnvironment::Triangle>& tris)
{
    glm::vec3 rayOrigin(x, 20.0f, z);
    glm::vec3 rayDir(0.0f, -1.0f, 0.0f);

    float bestY = -8.0f;
    bool  hit   = false;

    for (auto& tri : tris) {
        // Moller-Trumbore ray-triangle intersection
        glm::vec3 edge1 = tri.v1 - tri.v0;
        glm::vec3 edge2 = tri.v2 - tri.v0;
        glm::vec3 h     = glm::cross(rayDir, edge2);
        float     a     = glm::dot(edge1, h);

        if (fabs(a) < 1e-6f) continue;

        float     f = 1.0f / a;
        glm::vec3 s = rayOrigin - tri.v0;
        float     u = f * glm::dot(s, h);
        if (u < 0.0f || u > 1.0f) continue;

        glm::vec3 q = glm::cross(s, edge1);
        float     v = f * glm::dot(rayDir, q);
        if (v < 0.0f || u + v > 1.0f) continue;

        float t = f * glm::dot(edge2, q);
        if (t < 0.0f) continue;

        float hitY = rayOrigin.y + t * rayDir.y;
        if (!hit || hitY < bestY) {
            bestY = hitY;
            hit   = true;
        }
    }

    return bestY + 0.15f;
}

void CoralFormation::placeCoral() {
    auto floorTris = caveEnv->getFloorTriangles();

    // Reef colour palette
    glm::vec3 staghorn   = glm::vec3(171/255.0f, 116/255.0f,  54/255.0f);
    glm::vec3 brainOchre = glm::vec3(194/255.0f, 151/255.0f,  74/255.0f);
    glm::vec3 reefOlive  = glm::vec3(112/255.0f, 118/255.0f,  77/255.0f);
    glm::vec3 acropora   = glm::vec3(219/255.0f,  78/255.0f, 156/255.0f);
    glm::vec3 neonGreen  = glm::vec3( 57/255.0f, 242/255.0f, 122/255.0f);

    std::vector<glm::vec3> colors = {
        staghorn, brainOchre, reefOlive, acropora, neonGreen
    };

    // Tunnel coral placed against walls
    instances.push_back({ glm::vec3(-1.6f, -1.5f, 14.0f), 0.5f,  20.0f, staghorn   });
    instances.push_back({ glm::vec3( 1.0f, -1.5f,  9.5f), 0.4f, 150.0f, brainOchre });
    instances.push_back({ glm::vec3(-1.5f, -1.5f,  5.0f), 0.5f,  80.0f, reefOlive  });
    instances.push_back({ glm::vec3( 1.5f, -1.6f,  2.0f), 0.4f, 220.0f, acropora   });

    struct GridPoint { float x, y, z; glm::vec3 color; float scale; float rot; };
    std::vector<GridPoint> grid;

    // Dense grid across chamber floor, X: -8 to +8, Z: -2 to -25
    float spacing = 1.8f;
    for (float z = -2.0f; z >= -25.0f; z -= spacing) {
        for (float x = -8.0f; x <= 8.0f; x += spacing) {

            // Skip skeleton area
            if (x > -0.5f && x < 3.5f && z < -11.5f && z > -18.5f) continue;

            // Skip boulder positions
            if (glm::length(glm::vec2(x - (-6.0f), z - (-7.0f)))  < 2.0f) continue;
            if (glm::length(glm::vec2(x - ( 5.5f), z - (-11.0f))) < 2.0f) continue;
            if (glm::length(glm::vec2(x - (-4.0f), z - (-16.0f))) < 2.0f) continue;
            if (glm::length(glm::vec2(x - ( 6.5f), z - (-20.0f))) < 2.0f) continue;
            if (glm::length(glm::vec2(x - (-7.0f), z - (-22.0f))) < 1.5f) continue;

            // Position jitter so the grid doesnt look mechanical
            float jx = (hashf(glm::vec2(x, z))        - 0.5f) * 0.6f;
            float jz = (hashf(glm::vec2(z, x + 1.0f)) - 0.5f) * 0.6f;

            // Raycast to place coral exactly on the cave floor surface
            float jy = raycastFloorY(x + jx, z + jz, floorTris)
                     + hashf(glm::vec2(x * 2.0f, z * 1.5f)) * 0.15f;

            float scale = 0.6f + hashf(glm::vec2(x + z, z - x)) * 0.7f;
            float rot   = hashf(glm::vec2(x * 3.0f, z * 2.0f)) * 360.0f;

            // Pick colour from palette using hash
            int colorIdx = (int)(hashf(glm::vec2(x * 1.7f, z * 2.3f)) * 5) % 5;
            grid.push_back({ x + jx, jy, z + jz, colors[colorIdx], scale, rot });
        }
    }

    for (auto& g : grid) {
        instances.push_back({
            glm::vec3(g.x, g.y, g.z),
            g.scale, g.rot, g.color
        });
    }
}

CoralFormation::CoralFormation(CaveEnvironment* cave) : caveEnv(cave) {
    buildBranchMesh(1.2f, 0.12f, 0.15f, 1);
    buildBranchMesh(1.8f, 0.10f, 0.20f, 5);
    buildBranchMesh(0.9f, 0.14f, 0.12f, 9);
    buildBranchMesh(2.2f, 0.08f, 0.25f, 13);
    buildBranchMesh(1.5f, 0.11f, 0.18f, 17);
    placeCoral();
}

CoralFormation::~CoralFormation() {
    for (auto* m : branchMeshes) { m->cleanup(); delete m; }
}

void CoralFormation::draw(Shader& shader,
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
    shader.setFloat("specularStrength", 0.06f);
    shader.setFloat("shininess",        8.0f);
    shader.setBool ("useProceduralTexture", false);
    shader.setBool ("isBone",           false);
    shader.setBool ("useTexture",       false);

    int numLights = (int)std::min(lightPositions.size(), (size_t)12);
    shader.setInt("numLights", numLights);
    for (int i = 0; i < numLights; i++) {
        shader.setVec3 ("lightPositions["  +std::to_string(i)+"]", lightPositions[i]);
        shader.setVec3 ("lightColors["     +std::to_string(i)+"]", lightColors[i]);
        shader.setFloat("lightIntensities["+std::to_string(i)+"]", lightIntensities[i]);
    }

    int meshCount = (int)branchMeshes.size();
    for (auto& inst : instances) {
        // Draw 4-6 branches per cluster with randomised offsets and tilt
        int branchCount = 4 + (int)(hashf(glm::vec2(inst.rotY, 1.0f)) * 3);
        for (int b = 0; b < branchCount; b++) {
            float bSeed   = (float)b * 7.3f + inst.rotY;
            float offsetX = (hashf(glm::vec2(bSeed, 1.0f)) - 0.5f) * 0.5f * inst.scale;
            float offsetZ = (hashf(glm::vec2(bSeed, 2.0f)) - 0.5f) * 0.5f * inst.scale;
            float branchRot = hashf(glm::vec2(bSeed, 3.0f)) * 360.0f;
            float tilt      = hashf(glm::vec2(bSeed, 4.0f)) * 25.0f;

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, inst.position + glm::vec3(offsetX, 0.0f, offsetZ));
            model = glm::rotate(model, glm::radians(inst.rotY + branchRot),
                                glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(tilt),
                                glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::scale(model, glm::vec3(inst.scale));

            shader.setMat4("model",       model);
            shader.setVec3("objectColor", inst.color);

            branchMeshes[b % meshCount]->draw();
        }
    }
}