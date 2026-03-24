#pragma once
#include "../Common/ErrorHandling.h"

enum class VertexBufferDrawType { STATIC_DRAW, DYNAMIC_DRAW };

class VertexBuffer {
private:
  unsigned int ID;

public:
  VertexBuffer(const void *data, GLuint size,
               VertexBufferDrawType type = VertexBufferDrawType::STATIC_DRAW);
  ~VertexBuffer();

  void Bind() const;
  void UnBind() const;

  unsigned int GetId() const;
};
