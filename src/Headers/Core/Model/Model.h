#include "../../../../extern/soil/SOIL.h"
#include "../Common/ErrorHandling.h"
#include "../Mesh/Mesh.h"
#include "../ShaderProgram/ShaderProgram.h"
#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/types.h>
#include <iostream>
#include <string>
#include <vector>

class Model {
public:
  Model(const std::string &path);
  void Draw(ShaderProgram &ShaderProgram);

private:
  std::vector<Texture> texturesLoaded;
  std::vector<Mesh> meshes;

  std::string directory;

  void loadModel(std::string const &path);
  void processNode(aiNode *node, const aiScene *scene);
  Mesh processMesh(aiMesh *mesh, const aiScene *scene);
  std::vector<Texture> loadMaterialTextures(aiMaterial *material,
                                            aiTextureType type,
                                            std::string typeName);

  unsigned int textureFromFile(const char *path, const std::string &directory);
};
