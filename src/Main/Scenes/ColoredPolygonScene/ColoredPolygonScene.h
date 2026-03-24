#pragma once
#include "../../../Headers/Core/Common/Common.h"

class ColoredPolygonScene : public Scene {

private:
  GLuint _vao = 0;
  GLuint _vbo = 0;
  GLuint _shaderProgram = 0;

  GLuint _vertexShader = 0;
  GLuint _fragmentShader = 0;

public:
  ColoredPolygonScene(const std::string &name) : Scene(name) {}

  void InitScene() override {

    const char *vertexSource = R"glsl(
    #version 150 core

    in vec2 position;
    in vec3 color;

    out vec3 Color;

    void main()
    {
      Color = color;
      gl_Position = vec4(position, 0.0, 1.0);
    }
    )glsl";

    const char *fragmentSource = R"glsl(
    #version 150 core
    
    in vec3 Color;

    out vec4 outColor;

    void main()
    {
      outColor = vec4(Color, 1.0);
    }
    )glsl";

    float verties[]{
        0.0f,  0.5f,  1.0f, 0.0f,
        0.0f, // Red
        0.5f,  -0.5f, 0.0f, 1.0f,
        0.0f, // Green
        -0.5f, -0.5f, 0.0f, 0.0f,
        1.0 // Blue
    };

    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);

    glGenBuffers(1, &_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verties), verties, GL_STATIC_DRAW);

    _vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(_vertexShader, 1, &vertexSource, NULL);
    glCompileShader(_vertexShader);

    _fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(_fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(_fragmentShader);

    GLint statusVertex;
    glGetShaderiv(_vertexShader, GL_COMPILE_STATUS, &statusVertex);
    if (!statusVertex) {
      printf("Vertex Shader Error: %d\n", statusVertex);
    }

    GLint statusFragment;
    glGetShaderiv(_fragmentShader, GL_COMPILE_STATUS, &statusVertex);
    if (!statusFragment) {
      printf("Fragment Shader Error: %d\n", statusFragment);
    }

    _shaderProgram = glCreateProgram();
    glAttachShader(_shaderProgram, _vertexShader);
    glAttachShader(_shaderProgram, _fragmentShader);
    glLinkProgram(_shaderProgram);
    glUseProgram(_shaderProgram);

    GLint posAttr = glGetAttribLocation(_shaderProgram, "position");
    glEnableVertexAttribArray(posAttr);
    glVertexAttribPointer(posAttr, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);

    GLint collAttr = glGetAttribLocation(_shaderProgram, "color");
    glEnableVertexAttribArray(collAttr);
    glVertexAttribPointer(collAttr, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          (void *)(2 * sizeof(float)));
  }

  void HandleInput(GLFWwindow *window) override {}

  void Update(float dt) override {}

  void Render() override {
    // Backgound Color: 0.129f, 0.145f, 0.160f

    glBindVertexArray(_vao);
    glUseProgram(_shaderProgram);

    glDrawArrays(GL_TRIANGLES, 0, 3);
  }

  ~ColoredPolygonScene() override {
    glDeleteProgram(_shaderProgram);
    glDeleteShader(_fragmentShader);
    glDeleteShader(_vertexShader);

    glDeleteBuffers(1, &_vbo);

    glDeleteVertexArrays(1, &_vao);
  }
};
