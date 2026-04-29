#include "../../../Headers/Core/Common/Common.h"
#include <GLFW/glfw3.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <glm/trigonometric.hpp>
#include <string>

class BasicMaterialScene : public Scene {
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
  glm::vec3 lightColor;

  bool firstMouse = true;
  bool isCursorHidden = false;
  bool lightHasColor = false;

  float lastX, lastY;

public:
  BasicMaterialScene(const std::string &name) : Scene(name) {}

  void InitScene(GLFWwindow *window) override {
    lightingShaderProgramPtr =
        new ShaderProgram("/home/lax/Coding/GraphicEngine/src/Main/Scenes/"
                          "BasicMaterialScene/Shaders/materialShader.glsl");
    lightingShaderProgramPtr->Bind();

    lightCubeShaderProgramPtr =
        new ShaderProgram("/home/lax/Coding/GraphicEngine/src/Main/Scenes/"
                          "BasicMaterialScene/Shaders/lightSourceShader.glsl");
    lightCubeShaderProgramPtr->Bind();

    cubeVAOPtr = new VertexArray();

    vertexBufferPtr = new VertexBuffer(
        cubeVertices.data(),
        static_cast<GLuint>(cubeVertices.size() * sizeof(float)));

    cubeVAOPtr->Bind();
    vertexBufferPtr->Bind();

    VertexBufferLayout layout;
    layout.AddElement<float>(3);
    layout.AddElement<float>(3);
    cubeVAOPtr->AddBuffer(*vertexBufferPtr, layout);

    cubeVAOPtr->UnBind();
    vertexBufferPtr->UnBind();

    proj = glm::perspective(glm::radians(fpsCamera.Zoom), 1280.0f / 800.0f,
                            0.1f, 100.0f);

    glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    isCursorHidden = true;
  }

  void Update(float dt) override { deltaTime = dt; }

  void Render() override {
    RendererWrapper renderer{};

    // Color Change Effect
    lightColor.x = sin(glfwGetTime() * 2.0f);
    lightColor.y = sin(glfwGetTime() * 0.7f);
    lightColor.z = sin(glfwGetTime() * 1.3f);

    glm::vec3 diffuseColor = lightColor * glm::vec3(0.5f);
    glm::vec3 ambientColor = diffuseColor * glm::vec3(0.2f);

    lightingShaderProgramPtr->Bind();

    lightingShaderProgramPtr->setUniformMatrix4fv("view", view);
    lightingShaderProgramPtr->setUniform3fv("light.position", lightPos);

    lightingShaderProgramPtr->setUniform3f("objectColor", 1.0f, 0.5f, 0.3f);
    lightingShaderProgramPtr->setUniform3f("lightColor", 1.0f, 1.0f, 1.0f);

    lightingShaderProgramPtr->setUniform3f("material.ambient", 1.0f, 0.5f,
                                           0.31f);
    lightingShaderProgramPtr->setUniform3f("material.diffuse", 1.0f, 0.5f,
                                           0.31f);
    lightingShaderProgramPtr->setUniform3f("material.specular", 0.5, 0.5f,
                                           0.5f);
    lightingShaderProgramPtr->setUniform1f("material.shininess", 32.0f);

    lightingShaderProgramPtr->setUniform3fv("light.ambient", ambientColor);
    lightingShaderProgramPtr->setUniform3fv("light.diffuse", diffuseColor);
    lightingShaderProgramPtr->setUniform3f("light.specular", 1.0f, 1.0f, 1.0f);

    view = fpsCamera.GetViewMatrix();
    lightingShaderProgramPtr->setUniformMatrix4fv("projection", proj);
    lightingShaderProgramPtr->setUniform3fv("viewPos", fpsCamera.Position);

    glm::mat4 model = glm::mat4(1.0f);
    lightingShaderProgramPtr->setUniformMatrix4fv("model", model);

    renderer.draw(*cubeVAOPtr, *lightingShaderProgramPtr, 36);

    // lamp cube
    lightCubeShaderProgramPtr->Bind();
    if (lightHasColor)
      lightCubeShaderProgramPtr->setUniform3fv("lightColor", lightColor);
    else
      lightCubeShaderProgramPtr->setUniform3fv("lightColor", glm::vec3(1.0));

    lightCubeShaderProgramPtr->setUniformMatrix4fv("projection", proj);
    lightCubeShaderProgramPtr->setUniformMatrix4fv("view", view);
    model = glm::mat4(1.0f);
    model = glm::translate(model, lightPos);
    model = glm::scale(model, glm::vec3(0.2f));
    lightCubeShaderProgramPtr->setUniformMatrix4fv("model", model);

    renderer.draw(*cubeVAOPtr, *lightCubeShaderProgramPtr, 36);
  }

