#include "../../../Headers/Core/TextureManager/TextureManager.h"
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include "../../../../extern/stb/stb_image.h"

uint32_t TextureManager::LoadTexture(const std::string &path) {
  // Prüfen, ob Textur schon existiert
  for (auto &[handle, entry] : textures) {
    if (entry.path == path) {
      entry.refCount++;
      std::cout << "[TextureManager] Reused existing texture: " << path
                << " (RefCount: " << entry.refCount << ")" << std::endl;
      return handle;
    }
  }

  // Neue Textur laden
  std::cout << "[TextureManager] Loading new texture: " << path << std::endl;

  GLuint glId = 0;
  glGenTextures(1, &glId);

  if (glId == 0) {
    std::cerr << "[TextureManager] Failed to generate texture ID for: " << path
              << std::endl;
    return 0;
  }

  glBindTexture(GL_TEXTURE_2D, glId);

  // Bild laden
  int width = 0, height = 0, nrComponents = 0;
  stbi_set_flip_vertically_on_load(true);
  unsigned char *data =
      stbi_load(path.c_str(), &width, &height, &nrComponents, 0);

  if (data) {
    GLenum format = (nrComponents == 1)   ? GL_RED
                    : (nrComponents == 3) ? GL_RGB
                                          : GL_RGBA;

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
                 GL_UNSIGNED_BYTE, data);

    // Texture Parameter
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    std::cout << "[TextureManager] Successfully loaded: " << path << " ("
              << width << "x" << height << ")" << std::endl;
  } else {
    std::cerr << "[TextureManager] Failed to load image: " << path << std::endl;
    glDeleteTextures(1, &glId);
    return 0;
  }

  // In Map speichern
  uint32_t newHandle = nextHandle++;
  textures[newHandle] = {glId, 1, path};

  return newHandle;
}

void TextureManager::ReleaseTexture(uint32_t handle) {
  auto it = textures.find(handle);
  if (it != textures.end()) {
    it->second.refCount--;
    std::cout << "[TextureManager] Released texture: " << it->second.path
              << " (RefCount: " << it->second.refCount << ")" << std::endl;

    if (it->second.refCount <= 0) {
      std::cout << "[TextureManager] Deleting texture from GPU: "
                << it->second.path << std::endl;
      glDeleteTextures(1, &it->second.glId);
      textures.erase(it);
    }
  }
}

GLuint TextureManager::GetGLId(uint32_t handle) const {
  auto it = textures.find(handle);
  return (it != textures.end()) ? it->second.glId : 0;
}

bool TextureManager::HasTexture(uint32_t handle) const {
  return textures.find(handle) != textures.end();
}

void TextureManager::Shutdown() {
  std::cout << "[TextureManager] Shutting down, deleting " << textures.size()
            << " textures..." << std::endl;
  for (auto &[handle, entry] : textures) {
    if (entry.glId != 0) {
      glDeleteTextures(1, &entry.glId);
    }
  }
  textures.clear();
  nextHandle = 1;
}
