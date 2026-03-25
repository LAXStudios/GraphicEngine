#include "../../../Headers/Core/Common/Common.h"
#include <GLFW/glfw3.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <iostream>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <glm/trigonometric.hpp>
#include <string>

class ColorLightningScene : public Scene {
private:
  ShaderProgram *lightingShaderProgramPtr = nullptr;
  ShaderProgram *lightCubeShaderProgramPtr = nullptr;

  VertexArray *cubeVAOPtr = nullptr;
  VertexBuffer *vertexBufferPtr = nullptr;

  FPSCamera fpsCamera{glm::vec3(0.0f, 0.0f, 3.0f)};

  float deltaTime;

  glm::mat4 view;
  glm::mat4 proj;

  glm::vec3 lightPos = glm::vec3(1.2f, 1.0f, 2.0f);

public:
  ColorLightningScene(const std::string &name) : Scene(name) {}

  void InitScene(GLFWwindow *window) override {
    lightingShaderProgramPtr =
        new ShaderProgram("/home/lax/Coding/GraphicEngine/src/Main/Scenes/"
                          "ColorLightingScene/Shaders/cubeShader.glsl");
    lightingShaderProgramPtr->Bind();

    lightCubeShaderProgramPtr =
        new ShaderProgram("/home/lax/Coding/GraphicEngine/src/Main/Scenes/"
                          "ColorLightingScene/Shaders/lightCubeShader.glsl");
    lightCubeShaderProgramPtr->Bind();

    cubeVAOPtr = new VertexArray();

    vertexBufferPtr = new VertexBuffer(
        cubeVertices.data(),
        static_cast<GLuint>(cubeVertices.size() * sizeof(float)));

    cubeVAOPtr->Bind();
    vertexBufferPtr->Bind();

    VertexBufferLayout layout;
    layout.AddElement<float>(3);
    cubeVAOPtr->AddBuffer(*vertexBufferPtr, layout);

    cubeVAOPtr->UnBind();
    vertexBufferPtr->UnBind();

    proj = glm::perspective(glm::radians(fpsCamera.Zoom), 1280.0f / 800.0f,
                            0.1f, 100.0f);
  }

  void HandleInput(GLFWwindow *window) override {
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
      fpsCamera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
      fpsCamera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
      fpsCamera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
      fpsCamera.ProcessKeyboard(RIGHT, deltaTime);
  }

  void Update(float dt) override { deltaTime = dt; }

  void Render() override {
    RendererWrapper renderer{};

    lightingShaderProgramPtr->Bind();
    lightingShaderProgramPtr->setUniform3f("objectColor", 1.0f, 0.5f, 0.3f);
    lightingShaderProgramPtr->setUniform3f("lightColor", 1.0f, 1.0f, 1.0f);

    view = fpsCamera.GetViewMatrix();
    lightingShaderProgramPtr->setUniformMatrix4fv("projection", proj);
    lightingShaderProgramPtr->setUniformMatrix4fv("view", view);

    glm::mat4 model = glm::mat4(1.0f);
    lightingShaderProgramPtr->setUniformMatrix4fv("model", model);

    renderer.draw(*cubeVAOPtr, *lightingShaderProgramPtr, 36);

    // lamp cube
    lightCubeShaderProgramPtr->Bind();
    lightCubeShaderProgramPtr->setUniformMatrix4fv("projection", proj);
    lightCubeShaderProgramPtr->setUniformMatrix4fv("view", view);
    model = glm::mat4(1.0f);
    model = glm::translate(model, lightPos);
    model = glm::scale(model, glm::vec3(0.2f));
    lightCubeShaderProgramPtr->setUniformMatrix4fv("model", model);

    renderer.draw(*cubeVAOPtr, *lightCubeShaderProgramPtr, 36);
  }

  void OnResize(float aspectRatio) override {
    proj = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);
    lightingShaderProgramPtr->setUniformMatrix4fv("proj", proj);
    lightCubeShaderProgramPtr->setUniformMatrix4fv("proj", proj);
  }

private:
  std::vector<glm::vec3> cubePositions = {
      glm::vec3(0.0f, 0.0f, 0.0f),    glm::vec3(2.0f, 5.0f, -15.0f),
      glm::vec3(-1.5f, -2.2f, -2.5f), glm::vec3(-3.8f, -2.0f, -12.3f),
      glm::vec3(2.4f, -0.4f, -3.5f),  glm::vec3(-1.7f, 3.0f, -7.5f),
      glm::vec3(1.3f, -2.0f, -2.5f),  glm::vec3(1.5f, 2.0f, -2.5f),
      glm::vec3(1.5f, 0.2f, -1.5f),   glm::vec3(-1.3f, 1.0f, -1.5f)};

  std::vector<float> cubeVertices = {
      -0.5f, -0.5f, -0.5f, 0.5f,  -0.5f, -0.5f, 0.5f,  0.5f,  -0.5f,
      0.5f,  0.5f,  -0.5f, -0.5f, 0.5f,  -0.5f, -0.5f, -0.5f, -0.5f,

      -0.5f, -0.5f, 0.5f,  0.5f,  -0.5f, 0.5f,  0.5f,  0.5f,  0.5f,
      0.5f,  0.5f,  0.5f,  -0.5f, 0.5f,  0.5f,  -0.5f, -0.5f, 0.5f,

      -0.5f, 0.5f,  0.5f,  -0.5f, 0.5f,  -0.5f, -0.5f, -0.5f, -0.5f,
      -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, 0.5f,  -0.5f, 0.5f,  0.5f,

      0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  -0.5f, 0.5f,  -0.5f, -0.5f,
      0.5f,  -0.5f, -0.5f, 0.5f,  -0.5f, 0.5f,  0.5f,  0.5f,  0.5f,

      -0.5f, -0.5f, -0.5f, 0.5f,  -0.5f, -0.5f, 0.5f,  -0.5f, 0.5f,
      0.5f,  -0.5f, 0.5f,  -0.5f, -0.5f, 0.5f,  -0.5f, -0.5f, -0.5f,

      -0.5f, 0.5f,  -0.5f, 0.5f,  0.5f,  -0.5f, 0.5f,  0.5f,  0.5f,
      0.5f,  0.5f,  0.5f,  -0.5f, 0.5f,  0.5f,  -0.5f, 0.5f,  -0.5f,
  };
};
