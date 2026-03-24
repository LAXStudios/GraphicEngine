#include "../../../Headers/Core/VertexBuffer/VertexBuffer.h"

VertexBuffer::VertexBuffer(const void *data, GLuint size,
                           VertexBufferDrawType type) {
  glCall(glGenBuffers(1, &ID));
  glCall(glBindBuffer(GL_ARRAY_BUFFER, ID));
  switch (type) {
  case VertexBufferDrawType::STATIC_DRAW:
    glCall(glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW));
    break;
  case VertexBufferDrawType::DYNAMIC_DRAW:
    glCall(glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW));
    break;
  }
}

VertexBuffer::~VertexBuffer() { glCall(glDeleteBuffers(1, &ID)); }

void VertexBuffer::Bind() const { glCall(glBindBuffer(GL_ARRAY_BUFFER, ID)); }

void VertexBuffer::UnBind() const { glCall(glBindBuffer(GL_ARRAY_BUFFER, 0)); }

unsigned int VertexBuffer::GetId() const { return this->ID; }
