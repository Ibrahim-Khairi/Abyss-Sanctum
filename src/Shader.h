#ifndef SHADER_H
#define SHADER_H

// glad must come before any OpenGL header
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader {
public:
    // The ID of the compiled+linked shader program on the GPU
    // Every OpenGL object is just an unsigned int ID — the actual
    // data lives on the GPU, we just hold a reference to it
    unsigned int ID;

    // Constructor — takes paths to your .vert and .frag files
    // Does everything: reads, compiles, links
    Shader(const char* vertexPath, const char* fragmentPath);

    // Call this before drawing anything with this shader
    void use();

    // These send data from CPU → GPU (uniforms)
    void setBool (const std::string& name, bool value)              const;
    void setInt  (const std::string& name, int value)               const;
    void setFloat(const std::string& name, float value)             const;
    void setVec3 (const std::string& name, float x, float y, float z) const;
    void setVec3 (const std::string& name, const glm::vec3& value)  const;
    void setMat4 (const std::string& name, const glm::mat4& matrix) const;

private:
    // Helper — checks if compile/link succeeded and prints errors
    void checkCompileErrors(unsigned int shader, const std::string& type);
};

#endif