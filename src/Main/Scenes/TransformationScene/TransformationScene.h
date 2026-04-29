#include "../../../../extern/soil/SOIL.h"
// #include "../../../Headers/Core/Common/Common.h"
#include "../../../Headers/Core/Common/OpenGLHelper.h"
#include <GLFW/glfw3.h>
#include <chrono>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/trigonometric.hpp>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class TransformationScene : public Scene {
private:
  GLuint tex;

  GLuint shaderProgram;

  GLuint vertexShader;
  GLuint fragmentShader;

  OpenGLHelper::ShaderSource shaderSource;
  OpenGLHelper::Mesh *mehsPtr = nullptr;

  GLint uniModel;
  glm::mat4 model;
  glm::mat4 proj;

  std::chrono::time_point<std::chrono::system_clock> tStart;

  std::vector<GLfloat> vertices = {
      //  Position      Color             Texcoords
      -0.5f, 0.5f,  1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // Top-left
      0.5f,  0.5f,  0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // Top-right
      0.5f,  -0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, // Bottom-right
      -0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f  // Bottom-left
  };

  std::vector<GLuint> elements = {0, 1, 2, 2, 3, 0};

public:
  TransformationScene(const std::string &name) : Scene(name) {}

  void InitScene(GLFWwindow *window) override {

    shaderSource = OpenGLHelper::LoadShaderFromSourceFile(
        "/home/lax/Coding/GraphicEngine/src/Main/Scenes/TransformationScene/"
        "Shaders/shader.glsl");

    vertexShader = OpenGLHelper::CompileShader(
        shaderSource.vertexShader.c_str(), GL_VERTEX_SHADER);

    fragmentShader = OpenGLHelper::CompileShader(
        shaderSource.fragmentShader.c_str(), GL_FRAGMENT_SHADER);

    mehsPtr = new OpenGLHelper::Mesh(vertices, elements);
    mehsPtr->Bind();

    shaderProgram = OpenGLHelper::LinkProgram({vertexShader, fragmentShader});

    GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat),
                          0);

    GLint colAttrib = glGetAttribLocation(shaderProgram, "color");
    glEnableVertexAttribArray(colAttrib);
    glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat),
                          (void *)(2 * sizeof(GLfloat)));

    GLint texAttrib = glGetAttribLocation(shaderProgram, "texcoord");
    glEnableVertexAttribArray(texAttrib);
    glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat),
                          (void *)(5 * sizeof(GLfloat)));

    // Load Texture
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    int width, height;
    unsigned char *image =
        SOIL_load_image("/home/lax/Coding/GraphicEngine/src/Main/Scenes/"
                        "TransformationScene/Assets/sample.png",
                        &width, &height, 0, SOIL_LOAD_RGB);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, image);
    SOIL_free_image_data(image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    uniModel = glGetUniformLocation(shaderProgram, "model");

    // Projection / Camera Angle
    glm::mat4 view =
        glm::lookAt(glm::vec3(1.2f, 1.2f, 1.2f), glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 1.0f));

    glUseProgram(shaderProgram);

    GLint uniView = glGetUniformLocation(shaderProgram, "view");
    glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

    proj = glm::perspective(glm::radians(45.0f), 1280.0f / 800.0f, 1.0f, 10.0f);
    GLint uniProj = glGetUniformLocation(shaderProgram, "proj");
    glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));
  }

  void HandleInput(GLFWwindow *window) override {}

  void Update(float dt) override {
    // BUG: das wird hier falsch berechnet, habe damals mit ner flaschen
    // deltaTime gerechnet.
    model = glm::mat4(1.0f);
    model = glm::rotate(model, dt * glm::radians(180.0f),
                        glm::vec3(0.0f, 0.0f, 1.0f));

    GLfloat s = sin(dt * 5.0f) * 0.25f + 0.75f;
    model = glm::scale(model, glm::vec3(s, s, s));
  }

  void Render() override {
    glBindVertexArray(mehsPtr->vao);
    glUseProgram(shaderProgram);

    glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
  }

  void OnResize(float aspectRatio) override {
    proj = glm::perspective(glm::radians(45.0f), aspectRatio, 1.0f, 10.0f);
    GLint uniProj = glGetUniformLocation(shaderProgram, "proj");
    glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));
  }

  ~TransformationScene() {
    glDeleteProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    glDeleteTextures(1, &tex);
  }
};
