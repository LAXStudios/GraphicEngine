#pragma once
#include "../IndexBuffer/IndexBuffer.h"
#include "../ShaderProgram/ShaderProgram.h"
#include "../VertexArray/VertexArray.h"

class VertexArray;
class IndexBuffer;
class ShaderProgram;

class RendererWrapper {
public:
  void clear() const;
  void draw(const VertexArray &va, const IndexBuffer &ib,
            const ShaderProgram &shader) const;
  void draw(const VertexArray &va, const ShaderProgram &shader,
            unsigned int count);
};
