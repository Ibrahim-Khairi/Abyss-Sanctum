#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

// A Vertex holds all data for one point in 3D space.
// Keeping it in a struct means we can pass vectors of them cleanly.
struct Vertex {
    glm::vec3 Position;  // x, y, z
    glm::vec3 Normal;    // surface normal for lighting
    glm::vec2 TexCoord;  // texture UV coordinates (for later)
};

class Mesh {
public:
    // The actual data
    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;  // which vertices form each triangle

    // GPU object IDs
    unsigned int VAO, VBO, EBO;
    // EBO = Element Buffer Object
    // Stores the indices so GPU knows which vertices connect
    // into triangles. Avoids duplicating shared vertices.

    // Constructor — takes your vertex and index data,
    // uploads everything to the GPU immediately
    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices);

    // Draw this mesh — call inside render loop
    void draw();

    // Free GPU memory when done
    void cleanup();

private:
    void setupMesh(); // uploads data to GPU
};

#endif