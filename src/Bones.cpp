#include "Bones.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

float Bones::hashf(glm::vec2 p) {
    float v = glm::sin(glm::dot(p, glm::vec2(127.1f, 311.7f))) * 43758.5453f;
    return v - glm::floor(v);
}

// Skull base shape, scaled along each axis for an alligator-like profile
void Bones::buildEllipsoid(float sx, float sy, float sz,
                                    int stacks, int slices, float roughness)
{
    std::vector<Vertex>       verts;
    std::vector<unsigned int> indices;
    float PI = 3.14159265f;

    for (int i = 0; i <= stacks; i++) {
        float phi = PI * (float)i / stacks;

        for (int j = 0; j <= slices; j++) {
            float theta = 2.0f * PI * (float)j / slices;

            float x = sin(phi) * cos(theta);
            float y = cos(phi);
            float z = sin(phi) * sin(theta);

            // Noise-based pitting for surface roughness
            float noise = hashf(glm::vec2(phi * 5.3f, theta * 3.7f));
            float pit   = hashf(glm::vec2(phi * 11.0f, theta * 8.0f));
            float disp  = 1.0f - roughness * (noise * 0.6f + pit * 0.4f) * 0.30f;

            glm::vec3 pos(x * sx * disp, y * sy * disp, z * sz * disp);
            glm::vec3 normal = glm::normalize(glm::vec3(x/sx, y/sy, z/sz));

            Vertex v;
            v.Position = pos;
            v.Normal   = normal;
            v.TexCoord = glm::vec2((float)j/slices, (float)i/stacks);
            verts.push_back(v);
        }
    }

    for (int i = 0; i < stacks; i++) {
        for (int j = 0; j < slices; j++) {
            int curr=i*(slices+1)+j, next=curr+1;
            int below=curr+slices+1, belowNext=below+1;
            indices.push_back(curr); indices.push_back(below);
            indices.push_back(next);
            indices.push_back(next); indices.push_back(below);
            indices.push_back(belowNext);
        }
    }

    for (auto& v : verts) v.Normal = glm::vec3(0.0f);
    for (int i = 0; i < (int)indices.size(); i += 3) {
        auto& v0=verts[indices[i]];
        auto& v1=verts[indices[i+1]];
        auto& v2=verts[indices[i+2]];
        glm::vec3 fn = glm::normalize(
            glm::cross(v1.Position-v0.Position,
                       v2.Position-v0.Position));
        v0.Normal+=fn; v1.Normal+=fn; v2.Normal+=fn;
    }
    for (auto& v : verts)
        if (glm::length(v.Normal) > 0.001f)
            v.Normal = glm::normalize(v.Normal);

    boneMeshes.push_back(new Mesh(verts, indices));
    boneTransforms.push_back(glm::mat4(1.0f));
}

// Single conical tooth, tip curves forward based on curve param
// Hero fangs: height=0.5, radius=0.12, curve=0.18
// Standard teeth: height=0.18, radius=0.05, curve=0.04
void Bones::buildTooth(float height, float baseRadius, float curve) {
    std::vector<Vertex>       verts;
    std::vector<unsigned int> indices;

    int   SEGS   = 7;
    int   RINGS  = 6;
    float TWO_PI = 2.0f * 3.14159265f;

    glm::vec3 tip(curve, height, 0.0f);

    for (int r = 0; r <= RINGS; r++) {
        float t      = (float)r / RINGS;
        float y      = height * t;
        float taper  = pow(1.0f - t, 0.6f);
        float rad    = baseRadius * taper;
        float leanX  = curve * t;

        for (int s = 0; s < SEGS; s++) {
            float angle = (float)s / SEGS * TWO_PI;
            // Slight irregularity so cross section isnt a perfect circle
            float irregularity = 1.0f + hashf(glm::vec2(
                (float)s * 3.1f, (float)r * 7.3f)) * 0.12f;

            glm::vec3 pos(
                leanX + cos(angle) * rad * irregularity,
                y,
                sin(angle) * rad * irregularity
            );
            glm::vec3 outward = glm::normalize(
                glm::vec3(cos(angle), 0.2f, sin(angle)));

            Vertex v;
            v.Position = pos;
            v.Normal   = outward;
            v.TexCoord = glm::vec2((float)s/SEGS, t);
            verts.push_back(v);
        }
    }

    unsigned int tipIdx = (unsigned int)verts.size();
    verts.push_back({ tip, glm::vec3(0,1,0), glm::vec2(0.5f,1.0f) });

    for (int r = 0; r < RINGS; r++) {
        for (int s = 0; s < SEGS; s++) {
            int curr=r*SEGS+s, next=r*SEGS+(s+1)%SEGS;
            int below=(r+1)*SEGS+s, belowNext=(r+1)*SEGS+(s+1)%SEGS;
            indices.push_back(curr); indices.push_back(next);
            indices.push_back(belowNext);
            indices.push_back(curr); indices.push_back(belowNext);
            indices.push_back(below);
        }
    }

    int lastRing = RINGS * SEGS;
    for (int s = 0; s < SEGS; s++) {
        indices.push_back(lastRing+s);
        indices.push_back(lastRing+(s+1)%SEGS);
        indices.push_back(tipIdx);
    }

    boneMeshes.push_back(new Mesh(verts, indices));
    boneTransforms.push_back(glm::mat4(1.0f));
}

