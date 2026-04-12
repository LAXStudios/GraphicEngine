#include "../../../Headers/Core/Camera/FPSCamera.h"
#include <GLFW/glfw3.h>
#include <glm/detail/qualifier.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

FPSCamera::FPSCamera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : Front(glm::vec3(0.0f, 0.0f, 0.0f)), MovementSpeed(SPEED),
      MouseSensitivity(SENSITIVITY), Zoom(ZOOM) {
  Position = position;
  WorldUp = up;
  Yaw = yaw;
  Pitch = pitch;
  updateCameraVectors();
}

FPSCamera::FPSCamera(float posX, float posY, float posZ, float upX, float upY,
                     float upZ, float yaw, float pitch)
    : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED),
      MouseSensitivity(SENSITIVITY), Zoom(ZOOM) {
  Position = glm::vec3(posX, posY, posZ);
  WorldUp = glm::vec3(upX, upY, upZ);
  Yaw = yaw;
  Pitch = pitch;
  updateCameraVectors();
}

glm::mat4 FPSCamera::GetViewMatrix() {
  return glm::lookAt(Position, Position + Front, Up);
}

void FPSCamera::ProcessKeyboard(CameraMovement direction, float deltaTime) {
  float velocity = MovementSpeed * deltaTime;
  if (direction == FORWARD)
    Position += Front * velocity;
  if (direction == BACKWARD)
    Position -= Front * velocity;
  if (direction == RIGHT)
    Position += Right * velocity;
  if (direction == LEFT)
    Position -= Right * velocity;

  std::cout << "velocity: " << velocity << std::endl;
}

void FPSCamera::ProcessMouseMovement(float xOffset, float yOffset,
                                     bool constantPitch) {

  std::cout << "Mouse Movement\n";

  xOffset *= MouseSensitivity;
  yOffset *= MouseSensitivity;

  Yaw += xOffset;
  Pitch += yOffset;

  if (constantPitch) {
    if (Pitch > 89.0f)
      Pitch = 89.0f;
    if (Pitch < -89.0f)
      Pitch = -89.0f;
  }

  updateCameraVectors();
}

void FPSCamera::ProcessMouseScroll(float yOffset) {
  Zoom -= yOffset;
  if (Zoom < 1.0f)
    Zoom = 1.0f;
  if (Zoom > 45.0f)
    Zoom = 45.0f;
}
