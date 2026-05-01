#pragma once
#include <cmath>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/trigonometric.hpp>

class OrbitCamera {
public:
  glm::vec3 target = glm::vec3(0.0f);

  float azimuth = 0.0f;    // horizontal angle
  float elevation = 30.0f; // vertical angle
  float distance = 10.0f;

  // Settings
  float orbitSensitivity = 0.4f;
  float panSensitivity = 0.001;
  float zoomSensitivity = 0.5f;
  float minDistance = 0.5f;
  float maxDistance = 500.0f;

  glm::mat4 GetViewMatrix() const {
    glm::vec3 pos = GetPosition();
    return glm::lookAt(pos, target, glm::vec3(0.0f, 1.0f, 0.0));
  }

  glm::vec3 GetPosition() const {
    float azRad = glm::radians(azimuth);
    float elRad = glm::radians(elevation);

    glm::vec3 offset;
    offset.x = distance * std::cos(elRad) * std::sin(azRad);
    offset.y = distance * std::sin(elRad);
    offset.z = distance * std::cos(elRad) * std::cos(azRad);

    return target + offset;
  }

  void Orbit(float xOffset, float yOffset) {
    azimuth -= xOffset * orbitSensitivity;
    elevation += yOffset * orbitSensitivity;

    if (elevation > 89.0f)
      elevation = 89.0f;
    if (elevation < -89.0f)
      elevation = -89.0f;
  }

  void Pan(float xOffset, float yOffset) {
    glm::vec3 pos = GetPosition();
    glm::vec3 forward = glm::normalize(target - pos);
    glm::vec3 right =
        glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 up = glm::normalize(glm::cross(right, forward));

    float scale = distance * panSensitivity;

    target -= right * (xOffset * scale);
    target -= up * (yOffset * scale);
  }

  void Zoom(float yOffset) {
    distance -= yOffset * zoomSensitivity * (distance * 0.1f);

    if (distance < minDistance)
      distance = minDistance;
    if (distance > maxDistance)
      distance = maxDistance;
  }

  void SetFrontView() {
    azimuth = 0.0f;
    elevation = 0.0f;
  }
  void SetBackView() {
    azimuth = 180.0f;
    elevation = 0.0f;
  }
  void SetRightView() {
    azimuth = 90.0f;
    elevation = 0.0f;
  }
  void SetLeftView() {
    azimuth = 270.0f;
    elevation = 0.0f;
  }
  void SetTopView() {
    azimuth = 0.0f;
    elevation = 89.0f;
  }
  void SetBottomView() {
    azimuth = 0.0f;
    elevation = -89.0f;
  }

  void OnFocus(const glm::vec3 &point, float newDistance = -1.0f) {
    target = point;
    if (newDistance > 0.0f)
      distance = newDistance;
  }
};
