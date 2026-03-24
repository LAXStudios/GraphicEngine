#pragma once

#include <GL/glew.h>
#define NOTHING
#include "../../Scene/Scene.h"
#include "../../Scene/SceneManager.h"
#include "ErrorHandling.h"
#include <cstddef>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace OpenGLHelper {

struct ShaderSource {
  std::string vertexShader;
  std::string fragmentShader;
};

inline GLuint CompileShader(const GLchar *src, GLenum type) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &src, nullptr);
  glCompileShader(shader);

  // TODO: Add Error handling

  return shader;
}

inline GLuint LinkProgram(const std::vector<GLuint> &shaders) {
  GLuint program = glCreateProgram();
  for (auto s : shaders)
    glAttachShader(program, s);
  glLinkProgram(program);

  // TODO: Add Error handling

  return program;
}

inline ShaderSource LoadShaderFromSourceFile(const std::string &srcPath) {
  enum class shaderType { NONE = -1, VERTEX_SHADER = 0, FRAGMENT_SHADER = 1 };

  shaderType type = shaderType::NONE;

  std::string line;
  std::stringstream shaders[2];
  std::ifstream input(srcPath);
  if (!input.is_open()) {
    // TODO: Implement Error handling
    std::cout << "Could not open Shader Source File \n";
  }

  while (std::getline(input, line)) {
    if (line.find("#shader") != std::string::npos) {
      if (line.find("vertex") != std::string::npos) {
        type = shaderType::VERTEX_SHADER;
      } else if (line.find("fragment") != std::string::npos) {
        type = shaderType::FRAGMENT_SHADER;
      }
    } else
      shaders[(int)type] << line << "\n";
  }

  return ShaderSource{shaders[(int)shaderType::VERTEX_SHADER].str(),
                      shaders[(int)shaderType::FRAGMENT_SHADER].str()};
}

// INFO: Digga kein plan ob das der Weg ist.
class Mesh {
public:
  GLuint vao = 0, vbo = 0, ebo = 0;
  GLsizei indexCount = 0;

  Mesh(const std::vector<GLfloat> &vertices,
       const std::vector<GLuint> &indices = {}) {

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    // glCheckError();

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat),
                 vertices.data(), GL_STATIC_DRAW);

    if (!indices.empty()) {
      glGenBuffers(1, &ebo);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint),
                   indices.data(), GL_STATIC_DRAW);
      indexCount = static_cast<GLsizei>(indices.size());
    } else {
      indexCount = static_cast<GLsizei>(indices.size());
    }

    glBindVertexArray(0);
  }

  void Bind() const { glBindVertexArray(vao); }

  ~Mesh() {
    if (ebo)
      glDeleteBuffers(1, &ebo);
    if (vbo)
      glDeleteBuffers(1, &vbo);
    if (vao)
      glDeleteVertexArrays(1, &vao);
  }
};

} // namespace OpenGLHelper
