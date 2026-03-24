#include "../../../Headers/Core/Shader/Shader.h"
#include "../../../Headers/Core/Common/OpenGLHelper.h"
#include <GL/gl.h>
#include <GL/glext.h>

Shader::Shader(const char *shaderPath) {
  OpenGLHelper::ShaderSource shaderSource;
  shaderSource = OpenGLHelper::LoadShaderFromSourceFile(shaderPath);

  GLuint vertexShader = OpenGLHelper::CompileShader(
      shaderSource.vertexShader.c_str(), GL_VERTEX_SHADER);
  GLuint fragmentShader = OpenGLHelper::CompileShader(
      shaderSource.fragmentShader.c_str(), GL_FRAGMENT_SHADER);

  ID = OpenGLHelper::LinkProgram({vertexShader, fragmentShader});

  int success;
  glGetProgramiv(ID, GL_LINK_STATUS, &success);
  if (!success) {
    // TODO: Error handling
  }

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);
}

unsigned int Shader::GetProgram() const { return this->ID; }

void Shader::Use() { glUseProgram(ID); }

void Shader::SetBool(const std::string &name, bool value) const {
  glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}
void Shader::SetInt(const std::string &name, int value) const {
  glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}
void Shader::SetFloat(const std::string &name, float value) const {
  glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}
