#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>
#include "Shader.h"
#include "Camera.h"
#include "Mesh.h"
#include "CaveEnvironment.h"
#include "CrystalCluster.h"
#include "RockFormation.h"
#include "CoralFormation.h"
#include "Bones.h"
#include "ParticleSystem.h"
#include "ObjModel.h"

const unsigned int SCR_WIDTH  = 1280;
const unsigned int SCR_HEIGHT = 720;

Camera camera(glm::vec3(0.0f, 0.0f, 8.0f));
float lastX = SCR_WIDTH / 2.0f, lastY = SCR_HEIGHT / 2.0f;
bool  firstMouse = true;
float deltaTime = 0.0f, lastFrame = 0.0f;

void framebuffer_size_callback(GLFWwindow* w, int width, int height) {
    glViewport(0, 0, width, height); }
void mouse_callback(GLFWwindow* w, double xIn, double yIn) {
    float x = (float)xIn, y = (float)yIn;
    if (firstMouse) { lastX = x; lastY = y; firstMouse = false; }
    camera.processMouseMovement(x - lastX, lastY - y);
    lastX = x; lastY = y; }
void scroll_callback(GLFWwindow* w, double xO, double yO) {
    camera.processMouseScroll((float)yO); }
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.processKeyboard(FORWARD,  deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.processKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.processKeyboard(LEFT,     deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.processKeyboard(RIGHT,    deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.processKeyboard(UP,       deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        camera.processKeyboard(DOWN,     deltaTime); }

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        SCR_WIDTH, SCR_HEIGHT, "AbyssSanctum", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window,    scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;
    glEnable(GL_DEPTH_TEST);

    Shader caveShader("shaders/cave.vert", "shaders/cave.frag");
    Shader particleShader("shaders/particle.vert", "shaders/particle.frag");

    CaveEnvironment cave;
    RockFormation rocks;
    CoralFormation coral(&cave);
    Bones skeleton(glm::vec3(1.5f, -7.8f, -15.0f));
    ParticleSystem particles(200, 300);
    ObjModel chest("assets/models/old_chest.glb.obj", "assets/models");

    glm::vec3 manaBlue  = glm::vec3(0.3f, 0.6f, 1.0f);
    glm::vec3 manaCyan  = glm::vec3(0.2f, 0.8f, 1.0f);
    glm::vec3 manaDeep  = glm::vec3(0.4f, 0.5f, 1.0f);

    std::vector<CrystalCluster*> clusters;

    // ── TUNNEL crystals — flush against walls ────────────────────────
    // Left wall (X = -2.0, normal points right)
    clusters.push_back(new CrystalCluster(
        glm::vec3(-2.0f, -0.5f, 15.0f),
        glm::vec3( 1.0f,  0.0f,  0.0f),
        manaCyan, 3.5f, 1.2f, 0.0f, 5));

    clusters.push_back(new CrystalCluster(
        glm::vec3(-2.0f,  0.3f,  7.0f),
        glm::vec3( 1.0f,  0.0f,  0.0f),
        manaBlue, 3.5f, 1.0f, 1.4f, 4));

    // Right wall (X = +2.0, normal points left)
    clusters.push_back(new CrystalCluster(
        glm::vec3( 2.0f, -0.3f, 11.0f),
        glm::vec3(-1.0f,  0.0f,  0.0f),
        manaDeep, 3.5f, 0.9f, 2.1f, 4));

    clusters.push_back(new CrystalCluster(
        glm::vec3( 2.0f,  0.5f,  4.0f),
        glm::vec3(-1.0f,  0.0f,  0.0f),
        manaCyan, 3.5f, 1.1f, 0.8f, 5));

    // Tunnel floor (Y = -2.0, normal points up)
    clusters.push_back(new CrystalCluster(
        glm::vec3(-0.5f, -2.0f, 13.0f),
        glm::vec3( 0.0f,  1.0f,  0.0f),
        manaBlue, 3.5f, 0.8f, 1.6f, 3));

    clusters.push_back(new CrystalCluster(
        glm::vec3( 0.6f, -2.0f,  6.0f),
        glm::vec3( 0.0f,  1.0f,  0.0f),
        manaCyan, 3.5f, 1.0f, 2.4f, 3));

    clusters.push_back(new CrystalCluster(
        glm::vec3( 0.6f, -2.0f,  20.0f),
        glm::vec3( 0.0f,  1.0f,  0.0f),
        manaCyan, 3.5f, 1.0f, 2.4f, 3));

    // ── CHAMBER crystals — flush against walls ───────────────────────

    // Left wall (X ≈ -9.0, normal points right)
    clusters.push_back(new CrystalCluster(
        glm::vec3(-9.0f, -2.0f,  -6.0f),
        glm::vec3( 1.0f,  0.0f,   0.0f),
        manaBlue, 14.0f, 1.2f, 0.3f, 6));

    clusters.push_back(new CrystalCluster(
        glm::vec3(-9.0f,  3.0f, -12.0f),
        glm::vec3( 1.0f,  0.0f,   0.0f),
        manaCyan, 14.0f, 0.9f, 1.2f, 7));

    clusters.push_back(new CrystalCluster(
        glm::vec3(-9.0f, -1.0f, -19.0f),
        glm::vec3( 1.0f,  0.0f,   0.0f),
        manaDeep, 14.0f, 1.4f, 2.5f, 5));

    // Right wall (X ≈ +9.0, normal points left)
    clusters.push_back(new CrystalCluster(
        glm::vec3( 9.0f, -1.5f,  -8.0f),
        glm::vec3(-1.0f,  0.0f,   0.0f),
        manaCyan, 14.0f, 1.1f, 0.7f, 6));

    clusters.push_back(new CrystalCluster(
        glm::vec3( 9.0f,  2.5f, -15.0f),
        glm::vec3(-1.0f,  0.0f,   0.0f),
        manaBlue, 14.0f, 0.8f, 1.8f, 7));

    clusters.push_back(new CrystalCluster(
        glm::vec3( 9.0f, -3.0f, -22.0f),
        glm::vec3(-1.0f,  0.0f,   0.0f),
        manaDeep, 14.0f, 1.3f, 3.1f, 5));

    // Chamber floor (Y = -8.0, normal points up)
    clusters.push_back(new CrystalCluster(
        glm::vec3(-4.0f, -8.0f,  -5.0f),
        glm::vec3( 0.0f,  1.0f,   0.0f),
        manaBlue, 15.0f, 1.3f, 3.0f, 5));

    clusters.push_back(new CrystalCluster(
        glm::vec3( 4.0f, -8.0f,  -9.0f),
        glm::vec3( 0.0f,  1.0f,   0.0f),
        manaCyan, 15.0f, 1.0f, 4.2f, 5));

    clusters.push_back(new CrystalCluster(
        glm::vec3(-6.0f, -8.0f, -14.0f),
        glm::vec3( 0.0f,  1.0f,   0.0f),
        manaDeep, 15.0f, 0.9f, 1.5f, 4));

    clusters.push_back(new CrystalCluster(
        glm::vec3( 5.5f, -8.0f, -20.0f),
        glm::vec3( 0.0f,  1.0f,   0.0f),
        manaBlue, 15.0f, 1.1f, 2.8f, 5));

    // Back wall (Z ≈ -26.0, normal points toward camera)
    clusters.push_back(new CrystalCluster(
        glm::vec3( 0.0f,  0.0f, -26.0f),
        glm::vec3( 0.0f,  0.0f,  1.0f),
        manaBlue, 10.0f, 0.7f, 0.5f, 9));

    clusters.push_back(new CrystalCluster(
        glm::vec3(-3.5f,  2.0f, -26.0f),
        glm::vec3( 0.0f,  0.0f,  1.0f),
        manaCyan, 10.0f, 0.8f, 1.3f, 6));

    // Ceiling (Y ≈ +9.0, normal points down)

    clusters.push_back(new CrystalCluster(
        glm::vec3(-3.0f,  12.6f, -16.0f),
        glm::vec3( 0.0f, -1.0f,   0.0f),
        manaBlue, 13.0f, 1.0f, 2.9f, 5));

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        camera.confineToCave();

        glClearColor(0.01f, 0.02f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view       = camera.getViewMatrix();
        glm::mat4 projection = glm::perspective(
            glm::radians(camera.Fov),
            (float)SCR_WIDTH / (float)SCR_HEIGHT,
            0.1f, 200.0f);

        std::vector<glm::vec3> lightPositions;
        std::vector<glm::vec3> lightColors;
        std::vector<float>     lightIntensities;

        for (auto* cluster : clusters) {
            lightPositions.push_back(cluster->getLightPosition());
            lightColors.push_back(cluster->getAnimatedColor(currentFrame));
            lightIntensities.push_back(
                cluster->getAnimatedIntensity(currentFrame));
            if (lightPositions.size() >= 12) break;
        }

        caveShader.use();
        caveShader.setFloat("time", currentFrame);

        int numLights = (int)lightPositions.size();
        caveShader.setInt("numLights", numLights);
        for (int i = 0; i < numLights; i++) {
            caveShader.setVec3("lightPositions[" + std::to_string(i) + "]",
                               lightPositions[i]);
            caveShader.setVec3("lightColors[" + std::to_string(i) + "]",
                               lightColors[i]);
            caveShader.setFloat("lightIntensities[" + std::to_string(i) + "]",
                                lightIntensities[i]);
        }

        cave.draw(caveShader, view, projection, camera.Position);

        rocks.draw(caveShader, view, projection, camera.Position,
                   cave.fogColor, cave.fogDensity,
                   lightPositions, lightColors, lightIntensities);

        coral.draw(caveShader, view, projection, camera.Position,
                   cave.fogColor, cave.fogDensity,
                   lightPositions, lightColors, lightIntensities);

        skeleton.draw(caveShader, view, projection, camera.Position,
                      cave.fogColor, cave.fogDensity,
                      lightPositions, lightColors, lightIntensities);

        chest.draw(caveShader, view, projection, camera.Position,
           glm::vec3(-2.2f, -2.5f, -17.1f),  // position in chamber
           3.0f,                              // scale — adjust as needed
           45.0f,                              // rotation Y
           cave.fogColor, cave.fogDensity,
           lightPositions, lightColors, lightIntensities);

        for (auto* cluster : clusters) {
            cluster->draw(caveShader, view, projection,
                          camera.Position,
                          cave.fogColor, cave.fogDensity,
                          currentFrame,
                          lightPositions, lightColors, lightIntensities);
        }

        particles.update(deltaTime, camera.Position);
        particles.draw(particleShader, view, projection,
                       camera.Position,
                       cave.fogColor, cave.fogDensity);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    for (auto* c : clusters) delete c;
    glDeleteProgram(caveShader.ID);
    glDeleteProgram(particleShader.ID);
    glfwTerminate();
    return 0;
}