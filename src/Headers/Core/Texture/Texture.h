#pragma once

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
