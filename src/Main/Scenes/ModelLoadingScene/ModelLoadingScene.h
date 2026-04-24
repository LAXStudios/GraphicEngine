#include "../../../Headers/Core/Camera/FPSCamera.h"
#include "../../../Headers/Core/Model/Model.h"
#include "../../../Headers/Core/ShaderProgram/ShaderProgram.h"
#include "../../../Headers/Scene/Scene.h"
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/trigonometric.hpp>
#include <memory>
#include <thread>

class ModelLoadingScene : public Scene {
private:
  std::unique_ptr<ShaderProgram> shaderProgramPtr = nullptr;
  Model *myModel = nullptr;

  glm::mat4 view;
  glm::mat4 proj;

  float deltaTime, aspectRatio;

  bool firstMouse = true;
  bool isCursorHidden = false;

  float lastX, lastY;

  FPSCamera camera{glm::vec3(0.0f, 0.0f, 3.0f)};

public:
  ModelLoadingScene(const std::string &name) : Scene(name) {}

  void InitScene(GLFWwindow *window) override {

    glfwMakeContextCurrent(window);

    std::cout << "ModelLoadingScene::InitScene -> Main thread ID: "
              << std::this_thread::get_id() << std::endl;
    std::cout << "ModelLoadingScene::InitScene -> Current context:"
              << glfwGetCurrentContext() << std::endl;

    shaderProgramPtr = std::make_unique<ShaderProgram>(
        "/home/lax/Coding/GraphicEngine/src/Main/Scenes/"
        "ModelLoadingScene/Shader/basic.glsl");
    shaderProgramPtr->Bind();

    myModel = new Model("/home/lax/Coding/GraphicEngine/src/Main/Scenes/"
                        "ModelLoadingScene/Assets/backpack/backpack.obj");

    proj = glm::perspective(glm::radians(camera.Zoom), 1280.0f / 800.0f, 0.1f,
                            100.0f);

    glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    isCursorHidden = true;
  }

  void Render() override {

    glDisable(GL_CULL_FACE);

    shaderProgramPtr->Bind();

    // Was ist die tatsächliche ID?
    std::cout << "ShaderProgram ID: " << shaderProgramPtr->GetID() << std::endl;

    // Welches Programm ist gerade aktiv in OpenGL?
    GLint currentProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    std::cout << "GL_CURRENT_PROGRAM: " << currentProgram << std::endl;

    glm::mat4 view = camera.GetViewMatrix();

    shaderProgramPtr->setUniformMatrix4fv("view", view);

    proj =
        glm::perspective(glm::radians(camera.Zoom), aspectRatio, 0.1f, 100.0f);

    shaderProgramPtr->setUniformMatrix4fv("projection", proj);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
    model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
    shaderProgramPtr->setUniformMatrix4fv("model", model);

    myModel->Draw(*shaderProgramPtr);
  }

  void Update(float dt) override { deltaTime = dt; }

  void OnResize(float aspectRatio) override {
    this->aspectRatio = aspectRatio;

    proj =
        glm::perspective(glm::radians(camera.Zoom), aspectRatio, 0.1f, 100.0f);
    shaderProgramPtr->setUniformMatrix4fv("projection", proj);
  }

  void HandleInput(GLFWwindow *window) override {
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
      camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
      camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
      camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
      camera.ProcessKeyboard(RIGHT, deltaTime);
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
      camera.ProcessMouseMovement(xOffset, yOffset);
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
};
