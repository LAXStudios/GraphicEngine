#include "../../../Headers/Core/Common/Common.h"
#include <GLFW/glfw3.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <iostream>
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include "../../../../extern/stb/stb_image.h"
#include <glm/gtx/string_cast.hpp>
#include <glm/trigonometric.hpp>
#include <string>

class lTexture {
private:
  unsigned int ID;
  std::string filePath;
  unsigned char *data;

  int width, height;

public:
  lTexture(const std::string &filePath)
      : filePath(filePath), data(nullptr), width(0), height(0) {
    stbi_set_flip_vertically_on_load(true);

    int nrComponents = 0;
    unsigned char *data =
        stbi_load(filePath.c_str(), &width, &height, &nrComponents, 0);

    glCall(glGenTextures(1, &ID));
    glCall(glBindTexture(GL_TEXTURE_2D, ID));

    if (!data)
      std::cout << "Error while Texture gen\n";

    glCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                        GL_UNSIGNED_BYTE, data));
    glCall(glGenerateMipmap(GL_TEXTURE_2D));

    glCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    glCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    glCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    glCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
  }

  ~lTexture() { glCall(glDeleteTextures(1, &ID)); }

  unsigned int GetID() const { return ID; }

  void Bind(unsigned int slot = 0) const {
    glCall(glActiveTexture(GL_TEXTURE0 + slot));
    glCall(glBindTexture(GL_TEXTURE_2D, ID));
  }

  void UnBind() const { glCall(glBindTexture(GL_TEXTURE_2D, 0)); }
};

class BasicMultipleLightsScene : public Scene {
private:
  ShaderProgram *lightingShaderProgramPtr = nullptr;
  ShaderProgram *lampsShaderProgramPtr = nullptr;

  VertexArray *cubeVAOPtr = nullptr;
  VertexBuffer *vertexBufferPtr = nullptr;
  lTexture *texture = nullptr;
  lTexture *texture01 = nullptr;

  FPSCamera fpsCamera{glm::vec3(0.0f, 0.0f, 3.0f)};

  float deltaTime;

  glm::mat4 view;
  glm::mat4 proj;

  glm::vec3 lightColor;

  bool firstMouse = true;
  bool isCursorHidden = false;
  bool lightHasColor = false;

  float lastX, lastY;

public:
  BasicMultipleLightsScene(const std::string &name) : Scene(name) {}

