#include "ObjModel.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <algorithm>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <glm/gtc/matrix_transform.hpp>

ObjModel::ObjModel(const std::string& objPath,
                   const std::string& textureDir) {
    load(objPath, textureDir);
}

ObjModel::~ObjModel() {
    for (auto& m : meshes) {
        glDeleteVertexArrays(1, &m.VAO);
        glDeleteBuffers(1, &m.VBO);
        glDeleteBuffers(1, &m.EBO);
        glDeleteTextures(1, &m.textureID);
    }
}

void ObjModel::load(const std::string& objPath,
                    const std::string& textureDir)
{
    // Raw OBJ data
    std::vector<glm::vec3> rawPos;
    std::vector<glm::vec3> rawNorm;
    std::vector<glm::vec2> rawTex;

    // MTL material → texture mapping
    // We only care about map_Kd (diffuse/baseColor)
    std::map<std::string, std::string> matTextures;

    // Parse MTL file referenced in OBJ
    auto parseMTL = [&](const std::string& mtlPath) {
        std::ifstream f(mtlPath);
        if (!f.is_open()) {
            std::cerr << "Could not open MTL: " << mtlPath << "\n";
            return;
        }
        std::string line, currentMat;
        while (std::getline(f, line)) {
            std::istringstream ss(line);
            std::string token;
            ss >> token;
            if (token == "newmtl") {
                ss >> currentMat;
            } else if (token == "map_Kd" && !currentMat.empty()) {
                std::string texFile;
                ss >> texFile;
                // Strip any directory prefix from the texture filename
                size_t slash = texFile.find_last_of("/\\");
                if (slash != std::string::npos)
                    texFile = texFile.substr(slash + 1);
                matTextures[currentMat] = textureDir + "/" + texFile;
            }
        }
    };

    // Current mesh being built
    std::string      currentMat;
    std::vector<ObjVertex>      curVerts;
    std::vector<unsigned int>   curIdx;
    std::map<std::string, unsigned int> indexCache;

    auto flushMesh = [&]() {
        if (curVerts.empty()) return;
        ObjMesh m;
        m.vertices     = curVerts;
        m.indices      = curIdx;
        m.materialName = currentMat;

        // Assign texture
        auto it = matTextures.find(currentMat);
        if (it != matTextures.end())
            m.textureID = loadTexture(it->second);
        else
            m.textureID = 0;

        setupMesh(m);
        meshes.push_back(m);
        curVerts.clear();
        curIdx.clear();
        indexCache.clear();
    };

    std::ifstream file(objPath);
    if (!file.is_open()) {
        std::cerr << "Could not open OBJ: " << objPath << "\n";
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string token;
        ss >> token;

        if (token == "mtllib") {
            std::string mtlFile;
            ss >> mtlFile;
            // Look for MTL in same directory as OBJ
            size_t slash = objPath.find_last_of("/\\");
            std::string dir = (slash != std::string::npos)
                            ? objPath.substr(0, slash + 1) : "";
            parseMTL(dir + mtlFile);

        } else if (token == "v") {
            glm::vec3 p;
            ss >> p.x >> p.y >> p.z;
            rawPos.push_back(p);

        } else if (token == "vn") {
            glm::vec3 n;
            ss >> n.x >> n.y >> n.z;
            rawNorm.push_back(n);

        } else if (token == "vt") {
            glm::vec2 t;
            ss >> t.x >> t.y;
            rawTex.push_back(t);

        } else if (token == "usemtl") {
            std::string newMat;
            ss >> newMat;
            if (newMat != currentMat) {
                flushMesh();
                currentMat = newMat;
            }

        } else if (token == "f") {
            // Face — may be triangle or quad, handles v/vt/vn format
            std::vector<std::string> faceTokens;
            std::string ft;
            while (ss >> ft) faceTokens.push_back(ft);

            // Triangulate (fan from first vertex)
            for (int i = 1; i + 1 < (int)faceTokens.size(); i++) {
                std::string corners[3] = {
                    faceTokens[0], faceTokens[i], faceTokens[i+1]
                };
                for (auto& c : corners) {
                    // Check cache to reuse vertices
                    auto cached = indexCache.find(c);
                    if (cached != indexCache.end()) {
                        curIdx.push_back(cached->second);
                        continue;
                    }

                    // Parse v/vt/vn — all combinations
                    int vi = 0, ti = 0, ni = 0;
                    std::replace(c.begin(), c.end(), '/', ' ');
                    std::istringstream cs(c);
                    std::string a, b, d;
                    cs >> a;
                    vi = std::stoi(a) - 1;
                    if (cs >> b && !b.empty()) ti = std::stoi(b) - 1;
                    if (cs >> d && !d.empty()) ni = std::stoi(d) - 1;

                    ObjVertex vert;
                    vert.Position = (vi >= 0 && vi < (int)rawPos.size())
                                  ? rawPos[vi] : glm::vec3(0);
                    vert.Normal   = (ni >= 0 && ni < (int)rawNorm.size())
                                  ? rawNorm[ni] : glm::vec3(0,1,0);
                    vert.TexCoord = (ti >= 0 && ti < (int)rawTex.size())
                                  ? rawTex[ti]  : glm::vec2(0);

                    unsigned int idx = (unsigned int)curVerts.size();
                    curVerts.push_back(vert);
                    curIdx.push_back(idx);
                    indexCache[c] = idx;
                }
            }
        }
    }
    flushMesh(); // flush final mesh
}

