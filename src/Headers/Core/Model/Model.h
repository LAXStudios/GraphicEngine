#pragma once

#include "../Common/Common.h"
#include <GL/gl.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

class ShaderProgram;

class Model {
private:
  std::vector<Mesh> meshes;
  std::string directory;

  void loadModel(const std::string &path);
  void processNode(aiNode *node, const aiScene *scene);
  Mesh processMesh(aiMesh *mesh, const aiScene *scene);
  std::vector<Texture> loadMaterialTextures(aiMaterial *material,
                                            aiTextureType type,
                                            std::string typeName);

public:
  Model(const std::string &path);

  // Nicht Kopierbar, um double free zu verhindern
  Model(const Model &) = delete;
  Model &operator=(const Model &) = delete;

  // move erlaubt
  Model(Model &&) = default;
  Model &operator=(Model &&) = default;

  ~Model() = default;

  void Draw(ShaderProgram &shaderProgram);
};
