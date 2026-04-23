#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class Texture {
public:
  uint32_t textureHandle;
  std::string path;
  std::string type;

  Texture() : textureHandle(0) {}
  Texture(uint32_t handle, std::string path, std::string type)
      : textureHandle(handle), path(std::move(path)), type(std::move(type)) {}
};

// Old Texture
/*
#include <string>

class Texture {
private:
  unsigned int ID;
  std::string filePath;
  unsigned char *imageBuffer;

  int width, height;

public:
  Texture(const std::string &filePath);
  ~Texture();

  // INFO: slot = the gpu has a limited slots for loading/ storing full
  // textures.
  void Bind(unsigned int slot = 0) const;
  void UnBind() const;

  unsigned int GetID() const;

  inline int GetWidth() const { return width; }
  inline int GetHeight() const { return height; }
};

*/
