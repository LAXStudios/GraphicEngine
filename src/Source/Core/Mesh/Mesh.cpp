#include "../../../Headers/Core/Mesh/Mesh.h"
#include <iostream>

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices,
           std::vector<Texture> textures)
    : vertices(std::move(vertices)), indices(std::move(indices)),
      textures(std::move(textures)) {
  setupMesh();
}

Mesh::~Mesh() { cleanupMesh(); }

Mesh::Mesh(Mesh &&other) noexcept
    : vertices(std::move(other.vertices)), indices(std::move(other.indices)),
      textures(std::move(other.textures)), VAO(other.VAO), VBO(other.VBO),
      EBO(other.EBO), buffersInitialized(other.buffersInitialized) {
  other.VAO = 0;
  other.VBO = 0;
  other.EBO = 0;
  other.buffersInitialized = false;
}

Mesh &Mesh::operator=(Mesh &&other) noexcept {
  if (this != &other) {
    cleanupMesh(); // alte Ressourcen löschen

    vertices = std::move(other.vertices);
    indices = std::move(other.indices);
    textures = std::move(other.textures);
    VAO = other.VAO;
    VBO = other.VBO;
    EBO = other.EBO;
    buffersInitialized = other.buffersInitialized;

    other.VAO = 0;
    other.VBO = 0;
    other.EBO = 0;
    other.buffersInitialized = false;
  }
  return *this;
}

void Mesh::setupMesh() {
  if (buffersInitialized)
    return;

  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);

  glBindVertexArray(VAO);

  // Vertex Data
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
               vertices.data(), GL_STATIC_DRAW);

  // Index Data
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
               indices.data(), GL_STATIC_DRAW);

  // Vertex Attributes
  // Position (attribute 0)
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, Position));

  // Normal (attribute 1)
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, Normal));

  // TexCoords (attribute 2)
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, TexCoords));

  // Tangent (attribute 3)
  glEnableVertexAttribArray(3);
  glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, Tangent));

  // BiTangent (attribute 4)
  glEnableVertexAttribArray(4);
  glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, BiTangent));

  // Bone IDs (attribute 5)
  glEnableVertexAttribArray(5);
  glVertexAttribPointer(5, 4, GL_INT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, mBoneIDs));

  // Weights (attribute 6)
  glEnableVertexAttribArray(6);
  glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, mWeights));

  glBindVertexArray(0);
  buffersInitialized = true;

  std::cout << "[Mesh] Setup complete: " << vertices.size() << " vertices, "
            << indices.size() << " indices" << std::endl;
}

void Mesh::cleanupMesh() {
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
    std::cout << "[Mesh] Cleanup complete" << std::endl;
  }
}

void Mesh::Draw(ShaderProgram &shaderProgram) {
  if (!buffersInitialized || indices.empty() || vertices.empty()) {
    return;
  }

  unsigned int diffuseNr = 1;
  unsigned int specularNr = 1;
  unsigned int normalNr = 1;
  unsigned int heightNr = 1;

  for (size_t i = 0; i < textures.size(); i++) {
    // id vom texture manager
    GLuint glId = TextureManager::Get().GetGLId(textures[i].textureHandle);

    if (glId == 0) {
      std::cerr << "[Mesh] Invalid texture handle: "
                << textures[i].textureHandle
                << " for path: " << textures[i].path << std::endl;
      continue;
    }

    // Aktiviere Textur Unit (INDEX, nicht ID!)
    glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(i));
    glBindTexture(GL_TEXTURE_2D, glId);

    std::string number;
    std::string name = textures[i].type;

    if (name == "texture_diffuse")
      number = std::to_string(diffuseNr++);
    else if (name == "texture_specular")
      number = std::to_string(specularNr++);
    else if (name == "texture_normal")
      number = std::to_string(normalNr++);
    else if (name == "texture_height")
      number = std::to_string(heightNr++);

    shaderProgram.setUniform1i((name + number).c_str(), static_cast<int>(i));
  }

  glBindVertexArray(VAO);
  glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()),
                 GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);

  glActiveTexture(GL_TEXTURE0);
}
