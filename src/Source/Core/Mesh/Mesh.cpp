#include "../../../Headers/Core/Mesh/Mesh.h"
#include "../../../Headers/Core/Common/ErrorHandling.h"
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <cstddef>
#include <iostream>
#include <string>

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices,
           std::vector<Texture> texture) {
  this->vertices = vertices;
  this->indices = indices;
  this->textures = texture;

  setupMesh();
}

void Mesh::Draw(ShaderProgram &shaderProgram) {

  std::cout << "Mesh::Draw\n";

  if (indices.empty()) {
    std::cout << "Mesh::Draw indices empty\n";
    return;
  }

  if (vertices.empty()) {
    std::cout << "Mesh::Draw vertices empty\n";
    return;
  }

  std::cout << "Mesh::Draw -> Current Textures of the model: \n";
  for (auto texture : textures) {
    std::cout << "ID: " << texture.id << " | Path: " << texture.path
              << " | Type: " << texture.type << std::endl;
  }

  unsigned int diffuseNr = 1;
  unsigned int specularNr = 1;
  unsigned int normalNr = 1;
  unsigned int heightNr = 1;

  for (unsigned int i = 0; i < textures.size(); i++) {
    std::cout << "Mesh::Draw -> Current context: " << glfwGetCurrentContext()
              << std::endl;
    glCall(glActiveTexture(GL_TEXTURE0 + textures[i].id));

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

    std::cout << "Mesh::Draw -> current Texture ID: " << textures[i].id << " | "
              << textures[i].path << " | " << textures[i].type << std::endl;

    if (glIsTexture(textures[i].id) == GL_FALSE) {
      std::cout << "Mesh::Draw -> Texture with ID: { " << textures[i].id
                << " } does not exist in OpenGL | glIsTexture returned GL_FALSE"
                << std::endl;

      // break;
    }

    shaderProgram.setUniform1i((name + number).c_str(), i);

    std::cout << "Trying to bind: " << textures[i].id << " | "
              << textures[i].type << " | " << textures[i].path << std::endl;

    int whichID;
    glCall(glGetIntegerv(GL_TEXTURE_BINDING_2D, &whichID));
    std::cout
        << "Mesh::Draw -> Current ID of binded Texture: " << whichID
        << ". If 0, then there are no binded Textures in OpenGL context\n";

    glCall(glBindTexture(GL_TEXTURE_2D, textures[i].id));
  }

  std::cout << "Mesh::Draw -> draw mesh\n";

  // draw mesh
  glCall(glBindVertexArray(VAO));
  glCall(glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()),
                        GL_UNSIGNED_INT, 0));
  glCall(glBindVertexArray(0));

  glCall(glActiveTexture(GL_TEXTURE0));
}

void Mesh::setupMesh() {
  glCall(glGenVertexArrays(1, &VAO));
  glCall(glGenBuffers(1, &VBO));
  glCall(glGenBuffers(1, &EBO));

  glCall(glBindVertexArray(VAO));
  glCall(glBindBuffer(GL_ARRAY_BUFFER, VBO));

  glCall(glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
                      &vertices[0], GL_STATIC_DRAW));

  glCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO));
  glCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                      indices.size() * sizeof(unsigned int), &indices[0],
                      GL_STATIC_DRAW));

  // vertex position
  glCall(glEnableVertexAttribArray(0));
  glCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                               (void *)0));
  // vertex normal
  glCall(glEnableVertexAttribArray(1));
  glCall(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                               (void *)offsetof(Vertex, Normal)));
  // vertex texture coords
  glCall(glEnableVertexAttribArray(2));
  glCall(glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                               (void *)offsetof(Vertex, TexCoords)));

  // vertex tangent
  glCall(glEnableVertexAttribArray(3));
  glCall(glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                               (void *)offsetof(Vertex, Tangent)));

  // vertex bitangent
  glCall(glEnableVertexAttribArray(4));
  glCall(glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                               (void *)offsetof(Vertex, BiTangent)));

  // ids
  glCall(glEnableVertexAttribArray(5));
  glVertexAttribPointer(5, 4, GL_INT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, mBoneIDs));

  // wheights
  glCall(glEnableVertexAttribArray(6));
  glCall(glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                               (void *)offsetof(Vertex, mWeights)));

  glCall(glBindVertexArray(0));
}
