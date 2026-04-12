#include "../../../Headers/Core/Model/Model.h"
#include <iostream>
#include <ostream>
#define STB_IMAGE_IMPLEMENTATION
#include "../../../../extern/stb/stb_image.h"

Model::Model(const std::string &path) { loadModel(path); }

void Model::Draw(ShaderProgram &shaderProgram) {
  // std::cout << "Model::Draw\n";
  if (meshes.empty()) {
    std::cerr << "Warining: No meshes\n";
    return;
  }
  for (unsigned int i = 0; i < meshes.size(); i++)
    meshes[i].Draw(shaderProgram);
}

void Model::loadModel(std::string const &path) {
  std::cout << "Model::loadModel\n";
  Assimp::Importer importer;
  const aiScene *scene = importer.ReadFile(
      path, aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
      !scene->mRootNode) {
    std::cout << "Error::Assimp:: " << importer.GetErrorString() << std::endl;
  }
  directory = path.substr(0, path.find_last_of('/'));

  processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode *node, const aiScene *scene) {
  std::cout << "Model::processNode\n";
  for (unsigned int i = 0; i < node->mNumMeshes; i++) {
    aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
    meshes.push_back(processMesh(mesh, scene));
  }

  for (unsigned int i = 0; i < node->mNumChildren; i++) {
    processNode(node->mChildren[i], scene);
  }
}

Mesh Model::processMesh(aiMesh *mesh, const aiScene *scene) {
  std::cout << "Model::processMesh\n";

  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;
  std::vector<Texture> textures;

  for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
    if (i >= mesh->mNumVertices) {
      std::cerr << "Vertex index out of bounds\n";
      break;
    }
    Vertex vertex;
    glm::vec3 vector;

    vector.x = mesh->mVertices[i].x;
    vector.y = mesh->mVertices[i].y;
    vector.z = mesh->mVertices[i].z;

    vertex.Position = vector;

    if (mesh->HasNormals()) {
      vector.x = mesh->mNormals[i].x;
      vector.y = mesh->mNormals[i].y;
      vector.z = mesh->mNormals[i].z;
    }

    if (mesh->mTextureCoords[i]) {
      glm::vec2 vec;

      vec.x = mesh->mTextureCoords[0][i].x;
      vec.y = mesh->mTextureCoords[0][i].y;
      vertex.TexCoords = vec;

      vector.x = mesh->mTangents[i].x;
      vector.y = mesh->mTangents[i].y;
      vector.z = mesh->mTangents[i].z;
      vertex.Tangent = vector;

      vector.x = mesh->mBitangents[i].x;
      vector.y = mesh->mBitangents[i].y;
      vector.z = mesh->mBitangents[i].z;
      vertex.BiTangent = vector;
    } else {
      vertex.TexCoords = glm::vec2(0.0f, 0.0f);
    }

    vertices.push_back(vertex);
  }

  for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
    aiFace face = mesh->mFaces[i];
    for (unsigned int j = 0; j < face.mNumIndices; j++)
      indices.push_back(face.mIndices[j]);
  }

  std::cout << "Model::processMesh -> vertices: " << vertices.data()
            << std::endl;

  aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

  std::vector<Texture> diffuseMaps =
      loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
  std::cout << "Model::processMesh -> got diffuse texture\n";

  std::vector<Texture> specularMaps = loadMaterialTextures(
      material, aiTextureType_SPECULAR, "texture_specular");
  std::cout << "Model::processMesh -> got specular texture\n";

  std::vector<Texture> normalMaps =
      loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
  std::cout << "Model::processMesh -> got normal texture\n";

  std::vector<Texture> heightMaps =
      loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
  std::cout << "Model::processMesh -> got height texture\n";

  std::cout << "Model::processMesh -> loaded textures\n";

  return Mesh(vertices, indices, textures);
}

std::vector<Texture> Model::loadMaterialTextures(aiMaterial *material,
                                                 aiTextureType type,
                                                 std::string typeName) {
  std::cout << "Model::loadMaterialTextures\n";
  std::vector<Texture> textures;
  for (unsigned int i = 0; i < material->GetTextureCount(type); i++) {
    aiString str;

    material->GetTexture(type, i, &str);

    bool skip = false;
    for (unsigned int j = 0; j < texturesLoaded.size(); j++) {
      if (std::strcmp(texturesLoaded[j].path.data(), str.C_Str()) == 0) {
        textures.push_back(texturesLoaded[j]);
        skip = true;
        break;
      }
    }

    std::cout << "Model::loadMaterialTextures -> got textures\n";

    if (!skip) {
      Texture texture;
      texture.id = textureFromFile(str.C_Str(), this->directory);
      texture.type = typeName;
      texture.path = str.C_Str();
      textures.push_back(texture);
      texturesLoaded.push_back(texture);
    }

    std::cout
        << "Model::loadMaterialTextures -> setted texture and will return\n";
  }

  if (textures.empty()) {
    std::cout << "Model::loadMaterialTextures texture empty\n";
  }

  return textures;
}

unsigned int Model::textureFromFile(const char *path,
                                    const std::string &directory) {
  std::cout << "Model::textureFromFile start\n";

  std::string filename = std::string(path);
  filename = directory + '/' + filename;
  std::cout << "Model::textureFromFile filename: " << filename << std::endl;

  unsigned int textureId;
  glCall(glGenTextures(1, &textureId));

  int width, height, nrComponents;
  // unsigned char *data =
  // SOIL_load_image(filename.c_str(), &width, &height, 0, SOIL_LOAD_AUTO);

  unsigned char *data =
      stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);

  if (!data)
    std::cout << "data empty\n";

  if (data) {
    GLenum format;
    if (nrComponents == 1)
      format = GL_RED;
    else if (nrComponents == 3)
      format = GL_RGB;
    else if (nrComponents == 4)
      format = GL_RGBA;

    glCall(glBindTexture(GL_TEXTURE_2D, textureId));
    glCall(glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
                        GL_UNSIGNED_INT, data));

    glCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
    glCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
    glCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    glCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

    stbi_image_free(data);

  } else {
    std::cout << "Texure fgailed to load at path: " << path << std::endl;
    stbi_image_free(data);
  }

  return textureId;
}
