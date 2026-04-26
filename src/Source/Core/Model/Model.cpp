#include "../../../Headers/Core/Model/Model.h"
#include <GL/gl.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <cstdint>
#include <iostream>

Model::Model(const std::string &path) { loadModel(path); }

void Model::loadModel(const std::string &path) {
  std::cout << "[Model] Loading: " << path << std::endl;

  Assimp::Importer importer;
  const aiScene *scene = importer.ReadFile(
      path, aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
      !scene->mRootNode) {
    std::cerr << "[Model] Assimp Error: " << importer.GetErrorString()
              << std::endl;
    return;
  }

  // Directory extrahieren
  size_t lastSlash = path.find_last_of("/\\");
  if (lastSlash != std::string::npos) {
    directory = path.substr(0, lastSlash);
  } else {
    directory = ".";
  }

  processNode(scene->mRootNode, scene);
  std::cout << "[Model] Loaded " << meshes.size() << " meshes" << std::endl;
}

void Model::processNode(aiNode *node, const aiScene *scene) {
  // Meshes verarbeiten
  for (unsigned int i = 0; i < node->mNumMeshes; i++) {
    aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
    meshes.push_back(processMesh(mesh, scene));
  }

  // Kinder rekursiv verarbeiten
  for (unsigned int i = 0; i < node->mNumChildren; i++) {
    processNode(node->mChildren[i], scene);
  }
}

Mesh Model::processMesh(aiMesh *mesh, const aiScene *scene) {
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;

  // Vertices laden
  for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
    Vertex vertex;
    vertex.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y,
                                mesh->mVertices[i].z);

    if (mesh->HasNormals()) {
      vertex.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y,
                                mesh->mNormals[i].z);
    }

    if (mesh->mTextureCoords[0]) {
      vertex.TexCoords =
          glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
      if (mesh->HasTangentsAndBitangents()) {
        vertex.Tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y,
                                   mesh->mTangents[i].z);
        vertex.BiTangent =
            glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y,
                      mesh->mBitangents[i].z);
      }
    } else {
      vertex.TexCoords = glm::vec2(0.0f, 0.0f);
    }

    for (int j = 0; j < 4; j++)
      vertex.mBoneIDs[j] = -1;
    vertex.mWeights = glm::vec4(0.0f);
    vertices.push_back(vertex);
  }

  // Indices laden
  for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
    aiFace face = mesh->mFaces[i];
    for (unsigned int j = 0; j < face.mNumIndices; j++) {
      indices.push_back(face.mIndices[j]);
    }
  }

  // lade textures
  aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
  std::vector<Texture> textures;

  {
    auto diff = loadMaterialTextures(material, aiTextureType_DIFFUSE,
                                     "texture_diffuse");
    textures.insert(textures.end(), diff.begin(), diff.end());
  }

  {
    auto spec = loadMaterialTextures(material, aiTextureType_SPECULAR,
                                     "texture_specular");
    textures.insert(textures.end(), spec.begin(), spec.end());
  }

  {
    auto norm =
        loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
    textures.insert(textures.end(), norm.begin(), norm.end());
  }

  // Noch nicht umgesetzt
  // {
  //     auto amb = loadMaterialTextures(material, aiTextureType_AMBIENT,
  //     "texture_height"); textures.insert(textures.end(), amb.begin(),
  //     amb.end());
  // }

  return Mesh(std::move(vertices), std::move(indices), std::move(textures));
}

std::vector<Texture> Model::loadMaterialTextures(aiMaterial *material,
                                                 aiTextureType type,
                                                 std::string typeName) {
  std::vector<Texture> textures;
  unsigned int count = material->GetTextureCount(type);

  if (count == 0)
    return textures;

  for (unsigned int i = 0; i < count; i++) {
    aiString str;
    material->GetTexture(type, i, &str);

    // SICHERHEITS-CHECK: Ist der Pfad leer?
    if (str.length == 0) {
      std::cerr << "[Model] Warning: Empty texture path for type: " << typeName
                << std::endl;
      continue;
    }

    std::string fullPath = directory + "/" + str.C_Str();

    // Debug Output
    std::cout << "[Model] Attempting to load: " << fullPath << std::endl;

    // Lade über Manager
    uint32_t handle = TextureManager::Get().LoadTexture(fullPath);

    if (handle != 0) {
      textures.emplace_back(handle, fullPath, typeName);
    } else {
      std::cerr << "[Model] Failed to load texture: " << fullPath << std::endl;
    }
  }

  return textures;
}

void Model::Draw(ShaderProgram &shaderProgram) {
  for (auto &mesh : meshes) {
    mesh.Draw(shaderProgram);
  }
}
