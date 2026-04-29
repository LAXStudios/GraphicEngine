#include "../../../Headers/Core/Common/Common.h"
#include "Headers/Core/TextureManager/TextureManager.h"
#include <GL/gl.h>
#include <GL/glext.h>
#include <GLFW/glfw3.h>
#include <glm/detail/qualifier.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <string>
#include <thread>
#include <vector>

class MovementScene : public Scene {
private:
  ShaderProgram *shaderProgramPtr = nullptr;

  VertexArray *vertexArrayPtr = nullptr;
  VertexBuffer *vertexBufferPtr = nullptr;
  unsigned int texture;

  glm::mat4 view;
  glm::mat4 proj;

  bool firstMouse = true;
  bool isCursorHidden;

  float deltaTime, aspectRatio;
  float lastX, lastY;

  FPSCamera camera{glm::vec3(0.0f, 0.0f, 3.0f)};

public:
  MovementScene(const std::string &name) : Scene(name) {}

  void InitScene(GLFWwindow *window) override {
    shaderProgramPtr =
        new ShaderProgram(programPath("Main/Scenes/"
                                      "MovementScene/Shaders/shader.glsl"));
    shaderProgramPtr->Bind();

    vertexArrayPtr = new VertexArray();
    vertexBufferPtr = new VertexBuffer(
        cubeVertices.data(),
        static_cast<GLuint>(cubeVertices.size() * sizeof(float)));

    vertexArrayPtr->Bind();
    vertexBufferPtr->Bind();

    VertexBufferLayout layout;
    layout.AddElement<float>(3);
    layout.AddElement<float>(2);
    vertexArrayPtr->AddBuffer(*vertexBufferPtr, layout);

    vertexArrayPtr->UnBind();
    vertexBufferPtr->UnBind();

    texture = TextureManager::Get().LoadTexture(
        programPath("Main/Scenes/"
                    "MovementScene/Assets/soka_blue_cutie.png"));

    bindTexture(texture, 0);

    shaderProgramPtr->Bind();
    shaderProgramPtr->setUniform1i("texture0", 0);
    proj =
        glm::perspective(glm::radians(45.0f), 1280.0f / 800.0f, 0.1f, 100.0f);
    shaderProgramPtr->setUniformMatrix4fv("proj", proj);

    if (glfwRawMouseMotionSupported()) {
      glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
      std::cout << "Supported\n";
    }
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    isCursorHidden = true;
  }

  void HandleInput(GLFWwindow *window) override {
    if (isCursorHidden) {
      if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
      if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
      if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
      if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    }
  }

  void HandleInput(GLFWwindow *window, int key, int scancode, int action,
                   int mods) override {
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
      camera.ProcessMouseMovement(xOffset, yOffset);
  }

  void Update(float dt) override { deltaTime = dt; }

  void OnResize(float aspectRatio) override {
    this->aspectRatio = aspectRatio;
    proj = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);
    shaderProgramPtr->setUniformMatrix4fv("proj", proj);
  }

  void Render() override {
    RendererWrapper renderer{};

    view = camera.GetViewMatrix();
    shaderProgramPtr->setUniformMatrix4fv("view", view);

    proj =
        glm::perspective(glm::radians(camera.Zoom), aspectRatio, 0.1f, 100.0f);

    shaderProgramPtr->setUniformMatrix4fv("proj", proj);

    for (unsigned int i = 0; i < 10; i++) {
      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, cubePositions[i]);
      float angle = 20.0f * i;
      model =
          glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
      shaderProgramPtr->Bind();
      shaderProgramPtr->setUniformMatrix4fv("model", model);

      bindTexture(texture, 0);
      renderer.draw(*vertexArrayPtr, *shaderProgramPtr, 36);
    }
  }

  void ImGuiLayer() override {

    {
      if (ImGui::Begin("debug"), ImGuiWindowFlags_AlwaysAutoResize) {

        if (isCursorHidden)
          ImGui::Text("isCursorHidden = true");
        else
          ImGui::Text("isCursorHidden = false");

        ImGui::Text("dt: %.3f", deltaTime);
        ImGui::Text("cameraSpeed: %.2f", camera.MovementSpeed);
        ImGui::Text("cameraPos:\n X: %.2f Y: %.2f", camera.Position.x,
                    camera.Position.x);
        ImGui::Text("cameraFront:\n X: %.2f Y: %.2f", camera.Front.x,
                    camera.Front.y);
      }
      ImGui::End();
    }
  }

  ~MovementScene() {}

private:
  void changeCursorState(GLFWwindow *window) {
    if (!glfwGetWindowAttrib(window, GLFW_FOCUSED)) {
      std::cout << "Not Focused" << std::endl;
      return;
    }

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

  std::vector<glm::vec3> cubePositions = {
      glm::vec3(0.0f, 0.0f, 0.0f),    glm::vec3(2.0f, 5.0f, -15.0f),
      glm::vec3(-1.5f, -2.2f, -2.5f), glm::vec3(-3.8f, -2.0f, -12.3f),
      glm::vec3(2.4f, -0.4f, -3.5f),  glm::vec3(-1.7f, 3.0f, -7.5f),
      glm::vec3(1.3f, -2.0f, -2.5f),  glm::vec3(1.5f, 2.0f, -2.5f),
      glm::vec3(1.5f, 0.2f, -1.5f),   glm::vec3(-1.3f, 1.0f, -1.5f)};

  std::vector<float> cubeVertices = {
      -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.5f,  -0.5f, -0.5f, 1.0f, 0.0f,
      0.5f,  0.5f,  -0.5f, 1.0f, 1.0f, 0.5f,  0.5f,  -0.5f, 1.0f, 1.0f,
      -0.5f, 0.5f,  -0.5f, 0.0f, 1.0f, -0.5f, -0.5f, -0.5f, 0.0f, 0.0f,

      -0.5f, -0.5f, 0.5f,  0.0f, 0.0f, 0.5f,  -0.5f, 0.5f,  1.0f, 0.0f,
      0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
      -0.5f, 0.5f,  0.5f,  0.0f, 1.0f, -0.5f, -0.5f, 0.5f,  0.0f, 0.0f,

      -0.5f, 0.5f,  0.5f,  1.0f, 0.0f, -0.5f, 0.5f,  -0.5f, 1.0f, 1.0f,
      -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
      -0.5f, -0.5f, 0.5f,  0.0f, 0.0f, -0.5f, 0.5f,  0.5f,  1.0f, 0.0f,

      0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.5f,  0.5f,  -0.5f, 1.0f, 1.0f,
      0.5f,  -0.5f, -0.5f, 0.0f, 1.0f, 0.5f,  -0.5f, -0.5f, 0.0f, 1.0f,
      0.5f,  -0.5f, 0.5f,  0.0f, 0.0f, 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

      -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.5f,  -0.5f, -0.5f, 1.0f, 1.0f,
      0.5f,  -0.5f, 0.5f,  1.0f, 0.0f, 0.5f,  -0.5f, 0.5f,  1.0f, 0.0f,
      -0.5f, -0.5f, 0.5f,  0.0f, 0.0f, -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,

      -0.5f, 0.5f,  -0.5f, 0.0f, 1.0f, 0.5f,  0.5f,  -0.5f, 1.0f, 1.0f,
      0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
      -0.5f, 0.5f,  0.5f,  0.0f, 0.0f, -0.5f, 0.5f,  -0.5f, 0.0f, 1.0f};
};
