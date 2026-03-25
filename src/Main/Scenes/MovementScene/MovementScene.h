#include "../../../Headers/Core/Common/Common.h"
#include <GL/gl.h>
#include <GL/glext.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <cstdlib>
#include <glm/detail/qualifier.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

class MovementScene : public Scene {
private:
  ShaderProgram *shaderProgramPtr = nullptr;

  VertexArray *vertexArrayPtr = nullptr;
  VertexBuffer *vertexBufferPtr = nullptr;
  Texture *texture = nullptr;

  // glm::mat4 model;
  glm::mat4 view;
  glm::mat4 proj;

  glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 5.0);
  glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
  glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

  float deltaTime = 0;
  float cameraSpeed;

  bool firstMousePosInput = true;
  bool isCursorHidden = true;

  double lastX = 0, lastY = 0;

  float yaw, pitch;

public:
  MovementScene(const std::string &name) : Scene(name) {}

  void InitScene(GLFWwindow *window) override {
    shaderProgramPtr =
        new ShaderProgram("/home/lax/Coding/GraphicEngine/src/Main/Scenes/"
                          "MovementScene/Shaders/shader.glsl");
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

    texture = new Texture("/home/lax/Coding/GraphicEngine/src/Main/Scenes/"
                          "MovementScene/Assets/soka_blue_cutie.png");
    texture->Bind();

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
  }

  void HandleInput(GLFWwindow *window) override {
    cameraSpeed = 4.0f * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
      cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
      cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
      cameraPos -=
          glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
      cameraPos +=
          glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    }
  }

  void HandleInput(GLFWwindow *window, int key, int scancode, int action,
                   int mods) override {
    if (key == GLFW_KEY_TAB && action == GLFW_PRESS)
      changeCursorState(window);
  }

  void HandleMouseInput(GLFWwindow *window, double xpos, double ypos) override {
    if (isCursorHidden) {

      if (firstMousePosInput) {
        lastX = xpos;
        lastY = ypos;
        firstMousePosInput = false;
      }

      float xOffset = xpos - lastX;
      float yOffset = lastY - ypos;
      lastX = xpos;
      lastY = ypos;

      float sensitivity = 0.1f;
      xOffset *= sensitivity;
      yOffset *= sensitivity;

      yaw += xOffset;
      pitch += yOffset;

      if (pitch > 89.0f)
        pitch = 89.0f;
      if (pitch < -89.0f)
        pitch = -89.0f;

      glm::vec3 direction;
      direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
      direction.y = sin(glm::radians(pitch));
      direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
      cameraFront = glm::normalize(direction);
    }
  }

  void Update(float dt) override {
    // model = glm::mat4(1.0f);
    // view = glm::mat4(1.0f);

    // model = glm::rotate(model, dt, glm::vec3(0.5f, 1.0f, 0.0f));

    // view = glm::translate(view, glm::vec3(0.0f, 0.0f, -5.0f));
    deltaTime = dt;

    // view = glm::mat4(1.0f);
    view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
  }

  void OnResize(float aspectRatio) override {
    std::cout << "[MovementScene] OnResize\n";
    proj = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);
    shaderProgramPtr->setUniformMatrix4fv("proj", proj);
  }

  void Render() override {
    RendererWrapper renderer{};

    shaderProgramPtr->setUniformMatrix4fv("view", view);

    for (unsigned int i = 0; i < 10; i++) {
      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, cubePositions[i]);
      float angle = 20.0f * i;
      model =
          glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
      shaderProgramPtr->Bind();
      shaderProgramPtr->setUniformMatrix4fv("model", model);

      texture->Bind();
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
        ImGui::Text("cameraSpeed: %.2f", cameraSpeed);
        ImGui::Text("cameraPos:\n X: %.2f Y: %.2f", cameraPos.x, cameraPos.y);
        ImGui::Text("cameraFront:\n X: %.2f Y: %.2f", cameraFront.x,
                    cameraFront.y);
      }
      ImGui::End();
    }
  }

  // INFO: Eigentlicher plan war es wenn TAB dann die momentene Cursor Pos zu
  // nehmen, und wenn wieder Tab, dann den Cursor direkt zur letzten Pos
  // setzten, wegen Wayland geht das nicht.
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

    std::cout << "Current Thread ID: " << std::this_thread::get_id()
              << std::endl;

    glfwPollEvents();
    glfwWaitEventsTimeout(0.01);
    glfwPostEmptyEvent();
    glfwFocusWindow(window);
  }

  ~MovementScene() {}

private:
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
