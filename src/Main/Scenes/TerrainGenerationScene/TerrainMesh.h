#pragma once

#include "../../../Headers/Core/Noise/PerlinNoise.h"
#include "Headers/Core/Mesh/Mesh.h"
#include "Headers/Core/Noise/PerlinNoise.h"
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

  PerlinNoise noise;

  void setupMesh();
  void cleanupMesh();
  void updateMesh();

  std::vector<TerrainVertex> generateGrid(int width, int height, float spacing,
                                          float amplitude, float frequency);
  std::vector<unsigned int> generateIndices(int width, int height);
  glm::vec3 computeNormal(int x, int z, int width, int height,
                          std::vector<TerrainVertex> verts);

public:
  TerrainMesh(int width, int height, float spacing, float amplitude,
              float frequency, unsigned int seed);

  TerrainMesh(const TerrainMesh &) = delete;
  TerrainMesh &operator=(const Mesh &) = delete;

  TerrainMesh(TerrainMesh &&other) noexcept;
  TerrainMesh &operator=(TerrainMesh &&other) noexcept;

  ~TerrainMesh();

  void Draw(ShaderProgram &shaderProgram);
  void RegenerateGrid(int width, int height, float spacing, float amplitude,
                      float frequency);
};