void Bones::buildRib(float length, float thickness, float curve) {
    std::vector<Vertex>       verts;
    std::vector<unsigned int> indices;

    int   PATH_STEPS = 16;
    int   RING_SEGS  = 7;
    float TWO_PI     = 2.0f * 3.14159265f;
    float PI         = 3.14159265f;

    // Build curved path along rib length
    std::vector<glm::vec3> path;
    for (int i = 0; i <= PATH_STEPS; i++) {
        float t = (float)i / PATH_STEPS;
        path.push_back(glm::vec3(
            t * length,
            sin(t * PI) * curve,
            0.0f
        ));
    }

    std::vector<glm::vec3> tangents;
    for (int i = 0; i <= PATH_STEPS; i++) {
        glm::vec3 tang;
        if      (i == 0)          tang = glm::normalize(path[1]-path[0]);
        else if (i == PATH_STEPS) tang = glm::normalize(path[i]-path[i-1]);
        else                      tang = glm::normalize(path[i+1]-path[i-1]);
        tangents.push_back(tang);
    }

    for (int i = 0; i <= PATH_STEPS; i++) {
        glm::vec3 tang  = tangents[i];
        glm::vec3 wUp   = glm::vec3(0,1,0);
        glm::vec3 right = glm::normalize(glm::cross(tang, wUp));
        glm::vec3 up    = glm::normalize(glm::cross(right, tang));

        float t      = (float)i / PATH_STEPS;
        float taper  = 1.0f - t * 0.7f;
        float radius = thickness * taper;

        for (int s = 0; s < RING_SEGS; s++) {
            float angle  = (float)s / RING_SEGS * TWO_PI;
            glm::vec3 offset = (right*(float)cos(angle) +
                                up   *(float)sin(angle)) * radius;
            Vertex v;
            v.Position = path[i] + offset;
            v.Normal   = glm::normalize(offset);
            v.TexCoord = glm::vec2((float)s/RING_SEGS, t);
            verts.push_back(v);
        }
    }

    for (int i = 0; i < PATH_STEPS; i++) {
        for (int s = 0; s < RING_SEGS; s++) {
            int curr=i*RING_SEGS+s, next=i*RING_SEGS+(s+1)%RING_SEGS;
            int below=(i+1)*RING_SEGS+s, belowNext=(i+1)*RING_SEGS+(s+1)%RING_SEGS;
            indices.push_back(curr); indices.push_back(below); indices.push_back(next);
            indices.push_back(next); indices.push_back(below); indices.push_back(belowNext);
        }
    }

    boneMeshes.push_back(new Mesh(verts, indices));
    boneTransforms.push_back(glm::mat4(1.0f));
}

