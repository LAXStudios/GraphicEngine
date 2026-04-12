#include <GL/glew.h>

#include <glm/ext/vector_float3.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <string>
#include <vector>

#include "../ShaderProgram/ShaderProgram.h"

#define MAX_BONE_INFLUENCE 4

struct Vertex {
  glm::vec3 Position;
  glm::vec3 Normal;
  glm::vec2 TexCoords;
  glm::vec3 Tangent;
  glm::vec3 BiTangent;
  int mBoneIDs[MAX_BONE_INFLUENCE];
  float mWeights[MAX_BONE_INFLUENCE];
};

struct Texture {
  unsigned int id;
  std::string type;
  std::string path;
};

class Mesh {
public:
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;
  std::vector<Texture> textures;

  Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices,
       std::vector<Texture> texture);
  void Draw(ShaderProgram &shader);

private:
  unsigned int VAO, VBO, EBO;

  void setupMesh();
};
