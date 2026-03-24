#include "../../../Headers/Core/VertexBufferLayout/VertexBufferLayout.h"
#include <GL/gl.h>

VertexBufferLayout::~VertexBufferLayout() {}

template <> void VertexBufferLayout::AddElement<float>(unsigned int count) {
  layouts.push_back({GL_FLOAT, count, GL_FALSE});
  stride += VertexBufferLayoutElement::GetSize(GL_FLOAT) * count;
};

template <>
void VertexBufferLayout::AddElement<unsigned int>(unsigned int count) {
  layouts.push_back({GL_UNSIGNED_INT, count, GL_FALSE});
  stride += VertexBufferLayoutElement::GetSize(GL_UNSIGNED_INT) * count;
}

template <>
void VertexBufferLayout::AddElement<unsigned char>(unsigned int count) {
  layouts.push_back({GL_UNSIGNED_BYTE, count, GL_TRUE});
  stride += VertexBufferLayoutElement::GetSize(GL_UNSIGNED_BYTE) * count;
}