  void InitScene(GLFWwindow *window) override {
    lightingShaderProgramPtr = new ShaderProgram(
        "/home/lax/Coding/GraphicEngine/src/Main/Scenes/"
        "BasicMultipleLightsScene/Shaders/multipleLightsShader.glsl");
    lightingShaderProgramPtr->Bind();

    lampsShaderProgramPtr =
        new ShaderProgram("/home/lax/Coding/GraphicEngine/src/Main/Scenes/"
                          "BasicMultipleLightsScene/Shaders/cubeShader.glsl");
    lampsShaderProgramPtr->Bind();

    cubeVAOPtr = new VertexArray();

    vertexBufferPtr = new VertexBuffer(
        cubeVertices.data(),
        static_cast<GLuint>(cubeVertices.size() * sizeof(float)));

    cubeVAOPtr->Bind();
    vertexBufferPtr->Bind();

    texture = new lTexture("/home/lax/Coding/GraphicEngine/src/Main/Scenes/"
                           "BasicMultipleLightsScene/Assets/container2.png");
    texture01 =
        new lTexture("/home/lax/Coding/GraphicEngine/src/Main/Scenes/"
                     "BasicMultipleLightsScene/Assets/container2_specular.png");
    texture->Bind();
    lightingShaderProgramPtr->Bind();
    lightingShaderProgramPtr->setUniform1i("material.diffuse", 0);

    texture01->Bind();
    lightingShaderProgramPtr->setUniform1i("material.specular", 1);

    VertexBufferLayout layout;
    layout.AddElement<float>(3);
    layout.AddElement<float>(3);
    layout.AddElement<float>(2);
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

    lightingShaderProgramPtr->Bind();

    lightingShaderProgramPtr->setUniform3fv("viewPos", fpsCamera.Position);
    lightingShaderProgramPtr->setUniform1f("material.shininess", 32.0f);

    // directional light
    lightingShaderProgramPtr->setUniform3f("dirLight.direction", -0.2f, -1.0f,
                                           -0.3f);
    lightingShaderProgramPtr->setUniform3f("dirLight.ambient", 0.05f, 0.05,
                                           0.05f);
    lightingShaderProgramPtr->setUniform3f("dirLight.diffuse", 0.4f, 0.4f,
                                           0.4f);
    lightingShaderProgramPtr->setUniform3f("dirLight.specular", 0.5f, 0.5f,
                                           0.5f);

    // point light 1
    lightingShaderProgramPtr->setUniform3fv("pointLights[0].position",
                                            pointLightPositions[0]);
    lightingShaderProgramPtr->setUniform3f("pointLights[0].ambient", 0.05f,
                                           0.05f, 0.05f);
    lightingShaderProgramPtr->setUniform3f("pointLights[0].diffuse", 0.8f, 0.8f,
                                           0.8f);
    lightingShaderProgramPtr->setUniform3f("pointLights[0].specular", 1.0f,
                                           1.0f, 1.0f);
    lightingShaderProgramPtr->setUniform1f("pointLights[0].constant", 1.0f);
    lightingShaderProgramPtr->setUniform1f("pointLights[0].linear", 0.09f);
    lightingShaderProgramPtr->setUniform1f("pointLights[0].quadratic", 0.032f);
    // point light 2
    lightingShaderProgramPtr->setUniform3fv("pointLights[1].position",
                                            pointLightPositions[1]);
    lightingShaderProgramPtr->setUniform3f("pointLights[1].ambient", 0.05f,
                                           0.05f, 0.05f);
    lightingShaderProgramPtr->setUniform3f("pointLights[1].diffuse", 0.8f, 0.8f,
                                           0.8f);
    lightingShaderProgramPtr->setUniform3f("pointLights[1].specular", 1.0f,
                                           1.0f, 1.0f);
    lightingShaderProgramPtr->setUniform1f("pointLights[1].constant", 1.0f);
    lightingShaderProgramPtr->setUniform1f("pointLights[1].linear", 0.09f);
    lightingShaderProgramPtr->setUniform1f("pointLights[1].quadratic", 0.032f);
    // point light 3
    lightingShaderProgramPtr->setUniform3fv("pointLights[2].position",
                                            pointLightPositions[2]);
    lightingShaderProgramPtr->setUniform3f("pointLights[2].ambient", 0.05f,
                                           0.05f, 0.05f);
    lightingShaderProgramPtr->setUniform3f("pointLights[2].diffuse", 0.8f, 0.8f,
                                           0.8f);
    lightingShaderProgramPtr->setUniform3f("pointLights[2].specular", 1.0f,
                                           1.0f, 1.0f);
    lightingShaderProgramPtr->setUniform1f("pointLights[2].constant", 1.0f);
    lightingShaderProgramPtr->setUniform1f("pointLights[2].linear", 0.09f);
    lightingShaderProgramPtr->setUniform1f("pointLights[2].quadratic", 0.032f);
    // point light 4
    lightingShaderProgramPtr->setUniform3fv("pointLights[3].position",
                                            pointLightPositions[3]);
    lightingShaderProgramPtr->setUniform3f("pointLights[3].ambient", 0.05f,
                                           0.05f, 0.05f);
    lightingShaderProgramPtr->setUniform3f("pointLights[3].diffuse", 0.8f, 0.8f,
                                           0.8f);
    lightingShaderProgramPtr->setUniform3f("pointLights[3].specular", 1.0f,
                                           1.0f, 1.0f);
    lightingShaderProgramPtr->setUniform1f("pointLights[3].constant", 1.0f);
    lightingShaderProgramPtr->setUniform1f("pointLights[3].linear", 0.09f);
    lightingShaderProgramPtr->setUniform1f("pointLights[3].quadratic", 0.032f);

    // spotlight
    lightingShaderProgramPtr->setUniform3fv("spotlight.position",
                                            fpsCamera.Position);
    lightingShaderProgramPtr->setUniform3fv("spotlight.direction",
                                            fpsCamera.Front);
    lightingShaderProgramPtr->setUniform3f("spotlight.ambient", 0.0f, 0.0f,
                                           0.0f);
    lightingShaderProgramPtr->setUniform3f("spotlight.diffuse", 1.0f, 1.0f,
                                           1.0f);
    lightingShaderProgramPtr->setUniform3f("spotlight.specular", 1.0f, 1.0f,
                                           1.0f);
    lightingShaderProgramPtr->setUniform1f("spotlight.constant", 1.0f);
    lightingShaderProgramPtr->setUniform1f("spotlight.linear", 0.09f);
    lightingShaderProgramPtr->setUniform1f("spotlight.quadratic", 0.032f);
    lightingShaderProgramPtr->setUniform1f("spotlight.cutOff",
                                           glm::cos(glm::radians(12.5f)));
    lightingShaderProgramPtr->setUniform1f("spotlight.outerCutOff",
                                           glm::cos(glm::radians(15.0f)));

    // view projection
    glm::mat4 view = fpsCamera.GetViewMatrix();
    lightingShaderProgramPtr->setUniformMatrix4fv("projection", proj);
    lightingShaderProgramPtr->setUniformMatrix4fv("view", view);

    glm::mat4 model = glm::mat4(1.0f);
    lightingShaderProgramPtr->setUniformMatrix4fv("model", model);

    texture->Bind(0);
    texture01->Bind(1);

    for (unsigned int i = 0; i < cubePositions.size(); i++) {
      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, cubePositions[i]);

      float angle = 20.0f * i;
      model =
          glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
      lightingShaderProgramPtr->setUniformMatrix4fv("model", model);

      renderer.draw(*cubeVAOPtr, *lightingShaderProgramPtr, 36);
    }

