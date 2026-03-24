#include "../../../Headers/Core/IndexBuffer/IndexBuffer.h"
#include <GL/glext.h>

IndexBuffer::IndexBuffer(const unsigned int *data, int count) : count(count) {
  glCall(glGenBuffers(1, &ID));
  glCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID));
  glCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(unsigned int),
                      data, GL_STATIC_DRAW));
}

IndexBuffer::~IndexBuffer() { glCall(glDeleteBuffers(1, &ID)); }

void IndexBuffer::Bind() { glCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID)); }

void IndexBuffer::UnBind() { glCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0)); }

unsigned int IndexBuffer::GetCount() const { return this->count; }

unsigned int IndexBuffer::GetID() const { return this->ID; }
