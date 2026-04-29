#include "../../../Headers/Core/Common/Common.h"
#include "Headers/Core/TextureManager/TextureManager.h"
#include <GL/gl.h>
#include <GL/glext.h>
#include <glm/detail/qualifier.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/trigonometric.hpp>
#include <vector>

class CatCubes3DScene : public Scene {
private:
  ShaderProgram *shaderProgramPtr = nullptr;

  VertexArray *vertexArrayPtr = nullptr;
  VertexBuffer *vertexBufferPtr = nullptr;

  unsigned int texture;

  // glm::mat4 model;
  glm::mat4 view;
  glm::mat4 proj;

public:
  CatCubes3DScene(const std::string &name) : Scene(name) {}

  void InitScene() override {
    shaderProgramPtr =
        new ShaderProgram(programPath("Main/Scenes/"
                                      "CatCubes3DScene/Shaders/shader.glsl"));
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
                    "CatCubes3DScene/Assets/soka_blue_cutie.png"));
    bindTexture(texture, 0);

    shaderProgramPtr->Bind();
    shaderProgramPtr->setUniform1i("texture0", 0);
    proj =
        glm::perspective(glm::radians(45.0f), 1280.0f / 800.0f, 0.1f, 100.0f);
    shaderProgramPtr->setUniformMatrix4fv("proj", proj);
  }

  void HandleInput(GLFWwindow *window) override {}

  void Update(float dt) override {
    // model = glm::mat4(1.0f);
    // view = glm::mat4(1.0f);

    // model = glm::rotate(model, dt, glm::vec3(0.5f, 1.0f, 0.0f));

    // view = glm::translate(view, glm::vec3(0.0f, 0.0f, -5.0f));

    view = glm::mat4(1.0f);
    float radius = 10.0f;
    float speed = 2.0f;
    float camX = static_cast<float>(sin(dt * speed) * radius);
    float camZ = static_cast<float>(cos(dt * speed) * radius);

    glm::vec3 camPos = glm::vec3(camX, 0.0f, camZ);

    view = glm::lookAt(camPos, glm::vec3(0.0f, 0.0f, 0.0f),
                       glm::vec3(0.0f, 1.0f, 0.0f));
  }

  void OnResize(float aspectRatio) override {
    proj = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);
    shaderProgramPtr->setUniformMatrix4fv("proj", proj);
  }

  void Render() override {
    RendererWrapper renderer{};
    /*
    shaderProgramPtr->Bind();
    shaderProgramPtr->setUniformMatrix4fv("model", model);
    shaderProgramPtr->setUniformMatrix4fv("view", view);
    shaderProgramPtr->setUniformMatrix4fv("proj", proj);

    texture->Bind();
    renderer.draw(*vertexArrayPtr, *shaderProgramPtr, 36);

    */

    shaderProgramPtr->setUniformMatrix4fv("view", view);

    for (unsigned int i = 0; i < 10; i++) {
      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, cubePositions[i]);
      float angle = 20.0f * i;
      model =
          glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
      shaderProgramPtr->Bind();
      shaderProgramPtr->setUniformMatrix4fv("model", model);
      //  shaderProgramPtr->setUniformMatrix4fv("proj", proj);

      bindTexture(texture, 0);
      renderer.draw(*vertexArrayPtr, *shaderProgramPtr, 36);
    }
  }

  ~CatCubes3DScene() {}

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
