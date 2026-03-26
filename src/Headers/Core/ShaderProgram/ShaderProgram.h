#pragma once
// #include "../Common/Common.h"
#include "GL/glew.h"
#include <alloca.h>
#include <glm/ext/matrix_float4x4.hpp>
#include <string>

struct ShaderSource {
  std::string vertexShader;
  std::string fragmentShader;
};

class ShaderProgram {
private:
  std::string shaderPath;
  unsigned int ID;

  ShaderSource LoadShaderFromSourceFile(const std::string &shaderPath);

  unsigned int CompileShader(const std::string &source, unsigned int type);

  unsigned int LinkShaderToProgram(ShaderSource shaderSource);

public:
  ShaderProgram(std::string shaderPath);
  ~ShaderProgram();

  void Bind() const;
  void UnBind() const;

  unsigned int GetID() const;
  unsigned int GetProgram() const;

  void PrintProgramInfoLog(int programID);
  void PrintShaderInfoLog(int shaderID, GLenum type);

  unsigned int getUniformLocation(const std::string &uniformName);

  void setUniform1i(const std::string &uniformName, int value);
  void setUniform1f(const std::string &uniformName, float value);
  void setUniform3f(const std::string &uniformName, float v1, float v2,
                    float v3);
  void setUniformMatrix4fv(const std::string &uniformName, glm::mat4 matrix);
  void setUniform3fv(const std::string &uniformName, glm::vec3 vector);
};
