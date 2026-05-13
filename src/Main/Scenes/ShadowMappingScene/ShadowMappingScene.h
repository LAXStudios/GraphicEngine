#pragma once

#include "../../../Headers/Core/Common/Common.h"
#include "Headers/Core/Camera/OrbitCamera.h"
#include "Headers/Core/Common/ErrorHandling.h"
#include "Headers/Core/ShaderProgram/ShaderProgram.h"
#include "Headers/Core/TextureManager/TextureManager.h"
#include "Headers/Core/VertexArray/VertexArray.h"
#include "Headers/Core/VertexBuffer/VertexBuffer.h"
#include "Headers/Scene/Scene.h"
#include <GLFW/glfw3.h>
#include <cmath>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/trigonometric.hpp>
#include <vector>

class ShadowMappingScene : public Scene {
private:
  ShaderProgram *depthShaderPtr = nullptr;
  ShaderProgram *sceneShaderPtr = nullptr;

  VertexArray *floorVAO = nullptr;
  VertexBuffer *floorVBO = nullptr;
  VertexArray *cubeVAO = nullptr;
  VertexBuffer *cubeVBO = nullptr;

  unsigned int woodTexture;

  unsigned depthMapFBO = 0;
  unsigned depthMap = 0;
  const unsigned int SHADOW_W = 2048;
  const unsigned int SHADOW_H = 2048;

  // Licht
  glm::vec3 lightPos = glm::vec3(-2.0f, 8.0f, -1.0f);

  OrbitCamera camera;
  float deltaTime = 0.0f;
  float lastMouseX, lastMouseY = 0.0f;
  bool firstMouse = true;
  bool cursorHidden = true;
  float aspectRatio = 1280.0f / 800.0f;

  bool isOrbiting;
  bool isPanning;

  // Debug
  bool showDepthMap = false;
  float lightAngle = 0.0f;
  bool animateLight = true;

public:
  ShadowMappingScene(const std::string &name, const std::string &category)
      : Scene(name, category) {
    camera.target = glm::vec3(-2.0f, 0.0f, -5.0f);
    camera.azimuth = 35.0f;
    camera.elevation = 30.0f;
    camera.distance = 15.0f;
  }

  void InitScene(GLFWwindow *window) override {
    depthShaderPtr = new ShaderProgram(programPath(
        "Main/Scenes/ShadowMappingScene/Shaders/depth_shader.glsl"));
    sceneShaderPtr = new ShaderProgram(programPath(
        "Main/Scenes/ShadowMappingScene/Shaders/scene_shadow.glsl"));

    setupGeometry();
    setupShadowMap();

    woodTexture = TextureManager::Get().LoadTexture(
        programPath("Main/Scenes/CatCubes3DScene/Assets/soka_blue_cutie.png"));

    sceneShaderPtr->Bind();
    sceneShaderPtr->setUniform1i("diffuseTexture", 0);
    sceneShaderPtr->setUniform1i("shadowMap", 1);

    glCall(glEnable(GL_DEPTH_TEST));

    // if (glfwRawMouseMotionSupported())
    //   glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  }

  void Render() override {
    glm::mat4 lightSpaceMatrix = computeLightSpaceMatrix();

    glCall(glViewport(0, 0, SHADOW_W, SHADOW_H));
    glCall(glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO));
    glCall(glClear(GL_DEPTH_BUFFER_BIT));

    depthShaderPtr->Bind();
    depthShaderPtr->setUniformMatrix4fv("lightSpaceMatrix", lightSpaceMatrix);

    renderScene(depthShaderPtr);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 proj =
        glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);

    sceneShaderPtr->Bind();
    sceneShaderPtr->setUniformMatrix4fv("view", view);
    sceneShaderPtr->setUniformMatrix4fv("proj", proj);
    sceneShaderPtr->setUniformMatrix4fv("lightSpaceMatrix", lightSpaceMatrix);
    sceneShaderPtr->setUniform3fv("lightPos", lightPos);
    sceneShaderPtr->setUniform3fv("viewPos", camera.GetPosition());

    bindTexture(woodTexture, 0);

    glCall(glActiveTexture(GL_TEXTURE1));
    glCall(glBindTexture(GL_TEXTURE_2D, depthMap));