void ObjModel::setupMesh(ObjMesh& m) {
    glGenVertexArrays(1, &m.VAO);
    glGenBuffers(1, &m.VBO);
    glGenBuffers(1, &m.EBO);

    glBindVertexArray(m.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, m.VBO);
    glBufferData(GL_ARRAY_BUFFER,
        m.vertices.size() * sizeof(ObjVertex),
        m.vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        m.indices.size() * sizeof(unsigned int),
        m.indices.data(), GL_STATIC_DRAW);

    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ObjVertex),
        (void*)offsetof(ObjVertex, Position));
    glEnableVertexAttribArray(0);
    // Normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ObjVertex),
        (void*)offsetof(ObjVertex, Normal));
    glEnableVertexAttribArray(1);
    // TexCoord
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(ObjVertex),
        (void*)offsetof(ObjVertex, TexCoord));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

unsigned int ObjModel::loadTexture(const std::string& path) {
    unsigned int id;
    glGenTextures(1, &id);

    int w, h, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, 0);
    if (data) {
        GLenum fmt = (channels == 4) ? GL_RGBA : GL_RGB;
        glBindTexture(GL_TEXTURE_2D, id);
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt,
                     GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                        GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
    } else {
        std::cerr << "Failed to load texture: " << path << "\n";
        stbi_image_free(data);
    }
    return id;
}

void ObjModel::draw(Shader& shader,
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
                    const std::vector<float>&     lightIntensities)
{
    shader.use();
    shader.setMat4("view",        view);
    shader.setMat4("projection",  projection);
    shader.setVec3("viewPos",     viewPos);
    shader.setVec3("fogColor",    fogColor);
    shader.setFloat("fogDensity", fogDensity);
    shader.setBool("isEmissive",  false);
    shader.setBool("useProceduralTexture", false);
    shader.setBool("isBone",      false);
    shader.setFloat("specularStrength", 0.3f);
    shader.setFloat("shininess",        16.0f);

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

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(rotY),
                        glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(scale));
    shader.setMat4("model", model);

    for (auto& m : meshes) {
        // Set object color from texture or fallback
        if (m.textureID != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m.textureID);
            shader.setInt("textureSampler", 0);
            shader.setBool("useTexture", true);
        } else {
            shader.setBool("useTexture", false);
            shader.setVec3("objectColor", glm::vec3(0.6f, 0.4f, 0.2f));
        }

        glBindVertexArray(m.VAO);
        glDrawElements(GL_TRIANGLES, (int)m.indices.size(),
                       GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
}