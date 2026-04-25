#pragma once
#include "../Common/Common.h"

#include <cstdint>
#include <memory>
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

  // Verhindert Kopieren
  TextureManager(const TextureManager &) = delete;
  TextureManager &operator=(const TextureManager &) = delete;

  // Lade eine Textur oder gib die existierende zurück (Shared Ownership)
  uint32_t LoadTexture(const std::string &path);

  // Gib eine Textur frei (Referenzzähler senken)
  void ReleaseTexture(uint32_t handle);

  // Hole die echte OpenGL-ID für das Binden
  GLuint GetGLId(uint32_t handle) const;

  // Cleanup bei Engine-Shutdown (alle Texturen löschen)
  void Shutdown();

  // Prüfen, ob eine Textur existiert
  bool HasTexture(uint32_t handle) const;
};