    renderScene(sceneShaderPtr);
  }

  void Update(float dt) override {
    deltaTime = dt;
    if (animateLight) {
      lightAngle += dt * 0.5f;
      lightPos.x = sin(lightAngle) * 8.0f;
      lightPos.z = cos(lightAngle) * 8.0f;
    }
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

  void HandleScrollInput(GLFWwindow *window, double xoffset,
                         double yoffset) override {
    camera.Zoom(static_cast<float>(yoffset));
  }

  void OnResize(float ar) override { aspectRatio = ar; }

  void ImGuiLayer() override {
    ImGui::Begin("Shadow Mapping", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Text("Light-Position: %.1f, %.1f, %.1f", lightPos.x, lightPos.y,
                lightPos.z);
    ImGui::Checkbox("Animate light", &animateLight);
    if (!animateLight) {
      ImGui::SliderFloat("light X", &lightPos.x, -20.0f, 20.0f);
      ImGui::SliderFloat("light Y", &lightPos.y, 1.0f, 20.0f);
      ImGui::SliderFloat("light Z", &lightPos.z, -20.0f, 20.0f);
    }
    ImGui::Separator();
    ImGui::Text("Shadow Map Resolution: %dx%d", SHADOW_W, SHADOW_H);
    ImGui::End();
  }

  ~ShadowMappingScene() {
    delete depthShaderPtr;
    delete sceneShaderPtr;
    delete floorVAO;
    delete floorVBO;
    delete cubeVAO;
    delete cubeVBO;
    if (depthMapFBO)
      glCall(glDeleteFramebuffers(1, &depthMapFBO));
    if (depthMap)
      glCall(glDeleteTextures(1, &depthMap));
  }

private:
  void setupGeometry() {
    // Boden (großes Quad mit Normals)
    float floorVerts[] = {
        // pos                  normal           texcoord
        -10.f, 0.f,  -10.f, 0.f,   1.f,   0.f,  0.0f, 10.0f, 10.f,  0.f,
        10.f,  0.f,  1.f,   0.f,   10.0f, 0.0f, 10.f, 0.f,   -10.f, 0.f,
        1.f,   0.f,  10.0f, 10.0f, 10.f,  0.f,  10.f, 0.f,   1.f,   0.f,
        10.0f, 0.0f, -10.f, 0.f,   -10.f, 0.f,  1.f,  0.f,   0.0f,  10.0f,
        -10.f, 0.f,  10.f,  0.f,   1.f,   0.f,  0.0f, 0.0f,
    };
    floorVAO = new VertexArray();
    floorVBO = new VertexBuffer(floorVerts, sizeof(floorVerts));
    floorVAO->Bind();
    floorVBO->Bind();
    VertexBufferLayout floorLayout;
    floorLayout.AddElement<float>(3); // pos
    floorLayout.AddElement<float>(3); // normal
    floorLayout.AddElement<float>(2); // tex
    floorVAO->AddBuffer(*floorVBO, floorLayout);
    floorVAO->UnBind();
    floorVBO->UnBind();

    // Würfel mit Normals (vereinfacht, nur eine Fläche als Beispiel —
    // vollständige Vertex-Daten wie in BasicDiffuseMapScene verwenden)
    cubeVAO = new VertexArray();
    cubeVBO = new VertexBuffer(
        cubeWithNormals.data(),
        static_cast<GLuint>(cubeWithNormals.size() * sizeof(float)));
    cubeVAO->Bind();
    cubeVBO->Bind();
    VertexBufferLayout cubeLayout;
    cubeLayout.AddElement<float>(3); // pos
    cubeLayout.AddElement<float>(3); // normal
    cubeLayout.AddElement<float>(2); // tex
    cubeVAO->AddBuffer(*cubeVBO, cubeLayout);
    cubeVAO->UnBind();
    cubeVBO->UnBind();
  }

  void setupShadowMap() {
    glCall(glGenFramebuffers(1, &depthMapFBO));

    glCall(glGenTextures(1, &depthMap));
    glCall(glBindTexture(GL_TEXTURE_2D, depthMap));
    glCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_W,
                        SHADOW_H, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr));

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float border[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                           depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  glm::mat4 computeLightSpaceMatrix() {
    glm::mat4 lightProj = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, 0.1f, 50.0f);
    glm::mat4 lightView =
        glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    return lightProj * lightView;
  }

  void renderScene(ShaderProgram *shader) {
    glm::mat4 model = glm::mat4(1.0f);
    shader->setUniformMatrix4fv("model", model);
    floorVAO->Bind();
    glCall(glDrawArrays(GL_TRIANGLES, 0, 6));
    floorVAO->UnBind();

    // Würfel 1
    model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    shader->setUniformMatrix4fv("model", model);
    cubeVAO->Bind();
    glCall(glDrawArrays(GL_TRIANGLES, 0, 36));
    cubeVAO->UnBind();

    // Würfel 2 (gedreht)
    model = glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.5f, 1.0f));
    model =
        glm::rotate(model, glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    shader->setUniformMatrix4fv("model", model);
    cubeVAO->Bind();
    glCall(glDrawArrays(GL_TRIANGLES, 0, 36));
    cubeVAO->UnBind();

    // Würfel 3
    model = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 1.5f, -1.0f));
    shader->setUniformMatrix4fv("model", model);
    cubeVAO->Bind();
    glCall(glDrawArrays(GL_TRIANGLES, 0, 36));
    cubeVAO->UnBind();
  }

  const std::vector<float> cubeWithNormals = {
      -0.5f, -0.5f, -0.5f, 0.f,   0.f,   -1.f,  0.0f,  0.0f,  0.5f,  -0.5f,
      -0.5f, 0.f,   0.f,   -1.f,  1.0f,  0.0f,  0.5f,  0.5f,  -0.5f, 0.f,
      0.f,   -1.f,  1.0f,  1.0f,  0.5f,  0.5f,  -0.5f, 0.f,   0.f,   -1.f,
      1.0f,  1.0f,  -0.5f, 0.5f,  -0.5f, 0.f,   0.f,   -1.f,  0.0f,  1.0f,
      -0.5f, -0.5f, -0.5f, 0.f,   0.f,   -1.f,  0.0f,  0.0f,

      -0.5f, -0.5f, 0.5f,  0.f,   0.f,   1.f,   0.0f,  0.0f,  0.5f,  -0.5f,
      0.5f,  0.f,   0.f,   1.f,   1.0f,  0.0f,  0.5f,  0.5f,  0.5f,  0.f,
      0.f,   1.f,   1.0f,  1.0f,  0.5f,  0.5f,  0.5f,  0.f,   0.f,   1.f,
      1.0f,  1.0f,  -0.5f, 0.5f,  0.5f,  0.f,   0.f,   1.f,   0.0f,  1.0f,
      -0.5f, -0.5f, 0.5f,  0.f,   0.f,   1.f,   0.0f,  0.0f,

      -0.5f, 0.5f,  0.5f,  -1.f,  0.f,   0.f,   1.0f,  0.0f,  -0.5f, 0.5f,
      -0.5f, -1.f,  0.f,   0.f,   1.0f,  1.0f,  -0.5f, -0.5f, -0.5f, -1.f,
      0.f,   0.f,   0.0f,  1.0f,  -0.5f, -0.5f, -0.5f, -1.f,  0.f,   0.f,
      0.0f,  1.0f,  -0.5f, -0.5f, 0.5f,  -1.f,  0.f,   0.f,   0.0f,  0.0f,
      -0.5f, 0.5f,  0.5f,  -1.f,  0.f,   0.f,   1.0f,  0.0f,

      0.5f,  0.5f,  0.5f,  1.f,   0.f,   0.f,   1.0f,  0.0f,  0.5f,  0.5f,
      -0.5f, 1.f,   0.f,   0.f,   1.0f,  1.0f,  0.5f,  -0.5f, -0.5f, 1.f,
      0.f,   0.f,   0.0f,  1.0f,  0.5f,  -0.5f, -0.5f, 1.f,   0.f,   0.f,
      0.0f,  1.0f,  0.5f,  -0.5f, 0.5f,  1.f,   0.f,   0.f,   0.0f,  0.0f,
      0.5f,  0.5f,  0.5f,  1.f,   0.f,   0.f,   1.0f,  0.0f,

      -0.5f, -0.5f, -0.5f, 0.f,   -1.f,  0.f,   0.0f,  1.0f,  0.5f,  -0.5f,
      -0.5f, 0.f,   -1.f,  0.f,   1.0f,  1.0f,  0.5f,  -0.5f, 0.5f,  0.f,
      -1.f,  0.f,   1.0f,  0.0f,  0.5f,  -0.5f, 0.5f,  0.f,   -1.f,  0.f,
      1.0f,  0.0f,  -0.5f, -0.5f, 0.5f,  0.f,   -1.f,  0.f,   0.0f,  0.0f,
      -0.5f, -0.5f, -0.5f, 0.f,   -1.f,  0.f,   0.0f,  1.0f,

      -0.5f, 0.5f,  -0.5f, 0.f,   1.f,   0.f,   0.0f,  1.0f,  0.5f,  0.5f,
      -0.5f, 0.f,   1.f,   0.f,   1.0f,  1.0f,  0.5f,  0.5f,  0.5f,  0.f,
      1.f,   0.f,   1.0f,  0.0f,  0.5f,  0.5f,  0.5f,  0.f,   1.f,   0.f,
      1.0f,  0.0f,  -0.5f, 0.5f,  0.5f,  0.f,   1.f,   0.f,   0.0f,  0.0f,
      -0.5f, 0.5f,  -0.5f, 0.f,   1.f,   0.f,   0.0f,  1.0f,
  };
};
