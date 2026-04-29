// #include "../../../Headers/Core/Common/Common.h"
#include "../../../Headers/Core/Common/ErrorHandling.h"
#include "../../../Headers/Core/Common/OpenGLHelper.h"
#include <GL/gl.h>
#include <GL/glext.h>
#include <vector>

class QuadColored : public Scene {
private:
  GLuint shaderProgram;

  GLuint vertexShader;
  GLuint fragmentShader;

  OpenGLHelper::ShaderSource shaderSource;
  OpenGLHelper::Mesh *meshPtr = nullptr;

public:
  QuadColored(const std::string &name) : Scene(name) {}

  void InitScene(GLFWwindow *window) override {
    // Shader sources

    shaderSource = OpenGLHelper::LoadShaderFromSourceFile(
        "/home/lax/Coding/GraphicEngine/src/Main/Scenes/QuadColoredScene/"
        "Shaders/shader.glsl");

    vertexShader = OpenGLHelper::CompileShader(
        shaderSource.vertexShader.c_str(), GL_VERTEX_SHADER);

    fragmentShader = OpenGLHelper::CompileShader(
        shaderSource.fragmentShader.c_str(), GL_FRAGMENT_SHADER);

    std::vector<GLfloat> vertices = {
        -0.5f, 0.5f,  1.0f, 0.0f, 0.0f, // Top-left
        0.5f,  0.5f,  0.0f, 1.0f, 0.0f, // Top-right
        0.5f,  -0.5f, 0.0f, 0.0f, 1.0f, // Bottom-right
        -0.5f, -0.5f, 1.0f, 1.0f, 1.0f  // Bottom-left
    };

    std::vector<GLuint> elements = {0, 1, 2, 2, 3, 0};

    meshPtr = new OpenGLHelper::Mesh(vertices, elements);
    meshPtr->Bind();

    shaderProgram = OpenGLHelper::LinkProgram({vertexShader, fragmentShader});

    GLint posAttr = glGetAttribLocation(shaderProgram, "position");
    glEnableVertexAttribArray(posAttr);
    glVertexAttribPointer(posAttr, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
                          0);

    GLint colAttrib = glGetAttribLocation(shaderProgram, "color");
    glEnableVertexAttribArray(colAttrib);
    glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
                          (void *)(2 * sizeof(GLfloat)));
  }

  void HandleInput(GLFWwindow *window) override {}

  void Update(float dt) override {}

  void Render() override {
    glBindVertexArray(meshPtr->vao);
    glUseProgram(shaderProgram);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
  }

  ~QuadColored() {
    glDeleteProgram(shaderProgram);
    glDeleteShader(fragmentShader);
    glDeleteShader(vertexShader);
  }
};
