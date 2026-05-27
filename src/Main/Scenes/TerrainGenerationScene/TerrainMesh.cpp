#include "TerrainMesh.h"
#include "Headers/Core/Common/Common.h"
#include "Headers/Core/Common/ErrorHandling.h"
#include "Headers/Core/ShaderProgram/ShaderProgram.h"
#include <cmath>
#include <cstddef>
#include <glm/common.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/ext/vector_float3.hpp>
#include <vector>

TerrainMesh::TerrainMesh(int width, int height, float spacing) {
  vertices = generateGrid(width, height, spacing);
  indices = generateIndices(width, height);
  setupMesh();
}

TerrainMesh::~TerrainMesh() { cleanupMesh(); }

void TerrainMesh::setupMesh() {
  // Should when working rewrote to VertexArray and VertexBuffer etc

  if (buffersInitialized)
    return;

  glCall(glGenVertexArrays(1, &VAO));
  glCall(glGenBuffers(1, &VBO));
  glCall(glGenBuffers(1, &EBO));

  glCall(glBindVertexArray(VAO));

  glCall(glBindBuffer(GL_ARRAY_BUFFER, VBO));
  glCall(glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(TerrainVertex),
                      vertices.data(), GL_DYNAMIC_DRAW));

  glCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO));
  glCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                      indices.size() * sizeof(unsigned int), indices.data(),
                      GL_STATIC_DRAW));

  glCall(glEnableVertexAttribArray(0));
  glCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex),
                               (void *)offsetof(TerrainVertex, position)));

  glCall(glEnableVertexAttribArray(1));
  glCall(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex),
                               (void *)offsetof(TerrainVertex, normal)));

  glCall(glEnableVertexAttribArray(2));
  glCall(glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex),
                               (void *)offsetof(TerrainVertex, uv)));

  glCall(glBindVertexArray(0));

  buffersInitialized = true;
}

void TerrainMesh::Draw(ShaderProgram &shaderProgram) {
  shaderProgram.Bind();
  glBindVertexArray(VAO);
  glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()),
                 GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
}

void TerrainMesh::cleanupMesh() {
  if (buffersInitialized) {
    if (VAO != 0) {
      glDeleteVertexArrays(1, &VAO);
      VAO = 0;
    }
    if (VBO != 0) {
      glDeleteBuffers(1, &VBO);
      VBO = 0;
    }
    if (EBO != 0) {
      glDeleteBuffers(1, &EBO);
      EBO = 0;
    }
    buffersInitialized = false;
  }
}

std::vector<TerrainVertex> TerrainMesh::generateGrid(int width, int height,
                                                     float spacing) {
  std::vector<TerrainVertex> vertices;
  vertices.reserve(width * height);

  for (int z = 0; z < height; z++) {
    for (int x = 0; x < width; x++) {
      TerrainVertex vertex;

      vertex.position.x = (x - width * 0.5f) * spacing;
      vertex.position.z = (z - height * 0.5f) * spacing;
      float sx = std::sin(vertex.position.x * 0.4f);
      float cz = std::cos(vertex.position.z * 0.4f);
      vertex.position.y = ((sx * cz) + 1.0f) * 0.5f * 5.0f;

      vertex.uv.x = (float)x / (width - 1);
      vertex.uv.y = (float)z / (height - 1);

      vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);

      vertices.push_back(vertex);
    }
  }

  return vertices;
}

int getIndex(int px, int pz, int width, int height) {
  px = glm::clamp(px, 0, width - 1);
  pz = glm::clamp(pz, 0, height - 1);

  return pz * width + px;
}

glm::vec3 TerrainMesh::computeNormal(int x, int z, int height, int width) {
  // Get the points arround the current point

  float pointLeft = vertices[getIndex(x - 1, z, width, height)].position.y;
  float pointRight = vertices[getIndex(x + 1, z, width, height)].position.y;
  float pointNorth = vertices[getIndex(x, z - 1, width, height)].position.y;
  float pointSouth = vertices[getIndex(x, z + 1, width, height)].position.y;

  return glm::normalize(
      glm::vec3(pointLeft - pointRight, 2.0f, pointNorth - pointSouth));
}

std::vector<unsigned int> TerrainMesh::generateIndices(int width, int height) {
  std::vector<unsigned int> indices;

  indices.reserve((width - 1) * (height - 1) * 6);

  for (int z = 0; z < height - 1; z++) {
    for (int x = 0; x < width - 1; x++) {
      unsigned int A = z * width + x;
      unsigned int B = z * width + (x + 1);
      unsigned int C = (z + 1) * width + x;
      unsigned int D = (z + 1) * width + (x + 1);

      indices.push_back(A);
      indices.push_back(C);
      indices.push_back(B);

      indices.push_back(B);
      indices.push_back(C);
      indices.push_back(D);
    }
  }
  return indices;
}
