#ifndef CAMERA_H
#define CAMERA_H
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

const float YAW         = -90.0f;
const float PITCH        =   0.0f;
const float SPEED        =   3.0f;
const float SENSITIVITY  =   0.1f;
const float FOV          =  45.0f;

class Camera {
public:
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    float Yaw;
    float Pitch;
    float MovementSpeed;
    float MouseSensitivity;
    float Fov;

    Camera(glm::vec3 startPosition = glm::vec3(0.0f, 0.0f, 3.0f));

    glm::mat4 getViewMatrix();

    // deltaTime keeps movement speed frame-rate independent
    void processKeyboard(Camera_Movement direction, float deltaTime);
    void processMouseMovement(float xOffset, float yOffset,
                               bool constrainPitch = true);
    void processMouseScroll(float yOffset);
    void confineToTube(float z, float tubeRadius);
    void confineToCave();

private:
    // Recalculates Front, Right, Up from current Yaw and Pitch
    void updateCameraVectors();
};
#endif