void Bones::buildSpineSegment(float radius, float height) {
    std::vector<Vertex>       verts;
    std::vector<unsigned int> indices;

    int   RINGS  = 6;
    int   SEGS   = 10;
    float TWO_PI = 2.0f * 3.14159265f;

    for (int r = 0; r <= RINGS; r++) {
        float t    = (float)r / RINGS;
        float y    = height * t - height * 0.5f;
        // Sine bulge gives each vertebra its characteristic shape
        float bulge = 1.0f + sin(t * 3.14159f) * 0.3f;
        float rad  = radius * bulge;

        for (int s = 0; s < SEGS; s++) {
            float angle = (float)s / SEGS * TWO_PI;
            glm::vec3 pos(cos(angle)*rad, y, sin(angle)*rad);
            glm::vec3 normal = glm::normalize(
                glm::vec3(cos(angle), 0.0f, sin(angle)));
            verts.push_back({ pos, normal,
                glm::vec2((float)s/SEGS, t) });
        }
    }

    for (int r = 0; r < RINGS; r++) {
        for (int s = 0; s < SEGS; s++) {
            int curr=r*SEGS+s, next=r*SEGS+(s+1)%SEGS;
            int below=(r+1)*SEGS+s, belowNext=(r+1)*SEGS+(s+1)%SEGS;
            indices.push_back(curr); indices.push_back(below); indices.push_back(next);
            indices.push_back(next); indices.push_back(below); indices.push_back(belowNext);
        }
    }

    boneMeshes.push_back(new Mesh(verts, indices));
    boneTransforms.push_back(glm::mat4(1.0f));
}

