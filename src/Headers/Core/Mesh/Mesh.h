#pragma once

#include "../Common/Common.h"
#include <GL/gl.h>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

struct Vertex {
  glm::vec3 Position;
  glm::vec3 Normal;
  glm::vec2 TexCoords;
  glm::vec3 Tangent;
  glm::vec3 BiTangent;
  int mBoneIDs[4];
  glm::vec4 mWeights;
};

class ShaderProgram;

class Mesh {
private:
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;
  std::vector<Texture> textures;

  // Lokale GPU-Ressourcen (gehören nur diesem Mesh!)
  GLuint VAO = 0;
  GLuint VBO = 0;
  GLuint EBO = 0;

  bool buffersInitialized = false;

  void setupMesh();
  void cleanupMesh();

public:
  Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices,
       std::vector<Texture> textures);

  // Nicht kopierbar wegen diesem RAII verfahren
  Mesh(const Mesh &) = delete;
  Mesh &operator=(const Mesh &) = delete;

  // Man darf es moven, weil Ro5
  Mesh(Mesh &&other) noexcept;
  Mesh &operator=(Mesh &&other) noexcept;

  ~Mesh();

  void Draw(ShaderProgram &shaderProgram);

  const std::vector<Texture> &GetTextures() const { return textures; }
};
