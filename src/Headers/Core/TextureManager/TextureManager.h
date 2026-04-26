#pragma once
#include "../Common/Common.h"
#include "Headers/Core/Texture/Texture.h"

#include <cstdint>
#include <string>
#include <unordered_map>

class TextureManager {
private:
  // Interne Struktur: Speichert echte OpenGL-ID und Referenzzähler
  struct TextureEntry {
    GLuint glId;
    int refCount;
    std::string path;
  };

  std::unordered_map<uint32_t, TextureEntry> textures;
  uint32_t nextHandle = 1;

  // Private Constructor für Singleton
  TextureManager() = default;

public:
  // Singleton Accessor
  static TextureManager &Get() {
    static TextureManager instance;
    return instance;
  }

  // resists copyieng
  TextureManager(const TextureManager &) = delete;
  TextureManager &operator=(const TextureManager &) = delete;

  // laod texture or give existing back
  uint32_t LoadTexture(const std::string &path);

  void ReleaseTexture(uint32_t handle);

  GLuint GetGLId(uint32_t handle) const;

  void Shutdown();

  // check if texture exists
  bool HasTexture(uint32_t handle) const;
};
