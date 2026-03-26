#include "../../../Headers/Core/ShaderProgram/ShaderProgram.h"
#include "../../../Headers/Core/Common/ErrorHandling.h"
#include <GL/gl.h>
#include <alloca.h>
#include <fstream>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <sstream>
#include <string>

ShaderProgram::ShaderProgram(std::string shaderPath) {
  ShaderSource shaderSource = LoadShaderFromSourceFile(shaderPath);
  ID = LinkShaderToProgram(shaderSource);

  std::cout << "ShaderProgram ID: " << ID << "\n";
}

ShaderProgram::~ShaderProgram() {}

void ShaderProgram::Bind() const { glCall(glUseProgram(ID)); }

void ShaderProgram::UnBind() const { glCall(glUseProgram(0)); }

unsigned int ShaderProgram::GetID() const { return ID; }

unsigned int ShaderProgram::GetProgram() const { return ID; }

ShaderSource
ShaderProgram::LoadShaderFromSourceFile(const std::string &shaderPath) {
  enum class shaderType { NONE = -1, VERTEX_SHADER = 0, FRAGMENT_SHADER = 1 };

  shaderType type = shaderType::NONE;

  std::string line;
  std::stringstream shaders[2];
  std::ifstream input(shaderPath);
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
      continue;
    }

    if (type != shaderType::NONE) {
      shaders[(int)type] << line << "\n";
    }
  }

  /*

  std::cout << "Vertex:\n"
            << shaders[(int)shaderType::VERTEX_SHADER].str() << '\n'
            << "Fragment:\n"
            << shaders[(int)shaderType::FRAGMENT_SHADER].str() << '\n';
            */

  return ShaderSource{shaders[(int)shaderType::VERTEX_SHADER].str(),
                      shaders[(int)shaderType::FRAGMENT_SHADER].str()};
}

unsigned int ShaderProgram::CompileShader(const std::string &source,
                                          unsigned int type) {
  unsigned int shaderID = glCreateShader(type);
  const char *src = source.c_str();
  glCall(glShaderSource(shaderID, 1, &src, nullptr));
  glCall(glCompileShader(shaderID));

  // Error Handling
  int result;
  glCall(glGetShaderiv(shaderID, GL_COMPILE_STATUS, &result));

  if (!result) {
    PrintShaderInfoLog(shaderID, type);
  }

  return shaderID;
}

unsigned int ShaderProgram::LinkShaderToProgram(ShaderSource shaderSource) {
  unsigned int program = glCreateProgram();
  unsigned int vertex =
      CompileShader(shaderSource.vertexShader.c_str(), GL_VERTEX_SHADER);
  unsigned int fragment =
      CompileShader(shaderSource.fragmentShader.c_str(), GL_FRAGMENT_SHADER);

  int linkStatus;
  glCall(glAttachShader(program, vertex));
  glCall(glAttachShader(program, fragment));
  glCall(glLinkProgram(program));

  glCall(glGetProgramiv(program, GL_LINK_STATUS, &linkStatus));

  if (!linkStatus) {
    PrintProgramInfoLog(program);
  }

  glCall(glDeleteShader(vertex));
  glCall(glDeleteShader(fragment));

  return program;
}

//
// Uniform Stuff
//

unsigned int ShaderProgram::getUniformLocation(const std::string &uniformName) {
  unsigned int index;
  glCall(index = glGetUniformLocation(ID, uniformName.c_str()));
  return index;
}

void ShaderProgram::setUniform1i(const std::string &uniformName, int value) {
  // this->Bind();
  glCall(glUniform1i(getUniformLocation(uniformName), value));
  // this->UnBind();
}

void ShaderProgram::setUniform3f(const std::string &uniformName, float v1,
                                 float v2, float v3) {
  this->Bind();
  glCall(glUniform3f(getUniformLocation(uniformName), v1, v2, v3));
  this->UnBind();
}

void ShaderProgram::setUniformMatrix4fv(const std::string &uniformName,
                                        glm::mat4 matrix) {
  this->Bind();
  // INFO: glm::value_ptr könnte zu &matrix[0][0] geändert werden müssen
  glCall(glUniformMatrix4fv(getUniformLocation(uniformName), 1, GL_FALSE,
                            &matrix[0][0]));
  this->UnBind();
}

void ShaderProgram::setUniform3fv(const std::string &uniformName,
                                  glm::vec3 vector) {
  this->Bind();
  glCall(glUniform3fv(getUniformLocation(uniformName), 1, &vector[0]));
  this->UnBind();
}

/*
void ShaderProgram::PrintShaderInfoLog(int shaderID, GLenum type) {
  int length;
  // glCall(glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &length));
  char message[1024];
  glGetShaderInfoLog(shaderID, 1024, &length, message);
  std::cout << "Error while Compiling Shader: " << message << "\n";
}
*/

void ShaderProgram::PrintShaderInfoLog(int shaderID, GLenum type) {
  // 1. Prüfen, ob der Shader kompiliert wurde
  GLint compiled = 0;
  glGetShaderiv(shaderID, GL_COMPILE_STATUS, &compiled);
  if (compiled)
    return; // kein Fehler → nichts ausgeben

  // 2. Länge des Info‑Logs erfragen
  GLint logLength = 0;
  glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &logLength);
  if (logLength == 0) {
    std::cout << "Shader compiled, but no log available.\n";
    return;
  }

  // 3. Puffer exakt passend anlegen (+1 für Terminator)
  std::vector<GLchar> log(logLength);
  glGetShaderInfoLog(shaderID, logLength, nullptr, log.data());

  // 4. Ausgabe
  std::cout << "Error while compiling shader:\n" << log.data() << '\n';
}

/*
void ShaderProgram::PrintProgramInfoLog(int programID) {
  int length;
  // glCall(glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &length));
  char message[1024];
  glGetProgramInfoLog(programID, 1024, &length, message);
  std::cout << "Error while Linking Shader" << message << "\n";
}
*/

void ShaderProgram::PrintProgramInfoLog(int programID) {
  // 1. Prüfen, ob das Programm gelinkt wurde
  GLint linked = 0;
  glGetProgramiv(programID, GL_LINK_STATUS, &linked);
  if (linked)
    return; // kein Fehler → nichts ausgeben

  // 2. Länge des Info‑Logs erfragen
  GLint logLength = 0;
  glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &logLength);
  if (logLength == 0) {
    std::cout << "Program linked, but no log available.\n";
    return;
  }

  // 3. Puffer exakt passend anlegen
  std::vector<GLchar> log(logLength);
  glGetProgramInfoLog(programID, logLength, nullptr, log.data());

  // 4. Ausgabe
  std::cout << "Error while linking program:\n" << log.data() << '\n';
}
