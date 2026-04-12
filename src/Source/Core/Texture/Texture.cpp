#include "../../../Headers/Core/Texture/Texture.h"
#include "../../../Headers/Core/Common/ErrorHandling.h"
#include <GL/gl.h>

Texture::Texture(const std::string &filePath)
    : filePath(filePath), imageBuffer(nullptr), width(0), height(0) {

  /*
  imageBuffer =
      SOIL_load_image(filePath.c_str(), &width, &height, 0, SOIL_LOAD_RGBA);

  glCall(glGenTextures(1, &ID));
  glCall(glBindTexture(GL_TEXTURE_2D, ID));

  if (!imageBuffer)
    std::cout << "Error while Texture gen\n";

  glCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                      GL_UNSIGNED_BYTE, imageBuffer));
  glCall(glGenerateMipmap(GL_TEXTURE_2D));

  glCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
  glCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
  glCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
  glCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

  if (imageBuffer)
    SOIL_free_image_data(imageBuffer);
  */
}

Texture::~Texture() { glCall(glDeleteTextures(1, &ID)); }

unsigned int Texture::GetID() const { return ID; }

void Texture::Bind(unsigned int slot) const {
  glCall(glActiveTexture(GL_TEXTURE0 + slot));
  glCall(glBindTexture(GL_TEXTURE_2D, ID));
}

void Texture::UnBind() const { glCall(glBindTexture(GL_TEXTURE_2D, 0)); }