    lampsShaderProgramPtr->Bind();
    lampsShaderProgramPtr->setUniformMatrix4fv("projection", proj);
    lampsShaderProgramPtr->setUniformMatrix4fv("view", view);

    for (unsigned int i = 0; i < 4; i++) {
      model = glm::mat4(1.0f);
      model = glm::translate(model, pointLightPositions[i]);
      model = glm::scale(model, glm::vec3(0.2f));
      lampsShaderProgramPtr->setUniformMatrix4fv("model", model);

      renderer.draw(*cubeVAOPtr, *lampsShaderProgramPtr, 36);
    }
  }

  void ImGuiLayer() override {
    /*
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

      ImGui::End();
    }
    */
  }

  void OnResize(float aspectRatio) override {
    proj = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);
    lightingShaderProgramPtr->setUniformMatrix4fv("proj", proj);
  }

  void HandleInput(GLFWwindow *window) override {
    if (isCursorHidden) {
      if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        fpsCamera.ProcessKeyboard(FORWARD, deltaTime);
      if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        fpsCamera.ProcessKeyboard(BACKWARD, deltaTime);
      if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        fpsCamera.ProcessKeyboard(LEFT, deltaTime);
      if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        fpsCamera.ProcessKeyboard(RIGHT, deltaTime);
    }
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
  std::vector<glm::vec3> pointLightPositions = {
      glm::vec3(0.7f, 0.2f, 2.0f), glm::vec3(2.3f, -3.3f, -4.0f),
      glm::vec3(-4.0f, 2.0f, -12.0f), glm::vec3(0.0f, 0.0f, -3.0f)};

  std::vector<glm::vec3> cubePositions = {
      glm::vec3(0.0f, 0.0f, 0.0f),    glm::vec3(2.0f, 5.0f, -15.0f),
      glm::vec3(-1.5f, -2.2f, -2.5f), glm::vec3(-3.8f, -2.0f, -12.3f),
      glm::vec3(2.4f, -0.4f, -3.5f),  glm::vec3(-1.7f, 3.0f, -7.5f),
      glm::vec3(1.3f, -2.0f, -2.5f),  glm::vec3(1.5f, 2.0f, -2.5f),
      glm::vec3(1.5f, 0.2f, -1.5f),   glm::vec3(-1.3f, 1.0f, -1.5f)};

  std::vector<float> cubeVertices = {
      -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, 0.0f,  0.0f,  0.5f,  -0.5f,
      -0.5f, 0.0f,  0.0f,  -1.0f, 1.0f,  0.0f,  0.5f,  0.5f,  -0.5f, 0.0f,
      0.0f,  -1.0f, 1.0f,  1.0f,  0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f,
      1.0f,  1.0f,  -0.5f, 0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f, 0.0f,  1.0f,
      -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, 0.0f,  0.0f,

      -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.5f,  -0.5f,
      0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,  0.5f,  0.5f,  0.5f,  0.0f,
      0.0f,  1.0f,  1.0f,  1.0f,  0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
      1.0f,  1.0f,  -0.5f, 0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
      -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

      -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f,  1.0f,  0.0f,  -0.5f, 0.5f,
      -0.5f, -1.0f, 0.0f,  0.0f,  1.0f,  1.0f,  -0.5f, -0.5f, -0.5f, -1.0f,
      0.0f,  0.0f,  0.0f,  1.0f,  -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f,
      0.0f,  1.0f,  -0.5f, -0.5f, 0.5f,  -1.0f, 0.0f,  0.0f,  0.0f,  0.0f,
      -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f,  1.0f,  0.0f,

      0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.5f,  0.5f,
      -0.5f, 1.0f,  0.0f,  0.0f,  1.0f,  1.0f,  0.5f,  -0.5f, -0.5f, 1.0f,
      0.0f,  0.0f,  0.0f,  1.0f,  0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,  0.0f,
      0.0f,  1.0f,  0.5f,  -0.5f, 0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
      0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

      -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,  0.0f,  1.0f,  0.5f,  -0.5f,
      -0.5f, 0.0f,  -1.0f, 0.0f,  1.0f,  1.0f,  0.5f,  -0.5f, 0.5f,  0.0f,
      -1.0f, 0.0f,  1.0f,  0.0f,  0.5f,  -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,
      1.0f,  0.0f,  -0.5f, -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,  0.0f,  0.0f,
      -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,  0.0f,  1.0f,

      -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.5f,  0.5f,
      -0.5f, 0.0f,  1.0f,  0.0f,  1.0f,  1.0f,  0.5f,  0.5f,  0.5f,  0.0f,
      1.0f,  0.0f,  1.0f,  0.0f,  0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
      1.0f,  0.0f,  -0.5f, 0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
      -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,  0.0f,  1.0f};
};
