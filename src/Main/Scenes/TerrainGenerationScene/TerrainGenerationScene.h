#pragma once

#include "../../../Headers/Core/Common/Common.h"
#include "Headers/Core/Camera/OrbitCamera.h"
#include "Headers/Core/Common/ErrorHandling.h"
#include "Headers/Core/ShaderProgram/ShaderProgram.h"
#include "Headers/Scene/Scene.h"
#include "TerrainMesh.h"
#include <GLFW/glfw3.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <string>

class TerrainGenerationScene : public Scene {
private:
  TerrainMesh *terrainMesh = nullptr;
  ShaderProgram *shaderProgram = nullptr;

  OrbitCamera camera;
  float aspectRatio = 1280.0f / 800.0f;
  float deltaTime = 0.0f;
  float lastMouseX = 0.0f;
  float lastMouseY = 0.0f;
  bool firstMouse = true;
  bool isOrbiting = false;
  bool isPanning = false;

  float maxHeight = 5.0f;
  glm::vec3 lightDir = glm::normalize(glm::vec3(1.0f, 3.0f, 1.0f));
  glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);

public:
  TerrainGenerationScene(const std::string &name, const std::string &category)
      : Scene(name, category) {
    camera.target = glm::vec3(0.0f);
    camera.azimuth = 45.0f;
    camera.elevation = 35.0f;
    camera.distance = 30.0f;
  }

  void InitScene(GLFWwindow *window) override {
    shaderProgram = new ShaderProgram(
        programPath("Main/Scenes/TerrainGenerationScene/Shaders/terrain.glsl"));

    terrainMesh = new TerrainMesh(100, 100, 2.5f);

    glCall(glEnable(GL_DEPTH_TEST));
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  }

  void Update(float dt) override { deltaTime = dt; }

  void Render() override {
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 proj =
        glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 200.0f);

    shaderProgram->Bind();
    shaderProgram->setUniformMatrix4fv("model", model);
    shaderProgram->setUniformMatrix4fv("view", view);
    shaderProgram->setUniformMatrix4fv("proj", proj);
    shaderProgram->setUniform1f("maxHeight", maxHeight);
    shaderProgram->setUniform3fv("lightDir", lightDir);
    shaderProgram->setUniform3fv("lightColor", lightColor);

    terrainMesh->Draw(*shaderProgram);
  }

  void HandleInput(GLFWwindow *window) override {
    bool mmb =
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
    bool shift = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                 glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;

    isOrbiting = mmb && !shift;
    isPanning = mmb && shift;

    if (glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS)
      camera.Zoom(5.0f * deltaTime);
    if (glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS)
      camera.Zoom(-5.0f * deltaTime);
  }

  void HandleInput(GLFWwindow *window, int key, int scancode, int action,
                   int mods) override {
    if (action != GLFW_PRESS)
      return;
    if (key == GLFW_KEY_KP_1)
      camera.SetFrontView();
    if (key == GLFW_KEY_KP_3)
      camera.SetRightView();
    if (key == GLFW_KEY_KP_7)
      camera.SetTopView();
    if (key == GLFW_KEY_KP_9)
      camera.SetOppositeView();
    if (key == GLFW_KEY_R) {
      camera.target = glm::vec3(0.0f);
      camera.azimuth = 45.0f;
      camera.elevation = 35.0f;
      camera.distance = 30.0f;
    }
  }

  void HandleMouseInput(GLFWwindow *window, double xpos, double ypos) override {
    float x = static_cast<float>(xpos);
    float y = static_cast<float>(ypos);

    if (firstMouse) {
      lastMouseX = x;
      lastMouseY = y;
      firstMouse = false;
      return;
    }

    float dx = x - lastMouseX;
    float dy = lastMouseY - y;
    lastMouseX = x;
    lastMouseY = y;

    if (isOrbiting)
      camera.Orbit(dx, dy);
    if (isPanning)
      camera.Pan(dx, dy);
  }

  void HandleScrollInput(GLFWwindow *window, double xoffset,
                         double yoffset) override {
    camera.Zoom(static_cast<float>(yoffset));
  }

  void OnResize(float ar) override { aspectRatio = ar; }

  void ImGuiLayer() override {
    ImGui::Begin("Terrain", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::SliderFloat("Max Height", &maxHeight, 1.0f, 20.0f);
    ImGui::SliderFloat3("Light Dir", &lightDir.x, -1.0f, 1.0f);
    ImGui::ColorEdit3("Light Color", &lightColor.x);
    ImGui::Separator();
    ImGui::Text("MMB: Orbit  |  Shift+MMB: Pan  |  Scroll: Zoom");
    ImGui::Text("R: Reset Camera");
    ImGui::End();
  }

  ~TerrainGenerationScene() {
    delete shaderProgram;
    delete terrainMesh;
  }
};
