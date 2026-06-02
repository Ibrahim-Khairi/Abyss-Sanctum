#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

Camera::Camera(glm::vec3 startPosition) {
    Position         = startPosition;
    WorldUp          = glm::vec3(0.0f, 1.0f, 0.0f);
    Yaw              = YAW;
    Pitch            = PITCH;
    MovementSpeed    = SPEED;
    MouseSensitivity = SENSITIVITY;
    Fov              = FOV;
    updateCameraVectors();
}

glm::mat4 Camera::getViewMatrix() {
    return glm::lookAt(
        Position,
        Position + Front,
        Up
    );
}

void Camera::processKeyboard(Camera_Movement direction, float deltaTime) {
    // Multiply by deltaTime for frame-rate independent movement
    float velocity = MovementSpeed * deltaTime;

    if (direction == FORWARD)  Position += Front * velocity;
    if (direction == BACKWARD) Position -= Front * velocity;
    if (direction == LEFT)     Position -= Right * velocity;
    if (direction == RIGHT)    Position += Right * velocity;
    if (direction == UP)       Position += Up    * velocity;
    if (direction == DOWN)     Position -= Up    * velocity;
}

void Camera::processMouseMovement(float xOffset, float yOffset,
                                   bool constrainPitch) {
    xOffset *= MouseSensitivity;
    yOffset *= MouseSensitivity;

    Yaw   += xOffset;
    Pitch += yOffset;

    // Clamp pitch so the camera cant flip upside down
    if (constrainPitch)
        Pitch = std::clamp(Pitch, -89.0f, 89.0f);

    updateCameraVectors();
}

// Lower FOV zooms in, higher zooms out
void Camera::processMouseScroll(float yOffset) {
    Fov -= yOffset;
    Fov  = std::clamp(Fov, 1.0f, 90.0f);
}

void Camera::updateCameraVectors() {
    glm::vec3 front;

    // Convert yaw and pitch angles into a 3D direction vector
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));

    Front = glm::normalize(front);
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up    = glm::normalize(glm::cross(Right, Front));
}

void Camera::confineToTube(float posZ, float tubeRadius) {
    float margin     = 0.4f;
    float maxR       = tubeRadius - margin;
    glm::vec2 xySelf = glm::vec2(Position.x, Position.y);
    float dist       = glm::length(xySelf);

    // Push position back inside the tube radius if it exceeds the boundary
    if (dist > maxR)
        Position = glm::vec3(
            glm::vec2(Position.x, Position.y) / dist * maxR,
            Position.z
        );
}

void Camera::confineToCave() {
    float z = Position.z;

    // Hard Z clamp so the player cant leave the cave lengthwise
    Position.z = std::clamp(z, -27.5f, 21.5f);
    z = Position.z;

    if (z >= 2.0f) {
        // Tunnel section
        confineToTube(z, 1.6f);
        Position.y = std::clamp(Position.y, -1.8f, 1.8f);

    } else if (z >= -7.0f) {
        // Flare section, smoothstep matches the cave geometry exactly
        float t  = (z - 2.0f) / (-7.0f - 2.0f);
        float st = t * t * (3.0f - 2.0f * t);
        float r  = glm::mix(1.6f, 8.5f, st);
        confineToTube(z, r);

    } else {
        // Main chamber
        confineToTube(z, 8.0f);
        Position.y = std::clamp(Position.y, -7.8f, 11.5f);
    }
}