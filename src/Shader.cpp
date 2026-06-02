#include "Shader.h"

// ─────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────
Shader::Shader(const char* vertexPath, const char* fragmentPath) {

    // ── STEP 1: Read source code from files ──────────────────────────
    // We use C++ file streams to read the .vert and .frag text files
    // into strings. The GPU needs the raw source code as a string.

    std::string   vertexCode;
    std::string   fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;

    // Tell the file streams to throw exceptions on failure
    // so we can catch them and print a useful error
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);

        // Read the whole file into a string stream, then convert to string
        std::stringstream vStream, fStream;
        vStream << vShaderFile.rdbuf();
        fStream << fShaderFile.rdbuf();

        vShaderFile.close();
        fShaderFile.close();

        vertexCode   = vStream.str();
        fragmentCode = fStream.str();
    }
    catch (std::ifstream::failure& e) {
        std::cerr << "[Shader] ERROR: Could not read shader file.\n"
                  << "  Vertex:   " << vertexPath   << "\n"
                  << "  Fragment: " << fragmentPath << "\n"
                  << "  Make sure these paths are correct relative to where"
                  << " you run the .exe from.\n";
    }

    // Convert std::string to const char* because OpenGL needs C-style strings
    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();


    // ── STEP 2: Compile shaders on the GPU ───────────────────────────
    // glCreateShader tells the GPU "I'm about to give you a shader"
    // It returns an ID (just an unsigned int) so we can refer to it

    unsigned int vertex, fragment;

    // Vertex shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    // glShaderSource says "here is the source code for this shader"
    // The '1' means we're passing 1 string. NULL means the string
    // is null-terminated (normal C string)

    glCompileShader(vertex);
    checkCompileErrors(vertex, "VERTEX"); // did it work?

    // Fragment shader — same process
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    checkCompileErrors(fragment, "FRAGMENT");


    // ── STEP 3: Link into a shader program ───────────────────────────
    // A shader "program" is the vertex + fragment shader combined.
    // OpenGL needs them linked together before it can use them.
    // This is like linking .o files into an executable in normal C++.

    ID = glCreateProgram();         // create the program, get its ID
    glAttachShader(ID, vertex);     // attach compiled vertex shader
    glAttachShader(ID, fragment);   // attach compiled fragment shader
    glLinkProgram(ID);              // link them together
    checkCompileErrors(ID, "PROGRAM");

    // The individual vertex/fragment shader objects are no longer needed
    // now that they're linked into the program. Free GPU memory.
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}


// ─────────────────────────────────────────────
// use()
// Tells OpenGL: "use this shader program for all
// subsequent draw calls until told otherwise"
// ─────────────────────────────────────────────
void Shader::use() {
    glUseProgram(ID);
}


// ─────────────────────────────────────────────
// Uniform setters
// glGetUniformLocation finds the variable by name
// inside the shader program, then we set its value.
// Think of it like: find the variable, then assign to it.
// ─────────────────────────────────────────────

void Shader::setBool(const std::string& name, bool value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}

void Shader::setInt(const std::string& name, int value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setFloat(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

// Two versions of setVec3:
// one where you pass x,y,z separately
// one where you pass a glm::vec3 directly
void Shader::setVec3(const std::string& name, float x, float y, float z) const {
    glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
}

void Shader::setVec3(const std::string& name, const glm::vec3& value) const {
    glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(value));
}

// mat4 is a 4x4 matrix — used for transformations, view, projection
// glm::value_ptr converts it to a raw float array OpenGL understands
void Shader::setMat4(const std::string& name, const glm::mat4& matrix) const {
    glUniformMatrix4fv(
        glGetUniformLocation(ID, name.c_str()),
        1,        // number of matrices
        GL_FALSE, // transpose? No — GLM is already column-major like OpenGL wants
        glm::value_ptr(matrix)
    );
}


// ─────────────────────────────────────────────
// checkCompileErrors()
// Called after compiling/linking to see if it worked.
// If not, prints the error log from the GPU.
// ─────────────────────────────────────────────
void Shader::checkCompileErrors(unsigned int shader, const std::string& type) {
    int  success;
    char infoLog[1024];

    if (type != "PROGRAM") {
        // Checking a shader compile
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "[Shader] COMPILE ERROR (" << type << "):\n"
                      << infoLog << "\n";
        }
    } else {
        // Checking a program link
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "[Shader] LINK ERROR:\n" << infoLog << "\n";
        }
    }
}