void Bones::assembleSkeleton() {

    // Cranium
    buildEllipsoid(1.6f, 1.1f, 1.8f, 14, 16, 1.0f);
    {
        glm::mat4 m = glm::mat4(1.0f);
        m = glm::translate(m, origin + glm::vec3(0.0f, 1.2f, 1.0f));
        m = glm::rotate(m, glm::radians(10.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        boneTransforms.back() = m;
    }

    // Snout, shorter and flatter than cranium
    buildEllipsoid(1.2f, 0.7f, 1.4f, 10, 14, 0.8f);
    {
        glm::mat4 m = glm::mat4(1.0f);
        m = glm::translate(m, origin + glm::vec3(0.0f, 0.7f, 2.8f));
        m = glm::rotate(m, glm::radians(-8.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        boneTransforms.back() = m;
    }

    // Lower jaw, rotated open slightly
    buildEllipsoid(1.0f, 0.45f, 1.5f, 8, 14, 0.7f);
    {
        glm::mat4 m = glm::mat4(1.0f);
        m = glm::translate(m, origin + glm::vec3(0.0f, 0.1f, 2.6f));
        m = glm::rotate(m, glm::radians(18.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        boneTransforms.back() = m;
    }

    // Cranial nubs (left and right)
    buildEllipsoid(0.28f, 0.45f, 0.28f, 6, 8, 0.5f);
    {
        glm::mat4 m = glm::mat4(1.0f);
        m = glm::translate(m, origin + glm::vec3( 0.7f, 2.1f, 0.3f));
        boneTransforms.back() = m;
    }
    buildEllipsoid(0.28f, 0.45f, 0.28f, 6, 8, 0.5f);
    {
        glm::mat4 m = glm::mat4(1.0f);
        m = glm::translate(m, origin + glm::vec3(-0.7f, 2.1f, 0.3f));
        boneTransforms.back() = m;
    }

    // Upper fangs pointing down
    buildTooth(0.50f, 0.12f, 0.18f);
    {
        glm::mat4 m = glm::mat4(1.0f);
        m = glm::translate(m, origin + glm::vec3(-0.35f, 0.35f, 3.8f));
        m = glm::rotate(m, glm::radians(175.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        boneTransforms.back() = m;
    }
    buildTooth(0.50f, 0.12f, 0.18f);
    {
        glm::mat4 m = glm::mat4(1.0f);
        m = glm::translate(m, origin + glm::vec3( 0.35f, 0.35f, 3.8f));
        m = glm::rotate(m, glm::radians(175.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        boneTransforms.back() = m;
    }

    // Lower fangs pointing up
    buildTooth(0.45f, 0.10f, 0.15f);
    {
        glm::mat4 m = glm::mat4(1.0f);
        m = glm::translate(m, origin + glm::vec3(-0.25f, -0.05f, 3.9f));
        boneTransforms.back() = m;
    }
    buildTooth(0.45f, 0.10f, 0.15f);
    {
        glm::mat4 m = glm::mat4(1.0f);
        m = glm::translate(m, origin + glm::vec3( 0.25f, -0.05f, 3.9f));
        boneTransforms.back() = m;
    }

    // Standard teeth rows, shrink toward the back
    int   toothCount   = 8;
    float jawLength    = 2.2f;
    float startZ       = 3.5f;
    float toothSpacing = jawLength / (toothCount - 1);

    for (int i = 0; i < toothCount; i++) {
        float t        = (float)i / (toothCount - 1);
        float z        = startZ - i * toothSpacing;
        float scale    = 1.0f - t * 0.6f;
        float h        = 0.18f * scale;
        float r        = 0.045f * scale;
        float crv      = 0.05f * scale;
        float yUp      = 0.28f - t * 0.05f;

        for (int side = -1; side <= 1; side += 2) {
            float x = side * (0.55f + t * 0.1f);

            // Upper tooth pointing down
            buildTooth(h, r, crv);
            {
                glm::mat4 m = glm::mat4(1.0f);
                m = glm::translate(m, origin + glm::vec3(x, yUp, z));
                m = glm::rotate(m, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                boneTransforms.back() = m;
            }

            // Lower tooth offset by half spacing to interlock with upper
            float zOffset = toothSpacing * 0.5f;
            buildTooth(h * 0.85f, r * 0.9f, crv);
            {
                glm::mat4 m = glm::mat4(1.0f);
                m = glm::translate(m, origin +
                    glm::vec3(x, -0.08f - t * 0.04f, z - zOffset));
                boneTransforms.back() = m;
            }
        }
    }

    // Spine vertebrae
    int   spineCount   = 7;
    float spineSpacing = 1.4f;
    for (int i = 0; i < spineCount; i++) {
        buildSpineSegment(0.35f, 0.5f);
        glm::mat4 m = glm::mat4(1.0f);
        m = glm::translate(m, origin +
            glm::vec3(0.0f, 0.2f, -(float)i * spineSpacing));
        boneTransforms.back() = m;
    }

    // Rib pairs, length and curve reduce toward the tail
    int ribPairs = 6;
    for (int i = 0; i < ribPairs; i++) {
        float ribZ      = -(float)i * spineSpacing;
        float ribLength = 2.8f - i * 0.25f;
        float ribCurve  = 1.0f - i * 0.1f;

        buildRib(ribLength, 0.12f, ribCurve);
        {
            glm::mat4 m = glm::mat4(1.0f);
            m = glm::translate(m, origin + glm::vec3(0.35f, 0.2f, ribZ));
            m = glm::rotate(m, glm::radians(80.0f),  glm::vec3(0.0f, 0.0f, 1.0f));
            m = glm::rotate(m, glm::radians(-15.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            boneTransforms.back() = m;
        }

        buildRib(ribLength, 0.12f, ribCurve);
        {
            glm::mat4 m = glm::mat4(1.0f);
            m = glm::translate(m, origin + glm::vec3(-0.35f, 0.2f, ribZ));
            m = glm::rotate(m, glm::radians(-80.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            m = glm::rotate(m, glm::radians(15.0f),  glm::vec3(0.0f, 1.0f, 0.0f));
            boneTransforms.back() = m;
        }
    }
}

Bones::Bones(glm::vec3 org) : origin(org) {
    assembleSkeleton();
}

Bones::~Bones() {
    for (auto* m : boneMeshes) { m->cleanup(); delete m; }
}

void Bones::draw(Shader& shader,
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
    shader.setBool ("isBone",           true);
    shader.setBool ("useProceduralTexture", false);
    shader.setFloat("emissiveStrength", 0.0f);
    shader.setFloat("specularStrength", 0.35f);
    shader.setFloat("shininess",        6.0f);
    shader.setVec3 ("objectColor",      glm::vec3(0.78f, 0.68f, 0.48f));

    for (int i = 0; i < (int)boneMeshes.size(); i++) {
        shader.setMat4("model", boneTransforms[i]);
        boneMeshes[i]->draw();
    }

    shader.setBool("isBone", false);
}