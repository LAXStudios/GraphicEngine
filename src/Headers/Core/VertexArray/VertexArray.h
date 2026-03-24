#pragma once

#include "../Common/Common.h"

class VertexArray {
private:
  unsigned int ID;
  unsigned offset = 0;

public:
  VertexArray();
  ~VertexArray();

  void AddBuffer(const VertexBuffer &vbo, const VertexBufferLayout &layout);
  void AddInstanceBuffer(VertexBuffer &vbo, VertexBufferLayout &layout,
                         int divisor);

  void Bind() const;
  void UnBind() const;

  unsigned int GetID() const;
};
