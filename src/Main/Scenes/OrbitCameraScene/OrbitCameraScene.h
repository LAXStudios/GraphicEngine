#include "../../../Headers/Core/Common/Common.h"
#include "Headers/Core/Camera/OrbitCamera.h"
#include "Headers/Core/ShaderProgram/ShaderProgram.h"
#include "Headers/Core/TextureManager/TextureManager.h"
#include "Headers/Core/VertexArray/VertexArray.h"
#include "Headers/Core/VertexBuffer/VertexBuffer.h"
#include "Headers/Core/VertexBufferLayout/VertexBufferLayout.h"
#include "Headers/Scene/Scene.h"
#include "imgui_internal.h"
#include <GLFW/glfw3.h>
#include <glm/ext/vector_float3.hpp>
#include <vector>

class OrbitCameraScene : public Scene {
private:
  ShaderProgram *shaderProgramPtr = nullptr;
  VertexArray *cubeVAO = nullptr;
  VertexBuffer *cubeVBO = nullptr;
  VertexArray *gridVAO = nullptr;
  VertexBuffer *gridVBO = nullptr;

  unsigned int texture;

  OrbitCamera camera;

  float deltaTime = 0.0f;
  float lastMouseX = 0.0f;
  float lastMouseY = 0.0f;
  float aspectRatio = 1280.0f / 800.0f;

  bool firstMouse = true;

  int gridVertexCount = 0;

  bool isOrbiting = false;
  bool isPanning = false;

public:
  OrbitCameraScene(const std::string &name) : Scene(name) {
    camera.target = glm::vec3(0.0f);
    camera.azimuth = 45.0f;
    camera.elevation = 30.0f;
    camera.distance = 15.0f;
  }

  void InitScene(GLFWwindow *window) override {
    shaderProgramPtr = new ShaderProgram(
        programPath("Main/Scenes/OrbitCameraScene/Shaders/shader.glsl"));

    setupCubeGeometry();
    setupGrid(20, 1.0f);

    texture = TextureManager::Get().LoadTexture(
        programPath("Main/Scenes/OrbitCameraScene/Assets/soka_blue_cutie.png"));

    shaderProgramPtr->Bind();
    shaderProgramPtr->setUniform1i("texture0", 0);

    glEnable(GL_DEPTH_TEST);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  }

  void Render() override {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 proj =
        glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 1000.0f);

    shaderProgramPtr->Bind();
    shaderProgramPtr->setUniformMatrix4fv("view", view);
    shaderProgramPtr->setUniformMatrix4fv("proj", proj);

    bindTexture(texture, 0);

    for (int x = -2; x <= 2; x++) {
      for (int z = -2; z <= 2; z++) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f),
                                         glm::vec3(x * 2.0f, 0.5f, z * 2.0f));
        shaderProgramPtr->setUniformMatrix4fv("model", model);
        cubeVAO->Bind();
        glDrawArrays(GL_TRIANGLES, 0, 36);
        cubeVAO->UnBind();
      }
    }

    // Grid
    glm::mat4 model = glm::mat4(1.0f);
    shaderProgramPtr->setUniformMatrix4fv("model", model);
    gridVAO->Bind();
    glDrawArrays(GL_LINES, 0, gridVertexCount);
    gridVAO->UnBind();
  }

  void Update(float dt) override { deltaTime = dt; }

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
      camera.elevation = 30.0f;
      camera.distance = 15.0f;
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

  void ImGuiLayer() override {

    ImGui::Begin("Orbit Camera", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    glm::vec3 pos = camera.GetPosition();
    ImGui::Text("Kamera Position:  %.2f  %.2f  %.2f", pos.x, pos.y, pos.z);
    ImGui::Text("Target:           %.2f  %.2f  %.2f", camera.target.x,
                camera.target.y, camera.target.z);
    ImGui::Text("Azimuth:  %.1f°", camera.azimuth);
    ImGui::Text("Elevation: %.1f°", camera.elevation);
    ImGui::Text("Distance:  %.2f", camera.distance);

    ImGui::Separator();

    ImGui::SliderFloat("Orbit Sensitivity", &camera.orbitSensitivity, 0.1f,
                       2.0f);
    ImGui::SliderFloat("Pan Sensitivity", &camera.panSensitivity, 0.001f,
                       0.05f);
    ImGui::SliderFloat("Zoom Sensitivity", &camera.zoomSensitivity, 0.1f, 2.0f);

    ImGui::Separator();

    if (ImGui::Button("Front (Num 1)"))
      camera.SetFrontView();
    ImGui::SameLine();
    if (ImGui::Button("Right (Num 3)"))
      camera.SetRightView();
    ImGui::SameLine();
    if (ImGui::Button("Top (Num 7)"))
      camera.SetTopView();
    if (ImGui::Button("Opposite (Num 9)"))
      camera.SetOppositeView();

    ImGui::Separator();
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "MMB:       Orbit");
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Shift+MMB: Pan");
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Scroll:    Zoom");
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "R:         Reset");
    ImGui::End();
  }

  ~OrbitCameraScene() {
    delete shaderProgramPtr;
    delete cubeVAO;
    delete cubeVBO;
    delete gridVAO;
    delete gridVBO;
  }

private:
  void setupCubeGeometry() {
    cubeVAO = new VertexArray();
    cubeVBO = new VertexBuffer(
        cubeVerts.data(),
        static_cast<unsigned int>(cubeVerts.size() * sizeof(float)));
    cubeVAO->Bind();
    cubeVBO->Bind();

    VertexBufferLayout layout;
    layout.AddElement<float>(3);
    layout.AddElement<float>(2);
    cubeVAO->AddBuffer(*cubeVBO, layout);
    cubeVAO->UnBind();
    cubeVBO->UnBind();
  }

  void setupGrid(int halfsize, float step) {
    std::vector<float> verts;
    float limit = halfsize * step;

    for (int i = -halfsize; i <= halfsize; i++) {
      float pos = i * step;

      verts.insert(verts.end(), {-limit, 0.0f, pos, 0.0f, 0.0f});
      verts.insert(verts.end(), {limit, 0.0f, pos, 1.0f, 0.0f});

      verts.insert(verts.end(), {pos, 0.0f, -limit, 0.0f, 0.0f});
      verts.insert(verts.end(), {pos, 0.0f, limit, 0.0f, 1.0f});
    }

    gridVertexCount = static_cast<int>(verts.size() / 5);

    gridVAO = new VertexArray();
    gridVBO = new VertexBuffer(
        verts.data(), static_cast<unsigned int>(verts.size() * sizeof(float)));
    gridVAO->Bind();
    gridVBO->Bind();
    VertexBufferLayout layout;
    layout.AddElement<float>(3);
    layout.AddElement<float>(2);
    gridVAO->AddBuffer(*gridVBO, layout);
    gridVAO->UnBind();
    gridVBO->UnBind();
  }

  const std::vector<float> cubeVerts = {
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
      -0.5f, 0.5f,  0.5f,  0.0f, 0.0f, -0.5f, 0.5f,  -0.5f, 0.0f, 1.0f,
  };
};
