#pragma once

#include "Headers/Core/Mesh/Mesh.h"
#include "Headers/Core/ShaderProgram/ShaderProgram.h"
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <vector>

struct TerrainVertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 uv;
};

class TerrainMesh {
private:
  std::vector<TerrainVertex> vertices;
  std::vector<unsigned int> indices;

  unsigned int VAO = 0;
  unsigned int VBO = 0;
  unsigned int EBO = 0;

  bool buffersInitialized = false;

  void setupMesh();
  void cleanupMesh();

  std::vector<TerrainVertex> generateGrid(int width, int height, float spacing);
  std::vector<unsigned int> generateIndices(int width, int height);
  glm::vec3 computeNormal(int x, int z, int width, int height);

public:
  TerrainMesh(int width, int height, float spacing);

  TerrainMesh(const TerrainMesh &) = delete;
  TerrainMesh &operator=(const Mesh &) = delete;

  TerrainMesh(TerrainMesh &&other) noexcept;
  TerrainMesh &operator=(TerrainMesh &&other) noexcept;

  ~TerrainMesh();

  void Draw(ShaderProgram &shaderProgram);
};
