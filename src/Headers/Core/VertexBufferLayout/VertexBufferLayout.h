#pragma once
#include "../Common/ErrorHandling.h"
#include <GL/gl.h>
#include <vector>

struct VertexBufferLayoutElement {
  unsigned int type;
  unsigned int count;
  unsigned int normalized;

  static unsigned int GetSize(GLenum type) {
    switch (type) {
    case GL_FLOAT:
      return sizeof(float);
    case GL_UNSIGNED_INT:
      return sizeof(unsigned int);
    case GL_UNSIGNED_BYTE:
      return sizeof(unsigned char);
    }
    ASSERT(false);
    return 0;
  }
};

class VertexBufferLayout {
private:
  std::vector<VertexBufferLayoutElement> layouts;
  unsigned int stride;

public:
  VertexBufferLayout() : stride(0) {}
  ~VertexBufferLayout();

  template <typename T> void AddElement(unsigned int count) { ASSERT(true); }

  unsigned int GetStride() const { return stride; }

  std::vector<VertexBufferLayoutElement> GetLayoutElements() const {
    return layouts;
  }
};
