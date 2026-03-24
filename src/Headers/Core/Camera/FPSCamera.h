#pragma once
#include "glm/glm.hpp"

enum CameraMovement {
  FORWARD,
  BACKWARD,
  LEFT,
  RIGHT,
};

// DEFAULT SETTINGS
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 4.0f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 45.0f;

class FPSCamera {
public:
  // camera Attr
  glm::vec3 Position;
  glm::vec3 Front;
  glm::vec3 Up;
  glm::vec3 Right;
  glm::vec3 WorldUp;

  // euler Angles
  float Yaw, Pitch;

  // camera options
  float MovementSpeed, MouseSensitivity, Zoom;

  // Constructor with vectors
  FPSCamera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW,
            float pitch = PITCH);

  // Constructor with scalar values
  FPSCamera(float posX, float posY, float posZ, float upX, float upY, float upZ,
            float yaw, float pitch);

  glm::mat4 GetViewMatrix();

  void ProcessKeyboard(CameraMovement direction, float deltaTime);

  void ProcessMouseMovement(float xOffset, float yOffset,
                            bool constrainPitch = true);

  void ProcessMouseScroll(float yOffset);

private:
  void updateCameraVectors() {
    // calculate the new Front vector
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);
    // also re-calculate the Right and Up vector
    Right = glm::normalize(glm::cross(
        Front, WorldUp)); // normalize the vectors, because their length gets
                          // closer to 0 the more you look up or down which
                          // results in slower movement.
    Up = glm::normalize(glm::cross(Right, Front));
  }
};
