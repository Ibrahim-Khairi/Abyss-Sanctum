#include "Mesh.h"
#include <iostream>

// ─────────────────────────────────────────────
// Constructor
// Stores the data and immediately sends it to GPU
// ─────────────────────────────────────────────
Mesh::Mesh(std::vector<Vertex> verts, std::vector<unsigned int> inds) {
    vertices = verts;
    indices  = inds;
    setupMesh();
}

// ─────────────────────────────────────────────
// setupMesh
// This is the same VAO/VBO setup from main.cpp
// but now it works for ANY mesh data you give it.
//
// We also introduce the EBO here.
// Without EBO: a quad needs 6 vertices (2 triangles × 3)
//              even though a quad only has 4 unique corners
// With EBO:    store 4 vertices, then indices [0,1,2, 0,2,3]
//              to say "triangle 1 uses corners 0,1,2
//                       triangle 2 uses corners 0,2,3"
// Saves memory and becomes important with complex meshes.
// ─────────────────────────────────────────────
void Mesh::setupMesh() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO); // start recording format

    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
        vertices.size() * sizeof(Vertex), // total bytes
        vertices.data(),                   // pointer to first element
        GL_STATIC_DRAW);

    // Upload index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(unsigned int),
        indices.data(),
        GL_STATIC_DRAW);

    // Tell OpenGL the layout of a Vertex struct:
    // offsetof(Vertex, Position) = byte offset of Position inside struct
    // This works even if the struct layout changes — robust.

    // Attribute 0: Position (vec3, offset 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)offsetof(Vertex, Position));
    glEnableVertexAttribArray(0);

    // Attribute 1: Normal (vec3, after Position)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)offsetof(Vertex, Normal));
    glEnableVertexAttribArray(1);

    // Attribute 2: TexCoord (vec2, after Normal)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)offsetof(Vertex, TexCoord));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0); // stop recording
}

// ─────────────────────────────────────────────
// draw
// Binds the VAO and issues the draw call.
// Uses glDrawElements because we have an EBO (indices).
// ─────────────────────────────────────────────
void Mesh::draw() {
    glBindVertexArray(VAO);
    glDrawElements(
        GL_TRIANGLES,          // draw mode
        (int)indices.size(),   // how many indices
        GL_UNSIGNED_INT,       // type of each index
        0                      // offset into EBO
    );
    glBindVertexArray(0);
}

// ─────────────────────────────────────────────
// cleanup
// Delete GPU buffers when we're done.
// ─────────────────────────────────────────────
void Mesh::cleanup() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}