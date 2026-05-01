#include "../../../Headers/Core/Common/Common.h"
#include "../../../Headers/Core/TextureManager/TextureManager.h"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/trigonometric.hpp>
#include <string>
#include <vector>

class InstancedScene : public Scene {
private:
  ShaderProgram *shaderProgramPtr = nullptr;
  VertexArray *vaoPtr = nullptr;
  VertexBuffer *vboPtr = nullptr;

  unsigned int instanceVBO = 0;
  unsigned int instanceCount = 0;

  unsigned int texture;

  float deltaTime = 0.0f;
  float lastX = 0.0f;
  float lastY = 0.0f;
  float aspectRatio = 0.0f;

  bool firstMouse = true;
  bool isCursorHidden = true;

  glm::mat4 view;
  glm::mat4 proj;

  FPSCamera camera{glm::vec3(0.0f, 5.0f, 15.0f)};

  // Grid Config
  bool isConnected = true;
  int gridW = 50; // width
  int gridH = 50;
  int gridD = 50; // depth
  float spacing = 1.1f;

public:
  InstancedScene(const std::string &name) : Scene(name) {}

  void InitScene(GLFWwindow *window) override {
    shaderProgramPtr = new ShaderProgram(
        programPath("Main/Scenes/InstancedScene/Shaders/instanced.glsl"));

    vaoPtr = new VertexArray();
    vboPtr = new VertexBuffer(
        cubeVertices.data(),
        static_cast<unsigned int>(cubeVertices.size() * sizeof(float)));

    vaoPtr->Bind();
    vboPtr->Bind();

    VertexBufferLayout layout;
    layout.AddElement<float>(3); // pos
    layout.AddElement<float>(2); // texcoord
    vaoPtr->AddBuffer(*vboPtr, layout);

    vaoPtr->Bind();

    rebuildInstancBuffer();

    vaoPtr->UnBind();

    texture = TextureManager::Get().LoadTexture(
        programPath("Main/Scenes/InstancedScene/Assets/soka_blue_cutie.png"));

    shaderProgramPtr->Bind();
    shaderProgramPtr->setUniform1i("texture0", 0);

    glCall(glEnable(GL_DEPTH_TEST));

    if (glfwRawMouseMotionSupported())
      glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  }

  void Render() override {
    view = camera.GetViewMatrix();
    proj =
        glm::perspective(glm::radians(camera.Zoom), aspectRatio, 0.1f, 500.0f);

    shaderProgramPtr->Bind();
    shaderProgramPtr->setUniformMatrix4fv("view", view);
    shaderProgramPtr->setUniformMatrix4fv("proj", proj);

    bindTexture(texture, 0);

    vaoPtr->Bind();
    glCall(glDrawArraysInstanced(GL_TRIANGLES, 0, 36, instanceCount));
    vaoPtr->UnBind();
  }

  void Update(float dt) override { deltaTime = dt; }

  void OnResize(float ar) override { aspectRatio = ar; }

  void HandleInput(GLFWwindow *window) override {
    if (!isCursorHidden)
      return;
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
                   int mods) override {
    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
      isCursorHidden = !isCursorHidden;
      glfwSetInputMode(window, GLFW_CURSOR,
                       isCursorHidden ? GLFW_CURSOR_DISABLED
                                      : GLFW_CURSOR_NORMAL);
    }
  }

  void HandleMouseInput(GLFWwindow *window, double xpos, double ypos) override {
    if (!isCursorHidden)
      return;
    float x = static_cast<float>(xpos);
    float y = static_cast<float>(ypos);
    if (firstMouse) {
      lastX = x;
      lastY = y;
      firstMouse = false;
    }
    camera.ProcessMouseMovement(x - lastX, lastY - y);
    lastX = x;
    lastY = y;
  }

  void ImGuiLayer() override {
    ImGui::Begin("Instanced Rendering");

    ImGui::Text("Instances: %u", instanceCount);
    ImGui::Text("FPS:: %0.1f", ImGui::GetIO().Framerate);
    ImGui::Separator();

    ImGui::Checkbox("Height & Width Connected", &isConnected);

    bool rebuild = false;

    if (isConnected) {
      rebuild |= ImGui::SliderInt("Grid Width", &gridW, 1, 200);
      gridH = gridW;
      rebuild |= ImGui::SliderInt("Grid Depth", &gridD, 1, 200);
      rebuild |= ImGui::SliderFloat("Spacing", &spacing, 0.5f, 3.0f);
    } else {

      rebuild |= ImGui::SliderInt("Grid Width", &gridW, 1, 200);
      rebuild |= ImGui::SliderInt("Grid height", &gridH, 1, 200);
      rebuild |= ImGui::SliderInt("Grid Depth", &gridD, 1, 200);
      rebuild |= ImGui::SliderFloat("Spacing", &spacing, 0.5f, 3.0f);
    }

    if (rebuild) {
      vaoPtr->Bind();
      rebuildInstancBuffer();
      vaoPtr->UnBind();
    }

    ImGui::Separator();
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "TAB: Camera controll");
    ImGui::End();
  }

  ~InstancedScene() {
    delete shaderProgramPtr;
    delete vaoPtr;
    delete vboPtr;
    if (instanceVBO)
      glCall(glDeleteBuffers(1, &instanceVBO));
  }

private:
  void rebuildInstancBuffer() {
    if (instanceVBO) {
      glDeleteBuffers(1, &instanceVBO);
      instanceVBO = 0;
    }

    std::vector<glm::mat4> matrices;
    matrices.reserve(gridW * gridD);

    // 2D
    /*
    for (int x = 0; x < gridW; x++) {
      for (int z = 0; z < gridD; z++) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(
            model, glm::vec3(x * spacing - (gridW * spacing * 0.5f), 0.0f,
                             z * spacing - (gridD * spacing * 0.05)));
        matrices.push_back(model);
      }
    }
    */

    // 3D
    for (int x = 0; x < gridW; x++) {
      for (int y = 0; y < gridH; y++) {
        for (int z = 0; z < gridD; z++) {
          glm::mat4 model = glm::mat4(1.0f);
          model = glm::translate(
              model, glm::vec3(x * spacing - (gridW * spacing * 0.5f),
                               y * spacing - (gridH * spacing * 0.5f),
                               z * spacing - (gridD * spacing * 0.5f)));
          matrices.push_back(model);
        }
      }
    }

    instanceCount = static_cast<unsigned int>(matrices.size());

    glCall(glGenBuffers(1, &instanceVBO));
    glCall(glBindBuffer(GL_ARRAY_BUFFER, instanceVBO));
    glCall(glBufferData(GL_ARRAY_BUFFER, matrices.size() * sizeof(glm::mat4),
                        matrices.data(), GL_DYNAMIC_DRAW));

    for (int i = 0; i < 4; i++) {
      glCall(glEnableVertexAttribArray(2 + i));
      glCall(glVertexAttribPointer(2 + i, 4, GL_FLOAT, GL_FALSE,
                                   sizeof(glm::mat4),
                                   (void *)(i * sizeof(glm::vec4))));

      glCall(glVertexAttribDivisor(2 + i, 1));
    }

    glCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
  }

  const std::vector<float> cubeVertices = {
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