  void ImGuiLayer() override {
    {
      if (ImGui::Begin("debug"), ImGuiChildFlags_AlwaysAutoResize) {
        ImGui::Text("dt: %f", deltaTime);
        ImGui::Text("time passed: %f", glfwGetTime());
        ImGui::SeparatorText("light");
        ImGui::Text("R: %f", lightColor.r);
        ImGui::Text("G: %f", lightColor.g);
        ImGui::Text("B: %f", lightColor.b);
        if (ImGui::Checkbox("Light has Color", &lightHasColor)) {
        }
      }
    }

    ImGui::End();
  }

  void OnResize(float aspectRatio) override {
    proj = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);
    lightingShaderProgramPtr->setUniformMatrix4fv("proj", proj);
    lightCubeShaderProgramPtr->setUniformMatrix4fv("proj", proj);
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

  void HandleInput(GLFWwindow *window, int key, int scancode, int action,
                   int mod) override {
    if (key == GLFW_KEY_TAB && action == GLFW_PRESS)
      changeCursorState(window);
  }

  void HandleMouseInput(GLFWwindow *window, double xpos, double ypos) override {
    float xPos = static_cast<float>(xpos);
    float yPos = static_cast<float>(ypos);

    if (firstMouse) {
      lastX = xPos;
      lastY = yPos;
      firstMouse = false;
    }

    float xOffset = xPos - lastX;
    float yOffset = lastY - yPos;

    lastX = xPos;
    lastY = yPos;

    if (isCursorHidden)
      fpsCamera.ProcessMouseMovement(xOffset, yOffset);
  }

private:
  void changeCursorState(GLFWwindow *window) {

    double xpos = 0, ypos = 0;
    if (isCursorHidden) {
      glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
      isCursorHidden = false;
    } else {

      glfwGetCursorPos(window, &xpos, &ypos);

      if (glfwRawMouseMotionSupported())
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

      isCursorHidden = true;
    }

    glfwPollEvents();
    glfwWaitEventsTimeout(0.01);
    glfwPostEmptyEvent();
    glfwFocusWindow(window);
  }

  const std::vector<glm::vec3> cubePositions = {
      glm::vec3(0.0f, 0.0f, 0.0f),    glm::vec3(2.0f, 5.0f, -15.0f),
      glm::vec3(-1.5f, -2.2f, -2.5f), glm::vec3(-3.8f, -2.0f, -12.3f),
      glm::vec3(2.4f, -0.4f, -3.5f),  glm::vec3(-1.7f, 3.0f, -7.5f),
      glm::vec3(1.3f, -2.0f, -2.5f),  glm::vec3(1.5f, 2.0f, -2.5f),
      glm::vec3(1.5f, 0.2f, -1.5f),   glm::vec3(-1.3f, 1.0f, -1.5f)};

  const std::vector<float> cubeVertices = {
      -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, 0.5f,  -0.5f, -0.5f,
      0.0f,  0.0f,  -1.0f, 0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f,
      0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f, -0.5f, 0.5f,  -0.5f,
      0.0f,  0.0f,  -1.0f, -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f,

      -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,  0.5f,  -0.5f, 0.5f,
      0.0f,  0.0f,  1.0f,  0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
      0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  -0.5f, 0.5f,  0.5f,
      0.0f,  0.0f,  1.0f,  -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,

      -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f,  -0.5f, 0.5f,  -0.5f,
      -1.0f, 0.0f,  0.0f,  -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f,
      -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f,  -0.5f, -0.5f, 0.5f,
      -1.0f, 0.0f,  0.0f,  -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f,

      0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.5f,  0.5f,  -0.5f,
      1.0f,  0.0f,  0.0f,  0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,  0.0f,
      0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,  0.0f,  0.5f,  -0.5f, 0.5f,
      1.0f,  0.0f,  0.0f,  0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

      -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,  0.5f,  -0.5f, -0.5f,
      0.0f,  -1.0f, 0.0f,  0.5f,  -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,
      0.5f,  -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,  -0.5f, -0.5f, 0.5f,
      0.0f,  -1.0f, 0.0f,  -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,

      -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,  0.5f,  0.5f,  -0.5f,
      0.0f,  1.0f,  0.0f,  0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
      0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  -0.5f, 0.5f,  0.5f,
      0.0f,  1.0f,  0.0f,  -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f};
};
